#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ids { constexpr auto output="output", probability="probability", pitch="pitch", reverse="reverse", stretch="stretch", chaos="chaos", seed="seed", sequence="sequence"; }

ScraperAudioProcessor::ScraperAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "SCRAPER", makeLayout())
{
    formats.registerBasicFormats();
    sequencer.regenerate (1, 0.45f);
    publishPattern();
}

juce::AudioProcessorValueTreeState::ParameterLayout ScraperAudioProcessor::makeLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ids::output, "Output", -60.0f, 6.0f, -6.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ids::probability, "Probability", 0.0f, 1.0f, 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ids::pitch, "Pitch", -24.0f, 24.0f, 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterBool> (ids::reverse, "Reverse", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ids::stretch, "Stretch", 0.25f, 4.0f, 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ids::chaos, "Chaos", 0.0f, 1.0f, 0.45f));
    p.push_back (std::make_unique<juce::AudioParameterInt> (ids::seed, "Seed", 1, 65535, 1));
    p.push_back (std::make_unique<juce::AudioParameterBool> (ids::sequence, "Sequence", true));
    for (int i = 1; i <= 8; ++i) p.push_back (std::make_unique<juce::AudioParameterFloat> ("macro" + juce::String(i), "Macro " + juce::String(i), 0.0f, 1.0f, 0.0f));
    return { p.begin(), p.end() };
}

void ScraperAudioProcessor::prepareToPlay (double sr, int) { sampleRate = sr; sequencer.reset(); playing = false; }
bool ScraperAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }

void ScraperAudioProcessor::publishPendingSample() noexcept
{
    if (! pendingReady.load (std::memory_order_acquire) || ! pendingLock.tryEnter()) return;
    // Swap buffer ownership only. The old active allocation remains in
    // pendingSample and is reclaimed by the loader thread on the next decode.
    std::swap (activeSample, pendingSample);
    sliceBounds = pendingBounds;
    pendingReady.store (false, std::memory_order_release);
    pendingLock.exit();
}

void ScraperAudioProcessor::trigger (const scraper::Step& s) noexcept
{
    if (activeSample.getNumSamples() == 0) return;
    const int index = std::min<int> (s.slice, 15);
    playStart = sliceBounds[index]; playEnd = sliceBounds[index + 1];
    playbackReverse = s.reverse != (parameters.getRawParameterValue (ids::reverse)->load() >= 0.5f);
    activeStepPitch = static_cast<float> (s.pitchSemitones);
    refreshPlaybackRate();
    if (playbackReverse) playhead = static_cast<double> (playEnd - 1);
    else playhead = static_cast<double> (playStart);
    repeatsLeft = std::max (1, static_cast<int> (s.repeats)); playing = playEnd > playStart;
}

void ScraperAudioProcessor::refreshPlaybackRate() noexcept
{
    const float pitch = activeStepPitch + parameters.getRawParameterValue (ids::pitch)->load();
    const float stretch = parameters.getRawParameterValue (ids::stretch)->load();
    const double magnitude = scraper::semitonesToRatio (pitch) / std::max (0.25f, stretch);
    increment = playbackReverse ? -magnitude : magnitude;
}

void ScraperAudioProcessor::publishPattern() noexcept
{
    const auto& pattern = sequencer.pattern();
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        const auto& s = pattern[i];
        const uint32_t packed = static_cast<uint32_t> (s.slice)
                              | (static_cast<uint32_t> (s.probability) << 8u)
                              | (static_cast<uint32_t> (s.repeats) << 16u)
                              | (static_cast<uint32_t> (static_cast<uint8_t> (s.pitchSemitones)) << 20u)
                              | (static_cast<uint32_t> (s.reverse) << 28u);
        displayedSteps[i].store (packed, std::memory_order_release);
    }
}

void ScraperAudioProcessor::processBlock (juce::AudioBuffer<float>& out, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals; out.clear(); publishPendingSample();
    if (regenerateRequested.exchange (false, std::memory_order_acq_rel))
    {
        sequencer.regenerate (static_cast<uint32_t> (parameters.getRawParameterValue (ids::seed)->load()), parameters.getRawParameterValue (ids::chaos)->load());
        sequencer.reset(); publishPattern();
    }
    if (mutateRequested.exchange (false, std::memory_order_acq_rel))
    {
        const auto seed = static_cast<uint32_t> (parameters.getRawParameterValue (ids::seed)->load()) + 0x9e3779b9u;
        sequencer.mutate (seed, parameters.getRawParameterValue (ids::chaos)->load());
        publishPattern();
    }
    refreshPlaybackRate();
    const auto pos = getPlayHead() != nullptr ? getPlayHead()->getPosition() : std::optional<juce::AudioPlayHead::PositionInfo>{};
    const double bpm = pos && pos->getBpm() ? *pos->getBpm() : 120.0;
    const double ppq0 = pos && pos->getPpqPosition() ? *pos->getPpqPosition() : 0.0;
    const double ppqPerSample = bpm / (60.0 * sampleRate);
    const float globalProbability = parameters.getRawParameterValue (ids::probability)->load();
    const float gain = juce::Decibels::decibelsToGain (parameters.getRawParameterValue (ids::output)->load());

    auto renderOne = [&] (int frame)
    {
        const int64_t absoluteStep = static_cast<int64_t> (std::floor ((ppq0 + frame * ppqPerSample) * 4.0));
        auto event = parameters.getRawParameterValue (ids::sequence)->load() >= 0.5f ? sequencer.advanceTo (absoluteStep) : scraper::Trigger{};
        if (event.fired && globalProbability > 0.0f)
        {
            // Avoid a second PRNG and keep the sequence repeatable. The global
            // control acts as a deterministic density gate across the 16 steps.
            const auto phase = static_cast<float> ((absoluteStep % 16 + 16) % 16) / 15.0f;
            if (phase <= globalProbability) trigger (event.step);
        }
        if (! playing) return;
        const int i = static_cast<int> (playhead), j = std::clamp (i + (increment >= 0.0 ? 1 : -1), playStart, playEnd - 1);
        const float frac = static_cast<float> (std::abs (playhead - i));
        for (int ch = 0; ch < out.getNumChannels(); ++ch)
        {
            const auto* src = activeSample.getReadPointer (std::min (ch, activeSample.getNumChannels() - 1));
            out.setSample (ch, frame, gain * (src[i] + frac * (src[j] - src[i])));
        }
        playhead += increment;
        if (playhead < playStart || playhead >= playEnd)
        {
            if (--repeatsLeft > 0) playhead = increment < 0.0 ? playEnd - 1.0 : playStart;
            else playing = false;
        }
    };
    for (int frame = 0; frame < out.getNumSamples(); ++frame) renderOne (frame);
    for (const auto meta : midi) if (meta.getMessage().isNoteOn()) { auto s = scraper::Step{}; s.slice = static_cast<uint8_t> (meta.getMessage().getNoteNumber() % 16); trigger (s); }
}

void ScraperAudioProcessor::loadFileAsync (const juce::File& file)
{
    struct Job final : juce::ThreadPoolJob
    {
        Job (ScraperAudioProcessor& o, juce::File f) : ThreadPoolJob ("decode"), owner (o), file (std::move(f)) {}
        JobStatus runJob() override
        {
            std::unique_ptr<juce::AudioFormatReader> reader (owner.formats.createReaderFor (file));
            if (! reader) return jobHasFinished;
            const auto maxFrames = static_cast<juce::int64> (SCRAPER_MAX_SECONDS * reader->sampleRate);
            const int frames = static_cast<int> (std::min (reader->lengthInSamples, maxFrames));
            juce::AudioBuffer<float> decoded (std::min (2, static_cast<int> (reader->numChannels)), frames);
            reader->read (&decoded, 0, frames, 0, true, true);
            auto bounds = scraper::TransientDetector::detect (decoded);
            std::array<float, 256> preview {};
            std::array<float, 17> slicePreview {};
            if (decoded.getNumSamples() > 0)
            {
                const auto* source = decoded.getReadPointer (0);
                for (size_t i = 0; i < preview.size(); ++i)
                {
                    const int begin = static_cast<int> (i * static_cast<size_t> (decoded.getNumSamples()) / preview.size());
                    const int end = std::max (begin + 1, static_cast<int> ((i + 1) * static_cast<size_t> (decoded.getNumSamples()) / preview.size()));
                    float peak = 0.0f;
                    for (int j = begin; j < end; ++j) peak = std::max (peak, std::abs (source[j]));
                    preview[i] = peak;
                }
                for (size_t i = 0; i < slicePreview.size(); ++i)
                    slicePreview[i] = static_cast<float> (bounds[i]) / static_cast<float> (decoded.getNumSamples());
            }
            const juce::ScopedLock lock (owner.pendingLock);
            owner.pendingSample = std::move (decoded); owner.pendingBounds = bounds; owner.loadedName = file.getFileName();
            { const juce::ScopedLock previewGuard (owner.previewLock); owner.waveformPreview = preview; owner.slicePreview = slicePreview; }
            owner.pendingReady.store (true, std::memory_order_release);
            return jobHasFinished;
        }
        ScraperAudioProcessor& owner; juce::File file;
    };
    loader.removeAllJobs (true, 1000); loader.addJob (new Job (*this, file), true);
}

juce::String ScraperAudioProcessor::getLoadedName() const { return loadedName; }
std::array<float, 256> ScraperAudioProcessor::getWaveformPreview() const
{
    const juce::ScopedLock lock (previewLock);
    return waveformPreview;
}
std::array<float, 17> ScraperAudioProcessor::getSlicePreview() const
{
    const juce::ScopedLock lock (previewLock);
    return slicePreview;
}
void ScraperAudioProcessor::getStateInformation (juce::MemoryBlock& b) { if (auto xml = parameters.copyState().createXml()) copyXmlToBinary (*xml, b); }
void ScraperAudioProcessor::setStateInformation (const void* d, int n) { if (auto xml = getXmlFromBinary (d, n)) parameters.replaceState (juce::ValueTree::fromXml (*xml)); }
juce::AudioProcessorEditor* ScraperAudioProcessor::createEditor() { return new ScraperAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ScraperAudioProcessor(); }
