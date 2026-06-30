# Combination Modes Technical Notes

This document describes the implemented behavior of the six header combination
modes in MIDI Phrases. It focuses on how phrase rows are converted into
normalized MIDI events, how each mode transforms those events, and what MIDI
output is produced.

The implementation lives in two places:

- Audio output: `PluginProcessor::processCombinedScheduledRange()` in
  `source/PluginProcessor.cpp`
- Piano-roll preview: `applyCombinationModes()` in `ui/src/phraseSchedule.js`

The preview intentionally mirrors the same mode order and core transforms as the
audio scheduler. The preview remains deterministic and visual-only; actual MIDI
timing, probability, humanization, loop handling, and note output are owned by
the C++ processor.

## Mode Identity

The six modes are stored as a bit mask on each pattern slot.

| Bit      | Header | Name        | Processor constant               |
| -------- | ------ | ----------- | -------------------------------- |
| `1 << 0` | `X`    | Cross-Mod   | `combinationModeCrossModulation` |
| `1 << 5` | `R`    | Round Robin | `combinationModeRoundRobin`      |
| `1 << 1` | `B`    | Bloom       | `combinationModeBloom`           |
| `1 << 2` | `C`    | Counter     | `combinationModeCounter`         |
| `1 << 3` | `E`    | Echo        | `combinationModeMultiplyEcho`    |
| `1 << 4` | `W`    | Weave       | `combinationModeWeave`           |

The mask is pattern state, not global UI state. Copying a pattern copies its
mode mask, and selecting a different pattern selects that pattern's mode
combination. The mask is serialized in plugin state as `combinationModeMask`.

When the mask is `0`, the processor still uses the combined scheduler but skips
the mode transforms. Octavizer, shimmer, and note bandpass always run through the
same combined pipeline as the piano-roll preview.

## Normalized Event Model

The combined scheduler first converts all unmuted active rows into a fixed array
of normalized note events:

```cpp
struct CombinedNoteEvent
{
    double ppq;
    double gateQuarters;
    int row;
    int step;
    int channel;
    int note;
    int velocity;
};
```

The JavaScript preview uses the equivalent shape:

```js
{
  (start, end, midi, velocity, row, step);
}
```

An active row is one that is not row-muted, has at least one step, and has a
positive cycle length. Step-level skipped and muted state is still applied after
row activation:

- Skipped steps do not create triggers.
- Muted steps do not create audio events in the C++ scheduler.
- Velocity `0` steps do not create events.
- Duration `0` steps do not create events.

The initial event collection still honors row timing offset, step timing
multiplier, step cycle/cycle offset, step probability, step duration, MIDI
channel, and velocity humanization.

## Trigger Collection

For each active row, the scheduler computes step trigger positions in PPQ:

```text
triggerPpq = cycleIndex * rowCycleLength + stepStartInCycle + rowTimingOffset
```

Only triggers inside the current scheduled range are collected:

```text
schedulePpqStart <= triggerPpq < schedulePpqEnd
```

Each row has a fixed scratch trigger buffer. The combined event output also uses
fixed arrays, so the audio thread does not allocate while rendering. Current
combined capacity is `1024` events per scheduled range.

Events are sorted by:

```text
ppq, then row, then step
```

This sorted list is the input to the mode chain.

## Processing Order

Modes can be enabled in any combination, but they always execute in this fixed
order:

1. Cross-Mod
2. Round Robin
3. Bloom
4. Counter
5. Echo
6. Weave

The fixed order is important because some modes transform attributes, Round Robin
gates events by carrier-row time windows before additive modes run, some modes
append new events, Echo can expand one event into many events, and Weave can thin
same-time collisions. A stable order keeps combinations repeatable and makes
pattern state deterministic.

## Cross-Mod Mode

Cross-Mod transforms note attributes by reading from the other active rows. It
does not create or remove events.

For each event, the scheduler finds that event's row position in the active row
list. With four rows, if the event came from active row position `i`, the source
rows are:

```text
pitchRow    = activeRows[(i + 1) % activeRowCount]
velocityRow = activeRows[(i + 2) % activeRowCount]
durationRow = activeRows[(i + 3) % activeRowCount]
```

The event's own row remains the carrier for timing and MIDI channel. Other rows
provide transformed pitch, velocity, and duration.

### Pitch Rule

The pitch row is interpreted as scale-degree motion relative to its first note,
using the active pattern scale (root + mode from pattern state):

```text
pitchStep = event.step % pitchRow.stepCount
pitchDegreeDelta = scaleDegreeDelta(pitchRow.notes[0], pitchRow.notes[pitchStep], scaleRoot, scaleModeIndex)
event.note = clampMidi(transposeMidiByScaleDegrees(event.note, pitchDegreeDelta, scaleRoot, scaleModeIndex))
```

This keeps the carrier row recognizable while using another row as a contour
shape. Cross-Mod uses the same scale-degree helpers as Echo.

Scale changes are published to the audio thread as `SetPatternScale` commands so
Cross-Mod reads the same root/mode as the UI after queued pattern rebuilds.

In Chromatic mode each semitone counts as one scale degree, so Cross-Mod matches
the previous chromatic interval behavior. In diatonic and other modes, transformed
pitches stay in the current scale.

Example (C major, carrier C3 = 48, pitch row E4 → F4 at step 1):

```text
pitchDegreeDelta = +1 (E4 to F4)
output note:      D3 (48 + one scale degree)
```

### Velocity Rule

The velocity row directly supplies the output velocity:

```text
velocityStep = event.step % velocityRow.stepCount
event.velocity = clamp(velocityRow.velocity[velocityStep], 1, 127)
```

The C++ scheduler applies velocity humanization before Cross-Mod. If Cross-Mod
is enabled, the routed velocity replaces the collected event velocity.

### Duration Rule

The duration row supplies the output gate length:

```text
durationStep = event.step % durationRow.stepCount
event.gateQuarters =
    durationRow.stepLengthQuarters[durationStep]
    * durationRow.durationFraction[durationStep]
```

If the routed duration is not positive, the previous gate length is retained.

Output effect:

- Carrier row keeps timing and MIDI channel.
- Pitch can be shifted by another row's interval contour.
- Velocity and gate length can come from different rows.
- With fewer than two active rows, Cross-Mod is a no-op.

## Round Robin Mode

Round Robin gates carrier events into rotating time windows across active rows.
It runs after Cross-Mod and before Bloom, Counter, and Echo so additive modes
only operate on events allowed in the current lane.

The gesture pulse is the larger of the current pulse and the combination gesture
floor (`2` quarters). Each segment is that pulse length. Active rows rotate in
index order:

- **Exclusive segment** (first `75%`): only events from the current carrier row
  pass through.
- **Overlap segment** (last `25%`): events from the current and next carrier row
  are eligible; if both fire at the same PPQ, one winner is chosen with the same
  velocity-weighted deterministic hash used by Weave.

Output effect:

- Temporal alternation between phrase rows before ornaments, counters, and
  echoes are generated.
- Cross-Mod may still borrow pitch, velocity, and duration contours from other
  rows while those rows are rhythmically silent.
- With fewer than two active rows, Round Robin is a no-op.

## Bloom Mode

Bloom appends scale-neighbor ornaments after source events. It reads the next
active row as a motion source and uses that row's previous-to-current step motion
to choose the ornament direction.

For each source event:

```text
modRow = activeRows[(sourceActiveRowPosition + 1) % activeRowCount]
modStep = source.step % modRow.stepCount
movement = scaleDegreeDelta(modRow.notes[previousModStep], modRow.notes[modStep], scaleRoot, scaleModeIndex)
direction = movement < 0 ? -1 : 1
```

Bloom only opens on gesture anchors. The gesture pulse is the larger of the
current pulse and the combination gesture floor, so short-pulse patterns do not
spray ornaments on every subdivision.

The first ornament starts one quarter of the gesture pulse after the source. If
the source event is long enough and the modulator moves by at least two scale
degrees, Bloom adds a return ornament halfway through the gesture pulse.

Output effect:

- Adds scale-aware neighbor tones around source events.
- Keeps source timing and MIDI channel.
- Uses reduced ornament velocities (`65%` for the first ornament, `50%` for the
  return ornament).
- With fewer than two active rows, Bloom is a no-op.

## Counter Mode

Counter appends an offbeat response note after each source event. It reads the
next active row and uses the following step as the response source:

```text
modRow = activeRows[(sourceActiveRowPosition + 1) % activeRowCount]
modStep = (source.step + 1) % modRow.stepCount
```

Skipped, muted, or zero-velocity modulator steps do not create responses.

The response starts halfway through the combination gesture pulse. If that start
time collides with an existing event, Counter nudges it later by one eighth of
the gesture pulse; if it still collides, the response is skipped.

Output effect:

- Adds offbeat call-and-response notes.
- Uses the next row's scale-degree motion for response pitch.
- Uses a short response gate: the smaller of half the source gate and `37.5%` of
  the gesture pulse.
- With fewer than two active rows, Counter is a no-op.

## Echo Mode

Echo is the implemented Multiply/convolution mode. It expands each carrier event
against the next active row's step pattern.

For each event:

```text
modRow = activeRows[(carrierActiveRowPosition + 1) % activeRowCount]
```

Then each valid step in `modRow` creates one derived event.

Invalid modulator steps are skipped when:

- The step is skipped.
- The step is muted.
- The step velocity is `0`.
- The computed modulator gate is not positive.

### Time Rule

The modulator row's step start is added to the carrier event time:

```text
output.ppq = carrier.ppq + modRow.stepStartQuarters[modStep]
```

The event is kept only if it lands inside the current scheduled range:

```text
schedulePpqStart <= output.ppq < schedulePpqEnd
```

There is no wrap inside Echo. Events outside the current segment are truncated by
the scheduler range.

### Pitch Rule

The modulator row is interpreted as scale-degree motion relative to its first
note, using the active pattern scale (root + mode from pattern state):

```text
modDegreeDelta = scaleDegreeDelta(modRow.notes[0], modRow.notes[modStep], scaleRoot, scaleModeIndex)
output.note = clampMidi(transposeMidiByScaleDegrees(carrier.note, modDegreeDelta, scaleRoot, scaleModeIndex))
```

Scale changes are published to the audio thread as `SetPatternScale` commands so
Echo reads the same root/mode as the UI after queued pattern rebuilds (for example
after a pulse change).

In Chromatic mode each semitone counts as one scale degree, so Echo matches the
previous chromatic interval behavior. In diatonic and other modes, generated
echoes stay in the current scale.

### Velocity Rule

Echo averages carrier and modulator velocities:

```text
output.velocity = clamp((carrier.velocity + modRow.velocity[modStep]) / 2, 1, 127)
```

### Duration Rule

Echo uses the shorter of the carrier gate and modulator gate:

```text
modGate = modRow.stepLengthQuarters[modStep] * modRow.durationFraction[modStep]
output.gateQuarters = min(carrier.gateQuarters, modGate)
```

### Density Limit

Echo can grow quickly: one carrier event multiplied by `N` modulator steps yields
up to `N` output events. The audio scheduler writes into a fixed `1024` event
array. Once that array is full, additional Echo output for the current scheduled
range is dropped.

The preview has a separate visual cap of `4096` notes.

Output effect:

- Canons, ratchets, arpeggio clouds, and interval echoes.
- Generated notes keep the carrier row's MIDI channel.
- Generated `row` metadata remains the carrier row; generated `step` metadata is
  set to the modulator step.
- With fewer than two active rows, Echo is a no-op.

## Weave Mode

Weave chooses one winner when multiple events share the same PPQ start time. It
does not alter single-event groups.

Events are grouped by PPQ start. For each group:

- If the group contains one event, keep it.
- If the group contains multiple events, choose one event using deterministic
  velocity-weighted selection.

The weight for each candidate is:

```text
weight = max(1, event.velocity)
```

The picker uses a deterministic hash of the first grouped event:

```text
hash(row, step, ppq) % totalWeight
```

It then walks the group cumulatively until the weighted bucket is found.

This means Weave is probabilistic in feel but deterministic in playback. The
same phrase data at the same PPQ produces the same winner. It is not the same as
the existing per-step probability control, which can vary in the C++ playback
random stream.

Output effect:

- Same-time rows compete for a single output event.
- Higher velocity events are more likely to win.
- Since the choice is deterministic, repeated loops remain stable.
- Weave runs after Echo, so it can also thin Echo-generated collisions.

## Combining Modes

Because all modes consume and return the same normalized event structure, any
combination of the six mode bits can run together.

Examples:

### Cross-Mod + Bloom

1. Cross-Mod transforms pitch, velocity, and duration of carrier events.
2. Bloom appends scale-neighbor ornaments around the transformed events.

Result: cross-routed phrase material with ornamental motion.

### Cross-Mod + Round Robin + Echo

1. Cross-Mod transforms carrier pitch, velocity, and duration.
2. Round Robin gates carriers into alternating row time windows.
3. Echo multiplies only the surviving lane-local carriers through the next active
   row.

Result: echoed material per phrase turn rather than a fully simultaneous
multi-row convolution.

### Counter + Echo + Weave

1. Counter appends offbeat responses.
2. Echo expands the source and response events.
3. Weave thins any generated same-time collisions to one winner.

Result: call-and-response material expanded into echoes while avoiding dense
same-time stacks.

## MIDI Output Rules

After the mode chain completes, each event becomes MIDI note output:

1. Swing and timing humanization are applied to note-on time.
2. The event is converted from schedule PPQ to transport PPQ.
3. Transport PPQ is converted to sample offset.
4. `noteOn(channel, note, velocity)` is emitted.
5. A matching note-off is emitted in the same block or stored in a fixed pending
   note-off pool for a later block.

Combined-mode note-offs use a separate fixed pool from the original row-local
gate storage because Echo can generate more than one active note per row. The
current combined note-off capacity is `256`.

If the sample offset lands after the current buffer, the note-on is stored in
the existing pending note-on queue and emitted later.

## Loop and Pattern Switching

The combined scheduler receives the same scheduled ranges as the original row
scheduler. Loop brace mapping, transport discontinuity detection, and pattern or
loop-slot switching are handled before either scheduler runs.

When a row or mode state changes for the active audio pattern, the processor
requests a note flush. This prevents old generated notes from continuing after a
mode chain changes.

## Note Bandpass

After the mode chain, octavizer, and shimmer, each pattern applies a per-pattern
MIDI note bandpass before preview and audio output.

```text
bandpassLow <= event.note <= bandpassHigh
```

Notes outside the inclusive range are dropped. The bandpass does not edit phrase
data, step cards, or recording input. Default range is C1–C7 per pattern.

## Preview Parity

The UI preview follows the same mode order:

```text
Cross-Mod -> Round Robin -> Bloom -> Counter -> Echo -> Weave -> Octavizer -> Shimmer -> Note Bandpass
```

The preview intentionally differs in a few implementation details:

- It does not model C++ playback-random state for probability or humanization.
- It uses deterministic preview probability hashing already used by the
  existing preview.
- It caps visual Echo output at `4096` notes.
- It does not emit MIDI or manage pending note-offs.

The important musical outputs of the mode chain - how pitch is transformed, how
Round Robin gates rows in time, how Bloom and Counter append events, how Echo
expands events, and how Weave chooses collision winners - are mirrored.
