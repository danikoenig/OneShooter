#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>
#include <vector>

namespace ParamIDs
{
    inline constexpr const char* start     = "start";
    inline constexpr const char* length    = "length";
    inline constexpr const char* transpose = "transpose";
    inline constexpr const char* attack    = "attack";
    inline constexpr const char* hold      = "hold";
    inline constexpr const char* decay     = "decay";
    inline constexpr const char* velocity  = "velocity";
    inline constexpr const char* gain      = "gain";
}

// Geladenes Sample (unveraenderlich nach dem Laden)
struct SampleData
{
    juce::AudioBuffer<float> buffer;
    double sampleRate = 44100.0;
    juce::String name;
    juce::File file;
};

// Min/Max-Peaks fuer die Waveform-Anzeige
struct WaveformPeaks
{
    std::vector<float> mins, maxs;
    int numSourceSamples = 0;
    double sampleRate = 44100.0;
    juce::String name;
    bool isEmpty() const { return mins.empty(); }
};

class KickSamplerProcessor : public juce::AudioProcessor,
                             public juce::ChangeBroadcaster
{
public:
    KickSamplerProcessor();
    ~KickSamplerProcessor() override = default;

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

    // Sample laden (nicht vom Audio-Thread aufrufen)
    bool loadSample (const juce::File& file);
    static bool isSupportedFile (const juce::File& file);

    // Fuer den Editor
    WaveformPeaks getPeaks() const;
    double getHostBpm() const          { return hostBpm.load(); }
    bool   hasHostTempo() const        { return hostTempoValid.load(); }
    int    getTimeSigNumerator() const { return timeSigNum.load(); }
    int    getTimeSigDenominator() const { return timeSigDen.load(); }
    // Abspielposition im Sample (0..1) oder -1 wenn nichts spielt
    float  getPlayPosition() const     { return playPosition.load(); }

    // Hoerbare Laenge der Kick in Sekunden (Sample-Bereich, Pitch und Envelope)
    double getEffectiveLengthSeconds() const;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Voice
    {
        bool   active = false;
        double position = 0.0;     // Position im Sample (Samples)
        double endPosition = 0.0;
        double increment = 1.0;
        float  velocityGain = 1.0f;
        int    envStage = 0;        // 0 Attack, 1 Hold, 2 Decay
        double envSamples = 0.0;    // Samples seit Beginn der Stage
        float  envLevel = 0.0f;
        float  fadeGain = 1.0f;     // fuer Choke beim Retrigger
        float  fadeStep = 0.0f;
    };

    void startVoice (int midiVelocity, const SampleData& s);
    void renderVoices (juce::AudioBuffer<float>& out, int startSample, int numSamples, const SampleData& s);

    juce::AudioFormatManager formatManager;

    juce::SpinLock sampleLock;           // schuetzt currentSample
    std::unique_ptr<SampleData> currentSample;

    mutable juce::CriticalSection peaksLock;
    WaveformPeaks peaks;
    juce::File loadedFile;

    static constexpr int maxVoices = 4;
    Voice voices[maxVoices];

    double currentSampleRate = 44100.0;

    std::atomic<double> hostBpm { 120.0 };
    std::atomic<bool>   hostTempoValid { false };
    std::atomic<int>    timeSigNum { 4 }, timeSigDen { 4 };
    std::atomic<float>  playPosition { -1.0f };
    std::atomic<int>    loadedNumSamples { 0 };
    std::atomic<double> loadedSampleRate { 44100.0 };

    std::atomic<float>* pStart     = nullptr;
    std::atomic<float>* pLength    = nullptr;
    std::atomic<float>* pTranspose = nullptr;
    std::atomic<float>* pAttack    = nullptr;
    std::atomic<float>* pHold      = nullptr;
    std::atomic<float>* pDecay     = nullptr;
    std::atomic<float>* pVelocity  = nullptr;
    std::atomic<float>* pGain      = nullptr;

    juce::SmoothedValue<float> gainSmoothed;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickSamplerProcessor)
};
