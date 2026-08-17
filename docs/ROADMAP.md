# SCRAPER V1 — vertical slices

1. **Audio spine (current):** async decode, 16 transient slices, host-clocked
   probability pattern, pitch/reverse/repeat/stretch and MIDI note triggering.
2. **Folder scraper:** catalogue 64 paths, next/random file policy, generation
   swaps only at a safe block boundary.
3. **Pattern editor:** 16 compact cells; slice/probability/repeat per step.
4. **Granular mode:** fixed pool of 8 grains, preallocated, linear envelopes;
   activated per trigger, never as a permanent background engine.
5. **Mutation and macros:** snapshot pattern; mutate from snapshot; map the eight
   macros to density, file, slice, pitch, repeat, reverse, stretch and grain.
6. **DualSense bridge:** separate tiny controller translator (HID to MIDI CC),
   so the plug-in remains controller-agnostic and Bitwig-safe.
7. **Release hardening:** pluginval, Bitwig soak test, CPU benchmarks at
   44.1/48/96 kHz and preset/state migration.

## Performance budgets

- Idle: effectively zero DSP beyond the host callback.
- Active: target below 1% of one modern laptop core at 48 kHz / 256 samples.
- Audio thread: zero heap allocation and zero blocking locks.
- RAM: one active decoded sample plus one pending swap during loading.
- Grain pool: fixed-size arrays only.

