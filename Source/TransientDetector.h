#pragma once
#include <JuceHeader.h>
#include <array>
#include <algorithm>
#include <cmath>

namespace scraper
{
class TransientDetector
{
public:
    static std::array<int, 17> detect (const juce::AudioBuffer<float>& audio)
    {
        std::array<int, 17> bounds {};
        const int n = audio.getNumSamples();
        bounds[16] = n;
        if (n < 1024 || audio.getNumChannels() == 0)
        {
            for (int i = 1; i < 16; ++i) bounds[i] = n * i / 16;
            return bounds;
        }

        constexpr int hop = 256;
        constexpr int minGapFrames = 6;
        struct Peak { float flux; int sample; };
        std::array<Peak, 64> peaks {};
        int peakCount = 0;
        float previous = 0.0f;
        const auto* x = audio.getReadPointer (0);

        for (int pos = 0; pos + hop <= n; pos += hop)
        {
            float energy = 0.0f;
            for (int j = 0; j < hop; j += 4) energy += std::abs (x[pos + j]);
            const float flux = std::max (0.0f, energy - previous);
            previous = energy;
            if (peakCount < static_cast<int> (peaks.size())) peaks[peakCount++] = { flux, pos };
            else
            {
                auto weakest = std::min_element (peaks.begin(), peaks.end(), [] (auto a, auto b) { return a.flux < b.flux; });
                if (flux > weakest->flux) *weakest = { flux, pos };
            }
        }

        std::sort (peaks.begin(), peaks.begin() + peakCount, [] (auto a, auto b) { return a.flux > b.flux; });
        std::array<int, 15> selected {};
        int count = 0;
        for (int i = 0; i < peakCount && count < 15; ++i)
        {
            bool farEnough = peaks[i].sample > hop * minGapFrames;
            for (int j = 0; j < count; ++j) farEnough &= std::abs (selected[j] - peaks[i].sample) >= hop * minGapFrames;
            if (farEnough) selected[count++] = peaks[i].sample;
        }
        std::sort (selected.begin(), selected.begin() + count);
        bounds[0] = 0;
        for (int i = 1; i < 16; ++i)
            bounds[i] = i <= count ? selected[i - 1] : n * i / 16;
        std::sort (bounds.begin(), bounds.end());
        return bounds;
    }
};
}

