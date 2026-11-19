#pragma once
#include <juce_core/juce_core.h>
#include <cmath>

namespace NoteValues
{
    // Laenge in 16teln als Notenwert, z. B. 1/16, 1/8, 3/16, 1/4, 1 bar + 1/8
    inline juce::String sixteenthsLabel (int k, int perBar)
    {
        auto frac = [] (int sixteenths) -> juce::String
        {
            int numr = sixteenths, denr = 16;
            while (numr % 2 == 0 && denr > 1) { numr /= 2; denr /= 2; }
            return denr == 1 ? juce::String (numr) : juce::String (numr) + "/" + juce::String (denr);
        };

        const int bars = k / perBar;
        const int rem = k % perBar;
        if (bars == 0) return frac (rem);
        const juce::String b = juce::String (bars) + (bars == 1 ? " bar" : " bars");
        return rem == 0 ? b : b + " + " + frac (rem);
    }

    // Naechster gaengiger Notenwert zu einer Laenge in Viertelnoten
    inline juce::String nearest (double quarters, double barQuarters)
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
            if (d < bestDist) { bestDist = d; best = juce::String (bars) + (bars == 1 ? " bar" : " bars"); }
        }
        return best;
    }
}
