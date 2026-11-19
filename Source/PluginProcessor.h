#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include <vector>
#include "SampleLibrary.h"

namespace ParamIDs
{
    // Sample
    inline constexpr const char* start      = "start";
    inline constexpr const char* length     = "length";
    inline constexpr const char* transpose  = "transpose";
    // Pitch-Envelope
    inline constexpr const char* pitchAmt   = "pitchAmt";
    inline constexpr const char* pitchDecay = "pitchDecay";
    // Amp-Envelope
    inline constexpr const char* attack     = "attack";
    inline constexpr const char* hold       = "hold";
    inline constexpr const char* decay      = "decay";
    inline constexpr const char* release    = "release";
    // Sub
    inline constexpr const char* subVol     = "subVol";
    inline constexpr const char* subTune    = "subTune";
    inline constexpr const char* subDecay   = "subDecay";
    // Clipper
    inline constexpr const char* clipOn     = "clipOn";
    inline constexpr const char* clipHard   = "clipHard";
    inline constexpr const char* clipDrive  = "clipDrive";
    // Output
    inline constexpr const char* velocity   = "velocity";
    inline constexpr const char* gain       = "gain";
    // Chromatisch spielen (Root C3 = MIDI 60)
    inline constexpr const char* chromatic  = "chromatic";
    // Gate: kuerzt den Hit tempo-synchron (IDs heissen aus Kompatibilitaet weiter duck*)
    inline constexpr const char* duckSync   = "duckSync";
    inline constexpr const char* duckFade   = "duckFade";
    inline constexpr const char* duckCurve  = "duckCurve";
}

// Release-Wert ab dem der Release aus ist (One-Shot)
inline constexpr float releaseOffValue = 5000.0f;

// Gate-Laengen; Index 0 = Off. Laenge in Viertelnoten (-1 = ein Takt)
inline const juce::StringArray duckChoices { "Off", "1/32", "1/16", "1/16.", "1/8", "1/8.", "1/4", "1/4.", "1/2", "1 bar" };
inline double duckQuarters (int index, double barQuarters)
{
    static const double q[] = { 0.0, 0.125, 0.25, 0.375, 0.5, 0.75, 1.0, 1.5, 2.0, -1.0 };
    if (index <= 0 || index > 9) return 0.0;
    return q[index] < 0.0 ? barQuarters : q[index];
}

struct SampleData
{
    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
    juce::String name;
};

struct WaveformPeaks
{
    std::vector<float> mins, maxs;
    int numSourceSamples = 0;
    double sampleRate = 44100.0;
    juce::String name;
    bool isEmpty() const { return mins.empty(); }
};

// Schnappschuss der Parameter fuer Berechnungen im GUI
struct HitTimeline
{
    double dt = 0.0005;                 // Schrittweite in Sekunden
    std::vector<double> sourcePos;      // Position im Sample (Bruchteil 0..1) pro Schritt
    std::vector<float>  ampEnv;         // Amp-Envelope pro Schritt
    double sampleSeconds = 0.0;         // hoerbare Laenge des Sample-Teils
    double subSeconds = 0.0;            // Laenge des Sub-Teils
    double totalSeconds() const { return juce::jmax (sampleSeconds, subSeconds); }
};

class OneShooterProcessor : public juce::AudioProcessor,
                             public juce::ChangeBroadcaster
{
public:
    OneShooterProcessor();
    ~OneShooterProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    bool loadSample (const juce::File& file);
    static bool isSupportedFile (const juce::File& file);

    // Fuer den Editor
    WaveformPeaks getPeaks() const;
    double getHostBpm() const             { return hostBpm.load(); }
    bool   hasHostTempo() const           { return hostTempoValid.load(); }
    int    getTimeSigNumerator() const    { return timeSigNum.load(); }
    int    getTimeSigDenominator() const  { return timeSigDen.load(); }
    float  getPlayPosition() const        { return playPosition.load(); }
    float  getClipReductionDb() const     { return clipReductionDb.load(); }
    float  getOutputPeak() const          { return outputPeak.load(); }

    // Vorhoeren aus der Library (spielt das Sample einmal ab)
    void   triggerPreview()               { previewRequested = true; }
    SampleLibrary& getLibrary()           { return *library; }
    // Library-Seitenleiste sichtbar (wird im Projekt gespeichert)
    bool   showLibrary = true;
    juce::File getLoadedFile() const      { const juce::ScopedLock sl (peaksLock); return loadedFile; }

    // Simuliert den Kick-Verlauf (Pitch-Envelope, Amp-Envelope, Sub) mit den aktuellen Parametern
    HitTimeline computeTimeline (double dtSec = 0.0005) const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Voice
    {
        bool   active = false;
        int    note = -1;
        float  noteOffset = 0.0f;   // Halbtoene relativ zu C3 (Chromatic-Modus)
        // Sample-Teil
        bool   sampleActive = false;
        double position = 0.0, endPosition = 0.0, baseIncrement = 1.0;
        int    envStage = 0;
        double envSamples = 0.0;
        float  envLevel = 0.0f;
        // Pitch-Envelope
        float  pitchEnv = 1.0f;
        // Sub
        bool   subActive = false;
        double subPhase = 0.0;
        float  subLevel = 1.0f;
        float  subAttack = 0.0f;
        // Gemeinsam
        float  velocityGain = 1.0f;
        bool   releasing = false;
        float  releaseGain = 1.0f;
        float  releaseCoeff = 1.0f;
        float  fadeGain = 1.0f;     // Choke
        float  fadeStep = 0.0f;
        juce::uint32 age = 0;
        double ageSamples = 0.0;     // Samples seit Note-On (fuer Gate)
    };

    void startVoice (int note, int midiVelocity, const SampleData* s);
    void releaseVoices (int note);
    void renderVoices (juce::AudioBuffer<float>& out, int startSample, int numSamples, const SampleData* s);
    void processClipper (juce::AudioBuffer<float>& buffer);

    juce::AudioFormatManager formatManager;

    juce::SpinLock sampleLock;
    std::unique_ptr<SampleData> currentSample;

    mutable juce::CriticalSection peaksLock;
    WaveformPeaks peaks;
    juce::File loadedFile;

    static constexpr int maxVoices = 4;
    Voice voices[maxVoices];
    juce::uint32 voiceCounter = 0;

    double currentSampleRate = 44100.0;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::SmoothedValue<float> clipDriveSmoothed, gainSmoothed;

    std::atomic<double> hostBpm { 120.0 };
    std::atomic<bool>   hostTempoValid { false };
    std::atomic<int>    timeSigNum { 4 }, timeSigDen { 4 };
    std::atomic<float>  playPosition { -1.0f };
    std::atomic<float>  clipReductionDb { 0.0f };
    std::atomic<float>  outputPeak { 0.0f };
    std::atomic<bool>   previewRequested { false };
    juce::SharedResourcePointer<SampleLibrary> library;
    std::atomic<int>    loadedNumSamples { 0 };
    std::atomic<double> loadedSampleRate { 44100.0 };

    std::atomic<float>* p[21] {};
    enum P { pStart, pLength, pTranspose, pPitchAmt, pPitchDecay, pAttack, pHold, pDecay, pRelease,
             pSubVol, pSubTune, pSubDecay, pClipOn, pClipHard, pClipDrive, pVelocity, pGain, pChromatic, pDuckSync, pDuckFade, pDuckCurve };
    float param (P i) const { return p[i]->load(); }
    double duckLengthSeconds() const;   // 0 = aus

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneShooterProcessor)
};
