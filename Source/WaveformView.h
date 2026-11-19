#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Waveform mit Start/End-Markern, Beat-Raster und Playhead
class WaveformView : public juce::Component
{
public:
    explicit WaveformView (KickSamplerProcessor& p);

    void setPeaks (WaveformPeaks newPeaks);
    void refresh();   // vom Editor-Timer aufgerufen

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    enum class Drag { none, start, end };

    float startFrac() const;
    float endFrac() const;
    float xForFrac (float f) const;
    float fracForX (float x) const;
    Drag hitTestMarker (float x) const;
    void setParam (const char* id, float value);

    KickSamplerProcessor& processor;
    WaveformPeaks peaks;
    Drag dragging = Drag::none;
    float lastPlayPos = -1.0f;
    float lastStart = -1.0f, lastEnd = -1.0f, lastTranspose = 0.0f;
    double lastBpm = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformView)
};
