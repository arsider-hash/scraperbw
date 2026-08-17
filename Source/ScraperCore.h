#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace scraper
{
constexpr int numSteps = 16;
constexpr int numSlices = 16;

struct Step
{
    uint8_t slice = 0;
    uint8_t probability = 255;
    uint8_t repeats = 1;
    int8_t pitchSemitones = 0;
    bool reverse = false;
};

class FastRandom
{
public:
    void seed (uint32_t value) noexcept { state = value == 0 ? 0x9e3779b9u : value; }
    uint32_t next() noexcept
    {
        auto x = state;
        x ^= x << 13; x ^= x >> 17; x ^= x << 5;
        return state = x;
    }
    bool hit (uint8_t probability) noexcept { return static_cast<uint8_t> (next() >> 24) <= probability; }
private:
    uint32_t state = 0x243f6a88u;
};

struct Trigger
{
    bool fired = false;
    Step step {};
};

class Sequencer
{
public:
    void reset() noexcept { currentStep = -1; lastAbsoluteStep = -1; random.seed (seedValue); }
    void setSeed (uint32_t s) noexcept { seedValue = s; random.seed (s); }
    std::array<Step, numSteps>& pattern() noexcept { return steps; }
    const std::array<Step, numSteps>& pattern() const noexcept { return steps; }

    Trigger advanceTo (int64_t absoluteStep) noexcept
    {
        if (absoluteStep == lastAbsoluteStep) return {};
        lastAbsoluteStep = absoluteStep;
        currentStep = static_cast<int> ((absoluteStep % numSteps + numSteps) % numSteps);
        const auto s = steps[static_cast<size_t> (currentStep)];
        return { random.hit (s.probability), s };
    }

    void regenerate (uint32_t seed, float chaos) noexcept
    {
        FastRandom r; r.seed (seed);
        chaos = std::clamp (chaos, 0.0f, 1.0f);
        for (auto& s : steps)
        {
            s.slice = static_cast<uint8_t> (r.next() % numSlices);
            s.probability = static_cast<uint8_t> (255.0f * (1.0f - chaos * ((r.next() & 255u) / 510.0f)));
            s.repeats = static_cast<uint8_t> (1u + (r.next() % (1u + static_cast<uint32_t> (chaos * 3.0f))));
            s.pitchSemitones = static_cast<int8_t> ((static_cast<int> (r.next() % 25u) - 12) * chaos);
            s.reverse = chaos > 0.25f && (r.next() & 3u) == 0u;
        }
    }

private:
    std::array<Step, numSteps> steps {};
    FastRandom random;
    uint32_t seedValue = 1;
    int currentStep = -1;
    int64_t lastAbsoluteStep = -1;
};

inline double semitonesToRatio (float semitones) noexcept
{
    return std::exp2 (static_cast<double> (semitones) / 12.0);
}
}

