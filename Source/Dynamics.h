#pragma once
#include <algorithm>
#include <cmath>

// Independent approximation: the original proprietary transfer/time constants
// are not specified by the user manual. No claim of DSP equivalence is made.
class Dynamics
{
public:
    float liftDb = 0, reductionDb = 0;
    void prepare(double sampleRate,float initialOutput=0) noexcept
    {
        rate = sampleRate;
        attack = std::exp(-1.0f / (0.005f * static_cast<float>(rate)));
        release = std::exp(-1.0f / (0.150f * static_cast<float>(rate)));
        smoothing = std::exp(-1.0f / (0.010f * static_cast<float>(rate)));
        reset(initialOutput);
    }
    void reset(float initialOutput=0) noexcept { envelope = 0; gainDb = initialOutput; liftDb = 0; reductionDb = 0; outputDb = initialOutput; }
    static float db(float x) noexcept { return 20.0f * std::log10(std::max(x, 1.0e-8f)); }
    static float amplitude(float x) noexcept { return std::pow(10.0f, x / 20.0f); }
    static float aboveKnee(float x) noexcept
    {
        constexpr float half = 3.0f;
        if (x <= -half) return 0;
        if (x >= half) return x;
        return (x + half) * (x + half) / (4 * half);
    }
    float tick(float linkedPeak, float low, float high, float output) noexcept
    {
        const auto coefficient = linkedPeak > envelope ? attack : release;
        envelope = coefficient * envelope + (1 - coefficient) * linkedPeak;
        const auto level = db(envelope);
        const auto lift = std::min(std::clamp(low, 0.0f, 48.0f),
                                   0.5f * aboveKnee(-18.0f - level));
        const auto reduction = high < 0 ? 0.5f * aboveKnee(level - high) : 0.0f;
        liftDb = smoothing*liftDb+(1-smoothing)*lift;
        reductionDb = smoothing*reductionDb+(1-smoothing)*reduction;
        outputDb = smoothing*outputDb+(1-smoothing)*output;
        gainDb = liftDb-reductionDb+outputDb;
        return amplitude(gainDb);
    }
private:
    double rate = 44100;
    float attack = 0, release = 0, smoothing = 0, envelope = 0, gainDb = 0, outputDb = 0;
};
