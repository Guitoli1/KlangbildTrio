#pragma once

// Klangbild Trio – Drei-Band-Kompressor
// Frequenzweiche: Linkwitz-Riley 4. Ordnung (phasenrichtige Summe), das Tiefenband läuft zusätzlich durch
// den Allpass der oberen Trennung, damit alle drei Bänder zusammen wieder ein flaches Signal ergeben.

#include <array>
#include <cmath>
#include <algorithm>
#include "EQDsp.h"

namespace trio
{

struct LR4
{
    eq::Coeffs c;
    std::array<eq::BiquadState, 2> s;
    void reset() { for (auto& x : s) x.reset(); }
    float process (float x) { return s[1].process (c, s[0].process (c, x)); }
};

// Weiche für einen Kanal
class Crossover
{
public:
    void setFrequencies (double sr, double f1, double f2)
    {
        const double nyq = 0.45 * sr;
        f1 = std::min (f1, nyq); f2 = std::min (std::max (f2, f1 * 1.05), nyq);
        const auto lp1 = eq::makeLowPass (sr, f1, 0.70710678), hp1 = eq::makeHighPass (sr, f1, 0.70710678);
        const auto lp2 = eq::makeLowPass (sr, f2, 0.70710678), hp2 = eq::makeHighPass (sr, f2, 0.70710678);
        low.c = lp1; rest.c = hp1; mid.c = lp2; high.c = hp2; apLp.c = lp2; apHp.c = hp2;
    }
    void reset() { for (auto* f : { &low, &rest, &mid, &high, &apLp, &apHp }) f->reset(); }

    void process (float x, float& b0, float& b1, float& b2)
    {
        const float l = low.process (x);
        const float r = rest.process (x);
        b1 = mid.process (r);
        b2 = high.process (r);
        b0 = apLp.process (l) + apHp.process (l);   // Allpass der oberen Trennung
    }

private:
    LR4 low, rest, mid, high, apLp, apHp;
};

// Kompressor eines Bandes (Stereo verkoppelt), Pegelerkennung in dB mit weichem Knie
class Compressor
{
public:
    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset() { grDb = 0.0f; }

    void setParams (float thresholdDb, float ratio, float attackMs, float releaseMs)
    {
        thr = thresholdDb; slope = 1.0f - 1.0f / std::max (1.0f, ratio);
        att = (float) std::exp (-1.0 / (std::max (0.05, (double) attackMs) * 0.001 * sr));
        rel = (float) std::exp (-1.0 / (std::max (5.0, (double) releaseMs) * 0.001 * sr));
    }

    // gibt die Verstärkung (linear) für diesen Abtastwert zurück
    float process (float level)
    {
        const float inDb = level > 1.0e-6f ? 20.0f * std::log10 (level) : -120.0f;
        const float over = inDb - thr;
        float target;
        if (over <= -knee * 0.5f)      target = 0.0f;
        else if (over >= knee * 0.5f)  target = over * slope;
        else { const float k = over + knee * 0.5f; target = slope * k * k / (2.0f * knee); }
        grDb = target > grDb ? target + (grDb - target) * att : target + (grDb - target) * rel;
        return std::pow (10.0f, -grDb / 20.0f);
    }

    float reductionDb() const { return grDb; }

private:
    double sr = 48000.0;
    float thr = -20.0f, slope = 0.5f, att = 0.99f, rel = 0.999f, grDb = 0.0f;
    static constexpr float knee = 6.0f;
};

} // namespace trio
