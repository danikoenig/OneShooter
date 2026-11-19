#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>

// Waveform mit Start/End-Markern, Beat-Raster, Envelope-Overlay und Playhead
class WaveformView : public juce::Component
{
public:
    explicit WaveformView (KickSamplerProcessor& p);

    void setPeaks (WaveformPeaks newPeaks);
    void setTimeline (KickTimeline t);
    void refresh();
    void setDragHover (bool b) { dragHover = b; repaint(); }

    void paint (juce::Graphics&) override;
    void resized() override { waveCache = {}; }
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    enum class Drag { none, start, end };

    float startFrac() const;
    float endFrac() const;
    juce::Rectangle<float> waveArea() const;
    float xForFrac (float f) const;
    float fracForX (float x) const;
    Drag hitTestMarker (float x) const;
    void setParam (const char* id, float value);
    void beginGesture();
    void endGesture();

    KickSamplerProcessor& processor;
    WaveformPeaks peaks;
    KickTimeline timeline;
    juce::Path waveCache;
    Drag dragging = Drag::none;
    bool dragHover = false;
    float lastPlayPos = -1.0f, lastStart = -1.0f, lastEnd = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformView)
};
