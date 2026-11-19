#include "WaveformView.h"
#include "Colours.h"
#include <cmath>

using namespace KickColours;

WaveformView::WaveformView (KickSamplerProcessor& p) : processor (p)
{
    setPeaks (processor.getPeaks());
}

void WaveformView::setPeaks (WaveformPeaks newPeaks)
{
    peaks = std::move (newPeaks);
    waveCache = {};
    repaint();
}

void WaveformView::setTimeline (KickTimeline t)
{
    timeline = std::move (t);
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

juce::Rectangle<float> WaveformView::waveArea() const
{
    return getLocalBounds().toFloat().reduced (10.0f, 0.0f).withTrimmedTop (22.0f).withTrimmedBottom (8.0f);
}

float WaveformView::xForFrac (float f) const
{
    auto a = waveArea();
    return a.getX() + a.getWidth() * juce::jlimit (0.0f, 1.0f, f);
}

float WaveformView::fracForX (float x) const
{
    auto a = waveArea();
    return juce::jlimit (0.0f, 1.0f, (x - a.getX()) / juce::jmax (1.0f, a.getWidth()));
}

WaveformView::Drag WaveformView::hitTestMarker (float x) const
{
    if (peaks.isEmpty())
        return Drag::none;
    const float ds = std::abs (x - xForFrac (startFrac()));
    const float de = std::abs (x - xForFrac (endFrac()));
    constexpr float tol = 9.0f;
    if (ds <= tol && ds <= de) return Drag::start;
    if (de <= tol)             return Drag::end;
    return Drag::none;
}

void WaveformView::setParam (const char* id, float value)
{
    if (auto* prm = processor.apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (value));
}

void WaveformView::refresh()
{
    const float pp = processor.getPlayPosition();
    const float s = startFrac(), e = endFrac();
    if (pp != lastPlayPos || s != lastStart || e != lastEnd)
    {
        lastPlayPos = pp; lastStart = s; lastEnd = e;
        repaint();
    }
}

void WaveformView::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Hintergrund
    g.setColour (inset);
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (dragHover ? accent : cardBorder);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 8.0f, dragHover ? 2.0f : 1.0f);

    if (peaks.isEmpty())
    {
        g.setColour (textDim);
        g.setFont (juce::FontOptions (16.0f));
        g.drawFittedText ("Kick-Sample hierher ziehen", getLocalBounds().translated (0, -10), juce::Justification::centred, 1);
        g.setFont (juce::FontOptions (12.0f));
        g.drawFittedText ("WAV  AIFF  FLAC  MP3", getLocalBounds().translated (0, 14), juce::Justification::centred, 1);
        return;
    }

    auto a = waveArea();
    const float mid = a.getCentreY();
    const float xs = xForFrac (startFrac());
    const float xe = xForFrac (endFrac());

    // ---- Beat-Raster (nach Timeline, inkl. Pitch-Envelope)
    const double bpm = juce::jmax (20.0, processor.getHostBpm());
    const double sixteenthSec = 60.0 / bpm / 4.0;
    const int num = processor.getTimeSigNumerator();
    const int den = processor.getTimeSigDenominator();
    const int perBar = juce::jmax (1, (int) std::round (num * 16.0 / den));

    if (! timeline.sourcePos.empty())
    {
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        float lastX = -100.0f;
        for (int k = 1; ; ++k)
        {
            const double idx = k * sixteenthSec / timeline.dt;
            if (idx >= (double) timeline.sourcePos.size() - 1)
                break;
            const float x = xForFrac ((float) timeline.sourcePos[(size_t) idx]);
            if (x - lastX < 3.0f && k % 4 != 0)
                continue;
            lastX = x;

            const bool isBar = k % perBar == 0;
            const bool isBeat = k % 4 == 0;
            g.setColour (juce::Colours::white.withAlpha (isBar ? 0.35f : isBeat ? 0.16f : 0.06f));
            g.drawVerticalLine ((int) x, a.getY() - 4.0f, a.getBottom());

            if (isBar || isBeat)
            {
                g.setColour (isBar ? text : textDim);
                const juce::String lbl = juce::String (k / perBar + 1) + "." + juce::String ((k % perBar) / 4 + 1);
                g.drawText (lbl, (int) x + 4, 4, 40, 14, juce::Justification::left);
            }
        }
    }

    // ---- Waveform (gefuellt, gespiegelt)
    const int bins = (int) peaks.mins.size();
    float maxAbs = 0.001f;
    for (int i = 0; i < bins; ++i)
        maxAbs = juce::jmax (maxAbs, std::abs (peaks.mins[(size_t) i]), std::abs (peaks.maxs[(size_t) i]));
    const float scale = (a.getHeight() * 0.5f - 4.0f) / maxAbs;

    if (waveCache.isEmpty())
    {
        juce::Path top, bottom;
        const int w = (int) a.getWidth();
        top.startNewSubPath (a.getX(), mid);
        std::vector<float> mins ((size_t) w);
        for (int px = 0; px < w; ++px)
        {
            const int b0 = px * bins / w;
            const int b1 = juce::jmax (b0 + 1, (px + 1) * bins / w);
            float mn = 0.0f, mx = 0.0f;
            for (int b = b0; b < juce::jmin (b1, bins); ++b)
            {
                mn = juce::jmin (mn, peaks.mins[(size_t) b]);
                mx = juce::jmax (mx, peaks.maxs[(size_t) b]);
            }
            top.lineTo (a.getX() + (float) px, mid - mx * scale);
            mins[(size_t) px] = mn;
        }
        for (int px = w - 1; px >= 0; --px)
            top.lineTo (a.getX() + (float) px, mid - mins[(size_t) px] * scale);
        top.closeSubPath();
        waveCache = top;
    }

    // Aktiver Bereich: Verlauf
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (juce::Rectangle<float> (xs, a.getY(), xe - xs, a.getHeight()).toNearestInt());
        juce::ColourGradient grad (accent2, 0.0f, a.getY(), accent, 0.0f, mid, false);
        grad.addColour (1.0, accent2);
        grad.point2 = { 0.0f, a.getBottom() };
        grad.clearColours();
        grad.addColour (0.0, accent2);
        grad.addColour (0.5, accent);
        grad.addColour (1.0, accent2);
        g.setGradientFill (grad);
        g.fillPath (waveCache);
    }
    // Inaktiver Bereich
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.excludeClipRegion (juce::Rectangle<float> (xs, a.getY() - 30.0f, xe - xs, a.getHeight() + 60.0f).toNearestInt());
        g.setColour (waveDim);
        g.fillPath (waveCache);
    }

    // Mittellinie
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine ((int) mid, a.getX(), a.getRight());

    // ---- Amp-Envelope-Overlay
    if (! timeline.ampEnv.empty())
    {
        juce::Path envPath;
        const size_t steps = juce::jmin (timeline.ampEnv.size(),
                                         (size_t) (timeline.sampleSeconds / timeline.dt) + 1);
        const float top = a.getY() + 2.0f, bot = a.getBottom() - 2.0f;
        for (size_t i = 0; i < steps; i += 2)
        {
            const float x = xForFrac ((float) timeline.sourcePos[i]);
            const float y = bot - (bot - top) * timeline.ampEnv[i];
            if (i == 0) envPath.startNewSubPath (x, bot);
            envPath.lineTo (x, y);
        }
        if (! envPath.isEmpty())
        {
            g.setColour (env.withAlpha (0.9f));
            g.strokePath (envPath, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved));
        }
    }

    // ---- Marker
    auto drawMarker = [&] (float x, bool isStart)
    {
        g.setColour (text);
        g.fillRect (x - 1.0f, a.getY() - 6.0f, 2.0f, a.getHeight() + 6.0f);
        juce::Path flag;
        const float fy = a.getBottom() - 16.0f;
        if (isStart) flag.addRoundedRectangle (x, fy, 12.0f, 16.0f, 3.0f, 3.0f, false, true, false, true);
        else         flag.addRoundedRectangle (x - 12.0f, fy, 12.0f, 16.0f, 3.0f, 3.0f, true, false, true, false);
        g.fillPath (flag);
        g.setColour (inset);
        g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
        g.drawText (isStart ? "S" : "E", juce::Rectangle<float> (isStart ? x : x - 12.0f, fy, 12.0f, 16.0f),
                    juce::Justification::centred);
    };
    drawMarker (xs, true);
    drawMarker (xe, false);

    // ---- Playhead
    const float pp = processor.getPlayPosition();
    if (pp >= 0.0f)
    {
        const float x = xForFrac (pp);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.fillRect (x - 3.0f, a.getY(), 6.0f, a.getHeight());
        g.setColour (juce::Colours::white);
        g.fillRect (x - 0.75f, a.getY(), 1.5f, a.getHeight());
    }
}

void WaveformView::mouseMove (const juce::MouseEvent& e)
{
    setMouseCursor (hitTestMarker ((float) e.x) != Drag::none ? juce::MouseCursor::LeftRightResizeCursor
                                                               : juce::MouseCursor::NormalCursor);
}

void WaveformView::beginGesture()
{
    if (dragging == Drag::start)
        processor.apvts.getParameter (ParamIDs::start)->beginChangeGesture();
    if (dragging != Drag::none)
        processor.apvts.getParameter (ParamIDs::length)->beginChangeGesture();
}

void WaveformView::endGesture()
{
    if (dragging == Drag::start)
        processor.apvts.getParameter (ParamIDs::start)->endChangeGesture();
    if (dragging != Drag::none)
        processor.apvts.getParameter (ParamIDs::length)->endChangeGesture();
}

void WaveformView::mouseDown (const juce::MouseEvent& e)
{
    if (peaks.isEmpty())
        return;

    dragging = hitTestMarker ((float) e.x);
    if (dragging == Drag::none)
    {
        const float x = (float) e.x;
        dragging = std::abs (x - xForFrac (startFrac())) < std::abs (x - xForFrac (endFrac())) ? Drag::start : Drag::end;
    }
    beginGesture();
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
        const float newStart = juce::jlimit (0.0f, end - 0.001f, f);
        setParam (ParamIDs::start, newStart);
        setParam (ParamIDs::length, juce::jlimit (0.001f, 1.0f, (end - newStart) / juce::jmax (0.0001f, 1.0f - newStart)));
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
    endGesture();
    dragging = Drag::none;
}

void WaveformView::mouseDoubleClick (const juce::MouseEvent&)
{
    setParam (ParamIDs::start, 0.0f);
    setParam (ParamIDs::length, 1.0f);
    repaint();
}
