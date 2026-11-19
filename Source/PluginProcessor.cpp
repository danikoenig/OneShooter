#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    constexpr int   numPeakBins  = 2048;
    constexpr float chokeFadeMs  = 4.0f;
    constexpr float envFloor     = 0.001f;   // -60 dB
    constexpr float subAttackMs  = 1.0f;
    constexpr int   rootNote     = 60;      // C3: spielt das Sample in Originalhoehe

    inline float hermite (float xm1, float x0, float x1, float x2, float t)
    {
        const float c0 = x0;
        const float c1 = 0.5f * (x1 - xm1);
        const float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        const float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * t + c2) * t + c1) * t + c0;
    }

    inline float decayCoeff (double timeMs, double sampleRate)
    {
        const double n = juce::jmax (1.0, timeMs * 0.001 * sampleRate);
        return (float) std::exp (std::log ((double) envFloor) / n);
    }

    // Soft-Clip mit Knie ab 0.5, naehert sich 1.0 an
    inline float softClip (float x)
    {
        const float a = std::abs (x);
        if (a <= 0.5f)
            return x;
        const float y = 0.5f + 0.5f * std::tanh ((a - 0.5f) * 2.0f);
        return std::copysign (y, x);
    }

    inline float hardClip (float x) { return juce::jlimit (-1.0f, 1.0f, x); }

    juce::String noteName (float hz)
    {
        const int midi = (int) std::round (69.0 + 12.0 * std::log2 (hz / 440.0));
        return juce::MidiMessage::getMidiNoteName (midi, true, true, 3);
    }
}

OneShooterProcessor::OneShooterProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    formatManager.registerBasicFormats();

    const char* ids[] = { ParamIDs::start, ParamIDs::length, ParamIDs::transpose, ParamIDs::pitchAmt,
                          ParamIDs::pitchDecay, ParamIDs::attack, ParamIDs::hold, ParamIDs::decay,
                          ParamIDs::release, ParamIDs::subVol, ParamIDs::subTune, ParamIDs::subDecay,
                          ParamIDs::clipOn, ParamIDs::clipHard, ParamIDs::clipDrive, ParamIDs::velocity,
                          ParamIDs::gain, ParamIDs::chromatic, ParamIDs::duckSync, ParamIDs::duckFade,
                          ParamIDs::duckCurve, ParamIDs::poly, ParamIDs::tone, ParamIDs::lowCut, ParamIDs::highCut,
                          ParamIDs::clipAuto, ParamIDs::limiter };
    for (int i = 0; i < 27; ++i)
    {
        p[i] = apvts.getRawParameterValue (ids[i]);
        jassert (p[i] != nullptr);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout OneShooterProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = [] (float v, int) { return String (v * 100.0f, 1) + " %"; };
    auto ms  = [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 2) + " s" : (v < 10.0f ? String (v, 1) : String (roundToInt (v))) + " ms"; };
    auto st  = [] (float v, int) { return (v > 0.0f ? "+" : "") + String (v, 1) + " st"; };
    auto db  = [] (float v, int) { return String (v, 1) + " dB"; };

    auto addFloat = [&] (const char* id, const char* name, NormalisableRange<float> r, float def,
                         std::function<String (float, int)> fn)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                        AudioParameterFloatAttributes().withStringFromValueFunction (std::move (fn))));
    };

    addFloat (ParamIDs::start,      "Start",       { 0.0f, 1.0f }, 0.0f, pct);
    addFloat (ParamIDs::length,     "Length",      { 0.001f, 1.0f }, 1.0f, pct);
    addFloat (ParamIDs::transpose,  "Transpose",   { -48.0f, 48.0f, 0.01f }, 0.0f, st);

    addFloat (ParamIDs::pitchAmt,   "Pitch Amount", { -48.0f, 48.0f, 0.01f }, 0.0f, st);
    addFloat (ParamIDs::pitchDecay, "Pitch Decay",  { 1.0f, 2000.0f, 0.0f, 0.3f }, 60.0f, ms);

    addFloat (ParamIDs::attack,     "Attack",      { 0.0f, 200.0f, 0.0f, 0.35f }, 0.0f, ms);
    addFloat (ParamIDs::hold,       "Hold",        { 0.0f, 2000.0f, 0.0f, 0.35f }, 50.0f, ms);
    addFloat (ParamIDs::decay,      "Decay",       { 5.0f, 10000.0f, 0.0f, 0.3f }, 10000.0f,
              [] (float v, int) { return v >= 9999.0f ? String ("Max") : (v >= 1000.0f ? String (v / 1000.0f, 2) + " s" : String (roundToInt (v)) + " ms"); });
    addFloat (ParamIDs::release,    "Release",     { 5.0f, releaseOffValue, 0.0f, 0.35f }, releaseOffValue,
              [] (float v, int) { return v >= releaseOffValue - 0.5f ? String ("Off") : (v >= 1000.0f ? String (v / 1000.0f, 2) + " s" : String (roundToInt (v)) + " ms"); });

    addFloat (ParamIDs::subVol,     "Sub Volume",  { 0.0f, 1.0f }, 0.0f,
              [] (float v, int) { return v <= 0.0001f ? String ("Off") : String (Decibels::gainToDecibels (v), 1) + " dB"; });
    addFloat (ParamIDs::subTune,    "Sub Tune",    { 25.0f, 120.0f, 0.01f, 0.5f }, 50.0f,
              [] (float v, int) { return String (v, 1) + " Hz " + noteName (v); });
    addFloat (ParamIDs::subDecay,   "Sub Decay",   { 20.0f, 5000.0f, 0.0f, 0.35f }, 400.0f, ms);

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::clipOn, 1 },   "Clip On",   false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::clipHard, 1 }, "Clip Hard", false));
    addFloat (ParamIDs::clipDrive,  "Clip Drive",  { 0.0f, 24.0f, 0.1f }, 0.0f,
              [] (float v, int) { return "+" + String (v, 1) + " dB"; });

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::chromatic, 1 }, "Chromatic", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::duckSync, 1 }, "Gate Length", duckChoices, 0));
    addFloat (ParamIDs::duckFade,   "Gate Fade",   { 0.0f, 1.0f }, 0.3f, pct);
    addFloat (ParamIDs::duckCurve,  "Gate Curve",  { 0.0f, 1.0f }, 0.5f, pct);
    // v1.5
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::poly, 1 }, "Poly", false));
    addFloat (ParamIDs::tone, "Tone", { -1.0f, 1.0f }, 0.0f,
              [] (float v, int) { return std::abs (v) < 0.005f ? String ("0") : (v > 0 ? "+" : "") + String (roundToInt (v * 100.0f)); });
    auto hz = [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; };
    addFloat (ParamIDs::lowCut,  "Low Cut",  { 20.0f, 20000.0f, 0.0f, 0.25f }, 20.0f,
              [hz] (float v, int i) { return v <= 20.5f ? String ("Off") : hz (v, i); });
    addFloat (ParamIDs::highCut, "High Cut", { 20.0f, 20000.0f, 0.0f, 0.25f }, 20000.0f,
              [hz] (float v, int i) { return v >= 19999.0f ? String ("Off") : hz (v, i); });
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::clipAuto, 1 }, "Clip Auto Gain", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::limiter, 1 },  "Limiter", true));
    addFloat (ParamIDs::velocity,   "Velocity",    { 0.0f, 1.0f }, 0.5f, pct);
    addFloat (ParamIDs::gain,       "Gain",        { -36.0f, 12.0f, 0.1f }, 0.0f, db);

    return layout;
}

bool OneShooterProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OneShooterProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    gainSmoothed.reset (sampleRate, 0.02);
    gainSmoothed.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (param (pGain)));

    // 4x Oversampling fuer den Clipper
    const auto numCh = (size_t) juce::jmax (1, getTotalNumOutputChannels());
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        numCh, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampler->initProcessing ((size_t) samplesPerBlock);
    setLatencySamples ((int) std::round (oversampler->getLatencyInSamples()));

    clipDriveSmoothed.reset (sampleRate * 4.0, 0.02);
    clipDriveSmoothed.setCurrentAndTargetValue (1.0f);

    for (auto& v : voices)
        v.active = false;
}

//==============================================================================
bool OneShooterProcessor::isSupportedFile (const juce::File& file)
{
    return file.hasFileExtension ("wav;aif;aiff;flac;mp3;ogg;m4a;caf");
}

bool OneShooterProcessor::loadSample (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0)
        return false;

    const auto maxLen = (juce::int64) (reader->sampleRate * 30.0);
    const int numSamples = (int) juce::jmin (reader->lengthInSamples, maxLen);
    const int numChannels = (int) juce::jlimit (1u, 2u, reader->numChannels);

    auto data = std::make_unique<SampleData>();
    data->buffer.setSize (numChannels, numSamples);
    reader->read (&data->buffer, 0, numSamples, 0, true, numChannels > 1);
    data->sampleRate = reader->sampleRate;
    data->name = file.getFileNameWithoutExtension();

    WaveformPeaks newPeaks;
    newPeaks.mins.resize (numPeakBins);
    newPeaks.maxs.resize (numPeakBins);
    newPeaks.numSourceSamples = numSamples;
    newPeaks.sampleRate = reader->sampleRate;
    newPeaks.name = data->name;

    for (int b = 0; b < numPeakBins; ++b)
    {
        const int s0 = (int) ((juce::int64) b * numSamples / numPeakBins);
        const int s1 = juce::jmin (numSamples, juce::jmax (s0 + 1, (int) ((juce::int64) (b + 1) * numSamples / numPeakBins)));
        float mn = 0.0f, mx = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto r = data->buffer.findMinMax (ch, s0, s1 - s0);
            mn = juce::jmin (mn, r.getStart());
            mx = juce::jmax (mx, r.getEnd());
        }
        newPeaks.mins[(size_t) b] = mn;
        newPeaks.maxs[(size_t) b] = mx;
    }

    // Grundton erkennen (fuer Chromatic), bevor das Sample live geht (nicht unter dem Audio-Lock)
    // Tonart im Dateinamen hat Vorrang (Packs beschriften sie bewusst; bei Kicks mit Pitch-Abfall
    // und bei Akkorden ist die Erkennung unsicher). Sonst entscheidet die Tonerkennung.
    PitchResult det;
    bool fromName = false;
    if (const int nameRoot = rootFromFileName (data->name); nameRoot >= 0)
    {
        det.tonal = true;
        det.midi = (float) nameRoot;
        det.confidence = 1.0f;
        fromName = true;
    }
    else
        det = PitchDetector::detect (data->buffer, data->sampleRate);

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
        detected = det;
    }
    rootFromName = fromName;
    rootOverride = rootAuto;
    updateEffectiveRoot();

    sendChangeMessage();
    return true;
}

WaveformPeaks OneShooterProcessor::getPeaks() const
{
    const juce::ScopedLock sl (peaksLock);
    return peaks;
}

int OneShooterProcessor::rootFromFileName (const juce::String& name)
{
    // Tokens wie "Dm", "G", "C#m", "Ebmaj", "F#min" (nicht das erste Wort, das ist meist der Pack-Name)
    auto tokens = juce::StringArray::fromTokens (name, "_- .()[]", "");
    tokens.removeEmptyStrings();
    for (int i = tokens.size() - 1; i >= 1; --i)
    {
        const auto t = tokens[i];
        if (t.length() >= 2 && t.length() <= 4 && t.containsAnyOf ("0123456789"))
            if (const int withOct = PitchDetector::parseNoteName (t); withOct >= 0)   // z. B. "C3", "F#2"
                return withOct;
        const auto letter = juce::CharacterFunctions::toUpperCase (t[0]);
        if (letter < 'A' || letter > 'G')
            continue;
        auto rest = t.substring (1);
        int acc = 0;
        if (rest.startsWith ("#") || rest.startsWith (juce::String::fromUTF8 ("\xe2\x99\xaf"))) { acc = 1; rest = rest.fromFirstOccurrenceOf (rest.substring (0, 1), false, false); }
        else if (rest.startsWith ("b") && ! rest.startsWithIgnoreCase ("bass")) { acc = -1; rest = rest.substring (1); }
        const auto r = rest.toLowerCase();
        const bool isKey = r.isEmpty() || r == "m" || r == "min" || r == "minor" || r == "maj" || r == "major" || r == "dur" || r == "moll";
        if (! isKey || (t[0] != letter && r.isEmpty()))   // einzelnes kleines "a" o.ae. ignorieren
            continue;
        static const int pcs[] = { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
        const int pc = (pcs[letter - 'A'] + acc + 12) % 12;
        return 60 + pc;   // Oktave 3 (C3 = 60)
    }
    return -1;
}

void OneShooterProcessor::setRootOverride (int midiOrMode)
{
    rootOverride = midiOrMode;
    updateEffectiveRoot();
    sendChangeMessage();
}

void OneShooterProcessor::updateEffectiveRoot()
{
    const int ov = rootOverride.load();
    float root = noPitch;
    if (ov >= 0)
        root = (float) ov;
    else if (ov == rootAuto)
    {
        const juce::ScopedLock sl (peaksLock);
        if (detected.tonal)
        {
            // Taste fuer die Originalhoehe in Oktave 3 (C3..B3) legen, Cents bleiben erhalten.
            // So spielt man auch tiefe Kicks/Toms bequem in der Mitte der Tastatur.
            const float pc = std::fmod (std::fmod (detected.midi, 12.0f) + 12.0f, 12.0f);
            root = 60.0f + pc;
            if (root >= 71.5f) root -= 12.0f;
        }
    }
    effectiveRoot = root;
}

OneShooterProcessor::Svf OneShooterProcessor::makeSvf (float fc, double sr)
{
    Svf f;
    f.on = true;
    const float g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (10.0f, (float) (sr * 0.45), fc) / (float) sr);
    f.k = 1.41421f;   // Q 0.707
    f.a1 = 1.0f / (1.0f + g * (g + f.k));
    f.a2 = g * f.a1;
    f.a3 = g * f.a2;
    return f;
}

void OneShooterProcessor::processToneCut (juce::AudioBuffer<float>& buffer)
{
    const double sr = currentSampleRate;
    const float tone = param (pTone);
    const bool toneOn = std::abs (tone) > 0.005f;
    const float toneCoeff = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * 900.0f / (float) sr);
    const float toneLow = 1.0f - 0.8f * tone, toneHigh = 1.0f + 0.8f * tone;
    Svf hp, lp;
    if (param (pLowCut) > 20.5f)     hp = makeSvf (param (pLowCut), sr);
    if (param (pHighCut) < 19999.0f) lp = makeSvf (param (pHighCut), sr);

    if (! toneOn && ! hp.on && ! lp.on)
    {
        for (int ch = 0; ch < 2; ++ch) { toneLp[ch] = hp1[ch] = hp2[ch] = lp1[ch] = lp2[ch] = 0.0f; }
        return;
    }

    auto svf = [] (const Svf& f, float x, float& ic1, float& ic2, bool high)
    {
        const float v3 = x - ic2;
        const float v1 = f.a1 * ic1 + f.a2 * v3;
        const float v2 = ic2 + f.a2 * ic1 + f.a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return high ? x - f.k * v1 - v2 : v2;
    };

    const int numCh = juce::jmin (2, buffer.getNumChannels());
    for (int ch = 0; ch < numCh; ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = d[i];
            if (toneOn)
            {
                toneLp[ch] += toneCoeff * (x - toneLp[ch]);
                x = toneLp[ch] * toneLow + (x - toneLp[ch]) * toneHigh;
            }
            if (hp.on) x = svf (hp, x, hp1[ch], hp2[ch], true);
            if (lp.on) x = svf (lp, x, lp1[ch], lp2[ch], false);
            d[i] = x;
        }
    }
}

void OneShooterProcessor::processLimiter (juce::AudioBuffer<float>& buffer)
{
    if (param (pLimiter) < 0.5f)
    {
        limGain = 1.0f;
        limitReductionDb = limitReductionDb.load() * 0.85f;
        return;
    }

    // Peak-Limiter, Ceiling -1 dB: sofortiger Attack, 80 ms Release, danach harte Grenze
    const float ceiling = juce::Decibels::decibelsToGain (-1.0f);
    const float rel = std::exp (-1.0f / (0.08f * (float) currentSampleRate));
    const int numCh = buffer.getNumChannels();
    float minGain = 1.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = juce::jmax (peak, std::abs (buffer.getSample (ch, i)));
        const float target = peak * limGain > ceiling ? ceiling / juce::jmax (1e-9f, peak) : 1.0f;
        limGain = target < limGain ? target : target + (limGain - target) * rel;
        minGain = juce::jmin (minGain, limGain);
        for (int ch = 0; ch < numCh; ++ch)
            buffer.setSample (ch, i, juce::jlimit (-ceiling, ceiling, buffer.getSample (ch, i) * limGain));
    }

    const float red = -juce::Decibels::gainToDecibels (minGain);
    const float prev = limitReductionDb.load();
    limitReductionDb = red > prev ? red : prev * 0.85f;
}

double OneShooterProcessor::duckLengthSeconds() const
{
    const double barQ = timeSigNum.load() * 4.0 / juce::jmax (1, timeSigDen.load());
    const double q = duckQuarters ((int) std::round (param (pDuckSync)), barQ);
    return q > 0.0 ? q * 60.0 / juce::jmax (20.0, hostBpm.load()) : 0.0;
}

// Gate-Gain zum Zeitpunkt t (Sekunden seit Note-On)
static float duckGainAt (double t, double len, float fade, float curve)
{
    if (len <= 0.0)  return 1.0f;
    if (t >= len)    return 0.0f;
    const double fadeLen = juce::jmax (0.001, (double) fade * len);
    const double fadeStart = len - fadeLen;
    if (t <= fadeStart) return 1.0f;
    const double x = (t - fadeStart) / fadeLen;               // 0..1
    return (float) std::pow (1.0 - x, 1.0 + (double) curve * 4.0);
}

//==============================================================================
HitTimeline OneShooterProcessor::computeTimeline (double dtSec) const
{
    HitTimeline tl;
    tl.dt = dtSec;

    const int n = loadedNumSamples.load();
    const double fileSr = loadedSampleRate.load();

    const double duckLen = duckLengthSeconds();
    const float duckFade = param (pDuckFade), duckCurve = param (pDuckCurve);

    // Sub-Laenge
    if (param (pSubVol) > 0.0001f)
        tl.subSeconds = param (pSubDecay) / 1000.0;
    if (duckLen > 0.0)
        tl.subSeconds = juce::jmin (tl.subSeconds, duckLen);

    if (n <= 0)
        return tl;

    const double start = param (pStart) * (double) n;
    const double end   = juce::jmin ((double) n, start + param (pLength) * ((double) n - start));
    const double transpose = param (pTranspose);
    const double amt = param (pPitchAmt);
    const double pitchK = std::log ((double) envFloor) / juce::jmax (0.001, param (pPitchDecay) / 1000.0);
    const double att = param (pAttack) / 1000.0, hold = param (pHold) / 1000.0, dec = param (pDecay) / 1000.0;
    const double decK = std::log ((double) envFloor) / juce::jmax (0.001, dec);

    double pos = start;
    double t = 0.0;
    bool sampleDone = false;
    const int maxSteps = (int) (30.0 / dtSec);

    // bis Dateiende weiterlaufen (fuers Raster), Laenge separat bestimmen
    for (int i = 0; i < maxSteps && pos < (double) n; ++i)
    {
        float env;
        if (t < att)               env = (float) (t / juce::jmax (1e-9, att));
        else if (t < att + hold)   env = 1.0f;
        else                       env = (float) std::exp (decK * (t - att - hold));
        env *= duckGainAt (t, duckLen, duckFade, duckCurve);

        tl.sourcePos.push_back (pos / n);
        tl.ampEnv.push_back (env);

        if (! sampleDone && (pos >= end || (t > att + hold && env <= envFloor) || (duckLen > 0.0 && t >= duckLen)))
        {
            tl.sampleSeconds = t;
            sampleDone = true;
        }

        const double pe = std::exp (pitchK * t);
        const double semis = transpose + amt * (pe > envFloor ? pe : 0.0);
        pos += fileSr * dtSec * std::exp2 (semis / 12.0);
        t += dtSec;
    }

    if (! sampleDone)
        tl.sampleSeconds = t;

    return tl;
}

//==============================================================================
void OneShooterProcessor::startVoice (int note, int midiVelocity, const SampleData* s)
{
    const float chokeStep = 1.0f / (float) juce::jmax (1.0, currentSampleRate * chokeFadeMs / 1000.0);

    // Mono: jeder Hit wuergt die vorherigen ab. Poly: nur dieselbe Note (und Vorhoeren) wird abgewuergt.
    const bool poly = param (pPoly) > 0.5f && note >= 0;
    Voice* target = nullptr;
    for (auto& v : voices)
    {
        if (v.active)
        {
            if (! poly || v.note == note || v.note < 0)
                v.fadeStep = juce::jmax (v.fadeStep, chokeStep);
        }
        else if (target == nullptr)
            target = &v;
    }

    if (target == nullptr)
    {
        target = &voices[0];
        for (auto& v : voices)
            if (v.age < target->age)
                target = &v;
    }

    const float vel = (float) midiVelocity / 127.0f;
    const float sens = param (pVelocity);

    auto& v = *target;
    v = Voice();
    v.active = true;
    v.note = note;
    v.age = ++voiceCounter;
    v.velocityGain = (1.0f - sens) + sens * vel * vel;
    if (param (pChromatic) > 0.5f && note >= 0)
    {
        const float root = effectiveRoot.load();
        v.noteOffset = (float) note - (root > noPitch + 1.0f ? root : (float) rootNote);
    }
    else
        v.noteOffset = 0.0f;

    if (s != nullptr)
    {
        const int n = s->buffer.getNumSamples();
        const double start = (double) param (pStart) * n;
        const double len   = (double) param (pLength) * (n - start);
        v.sampleActive = true;
        v.position = start;
        v.endPosition = juce::jmin ((double) n - 1.0, start + len);
        v.baseIncrement = s->sampleRate / currentSampleRate;
    }

    v.subActive = param (pSubVol) > 0.0001f;
    v.subPhase = 0.0;
    v.subLevel = 1.0f;
    v.subAttack = 0.0f;

    if (! v.sampleActive && ! v.subActive)
        v.active = false;
}

void OneShooterProcessor::releaseVoices (int note)
{
    const float rel = param (pRelease);
    if (rel >= releaseOffValue - 0.5f)
        return; // One-Shot

    const float coeff = decayCoeff (rel, currentSampleRate);
    for (auto& v : voices)
        if (v.active && v.note == note && ! v.releasing)
        {
            v.releasing = true;
            v.releaseCoeff = coeff;
        }
}

void OneShooterProcessor::renderVoices (juce::AudioBuffer<float>& out, int startSample, int numSamples,
                                         const SampleData* s)
{
    const int outChannels = out.getNumChannels();
    const int srcChannels = s != nullptr ? s->buffer.getNumChannels() : 0;
    const int n = s != nullptr ? s->buffer.getNumSamples() : 0;
    const double sr = currentSampleRate;

    const double attackSamples = param (pAttack) * 0.001 * sr;
    const double holdSamples   = param (pHold)   * 0.001 * sr;
    const float  ampCoeff      = decayCoeff (param (pDecay), sr);
    const float  pitchCoeff    = decayCoeff (param (pPitchDecay), sr);
    const float  subCoeff      = decayCoeff (param (pSubDecay), sr);
    const float  transpose     = param (pTranspose);
    const float  pitchAmt      = param (pPitchAmt);
    const float  subVol        = param (pSubVol);
    const double subFreq       = param (pSubTune);
    const float  subAttackStep = 1.0f / (float) juce::jmax (1.0, sr * subAttackMs / 1000.0);
    const double endFade       = sr * 0.002;
    const double duckLen       = duckLengthSeconds();
    const float  duckFade      = param (pDuckFade);
    const float  duckCurve     = param (pDuckCurve);

    for (int i = 0; i < numSamples; ++i)
    {
        float l = 0.0f, r = 0.0f;

        for (auto& v : voices)
        {
            if (! v.active)
                continue;

            const float pe = v.pitchEnv > envFloor ? v.pitchEnv : 0.0f;
            v.pitchEnv *= pitchCoeff;
            const float duck = duckGainAt (v.ageSamples / sr, duckLen, duckFade, duckCurve);
            v.ageSamples += 1.0;
            if (duckLen > 0.0 && v.ageSamples / sr >= duckLen)
            {
                v.active = false;   // Gate-Ende erreicht
                continue;
            }
            const float common = v.velocityGain * v.fadeGain * v.releaseGain * duck;

            // ---- Sample-Teil
            if (v.sampleActive && s == nullptr)
                v.sampleActive = false;

            if (v.sampleActive)
            {
                switch (v.envStage)
                {
                    case 0:
                        if (attackSamples < 1.0 || v.envSamples >= attackSamples)
                        { v.envStage = 1; v.envSamples = 0.0; v.envLevel = 1.0f; }
                        else
                            v.envLevel = (float) (v.envSamples / attackSamples);
                        break;
                    case 1:
                        v.envLevel = 1.0f;
                        if (v.envSamples >= holdSamples) { v.envStage = 2; v.envSamples = 0.0; }
                        break;
                    default:
                        v.envLevel *= ampCoeff;
                        break;
                }
                v.envSamples += 1.0;

                const int   idx  = (int) v.position;
                const float frac = (float) (v.position - idx);
                auto at = [&] (int ch, int k) { return s->buffer.getSample (ch, juce::jlimit (0, n - 1, k)); };

                const double remaining = v.endPosition - v.position;
                const double inc = v.baseIncrement * std::exp2 ((transpose + v.noteOffset + pitchAmt * pe) / 12.0f);
                const float  endGain = (float) juce::jlimit (0.0, 1.0, (remaining / inc) / endFade);

                const float amp = v.envLevel * endGain * common;
                const float sl = hermite (at (0, idx - 1), at (0, idx), at (0, idx + 1), at (0, idx + 2), frac);
                const float sr2 = srcChannels > 1
                    ? hermite (at (1, idx - 1), at (1, idx), at (1, idx + 1), at (1, idx + 2), frac) : sl;

                l += sl * amp;
                r += sr2 * amp;

                v.position += inc;
                if (v.position >= v.endPosition || (v.envStage == 2 && v.envLevel <= envFloor))
                    v.sampleActive = false;
            }

            // ---- Sub-Oszillator (folgt der Pitch-Envelope)
            if (v.subActive)
            {
                v.subAttack = juce::jmin (1.0f, v.subAttack + subAttackStep);
                const float sub = (float) std::sin (juce::MathConstants<double>::twoPi * v.subPhase)
                                  * v.subLevel * v.subAttack * subVol * common;
                l += sub;
                r += sub;

                v.subPhase += subFreq * std::exp2 ((v.noteOffset + pitchAmt * pe) / 12.0f) / sr;
                if (v.subPhase >= 1.0) v.subPhase -= 1.0;
                v.subLevel *= subCoeff;
                if (v.subLevel <= envFloor)
                    v.subActive = false;
            }

            if (v.releasing)
                v.releaseGain *= v.releaseCoeff;
            if (v.fadeStep > 0.0f)
                v.fadeGain -= v.fadeStep;

            if ((! v.sampleActive && ! v.subActive) || v.fadeGain <= 0.0f || v.releaseGain <= envFloor)
                v.active = false;
        }

        if (outChannels == 1)
            out.addSample (0, startSample + i, 0.5f * (l + r));
        else
        {
            out.addSample (0, startSample + i, l);
            out.addSample (1, startSample + i, r);
        }
    }
}

void OneShooterProcessor::processClipper (juce::AudioBuffer<float>& buffer)
{
    const bool on = param (pClipOn) > 0.5f;
    const bool hard = param (pClipHard) > 0.5f;
    const bool autoGain = param (pClipAuto) > 0.5f;
    clipDriveSmoothed.setTargetValue (on ? juce::Decibels::decibelsToGain (param (pClipDrive)) : 1.0f);

    // Pegel vor dem Clipper (fuer Auto Gain)
    double inSum = 0.0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            inSum += (double) buffer.getSample (ch, i) * buffer.getSample (ch, i);

    juce::dsp::AudioBlock<float> block (buffer);
    auto up = oversampler->processSamplesUp (block);

    float maxReduction = 0.0f;
    const auto numCh = up.getNumChannels();
    const auto numS  = up.getNumSamples();

    for (size_t i = 0; i < numS; ++i)
    {
        const float drive = clipDriveSmoothed.getNextValue();
        for (size_t ch = 0; ch < numCh; ++ch)
        {
            const float x = up.getSample ((int) ch, (int) i) * drive;
            float y = x;
            if (on)
            {
                y = hard ? hardClip (x) : softClip (x);
                const float ax = std::abs (x), ay = std::abs (y);
                if (ax > 1e-6f && ay < ax)
                    maxReduction = juce::jmax (maxReduction, juce::Decibels::gainToDecibels (ax / juce::jmax (1e-6f, ay)));
            }
            up.setSample ((int) ch, (int) i, y);
        }
    }

    oversampler->processSamplesDown (block);

    // Auto Gain: Lautheit nach dem Clipper an die vorher angleichen (gleitend ~300 ms),
    // so hoert man die Saettigung, nicht einfach mehr Pegel
    {
        double outSum = 0.0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                outSum += (double) buffer.getSample (ch, i) * buffer.getSample (ch, i);
        const float n = (float) juce::jmax (1, buffer.getNumSamples() * buffer.getNumChannels());
        const float a = 1.0f - std::exp (-(float) buffer.getNumSamples() / (0.3f * (float) currentSampleRate));
        if (inSum / n > 1e-8)
        {
            clipInPow  += a * ((float) (inSum / n)  - clipInPow);
            clipOutPow += a * ((float) (outSum / n) - clipOutPow);
        }
        const float target = (autoGain && on && clipOutPow > 1e-10f)
            ? juce::jlimit (0.06f, 2.0f, std::sqrt (clipInPow / clipOutPow)) : 1.0f;
        const float startG = clipAutoGain;
        const int numS2 = buffer.getNumSamples();
        for (int i = 0; i < numS2; ++i)
        {
            const float g = startG + (target - startG) * (float) (i + 1) / (float) numS2;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample (ch, i, buffer.getSample (ch, i) * g);
        }
        clipAutoGain = target;
    }

    // Meter: schnell hoch, langsam runter
    const float prev = clipReductionDb.load();
    clipReductionDb = maxReduction > prev ? maxReduction : prev * 0.85f;
}

void OneShooterProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

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

    const int numSamples = buffer.getNumSamples();

    {
        const juce::SpinLock::ScopedTryLockType tl (sampleLock);
        const SampleData* s = tl.isLocked() ? currentSample.get() : nullptr;

        if (previewRequested.exchange (false) && s != nullptr)
            startVoice (-1, 100, s);

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
                startVoice (msg.getNoteNumber(), msg.getVelocity(), s);
            else if (msg.isNoteOff())
                releaseVoices (msg.getNoteNumber());
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
                for (auto& v : voices)
                    if (v.active)
                        v.fadeStep = 1.0f / (float) (currentSampleRate * chokeFadeMs / 1000.0);
        }

        if (pos < numSamples)
            renderVoices (buffer, pos, numSamples - pos, s);

        // Playhead der juengsten Stimme
        float newest = -1.0f;
        juce::uint32 newestAge = 0;
        if (s != nullptr)
            for (auto& v : voices)
                if (v.active && v.sampleActive && v.fadeStep == 0.0f && v.age > newestAge)
                {
                    newestAge = v.age;
                    newest = (float) (v.position / s->buffer.getNumSamples());
                }
        playPosition = newest;
    }

    // Tone / Low Cut / High Cut
    processToneCut (buffer);

    // Clipper (4x Oversampling, laeuft immer fuer konstante Latenz)
    if (oversampler != nullptr && buffer.getNumChannels() <= (int) oversampler->numChannels)
        processClipper (buffer);

    // Output-Gain
    gainSmoothed.setTargetValue (juce::Decibels::decibelsToGain (param (pGain)));
    if (gainSmoothed.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = gainSmoothed.getNextValue();
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.setSample (ch, i, buffer.getSample (ch, i) * g);
        }
    }
    else
    {
        buffer.applyGain (gainSmoothed.getTargetValue());
    }

    // Limiter (Ceiling -1 dB)
    processLimiter (buffer);

    const float peak = buffer.getMagnitude (0, numSamples);
    const float prevPeak = outputPeak.load();
    outputPeak = peak > prevPeak ? peak : prevPeak * 0.9f;
}

//==============================================================================
void OneShooterProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    juce::String path;
    {
        const juce::ScopedLock sl (peaksLock);
        path = loadedFile.getFullPathName();
    }
    state.setProperty ("samplePath", path, nullptr);
    state.setProperty ("showLibrary", showLibrary, nullptr);
    state.setProperty ("rootOverride", rootOverride.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void OneShooterProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (! state.isValid())
            return;

        const juce::String path = state.getProperty ("samplePath").toString();
        showLibrary = (bool) state.getProperty ("showLibrary", true);
        const int savedRoot = (int) state.getProperty ("rootOverride", rootAuto);
        apvts.replaceState (state);

        if (path.isNotEmpty())
        {
            juce::File f (path);
            if (f.existsAsFile())
                loadSample (f);   // setzt den Root auf Auto zurueck ...
        }
        setRootOverride (savedRoot);  // ... darum danach wiederherstellen
    }
}

juce::AudioProcessorEditor* OneShooterProcessor::createEditor()
{
    return new OneShooterEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OneShooterProcessor();
}
