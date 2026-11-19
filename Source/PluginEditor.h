#pragma once

#include "PluginProcessor.h"
#include "WaveformView.h"
#include <juce_audio_utils/juce_audio_utils.h>

class KickLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KickLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&,
                               bool highlighted, bool down) override;
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
    void fileDragEnter (const juce::StringArray&, int, int) override { dragHover = true;  repaint(); }
    void fileDragExit (const juce::StringArray&) override             { dragHover = false; repaint(); }

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void openFileChooser();
    void updateInfo();

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    void setupKnob (Knob& k, const char* paramID, const juce::String& name);

    KickSamplerProcessor& processor;
    KickLookAndFeel lnf;

    WaveformView waveform;
    juce::TextButton loadButton { "Sample laden" };
    juce::Label sampleName;
    juce::Label tempoLabel;
    juce::Label lengthLabel;
    juce::Label lengthDetail;

    Knob kStart, kLength, kTranspose, kAttack, kHold, kDecay, kVelocity, kGain;

    std::unique_ptr<juce::FileChooser> chooser;
    bool dragHover = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickSamplerEditor)
};
