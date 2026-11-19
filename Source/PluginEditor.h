#pragma once

#include "PluginProcessor.h"
#include "WaveformView.h"
#include "LibraryPanel.h"
#include <juce_audio_utils/juce_audio_utils.h>

class KickLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KickLookAndFeel();
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
    Meter (juce::Colour c, bool reduction) : colour (c), isReduction (reduction) {}
    void setValue (float v) { if (std::abs (v - value) > 0.001f) { value = v; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    juce::Colour colour;
    bool isReduction;
    float value = 0.0f;
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

class KickSamplerEditor : public juce::AudioProcessorEditor,
                          public juce::FileDragAndDropTarget,
                          private juce::ChangeListener,
                          private juce::Timer
{
public:
    explicit KickSamplerEditor (KickSamplerProcessor&);
    ~KickSamplerEditor() override;

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

    KickSamplerProcessor& processor;
    KickLookAndFeel lnf;

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
    Knob kVelocity, kGain;

    juce::ToggleButton clipOn { "ON" }, clipHard { "HARD" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> clipOnAtt, clipHardAtt;
    Meter clipMeter, outMeter;

    Module mSample, mPitch, mAmp, mSub, mDuck, mClip, mOut;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::int64 lastTimelineHash = 0;
    juce::Rectangle<int> tempoBounds, namePill, logoArea;
    juce::String tempoText;
    KickTimeline timeline;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickSamplerEditor)
};
