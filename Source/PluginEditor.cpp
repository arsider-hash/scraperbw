#include "PluginEditor.h"

ScraperAudioProcessorEditor::ScraperAudioProcessorEditor (ScraperAudioProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    addAndMakeVisible (load); addAndMakeVisible (status); addAndMakeVisible (sequence); addAndMakeVisible (regenerate); addAndMakeVisible (mutate);
    status.setJustificationType (juce::Justification::centred);
    for (auto* slider : { &pitch, &stretch, &probability, &chaos })
    {
        addAndMakeVisible (*slider); slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 68, 18);
    }
    pitch.setName ("PITCH"); stretch.setName ("STRETCH"); probability.setName ("DENSITY"); chaos.setName ("CHAOS");
    sequence.setClickingTogglesState (true);
    sequenceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.parameters, "sequence", sequence);
    pitchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.parameters, "pitch", pitch);
    stretchAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.parameters, "stretch", stretch);
    probabilityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.parameters, "probability", probability);
    chaosAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.parameters, "chaos", chaos);
    regenerate.onClick = [this] { processor.requestRegenerate(); };
    mutate.onClick = [this] { processor.requestMutate(); };
    load.onClick = [this]
    {
        auto chooser = std::make_shared<juce::FileChooser> ("Load a sample", juce::File{}, "*.wav;*.aif;*.aiff;*.flac;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser] (const juce::FileChooser& c) { auto f = c.getResult(); if (f.existsAsFile()) { processor.loadFileAsync (f); status.setText (f.getFileName(), juce::dontSendNotification); } });
    };
    startTimerHz (20);
    setSize (760, 460);
}

void ScraperAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0b0b0d)); g.setColour (juce::Colour (0xffd8ff3e));
    g.setFont (juce::FontOptions (28.0f, juce::Font::bold)); g.drawText ("SCRAPER / 0.2", 24, 14, 712, 40, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff77777c)); g.setFont (12.0f); g.drawText ("16 SLICES  ·  ONE SHOT / SEQ  ·  ONE VOICE  ·  NO MERCY", 25, 52, 710, 24, juce::Justification::centredLeft);

    const auto wave = processor.getWaveformPreview();
    const auto slices = processor.getSlicePreview();
    const juce::Rectangle<float> waveArea (24.0f, 86.0f, 712.0f, 116.0f);
    g.setColour (juce::Colour (0xff151519)); g.fillRoundedRectangle (waveArea, 4.0f);
    juce::Path path; path.startNewSubPath (waveArea.getX(), waveArea.getCentreY());
    for (size_t i = 0; i < wave.size(); ++i)
    {
        const float x = waveArea.getX() + waveArea.getWidth() * static_cast<float> (i) / static_cast<float> (wave.size() - 1);
        const float h = wave[i] * waveArea.getHeight() * 0.46f;
        path.startNewSubPath (x, waveArea.getCentreY() - h); path.lineTo (x, waveArea.getCentreY() + h);
    }
    g.setColour (juce::Colour (0xffd8ff3e)); g.strokePath (path, juce::PathStrokeType (1.0f));
    g.setColour (juce::Colour (0x6677777c));
    for (int i = 1; i < 16; ++i) { const float x = waveArea.getX() + waveArea.getWidth() * slices[static_cast<size_t> (i)]; g.drawVerticalLine (static_cast<int> (x), waveArea.getY(), waveArea.getBottom()); }

    const float cellW = 43.0f;
    for (int i = 0; i < 16; ++i)
    {
        const uint32_t s = processor.getPackedStep (i);
        const int slice = static_cast<int> (s & 0xffu), probabilityValue = static_cast<int> ((s >> 8u) & 0xffu), repeats = static_cast<int> ((s >> 16u) & 0x0fu);
        juce::Rectangle<float> cell (24.0f + i * (cellW + 1.0f), 220.0f, cellW, 64.0f);
        g.setColour (juce::Colour (0xff202025).interpolatedWith (juce::Colour (0xffd8ff3e), probabilityValue / 510.0f)); g.fillRoundedRectangle (cell, 3.0f);
        g.setColour (juce::Colours::white); g.setFont (11.0f); g.drawText (juce::String (slice + 1), cell.removeFromTop (36.0f), juce::Justification::centred);
        g.setColour (juce::Colour (0xff0b0b0d)); g.drawText ("x" + juce::String (repeats), cell, juce::Justification::centred);
    }
    g.setColour (juce::Colour (0xff77777c)); g.setFont (10.0f);
    for (auto* slider : { &pitch, &stretch, &probability, &chaos }) g.drawText (slider->getName(), slider->getX(), slider->getY() - 15, slider->getWidth(), 14, juce::Justification::centred);
}

void ScraperAudioProcessorEditor::resized()
{
    load.setBounds (24, 302, 135, 40); status.setBounds (166, 302, 220, 40);
    sequence.setBounds (398, 302, 92, 40); regenerate.setBounds (498, 302, 112, 40); mutate.setBounds (618, 302, 118, 40);
    pitch.setBounds (24, 370, 150, 76); stretch.setBounds (187, 370, 150, 76); probability.setBounds (350, 370, 150, 76); chaos.setBounds (513, 370, 150, 76);
}
