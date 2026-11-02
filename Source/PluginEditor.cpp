// 808 · AFTERHOURS
// Layout ported from the browser test (Y2): stippled scrolling waveform | big DRIVE knob | preset bar + 4 tiles,
// and a row of 9 dotted knobs underneath. Drawn at 1120 x 390 and scaled to 80% (same as the test panel).
#include "PluginEditor.h"
#include "Look.h"

namespace ui808
{
using namespace ahui;

static juce::String signedInt (float x, const char* unit) { const int v = juce::roundToInt (x); return (v > 0 ? "+" : "") + juce::String (v) + unit; }

// ---------- Knob ----------
Knob::Knob (juce::RangedAudioParameter& p, juce::String l, std::function<juce::String (float)> f, bool b)
    : param (p), attach (p, [this] (float v) { value = param.convertTo0to1 (v); repaint(); }), label (l), fmt (std::move (f)), big (b)
{
    attach.sendInitialUpdate();
}

void Knob::paint (juce::Graphics& g)
{
    g.fillAll (panel);
    const float w = (float) getWidth(), h = (float) getHeight();
    const juce::String txt = fmt (param.convertFrom0to1 (value));
    if (big)
    {
        const float size = 130, top = (h - 146) * 0.5f;
        drawKnob (g, w * 0.5f, top + size * 0.5f, size, 40, 55, value);
        g.setColour (ink); g.setFont (mono (11, 1));
        g.drawText (label + "  " + txt, juce::Rectangle<float> (0, top + size + 2, w, 14), juce::Justification::centred);
    }
    else
    {
        const float size = 96, top = (h - 128) * 0.5f;
        drawKnob (g, w * 0.5f, top + size * 0.5f, size, 29, 41, value);
        g.setColour (ink); g.setFont (mono (11, 1));
        g.drawText (label, juce::Rectangle<float> (0, top + size + 2, w, 14), juce::Justification::centred);
        g.setColour (muted); g.setFont (mono (11));
        g.drawText (txt, juce::Rectangle<float> (0, top + size + 18, w, 14), juce::Justification::centred);
    }
}

void Knob::mouseDown (const juce::MouseEvent&) { start = value; attach.beginGesture(); }
void Knob::mouseDrag (const juce::MouseEvent& e)
{
    value = juce::jlimit (0.0f, 1.0f, start - e.getDistanceFromDragStartY() / (e.mods.isShiftDown() ? 600.0f : 180.0f));
    attach.setValueAsPartOfGesture (param.convertFrom0to1 (value));
    repaint();
}
void Knob::mouseUp (const juce::MouseEvent&) { attach.endGesture(); }
void Knob::mouseDoubleClick (const juce::MouseEvent&) { attach.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue())); }
void Knob::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& d)
{
    value = juce::jlimit (0.0f, 1.0f, value + (d.deltaY > 0 ? 0.02f : -0.02f));
    attach.setValueAsCompleteGesture (param.convertFrom0to1 (value));
}

// ---------- Tile ----------
void Tile::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.fillAll (panel);
    if (lit) stipple (g, r);
    const float top = (r.getHeight() - 77) * 0.5f;
    g.setColour (ink.withAlpha (0.75f)); g.setFont (mono (10, 1));
    g.drawText (key(), juce::Rectangle<float> (0, top, r.getWidth(), 13), juce::Justification::centred);
    juce::Path ip = juce::Drawable::parseSVGPath (icon());
    ip.applyTransform (juce::AffineTransform::translation ((r.getWidth() - 64) * 0.5f, top + 23));
    g.setColour (ink); dotPath (g, ip, 3.0f, 2.4f);
    g.setFont (mono (13, 1));
    g.drawText (value(), juce::Rectangle<float> (0, top + 61, r.getWidth(), 16), juce::Justification::centred);
}

// ---------- Content ----------
static const char* ICON_MONO = "M2 22H14V6H50V22H62";
static const char* ICON_LEGATO = "M2 22H10V8H30V16H54V22H62";
static const char* ICON_TYPE[3] = { "M2 14L8 4H22L28 14L34 24H48L54 14L60 6",
                                    "M2 14C10 2 18 2 24 14S38 26 44 14S56 4 62 10",
                                    "M2 20L8 6L12 14L16 6L22 22L28 8L32 16L36 8L42 22L48 6L52 14L56 6L62 20" };
static const char* ICON_GLIDE = "M2 24C20 24 26 4 44 4H62";
static const char* ICON_NOGLIDE = "M2 14H62";
static const char* ICON_OCT = "M12 22H28M36 6H52";

Content::Content (Processor808& p) : proc (p)
{
    auto& ap = proc.apvts;
    auto add = [&] (const char* id, const char* label, std::function<juce::String (float)> f, bool big) {
        auto* k = knobs.add (new Knob (*ap.getParameter (id), label, std::move (f), big));
        addAndMakeVisible (k);
        return k;
    };
    add ("drive", "DRIVE", [] (float x) { return juce::String (juce::roundToInt (x)) + "%"; }, true)->setBounds (441, 0, 170, 240);
    struct K { const char* id; const char* label; std::function<juce::String (float)> f; };
    const K row[9] = {
        { "tune", "TUNE", [] (float x) { return signedInt (x, "ST"); } },
        { "glide", "GLIDE", [] (float x) { return juce::String (juce::roundToInt (x)) + "MS"; } },
        { "decay", "DECAY", [] (float x) { return juce::String (x, 1) + "S"; } },
        { "punch", "PUNCH", [] (float x) { return juce::String (juce::roundToInt (x)) + "%"; } },
        { "sub", "SUB", [] (float x) { return juce::String (juce::roundToInt (x)) + "%"; } },
        { "tone", "TONE", [] (float x) { return x >= 1000 ? juce::String (x / 1000, 1) + "K" : juce::String (juce::roundToInt (x)); } },
        { "click", "CLICK", [] (float x) { return juce::String (juce::roundToInt (x)) + "%"; } },
        { "penv", "P.ENV", [] (float x) { return juce::String (juce::roundToInt (x)) + "ST"; } },
        { "output", "OUTPUT", [] (float x) { return signedInt (x, "DB"); } }
    };
    const float cw = (W - 8) / 9.0f;
    for (int i = 0; i < 9; ++i)
    {
        const int x0 = juce::roundToInt (i * (cw + 1)), x1 = juce::roundToInt (i * (cw + 1) + cw);
        add (row[i].id, row[i].label, row[i].f, false)->setBounds (x0, 241, x1 - x0, H - 241);
    }

    auto choice = [&] (const char* id) { return dynamic_cast<juce::AudioParameterChoice*> (ap.getParameter (id)); };
    auto* mode = choice ("mode"); auto* type = choice ("type"); auto* oct = choice ("octave");
    auto* glideOn = dynamic_cast<juce::AudioParameterBool*> (ap.getParameter ("glideOn"));
    auto setChoice = [] (juce::AudioParameterChoice* c, int i) { c->beginChangeGesture(); *c = i; c->endChangeGesture(); };

    tiles[0].key = [] { return juce::String ("MODE"); };
    tiles[0].value = [mode] { return mode->getCurrentChoiceName(); };
    tiles[0].icon = [mode] { return juce::String (mode->getIndex() == 0 ? ICON_MONO : ICON_LEGATO); };
    tiles[0].onClick = [mode, setChoice] { setChoice (mode, 1 - mode->getIndex()); };

    tiles[1].key = [] { return juce::String ("DRIVE"); };
    tiles[1].value = [type] { return type->getCurrentChoiceName(); };
    tiles[1].icon = [type] { return juce::String (ICON_TYPE[type->getIndex()]); };
    tiles[1].onClick = [type, setChoice] { setChoice (type, (type->getIndex() + 1) % 3); };
    tiles[1].lit = true;

    tiles[2].key = [] { return juce::String ("GLIDE"); };
    tiles[2].value = [glideOn] { return juce::String (glideOn->get() ? "ON" : "OFF"); };
    tiles[2].icon = [glideOn] { return juce::String (glideOn->get() ? ICON_GLIDE : ICON_NOGLIDE); };
    tiles[2].onClick = [glideOn] { glideOn->beginChangeGesture(); *glideOn = ! glideOn->get(); glideOn->endChangeGesture(); };

    tiles[3].key = [] { return juce::String ("OCTAVE"); };
    tiles[3].value = [oct] { return oct->getCurrentChoiceName(); };
    tiles[3].icon = [] { return juce::String (ICON_OCT); };
    tiles[3].onClick = [oct, setChoice] { setChoice (oct, oct->getIndex() >= 2 ? 0 : oct->getIndex() + 1); };

    const float tw = (508 - 3) / 4.0f;
    for (int i = 0; i < 4; ++i)
    {
        const int x0 = 612 + juce::roundToInt (i * (tw + 1)), x1 = 612 + juce::roundToInt (i * (tw + 1) + tw);
        addAndMakeVisible (tiles[i]);
        tiles[i].setBounds (x0, 45, x1 - x0, 195);
    }

    screen = juce::Image (juce::Image::ARGB, 880, 480, true);
    restPattern();
    buf.assign (8192, 0.0f);
    lastMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

void Content::restPattern()
{
    juce::Graphics ig (screen);
    ig.addTransform (juce::AffineTransform::scale (2.0f));
    ig.fillAll (panel);
    ig.setColour (ink);
    for (float x = 0; x < 440; x += 0.5f)
    {
        const float t = x / 440; float env = 0.05f;
        for (int h = 0; h < 4; ++h) { const float dt = t - h / 4.0f - 0.02f; if (dt >= 0) env += std::exp (-dt * 5); }
        const float ph = std::sin (x * 0.55f);
        for (int k = 0; k < 3; ++k) { const float s = (ph * 0.8f + (rv() * 2 - 1) * 0.35f) * std::min (1.0f, env); ig.fillRect (x, 120 - s * 85, 1.15f, 1.15f); }
    }
}

void Content::timerCallback()
{
    // stippled waveform: scroll left 3 px, plot 90 random samples of the newest audio in the free column
    const double now = juce::Time::getMillisecondCounterHiRes();
    const double dtms = std::min (60.0, now - lastMs); lastMs = now;
    const int n = juce::jlimit (16, 8192, (int) (proc.getRate() * dtms / 1000.0));
    proc.readScope (buf.data(), n);
    screen.moveImageSection (0, 0, 6, 0, 874, 480);
    {
        juce::Graphics ig (screen);
        ig.addTransform (juce::AffineTransform::scale (2.0f));
        ig.setColour (panel); ig.fillRect (437.0f, 0.0f, 3.0f, 240.0f);
        ig.setColour (ink);
        for (int k = 0; k < 90; ++k)
        {
            const float s = buf[(size_t) std::min (n - 1, (int) (rv() * n))];
            ig.fillRect (437 + rv() * 3, juce::jlimit (1.0f, 238.0f, 120 - s * 100), 1.15f, 1.15f);
        }
    }
    repaint (0, 0, 440, 240);

    // tiles and preset name follow the parameters (automation, presets, host)
    auto& ap = proc.apvts;
    const int st = (int) ap.getRawParameterValue ("mode")->load() + 2 * (int) ap.getRawParameterValue ("type")->load()
                 + 8 * (int) ap.getRawParameterValue ("glideOn")->load() + 16 * (int) ap.getRawParameterValue ("octave")->load()
                 + 64 * proc.getCurrentProgram();
    if (st != lastState)
    {
        lastState = st;
        for (auto& t : tiles) t.repaint();
        repaint (612, 0, 508, 44);
    }
}

juce::String Content::presetText() const
{
    const int i = proc.getCurrentProgram();
    return juce::String (i + 1).paddedLeft ('0', 2) + "  " + Processor808::PRESETS[i].name;
}

void Content::paint (juce::Graphics& g)
{
    g.fillAll (line);                                              // 1 px separators show through the gaps
    g.drawImage (screen, juce::Rectangle<float> (0, 0, 440, 240));
    g.setColour (panel);
    g.fillRect (612, 0, 508, 44);
    g.setColour (ink);
    g.setFont (mono (16));
    g.drawText ("<", juce::Rectangle<float> (612, 0, 44, 44), juce::Justification::centred);
    g.drawText (">", juce::Rectangle<float> (1076, 0, 44, 44), juce::Justification::centred);
    g.setFont (mono (14, 3));
    g.drawText (presetText(), juce::Rectangle<float> (656, 0, 420, 44), juce::Justification::centred);
}

void Content::mouseUp (const juce::MouseEvent& e)
{
    const auto p = e.position;
    if (p.y < 0 || p.y >= 44) return;
    const int cur = proc.getCurrentProgram();
    if (p.x >= 612 && p.x < 656) proc.setCurrentProgram ((cur + 5) % 6);
    else if (p.x >= 1076 && p.x < 1120) proc.setCurrentProgram ((cur + 1) % 6);
    else return;
    proc.updateHostDisplay (juce::AudioProcessor::ChangeDetails().withProgramChanged (true));
    repaint (612, 0, 508, 44);
}
} // namespace ui808

Editor808::Editor808 (Processor808& p) : AudioProcessorEditor (p), content (p)
{
    addAndMakeVisible (content);
    setResizable (true, true);
    setResizeLimits (560, 195, 1680, 585);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui808::Content::W / ui808::Content::H);
    setSize (896, 312);
}

void Editor808::resized()
{
    content.setBounds (0, 0, ui808::Content::W, ui808::Content::H);
    content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) ui808::Content::W));
}
