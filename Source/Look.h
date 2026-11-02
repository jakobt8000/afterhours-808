// AFTERHOURS shared look: grey panel, white ink, everything drawn as dots ("ink on paper", inverted).
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace ahui
{
// palette (same values as the browser test versions)
inline const juce::Colour panel { 0xff505053 }, line { 0xff2e2e31 }, ink { 0xffffffff }, muted { 0xffc8c8cb }, dimArc { 0xff848487 };

inline juce::Font mono (float px, float spacingPx = 0)
{
    static juce::Typeface::Ptr tf = juce::Typeface::createSystemTypefaceFor (BinaryData::IBMPlexMonoRegular_ttf, BinaryData::IBMPlexMonoRegular_ttfSize);
    juce::Font f { juce::FontOptions (tf).withPointHeight (px) };
    if (spacingPx != 0) f.setExtraKerningFactor (spacingPx / px);
    return f;
}

// Stroke a path as a row of round dots (SVG: stroke-dasharray "0.01 gap" with round caps)
inline void dotPath (juce::Graphics& g, const juce::Path& path, float spacing, float diameter)
{
    juce::PathFlatteningIterator it (path, {}, 0.15f);
    float carry = 0; int sub = -1;
    const float r = diameter * 0.5f;
    while (it.next())
    {
        if (it.subPathIndex != sub) { sub = it.subPathIndex; carry = 0; }
        const float dx = it.x2 - it.x1, dy = it.y2 - it.y1, len = std::sqrt (dx * dx + dy * dy);
        if (len <= 0) continue;
        while (carry <= len)
        {
            const float t = carry / len;
            g.fillEllipse (it.x1 + dx * t - r, it.y1 + dy * t - r, diameter, diameter);
            carry += spacing;
        }
        carry -= len;
    }
}

// Light dot texture used for "selected" areas (CSS: radial-gradient .55px on a 3px grid, 60% ink)
inline void stipple (juce::Graphics& g, juce::Rectangle<float> r, float alpha = 0.6f, float step = 3.0f, float d = 1.1f)
{
    g.setColour (ink.withAlpha (alpha));
    for (float y = r.getY() + step * 0.5f; y < r.getBottom(); y += step)
        for (float x = r.getX() + step * 0.5f; x < r.getRight(); x += step)
            g.fillEllipse (x - d * 0.5f, y - d * 0.5f, d, d);
}

// SVG angle (0 = 3 o'clock, clockwise) -> JUCE arc angle (0 = 12 o'clock, clockwise)
inline float rad (float svgDeg) { return (svgDeg + 90.0f) * juce::MathConstants<float>::pi / 180.0f; }

// The dotted AFTERHOURS knob: dim outer track, lit arc, dotted ring, centre dot and dotted needle
inline void drawKnob (juce::Graphics& g, float cx, float cy, float size, float r, float lr, float v)
{
    const bool big = size > 110;
    juce::Path track; track.addCentredArc (cx, cy, lr, lr, 0, rad (135), rad (405), true);
    g.setColour (dimArc); dotPath (g, track, big ? 3.6f : 3.2f, big ? 2.0f : 1.5f);
    g.setColour (ink);
    if (v > 0.004f)
    {
        juce::Path lit; lit.addCentredArc (cx, cy, lr, lr, 0, rad (135), rad (135 + 270 * v), true);
        dotPath (g, lit, big ? 2.6f : 2.1f, big ? 3.2f : 2.4f);
    }
    juce::Path ring; ring.addCentredArc (cx, cy, r, r, 0, rad (0), rad (360), true);
    dotPath (g, ring, big ? 3.0f : 2.6f, big ? 1.8f : 1.5f);
    const float cd = big ? 4.8f : 3.2f;
    g.fillEllipse (cx - cd * 0.5f, cy - cd * 0.5f, cd, cd);
    const float a = (135 + 270 * v) * juce::MathConstants<float>::pi / 180.0f;
    juce::Path nd; nd.startNewSubPath (cx, cy); nd.lineTo (cx + (r - 4) * std::cos (a), cy + (r - 4) * std::sin (a));
    dotPath (g, nd, big ? 3.0f : 2.4f, big ? 3.0f : 2.4f);
}
} // namespace ahui
