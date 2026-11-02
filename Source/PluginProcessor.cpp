// 808 · AFTERHOURS
// The 808 engine ported 1:1 from the browser prototype:
// sine -> waveshaper (4x oversampled) -> lowpass TONE -> drive gain  +  clean SUB  -> amp envelope -> + click -> output
#include "PluginProcessor.h"
#include "PluginEditor.h"

const Preset808 Processor808::PRESETS[6] = {
    //  name           tune glide decay punch sub  tone   click penv out  drive  type      glide  mode
    { "LONG SLIDE",    0, 140, 2.4f, 62,  80,  3200, 30, 7,  -3, 48, ah::CLIP, true,  0 },
    { "CLEAN SUB",     0, 120, 1.8f, 40,  100, 1500, 10, 5,  -2, 6,  ah::TAPE, false, 0 },
    { "RAGE DIST",     0, 90,  2.0f, 80,  60,  6500, 55, 12, -6, 86, ah::CLIP, true,  0 },
    { "FOLD BUZZ",     0, 160, 1.5f, 55,  70,  4500, 20, 9,  -5, 70, ah::FOLD, true,  1 },
    { "SHORT KNOCK",   0, 60,  0.5f, 90,  75,  8000, 70, 14, -3, 35, ah::TAPE, false, 0 },
    { "TAPE BOOM",     0, 220, 3.2f, 50,  90,  2500, 15, 6,  -3, 60, ah::TAPE, true,  1 }
};

static juce::NormalisableRange<float> logRange (float a, float b)
{
    return { a, b,
             [] (float s, float e, float v) { return s * std::pow (e / s, v); },
             [] (float s, float e, float x) { return std::log (x / s) / std::log (e / s); } };
}

juce::AudioProcessorValueTreeState::ParameterLayout Processor808::createLayout()
{
    using F = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<F> (juce::ParameterID { "tune", 1 }, "Tune", juce::NormalisableRange<float> (-12, 12, 1), 0));
    l.add (std::make_unique<F> (juce::ParameterID { "glide", 1 }, "Glide", logRange (10, 600), 140));
    l.add (std::make_unique<F> (juce::ParameterID { "decay", 1 }, "Decay", logRange (0.15f, 5), 2.4f));
    l.add (std::make_unique<F> (juce::ParameterID { "punch", 1 }, "Punch", juce::NormalisableRange<float> (0, 100), 62));
    l.add (std::make_unique<F> (juce::ParameterID { "sub", 1 }, "Sub", juce::NormalisableRange<float> (0, 100), 80));
    l.add (std::make_unique<F> (juce::ParameterID { "tone", 1 }, "Tone", logRange (150, 14000), 3200));
    l.add (std::make_unique<F> (juce::ParameterID { "click", 1 }, "Click", juce::NormalisableRange<float> (0, 100), 30));
    l.add (std::make_unique<F> (juce::ParameterID { "penv", 1 }, "Pitch env", juce::NormalisableRange<float> (0, 24), 7));
    l.add (std::make_unique<F> (juce::ParameterID { "output", 1 }, "Output", juce::NormalisableRange<float> (-24, 6), -3));
    l.add (std::make_unique<F> (juce::ParameterID { "drive", 1 }, "Drive", juce::NormalisableRange<float> (0, 100), 48));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Mode", juce::StringArray { "MONO", "LEGATO" }, 0));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "type", 1 }, "Drive type", juce::StringArray { "CLIP", "TAPE", "FOLD" }, 0));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "glideOn", 1 }, "Glide on", true));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "octave", 1 }, "Octave", juce::StringArray { "-1", "0", "+1" }, 1));
    return l;
}

Processor808::Processor808()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AH808", createLayout())
{
    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    tuneP = raw ("tune"); glideP = raw ("glide"); decayP = raw ("decay"); punchP = raw ("punch"); subP = raw ("sub");
    toneP = raw ("tone"); clickP = raw ("click"); penvP = raw ("penv"); outP = raw ("output"); driveP = raw ("drive");
    modeP = raw ("mode"); typeP = raw ("type"); glideOnP = raw ("glideOn"); octP = raw ("octave");
    scope.assign (8192, 0.0f);
}

bool Processor808::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void Processor808::setCurrentProgram (int index)
{
    currentPreset = juce::jlimit (0, 5, index);
    const auto& pr = PRESETS[currentPreset];
    auto set = [this] (const char* id, float v) {
        if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };
    set ("tune", pr.tune); set ("glide", pr.glide); set ("decay", pr.decay); set ("punch", pr.punch); set ("sub", pr.sub);
    set ("tone", pr.tone); set ("click", pr.click); set ("penv", pr.penv); set ("output", pr.output); set ("drive", pr.drive);
    set ("type", (float) pr.type); set ("glideOn", pr.glideOn ? 1.0f : 0.0f); set ("mode", (float) pr.mode);
}

void Processor808::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const float fs = (float) sr;
    os.reset();
    os.initProcessing ((size_t) samplesPerBlock);
    setLatencySamples (0);
    zero.setSize (1, samplesPerBlock); zero.clear();
    shaped.setSize (1, samplesPerBlock);
    phStart.assign ((size_t) samplesPerBlock, 0.0); phInc.assign ((size_t) samplesPerBlock, 0.0);
    sine.assign ((size_t) samplesPerBlock, 0.0f); amp.assign ((size_t) samplesPerBlock, 0.0f); click.assign ((size_t) samplesPerBlock, 0.0f);

    clickHp.set (ah::HP, 1800, 1, fs); clickHp.reset();
    tone.reset();
    clickCoef = std::exp (-1.0f / (0.006f * fs));
    freq.jump (32.7f);
    ampStage = -1; ampV = 0; clickV = 0; clickLeft = 0; sinceTrig = 1 << 30;
    const float dr = p (driveP) / 100, sb = p (subP) / 100;
    drvG.jump (0.25f + dr * 0.75f); subG.jump (sb * (0.9f - dr * 0.3f));
    outG.jump (std::pow (10.0f, p (outP) / 20) * 0.8f); toneF.jump (p (toneP));
    held.clear();
}

void Processor808::noteOn (int note)
{
    const float fs = (float) sr;
    const int oct = juce::roundToInt (p (octP)) - 1;
    const int midi = note + 12 * oct + juce::roundToInt (p (tuneP));
    const float f = 440.0f * std::pow (2.0f, (midi - 69) / 12.0f);
    const float decay = p (decayP), punch = p (punchP) / 100, glideT = p (glideP) / 1000;
    const bool stillRinging = sinceTrig < (int) (lastDecay * 0.9f * fs);
    const bool doGlide = p (glideOnP) > 0.5f && held.size() > 1 && stillRinging;
    const bool legato = juce::roundToInt (p (modeP)) == 1 && doGlide;

    if (doGlide) freq.to (f, glideT / 3, fs);
    else { freq.v = f * std::pow (2.0f, p (penvP) / 12); freq.to (f, 0.012f + (1 - punch) * 0.06f, fs); }

    if (! legato)
    {
        ampV = std::max (1e-4f, ampV * 0.3f);
        const float peak = 1 + punch * 0.7f;
        rampN = std::max (1, (int) std::round (0.003 * sr));
        ampInc = (peak - ampV) / (float) rampN;
        settleEnd = (int) std::round (0.06 * sr);
        c1 = std::exp (-1.0f / ((0.02f + (1 - punch) * 0.03f) * fs));
        c2 = std::exp (-1.0f / ((decay / 4.5f) * fs));
        ampStage = 0; ampCount = 0;
        const float cl = p (clickP) / 100;
        if (cl > 0.01f) { clickV = cl * 0.9f; clickLeft = (int) (0.05 * sr); }
        sinceTrig = 0;
        lastDecay = decay;
    }
}

void Processor808::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if ((int) sine.size() < n)
    {
        // host sent a bigger block than announced: grow everything (rare)
        prepareToPlay (sr, n);
    }
    const float fs = (float) sr;

    // block-rate parameter targets (Web Audio used setTargetAtTime(.., 0.02))
    const float dr = p (driveP) / 100, sb = p (subP) / 100;
    drvG.to (0.25f + dr * 0.75f, 0.02f, fs);
    subG.to (sb * (0.9f - dr * 0.3f), 0.02f, fs);
    outG.to (std::pow (10.0f, p (outP) / 20) * 0.8f, 0.02f, fs);
    toneF.to (p (toneP), 0.02f, fs);
    const int type = juce::roundToInt (p (typeP));

    // pass 1: events, oscillator phase, envelopes
    auto it = midi.begin();
    for (int i = 0; i < n; ++i)
    {
        for (; it != midi.end() && (*it).samplePosition <= i; ++it)
        {
            const auto m = (*it).getMessage();
            if (m.isNoteOn()) { held.push_back (m.getNoteNumber()); noteOn (m.getNoteNumber()); }
            else if (m.isNoteOff()) { auto f = std::find (held.begin(), held.end(), m.getNoteNumber()); if (f != held.end()) held.erase (f); }
            else if (m.isAllNotesOff() || m.isAllSoundOff()) held.clear();
        }
        const double inc = (double) freq.next() / sr;
        phStart[(size_t) i] = phase; phInc[(size_t) i] = inc;
        sine[(size_t) i] = std::sin (2.0 * juce::MathConstants<double>::pi * phase);
        phase += inc; if (phase >= 1) phase -= 1;

        if (ampStage == 0) { ampV += ampInc; if (++ampCount >= rampN) ampStage = 1; }
        else if (ampStage == 1) { ampV = 1 + (ampV - 1) * c1; if (++ampCount >= settleEnd) ampStage = 2; }
        else if (ampStage == 2) { ampV *= c2; if (ampV < 1e-6f) { ampV = 0; ampStage = -1; } }
        amp[(size_t) i] = ampV;

        float c = 0;
        if (clickLeft > 0) { c = clickHp.process (rng.next()) * clickV; --clickLeft; }
        else c = clickHp.process (0.0f) * clickV;
        clickV *= clickCoef;
        click[(size_t) i] = c;
        if (sinceTrig < (1 << 30)) ++sinceTrig;
    }

    // pass 2: the waveshaper runs at 4x — the sine is generated directly at the high rate
    {
        juce::dsp::AudioBlock<float> zb (zero.getArrayOfWritePointers(), 1, (size_t) n);
        auto up = os.processSamplesUp (zb);
        float* u = up.getChannelPointer (0);
        const int f = (int) os.getOversamplingFactor();
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < f; ++j)
            {
                const double ph = phStart[(size_t) i] + phInc[(size_t) i] * j / f;
                u[i * f + j] = ah::shape ((float) std::sin (2.0 * juce::MathConstants<double>::pi * ph), type, dr);
            }
        juce::dsp::AudioBlock<float> sbk (shaped.getArrayOfWritePointers(), 1, (size_t) n);
        os.processSamplesDown (sbk);
    }

    // pass 3: tone, mix, envelope, click, output
    const float* sh = shaped.getReadPointer (0);
    float* oL = buffer.getWritePointer (0);
    float* oR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    int sw = scopeW.load();
    for (int i = 0; i < n; ++i)
    {
        const float tf = toneF.next();
        if ((i & 15) == 0) tone.set (ah::LP, tf, 0.8f, fs);
        const float body = tone.process (sh[i]) * drvG.next() + sine[(size_t) i] * subG.next();
        const float y = (body * amp[(size_t) i] + click[(size_t) i]) * outG.next();
        const float a = std::abs (y);   // soft ceiling at 0 dBFS (the browser hard-clipped here)
        const float yc = a <= 0.9f ? y : std::copysign (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f), y);
        oL[i] = yc; if (oR) oR[i] = yc;
        scope[(size_t) sw] = y; sw = (sw + 1) & 8191;
    }
    scopeW = sw;
}

void Processor808::readScope (float* dst, int n) const
{
    n = std::min (n, 8192);
    const int w = scopeW.load();
    for (int i = 0; i < n; ++i) dst[i] = scope[(size_t) ((w - n + i + 8192) & 8191)];
}

void Processor808::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void Processor808::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto st = juce::ValueTree::fromXml (*xml);
        if (st.isValid())
        {
            currentPreset = juce::jlimit (0, 5, (int) st.getProperty ("preset", 0));
            apvts.replaceState (st);
        }
    }
}

juce::AudioProcessorEditor* Processor808::createEditor() { return new Editor808 (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Processor808(); }
