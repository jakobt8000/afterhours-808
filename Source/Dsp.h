// 808 · AFTERHOURS
// Small DSP toolkit mirroring the Web Audio nodes the browser prototype used.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace ah
{
constexpr float kPi = 3.14159265358979f;
enum FType { LP, HP };

struct Rng
{
    uint32_t s = 0x2545f491u;
    inline float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffff) / 8388608.0f - 1.0f; }
    inline float uni() { return (next() + 1.0f) * 0.5f; }
};

// RBJ biquad with Web Audio parameter meaning (LP/HP Q given in dB, like BiquadFilterNode)
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void set (FType type, float f, float qDb, float sr)
    {
        f = std::clamp (f, 10.0f, sr * 0.49f);
        const float w0 = 2 * kPi * f / sr, cw = std::cos (w0), sw = std::sin (w0);
        const float alpha = sw / (2 * std::pow (10.0f, qDb / 20.0f));
        float nb0, nb1, nb2;
        if (type == LP) { nb0 = (1 - cw) * 0.5f; nb1 = 1 - cw; nb2 = (1 - cw) * 0.5f; }
        else            { nb0 = (1 + cw) * 0.5f; nb1 = -(1 + cw); nb2 = (1 + cw) * 0.5f; }
        const float na0 = 1 + alpha, na1 = -2 * cw, na2 = 1 - alpha;
        b0 = nb0 / na0; b1 = nb1 / na0; b2 = nb2 / na0; a1 = na1 / na0; a2 = na2 / na0;
    }
    inline float process (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
};

// Web Audio setTargetAtTime-style smoother
struct Target
{
    float v = 0, target = 0, coef = 0;
    void to (float t, float tau, float sr) { target = t; coef = tau <= 0 ? 0 : std::exp (-1.0f / (tau * sr)); }
    void jump (float x) { v = target = x; }
    inline float next() { v = target + (v - target) * coef; return v; }
};

// The drive waveshaper, same curve formulas as the prototype's WaveShaperNode
enum DriveType { CLIP = 0, TAPE = 1, FOLD = 2 };
inline float shape (float x, int type, float drive)
{
    const float k = 1 + drive * drive * 30, xx = x * k;
    float y;
    if (type == TAPE) y = std::tanh (xx + 0.08f * drive * xx * xx) / std::tanh (k * 0.9f);
    else if (type == CLIP) y = std::clamp (xx * 1.1f, -1.0f, 1.0f);
    else y = std::sin (xx * kPi * 0.5f * (1 + drive));
    return std::clamp (y, -1.2f, 1.2f);
}
} // namespace ah
