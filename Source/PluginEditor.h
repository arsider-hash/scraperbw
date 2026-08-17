#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ScraperAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ScraperAudioProcessorEditor (ScraperAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override { repaint(); }
    ScraperAudioProcessor& processor;
    juce::TextButton load { "LOAD SAMPLE" };
    juce::TextButton sequence { "SEQ" }, regenerate { "REGENERATE" }, mutate { "MUTATE" };
    juce::Label status;
    juce::Slider pitch, stretch, probability, chaos;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sequenceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAttachment, stretchAttachment, probabilityAttachment, chaosAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScraperAudioProcessorEditor)
};
