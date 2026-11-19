#include "WaveformView.h"
#include "Colours.h"
#include <cmath>

WaveformView::WaveformView (KickSamplerProcessor& p) : processor (p)
{
    setPeaks (processor.getPeaks());
}

void WaveformView::setPeaks (WaveformPeaks newPeaks)
{
    peaks = std::move (newPeaks);
    repaint();
}

float WaveformView::startFrac() const
{
    return processor.apvts.getRawParameterValue (ParamIDs::start)->load();
}

float WaveformView::endFrac() const
{
    const float s = startFrac();
    const float l = processor.apvts.getRawParameterValue (ParamIDs::length)->load();
    return s + (1.0f - s) * l;
}

float WaveformView::xForFrac (float f) const   { return (float) getWidth() * juce::jlimit (0.0f, 1.0f, f); }
float WaveformView::fracForX (float x) const   { return juce::jlimit (0.0f, 1.0f, x / (float) juce::jmax (1, getWidth())); }

WaveformView::Drag WaveformView::hitTestMarker (float x) const
{
    if (peaks.isEmpty())
        return Drag::none;

    const float xs = xForFrac (startFrac());
    const float xe = xForFrac (endFrac());
    const float ds = std::abs (x - xs), de = std::abs (x - xe);
    constexpr float tol = 8.0f;

    if (ds <= tol && ds <= de) return Drag::start;
    if (de <= tol)             return Drag::end;
    return Drag::none;
}

void WaveformView::setParam (const char* id, float value)
{
    if (auto* p = processor.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (value));
}

void WaveformView::refresh()
{
    const float pp = processor.getPlayPosition();
    const float s = startFrac(), e = endFrac();
    const float tr = processor.apvts.getRawParameterValue (ParamIDs::transpose)->load();
    const double bpm = processor.getHostBpm();

    if (pp != lastPlayPos || s != lastStart || e != lastEnd || tr != lastTranspose || bpm != lastBpm)
    {
        lastPlayPos = pp; lastStart = s; lastEnd = e; lastTranspose = tr; lastBpm = bpm;
        repaint();
    }
}

void WaveformView::paint (juce::Graphics& g)
{
    using namespace KickColours;
    auto bounds = getLocalBounds().toFloat();

    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 6.0f);

    if (peaks.isEmpty())
    {
        g.setColour (textDim);
        g.setFont (juce::FontOptions (15.0f));
        g.drawFittedText ("Kick-Sample hierher ziehen (WAV, AIFF, FLAC, MP3)",
                          getLocalBounds(), juce::Justification::centred, 2);
        return;
    }

    const float w = bounds.getWidth();
    const float h = bounds.getHeight();
    const float mid = h * 0.5f;
    const float xs = xForFrac (startFrac());
    const float xe = xForFrac (endFrac());

    // Beat-Raster (16tel ab Start, Pitch beruecksichtigt)
    const double bpm = processor.getHostBpm();
    const double ratio = std::pow (2.0, processor.apvts.getRawParameterValue (ParamIDs::transpose)->load() / 12.0);
    const double sixteenthSec = 60.0 / juce::jmax (20.0, bpm) / 4.0;
    const double sixteenthFrac = sixteenthSec * ratio * peaks.sampleRate / juce::jmax (1, peaks.numSourceSamples);
    const int num = processor.getTimeSigNumerator();
    const int den = processor.getTimeSigDenominator();
    const int sixteenthsPerBar = juce::jmax (1, (int) std::round (num * 16.0 / den));

    if (sixteenthFrac * w >= 3.0)
    {
        g.setFont (juce::FontOptions (11.0f));
        for (int k = 1; ; ++k)
        {
            const double f = startFrac() + k * sixteenthFrac;
            if (f > 1.0) break;
            const float x = xForFrac ((float) f);

            if (k % sixteenthsPerBar == 0)
            {
                g.setColour (gridBar);
                g.drawVerticalLine ((int) x, 0.0f, h);
                g.setColour (textDim);
                g.drawText (juce::String (k / sixteenthsPerBar + 1) + ".1", (int) x + 3, 2, 40, 14, juce::Justification::left);
            }
            else if (k % 4 == 0)
            {
                g.setColour (gridBeat);
                g.drawVerticalLine ((int) x, 0.0f, h);
                g.setColour (textDim);
                g.drawText (juce::String (k / sixteenthsPerBar + 1) + "." + juce::String ((k % sixteenthsPerBar) / 4 + 1), (int) x + 3, 2, 40, 14, juce::Justification::left);
            }
            else
            {
                g.setColour (gridSixteenth);
                g.drawVerticalLine ((int) x, 0.0f, h);
            }
        }
    }

    // Waveform
    const int bins = (int) peaks.mins.size();
    float maxAbs = 0.001f;
    for (int i = 0; i < bins; ++i)
        maxAbs = juce::jmax (maxAbs, std::abs (peaks.mins[(size_t) i]), std::abs (peaks.maxs[(size_t) i]));
    const float scale = (mid - 6.0f) / maxAbs;

    for (int px = 0; px < (int) w; ++px)
    {
        const int b0 = px * bins / (int) w;
        const int b1 = juce::jmax (b0 + 1, (px + 1) * bins / (int) w);
        float mn = 0.0f, mx = 0.0f;
        for (int b = b0; b < juce::jmin (b1, bins); ++b)
        {
            mn = juce::jmin (mn, peaks.mins[(size_t) b]);
            mx = juce::jmax (mx, peaks.maxs[(size_t) b]);
        }
        const bool inRegion = (float) px >= xs && (float) px <= xe;
        g.setColour (inRegion ? accent : waveDim);
        g.drawVerticalLine (px, mid - mx * scale, mid - mn * scale + 1.0f);
    }

    // Bereich ausserhalb abdunkeln
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRect (juce::Rectangle<float> (0.0f, 0.0f, xs, h));
    g.fillRect (juce::Rectangle<float> (xe, 0.0f, w - xe, h));

    // Marker
    auto drawMarker = [&] (float x, bool isStart)
    {
        g.setColour (marker);
        g.fillRect (x - 1.0f, 0.0f, 2.0f, h);
        juce::Path tri;
        if (isStart)
            tri.addTriangle (x, h - 14.0f, x + 10.0f, h - 7.0f, x, h);
        else
            tri.addTriangle (x, h - 14.0f, x - 10.0f, h - 7.0f, x, h);
        g.fillPath (tri);
    };
    drawMarker (xs, true);
    drawMarker (xe, false);

    // Playhead
    const float pp = processor.getPlayPosition();
    if (pp >= 0.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawVerticalLine ((int) xForFrac (pp), 0.0f, h);
    }
}

void WaveformView::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (hitTestMarker ((float) e.x) != Drag::none ? juce::MouseCursor::LeftRightResizeCursor
                                                               : juce::MouseCursor::NormalCursor);
}

void WaveformView::mouseDown (const juce::MouseEvent& e)
{
    dragging = hitTestMarker ((float) e.x);
    if (dragging == Drag::none && ! peaks.isEmpty())
    {
        // Klick in die Waveform: naechsten Marker greifen
        const float x = (float) e.x;
        dragging = std::abs (x - xForFrac (startFrac())) < std::abs (x - xForFrac (endFrac())) ? Drag::start : Drag::end;
    }

    if (dragging == Drag::start)
    {
        processor.apvts.getParameter (ParamIDs::start)->beginChangeGesture();
        processor.apvts.getParameter (ParamIDs::length)->beginChangeGesture();
    }
    else if (dragging == Drag::end)
    {
        processor.apvts.getParameter (ParamIDs::length)->beginChangeGesture();
    }

    mouseDrag (e);
}

void WaveformView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == Drag::none)
        return;

    const float f = fracForX ((float) e.x);
    const float end = endFrac();

    if (dragging == Drag::start)
    {
        // Ende bleibt an seiner Position stehen
        const float newStart = juce::jmin (f, end - 0.001f);
        const float newLen = (end - newStart) / juce::jmax (0.0001f, 1.0f - newStart);
        setParam (ParamIDs::start, juce::jmax (0.0f, newStart));
        setParam (ParamIDs::length, juce::jlimit (0.001f, 1.0f, newLen));
    }
    else
    {
        const float s = startFrac();
        const float newEnd = juce::jmax (f, s + 0.001f);
        setParam (ParamIDs::length, juce::jlimit (0.001f, 1.0f, (newEnd - s) / juce::jmax (0.0001f, 1.0f - s)));
    }
    repaint();
}

void WaveformView::mouseUp (const juce::MouseEvent&)
{
    if (dragging == Drag::start)
    {
        processor.apvts.getParameter (ParamIDs::start)->endChangeGesture();
        processor.apvts.getParameter (ParamIDs::length)->endChangeGesture();
    }
    else if (dragging == Drag::end)
    {
        processor.apvts.getParameter (ParamIDs::length)->endChangeGesture();
    }
    dragging = Drag::none;
}

void WaveformView::mouseDoubleClick (const juce::MouseEvent&)
{
    // Doppelklick: ganzes Sample
    setParam (ParamIDs::start, 0.0f);
    setParam (ParamIDs::length, 1.0f);
    repaint();
}
