// 808 · AFTERHOURS
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include "Dsp.h"

struct Preset808
{
    const char* name;
    float tune, glide, decay, punch, sub, tone, click, penv, output, drive;
    int type; bool glideOn; int mode;
};

class Processor808 : public juce::AudioProcessor
{
public:
    static const Preset808 PRESETS[6];

    Processor808();
    ~Processor808() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "808"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 5.0; }
    int getNumPrograms() override { return 6; }
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override { return PRESETS[juce::jlimit (0, 5, index)].name; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // editor helpers
    void readScope (float* dst, int n) const;
    double getRate() const { return sr; }
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void noteOn (int note);
    float p (std::atomic<float>* a) const { return a->load(); }

    std::atomic<float> *tuneP, *glideP, *decayP, *punchP, *subP, *toneP, *clickP, *penvP, *outP, *driveP, *modeP, *typeP, *glideOnP, *octP;
    int currentPreset = 0;

    double sr = 44100.0;
    std::vector<int> held;

    // voice
    double phase = 0;
    ah::Target freq;
    int sinceTrig = 1 << 30;
    float lastDecay = 2.4f;

    // amp envelope (ramp 3 ms -> settle to 1 -> exponential decay from 60 ms)
    int ampStage = -1, ampCount = 0, rampN = 1, settleEnd = 1;
    float ampV = 0, ampInc = 0, c1 = 0, c2 = 0;

    // click
    float clickV = 0, clickCoef = 0; int clickLeft = 0;
    ah::Biquad clickHp, tone;
    ah::Rng rng;

    ah::Target drvG, subG, outG, toneF;
    juce::dsp::Oversampling<float> os { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    juce::AudioBuffer<float> zero, shaped;
    std::vector<double> phStart, phInc;
    std::vector<float> sine, amp, click;

    std::vector<float> scope;
    std::atomic<int> scopeW { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Processor808)
};
