# Combination Modes Technical Notes

This document describes the implemented behavior of the active header combination
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

The modes are stored as a bit mask on each pattern slot. Bits `1 << 2` and
`1 << 5` are retired and are masked out when state is loaded.

| Bit      | Header | Name        | Processor constant               |
| -------- | ------ | ----------- | -------------------------------- |
| `1 << 0` | `X`    | Cross-Mod   | `combinationModeCrossModulation` |
| `1 << 7` | `Ca`   | Canon       | `combinationModeCanon`           |
| `1 << 8` | `Ri`   | Retro-Inv   | `combinationModeRetroInversion`  |
| `1 << 6` | `H`    | Hocket      | `combinationModeHocket`          |
| `1 << 1` | `T`    | Tendril     | `combinationModeTendril`         |
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
2. Canon
3. Retro-Inv
4. Tendril
5. Hocket
6. Echo
7. Weave

The fixed order is important because some modes transform attributes, Canon and
Retro-Inv add structural followers, Tendril appends small phraselets, Hocket
gates the expanded pool by carrier-row time windows, Echo can expand one event
into many events, and Weave can thin same-time collisions.
A stable order keeps combinations repeatable and makes pattern state
deterministic.

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

## Tendril Mode

Tendril appends a small scale-aware phraselet after gesture-anchor source events.
The first generated note curls by a neighbor scale degree, the next generated
note answers from the next active row's contour, and long strongly-moving sources
may add a quiet resolution.

For each source event:

```text
modRow = activeRows[(sourceActiveRowPosition + 1) % activeRowCount]
modStep = source.step % modRow.stepCount
movement = scaleDegreeDelta(modRow.notes[previousModStep], modRow.notes[modStep], scaleRoot, scaleModeIndex)
if movement == 0:
    movement = scaleDegreeDelta(modRow.notes[modStep], modRow.notes[nextModStep], scaleRoot, scaleModeIndex)
direction = movement < 0 ? -1 : movement > 0 ? 1 : deterministic alternate
```

Tendril only opens on gesture anchors. The gesture pulse is the larger of the
current pulse and the combination gesture floor, so short-pulse patterns do not
spray generated notes on every subdivision.

Tendril uses a rhythmic grid of one quarter of the gesture pulse. The curl starts
on the first grid slot (`25%` of the gesture pulse) and transposes the source by
one scale degree in the motion direction. The answer starts on the second grid
slot (`50%`), reading the next playable step from the next active row. If that
start collides with an existing event it nudges later by one full Tendril grid
step, so collision handling remains on-grid. The answer uses the modulator row's
MIDI channel, so multi-channel phrase rows can pass the response to a different
instrument. Answer pitches preserve the borrowed contour's pitch class but are
folded by octaves into the nearest register around the source note, bounded to
`source ± 7` semitones.

For long sources whose modulator moves by at least two scale degrees, Tendril may
add a quiet resolution on the third grid slot (`75%`). With at least three active
rows, the resolution borrows contour and MIDI channel from the following row;
with only two active rows, it falls back to a scale-degree return from the source.
Borrowed resolution pitches use the same nearest-register fold as answers.

Output effect:

- Adds sparse, generative phraselets rather than simple repeats.
- Keeps the source event intact.
- Uses the source row/channel for curls and borrowed row/channel for answers or
  resolutions.
- Uses reduced velocities and short gates so Tendril reads as connective tissue,
  not a full duplicate phrase.
- With fewer than two active rows, Tendril is a no-op.

## Hocket Mode

Hocket thins overlapping active rows into a monophonic handoff. It does not add
events; it re-times survivors onto a fixed beat-aligned slice grid and picks one
winner per slice.

The gesture pulse is the larger of the current pulse and the combination gesture
floor (`2` quarters), matching Canon and Tendril:

```text
combinationGesturePulse = max(pulse, 2)
sliceQuarters = combinationGesturePulse * 0.25
```

Each slice is one quarter of the gesture pulse. At quarter-note pulse this lands
on half-beat (eighth-note) boundaries. Row count controls which row owns each
slice, not how wide the slices are:

```text
sliceTargetRow = activeRows[slice % activeRowCount]
```

For each slice, every overlapping source event becomes a candidate. Candidates
need at least `50%` overlap with the slice or they are dropped. When multiple
candidates compete, the picker uses deterministic velocity-weighted selection
from the slice target row, slice index, and slice start time.

Output gates are capped at `85%` of the slice width. The emitted note-on time is
the later of the slice start and the winning source start, so row timing offsets
can place attacks slightly inside a slice.

Cross-Mod duration borrowing is deferred until Hocket selection when Canon and
Retro-Inv are off, so Hocket keeps source gate lengths while still applying
Cross-Mod pitch and velocity at output time.

Output effect:

- One note per slice across all active rows.
- Handoffs cycle through active rows on the same grid Tendril uses for curls and
  answers.
- With fewer than two active rows, Hocket is a no-op.

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

There is no wrap inside Echo. Events outside the current emit segment are
truncated by the scheduler range. Echo products are still built from the same
collection lookback window as carrier events so Shimmer can derive delayed taps
from lookback carriers (audio uses `collectionPpqStart`, not `schedulePpqStart`,
for the Echo inclusion filter).

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
array. When Shimmer is enabled, combination expanders (Canon, Retro-Inv, Tendril,
Echo) cap output at `combinedEventCapacity / (1 + maxShimmerTapsPerSource)` so
Shimmer can still append delayed octave-up taps to every surviving note. Without
that reservation, a full buffer leaves no room for Shimmer taps.

The preview mirrors the same cap against its `4096` note limit.

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
combination of the active mode bits can run together.

Examples:

### Cross-Mod + Tendril

1. Cross-Mod transforms pitch, velocity, and duration of carrier events.
2. Tendril curls from the transformed events and answers through the next row's
   contour.

Result: cross-routed phrase material with connective, scale-aware responses.

### Hocket + Tendril + Echo

1. Tendril appends sparse curls, answers, and optional resolutions on row carriers.
2. Hocket gates the expanded pool into gesture-grid slices (one winner per slice).
3. Echo multiplies only those rhythmically-selected survivors through the next
   active row.

Result: echoed material that still reads as a rhythmic handoff rather than a
fully simultaneous multi-row convolution or dense micro-cluster stack.

### Tendril + Echo + Weave

1. Tendril appends sparse curls, answers, and optional resolutions.
2. Echo expands the source and Tendril events.
3. Weave thins any generated same-time collisions to one winner.

Result: generative phraselets expanded into echoes while avoiding dense same-time
stacks.

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
Cross-Mod -> Canon -> Retro-Inv -> Tendril -> Hocket -> Echo -> Weave -> Octavizer -> Shimmer -> Note Bandpass
```

The preview intentionally differs in a few implementation details:

- It does not model C++ playback-random state for probability or humanization.
- It uses deterministic preview probability hashing already used by the
  existing preview.
- It caps visual Echo output at `4096` notes.
- It does not emit MIDI or manage pending note-offs.

The important musical outputs of the mode chain - how pitch is transformed, how
Hocket gates the tendril-expanded pool in time, how Tendril appends phraselets before thinning, how Echo expands
events, and how Weave chooses collision winners - are mirrored.
