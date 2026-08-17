#pragma once
#include <JuceHeader.h>
#include "ScraperCore.h"
#include "TransientDetector.h"

class ScraperAudioProcessor final : public juce::AudioProcessor
{
public:
    ScraperAudioProcessor();
    void prepareToPlay (double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    void loadFileAsync (const juce::File&);
    juce::AudioProcessorValueTreeState parameters;
    juce::String getLoadedName() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    void trigger (const scraper::Step&) noexcept;
    void publishPendingSample() noexcept;

    juce::AudioFormatManager formats;
    juce::AudioBuffer<float> activeSample;
    std::array<int, 17> sliceBounds {};
    juce::CriticalSection pendingLock;
    juce::AudioBuffer<float> pendingSample;
    std::array<int, 17> pendingBounds {};
    std::atomic<bool> pendingReady { false };
    juce::String loadedName;
    juce::ThreadPool loader { 1 };
    scraper::Sequencer sequencer;
    double sampleRate = 44100.0;
    double playhead = 0.0, increment = 1.0;
    int playStart = 0, playEnd = 0, repeatsLeft = 0;
    bool playing = false;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScraperAudioProcessor)
};

