#include "PluginEditor.h"
#include "Colours.h"
#include <cmath>

using namespace KickColours;

//==============================================================================
KickLookAndFeel::KickLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, text);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, background);
}

void KickLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float lineW = 3.5f;
    const float arcR = radius - lineW;
    const float angle = startAngle + pos * (endAngle - startAngle);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (knobTrack);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Bipolare Parameter (Transpose) von der Mitte aus zeichnen
    const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0 && s.getName() == "Transpose";
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setColour (panel.brighter (0.15f));
    g.fillEllipse (juce::Rectangle<float> (arcR * 1.4f, arcR * 1.4f).withCentre (centre));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -arcR * 0.7f, 3.0f, arcR * 0.4f, 1.5f);
    g.setColour (text);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void KickLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                            bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? accent.darker (0.2f) : highlighted ? panel.brighter (0.25f) : panel.brighter (0.1f));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (accent.withAlpha (0.7f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

//==============================================================================
namespace
{
    // Naechster Notenwert zu einer Laenge in Viertelnoten
    juce::String nearestNoteValue (double quarters, double barQuarters)
    {
        struct NV { double q; const char* name; };
        const NV values[] = {
            { 0.0625, "1/64" }, { 0.125, "1/32" }, { 0.1875, "1/32." }, { 0.25, "1/16" }, { 0.375, "1/16." },
            { 0.5, "1/8" }, { 0.75, "1/8." }, { 1.0, "1/4" }, { 1.5, "1/4." }, { 2.0, "1/2" }, { 3.0, "1/2." },
        };

        double bestDist = 1e9;
        juce::String best;
        for (auto& v : values)
        {
            const double d = std::abs (std::log (quarters / v.q));
            if (d < bestDist) { bestDist = d; best = v.name; }
        }
        for (int bars = 1; bars <= 8; bars *= 2)
        {
            const double d = std::abs (std::log (quarters / (bars * barQuarters)));
            if (d < bestDist) { bestDist = d; best = juce::String (bars) + (bars == 1 ? " Takt" : " Takte"); }
        }
        return best;
    }
}

KickSamplerEditor::KickSamplerEditor (KickSamplerProcessor& p)
    : AudioProcessorEditor (&p), processor (p), waveform (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (waveform);

    addAndMakeVisible (loadButton);
    loadButton.onClick = [this] { openFileChooser(); };

    sampleName.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    sampleName.setColour (juce::Label::textColourId, text);
    addAndMakeVisible (sampleName);

    tempoLabel.setFont (juce::FontOptions (13.0f));
    tempoLabel.setColour (juce::Label::textColourId, textDim);
    tempoLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (tempoLabel);

    lengthLabel.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    lengthLabel.setColour (juce::Label::textColourId, accent);
    addAndMakeVisible (lengthLabel);

    lengthDetail.setFont (juce::FontOptions (13.0f));
    lengthDetail.setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (lengthDetail);

    setupKnob (kStart,     ParamIDs::start,     "Start");
    setupKnob (kLength,    ParamIDs::length,    "Length");
    setupKnob (kTranspose, ParamIDs::transpose, "Transpose");
    setupKnob (kAttack,    ParamIDs::attack,    "Attack");
    setupKnob (kHold,      ParamIDs::hold,      "Hold");
    setupKnob (kDecay,     ParamIDs::decay,     "Decay");
    setupKnob (kVelocity,  ParamIDs::velocity,  "Velocity");
    setupKnob (kGain,      ParamIDs::gain,      "Gain");

    processor.addChangeListener (this);

    setResizable (true, true);
    setResizeLimits (640, 380, 1400, 800);
    setSize (780, 430);

    updateInfo();
    startTimerHz (30);
}

KickSamplerEditor::~KickSamplerEditor()
{
    processor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void KickSamplerEditor::setupKnob (Knob& k, const char* paramID, const juce::String& name)
{
    k.slider.setName (name);
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    addAndMakeVisible (k.slider);

    k.label.setText (name.toUpperCase(), juce::dontSendNotification);
    k.label.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    k.label.setColour (juce::Label::textColourId, textDim);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramID, k.slider);

    // Double-Click-Reset auf den Default (in echten Einheiten)
    auto* param = processor.apvts.getParameter (paramID);
    k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void KickSamplerEditor::paint (juce::Graphics& g)
{
    g.fillAll (background);

    g.setColour (text);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("KICK SAMPLER", 16, 10, 200, 26, juce::Justification::centredLeft);

    if (dragHover)
    {
        g.setColour (accent);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2.0f), 6.0f, 2.0f);
    }
}

void KickSamplerEditor::resized()
{
    auto r = getLocalBounds().reduced (16, 10);

    auto header = r.removeFromTop (28);
    header.removeFromLeft (170);
    loadButton.setBounds (header.removeFromRight (120).reduced (0, 2));
    header.removeFromRight (10);
    tempoLabel.setBounds (header.removeFromRight (170));
    sampleName.setBounds (header);

    r.removeFromTop (8);
    auto knobs = r.removeFromBottom (110);
    r.removeFromBottom (6);
    auto info = r.removeFromBottom (44);
    r.removeFromBottom (6);
    waveform.setBounds (r);

    lengthLabel.setBounds (info.removeFromLeft (260));
    lengthDetail.setBounds (info);

    Knob* all[] = { &kStart, &kLength, &kTranspose, &kAttack, &kHold, &kDecay, &kVelocity, &kGain };
    const int kw = knobs.getWidth() / 8;
    for (auto* k : all)
    {
        auto col = knobs.removeFromLeft (kw);
        k->label.setBounds (col.removeFromTop (16));
        k->slider.setBounds (col);
    }
}

bool KickSamplerEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
        if (KickSamplerProcessor::isSupportedFile (juce::File (f)))
            return true;
    return false;
}

void KickSamplerEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragHover = false;
    for (auto& f : files)
    {
        juce::File file (f);
        if (KickSamplerProcessor::isSupportedFile (file) && processor.loadSample (file))
            break;
    }
    repaint();
}

void KickSamplerEditor::openFileChooser()
{
    chooser = std::make_unique<juce::FileChooser> ("Kick-Sample laden", juce::File(),
                                                   "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg;*.m4a;*.caf");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              auto file = fc.getResult();
                              if (file.existsAsFile())
                                  processor.loadSample (file);
                          });
}

void KickSamplerEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    waveform.setPeaks (processor.getPeaks());
    updateInfo();
}

void KickSamplerEditor::timerCallback()
{
    waveform.refresh();
    updateInfo();
}

void KickSamplerEditor::updateInfo()
{
    const auto peaks = processor.getPeaks();
    sampleName.setText (peaks.isEmpty() ? juce::String ("Kein Sample geladen") : peaks.name, juce::dontSendNotification);

    const double bpm = processor.getHostBpm();
    const int num = processor.getTimeSigNumerator();
    const int den = processor.getTimeSigDenominator();

    tempoLabel.setText (processor.hasHostTempo()
                            ? juce::String (bpm, 2) + " BPM  " + juce::String (num) + "/" + juce::String (den)
                            : juce::String ("Kein Host-Tempo (120 BPM)"),
                        juce::dontSendNotification);

    const double sec = processor.getEffectiveLengthSeconds();
    if (sec <= 0.0)
    {
        lengthLabel.setText ("-- Takte", juce::dontSendNotification);
        lengthDetail.setText ("", juce::dontSendNotification);
        return;
    }

    const double quarters = sec * bpm / 60.0;
    const double barQuarters = num * 4.0 / den;
    const double bars = quarters / barQuarters;
    const double sixteenths = quarters * 4.0;

    lengthLabel.setText (juce::String (bars, 3) + (std::abs (bars - 1.0) < 0.0005 ? " Takt" : " Takte"),
                         juce::dontSendNotification);
    lengthDetail.setText (juce::String (quarters, 2) + " Beats  |  " + juce::String (sixteenths, 1) + " x 1/16  |  "
                              + juce::String (sec * 1000.0, 0) + " ms  |  ca. " + nearestNoteValue (quarters, barQuarters),
                          juce::dontSendNotification);
}
