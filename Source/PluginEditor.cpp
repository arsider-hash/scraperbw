#include "PluginEditor.h"

ScraperAudioProcessorEditor::ScraperAudioProcessorEditor (ScraperAudioProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    addAndMakeVisible (load); addAndMakeVisible (status);
    status.setJustificationType (juce::Justification::centred);
    load.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Load a sample", juce::File{}, "*.wav;*.aif;*.aiff;*.flac;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& c) { auto f = c.getResult(); if (f.existsAsFile()) { processor.loadFileAsync (f); status.setText (f.getFileName(), juce::dontSendNotification); } });
    };
    setSize (520, 180);
}

void ScraperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0b0b0d)); g.setColour (juce::Colour (0xffd8ff3e));
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold)); g.drawText ("SCRAPER / V1", 24, 18, 472, 40, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff77777c)); g.setFont (12.0f); g.drawText ("16 SLICES  ·  16 STEPS  ·  ONE VOICE  ·  NO MERCY", 25, 57, 470, 24, juce::Justification::centredLeft);
}

void ScraperAudioProcessorEditor::resized()
{
    load.setBounds (24, 100, 160, 44); status.setBounds (198, 100, 298, 44);
}

