#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr int   numPeakBins     = 2048;
    constexpr float chokeFadeMs     = 4.0f;
    constexpr float decayFloor      = 0.001f; // -60 dB = Ende der Decay-Phase

    inline float hermite (float xm1, float x0, float x1, float x2, float t)
    {
        const float c0 = x0;
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }
}

KickSamplerProcessor::KickSamplerProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    formatManager.registerBasicFormats();

    pStart     = apvts.getRawParameterValue (ParamIDs::start);
    pLength    = apvts.getRawParameterValue (ParamIDs::length);
    pTranspose = apvts.getRawParameterValue (ParamIDs::transpose);
    pAttack    = apvts.getRawParameterValue (ParamIDs::attack);
    pHold      = apvts.getRawParameterValue (ParamIDs::hold);
    pDecay     = apvts.getRawParameterValue (ParamIDs::decay);
    pVelocity  = apvts.getRawParameterValue (ParamIDs::velocity);
    pGain      = apvts.getRawParameterValue (ParamIDs::gain);
}

juce::AudioProcessorValueTreeState::ParameterLayout KickSamplerProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (float v, int) { return String (v * 100.0f, 1) + " %"; };
    auto ms  = [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " s" : String (v, 1) + " ms"; };

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::start, 1 }, "Start",
        NormalisableRange<float> (0.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::length, 1 }, "Length",
        NormalisableRange<float> (0.001f, 1.0f), 1.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::transpose, 1 }, "Transpose",
        NormalisableRange<float> (-48.0f, 48.0f, 0.01f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            return (v > 0 ? "+" : "") + String (v, 2) + " st";
        })));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::attack, 1 }, "Attack",
        NormalisableRange<float> (0.0f, 200.0f, 0.0f, 0.35f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (ms)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::hold, 1 }, "Hold",
        NormalisableRange<float> (0.0f, 2000.0f, 0.0f, 0.35f), 50.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (ms)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::decay, 1 }, "Decay",
        NormalisableRange<float> (5.0f, 10000.0f, 0.0f, 0.3f), 10000.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction (ms)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::velocity, 1 }, "Velocity",
        NormalisableRange<float> (0.0f, 1.0f), 0.5f,
        AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::gain, 1 }, "Gain",
        NormalisableRange<float> (-36.0f, 12.0f, 0.1f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int)
        {
            return String (v, 1) + " dB";
        })));

    return layout;
}

bool KickSamplerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void KickSamplerProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = sampleRate;
    gainSmoothed.reset (sampleRate, 0.02);
    gainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pGain->load()));
    for (auto& v : voices)
        v.active = false;
}

//==============================================================================
bool KickSamplerProcessor::isSupportedFile (const juce::File& file)
{
    return file.hasFileExtension ("wav;aif;aiff;flac;mp3;ogg;m4a;caf");
}

bool KickSamplerProcessor::loadSample (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    // Max. 30 Sekunden – fuer Kicks mehr als genug
    const auto maxLen = (juce::int64) (reader->sampleRate * 30.0);
    const int numSamples = (int) juce::jmin (reader->lengthInSamples, maxLen);
    const int numChannels = (int) juce::jlimit (1u, 2u, reader->numChannels);

    auto data = std::make_unique<SampleData>();
    data->buffer.setSize (numChannels, numSamples);
    reader->read (&data->buffer, 0, numSamples, 0, true, numChannels > 1);
    data->sampleRate = reader->sampleRate;
    data->name = file.getFileNameWithoutExtension();
    data->file = file;

    // Peaks berechnen
    WaveformPeaks newPeaks;
    newPeaks.mins.resize (numPeakBins);
    newPeaks.maxs.resize (numPeakBins);
    newPeaks.numSourceSamples = numSamples;
    newPeaks.sampleRate = reader->sampleRate;
    newPeaks.name = data->name;

    for (int b = 0; b < numPeakBins; ++b)
    {
        const int s0 = (int) ((juce::int64) b * numSamples / numPeakBins);
        const int s1 = juce::jmax (s0 + 1, (int) ((juce::int64) (b + 1) * numSamples / numPeakBins));
        float mn = 0.0f, mx = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto r = data->buffer.findMinMax (ch, s0, juce::jmin (s1, numSamples) - s0);
            mn = juce::jmin (mn, r.getStart());
            mx = juce::jmax (mx, r.getEnd());
        }
        newPeaks.mins[(size_t) b] = mn;
        newPeaks.maxs[(size_t) b] = mx;
    }

    // Austauschen; altes Sample ausserhalb des Locks freigeben
    std::unique_ptr<SampleData> old;
    {
        const juce::SpinLock::ScopedLockType sl (sampleLock);
        old = std::move (currentSample);
        currentSample = std::move (data);
        for (auto& v : voices)
            v.active = false;
    }
    old.reset();

    loadedNumSamples = numSamples;
    loadedSampleRate = reader->sampleRate;

    {
        const juce::ScopedLock sl (peaksLock);
        peaks = std::move (newPeaks);
        loadedFile = file;
    }

    sendChangeMessage();
    return true;
}

WaveformPeaks KickSamplerProcessor::getPeaks() const
{
    const juce::ScopedLock sl (peaksLock);
    return peaks;
}

double KickSamplerProcessor::getEffectiveLengthSeconds() const
{
    const int n = loadedNumSamples.load();
    if (n <= 0)
        return 0.0;

    const double start = pStart->load();
    const double len   = pLength->load();
    const double regionSamples = (1.0 - start) * len * n;
    const double ratio = std::pow (2.0, pTranspose->load() / 12.0);
    const double regionSec = regionSamples / loadedSampleRate.load() / ratio;
    const double envSec = (pAttack->load() + pHold->load() + pDecay->load()) / 1000.0;
    return juce::jmin (regionSec, envSec);
}

//==============================================================================
void KickSamplerProcessor::startVoice (int midiVelocity, const SampleData& s)
{
    const float chokeStep = 1.0f / (float) juce::jmax (1.0, currentSampleRate * chokeFadeMs / 1000.0);

    // Laufende Stimmen kurz ausblenden (Choke wie bei einer Drum-Machine)
    Voice* target = nullptr;
    for (auto& v : voices)
    {
        if (v.active)
            v.fadeStep = juce::jmax (v.fadeStep, chokeStep);
        else if (target == nullptr)
            target = &v;
    }

    if (target == nullptr)
    {
        // Stimme mit kleinstem Fade-Gain stehlen
        target = &voices[0];
        for (auto& v : voices)
            if (v.fadeGain < target->fadeGain)
                target = &v;
    }

    const int n = s.buffer.getNumSamples();
    const double start = (double) pStart->load() * n;
    const double len   = (double) pLength->load() * (n - start);

    const float vel = (float) midiVelocity / 127.0f;
    const float sens = pVelocity->load();

    auto& v = *target;
    v.active = true;
    v.position = start;
    v.endPosition = juce::jmin ((double) n - 1.0, start + len);
    v.increment = (s.sampleRate / currentSampleRate) * std::pow (2.0, pTranspose->load() / 12.0);
    v.velocityGain = (1.0f - sens) + sens * vel * vel;
    v.envStage = 0;
    v.envSamples = 0.0;
    v.envLevel = 0.0f;
    v.fadeGain = 1.0f;
    v.fadeStep = 0.0f;
}

void KickSamplerProcessor::renderVoices (juce::AudioBuffer<float>& out, int startSample, int numSamples,
                                         const SampleData& s)
{
    const int outChannels = out.getNumChannels();
    const int srcChannels = s.buffer.getNumChannels();
    const int n = s.buffer.getNumSamples();

    const double attackSamples = pAttack->load() * 0.001 * currentSampleRate;
    const double holdSamples   = pHold->load()   * 0.001 * currentSampleRate;
    const double decaySamples  = juce::jmax (1.0, pDecay->load() * 0.001 * currentSampleRate);
    const float  decayCoeff    = (float) std::exp (std::log (decayFloor) / decaySamples);

    for (int i = 0; i < numSamples; ++i)
    {
        const float g = gainSmoothed.getNextValue();
        float l = 0.0f, r = 0.0f;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            // Envelope
            switch (v.envStage)
            {
                case 0:
                    if (attackSamples < 1.0 || v.envSamples >= attackSamples)
                    {
                        v.envStage = 1; v.envSamples = 0.0; v.envLevel = 1.0f;
                    }
                    else
                    {
                        v.envLevel = (float) (v.envSamples / attackSamples);
                    }
                    break;
                case 1:
                    v.envLevel = 1.0f;
                    if (v.envSamples >= holdSamples) { v.envStage = 2; v.envSamples = 0.0; }
                    break;
                default:
                    v.envLevel *= decayCoeff;
                    break;
            }
            v.envSamples += 1.0;

            if (v.envStage == 2 && v.envLevel <= decayFloor)
            {
                v.active = false;
                continue;
            }

            // Sample lesen (Hermite)
            const int   idx  = (int) v.position;
            const float frac = (float) (v.position - idx);
            auto sampleAt = [&] (int ch, int k) -> float
            {
                return s.buffer.getSample (ch, juce::jlimit (0, n - 1, k));
            };

            const float amp = v.envLevel * v.velocityGain * v.fadeGain * g;
            const float sl = hermite (sampleAt (0, idx - 1), sampleAt (0, idx), sampleAt (0, idx + 1), sampleAt (0, idx + 2), frac);
            const float sr = srcChannels > 1
                ? hermite (sampleAt (1, idx - 1), sampleAt (1, idx), sampleAt (1, idx + 1), sampleAt (1, idx + 2), frac)
                : sl;

            l += sl * amp;
            r += sr * amp;

            v.position += v.increment;

            // Kurzer Fade am Ende des Bereichs gegen Klicks
            const double remaining = (v.endPosition - v.position) / v.increment;
            const double endFade = currentSampleRate * 0.002;
            if (remaining < endFade)
                v.fadeGain = juce::jmin (v.fadeGain, (float) juce::jmax (0.0, remaining / endFade));

            if (v.fadeStep > 0.0f)
                v.fadeGain -= v.fadeStep;

            if (v.position >= v.endPosition || v.fadeGain <= 0.0f)
                v.active = false;
        }

        if (outChannels == 1)
        {
            out.addSample (0, startSample + i, 0.5f * (l + r));
        }
        else
        {
            out.addSample (0, startSample + i, l);
            out.addSample (1, startSample + i, r);
        }
    }
}

void KickSamplerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Tempo und Taktart vom Host
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto bpm = pos->getBpm())
            {
                hostBpm = *bpm;
                hostTempoValid = true;
            }
            if (auto ts = pos->getTimeSignature())
            {
                timeSigNum = juce::jmax (1, ts->numerator);
                timeSigDen = juce::jmax (1, ts->denominator);
            }
        }
    }

    gainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (pGain->load()));

    const juce::SpinLock::ScopedTryLockType tl (sampleLock);
    if (! tl.isLocked() || currentSample == nullptr)
    {
        playPosition = -1.0f;
        return;
    }

    const auto& s = *currentSample;
    const int numSamples = buffer.getNumSamples();
    int pos = 0;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int when = juce::jlimit (0, numSamples, meta.samplePosition);

        if (when > pos)
        {
            renderVoices (buffer, pos, when - pos, s);
            pos = when;
        }

        if (msg.isNoteOn())
            startVoice (msg.getVelocity(), s);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            for (auto& v : voices)
                if (v.active)
                    v.fadeStep = 1.0f / (float) (currentSampleRate * chokeFadeMs / 1000.0);
    }

    if (pos < numSamples)
        renderVoices (buffer, pos, numSamples - pos, s);

    // Abspielposition der juengsten Stimme fuer die Anzeige
    float newest = -1.0f;
    double minEnv = 1e12;
    for (auto& v : voices)
    {
        if (v.active && v.fadeStep == 0.0f)
        {
            const double age = v.envStage * 1e9 + v.envSamples;
            if (age < minEnv)
            {
                minEnv = age;
                newest = (float) (v.position / s.buffer.getNumSamples());
            }
        }
    }
    playPosition = newest;
}

//==============================================================================
void KickSamplerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    juce::String path;
    {
        const juce::ScopedLock sl (peaksLock);
        path = loadedFile.getFullPathName();
    }
    state.setProperty ("samplePath", path, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void KickSamplerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid())
            return;

        const juce::String path = state.getProperty ("samplePath").toString();
        apvts.replaceState (state);

        if (path.isNotEmpty())
        {
            juce::File f (path);
            if (f.existsAsFile())
                loadSample (f);
        }
    }
}

juce::AudioProcessorEditor* KickSamplerProcessor::createEditor()
{
    return new KickSamplerEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KickSamplerProcessor();
}
