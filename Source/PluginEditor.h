#pragma once

#include "PluginProcessor.h"
#include "FrameOverlay.h"
#include "WaveformView.h"
#include "LibraryPanel.h"
#include <juce_audio_utils/juce_audio_utils.h>

class OneShooterLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OneShooterLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical,
                        int thumbStart, int thumbSize, bool over, bool down) override;
};

// Vertikales Meter (Pegel oder Gain Reduction)
class Meter : public juce::Component
{
public:
    Meter (juce::Colour c, bool reduction, juce::String cap = {})
        : colour (c), isReduction (reduction), caption (cap.isNotEmpty() ? cap : juce::String (reduction ? "GR" : "OUT")) {}
    void setValue (float v) { if (std::abs (v - value) > 0.001f) { value = v; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    juce::Colour colour;
    bool isReduction;
    juce::String caption;
    float value = 0.0f;
};

// Root-Note-Anzeige im Header: Klick = Menue, Doppelklick = eintippen
class RootPill : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onClick, onDoubleClick;
    void set (const juce::String& note, const juce::String& mode)
    {
        if (note != noteText || mode != modeText) { noteText = note; modeText = mode; repaint(); }
    }
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mouseWasClicked() && e.getNumberOfClicks() == 1 && onClick) onClick();
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { if (onDoubleClick) onDoubleClick(); }
private:
    juce::String noteText, modeText;
};

// Info-Karte: Laenge als Notenwert + 16tel-Raster
class LengthCard : public juce::Component
{
public:
    void set (double sixteenths, double beats, double ms, const juce::String& note, int perBar);
    void clear();
    void paint (juce::Graphics&) override;
private:
    bool hasValue = false;
    double sixteenths = 0.0, beats = 0.0, ms = 0.0;
    juce::String note;
    int perBar = 16;
};

class OneShooterEditor : public juce::AudioProcessorEditor,
                          public juce::FileDragAndDropTarget,
                          private juce::ChangeListener,
                          private juce::Timer
{
    FrameOverlay frameOverlay;

public:
    explicit OneShooterEditor (OneShooterProcessor&);
    ~OneShooterEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { waveform.setDragHover (true); }
    void fileDragExit (const juce::StringArray&) override             { waveform.setDragHover (false); }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void openFileChooser();
    void updateTimelineIfNeeded (bool force);
    void updateInfo();

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Module
    {
        juce::String title;
        juce::Colour colour;
        juce::Rectangle<int> bounds;
    };

    void setupKnob (Knob& k, const char* paramID, const juce::String& name, juce::Colour c);
    void layoutKnobs (juce::Rectangle<int> area, std::initializer_list<Knob*> knobs);

    OneShooterProcessor& processor;
    OneShooterLookAndFeel lnf;

    LibraryPanel library;
    WaveformView waveform;
    LengthCard lengthCard;
    juce::TextButton loadButton { "Load" };
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::Label sampleName, sampleFolder;
    juce::ToggleButton chromaticButton, libraryButton;
    void setLibraryVisible (bool show);
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> chromaticAtt;
    juce::TooltipWindow tooltips { this, 500 };

    Knob kStart, kLength, kTranspose;
    Knob kPitchAmt, kPitchDecay;
    Knob kAttack, kHold, kDecay, kRelease;
    Knob kSubVol, kSubTune, kSubDecay;
    Knob kClipDrive;
    Knob kDuckSync, kDuckFade, kDuckCurve;
    Knob kTone, kLowCut, kHighCut;
    juce::ToggleButton polyButton, clipAutoButton, limiterButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> polyAtt, clipAutoAtt, limiterAtt;
    RootPill rootPill;
    std::unique_ptr<juce::TextEditor> rootEditor;
    void showRootMenu();
    void showRootEditor();
    void applyRootText();
    Knob kVelocity, kGain;

    juce::ToggleButton clipOn { "ON" }, clipHard { "HARD" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clipOnAtt, clipHardAtt;
    Meter clipMeter, outMeter, limMeter;

    Module mSample, mPitch, mAmp, mTone, mSub, mDuck, mClip, mOut;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::int64 lastTimelineHash = 0;
    juce::Rectangle<int> tempoBounds, namePill, logoArea, rootBounds;
    juce::String tempoText;
    HitTimeline timeline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OneShooterEditor)
};
