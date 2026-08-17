# SCRAPER 0.2

Lightweight JUCE/VST3 instrument for Bitwig 6: transient slicing, a 16-step
probability sequencer and destructive single-voice sample playback.

## V1.0 architecture

- Folder catalogue: at most 64 audio files; only the active file is decoded.
- Sample storage: one stereo `float` buffer, capped by `SCRAPER_MAX_SECONDS`
  (default 180 s). A three-minute stereo file at 48 kHz uses about 66 MiB.
- Audio thread: no locks, file I/O, allocation or logging.
- Playback: one stereo voice with pitch, reverse, repeat and linear-resampling
  stretch. New triggers intentionally choke the previous slice.
- Sequencer: 16 fixed steps; deterministic xorshift PRNG; host-synchronised.
- Transients: offline energy-flux scan; at most 16 slice starts.
- UI: deliberately minimal; host automation/MIDI mapping is the primary
  control surface.

This build adds immediate pitch response, ONE SHOT/SEQ switching, waveform
preview, pattern overview and live-safe mutate/regenerate requests. Granular
playback remains the next vertical slice. The parameter/state model already
reserves the eight macros used by a DualSense MIDI bridge.

## Windows build

Requirements for local builds: Visual Studio 2022 (Desktop C++) and CMake
3.22+. JUCE 8.0.11 is downloaded automatically when `JUCE_DIR` is omitted.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=C:/SDK/JUCE
cmake --build build --config Release
```

The VST3 is copied to the current user's VST3 folder by JUCE. In Bitwig, rescan
plug-ins, add **SCRAPER**, then map controller CCs to Macro 1..8.

## CPU/RAM rules

Do not add work to `processBlock` that allocates, locks, scans a directory or
decodes audio. Expensive analysis belongs to the loader/background path. Keep
the one-voice choke behaviour until profiling proves polyphony is affordable.
