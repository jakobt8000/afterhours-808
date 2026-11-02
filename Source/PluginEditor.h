// 808 · AFTERHOURS
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace ui808
{
// Knob bound to a parameter. Drag up/down (shift = fine), double-click = default, wheel.
class Knob : public juce::Component
{
public:
    Knob (juce::RangedAudioParameter& p, juce::String label, std::function<juce::String (float)> fmt, bool big);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    juce::RangedAudioParameter& param;
    juce::ParameterAttachment attach;
    juce::String label;
    std::function<juce::String (float)> fmt;
    bool big;
    float value = 0, start = 0;
};

// One of the four mode tiles (MODE / DRIVE / GLIDE / OCTAVE)
class Tile : public juce::Component
{
public:
    std::function<juce::String()> key, value, icon;
    std::function<void()> onClick;
    bool lit = false;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override { if (onClick) onClick(); }
};

class Content : public juce::Component, private juce::Timer
{
public:
    explicit Content (Processor808&);
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent&) override;
    static constexpr int W = 1120, H = 390;
private:
    void timerCallback() override;
    void restPattern();
    juce::String presetText() const;
    Processor808& proc;
    juce::OwnedArray<Knob> knobs;
    Tile tiles[4];
    juce::Image screen;
    std::vector<float> buf;
    uint32_t seed = 11;
    double lastMs = 0;
    int lastState = -1;
    float rv() { seed = (uint32_t) (((uint64_t) seed * 16807u) % 2147483647u); return (float) seed / 2147483647.0f; }
};
} // namespace ui808

class Editor808 : public juce::AudioProcessorEditor
{
public:
    explicit Editor808 (Processor808&);
    void resized() override;
    void paint (juce::Graphics& g) override { g.fillAll (juce::Colour (0xff505053)); }
private:
    ui808::Content content;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Editor808)
};
