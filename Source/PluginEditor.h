#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ScraperAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit ScraperAudioProcessorEditor (ScraperAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    ScraperAudioProcessor& processor;
    juce::TextButton load { "LOAD SAMPLE" };
    juce::Label status;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScraperAudioProcessorEditor)
};

