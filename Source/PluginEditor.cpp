#include "PluginEditor.h"
#include "Colours.h"
#include "NoteValues.h"
#include <cmath>

using namespace KickColours;

//==============================================================================
// LookAndFeel
KickLookAndFeel::KickLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, accent.withAlpha (0.4f));
    setColour (juce::Label::textColourId, text);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, text);
    setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, accent);
    setColour (juce::PopupMenu::backgroundColourId, card);
}

void KickLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const auto col = juce::Colour::fromString (s.getProperties().getWithDefault ("accent", accent.toString()).toString());
    auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat();
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight()) - 6.0f;
    auto r = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre());
    const auto c = r.getCentre();
    const float radius = size * 0.5f;
    const float lineW = juce::jmax (3.0f, size * 0.075f);
    const float arcR = radius - lineW * 0.5f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool hover = s.isMouseOverOrDragging();

    // Track
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (knobTrack);
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Wert (bipolar von der Mitte)
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
    if (std::abs (angle - from) > 0.001f)
    {
        juce::Path val;
        val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
        g.setColour (col.withAlpha (0.25f));
        g.strokePath (val, juce::PathStrokeType (lineW * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (col);
        g.strokePath (val, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Body
    const float bodyR = arcR - lineW * 1.6f;
    auto body = juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c);
    g.setGradientFill (juce::ColourGradient (knobBody.brighter (hover ? 0.25f : 0.15f), body.getX(), body.getY(),
                                             knobBody.darker (0.3f), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawEllipse (body.reduced (0.5f), 1.0f);

    // Zeiger
    const float px = c.x + std::sin (angle) * bodyR * 0.72f;
    const float py = c.y - std::cos (angle) * bodyR * 0.72f;
    const float qx = c.x + std::sin (angle) * bodyR * 0.25f;
    const float qy = c.y - std::cos (angle) * bodyR * 0.25f;
    g.setColour (text);
    g.drawLine (qx, qy, px, py, 2.2f);
}

juce::Label* KickLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::FontOptions (11.5f));
    l->setColour (juce::Label::textColourId, text.withAlpha (0.9f));
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::TextEditor::backgroundColourId, inset);
    l->setColour (juce::TextEditor::textColourId, text);
    l->setColour (juce::TextEditor::outlineColourId, accent);
    return l;
}

void KickLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                                            bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool primary = (bool) b.getProperties().getWithDefault ("primary", false);
    if (primary)
    {
        g.setGradientFill (juce::ColourGradient (accent2, r.getX(), r.getY(), accent, r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 6.0f);
        if (highlighted || down)
        {
            g.setColour (juce::Colours::white.withAlpha (down ? 0.2f : 0.1f));
            g.fillRoundedRectangle (r, 6.0f);
        }
        b.setColour (juce::TextButton::textColourOffId, bgBottom);
        return;
    }
    g.setColour (down ? cardBorder.brighter (0.2f) : highlighted ? cardBorder : inset);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (cardBorder.brighter (0.15f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void KickLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const auto col = juce::Colour::fromString (b.getProperties().getWithDefault ("accent", accent.toString()).toString());
    const auto style = b.getProperties().getWithDefault ("style", "power").toString();
    const bool on = b.getToggleState();

    if (style == "segment")
    {
        // SOFT | HARD
        g.setColour (inset);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (cardBorder);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto left = r.withWidth (r.getWidth() * 0.5f).reduced (2.0f);
        auto right = left.withX (r.getCentreX() + 2.0f);
        g.setColour (col.withAlpha (0.9f));
        g.fillRoundedRectangle (on ? right : left, 4.0f);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.setColour (on ? textDim : bgBottom);
        g.drawText ("SOFT", left, juce::Justification::centred);
        g.setColour (on ? bgBottom : textDim);
        g.drawText ("HARD", right, juce::Justification::centred);
        return;
    }

    if (style == "keys")
    {
        // Mini-Keyboard wie in Bitwig
        g.setColour (on ? col.withAlpha (0.2f) : inset);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (on ? col : (highlighted ? textDim : cardBorder.brighter (0.2f)));
        g.drawRoundedRectangle (r, 5.0f, 1.2f);

        auto kb = juce::Rectangle<float> (26.0f, 14.0f).withCentre ({ r.getX() + 19.0f, r.getCentreY() });
        const float kw = kb.getWidth() / 5.0f;
        for (int i = 0; i < 5; ++i)
        {
            g.setColour (on ? text : textDim);
            g.fillRect (kb.getX() + (float) i * kw + 0.5f, kb.getY(), kw - 1.0f, kb.getHeight());
        }
        g.setColour (on ? bgBottom : inset);
        for (int i : { 1, 2, 4 })
            g.fillRect (kb.getX() + (float) i * kw - kw * 0.3f, kb.getY(), kw * 0.6f, kb.getHeight() * 0.6f);

        g.setColour (on ? text : textDim);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (on ? "CHROMATIC" : "ONE KEY", r.withTrimmedLeft (36.0f), juce::Justification::centredLeft);
        return;
    }

    // Power-Pill
    g.setColour (on ? col.withAlpha (0.18f) : inset);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (on ? col : (highlighted ? textDim : cardBorder.brighter (0.2f)));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
    auto dot = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ r.getX() + 14.0f, r.getCentreY() });
    g.setColour (on ? col : textDim.withAlpha (0.5f));
    g.fillEllipse (dot);
    g.setColour (on ? text : textDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (on ? "ON" : "OFF", r.withTrimmedLeft (22.0f), juce::Justification::centred);
}

void KickLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                                     int thumbStart, int thumbSize, bool over, bool down)
{
    auto thumb = vertical ? juce::Rectangle<int> (x, thumbStart, w, thumbSize)
                          : juce::Rectangle<int> (thumbStart, y, thumbSize, h);
    g.setColour (textDim.withAlpha (over || down ? 0.6f : 0.3f));
    g.fillRoundedRectangle (thumb.reduced (1).toFloat(), 3.0f);
}

//==============================================================================
void Meter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (textDim);
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.drawText (isReduction ? "GR" : "OUT", r.withHeight (12.0f).expanded (10.0f, 0.0f), juce::Justification::centred);
    auto bar = r.withTrimmedTop (16.0f).withTrimmedBottom (16.0f);
    g.setColour (inset);
    g.fillRoundedRectangle (bar, 3.0f);

    float frac;
    juce::String label;
    if (isReduction)
    {
        frac = juce::jlimit (0.0f, 1.0f, value / 12.0f);
        label = value > 0.05f ? "-" + juce::String (value, 1) : "0.0";
        // von oben nach unten
        auto fill = bar.withHeight (bar.getHeight() * frac);
        g.setColour (colour);
        g.fillRoundedRectangle (fill, 3.0f);
    }
    else
    {
        const float db = juce::Decibels::gainToDecibels (value, -60.0f);
        frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);
        label = value > 0.0001f ? juce::String (db, 1) : "-inf";
        auto fill = bar.withTrimmedTop (bar.getHeight() * (1.0f - frac));
        g.setGradientFill (juce::ColourGradient (value > 1.0f ? clip : colour, 0.0f, bar.getY(),
                                                 colour.darker (0.4f), 0.0f, bar.getBottom(), false));
        g.fillRoundedRectangle (fill, 3.0f);
        // 0-dB-Linie
        const float y0 = bar.getBottom() - bar.getHeight() * (60.0f / 66.0f);
        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.drawHorizontalLine ((int) y0, bar.getX(), bar.getRight());
    }

    g.setColour (textDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText (label, r.withTop (r.getBottom() - 14.0f).expanded (12.0f, 0.0f), juce::Justification::centred);
}

//==============================================================================
void LengthCard::clear()
{
    if (hasValue) { hasValue = false; repaint(); }
}

void LengthCard::set (double s16, double b, double m, const juce::String& n, int pb)
{
    if (! hasValue || std::abs (s16 - sixteenths) > 0.005 || n != note || pb != perBar || std::abs (m - ms) > 0.5)
    {
        hasValue = true; sixteenths = s16; beats = b; ms = m; note = n; perBar = juce::jmax (1, pb);
        repaint();
    }
}

void LengthCard::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (card);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (cardBorder);
    g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);

    auto inner = getLocalBounds().reduced (16, 14);
    g.setColour (env);
    g.fillEllipse ((float) inner.getX(), (float) inner.getY() + 5.0f, 7.0f, 7.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("LENGTH", inner.removeFromTop (18).withTrimmedLeft (14), juce::Justification::centredLeft);

    inner.removeFromTop (6);
    g.setColour (accent2);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawText (hasValue ? juce::String::fromUTF8 ("\xe2\x89\x88 ") + note : juce::String ("--"),
                inner.removeFromTop (42), juce::Justification::centredLeft);

    // 16tel-Raster (ein Takt), gefuellt bis zur Laenge
    inner.removeFromTop (4);
    auto grid = inner.removeFromTop (18).toFloat();
    const int cells = perBar;
    const float gap = 2.0f;
    const float cw = (grid.getWidth() - gap * (float) (cells - 1)) / (float) cells;
    const double inBar = hasValue ? std::fmod (sixteenths, (double) cells) : 0.0;
    const int fullBars = hasValue ? (int) (sixteenths / cells) : 0;
    for (int i = 0; i < cells; ++i)
    {
        auto c = juce::Rectangle<float> (grid.getX() + (float) i * (cw + gap), grid.getY(), cw, grid.getHeight());
        g.setColour (i % 4 == 0 ? knobTrack.brighter (0.15f) : knobTrack);
        g.fillRoundedRectangle (c, 2.0f);
        const double fill = (fullBars > 0 && inBar < 0.01) ? 1.0 : juce::jlimit (0.0, 1.0, inBar - i);
        if (fill > 0.0)
        {
            g.setColour (accent);
            g.fillRoundedRectangle (c.withWidth (c.getWidth() * (float) fill), 2.0f);
        }
    }
    inner.removeFromTop (3);
    g.setColour (textDim);
    g.setFont (juce::FontOptions (10.0f));
    auto lbl = inner.removeFromTop (14);
    g.drawText ("1 bar in 1/16", lbl, juce::Justification::centredLeft);
    if (fullBars > 0)
    {
        g.setColour (accent2);
        g.drawText ("+" + juce::String (fullBars) + (fullBars == 1 ? " bar" : " bars"), lbl, juce::Justification::centredRight);
    }

    inner.removeFromTop (8);
    g.setFont (juce::FontOptions (13.0f));
    auto row = [&] (const juce::String& k, const juce::String& v)
    {
        auto rr = inner.removeFromTop (21);
        g.setColour (textDim);
        g.drawText (k, rr, juce::Justification::centredLeft);
        g.setColour (text);
        g.drawText (v, rr, juce::Justification::centredRight);
    };
    row ("Sixteenths", hasValue ? juce::String (sixteenths, 1) : "--");
    row ("Beats",      hasValue ? juce::String (beats, 2) : "--");
    row ("Duration",   hasValue ? juce::String (juce::roundToInt (ms)) + " ms" : "--");
}

KickSamplerEditor::KickSamplerEditor (KickSamplerProcessor& p)
    : AudioProcessorEditor (&p), processor (p), library (p), waveform (p),
      clipMeter (clip, true), outMeter (accent, false)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (library);
    addAndMakeVisible (waveform);
    addAndMakeVisible (lengthCard);

    loadButton.getProperties().set ("primary", true);
    loadButton.onClick = [this] { openFileChooser(); };
    addAndMakeVisible (loadButton);

    sampleName.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    sampleName.setColour (juce::Label::textColourId, text);
    sampleName.setJustificationType (juce::Justification::centredLeft);
    sampleName.setMinimumHorizontalScale (1.0f);
    addAndMakeVisible (sampleName);

    sampleFolder.setFont (juce::FontOptions (11.0f));
    sampleFolder.setColour (juce::Label::textColourId, textDim);
    sampleFolder.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (sampleFolder);

    prevButton.setTooltip ("Previous sample in this folder");
    nextButton.setTooltip ("Next sample in this folder");
    prevButton.onClick = [this] { library.step (-1); };
    nextButton.onClick = [this] { library.step (+1); };
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    chromaticButton.getProperties().set ("style", "keys");
    chromaticButton.getProperties().set ("accent", accent.toString());
    chromaticButton.setTooltip ("Chromatic mode: play the sample pitched across the keyboard (C3 = original pitch). "
                                "Off: every key plays the sample at the same pitch.");
    addAndMakeVisible (chromaticButton);
    chromaticAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, ParamIDs::chromatic, chromaticButton);

    mSample = { "SAMPLE",    accent, {} };
    mPitch  = { "PITCH ENV", pitch,  {} };
    mAmp    = { "AMP ENV",   env,    {} };
    mSub    = { "SUB",       sub,    {} };
    mClip   = { "CLIPPER",   clip,   {} };
    mOut    = { "OUTPUT",    accent2, {} };

    setupKnob (kStart,      ParamIDs::start,      "Start",     accent);
    setupKnob (kLength,     ParamIDs::length,     "Length",    accent);
    setupKnob (kTranspose,  ParamIDs::transpose,  "Transpose", accent);
    setupKnob (kPitchAmt,   ParamIDs::pitchAmt,   "Amount",    pitch);
    setupKnob (kPitchDecay, ParamIDs::pitchDecay, "Decay",     pitch);
    setupKnob (kAttack,     ParamIDs::attack,     "Attack",    env);
    setupKnob (kHold,       ParamIDs::hold,       "Hold",      env);
    setupKnob (kDecay,      ParamIDs::decay,      "Decay",     env);
    setupKnob (kRelease,    ParamIDs::release,    "Release",   env);
    setupKnob (kSubVol,     ParamIDs::subVol,     "Volume",    sub);
    setupKnob (kSubTune,    ParamIDs::subTune,    "Tune",      sub);
    setupKnob (kSubDecay,   ParamIDs::subDecay,   "Decay",     sub);
    setupKnob (kClipDrive,  ParamIDs::clipDrive,  "Drive",     clip);
    setupKnob (kVelocity,   ParamIDs::velocity,   "Velocity",  accent2);
    setupKnob (kGain,       ParamIDs::gain,       "Gain",      accent2);

    kTranspose.slider.getProperties().set ("bipolar", true);
    kPitchAmt.slider.getProperties().set ("bipolar", true);

    clipOn.getProperties().set ("accent", clip.toString());
    clipHard.getProperties().set ("accent", clip.toString());
    clipHard.getProperties().set ("style", "segment");
    addAndMakeVisible (clipOn);
    addAndMakeVisible (clipHard);
    clipOnAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, ParamIDs::clipOn, clipOn);
    clipHardAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, ParamIDs::clipHard, clipHard);

    addAndMakeVisible (clipMeter);
    addAndMakeVisible (outMeter);

    processor.addChangeListener (this);

    setResizable (true, true);
    setResizeLimits (1100, 660, 1800, 1100);
    setSize (1240, 720);

    updateTimelineIfNeeded (true);
    updateInfo();
    startTimerHz (30);
}

KickSamplerEditor::~KickSamplerEditor()
{
    processor.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void KickSamplerEditor::setupKnob (Knob& k, const char* paramID, const juce::String& name, juce::Colour c)
{
    k.slider.setName (name);
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    k.slider.getProperties().set ("accent", c.toString());
    k.slider.setMouseDragSensitivity (180);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (k.slider);

    k.label.setText (name.toUpperCase(), juce::dontSendNotification);
    k.label.setFont (juce::FontOptions (10.5f, juce::Font::bold));
    k.label.setColour (juce::Label::textColourId, textDim);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramID, k.slider);

    auto* param = processor.apvts.getParameter (paramID);
    k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
}

void KickSamplerEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Logo
    auto header = getLocalBounds().reduced (14).withTrimmedLeft (library.getRight()).removeFromTop (40);
    header.removeFromLeft (14);
    {
        // Fadenkreuz-Logo
        const auto lc = juce::Point<float> ((float) header.getX() + 12.0f, (float) header.getY() + 20.0f);
        g.setColour (accent);
        g.drawEllipse (juce::Rectangle<float> (22.0f, 22.0f).withCentre (lc), 2.2f);
        g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre (lc));
        for (auto d : { juce::Point<float> (0, -1), juce::Point<float> (0, 1), juce::Point<float> (-1, 0), juce::Point<float> (1, 0) })
            g.drawLine (lc.x + d.x * 7.0f, lc.y + d.y * 7.0f, lc.x + d.x * 14.0f, lc.y + d.y * 14.0f, 2.2f);
    }
    g.setColour (text);
    g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
    g.drawText ("ONE SHOOTER", header.getX() + 34, header.getY() + 4, 200, 22, juce::Justification::centredLeft);
    g.setColour (textDim);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("by Dani Koenig", header.getX() + 34, header.getY() + 24, 200, 14, juce::Justification::centredLeft);

    // Modul-Karten
    for (auto* m : { &mSample, &mPitch, &mAmp, &mSub, &mClip, &mOut })
    {
        auto r = m->bounds.toFloat();
        g.setColour (card);
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (cardBorder);
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
        g.setColour (m->colour);
        g.fillEllipse (r.getX() + 14.0f, r.getY() + 15.0f, 7.0f, 7.0f);
        g.setColour (text);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (m->title, (int) r.getX() + 28, (int) r.getY() + 10, 160, 18, juce::Justification::centredLeft);
    }

    // Sample-Name- und Tempo-Pill
    for (auto pill : { namePill.toFloat(), tempoBounds.toFloat() })
    {
        g.setColour (inset);
        g.fillRoundedRectangle (pill, 6.0f);
        g.setColour (cardBorder);
        g.drawRoundedRectangle (pill.reduced (0.5f), 6.0f, 1.0f);
    }
    g.setColour (env);
    g.fillEllipse ((float) tempoBounds.getX() + 12.0f, (float) tempoBounds.getCentreY() - 3.5f, 7.0f, 7.0f);
    g.setColour (text);
    g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
    g.drawText (tempoText, tempoBounds.withTrimmedLeft (26), juce::Justification::centredLeft);
}

void KickSamplerEditor::layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs)
{
    const int w = area.getWidth() / (int) knobs.size();
    for (auto* k : knobs)
    {
        auto col = area.removeFromLeft (w);
        k->label.setBounds (col.removeFromTop (16));
        k->slider.setBounds (col);
    }
}

void KickSamplerEditor::resized()
{
    auto r = getLocalBounds().reduced (14);

    library.setBounds (r.removeFromLeft (290));
    r.removeFromLeft (14);

    // Header
    auto header = r.removeFromTop (40);
    header.removeFromLeft (240);
    loadButton.setBounds (header.removeFromRight (130).reduced (0, 4));
    header.removeFromRight (12);
    tempoBounds = header.removeFromRight (190).reduced (0, 4);
    header.removeFromRight (12);
    nextButton.setBounds (header.removeFromRight (30).reduced (0, 4));
    header.removeFromRight (4);
    prevButton.setBounds (header.removeFromRight (30).reduced (0, 4));
    header.removeFromRight (8);
    namePill = header.reduced (0, 4);
    auto np = namePill.reduced (12, 0);
    sampleFolder.setBounds (np.removeFromRight (np.getWidth() / 2));
    sampleName.setBounds (np);

    r.removeFromTop (12);

    // Module (zwei Reihen)
    const int rowH = (r.getHeight() - 12 - 12 - 200) / 2;
    auto rowB = r.removeFromBottom (juce::jmax (150, rowH));
    r.removeFromBottom (12);
    auto rowA = r.removeFromBottom (juce::jmax (150, rowH));
    r.removeFromBottom (12);

    // Waveform + Laengen-Karte
    lengthCard.setBounds (r.removeFromRight (250));
    r.removeFromRight (12);
    waveform.setBounds (r);

    auto inner = [] (juce::Rectangle<int> b) { return b.reduced (10, 8).withTrimmedTop (26); };

    // Reihe A: Sample 3 | Pitch 2 | Amp 4
    {
        const int gap = 12;
        const int unit = (rowA.getWidth() - 2 * gap) / 9;
        mSample.bounds = rowA.removeFromLeft (unit * 3); rowA.removeFromLeft (gap);
        mPitch.bounds  = rowA.removeFromLeft (unit * 2); rowA.removeFromLeft (gap);
        mAmp.bounds    = rowA;
        layoutKnobs (inner (mSample.bounds), { &kStart, &kLength, &kTranspose });
        chromaticButton.setBounds (mSample.bounds.getRight() - 122, mSample.bounds.getY() + 8, 110, 24);
        layoutKnobs (inner (mPitch.bounds),  { &kPitchAmt, &kPitchDecay });
        layoutKnobs (inner (mAmp.bounds),    { &kAttack, &kHold, &kDecay, &kRelease });
    }

    // Reihe B: Sub 3 | Clip 3 | Out 3
    {
        const int gap = 12;
        const int unit = (rowB.getWidth() - 2 * gap) / 9;
        mSub.bounds  = rowB.removeFromLeft (unit * 3); rowB.removeFromLeft (gap);
        mClip.bounds = rowB.removeFromLeft (unit * 3); rowB.removeFromLeft (gap);
        mOut.bounds  = rowB;
        layoutKnobs (inner (mSub.bounds), { &kSubVol, &kSubTune, &kSubDecay });

        auto c = inner (mClip.bounds);
        clipMeter.setBounds (c.removeFromRight (22).withTrimmedTop (4));
        c.removeFromRight (12);
        auto toggles = c.removeFromRight (juce::jmin (120, c.getWidth() / 2));
        const int ty = toggles.getCentreY();
        clipOn.setBounds (toggles.getX(), ty - 36, toggles.getWidth(), 28);
        clipHard.setBounds (toggles.getX(), ty + 6, toggles.getWidth(), 28);
        c.removeFromRight (8);
        layoutKnobs (c, { &kClipDrive });

        auto o = inner (mOut.bounds);
        outMeter.setBounds (o.removeFromRight (22).withTrimmedTop (4));
        o.removeFromRight (12);
        layoutKnobs (o, { &kVelocity, &kGain });
    }

    // Titel in der Clip-Karte zeigt GR an: Meter-Label
}

bool KickSamplerEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
    {
        juce::File file (f);
        if (file.isDirectory() || KickSamplerProcessor::isSupportedFile (file))
            return true;
    }
    return false;
}

void KickSamplerEditor::filesDropped (const juce::StringArray& files, int, int)
{
    waveform.setDragHover (false);
    bool loaded = false;
    for (auto& f : files)
    {
        juce::File file (f);
        if (file.isDirectory())
            processor.getLibrary().addFolder (file);   // Ordner -> One-Shot-Index
        else if (! loaded && KickSamplerProcessor::isSupportedFile (file))
            loaded = processor.loadSample (file);
    }
}

void KickSamplerEditor::openFileChooser()
{
    chooser = std::make_unique<juce::FileChooser> ("Load sample", juce::File(),
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
    updateTimelineIfNeeded (true);
    updateInfo();
    library.repaint();
}

void KickSamplerEditor::timerCallback()
{
    waveform.refresh();
    updateTimelineIfNeeded (false);
    updateInfo();
    clipMeter.setValue (processor.getClipReductionDb());
    outMeter.setValue (processor.getOutputPeak());
    library.refreshStatus();
}

void KickSamplerEditor::updateTimelineIfNeeded (bool force)
{
    const char* ids[] = { ParamIDs::start, ParamIDs::length, ParamIDs::transpose, ParamIDs::pitchAmt,
                          ParamIDs::pitchDecay, ParamIDs::attack, ParamIDs::hold, ParamIDs::decay,
                          ParamIDs::subVol, ParamIDs::subDecay };
    juce::int64 h = (juce::int64) (processor.getHostBpm() * 1000.0) * 31 + processor.getTimeSigNumerator();
    for (auto* id : ids)
        h = h * 1000003 + (juce::int64) (processor.apvts.getRawParameterValue (id)->load() * 100000.0f);
    h = h * 7 + processor.getPeaks().numSourceSamples;

    if (! force && h == lastTimelineHash)
        return;

    lastTimelineHash = h;
    timeline = processor.computeTimeline();
    waveform.setTimeline (timeline);
}

void KickSamplerEditor::updateInfo()
{
    const auto peaks = processor.getPeaks();
    sampleName.setText (peaks.isEmpty() ? juce::String ("No sample loaded") : peaks.name, juce::dontSendNotification);
    {
        auto folder = library.folderOfLoaded();
        if (folder.isEmpty() && ! peaks.isEmpty())
            folder = processor.getLoadedFile().getParentDirectory().getFileName();
        sampleFolder.setText (folder.replace ("/", " / "), juce::dontSendNotification);
    }

    const double bpm = processor.getHostBpm();
    const int num = processor.getTimeSigNumerator();
    const int den = processor.getTimeSigDenominator();
    const juce::String tempo = processor.hasHostTempo()
        ? juce::String (bpm, 2) + " BPM   " + juce::String (num) + "/" + juce::String (den)
        : juce::String ("120 BPM (no host)");

    if (tempo != tempoText) { tempoText = tempo; repaint (tempoBounds); }

    const double sec = timeline.totalSeconds();
    if (sec <= 0.0)
    {
        lengthCard.clear();
        return;
    }

    const double quarters = sec * bpm / 60.0;
    const double barQuarters = num * 4.0 / den;
    lengthCard.set (quarters * 4.0, quarters, sec * 1000.0, NoteValues::nearest (quarters, barQuarters),
                    juce::jmax (1, (int) std::round (num * 16.0 / den)));
}
