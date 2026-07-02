#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace
{
constexpr double pulseQuartersTable[] = { 0.5, 1.0, 2.0, 4.0 };
constexpr double combinationGesturePulseQuartersFloor = 2.0;
constexpr double roundRobinOverlapFraction = 0.25;
constexpr double hocketMinimumSliceOverlapFraction = 0.5;
constexpr double unisonOverlapWindowQuarters = 1.0 / 96.0;
constexpr double swingSubdivisionValues[] = { 0.25, 0.5, 1.0 };
constexpr double timingHumanizeScale = 0.2;
constexpr int phraseStateVersion = 25;

int clampStepProbability (const int probability)
{
    return juce::jlimit (0, PluginProcessor::maxStepProbabilityValue, probability);
}

int stepProbabilityFromStateProperty (const juce::var& property, const int stateVersion)
{
    const auto storedProbability = static_cast<int> (property);

    if (stateVersion < 22 && storedProbability == PluginProcessor::maxPercentValue)
        return PluginProcessor::defaultStepProbability;

    return clampStepProbability (storedProbability);
}

int clampSeedingPhraseLength (const int phraseLength)
{
    return juce::jlimit (PluginProcessor::minSeedingPhraseLength,
                         PluginProcessor::maxSeedingPhraseLength,
                         phraseLength);
}

int clampSeedingRangeSemitones (const int rangeSemitones)
{
    return juce::jlimit (PluginProcessor::minSeedingRangeSemitones,
                         PluginProcessor::maxSeedingRangeSemitones,
                         rangeSemitones);
}

int clampSeedingCenterMidi (const int centerMidi)
{
    if (centerMidi < PluginProcessor::minMidiNote)
        return PluginProcessor::defaultSeedingCenterMidi;

    return juce::jlimit (PluginProcessor::minMidiNote,
                         PluginProcessor::maxMidiNote,
                         centerMidi);
}

int clampSeedingPercent (const int value)
{
    return juce::jlimit (0, PluginProcessor::maxPercentValue, value);
}

int clampSeedingTimingMeanMultiplierIndex (const int multiplierIndex)
{
    return juce::jlimit (PluginProcessor::minSeedingTimingMeanMultiplierIndex,
                         PluginProcessor::maxSeedingTimingMeanMultiplierIndex,
                         multiplierIndex);
}

int clampSeedingSeed (const int seed)
{
    return juce::jmax (1, seed);
}

int clampSeedingAspectSeed (const int seed)
{
    return seed >= 1 ? juce::jmax (1, seed) : 0;
}

int clampSeedingRhythmStep (const int rhythmStep)
{
    return juce::jlimit (PluginProcessor::minSeedingRhythmStep,
                         PluginProcessor::maxSeedingRhythmStep,
                         rhythmStep);
}

PluginProcessor::SeedingRowState clampSeedingRowState (const PluginProcessor::SeedingRowState& rowState)
{
    PluginProcessor::SeedingRowState clamped;
    clamped.phraseLength = clampSeedingPhraseLength (rowState.phraseLength);
    clamped.centerMidi = clampSeedingCenterMidi (rowState.centerMidi);
    clamped.rangeSemitones = clampSeedingRangeSemitones (rowState.rangeSemitones);
    clamped.repetition = clampSeedingPercent (rowState.repetition);
    clamped.complexity = clampSeedingPercent (rowState.complexity);
    clamped.randomness = clampSeedingPercent (rowState.randomness);
    clamped.timingMeanMultiplierIndex =
        clampSeedingTimingMeanMultiplierIndex (rowState.timingMeanMultiplierIndex);
    clamped.timingVariance = clampSeedingPercent (rowState.timingVariance);
    clamped.symmetry = rowState.symmetry != 0 ? 1 : 0;
    clamped.seed = clampSeedingSeed (rowState.seed);
    clamped.repetitionSeed = clampSeedingAspectSeed (rowState.repetitionSeed);
    clamped.complexitySeed = clampSeedingAspectSeed (rowState.complexitySeed);
    clamped.randomnessSeed = clampSeedingAspectSeed (rowState.randomnessSeed);
    clamped.timingVarianceSeed = clampSeedingAspectSeed (rowState.timingVarianceSeed);
    return clamped;
}

int clampStepCycle (const int cycle)
{
    return juce::jlimit (PluginProcessor::minStepCycle, PluginProcessor::maxStepCycle, cycle);
}

int clampStepCycleMask (const int mask, const int cycle)
{
    const auto length = clampStepCycle (cycle);
    const auto bitCount = juce::jmin (length, 30);
    const auto maxMask = bitCount <= 0 ? 1 : ((1 << bitCount) - 1);

    return mask & maxMask;
}

int cycleMaskFromLegacyOffset (const int offset, const int cycle)
{
    const auto length = clampStepCycle (cycle);

    if (length <= 1)
        return 1;

    const auto phase = juce::jlimit (0, length - 1, offset);

    return 1 << juce::jmin (phase, 30);
}

bool cycleGateMatches (const int count, const int cycle, const int mask)
{
    const auto length = clampStepCycle (cycle);
    auto pattern = clampStepCycleMask (mask, length);

    if (pattern == 0)
        pattern = PluginProcessor::defaultStepCycleMask;

    if (length <= 1)
        return (pattern & 1) != 0;

    const auto phase = ((count % length) + length) % length;

    return ((pattern >> phase) & 1) != 0;
}

bool probabilityPassesDeterministic (const int step, const int triggerCount, const int probability)
{
    const auto chance = clampStepProbability (probability);

    if (chance >= PluginProcessor::maxStepProbabilityValue)
        return true;

    if (chance <= 0)
        return false;

    const auto hash = static_cast<std::uint32_t> (step) * 2654435761u
                      + static_cast<std::uint32_t> (triggerCount) * 1597334677u;

    return (hash % 100u) < static_cast<std::uint32_t> (chance);
}

double nextRandomUnitDouble (std::uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    return static_cast<double> (state) / static_cast<double> (UINT32_MAX);
}

float nextRandomUnit (std::uint32_t& state)
{
    return static_cast<float> (nextRandomUnitDouble (state));
}

bool isBloomGestureAnchor (const double ppq,
                           const double gateQuarters,
                           const double gesturePulseQuarters)
{
    constexpr auto epsilon = 1.0e-9;

    if (gesturePulseQuarters <= epsilon)
        return true;

    auto phase = ppq - std::floor ((ppq + epsilon) / gesturePulseQuarters)
                           * gesturePulseQuarters;

    if (phase < 0.0)
        phase += gesturePulseQuarters;

    if (phase >= gesturePulseQuarters - epsilon)
        phase = 0.0;

    const auto anchorWindow = juce::jmin (
        gesturePulseQuarters * 0.125,
        juce::jmax (gateQuarters * 0.25, gesturePulseQuarters * 0.0625));

    return phase <= anchorWindow + epsilon;
}

int clampPercent (const int percent)
{
    return juce::jlimit (0, PluginProcessor::maxPercentValue, percent);
}

int combinationModeBit (const int modeIndex)
{
    if (modeIndex < 0 || modeIndex >= PluginProcessor::combinationModeCount)
        return 0;

    return 1 << modeIndex;
}

int clampCombinationModeMask (const int mask)
{
    return mask & ((1 << PluginProcessor::combinationModeCount) - 1);
}

int clampScaleRoot (const int root)
{
    return juce::jlimit (0, PluginProcessor::scaleRootCount - 1, root);
}

int clampScaleModeIndex (const int modeIndex)
{
    return juce::jlimit (0, PluginProcessor::scaleModeCount - 1, modeIndex);
}

struct ScaleModeDefinition
{
    const int* intervals = nullptr;
    int count = 0;
};

constexpr int scaleModeChromaticIntervals[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
constexpr int scaleModeMajorIntervals[] = { 0, 2, 4, 5, 7, 9, 11 };
constexpr int scaleModeNaturalMinorIntervals[] = { 0, 2, 3, 5, 7, 8, 10 };
constexpr int scaleModeDorianIntervals[] = { 0, 2, 3, 5, 7, 9, 10 };
constexpr int scaleModePhrygianIntervals[] = { 0, 1, 3, 5, 7, 8, 10 };
constexpr int scaleModeLydianIntervals[] = { 0, 2, 4, 6, 7, 9, 11 };
constexpr int scaleModeMixolydianIntervals[] = { 0, 2, 4, 5, 7, 9, 10 };
constexpr int scaleModeLocrianIntervals[] = { 0, 1, 3, 5, 6, 8, 10 };
constexpr int scaleModeHarmonicMinorIntervals[] = { 0, 2, 3, 5, 7, 8, 11 };
constexpr int scaleModeMelodicMinorIntervals[] = { 0, 2, 3, 5, 7, 9, 11 };
constexpr int scaleModeMajorPentatonicIntervals[] = { 0, 2, 4, 7, 9 };
constexpr int scaleModeMinorPentatonicIntervals[] = { 0, 3, 5, 7, 10 };
constexpr int scaleModeBluesIntervals[] = { 0, 3, 5, 6, 7, 10 };
constexpr int scaleModeWholeToneIntervals[] = { 0, 2, 4, 6, 8, 10 };
constexpr int scaleModeDiminishedWholeHalfIntervals[] = { 0, 2, 3, 5, 6, 8, 9, 11 };
constexpr int scaleModeDiminishedHalfWholeIntervals[] = { 0, 1, 3, 4, 6, 7, 9, 10 };

constexpr std::array<ScaleModeDefinition, PluginProcessor::scaleModeCount> scaleModeDefinitions { {
    { scaleModeChromaticIntervals, 12 },
    { scaleModeMajorIntervals, 7 },
    { scaleModeNaturalMinorIntervals, 7 },
    { scaleModeDorianIntervals, 7 },
    { scaleModePhrygianIntervals, 7 },
    { scaleModeLydianIntervals, 7 },
    { scaleModeMixolydianIntervals, 7 },
    { scaleModeLocrianIntervals, 7 },
    { scaleModeHarmonicMinorIntervals, 7 },
    { scaleModeMelodicMinorIntervals, 7 },
    { scaleModeMajorPentatonicIntervals, 5 },
    { scaleModeMinorPentatonicIntervals, 5 },
    { scaleModeBluesIntervals, 6 },
    { scaleModeWholeToneIntervals, 6 },
    { scaleModeDiminishedWholeHalfIntervals, 8 },
    { scaleModeDiminishedHalfWholeIntervals, 8 },
} };

bool pitchClassInScale (const int midi, const int root, const int modeIndex)
{
    const auto& mode = scaleModeDefinitions[static_cast<size_t> (clampScaleModeIndex (modeIndex))];
    const auto pitchClass = juce::jlimit (0, 11, ((midi % 12) + 12) % 12);
    const auto relative = (pitchClass - clampScaleRoot (root) + 12) % 12;

    for (int index = 0; index < mode.count; ++index)
    {
        if (mode.intervals[index] == relative)
            return true;
    }

    return false;
}

int transposeMidiByScaleDegrees (const int note,
                                 const int degreeDelta,
                                 const int root,
                                 const int modeIndex)
{
    const auto delta = degreeDelta;
    auto current = juce::jlimit (0, 127, note);

    if (delta == 0)
        return current;

    const auto direction = delta > 0 ? 1 : -1;
    auto remaining = std::abs (delta);

    while (remaining > 0)
    {
        if ((direction > 0 && current >= 127) || (direction < 0 && current <= 0))
            return current;

        current += direction;

        if (pitchClassInScale (current, root, modeIndex))
            --remaining;
    }

    return current;
}

int scaleDegreeDelta (const int fromNote, const int toNote, const int root, const int modeIndex)
{
    const auto from = juce::jlimit (0, 127, fromNote);
    const auto to = juce::jlimit (0, 127, toNote);

    if (from == to)
        return 0;

    const auto direction = to > from ? 1 : -1;
    auto current = from;
    auto degrees = 0;

    while ((direction > 0 && current < to) || (direction < 0 && current > to))
    {
        if ((direction > 0 && current >= 127) || (direction < 0 && current <= 0))
            break;

        current += direction;

        if (pitchClassInScale (current, root, modeIndex))
            degrees += direction;
    }

    return degrees;
}

int echoNoteFromModStep (const int carrierNote,
                         const int modBaseNote,
                         const int modStepNote,
                         const int root,
                         const int modeIndex)
{
    return transposeMidiByScaleDegrees (carrierNote,
                                        scaleDegreeDelta (modBaseNote, modStepNote, root, modeIndex),
                                        root,
                                        modeIndex);
}

bool combinationModeEnabled (const int mask, const int modeIndex)
{
    const auto bit = combinationModeBit (modeIndex);
    return bit != 0 && (mask & bit) != 0;
}

std::pair<int, int> clampNoteBandpassBounds (int lowMidi, int highMidi)
{
    auto low = juce::jlimit (PluginProcessor::minMidiNote, PluginProcessor::maxMidiNote, lowMidi);
    auto high = juce::jlimit (PluginProcessor::minMidiNote, PluginProcessor::maxMidiNote, highMidi);

    if (low > high)
        std::swap (low, high);

    return { low, high };
}

int clampVelocityTiltPivotMidi (const int pivotMidi)
{
    return juce::jlimit (PluginProcessor::minMidiNote, PluginProcessor::maxMidiNote, pivotMidi);
}

int clampVelocityTiltAmount (const int amount)
{
    return juce::jlimit (PluginProcessor::minVelocityTiltAmount,
                         PluginProcessor::maxVelocityTiltAmount,
                         amount);
}

int clampGlobalTransposeSemitones (const int semitones)
{
    return juce::jlimit (PluginProcessor::minGlobalTransposeSemitones,
                         PluginProcessor::maxGlobalTransposeSemitones,
                         semitones);
}

int velocityTiltOutputVelocity (const int baseVelocity,
                                const int note,
                                const int pivotMidi,
                                const int amount)
{
    const auto clampedVelocity = juce::jlimit (1, 127, baseVelocity);
    const auto tilt = clampVelocityTiltAmount (amount);

    if (tilt == 0)
        return clampedVelocity;

    const auto octaveDistance =
        static_cast<double> (juce::jlimit (PluginProcessor::minMidiNote,
                                           PluginProcessor::maxMidiNote,
                                           note)
                             - clampVelocityTiltPivotMidi (pivotMidi))
        / 12.0;
    const auto delta = static_cast<int> (std::lround (octaveDistance * static_cast<double> (tilt)));

    return juce::jlimit (1, 127, clampedVelocity + delta);
}

int clampOctavizerRelativeVelocity (const int relativeVelocity)
{
    return juce::jlimit (PluginProcessor::minOctavizerRelativeVelocity,
                       PluginProcessor::maxOctavizerRelativeVelocity,
                       relativeVelocity);
}

int octavizerOutputVelocity (const int baseVelocity, const int relativeVelocity)
{
    return juce::jlimit (1, 127, baseVelocity + clampOctavizerRelativeVelocity (relativeVelocity));
}

int clampShimmerDelayMultiplierIndex (const int multiplierIndex)
{
    return juce::jlimit (0, PluginProcessor::stepTimingMultiplierCount - 1, multiplierIndex);
}

int clampShimmerFeedbackPercent (const int feedbackPercent)
{
    return juce::jlimit (PluginProcessor::minShimmerFeedbackPercent,
                       PluginProcessor::maxShimmerFeedbackPercent,
                       feedbackPercent);
}

int clampShimmerMixPercent (const int mixPercent)
{
    return juce::jlimit (PluginProcessor::minShimmerMixPercent,
                       PluginProcessor::maxShimmerMixPercent,
                       mixPercent);
}

int shimmerTapVelocity (const int sourceVelocity,
                        const int tap,
                        const int feedbackPercent,
                        const int mixPercent)
{
    const auto feedback =
        static_cast<double> (clampShimmerFeedbackPercent (feedbackPercent)) / 100.0;
    const auto mix = static_cast<double> (clampShimmerMixPercent (mixPercent)) / 100.0;

    return static_cast<int> (std::lround (
        static_cast<double> (sourceVelocity) * std::pow (feedback, tap) * mix));
}

std::uint32_t deterministicEventHash (const int row, const int step, const double ppq)
{
    auto value = static_cast<std::uint32_t> (row + 1) * 0x9E3779B9u
                 ^ static_cast<std::uint32_t> (step + 1) * 0x85EBCA6Bu
                 ^ static_cast<std::uint32_t> (std::llround (ppq * 960.0)) * 0xC2B2AE35u;
    value ^= value >> 16;
    value *= 0x7FEB352Du;
    value ^= value >> 15;
    value *= 0x846CA68Bu;
    value ^= value >> 16;
    return value;
}

int gcdInt (const int a, const int b)
{
    auto x = std::abs (a);
    auto y = std::abs (b);

    while (y != 0)
    {
        const auto next = x % y;
        x = y;
        y = next;
    }

    return x;
}

int lcmInt (const int a, const int b)
{
    if (a == 0 || b == 0)
        return 0;

    return std::abs (a / gcdInt (a, b) * b);
}

int humanizeVelocityValue (int velocity, const int humanizePercent, std::uint32_t& randomState)
{
    velocity = juce::jlimit (1, 127, velocity);

    if (humanizePercent <= 0)
        return velocity;

    const auto range = static_cast<double> (velocity)
                       * (static_cast<double> (clampPercent (humanizePercent)) / 100.0);
    const auto humanized = static_cast<double> (velocity)
                           + (nextRandomUnitDouble (randomState) * 2.0 - 1.0) * range;

    return juce::jlimit (1, 127, static_cast<int> (std::lround (humanized)));
}

double swingDelayQuartersForPpq (const double ppq,
                                 const double pulseQuarters,
                                 const int swingPercent,
                                 const int subdivisionIndex)
{
    if (swingPercent <= 0)
        return 0.0;

    const auto subdivision =
        pulseQuarters * swingSubdivisionValues[static_cast<size_t> (
                           juce::jlimit (0, PluginProcessor::swingSubdivisionCount - 1, subdivisionIndex))];

    if (subdivision <= 0.0)
        return 0.0;

    constexpr auto epsilon = 1.0e-9;
    const auto subdivisionNumber = static_cast<int> (std::floor ((ppq + epsilon) / subdivision));

    return subdivisionNumber % 2 != 0
               ? subdivision * 0.5 * (static_cast<double> (clampPercent (swingPercent)) / 100.0)
               : 0.0;
}

int stepTimingMultiplierIndexFromState (const int storedIndex, const int stateVersion)
{
    if (stateVersion >= 4)
        return juce::jlimit (0, PluginProcessor::stepTimingMultiplierCount - 1, storedIndex);

    constexpr int legacyToNew[] = { 0, 1, 3, 7, 15 };

    return legacyToNew[juce::jlimit (0, 4, storedIndex)];
}

int rowTimingOffsetIndexFromState (const int storedIndex, const int stateVersion)
{
    if (stateVersion >= 23)
        return juce::jlimit (0, PluginProcessor::rowTimingOffsetCount - 1, storedIndex);

    constexpr double legacyRowTimingOffsetValues[] = { -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75 };
    constexpr int legacyCount = 7;
    const auto legacyIndex = juce::jlimit (0, legacyCount - 1, storedIndex);
    const auto quarters = legacyRowTimingOffsetValues[static_cast<size_t> (legacyIndex)];
    const auto newIndex = static_cast<int> (std::round (
        (quarters - PluginProcessor::rowTimingOffsetMinQuarters)
        / PluginProcessor::rowTimingOffsetQuarterStep));

    return juce::jlimit (0, PluginProcessor::rowTimingOffsetCount - 1, newIndex);
}
constexpr double legacyStepDurationFractionValues[] = { 0.25, 0.5, 0.75, 1.0 };

double clampStepDurationFraction (const double fraction)
{
    return juce::jlimit (0.0, 1.0, fraction);
}

double durationFractionFromStateProperty (const juce::var& value, const int stateVersion)
{
    // Version 1 stored duration as a discrete index; version 2+ stores 0–1 fractions.
    if (stateVersion < 2)
    {
        const auto legacyIndex = juce::jlimit (0, 3, static_cast<int> (value));

        return legacyStepDurationFractionValues[static_cast<size_t> (legacyIndex)];
    }

    return clampStepDurationFraction (static_cast<double> (value));
}

double positiveMod (const double value, const double modulus)
{
    if (modulus <= 0.0)
        return value;

    auto remainder = std::fmod (value, modulus);

    if (remainder < 0.0)
        remainder += modulus;

    return remainder;
}

double snapLoopBraceQuarters (const double quarters)
{
    return std::round (quarters / PluginProcessor::loopBraceSnapQuarters)
           * PluginProcessor::loopBraceSnapQuarters;
}

double clampLoopBraceStart (const double startQuarters, const double endQuarters)
{
    return juce::jmax (0.0,
                       juce::jmin (snapLoopBraceQuarters (startQuarters),
                                   endQuarters - PluginProcessor::loopBraceSnapQuarters));
}

double clampLoopBraceEnd (const double endQuarters, const double startQuarters)
{
    return juce::jmax (startQuarters + PluginProcessor::loopBraceSnapQuarters,
                       snapLoopBraceQuarters (endQuarters));
}

double clampStandaloneTempoBpm (const double bpm)
{
    return juce::jlimit (20.0, 300.0, bpm);
}
} // namespace

void PluginProcessor::resetPendingNoteOffs()
{
    for (auto& pending : pendingNoteOffs)
    {
        pending.note = -1;
        pending.activeStep = -1;
        pending.samplesRemaining = 0;
    }

    for (auto& audition : phraseRowNoteAuditions)
        audition.pending.store (0, std::memory_order_relaxed);
}

void PluginProcessor::resetPendingNoteOns()
{
    pendingNoteOnCount = 0;
}

void PluginProcessor::resetPendingCombinedNoteOffs()
{
    pendingCombinedNoteOffCount = 0;
}

void PluginProcessor::resetActiveGeneratedNotes()
{
    for (auto& channelNotes : activeGeneratedNoteCounts)
        channelNotes.fill (0);

    resetPendingCombinedNoteOffs();
}

bool PluginProcessor::hasActiveGeneratedNotes() const
{
    for (const auto& channelNotes : activeGeneratedNoteCounts)
    {
        for (const auto activeCount : channelNotes)
        {
            if (activeCount > 0)
                return true;
        }
    }

    return false;
}

void PluginProcessor::resetPlaybackMidiState()
{
    wasPlaying = false;
    resolvePendingAudioLoopBraceEnableForStoppedPlayback();
    resetLastEmittedTriggers();
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetPendingCombinedNoteOffs();
    resetActiveGeneratedNotes();
    clearLoopScheduleAnchor();
    clearPatternScheduleAnchor();
    loopScheduleReanchorRequested = false;
    currentPlaybackPpq.store (-1.0, std::memory_order_relaxed);
}

void PluginProcessor::appendGeneratedNotePanicMessages (juce::MidiBuffer& midiMessages)
{
    if (! hasActiveGeneratedNotes() && ! wasPlaying)
        return;

    flushActiveGeneratedNotes (0, midiMessages);

    for (int channel = 1; channel <= 16; ++channel)
        midiMessages.addEvent (juce::MidiMessage::allNotesOff (channel), 0);

    resetPlaybackMidiState();
}

void PluginProcessor::resetLastEmittedTriggers()
{
    for (auto& lastTrigger : lastEmittedTriggerPpq)
        lastTrigger = -1.0;

    resetStepCycleCounters();
}

void PluginProcessor::resetStepCycleCounters()
{
    for (auto& rowCounters : stepCycleCounters)
        rowCounters.fill (0);
}

void PluginProcessor::emitGeneratedNoteOn (const int midiChannel,
                                           const int note,
                                           const int velocity,
                                           const int sampleOffset,
                                           juce::MidiBuffer& midiMessages)
{
    if (midiChannel < minPhraseRowMidiChannel || midiChannel > maxPhraseRowMidiChannel
        || note < 0 || note >= 128 || velocity <= 0)
        return;

    ++activeGeneratedNoteCounts[static_cast<size_t> (midiChannel - 1)][static_cast<size_t> (note)];
    midiMessages.addEvent (
        juce::MidiMessage::noteOn (midiChannel,
                                   note,
                                   static_cast<juce::uint8> (juce::jlimit (1, 127, velocity))),
        juce::jmax (0, sampleOffset));
}

void PluginProcessor::emitGeneratedNoteOff (const int midiChannel,
                                            const int note,
                                            const int sampleOffset,
                                            juce::MidiBuffer& midiMessages)
{
    if (midiChannel < minPhraseRowMidiChannel || midiChannel > maxPhraseRowMidiChannel
        || note < 0 || note >= 128)
        return;

    auto& activeCount =
        activeGeneratedNoteCounts[static_cast<size_t> (midiChannel - 1)][static_cast<size_t> (note)];

    if (activeCount > 0)
        --activeCount;

    midiMessages.addEvent (juce::MidiMessage::noteOff (midiChannel, note),
                           juce::jmax (0, sampleOffset));
}

void PluginProcessor::flushPendingGeneratedNoteOffs (const int sampleOffset,
                                                     juce::MidiBuffer& midiMessages)
{
    const auto offset = juce::jmax (0, sampleOffset);

    for (auto& pending : pendingNoteOffs)
    {
        if (pending.note < 0)
            continue;

        emitGeneratedNoteOff (pending.channel, pending.note, offset, midiMessages);
        pending.note = -1;
        pending.samplesRemaining = 0;
    }

    for (size_t i = 0; i < pendingCombinedNoteOffCount; ++i)
    {
        const auto& pending = pendingCombinedNoteOffs[i];

        if (pending.note >= 0)
            emitGeneratedNoteOff (pending.channel, pending.note, offset, midiMessages);
    }

    resetPendingCombinedNoteOffs();
}

void PluginProcessor::flushPendingCombinedNoteOffsForChannel (const int midiChannel,
                                                              const int sampleOffset,
                                                              juce::MidiBuffer& midiMessages)
{
    const auto offset = juce::jmax (0, sampleOffset);

    for (size_t i = 0; i < pendingCombinedNoteOffCount; ++i)
    {
        auto& pending = pendingCombinedNoteOffs[i];

        if (pending.note < 0 || pending.channel != midiChannel)
            continue;

        emitGeneratedNoteOff (pending.channel, pending.note, offset, midiMessages);
        pending.note = -1;
    }
}

void PluginProcessor::flushPendingCombinedNoteOffs (const int bufferSamples,
                                                    juce::MidiBuffer& midiMessages)
{
    if (pendingCombinedNoteOffCount == 0)
        return;

    size_t writeIndex = 0;

    for (size_t i = 0; i < pendingCombinedNoteOffCount; ++i)
    {
        auto pending = pendingCombinedNoteOffs[i];

        if (pending.note < 0)
            continue;

        if (pending.samplesRemaining < bufferSamples)
        {
            emitGeneratedNoteOff (pending.channel,
                                  pending.note,
                                  pending.samplesRemaining,
                                  midiMessages);
        }
        else
        {
            pending.samplesRemaining -= bufferSamples;
            pendingCombinedNoteOffs[writeIndex++] = pending;
        }
    }

    pendingCombinedNoteOffCount = writeIndex;
}

void PluginProcessor::flushActiveGeneratedNotes (const int sampleOffset,
                                                 juce::MidiBuffer& midiMessages)
{
    const auto offset = juce::jmax (0, sampleOffset);

    for (int channel = minPhraseRowMidiChannel; channel <= maxPhraseRowMidiChannel; ++channel)
    {
        auto& channelNotes = activeGeneratedNoteCounts[static_cast<size_t> (channel - 1)];

        for (int note = 0; note < 128; ++note)
        {
            auto& activeCount = channelNotes[static_cast<size_t> (note)];

            while (activeCount > 0)
            {
                midiMessages.addEvent (juce::MidiMessage::noteOff (channel, note), offset);
                --activeCount;
            }
        }
    }
}

void PluginProcessor::flushActiveWeaveNotes (const int sampleOffset,
                                             juce::MidiBuffer& midiMessages)
{
    if (! hasActiveGeneratedNotes())
        return;

    flushActiveGeneratedNotes (sampleOffset, midiMessages);
    resetPendingCombinedNoteOffs();
}

void PluginProcessor::resetStepCycleCountersForRow (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    stepCycleCounters[static_cast<size_t> (row)].fill (0);
}

PluginProcessor::BusesProperties PluginProcessor::createBusesProperties()
{
  #if JucePlugin_IsMidiEffect
    return BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo(), true);
  #else
    return BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true);
  #endif
}

PluginProcessor::PluginProcessor()
     : AudioProcessor (createBusesProperties())
{
    patternSlotParameter =
        new juce::AudioParameterInt (juce::ParameterID { "patternSlot", 1 },
                                     "Pattern",
                                     1,
                                     patternSlotCount,
                                     1);
    addParameter (patternSlotParameter);

    sequencerCommandQueue =
        std::make_unique<std::array<SequencerCommand, sequencerCommandQueueCapacity>>();

    for (auto& pattern : modelPatterns)
        initialisePatternDefaults (pattern);

    audioPatterns = modelPatterns;

    for (auto& flush : phraseRowFlushNoteOff)
        flush.store (0);

    resetLastEmittedTriggers();
}

PluginProcessor::~PluginProcessor()
{
}

int PluginProcessor::defaultStepNoteForScaleRoot (const int scaleRoot)
{
    return (defaultStepOctave + 2) * 12 + clampScaleRoot (scaleRoot);
}

int PluginProcessor::defaultNoteForRow (const int row) const
{
    juce::ignoreUnused (row);
    return defaultStepNoteForScaleRoot (modelPattern (getViewPatternSlot()).scaleRoot);
}

bool PluginProcessor::isValidStep (const int row, const int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return false;

    return step < getPhraseRowStepCount (row);
}

bool PluginProcessor::isValidAudioStep (const SequencerState& state, const int row, const int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return false;

    return step < state.rows[static_cast<size_t> (row)].stepCount;
}

int PluginProcessor::clampPatternSlot (const int patternSlot) const
{
    return juce::jlimit (0, patternSlotCount - 1, patternSlot);
}

int PluginProcessor::clampLoopSlot (const int loopSlot) const
{
    return juce::jlimit (0, loopSlotCount - 1, loopSlot);
}

PluginProcessor::PatternState& PluginProcessor::modelPattern (const int patternSlot)
{
    return modelPatterns[static_cast<size_t> (clampPatternSlot (patternSlot))];
}

const PluginProcessor::PatternState& PluginProcessor::modelPattern (const int patternSlot) const
{
    return modelPatterns[static_cast<size_t> (clampPatternSlot (patternSlot))];
}

PluginProcessor::SequencerState& PluginProcessor::modelSequencer()
{
    return modelPattern (getViewPatternSlot()).sequencer;
}

const PluginProcessor::SequencerState& PluginProcessor::modelSequencer() const
{
    return modelPattern (getViewPatternSlot()).sequencer;
}

const PluginProcessor::SequencerState& PluginProcessor::audioSequencer() const
{
    return audioPatterns[static_cast<size_t> (clampPatternSlot (audioActivePatternSlot))].sequencer;
}

PluginProcessor::LoopBraceState& PluginProcessor::modelLoopBrace()
{
    return modelPattern (getViewPatternSlot()).loopBrace;
}

const PluginProcessor::LoopBraceState& PluginProcessor::modelLoopBrace() const
{
    return modelPattern (getViewPatternSlot()).loopBrace;
}

const PluginProcessor::LoopBraceState& PluginProcessor::audioLoopBrace() const
{
    return audioPatterns[static_cast<size_t> (clampPatternSlot (audioActivePatternSlot))].loopBrace;
}

PluginProcessor::PhraseRowSteps& PluginProcessor::modelRow (const int row)
{
    return modelSequencer().rows[static_cast<size_t> (row)];
}

const PluginProcessor::PhraseRowSteps& PluginProcessor::modelRow (const int row) const
{
    return modelSequencer().rows[static_cast<size_t> (row)];
}

void PluginProcessor::initialiseRowDefaults (PhraseRowSteps& steps,
                                             const int row,
                                             const int stepCount,
                                             const int scaleRoot)
{
    juce::ignoreUnused (row);
    steps.stepCount = juce::jlimit (0, maxPhraseStepsPerRow, stepCount);

    const auto defaultNote = defaultStepNoteForScaleRoot (scaleRoot);

    for (int step = 0; step < maxPhraseStepsPerRow; ++step)
    {
        steps.notes[static_cast<size_t> (step)] = defaultNote;
        steps.timingMultiplier[static_cast<size_t> (step)] = defaultStepTimingMultiplierIndex;
        steps.durationFraction[static_cast<size_t> (step)] = defaultStepDurationFraction;
        steps.velocity[static_cast<size_t> (step)] = defaultStepVelocity;
        steps.stepMuted[static_cast<size_t> (step)] = 0;
        steps.stepSkipped[static_cast<size_t> (step)] = 0;
        steps.probability[static_cast<size_t> (step)] = PluginProcessor::defaultStepProbability;
        steps.cycle[static_cast<size_t> (step)] = PluginProcessor::defaultStepCycle;
        steps.cycleOffset[static_cast<size_t> (step)] = PluginProcessor::PluginProcessor::defaultStepCycleMask;
    }

    rebuildRowTimingLayout (steps);
}

void PluginProcessor::initialisePatternDefaults (PatternState& pattern)
{
    pattern.scaleRoot = defaultScaleRoot;
    pattern.scaleModeIndex = defaultScaleModeIndex;
    pattern.noteBandpassLowMidi = defaultNoteBandpassLowMidi;
    pattern.noteBandpassHighMidi = defaultNoteBandpassHighMidi;
    pattern.velocityTiltPivotMidi = defaultVelocityTiltPivotMidi;
    pattern.velocityTiltAmount = defaultVelocityTiltAmount;
    pattern.globalTransposeSemitones = defaultGlobalTransposeSemitones;
    pattern.octavizerDown8vaEnabled = 0;
    pattern.octavizerUp8vaEnabled = 0;
    pattern.octavizerDown8vaRelativeVelocity = defaultOctavizerRelativeVelocity;
    pattern.octavizerUp8vaRelativeVelocity = defaultOctavizerRelativeVelocity;
    pattern.shimmerEnabled = 0;
    pattern.shimmerDelayMultiplierIndex = defaultStepTimingMultiplierIndex;
    pattern.shimmerFeedbackPercent = defaultShimmerFeedbackPercent;
    pattern.shimmerMixPercent = defaultShimmerMixPercent;
    pattern.seedingRhythmStep = defaultSeedingRhythmStep;

    for (auto& seedingRow : pattern.seedingRows)
        seedingRow = {};

    pattern.seedingRowTargets = { 1, 0, 0, 0 };
    pattern.sequencer.combinationModeMask = 0;

    for (int row = 0; row < phraseRowCount; ++row)
    {
        initialiseRowDefaults (pattern.sequencer.rows[static_cast<size_t> (row)],
                               row,
                               defaultPhraseStepsPerRow,
                               pattern.scaleRoot);

        pattern.sequencer.muted[static_cast<size_t> (row)] = row == 0 ? 0 : 1;
        pattern.sequencer.timingOffset[static_cast<size_t> (row)] = defaultRowTimingOffsetIndex;
        pattern.sequencer.midiChannel[static_cast<size_t> (row)] =
            defaultPhraseRowMidiChannelForRow (row);
    }

    pattern.loopBrace.enabled = 0;
    pattern.loopBrace.startQuarters = defaultLoopBraceStartQuarters;
    pattern.loopBrace.endQuarters = defaultLoopBraceEndQuarters;
}

void PluginProcessor::rebuildRowTimingLayout (PhraseRowSteps& steps)
{
    const auto pulse = pulseQuartersForIndex (pulseIndex.load (std::memory_order_relaxed));
    auto cycleLengthQuarters = 0.0;

    for (int step = 0; step < maxPhraseStepsPerRow; ++step)
    {
        const auto index = static_cast<size_t> (step);
        const auto inCount = step < steps.stepCount;
        const auto length =
            inCount ? stepTimingMultiplierForIndex (steps.timingMultiplier[index]) * pulse : 0.0;

        steps.stepLengthQuarters[index] = length;
        steps.stepStartQuarters[index] = cycleLengthQuarters;

        if (inCount && steps.stepSkipped[index] == 0)
            cycleLengthQuarters += length;
    }

    steps.cycleLengthQuarters = cycleLengthQuarters;
}

void PluginProcessor::publishCommandToAudio (const SequencerCommand& command)
{
    auto queuedCommand = command;

    if (queuedCommand.patternSlot < 0)
        queuedCommand.patternSlot = getViewPatternSlot();

    const auto write = sequencerCommandWriteIndex.load (std::memory_order_relaxed);
    const auto nextWrite = (write + 1) % sequencerCommandQueueCapacity;

    if (nextWrite == sequencerCommandReadIndex.load (std::memory_order_acquire))
    {
        jassertfalse;
        return;
    }

    (*sequencerCommandQueue)[write] = queuedCommand;
    sequencerCommandWriteIndex.store (nextWrite, std::memory_order_release);
}

void PluginProcessor::publishRowToAudio (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    SequencerCommand command;
    command.type = SequencerCommand::Type::ReplaceRow;
    command.patternSlot = getViewPatternSlot();
    command.row = row;
    command.rowState = modelRow (row);
    publishCommandToAudio (command);
}

void PluginProcessor::publishPatternToAudio (const int patternSlot)
{
    const auto slot = clampPatternSlot (patternSlot);

    SequencerCommand command;
    command.type = SequencerCommand::Type::ReplacePattern;
    command.patternSlot = slot;
    command.patternState = modelPattern (slot);
    publishCommandToAudio (command);
}

void PluginProcessor::publishLoopBraceCommandToAudio (const SequencerCommand::Type type,
                                                      const int patternSlot)
{
    const auto slot = clampPatternSlot (patternSlot);

    SequencerCommand command;
    command.type = type;
    command.patternSlot = slot;
    command.intValue = modelPattern (slot).loopBrace.enabled;
    command.doubleValue = type == SequencerCommand::Type::SetLoopBraceStart
                              ? modelPattern (slot).loopBrace.startQuarters
                              : modelPattern (slot).loopBrace.endQuarters;
    publishCommandToAudio (command);
}

void PluginProcessor::applySequencerCommand (const SequencerCommand& command)
{
    const auto patternSlot = clampPatternSlot (command.patternSlot);
    auto& pattern = audioPatterns[static_cast<size_t> (patternSlot)];
    auto& state = pattern.sequencer;

    if (command.row < 0 || command.row >= phraseRowCount)
    {
        if (command.type != SequencerCommand::Type::ReplacePattern
            && command.type != SequencerCommand::Type::SetLoopBraceEnabled
            && command.type != SequencerCommand::Type::SetLoopBraceStart
            && command.type != SequencerCommand::Type::SetLoopBraceEnd
            && command.type != SequencerCommand::Type::SetCombinationModeMask
            && command.type != SequencerCommand::Type::SetPatternScale)
            return;
    }

    auto& row = state.rows[static_cast<size_t> (juce::jlimit (0, phraseRowCount - 1, command.row))];
    const auto step = command.step;
    const auto index = static_cast<size_t> (juce::jlimit (0, maxPhraseStepsPerRow - 1, step));

    switch (command.type)
    {
        case SequencerCommand::Type::SetNote:
            if (isValidAudioStep (state, command.row, step))
            {
                const auto oldNote = row.notes[index];
                row.notes[index] = command.intValue;

                if (patternSlot == audioActivePatternSlot
                    && oldNote != command.intValue)
                {
                    auto& rowPending = pendingNoteOffs[static_cast<size_t> (command.row)];

                    if (rowPending.note >= 0 && rowPending.activeStep == step)
                    {
                        auto& audition =
                            phraseRowNoteAuditions[static_cast<size_t> (command.row)];
                        audition.step = step;
                        audition.note = command.intValue;
                        audition.pending.store (1, std::memory_order_release);
                    }
                }
            }
            break;

        case SequencerCommand::Type::SetRowMuted:
            state.muted[static_cast<size_t> (command.row)] = command.intValue != 0 ? 1 : 0;
            break;

        case SequencerCommand::Type::SetRowTimingOffset:
            state.timingOffset[static_cast<size_t> (command.row)] =
                juce::jlimit (0, rowTimingOffsetCount - 1, command.intValue);
            break;

        case SequencerCommand::Type::SetRowMidiChannel:
            state.midiChannel[static_cast<size_t> (command.row)] =
                juce::jlimit (minPhraseRowMidiChannel, maxPhraseRowMidiChannel, command.intValue);
            break;

        case SequencerCommand::Type::SetStepTimingMultiplier:
            if (isValidAudioStep (state, command.row, step))
            {
                row.timingMultiplier[index] =
                    juce::jlimit (0, stepTimingMultiplierCount - 1, command.intValue);
                rebuildRowTimingLayout (row);
            }
            break;

        case SequencerCommand::Type::SetStepDurationFraction:
            if (isValidAudioStep (state, command.row, step))
                row.durationFraction[index] = clampStepDurationFraction (command.doubleValue);
            break;

        case SequencerCommand::Type::SetStepVelocity:
            if (isValidAudioStep (state, command.row, step))
                row.velocity[index] = juce::jlimit (0, 127, command.intValue);
            break;

        case SequencerCommand::Type::SetStepMuted:
            if (isValidAudioStep (state, command.row, step))
            {
                row.stepMuted[index] = command.intValue != 0 ? 1 : 0;

                if (command.intValue != 0)
                    row.stepSkipped[index] = 0;
            }
            break;

        case SequencerCommand::Type::SetStepSkipped:
            if (isValidAudioStep (state, command.row, step))
            {
                if (command.intValue != 0)
                    row.stepMuted[index] = 0;

                row.stepSkipped[index] = command.intValue != 0 ? 1 : 0;
                rebuildRowTimingLayout (row);
            }
            break;

        case SequencerCommand::Type::SetStepProbability:
            if (isValidAudioStep (state, command.row, step))
                row.probability[index] = command.intValue;
            break;

        case SequencerCommand::Type::SetStepCycle:
            if (isValidAudioStep (state, command.row, step))
            {
                row.cycle[index] = command.intValue;
                row.cycleOffset[index] =
                    clampStepCycleMask (row.cycleOffset[index], row.cycle[index]);
            }
            break;

        case SequencerCommand::Type::SetStepCycleOffset:
            if (isValidAudioStep (state, command.row, step))
                row.cycleOffset[index] =
                    clampStepCycleMask (command.intValue, row.cycle[index]);
            break;

        case SequencerCommand::Type::RemoveStep:
            if (isValidAudioStep (state, command.row, step))
            {
                for (int i = step; i < row.stepCount - 1; ++i)
                {
                    const auto current = static_cast<size_t> (i);
                    const auto next = static_cast<size_t> (i + 1);
                    row.notes[current] = row.notes[next];
                    row.timingMultiplier[current] = row.timingMultiplier[next];
                    row.durationFraction[current] = row.durationFraction[next];
                    row.velocity[current] = row.velocity[next];
                    row.stepMuted[current] = row.stepMuted[next];
                    row.stepSkipped[current] = row.stepSkipped[next];
                    row.probability[current] = row.probability[next];
                    row.cycle[current] = row.cycle[next];
                    row.cycleOffset[current] = row.cycleOffset[next];
                }

                --row.stepCount;
                rebuildRowTimingLayout (row);
                resetStepCycleCountersForRow (command.row);
            }
            break;

        case SequencerCommand::Type::InsertStep:
            if (step >= 0 && step <= row.stepCount && row.stepCount < maxPhraseStepsPerRow)
            {
                for (int i = row.stepCount; i > step; --i)
                {
                    const auto current = static_cast<size_t> (i);
                    const auto previous = static_cast<size_t> (i - 1);
                    row.notes[current] = row.notes[previous];
                    row.timingMultiplier[current] = row.timingMultiplier[previous];
                    row.durationFraction[current] = row.durationFraction[previous];
                    row.velocity[current] = row.velocity[previous];
                    row.stepMuted[current] = row.stepMuted[previous];
                    row.stepSkipped[current] = row.stepSkipped[previous];
                    row.probability[current] = row.probability[previous];
                    row.cycle[current] = row.cycle[previous];
                    row.cycleOffset[current] = row.cycleOffset[previous];
                }

                row.notes[index] = defaultStepNoteForScaleRoot (pattern.scaleRoot);
                row.timingMultiplier[index] = defaultStepTimingMultiplierIndex;
                row.durationFraction[index] = defaultStepDurationFraction;
                row.velocity[index] = defaultStepVelocity;
                row.stepMuted[index] = 0;
                row.stepSkipped[index] = 0;
                row.probability[index] = PluginProcessor::defaultStepProbability;
                row.cycle[index] = PluginProcessor::defaultStepCycle;
                row.cycleOffset[index] = PluginProcessor::defaultStepCycleMask;
                ++row.stepCount;
                rebuildRowTimingLayout (row);
                resetStepCycleCountersForRow (command.row);
            }
            break;

        case SequencerCommand::Type::DuplicateStep:
            if (step > 0 && step <= row.stepCount && row.stepCount < maxPhraseStepsPerRow)
            {
                const auto sourceIndex = static_cast<size_t> (step - 1);

                for (int i = row.stepCount; i > step; --i)
                {
                    const auto current = static_cast<size_t> (i);
                    const auto previous = static_cast<size_t> (i - 1);
                    row.notes[current] = row.notes[previous];
                    row.timingMultiplier[current] = row.timingMultiplier[previous];
                    row.durationFraction[current] = row.durationFraction[previous];
                    row.velocity[current] = row.velocity[previous];
                    row.stepMuted[current] = row.stepMuted[previous];
                    row.stepSkipped[current] = row.stepSkipped[previous];
                    row.probability[current] = row.probability[previous];
                    row.cycle[current] = row.cycle[previous];
                    row.cycleOffset[current] = row.cycleOffset[previous];
                }

                row.notes[index] = row.notes[sourceIndex];
                row.timingMultiplier[index] = row.timingMultiplier[sourceIndex];
                row.durationFraction[index] = row.durationFraction[sourceIndex];
                row.velocity[index] = row.velocity[sourceIndex];
                row.stepMuted[index] = row.stepMuted[sourceIndex];
                row.stepSkipped[index] = row.stepSkipped[sourceIndex];
                row.probability[index] = row.probability[sourceIndex];
                row.cycle[index] = row.cycle[sourceIndex];
                row.cycleOffset[index] = row.cycleOffset[sourceIndex];
                ++row.stepCount;
                rebuildRowTimingLayout (row);
                resetStepCycleCountersForRow (command.row);
            }
            break;

        case SequencerCommand::Type::MoveStep:
            if (isValidAudioStep (state, command.row, command.step)
                && isValidAudioStep (state, command.row, command.toStep)
                && command.step != command.toStep)
            {
                const auto note = row.notes[static_cast<size_t> (command.step)];
                const auto timingMultiplier = row.timingMultiplier[static_cast<size_t> (command.step)];
                const auto durationFraction = row.durationFraction[static_cast<size_t> (command.step)];
                const auto velocity = row.velocity[static_cast<size_t> (command.step)];
                const auto stepMuted = row.stepMuted[static_cast<size_t> (command.step)];
                const auto stepSkipped = row.stepSkipped[static_cast<size_t> (command.step)];
                const auto probability = row.probability[static_cast<size_t> (command.step)];
                const auto cycle = row.cycle[static_cast<size_t> (command.step)];
                const auto cycleOffset = row.cycleOffset[static_cast<size_t> (command.step)];

                if (command.step < command.toStep)
                {
                    for (int i = command.step; i < command.toStep; ++i)
                    {
                        const auto current = static_cast<size_t> (i);
                        const auto next = static_cast<size_t> (i + 1);
                        row.notes[current] = row.notes[next];
                        row.timingMultiplier[current] = row.timingMultiplier[next];
                        row.durationFraction[current] = row.durationFraction[next];
                        row.velocity[current] = row.velocity[next];
                        row.stepMuted[current] = row.stepMuted[next];
                        row.stepSkipped[current] = row.stepSkipped[next];
                        row.probability[current] = row.probability[next];
                        row.cycle[current] = row.cycle[next];
                        row.cycleOffset[current] = row.cycleOffset[next];
                    }
                }
                else
                {
                    for (int i = command.step; i > command.toStep; --i)
                    {
                        const auto current = static_cast<size_t> (i);
                        const auto previous = static_cast<size_t> (i - 1);
                        row.notes[current] = row.notes[previous];
                        row.timingMultiplier[current] = row.timingMultiplier[previous];
                        row.durationFraction[current] = row.durationFraction[previous];
                        row.velocity[current] = row.velocity[previous];
                        row.stepMuted[current] = row.stepMuted[previous];
                        row.stepSkipped[current] = row.stepSkipped[previous];
                        row.probability[current] = row.probability[previous];
                        row.cycle[current] = row.cycle[previous];
                        row.cycleOffset[current] = row.cycleOffset[previous];
                    }
                }

                const auto destination = static_cast<size_t> (command.toStep);
                row.notes[destination] = note;
                row.timingMultiplier[destination] = timingMultiplier;
                row.durationFraction[destination] = durationFraction;
                row.velocity[destination] = velocity;
                row.stepMuted[destination] = stepMuted;
                row.stepSkipped[destination] = stepSkipped;
                row.probability[destination] = probability;
                row.cycle[destination] = cycle;
                row.cycleOffset[destination] = cycleOffset;
                rebuildRowTimingLayout (row);
                resetStepCycleCountersForRow (command.row);
            }
            break;

        case SequencerCommand::Type::ReplaceRow:
            row = command.rowState;
            row.stepCount = juce::jlimit (0, maxPhraseStepsPerRow, row.stepCount);
            rebuildRowTimingLayout (row);
            resetStepCycleCountersForRow (command.row);
            break;

        case SequencerCommand::Type::SetLoopBraceEnabled:
            if (command.intValue != 0)
            {
                if (patternSlot == audioActivePatternSlot
                    && wasPlaying
                    && pattern.loopBrace.enabled == 0)
                {
                    requestAudioLoopBraceEnable (patternSlot);
                    clearLoopScheduleAnchor();
                }
                else
                {
                    pattern.loopBrace.enabled = 1;

                    if (patternSlot == audioActivePatternSlot && ! wasPlaying)
                        clearLoopScheduleAnchor();
                    else if (patternSlot == audioActivePatternSlot)
                        loopScheduleReanchorRequested = true;
                }
            }
            else
            {
                pattern.loopBrace.enabled = 0;

                if (pendingAudioLoopBraceEnablePatternSlot == patternSlot)
                    clearPendingAudioLoopBraceEnable();

                if (patternSlot == audioActivePatternSlot)
                    clearLoopScheduleAnchor();
            }

            break;

        case SequencerCommand::Type::SetLoopBraceStart:
            pattern.loopBrace.startQuarters =
                clampLoopBraceStart (command.doubleValue, pattern.loopBrace.endQuarters);
            break;

        case SequencerCommand::Type::SetLoopBraceEnd:
            pattern.loopBrace.endQuarters =
                clampLoopBraceEnd (command.doubleValue, pattern.loopBrace.startQuarters);
            break;

        case SequencerCommand::Type::SetCombinationModeMask:
            state.combinationModeMask = clampCombinationModeMask (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternScale:
            pattern.scaleRoot = clampScaleRoot (command.step);
            pattern.scaleModeIndex = clampScaleModeIndex (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternNoteBandpass:
        {
            const auto bounds = clampNoteBandpassBounds (command.step, command.intValue);
            pattern.noteBandpassLowMidi = bounds.first;
            pattern.noteBandpassHighMidi = bounds.second;
            break;
        }

        case SequencerCommand::Type::SetPatternVelocityTiltPivotMidi:
            pattern.velocityTiltPivotMidi = clampVelocityTiltPivotMidi (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternVelocityTiltAmount:
            pattern.velocityTiltAmount = clampVelocityTiltAmount (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternGlobalTransposeSemitones:
            pattern.globalTransposeSemitones = clampGlobalTransposeSemitones (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternOctavizerDown8vaEnabled:
            pattern.octavizerDown8vaEnabled = command.intValue != 0 ? 1 : 0;
            break;

        case SequencerCommand::Type::SetPatternOctavizerUp8vaEnabled:
            pattern.octavizerUp8vaEnabled = command.intValue != 0 ? 1 : 0;
            break;

        case SequencerCommand::Type::SetPatternOctavizerDown8vaRelativeVelocity:
            pattern.octavizerDown8vaRelativeVelocity =
                clampOctavizerRelativeVelocity (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternOctavizerUp8vaRelativeVelocity:
            pattern.octavizerUp8vaRelativeVelocity =
                clampOctavizerRelativeVelocity (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternShimmerEnabled:
            pattern.shimmerEnabled = command.intValue != 0 ? 1 : 0;
            break;

        case SequencerCommand::Type::SetPatternShimmerDelayMultiplierIndex:
            pattern.shimmerDelayMultiplierIndex =
                clampShimmerDelayMultiplierIndex (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternShimmerFeedbackPercent:
            pattern.shimmerFeedbackPercent = clampShimmerFeedbackPercent (command.intValue);
            break;

        case SequencerCommand::Type::SetPatternShimmerMixPercent:
            pattern.shimmerMixPercent = clampShimmerMixPercent (command.intValue);
            break;

        case SequencerCommand::Type::ReplacePattern:
            pattern = command.patternState;
            for (auto& patternRow : pattern.sequencer.rows)
                rebuildRowTimingLayout (patternRow);
            resetStepCycleCounters();
            break;
    }

    if (patternSlot != audioActivePatternSlot)
        return;

    const auto flushAllRows = [&] {
        for (auto& flush : phraseRowFlushNoteOff)
            flush.store (1);
    };

    const auto flushRow = [&] {
        if (command.row >= 0 && command.row < phraseRowCount)
            phraseRowFlushNoteOff[static_cast<size_t> (command.row)].store (1);
    };

    if (command.type == SequencerCommand::Type::ReplacePattern
        || command.type == SequencerCommand::Type::SetCombinationModeMask
        || command.type == SequencerCommand::Type::SetPatternNoteBandpass)
    {
        flushAllRows();
    }
    else if (command.type == SequencerCommand::Type::ReplaceRow
             || command.type == SequencerCommand::Type::SetRowMidiChannel)
    {
        flushRow();
    }
    else if (command.type == SequencerCommand::Type::SetRowMuted && command.intValue != 0)
    {
        flushRow();
    }
}

void PluginProcessor::drainSequencerCommands()
{
    auto read = sequencerCommandReadIndex.load (std::memory_order_relaxed);

    while (read != sequencerCommandWriteIndex.load (std::memory_order_acquire))
    {
        applySequencerCommand ((*sequencerCommandQueue)[read]);
        read = (read + 1) % sequencerCommandQueueCapacity;
        sequencerCommandReadIndex.store (read, std::memory_order_release);
    }
}

void PluginProcessor::resetPhraseStepToDefaults (const int row, const int step)
{
    if (! isValidStep (row, step))
        return;

    auto& steps = modelRow (row);
    steps.notes[static_cast<size_t> (step)] = defaultNoteForRow (row);
    steps.timingMultiplier[static_cast<size_t> (step)] = defaultStepTimingMultiplierIndex;
    steps.durationFraction[static_cast<size_t> (step)] = defaultStepDurationFraction;
    steps.velocity[static_cast<size_t> (step)] = defaultStepVelocity;
    steps.stepMuted[static_cast<size_t> (step)] = 0;
    steps.stepSkipped[static_cast<size_t> (step)] = 0;
    steps.probability[static_cast<size_t> (step)] = PluginProcessor::defaultStepProbability;
    steps.cycle[static_cast<size_t> (step)] = PluginProcessor::defaultStepCycle;
    steps.cycleOffset[static_cast<size_t> (step)] = PluginProcessor::defaultStepCycleMask;
    rebuildRowTimingLayout (steps);
    publishRowToAudio (row);
}

double PluginProcessor::rowTimingOffsetForIndex (const int offsetIndex)
{
    const auto index = juce::jlimit (0, rowTimingOffsetCount - 1, offsetIndex);
    return rowTimingOffsetMinQuarters + static_cast<double> (index) * rowTimingOffsetQuarterStep;
}

double PluginProcessor::stepTimingMultiplierForIndex (const int multiplierIndex)
{
    const auto index = juce::jlimit (0, stepTimingMultiplierCount - 1, multiplierIndex);
    return stepTimingMultiplierMin
           + static_cast<double> (index) * stepTimingMultiplierQuarterStep;
}

double PluginProcessor::pulseQuartersForIndex (const int pulseIndexIn)
{
    const auto index = juce::jlimit (0, pulseCount - 1, pulseIndexIn);
    return pulseQuartersTable[static_cast<size_t> (index)];
}

double PluginProcessor::swingSubdivisionForIndex (const int subdivisionIndex)
{
    const auto index = juce::jlimit (0, swingSubdivisionCount - 1, subdivisionIndex);
    return swingSubdivisionValues[static_cast<size_t> (index)];
}

void PluginProcessor::setPulseIndex (const int pulseIndexIn)
{
    pulseIndex.store (juce::jlimit (0, pulseCount - 1, pulseIndexIn), std::memory_order_relaxed);

    for (int pattern = 0; pattern < patternSlotCount; ++pattern)
    {
        for (int row = 0; row < phraseRowCount; ++row)
            rebuildRowTimingLayout (
                modelPatterns[static_cast<size_t> (pattern)].sequencer.rows[static_cast<size_t> (row)]);

        publishPatternToAudio (pattern);
    }
}

int PluginProcessor::getPulseIndex() const
{
    return pulseIndex.load (std::memory_order_relaxed);
}

void PluginProcessor::setSwingPercent (const int percent)
{
    swingPercent.store (clampPercent (percent), std::memory_order_relaxed);
}

int PluginProcessor::getSwingPercent() const
{
    return swingPercent.load (std::memory_order_relaxed);
}

void PluginProcessor::setVelocityHumanizePercent (const int percent)
{
    velocityHumanizePercent.store (clampPercent (percent), std::memory_order_relaxed);
}

int PluginProcessor::getVelocityHumanizePercent() const
{
    return velocityHumanizePercent.load (std::memory_order_relaxed);
}

void PluginProcessor::setTimingHumanizePercent (const int percent)
{
    timingHumanizePercent.store (clampPercent (percent), std::memory_order_relaxed);
}

int PluginProcessor::getTimingHumanizePercent() const
{
    return timingHumanizePercent.load (std::memory_order_relaxed);
}

void PluginProcessor::setSwingSubdivisionIndex (const int subdivisionIndex)
{
    swingSubdivisionIndex.store (
        juce::jlimit (0, swingSubdivisionCount - 1, subdivisionIndex),
        std::memory_order_relaxed);
}

int PluginProcessor::getSwingSubdivisionIndex() const
{
    return swingSubdivisionIndex.load (std::memory_order_relaxed);
}

void PluginProcessor::setRowColorsEnabled (const bool enabled)
{
    rowColorsEnabled.store (enabled ? 1 : 0, std::memory_order_relaxed);
}

bool PluginProcessor::isRowColorsEnabled() const
{
    return rowColorsEnabled.load (std::memory_order_relaxed) != 0;
}

const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    sampleRateHz = sampleRate;
    clearPendingAudioLoopBraceEnable();
    resetLastEmittedTriggers();
    wasPlaying = false;
    clearLoopScheduleAnchor();
    loopScheduleReanchorRequested = false;
    currentPlaybackPpq.store (-1.0, std::memory_order_relaxed);
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetPendingCombinedNoteOffs();
    resetActiveGeneratedNotes();
}

void PluginProcessor::setPhraseNote (int row, int step, int noteNumber)
{
    if (! isValidStep (row, step))
        return;

    const auto value = juce::jlimit (0, 127, noteNumber);
    modelRow (row).notes[static_cast<size_t> (step)] = value;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetNote;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);
}

int PluginProcessor::getPhraseNote (int row, int step) const
{
    if (! isValidStep (row, step))
        return 60;

    return modelRow (row).notes[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseRowMuted (int row, bool muted)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    modelSequencer().muted[static_cast<size_t> (row)] = muted ? 1 : 0;

    if (muted)
        phraseRowFlushNoteOff[static_cast<size_t> (row)].store (1);

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetRowMuted;
    command.row = row;
    command.intValue = muted ? 1 : 0;
    publishCommandToAudio (command);
}

bool PluginProcessor::isPhraseRowMuted (int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return false;

    return modelSequencer().muted[static_cast<size_t> (row)] != 0;
}

void PluginProcessor::setCombinationModeEnabled (const int modeIndex, const bool enabled)
{
    const auto bit = combinationModeBit (modeIndex);

    if (bit == 0)
        return;

    auto& state = modelSequencer();
    const auto previous = clampCombinationModeMask (state.combinationModeMask);
    const auto next = enabled ? (previous | bit) : (previous & ~bit);

    if (next == previous)
        return;

    state.combinationModeMask = next;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetCombinationModeMask;
    command.patternSlot = getViewPatternSlot();
    command.intValue = next;
    publishCommandToAudio (command);

    for (auto& flush : phraseRowFlushNoteOff)
        flush.store (1);
}

bool PluginProcessor::isCombinationModeEnabled (const int modeIndex) const
{
    return combinationModeEnabled (getCombinationModeMask(), modeIndex);
}

int PluginProcessor::getCombinationModeMask() const
{
    return clampCombinationModeMask (modelSequencer().combinationModeMask);
}

int PluginProcessor::getPatternCombinationModeMask (const int patternSlot) const
{
    return clampCombinationModeMask (modelPattern (patternSlot).sequencer.combinationModeMask);
}

void PluginProcessor::setPatternScale (const int root, const int modeIndex)
{
    const auto patternSlot = getViewPatternSlot();
    const auto clampedRoot = clampScaleRoot (root);
    const auto clampedMode = clampScaleModeIndex (modeIndex);

    auto& pattern = modelPattern (patternSlot);
    pattern.scaleRoot = clampedRoot;
    pattern.scaleModeIndex = clampedMode;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternScale;
    command.patternSlot = patternSlot;
    command.step = clampedRoot;
    command.intValue = clampedMode;
    publishCommandToAudio (command);
}

int PluginProcessor::getPatternScaleRoot (const int patternSlot) const
{
    return clampScaleRoot (modelPattern (patternSlot).scaleRoot);
}

int PluginProcessor::getPatternScaleModeIndex (const int patternSlot) const
{
    return clampScaleModeIndex (modelPattern (patternSlot).scaleModeIndex);
}

void PluginProcessor::setPatternNoteBandpass (const int lowMidi, const int highMidi)
{
    const auto patternSlot = getViewPatternSlot();
    const auto bounds = clampNoteBandpassBounds (lowMidi, highMidi);

    auto& pattern = modelPattern (patternSlot);
    pattern.noteBandpassLowMidi = bounds.first;
    pattern.noteBandpassHighMidi = bounds.second;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternNoteBandpass;
    command.patternSlot = patternSlot;
    command.step = bounds.first;
    command.intValue = bounds.second;
    publishCommandToAudio (command);
}

int PluginProcessor::getPatternNoteBandpassLow (const int patternSlot) const
{
    return clampNoteBandpassBounds (modelPattern (patternSlot).noteBandpassLowMidi,
                                    modelPattern (patternSlot).noteBandpassHighMidi)
        .first;
}

int PluginProcessor::getPatternNoteBandpassHigh (const int patternSlot) const
{
    return clampNoteBandpassBounds (modelPattern (patternSlot).noteBandpassLowMidi,
                                    modelPattern (patternSlot).noteBandpassHighMidi)
        .second;
}

void PluginProcessor::setPatternVelocityTiltPivotMidi (const int pivotMidi)
{
    const auto patternSlot = getViewPatternSlot();
    const auto clamped = clampVelocityTiltPivotMidi (pivotMidi);

    auto& pattern = modelPattern (patternSlot);
    pattern.velocityTiltPivotMidi = clamped;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternVelocityTiltPivotMidi;
    command.patternSlot = patternSlot;
    command.intValue = clamped;
    publishCommandToAudio (command);
}

void PluginProcessor::setPatternVelocityTiltAmount (const int amount)
{
    const auto patternSlot = getViewPatternSlot();
    const auto clamped = clampVelocityTiltAmount (amount);

    auto& pattern = modelPattern (patternSlot);
    pattern.velocityTiltAmount = clamped;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternVelocityTiltAmount;
    command.patternSlot = patternSlot;
    command.intValue = clamped;
    publishCommandToAudio (command);
}

int PluginProcessor::getPatternVelocityTiltPivotMidi (const int patternSlot) const
{
    return clampVelocityTiltPivotMidi (modelPattern (patternSlot).velocityTiltPivotMidi);
}

int PluginProcessor::getPatternVelocityTiltAmount (const int patternSlot) const
{
    return clampVelocityTiltAmount (modelPattern (patternSlot).velocityTiltAmount);
}

void PluginProcessor::setPatternGlobalTransposeSemitones (const int semitones)
{
    const auto patternSlot = getViewPatternSlot();
    const auto clamped = clampGlobalTransposeSemitones (semitones);

    modelPattern (patternSlot).globalTransposeSemitones = clamped;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternGlobalTransposeSemitones;
    command.patternSlot = patternSlot;
    command.intValue = clamped;
    publishCommandToAudio (command);
}

int PluginProcessor::getPatternGlobalTransposeSemitones (const int patternSlot) const
{
    return clampGlobalTransposeSemitones (modelPattern (patternSlot).globalTransposeSemitones);
}

void PluginProcessor::setPatternOctavizerDown8vaEnabled (const bool enabled)
{
    const auto patternSlot = getViewPatternSlot();

    auto& pattern = modelPattern (patternSlot);
    pattern.octavizerDown8vaEnabled = enabled ? 1 : 0;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternOctavizerDown8vaEnabled;
    command.patternSlot = patternSlot;
    command.intValue = enabled ? 1 : 0;
    publishCommandToAudio (command);
}

void PluginProcessor::setPatternOctavizerUp8vaEnabled (const bool enabled)
{
    const auto patternSlot = getViewPatternSlot();

    auto& pattern = modelPattern (patternSlot);
    pattern.octavizerUp8vaEnabled = enabled ? 1 : 0;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternOctavizerUp8vaEnabled;
    command.patternSlot = patternSlot;
    command.intValue = enabled ? 1 : 0;
    publishCommandToAudio (command);
}

void PluginProcessor::setPatternOctavizerDown8vaRelativeVelocity (const int relativeVelocity)
{
    const auto patternSlot = getViewPatternSlot();
    const auto clamped = clampOctavizerRelativeVelocity (relativeVelocity);

    auto& pattern = modelPattern (patternSlot);
    pattern.octavizerDown8vaRelativeVelocity = clamped;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternOctavizerDown8vaRelativeVelocity;
    command.patternSlot = patternSlot;
    command.intValue = clamped;
    publishCommandToAudio (command);
}

void PluginProcessor::setPatternOctavizerUp8vaRelativeVelocity (const int relativeVelocity)
{
    const auto patternSlot = getViewPatternSlot();
    const auto clamped = clampOctavizerRelativeVelocity (relativeVelocity);

    auto& pattern = modelPattern (patternSlot);
    pattern.octavizerUp8vaRelativeVelocity = clamped;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternOctavizerUp8vaRelativeVelocity;
    command.patternSlot = patternSlot;
    command.intValue = clamped;
    publishCommandToAudio (command);
}

bool PluginProcessor::isPatternOctavizerDown8vaEnabled (const int patternSlot) const
{
    return modelPattern (patternSlot).octavizerDown8vaEnabled != 0;
}

bool PluginProcessor::isPatternOctavizerUp8vaEnabled (const int patternSlot) const
{
    return modelPattern (patternSlot).octavizerUp8vaEnabled != 0;
}

int PluginProcessor::getPatternOctavizerDown8vaRelativeVelocity (const int patternSlot) const
{
    return clampOctavizerRelativeVelocity (
        modelPattern (patternSlot).octavizerDown8vaRelativeVelocity);
}

int PluginProcessor::getPatternOctavizerUp8vaRelativeVelocity (const int patternSlot) const
{
    return clampOctavizerRelativeVelocity (
        modelPattern (patternSlot).octavizerUp8vaRelativeVelocity);
}

void PluginProcessor::setPatternShimmerEnabled (const bool enabled)
{
    const auto patternSlot = getCurrentPatternSlot();
    auto& pattern = modelPattern (patternSlot);

    pattern.shimmerEnabled = enabled ? 1 : 0;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternShimmerEnabled;
    command.patternSlot = patternSlot;
    command.intValue = enabled ? 1 : 0;
    publishCommandToAudio (command);
}

void PluginProcessor::setPatternShimmerDelayMultiplierIndex (const int multiplierIndex)
{
    const auto patternSlot = getCurrentPatternSlot();
    const auto clamped = clampShimmerDelayMultiplierIndex (multiplierIndex);
    auto& pattern = modelPattern (patternSlot);

    pattern.shimmerDelayMultiplierIndex = clamped;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternShimmerDelayMultiplierIndex;
    command.patternSlot = patternSlot;
    command.intValue = clamped;
    publishCommandToAudio (command);
}

void PluginProcessor::setPatternShimmerFeedbackPercent (const int feedbackPercent)
{
    const auto patternSlot = getCurrentPatternSlot();
    const auto clamped = clampShimmerFeedbackPercent (feedbackPercent);
    auto& pattern = modelPattern (patternSlot);

    pattern.shimmerFeedbackPercent = clamped;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternShimmerFeedbackPercent;
    command.patternSlot = patternSlot;
    command.intValue = clamped;
    publishCommandToAudio (command);
}

void PluginProcessor::setPatternShimmerMixPercent (const int mixPercent)
{
    const auto patternSlot = getCurrentPatternSlot();
    const auto clamped = clampShimmerMixPercent (mixPercent);
    auto& pattern = modelPattern (patternSlot);

    pattern.shimmerMixPercent = clamped;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetPatternShimmerMixPercent;
    command.patternSlot = patternSlot;
    command.intValue = clamped;
    publishCommandToAudio (command);
}

bool PluginProcessor::isPatternShimmerEnabled (const int patternSlot) const
{
    return modelPattern (patternSlot).shimmerEnabled != 0;
}

int PluginProcessor::getPatternShimmerDelayMultiplierIndex (const int patternSlot) const
{
    return clampShimmerDelayMultiplierIndex (
        modelPattern (patternSlot).shimmerDelayMultiplierIndex);
}

int PluginProcessor::getPatternShimmerFeedbackPercent (const int patternSlot) const
{
    return clampShimmerFeedbackPercent (modelPattern (patternSlot).shimmerFeedbackPercent);
}

int PluginProcessor::getPatternShimmerMixPercent (const int patternSlot) const
{
    return clampShimmerMixPercent (modelPattern (patternSlot).shimmerMixPercent);
}

void PluginProcessor::setPatternSeedModeState (
    const int rhythmStep,
    const std::array<SeedingRowState, phraseRowCount>& rowSettings,
    const std::array<int, phraseRowCount>& rowTargets)
{
    const auto patternSlot = getCurrentPatternSlot();
    auto& pattern = modelPattern (patternSlot);

    pattern.seedingRhythmStep = clampSeedingRhythmStep (rhythmStep);

    for (int row = 0; row < phraseRowCount; ++row)
    {
        pattern.seedingRows[static_cast<size_t> (row)] =
            clampSeedingRowState (rowSettings[static_cast<size_t> (row)]);
        pattern.seedingRowTargets[static_cast<size_t> (row)] =
            rowTargets[static_cast<size_t> (row)] != 0 ? 1 : 0;
    }
}

int PluginProcessor::getPatternSeedingRhythmStep (const int patternSlot) const
{
    return clampSeedingRhythmStep (modelPattern (patternSlot).seedingRhythmStep);
}

PluginProcessor::SeedingRowState PluginProcessor::getPatternSeedingRowState (const int patternSlot,
                                                                             const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return {};

    return clampSeedingRowState (modelPattern (patternSlot).seedingRows[static_cast<size_t> (row)]);
}

bool PluginProcessor::isPatternSeedingRowTargeted (const int patternSlot, const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return false;

    return modelPattern (patternSlot).seedingRowTargets[static_cast<size_t> (row)] != 0;
}

void PluginProcessor::reverseRowSteps (PhraseRowSteps& steps)
{
    for (int left = 0, right = steps.stepCount - 1; left < right; ++left, --right)
    {
        const auto leftIndex = static_cast<size_t> (left);
        const auto rightIndex = static_cast<size_t> (right);

        std::swap (steps.notes[leftIndex], steps.notes[rightIndex]);
        std::swap (steps.timingMultiplier[leftIndex], steps.timingMultiplier[rightIndex]);
        std::swap (steps.durationFraction[leftIndex], steps.durationFraction[rightIndex]);
        std::swap (steps.velocity[leftIndex], steps.velocity[rightIndex]);
        std::swap (steps.stepMuted[leftIndex], steps.stepMuted[rightIndex]);
        std::swap (steps.stepSkipped[leftIndex], steps.stepSkipped[rightIndex]);
        std::swap (steps.probability[leftIndex], steps.probability[rightIndex]);
        std::swap (steps.cycle[leftIndex], steps.cycle[rightIndex]);
        std::swap (steps.cycleOffset[leftIndex], steps.cycleOffset[rightIndex]);
    }

    rebuildRowTimingLayout (steps);
}

void PluginProcessor::reversePhraseRowSteps (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    auto& steps = modelRow (row);

    if (steps.stepCount <= 1)
        return;

    reverseRowSteps (steps);
    phraseRowFlushNoteOff[static_cast<size_t> (row)].store (1);
    publishRowToAudio (row);
}

bool PluginProcessor::applyRowStepOrder (PhraseRowSteps& steps,
                                         const std::array<int, maxPhraseStepsPerRow>& stepOrder,
                                         const int orderSize)
{
    if (orderSize != steps.stepCount || orderSize <= 1)
        return false;

    std::array<int, maxPhraseStepsPerRow> seen {};

    for (int index = 0; index < orderSize; ++index)
    {
        const auto source = stepOrder[static_cast<size_t> (index)];

        if (source < 0 || source >= orderSize)
            return false;

        if (seen[static_cast<size_t> (source)] != 0)
            return false;

        seen[static_cast<size_t> (source)] = 1;
    }

    const auto original = steps;

    for (int destination = 0; destination < orderSize; ++destination)
    {
        const auto destinationIndex = static_cast<size_t> (destination);
        const auto sourceIndex = static_cast<size_t> (stepOrder[destinationIndex]);

        steps.notes[destinationIndex] = original.notes[sourceIndex];
        steps.timingMultiplier[destinationIndex] = original.timingMultiplier[sourceIndex];
        steps.durationFraction[destinationIndex] = original.durationFraction[sourceIndex];
        steps.velocity[destinationIndex] = original.velocity[sourceIndex];
        steps.stepMuted[destinationIndex] = original.stepMuted[sourceIndex];
        steps.stepSkipped[destinationIndex] = original.stepSkipped[sourceIndex];
        steps.probability[destinationIndex] = original.probability[sourceIndex];
        steps.cycle[destinationIndex] = original.cycle[sourceIndex];
        steps.cycleOffset[destinationIndex] = original.cycleOffset[sourceIndex];
    }

    rebuildRowTimingLayout (steps);
    return true;
}

void PluginProcessor::reorderPhraseRowSteps (const int row,
                                             const std::array<int, maxPhraseStepsPerRow>& stepOrder,
                                             const int orderSize)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    auto& steps = modelRow (row);

    if (! applyRowStepOrder (steps, stepOrder, orderSize))
        return;

    phraseRowFlushNoteOff[static_cast<size_t> (row)].store (1);
    publishRowToAudio (row);
}

void PluginProcessor::setPhraseRowTimingOffset (const int row, const int offsetIndex)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    const auto value = juce::jlimit (0, rowTimingOffsetCount - 1, offsetIndex);
    modelSequencer().timingOffset[static_cast<size_t> (row)] = value;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetRowTimingOffset;
    command.row = row;
    command.intValue = value;
    publishCommandToAudio (command);
}

int PluginProcessor::getPhraseRowTimingOffset (const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return defaultRowTimingOffsetIndex;

    return modelSequencer().timingOffset[static_cast<size_t> (row)];
}

void PluginProcessor::setPhraseRowMidiChannel (const int row, const int channel)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    const auto value =
        juce::jlimit (minPhraseRowMidiChannel, maxPhraseRowMidiChannel, channel);
    modelSequencer().midiChannel[static_cast<size_t> (row)] = value;
    phraseRowFlushNoteOff[static_cast<size_t> (row)].store (1);

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetRowMidiChannel;
    command.row = row;
    command.intValue = value;
    publishCommandToAudio (command);
}

int PluginProcessor::getPhraseRowMidiChannel (const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return defaultPhraseRowMidiChannelForRow (row);

    return modelSequencer().midiChannel[static_cast<size_t> (row)];
}

int PluginProcessor::defaultPhraseRowMidiChannelForRow (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return defaultPhraseRowMidiChannel;

    return defaultPhraseRowMidiChannels[static_cast<size_t> (row)];
}

void PluginProcessor::setPhraseStepTimingMultiplier (const int row,
                                                    const int step,
                                                    const int multiplierIndex)
{
    if (! isValidStep (row, step))
        return;

    const auto value = juce::jlimit (0, stepTimingMultiplierCount - 1, multiplierIndex);
    modelRow (row).timingMultiplier[static_cast<size_t> (step)] = value;
    rebuildRowTimingLayout (modelRow (row));

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepTimingMultiplier;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);
}

int PluginProcessor::getPhraseStepTimingMultiplier (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return defaultStepTimingMultiplierIndex;

    return modelRow (row).timingMultiplier[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseStepDurationFraction (const int row,
                                                   const int step,
                                                   const double fraction)
{
    if (! isValidStep (row, step))
        return;

    const auto value = clampStepDurationFraction (fraction);
    modelRow (row).durationFraction[static_cast<size_t> (step)] = value;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepDurationFraction;
    command.row = row;
    command.step = step;
    command.doubleValue = value;
    publishCommandToAudio (command);
}

double PluginProcessor::getPhraseStepDurationFraction (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return defaultStepDurationFraction;

    return modelRow (row).durationFraction[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseStepVelocity (const int row, const int step, const int velocity)
{
    if (! isValidStep (row, step))
        return;

    const auto value = juce::jlimit (0, 127, velocity);
    modelRow (row).velocity[static_cast<size_t> (step)] = value;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepVelocity;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);
}

int PluginProcessor::getPhraseStepVelocity (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return defaultStepVelocity;

    return modelRow (row).velocity[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseStepMuted (const int row, const int step, const bool muted)
{
    if (! isValidStep (row, step))
        return;

    auto& steps = modelRow (row);
    const auto index = static_cast<size_t> (step);
    const auto value = muted ? 1 : 0;
    steps.stepMuted[index] = value;

    const auto clearedSkip = muted && steps.stepSkipped[index] != 0;

    if (clearedSkip)
    {
        steps.stepSkipped[index] = 0;
        rebuildRowTimingLayout (steps);
    }

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepMuted;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);

    if (clearedSkip)
    {
        SequencerCommand skipCommand;
        skipCommand.type = SequencerCommand::Type::SetStepSkipped;
        skipCommand.row = row;
        skipCommand.step = step;
        skipCommand.intValue = 0;
        publishCommandToAudio (skipCommand);
    }
}

bool PluginProcessor::isPhraseStepMuted (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return false;

    return modelRow (row).stepMuted[static_cast<size_t> (step)] != 0;
}

void PluginProcessor::setPhraseStepSkipped (const int row, const int step, const bool skipped)
{
    if (! isValidStep (row, step))
        return;

    auto& steps = modelRow (row);
    const auto index = static_cast<size_t> (step);
    const auto value = skipped ? 1 : 0;
    const auto clearedMute = skipped && steps.stepMuted[index] != 0;

    if (clearedMute)
        steps.stepMuted[index] = 0;

    steps.stepSkipped[index] = value;
    rebuildRowTimingLayout (steps);

    if (clearedMute)
    {
        SequencerCommand muteCommand;
        muteCommand.type = SequencerCommand::Type::SetStepMuted;
        muteCommand.row = row;
        muteCommand.step = step;
        muteCommand.intValue = 0;
        publishCommandToAudio (muteCommand);
    }

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepSkipped;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);
}

bool PluginProcessor::isPhraseStepSkipped (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return false;

    return modelRow (row).stepSkipped[static_cast<size_t> (step)] != 0;
}

void PluginProcessor::setPhraseStepProbability (const int row, const int step, const int probability)
{
    if (! isValidStep (row, step))
        return;

    const auto value = clampStepProbability (probability);
    modelRow (row).probability[static_cast<size_t> (step)] = value;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepProbability;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);
}

int PluginProcessor::getPhraseStepProbability (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return PluginProcessor::defaultStepProbability;

    return modelRow (row).probability[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseStepCycle (const int row, const int step, const int cycle)
{
    if (! isValidStep (row, step))
        return;

    auto& steps = modelRow (row);
    const auto index = static_cast<size_t> (step);
    const auto value = clampStepCycle (cycle);
    const auto previousMask = steps.cycleOffset[index];
    steps.cycle[index] = value;
    steps.cycleOffset[index] = clampStepCycleMask (previousMask, value);

    if (steps.cycleOffset[index] == 0)
        steps.cycleOffset[index] = defaultStepCycleMask;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepCycle;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);

    if (steps.cycleOffset[index] != previousMask)
    {
        SequencerCommand offsetCommand;
        offsetCommand.type = SequencerCommand::Type::SetStepCycleOffset;
        offsetCommand.row = row;
        offsetCommand.step = step;
        offsetCommand.intValue = steps.cycleOffset[index];
        publishCommandToAudio (offsetCommand);
    }
}

int PluginProcessor::getPhraseStepCycle (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return PluginProcessor::defaultStepCycle;

    return modelRow (row).cycle[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseStepCycleOffset (const int row, const int step, const int cycleMask)
{
    if (! isValidStep (row, step))
        return;

    auto& steps = modelRow (row);
    const auto index = static_cast<size_t> (step);
    auto value = clampStepCycleMask (cycleMask, steps.cycle[index]);

    if (value == 0)
        value = defaultStepCycleMask;

    steps.cycleOffset[index] = value;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepCycleOffset;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);
}

int PluginProcessor::getPhraseStepCycleOffset (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return PluginProcessor::defaultStepCycleMask;

    return modelRow (row).cycleOffset[static_cast<size_t> (step)];
}

int PluginProcessor::getPhraseRowStepCount (const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return 0;

    return modelRow (row).stepCount;
}

void PluginProcessor::replacePhraseRowSteps (
    const int row,
    const int stepCount,
    const std::array<int, maxPhraseStepsPerRow>& notes,
    const std::array<int, maxPhraseStepsPerRow>& timingMultiplier,
    const std::array<double, maxPhraseStepsPerRow>& durationFraction,
    const std::array<int, maxPhraseStepsPerRow>& velocity,
    const std::array<int, maxPhraseStepsPerRow>& stepMuted,
    const std::array<int, maxPhraseStepsPerRow>& stepSkipped,
    const std::array<int, maxPhraseStepsPerRow>& probability,
    const std::array<int, maxPhraseStepsPerRow>& cycle,
    const std::array<int, maxPhraseStepsPerRow>& cycleOffset)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    auto& steps = modelRow (row);
    initialiseRowDefaults (steps,
                           row,
                           juce::jlimit (0, maxPhraseStepsPerRow, stepCount),
                           modelPattern (getViewPatternSlot()).scaleRoot);

    for (int step = 0; step < steps.stepCount; ++step)
    {
        const auto index = static_cast<size_t> (step);
        steps.notes[index] = juce::jlimit (0, 127, notes[index]);
        steps.timingMultiplier[index] =
            juce::jlimit (0, stepTimingMultiplierCount - 1, timingMultiplier[index]);
        steps.durationFraction[index] = clampStepDurationFraction (durationFraction[index]);
        steps.velocity[index] = juce::jlimit (0, 127, velocity[index]);
        steps.stepMuted[index] = stepMuted[index] != 0 ? 1 : 0;
        steps.stepSkipped[index] = stepSkipped[index] != 0 ? 1 : 0;

        if (steps.stepMuted[index] != 0 && steps.stepSkipped[index] != 0)
            steps.stepMuted[index] = 0;

        steps.probability[index] = clampStepProbability (probability[index]);
        steps.cycle[index] = clampStepCycle (cycle[index]);
        steps.cycleOffset[index] = clampStepCycleMask (cycleOffset[index], steps.cycle[index]);
    }

    rebuildRowTimingLayout (steps);
    phraseRowFlushNoteOff[static_cast<size_t> (row)].store (1);
    publishRowToAudio (row);
}

void PluginProcessor::removePhraseStep (const int row, const int step)
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return;

    const auto count = getPhraseRowStepCount (row);

    if (step >= count)
        return;

    auto& steps = modelRow (row);

    for (int index = step; index < count - 1; ++index)
    {
        const auto current = static_cast<size_t> (index);
        const auto next = static_cast<size_t> (index + 1);
        steps.notes[current] = steps.notes[next];
        steps.timingMultiplier[current] = steps.timingMultiplier[next];
        steps.durationFraction[current] = steps.durationFraction[next];
        steps.velocity[current] = steps.velocity[next];
        steps.stepMuted[current] = steps.stepMuted[next];
        steps.stepSkipped[current] = steps.stepSkipped[next];
        steps.probability[current] = steps.probability[next];
        steps.cycle[current] = steps.cycle[next];
        steps.cycleOffset[current] = steps.cycleOffset[next];
    }

    steps.stepCount = count - 1;
    rebuildRowTimingLayout (steps);
    SequencerCommand command;
    command.type = SequencerCommand::Type::RemoveStep;
    command.row = row;
    command.step = step;
    publishCommandToAudio (command);
}

void PluginProcessor::insertPhraseStep (const int row, const int step)
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return;

    const auto count = getPhraseRowStepCount (row);

    if (step > count || count >= maxPhraseStepsPerRow)
        return;

    auto& steps = modelRow (row);

    for (int index = count; index > step; --index)
    {
        const auto current = static_cast<size_t> (index);
        const auto previous = static_cast<size_t> (index - 1);
        steps.notes[current] = steps.notes[previous];
        steps.timingMultiplier[current] = steps.timingMultiplier[previous];
        steps.durationFraction[current] = steps.durationFraction[previous];
        steps.velocity[current] = steps.velocity[previous];
        steps.stepMuted[current] = steps.stepMuted[previous];
        steps.stepSkipped[current] = steps.stepSkipped[previous];
        steps.probability[current] = steps.probability[previous];
        steps.cycle[current] = steps.cycle[previous];
        steps.cycleOffset[current] = steps.cycleOffset[previous];
    }

    const auto insertIndex = static_cast<size_t> (step);
    steps.notes[insertIndex] = defaultNoteForRow (row);
    steps.timingMultiplier[insertIndex] = defaultStepTimingMultiplierIndex;
    steps.durationFraction[insertIndex] = defaultStepDurationFraction;
    steps.velocity[insertIndex] = defaultStepVelocity;
    steps.stepMuted[insertIndex] = 0;
    steps.stepSkipped[insertIndex] = 0;
    steps.probability[insertIndex] = PluginProcessor::defaultStepProbability;
    steps.cycle[insertIndex] = PluginProcessor::defaultStepCycle;
    steps.cycleOffset[insertIndex] = PluginProcessor::defaultStepCycleMask;
    steps.stepCount = count + 1;
    rebuildRowTimingLayout (steps);
    SequencerCommand command;
    command.type = SequencerCommand::Type::InsertStep;
    command.row = row;
    command.step = step;
    publishCommandToAudio (command);
}

void PluginProcessor::duplicatePhraseStep (const int row, const int step)
{
    if (row < 0 || row >= phraseRowCount || step <= 0)
        return;

    const auto count = getPhraseRowStepCount (row);

    if (step > count || count >= maxPhraseStepsPerRow)
        return;

    auto& steps = modelRow (row);
    const auto sourceIndex = static_cast<size_t> (step - 1);

    for (int index = count; index > step; --index)
    {
        const auto current = static_cast<size_t> (index);
        const auto previous = static_cast<size_t> (index - 1);
        steps.notes[current] = steps.notes[previous];
        steps.timingMultiplier[current] = steps.timingMultiplier[previous];
        steps.durationFraction[current] = steps.durationFraction[previous];
        steps.velocity[current] = steps.velocity[previous];
        steps.stepMuted[current] = steps.stepMuted[previous];
        steps.stepSkipped[current] = steps.stepSkipped[previous];
        steps.probability[current] = steps.probability[previous];
        steps.cycle[current] = steps.cycle[previous];
        steps.cycleOffset[current] = steps.cycleOffset[previous];
    }

    const auto insertIndex = static_cast<size_t> (step);
    steps.notes[insertIndex] = steps.notes[sourceIndex];
    steps.timingMultiplier[insertIndex] = steps.timingMultiplier[sourceIndex];
    steps.durationFraction[insertIndex] = steps.durationFraction[sourceIndex];
    steps.velocity[insertIndex] = steps.velocity[sourceIndex];
    steps.stepMuted[insertIndex] = steps.stepMuted[sourceIndex];
    steps.stepSkipped[insertIndex] = steps.stepSkipped[sourceIndex];
    steps.probability[insertIndex] = steps.probability[sourceIndex];
    steps.cycle[insertIndex] = steps.cycle[sourceIndex];
    steps.cycleOffset[insertIndex] = steps.cycleOffset[sourceIndex];
    steps.stepCount = count + 1;
    rebuildRowTimingLayout (steps);
    SequencerCommand command;
    command.type = SequencerCommand::Type::DuplicateStep;
    command.row = row;
    command.step = step;
    publishCommandToAudio (command);
}

void PluginProcessor::movePhraseStep (const int row, const int fromStep, const int toStep)
{
    if (row < 0 || row >= phraseRowCount || fromStep < 0 || toStep < 0)
        return;

    const auto count = getPhraseRowStepCount (row);

    if (fromStep >= count || toStep >= count || fromStep == toStep)
        return;

    auto& steps = modelRow (row);
    const auto note = steps.notes[static_cast<size_t> (fromStep)];
    const auto timingMultiplier = steps.timingMultiplier[static_cast<size_t> (fromStep)];
    const auto durationFraction = steps.durationFraction[static_cast<size_t> (fromStep)];
    const auto velocity = steps.velocity[static_cast<size_t> (fromStep)];
    const auto stepMuted = steps.stepMuted[static_cast<size_t> (fromStep)];
    const auto stepSkipped = steps.stepSkipped[static_cast<size_t> (fromStep)];
    const auto probability = steps.probability[static_cast<size_t> (fromStep)];
    const auto cycle = steps.cycle[static_cast<size_t> (fromStep)];
    const auto cycleOffset = steps.cycleOffset[static_cast<size_t> (fromStep)];

    if (fromStep < toStep)
    {
        for (int index = fromStep; index < toStep; ++index)
        {
            const auto nextIndex = index + 1;

            steps.notes[static_cast<size_t> (index)] = steps.notes[static_cast<size_t> (nextIndex)];
            steps.timingMultiplier[static_cast<size_t> (index)] =
                steps.timingMultiplier[static_cast<size_t> (nextIndex)];
            steps.durationFraction[static_cast<size_t> (index)] =
                steps.durationFraction[static_cast<size_t> (nextIndex)];
            steps.velocity[static_cast<size_t> (index)] = steps.velocity[static_cast<size_t> (nextIndex)];
            steps.stepMuted[static_cast<size_t> (index)] = steps.stepMuted[static_cast<size_t> (nextIndex)];
            steps.stepSkipped[static_cast<size_t> (index)] = steps.stepSkipped[static_cast<size_t> (nextIndex)];
            steps.probability[static_cast<size_t> (index)] =
                steps.probability[static_cast<size_t> (nextIndex)];
            steps.cycle[static_cast<size_t> (index)] = steps.cycle[static_cast<size_t> (nextIndex)];
            steps.cycleOffset[static_cast<size_t> (index)] =
                steps.cycleOffset[static_cast<size_t> (nextIndex)];
        }
    }
    else
    {
        for (int index = fromStep; index > toStep; --index)
        {
            const auto prevIndex = index - 1;

            steps.notes[static_cast<size_t> (index)] = steps.notes[static_cast<size_t> (prevIndex)];
            steps.timingMultiplier[static_cast<size_t> (index)] =
                steps.timingMultiplier[static_cast<size_t> (prevIndex)];
            steps.durationFraction[static_cast<size_t> (index)] =
                steps.durationFraction[static_cast<size_t> (prevIndex)];
            steps.velocity[static_cast<size_t> (index)] = steps.velocity[static_cast<size_t> (prevIndex)];
            steps.stepMuted[static_cast<size_t> (index)] = steps.stepMuted[static_cast<size_t> (prevIndex)];
            steps.stepSkipped[static_cast<size_t> (index)] = steps.stepSkipped[static_cast<size_t> (prevIndex)];
            steps.probability[static_cast<size_t> (index)] =
                steps.probability[static_cast<size_t> (prevIndex)];
            steps.cycle[static_cast<size_t> (index)] = steps.cycle[static_cast<size_t> (prevIndex)];
            steps.cycleOffset[static_cast<size_t> (index)] =
                steps.cycleOffset[static_cast<size_t> (prevIndex)];
        }
    }

    steps.notes[static_cast<size_t> (toStep)] = note;
    steps.timingMultiplier[static_cast<size_t> (toStep)] = timingMultiplier;
    steps.durationFraction[static_cast<size_t> (toStep)] = durationFraction;
    steps.velocity[static_cast<size_t> (toStep)] = velocity;
    steps.stepMuted[static_cast<size_t> (toStep)] = stepMuted;
    steps.stepSkipped[static_cast<size_t> (toStep)] = stepSkipped;
    steps.probability[static_cast<size_t> (toStep)] = probability;
    steps.cycle[static_cast<size_t> (toStep)] = cycle;
    steps.cycleOffset[static_cast<size_t> (toStep)] = cycleOffset;
    rebuildRowTimingLayout (steps);
    SequencerCommand command;
    command.type = SequencerCommand::Type::MoveStep;
    command.row = row;
    command.step = fromStep;
    command.toStep = toStep;
    publishCommandToAudio (command);
}

double PluginProcessor::playbackBeatForUi() const
{
    const auto currentPpq = currentPlaybackPpq.load (std::memory_order_relaxed);

    if (currentPpq < 0.0)
        return -1.0;

    const auto& loop = modelLoopBrace();

    if (loop.enabled != 0)
    {
        const auto loopLength = loop.endQuarters - loop.startQuarters;

        if (loopLength > 0.0)
            return mapTransportToLoopSchedulePpq (currentPpq);
    }

    return mapTransportToPatternSchedulePpq (currentPpq);
}

void PluginProcessor::clearLoopScheduleAnchor()
{
    loopScheduleAnchorTransportPpq = -1.0;
}

void PluginProcessor::reanchorLoopScheduleAt (const double transportPpq)
{
    const auto& loop = audioLoopBrace();
    const auto loopLength = loop.endQuarters - loop.startQuarters;

    if (loop.enabled == 0 || loopLength <= 0.0)
    {
        clearLoopScheduleAnchor();
        return;
    }

    loopScheduleAnchorTransportPpq = transportPpq;
}

double PluginProcessor::mapTransportToLoopSchedulePpq (const double transportPpq) const
{
    const auto& loop = audioLoopBrace();
    const auto loopLength = loop.endQuarters - loop.startQuarters;
    constexpr auto epsilon = 1.0e-9;

    if (loop.enabled == 0 || loopLength <= epsilon || loopScheduleAnchorTransportPpq < -epsilon)
        return transportPpq;

    return loop.startQuarters
           + positiveMod (transportPpq - loopScheduleAnchorTransportPpq, loopLength);
}

void PluginProcessor::clearPatternScheduleAnchor()
{
    patternScheduleAnchorTransportPpq = -1.0;
}

void PluginProcessor::reanchorPatternScheduleAt (const double transportPpq)
{
    patternScheduleAnchorTransportPpq = transportPpq;
}

double PluginProcessor::mapTransportToPatternSchedulePpq (const double transportPpq) const
{
    constexpr auto epsilon = 1.0e-9;

    if (patternScheduleAnchorTransportPpq < -epsilon)
        return transportPpq;

    return transportPpq - patternScheduleAnchorTransportPpq;
}

void PluginProcessor::setLoopBraceEnabled (const bool enabled)
{
    modelLoopBrace().enabled = enabled ? 1 : 0;
    publishLoopBraceCommandToAudio (SequencerCommand::Type::SetLoopBraceEnabled,
                                    getViewPatternSlot());
}

bool PluginProcessor::isLoopBraceEnabled() const
{
    return modelLoopBrace().enabled != 0;
}

void PluginProcessor::setLoopBraceStartQuarters (const double startQuarters)
{
    auto& loop = modelLoopBrace();
    loop.startQuarters = clampLoopBraceStart (startQuarters, loop.endQuarters);
    publishLoopBraceCommandToAudio (SequencerCommand::Type::SetLoopBraceStart,
                                    getViewPatternSlot());
}

double PluginProcessor::getLoopBraceStartQuarters() const
{
    return modelLoopBrace().startQuarters;
}

void PluginProcessor::setLoopBraceEndQuarters (const double endQuarters)
{
    auto& loop = modelLoopBrace();
    loop.endQuarters = clampLoopBraceEnd (endQuarters, loop.startQuarters);
    publishLoopBraceCommandToAudio (SequencerCommand::Type::SetLoopBraceEnd,
                                    getViewPatternSlot());
}

double PluginProcessor::getLoopBraceEndQuarters() const
{
    return modelLoopBrace().endQuarters;
}

void PluginProcessor::deactivateLoopBraceForPatternSelection (const int patternSlot)
{
    const auto slot = clampPatternSlot (patternSlot);
    const auto audioSlot = clampPatternSlot (audioActivePatternSlot);

    currentLoopSlot.store (-1, std::memory_order_release);
    audioActiveLoopSlot.store (-1, std::memory_order_release);
    pendingAudioLoopSlot.store (-1, std::memory_order_release);
    clearPendingAudioLoopBraceEnable();
    modelPattern (slot).loopBrace.enabled = 0;
    audioPatterns[static_cast<size_t> (slot)].loopBrace.enabled = 0;
    audioPatterns[static_cast<size_t> (audioSlot)].loopBrace.enabled = 0;

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetLoopBraceEnabled;
    command.patternSlot = slot;
    command.intValue = 0;
    publishCommandToAudio (command);

    if (audioSlot != slot)
    {
        SequencerCommand audioCommand;
        audioCommand.type = SequencerCommand::Type::SetLoopBraceEnabled;
        audioCommand.patternSlot = audioSlot;
        audioCommand.intValue = 0;
        publishCommandToAudio (audioCommand);
    }
}

bool PluginProcessor::isPatternOutputArmed() const
{
    return patternOutputArmed.load (std::memory_order_acquire) != 0;
}

int PluginProcessor::getViewPatternSlot() const
{
    const auto current = currentModelPatternSlot.load (std::memory_order_acquire);

    if (current >= 0 && current < patternSlotCount)
        return current;

    return clampPatternSlot (lastViewPatternSlot);
}

void PluginProcessor::deactivatePatternOutput()
{
    const auto current = currentModelPatternSlot.load (std::memory_order_acquire);

    if (current >= 0 && current < patternSlotCount)
        lastViewPatternSlot = current;

    patternOutputArmed.store (0, std::memory_order_release);
    currentModelPatternSlot.store (-1, std::memory_order_release);
    currentLoopSlot.store (-1, std::memory_order_release);
    audioActiveLoopSlot.store (-1, std::memory_order_release);
    pendingAudioPatternSlot.store (-1, std::memory_order_release);
    pendingAudioLoopSlot.store (-1, std::memory_order_release);
    clearPendingAudioLoopBraceEnable();

    deactivateLoopBraceForPatternSelection (lastViewPatternSlot);
    muteFlushRequested.store (1, std::memory_order_release);
}

void PluginProcessor::applyMuteOutputSilence (juce::MidiBuffer& midiMessages)
{
    if (muteFlushRequested.exchange (0, std::memory_order_acq_rel) == 0)
        return;

    flushActiveGeneratedNotes (0, midiMessages);
    resetLastEmittedTriggers();
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetPendingCombinedNoteOffs();
    resetStepCycleCounters();
}

void PluginProcessor::requestAudioPatternSlot (const int patternSlot)
{
    pendingAudioPatternSlot.store (clampPatternSlot (patternSlot), std::memory_order_release);
}

void PluginProcessor::applyAudioPatternSlot (const int patternSlot,
                                             const double reanchorTransportPpq)
{
    const auto slot = clampPatternSlot (patternSlot);

    if (audioActivePatternSlot == slot)
    {
        pendingAudioPatternSlot.store (-1, std::memory_order_release);
        return;
    }

    clearPendingAudioLoopBraceEnable();
    audioActivePatternSlot = slot;
    pendingAudioPatternSlot.store (-1, std::memory_order_release);
    currentLoopSlot.store (-1, std::memory_order_release);
    audioActiveLoopSlot.store (-1, std::memory_order_release);
    audioPatterns[static_cast<size_t> (slot)].loopBrace.enabled = 0;
    clearLoopScheduleAnchor();

    if (reanchorTransportPpq >= 0.0)
        reanchorPatternScheduleAt (reanchorTransportPpq);

    resetLastEmittedTriggers();
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetPendingCombinedNoteOffs();
    resetStepCycleCounters();
}

void PluginProcessor::setCurrentPatternSlot (const int patternSlot)
{
    const auto slot = clampPatternSlot (patternSlot);

    patternOutputArmed.store (1, std::memory_order_release);
    lastViewPatternSlot = slot;

    const auto activeLoop = getCurrentLoopSlot();
    const auto breakingLoopSelection =
        activeLoop >= 0
        && isLoopSlotAssigned (activeLoop)
        && clampPatternSlot (getLoopSlotPatternSlot (activeLoop)) != slot;

    currentModelPatternSlot.store (slot, std::memory_order_release);

    if (breakingLoopSelection)
    {
        deactivateLoopBraceForPatternSelection (slot);

        // Defer the audio switch while playing so note-offs flush on the audio
        // thread at the next pulse boundary (same as pattern/pattern switches).
        if (wasPlaying)
            requestAudioPatternSlot (slot);
        else
            applyAudioPatternSlot (slot);
    }
    else if (activeLoop < 0)
    {
        deactivateLoopBraceForPatternSelection (slot);
        requestAudioPatternSlot (slot);
    }
    else
        requestAudioPatternSlot (slot);

    if (patternSlotParameter != nullptr && patternSlotParameter->get() != slot + 1)
        patternSlotParameter->setValueNotifyingHost (patternSlotParameter->convertTo0to1 (slot + 1));

    lastObservedParameterPatternSlot = slot;
}

int PluginProcessor::getCurrentPatternSlot() const
{
    return currentModelPatternSlot.load (std::memory_order_acquire);
}

int PluginProcessor::getAudioPatternSlot() const
{
    return clampPatternSlot (audioActivePatternSlot);
}

void PluginProcessor::clearPatternSlot (const int patternSlot)
{
    const auto slot = clampPatternSlot (patternSlot);
    initialisePatternDefaults (modelPatterns[static_cast<size_t> (slot)]);
    publishPatternToAudio (slot);

    if (slot == getCurrentPatternSlot()
        || (getCurrentPatternSlot() < 0 && slot == getViewPatternSlot()))
    {
        currentLoopSlot.store (-1, std::memory_order_release);
        audioActiveLoopSlot.store (-1, std::memory_order_release);
    }
}

void PluginProcessor::copyPatternSlot (const int sourcePatternSlot,
                                       const int destinationPatternSlot)
{
    const auto sourceSlot = clampPatternSlot (sourcePatternSlot);
    const auto destinationSlot = clampPatternSlot (destinationPatternSlot);

    if (sourceSlot == destinationSlot)
        return;

    modelPatterns[static_cast<size_t> (destinationSlot)] =
        modelPatterns[static_cast<size_t> (sourceSlot)];
    publishPatternToAudio (destinationSlot);
}

int PluginProcessor::getPatternPhraseRowStepCount (const int patternSlot, const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return 0;

    return modelPattern (patternSlot).sequencer.rows[static_cast<size_t> (row)].stepCount;
}

int PluginProcessor::getPatternPhraseNote (const int patternSlot, const int row, const int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0
        || step >= getPatternPhraseRowStepCount (patternSlot, row))
        return defaultStepNoteForScaleRoot (modelPattern (patternSlot).scaleRoot);

    return modelPattern (patternSlot).sequencer.rows[static_cast<size_t> (row)]
        .notes[static_cast<size_t> (step)];
}

bool PluginProcessor::isPatternPhraseRowMuted (const int patternSlot, const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return false;

    return modelPattern (patternSlot).sequencer.muted[static_cast<size_t> (row)] != 0;
}

int PluginProcessor::getPatternPhraseRowTimingOffset (const int patternSlot, const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return defaultRowTimingOffsetIndex;

    return modelPattern (patternSlot).sequencer.timingOffset[static_cast<size_t> (row)];
}

int PluginProcessor::getPatternPhraseRowMidiChannel (const int patternSlot, const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return defaultPhraseRowMidiChannelForRow (row);

    return modelPattern (patternSlot).sequencer.midiChannel[static_cast<size_t> (row)];
}

const PluginProcessor::PhraseRowSteps* PluginProcessor::patternRowForStep (const int patternSlot,
                                                                           const int row,
                                                                           const int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return nullptr;

    const auto& steps = modelPattern (patternSlot).sequencer.rows[static_cast<size_t> (row)];

    return step < steps.stepCount ? &steps : nullptr;
}

int PluginProcessor::getPatternPhraseStepTimingMultiplier (const int patternSlot,
                                                           const int row,
                                                           const int step) const
{
    if (const auto* steps = patternRowForStep (patternSlot, row, step))
        return steps->timingMultiplier[static_cast<size_t> (step)];

    return defaultStepTimingMultiplierIndex;
}

double PluginProcessor::getPatternPhraseStepDurationFraction (const int patternSlot,
                                                              const int row,
                                                              const int step) const
{
    if (const auto* steps = patternRowForStep (patternSlot, row, step))
        return steps->durationFraction[static_cast<size_t> (step)];

    return defaultStepDurationFraction;
}

int PluginProcessor::getPatternPhraseStepVelocity (const int patternSlot,
                                                   const int row,
                                                   const int step) const
{
    if (const auto* steps = patternRowForStep (patternSlot, row, step))
        return steps->velocity[static_cast<size_t> (step)];

    return defaultStepVelocity;
}

bool PluginProcessor::isPatternPhraseStepMuted (const int patternSlot,
                                                const int row,
                                                const int step) const
{
    if (const auto* steps = patternRowForStep (patternSlot, row, step))
        return steps->stepMuted[static_cast<size_t> (step)] != 0;

    return false;
}

bool PluginProcessor::isPatternPhraseStepSkipped (const int patternSlot,
                                                  const int row,
                                                  const int step) const
{
    if (const auto* steps = patternRowForStep (patternSlot, row, step))
        return steps->stepSkipped[static_cast<size_t> (step)] != 0;

    return false;
}

int PluginProcessor::getPatternPhraseStepProbability (const int patternSlot,
                                                      const int row,
                                                      const int step) const
{
    if (const auto* steps = patternRowForStep (patternSlot, row, step))
        return steps->probability[static_cast<size_t> (step)];

    return defaultStepProbability;
}

int PluginProcessor::getPatternPhraseStepCycle (const int patternSlot,
                                                const int row,
                                                const int step) const
{
    if (const auto* steps = patternRowForStep (patternSlot, row, step))
        return steps->cycle[static_cast<size_t> (step)];

    return defaultStepCycle;
}

int PluginProcessor::getPatternPhraseStepCycleOffset (const int patternSlot,
                                                      const int row,
                                                      const int step) const
{
    if (const auto* steps = patternRowForStep (patternSlot, row, step))
        return steps->cycleOffset[static_cast<size_t> (step)];

    return defaultStepCycleMask;
}

bool PluginProcessor::isPatternLoopBraceEnabled (const int patternSlot) const
{
    return modelPattern (patternSlot).loopBrace.enabled != 0;
}

double PluginProcessor::getPatternLoopBraceStartQuarters (const int patternSlot) const
{
    return modelPattern (patternSlot).loopBrace.startQuarters;
}

double PluginProcessor::getPatternLoopBraceEndQuarters (const int patternSlot) const
{
    return modelPattern (patternSlot).loopBrace.endQuarters;
}

void PluginProcessor::saveCurrentBraceToLoopSlot (const int loopSlot)
{
    const auto slot = clampLoopSlot (loopSlot);
    const auto patternSlot = getViewPatternSlot();
    const auto& loopBrace = modelPattern (patternSlot).loopBrace;
    auto& loopSlotState = loopSlots[static_cast<size_t> (slot)];

    loopSlotState.assigned = 1;
    loopSlotState.patternSlot = patternSlot;
    loopSlotState.startQuarters = loopBrace.startQuarters;
    loopSlotState.endQuarters = loopBrace.endQuarters;

    selectLoopSlot (slot);
}

PluginProcessor::RowTriggerHit PluginProcessor::findEarliestRowTriggerInMappedRange (
    const int row,
    const double schedulePpqStart,
    const double schedulePpqEnd) const
{
    RowTriggerHit hit;

    if (row < 0 || row >= phraseRowCount)
        return hit;

    constexpr auto epsilon = 1.0e-9;
    const auto& state = audioSequencer();

    if (state.muted[static_cast<size_t> (row)] != 0)
        return hit;

    const auto pulse = pulseQuartersForIndex (pulseIndex.load (std::memory_order_relaxed));
    const auto offset = rowTimingOffsetForIndex (state.timingOffset[static_cast<size_t> (row)]) * pulse;
    const auto& rowSteps = state.rows[static_cast<size_t> (row)];
    const auto stepCount = rowSteps.stepCount;

    if (stepCount <= 0)
        return hit;

    const auto cycleLengthQuarters = rowSteps.cycleLengthQuarters;

    if (cycleLengthQuarters <= 0.0)
        return hit;

    for (int step = 0; step < stepCount; ++step)
    {
        if (rowSteps.stepSkipped[static_cast<size_t> (step)] != 0)
            continue;

        const auto stepStartInCycle = rowSteps.stepStartQuarters[static_cast<size_t> (step)];
        const auto nMin = static_cast<int> (std::ceil (
            (schedulePpqStart - stepStartInCycle - offset - epsilon) / cycleLengthQuarters));
        const auto nMax = static_cast<int> (std::floor (
            (schedulePpqEnd - stepStartInCycle - offset - epsilon) / cycleLengthQuarters));

        for (int cycle = nMin; cycle <= nMax; ++cycle)
        {
            const auto triggerPpq = static_cast<double> (cycle) * cycleLengthQuarters
                                    + stepStartInCycle + offset;

            if (triggerPpq < schedulePpqStart - epsilon || triggerPpq >= schedulePpqEnd - epsilon)
                continue;

            const auto velocity = rowSteps.velocity[static_cast<size_t> (step)];

            if (velocity <= 0 || rowSteps.stepMuted[static_cast<size_t> (step)] != 0)
                continue;

            const auto note = rowSteps.notes[static_cast<size_t> (step)];

            if (! hit.valid || triggerPpq < hit.ppq - epsilon)
            {
                hit.valid = true;
                hit.ppq = triggerPpq;
                hit.step = step;
                hit.midiNote = note;
            }
        }
    }

    return hit;
}

bool PluginProcessor::rowHasConflictingPitchBeforeMappedPpq (const int row,
                                                             const int midiNote,
                                                             const double mappedPpqExclusiveEnd) const
{
    const auto& loop = audioLoopBrace();

    if (loop.enabled == 0)
        return false;

    const auto hit =
        findEarliestRowTriggerInMappedRange (row, loop.startQuarters, mappedPpqExclusiveEnd);

    return hit.valid && hit.midiNote != midiNote;
}

bool PluginProcessor::shouldSustainGateAcrossLoopWrap (const int row,
                                                       const int midiNote,
                                                       const double mappedTriggerPpq,
                                                       const double gateQuarters,
                                                       const double mappedScheduleEnd) const
{
    constexpr auto epsilon = 1.0e-9;
    const auto& loop = audioLoopBrace();

    if (loop.enabled == 0)
        return false;

    if (mappedScheduleEnd < loop.endQuarters - epsilon)
        return false;

    const auto mappedGateEnd = mappedTriggerPpq + gateQuarters;

    if (mappedGateEnd <= loop.endQuarters + epsilon)
        return false;

    return ! rowHasConflictingPitchBeforeMappedPpq (row, midiNote, mappedGateEnd);
}

bool PluginProcessor::shouldPreservePendingNoteAcrossLoopWrap (const int row,
                                                               const int midiNote) const
{
    const auto& loop = audioLoopBrace();

    if (loop.enabled == 0)
        return false;

    const auto hit =
        findEarliestRowTriggerInMappedRange (row, loop.startQuarters, loop.endQuarters);

    if (! hit.valid)
        return true;

    return hit.midiNote == midiNote;
}

double PluginProcessor::loopDownbeatTransportForSlot (const int loopSlot,
                                                    const double transportPpq) const
{
    const auto slot = clampLoopSlot (loopSlot);
    const auto& loopSlotState = loopSlots[static_cast<size_t> (slot)];

    if (loopSlotState.assigned == 0)
        return transportPpq;

    const auto loopStart = clampLoopBraceStart (loopSlotState.startQuarters,
                                                loopSlotState.endQuarters);
    const auto loopEnd = clampLoopBraceEnd (loopSlotState.endQuarters, loopStart);
    const auto loopLength = loopEnd - loopStart;

    if (loopLength <= 0.0)
        return transportPpq;

    constexpr auto epsilon = 1.0e-9;

    if (transportPpq + epsilon < loopStart)
        return loopStart;

    const auto cycles =
        std::floor ((transportPpq - loopStart + epsilon) / loopLength);
    return loopStart + cycles * loopLength;
}

bool PluginProcessor::isAudioLoopSlotApplied (const int loopSlot) const
{
    if (audioActiveLoopSlot.load (std::memory_order_acquire) != loopSlot)
        return false;

    const auto slot = clampLoopSlot (loopSlot);
    const auto& loopSlotState = loopSlots[static_cast<size_t> (slot)];

    if (loopSlotState.assigned == 0)
        return false;

    const auto patternSlot = clampPatternSlot (loopSlotState.patternSlot);

    if (clampPatternSlot (audioActivePatternSlot) != patternSlot)
        return false;

    const auto loopStart = clampLoopBraceStart (loopSlotState.startQuarters,
                                                loopSlotState.endQuarters);
    const auto loopEnd = clampLoopBraceEnd (loopSlotState.endQuarters, loopStart);
    const auto& loopBrace = audioPatterns[static_cast<size_t> (patternSlot)].loopBrace;

    constexpr auto epsilon = 1.0e-9;

    return loopBrace.enabled != 0
           && std::abs (loopBrace.startQuarters - loopStart) <= epsilon
           && std::abs (loopBrace.endQuarters - loopEnd) <= epsilon;
}

void PluginProcessor::requestAudioLoopSlot (const int loopSlot)
{
    const auto slot = clampLoopSlot (loopSlot);

    if (loopSlots[static_cast<size_t> (slot)].assigned == 0)
        return;

    clearPendingAudioLoopBraceEnable();
    pendingAudioLoopSlot.store (slot, std::memory_order_release);
}

void PluginProcessor::applyAudioLoopSlot (const int loopSlot, const double reanchorTransportPpq)
{
    const auto slot = clampLoopSlot (loopSlot);
    const auto& loopSlotState = loopSlots[static_cast<size_t> (slot)];

    if (loopSlotState.assigned == 0)
        return;

    clearPendingAudioLoopBraceEnable();
    audioActivePatternSlot = clampPatternSlot (loopSlotState.patternSlot);

    auto& loopBrace = audioPatterns[static_cast<size_t> (audioActivePatternSlot)].loopBrace;
    loopBrace.enabled = 1;
    loopBrace.startQuarters = clampLoopBraceStart (loopSlotState.startQuarters,
                                                   loopSlotState.endQuarters);
    loopBrace.endQuarters = clampLoopBraceEnd (loopSlotState.endQuarters,
                                               loopBrace.startQuarters);

    pendingAudioLoopSlot.store (-1, std::memory_order_release);
    currentLoopSlot.store (slot, std::memory_order_release);
    audioActiveLoopSlot.store (slot, std::memory_order_release);
    reanchorLoopScheduleAt (reanchorTransportPpq);
    resetLastEmittedTriggers();
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetPendingCombinedNoteOffs();
    resetStepCycleCounters();
}

void PluginProcessor::requestAudioLoopBraceEnable (const int patternSlot)
{
    pendingAudioLoopBraceEnablePatternSlot = clampPatternSlot (patternSlot);
}

void PluginProcessor::clearPendingAudioLoopBraceEnable()
{
    pendingAudioLoopBraceEnablePatternSlot = -1;
}

void PluginProcessor::applyAudioLoopBraceEnable (const double reanchorTransportPpq)
{
    const auto slot = pendingAudioLoopBraceEnablePatternSlot;
    clearPendingAudioLoopBraceEnable();

    if (slot < 0 || slot >= patternSlotCount)
        return;

    auto& loopBrace = audioPatterns[static_cast<size_t> (slot)].loopBrace;
    loopBrace.enabled = 1;

    if (slot != audioActivePatternSlot)
    {
        clearLoopScheduleAnchor();
        return;
    }

    reanchorLoopScheduleAt (reanchorTransportPpq);
    resetLastEmittedTriggers();
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetPendingCombinedNoteOffs();
    resetStepCycleCounters();
}

void PluginProcessor::resolvePendingAudioLoopBraceEnableForStoppedPlayback()
{
    const auto slot = pendingAudioLoopBraceEnablePatternSlot;
    clearPendingAudioLoopBraceEnable();

    if (slot < 0 || slot >= patternSlotCount)
        return;

    audioPatterns[static_cast<size_t> (slot)].loopBrace.enabled = 1;
    clearLoopScheduleAnchor();
}

void PluginProcessor::selectLoopSlot (const int loopSlot)
{
    const auto slot = clampLoopSlot (loopSlot);
    const auto& loopSlotState = loopSlots[static_cast<size_t> (slot)];

    if (loopSlotState.assigned == 0)
        return;

    patternOutputArmed.store (1, std::memory_order_release);

    const auto patternSlot = clampPatternSlot (loopSlotState.patternSlot);
    lastViewPatternSlot = patternSlot;
    const auto loopStart = clampLoopBraceStart (loopSlotState.startQuarters,
                                                loopSlotState.endQuarters);
    const auto loopEnd = clampLoopBraceEnd (loopSlotState.endQuarters, loopStart);

    currentModelPatternSlot.store (patternSlot, std::memory_order_release);

    auto& loopBrace = modelPatterns[static_cast<size_t> (patternSlot)].loopBrace;
    loopBrace.enabled = 1;
    loopBrace.startQuarters = loopStart;
    loopBrace.endQuarters = loopEnd;

    const auto audioAlreadyApplied = isAudioLoopSlotApplied (slot);

    currentLoopSlot.store (slot, std::memory_order_release);

    if (! audioAlreadyApplied)
    {
        publishPatternToAudio (patternSlot);
        requestAudioLoopSlot (slot);
    }
    else
        pendingAudioLoopSlot.store (-1, std::memory_order_release);

    if (patternSlotParameter != nullptr && patternSlotParameter->get() != patternSlot + 1)
        patternSlotParameter->setValueNotifyingHost (
            patternSlotParameter->convertTo0to1 (patternSlot + 1));

    lastObservedParameterPatternSlot = patternSlot;
}

int PluginProcessor::getCurrentLoopSlot() const
{
    return currentLoopSlot.load (std::memory_order_acquire);
}

bool PluginProcessor::isLoopSlotAssigned (const int loopSlot) const
{
    return loopSlots[static_cast<size_t> (clampLoopSlot (loopSlot))].assigned != 0;
}

int PluginProcessor::getLoopSlotPatternSlot (const int loopSlot) const
{
    return loopSlots[static_cast<size_t> (clampLoopSlot (loopSlot))].patternSlot;
}

double PluginProcessor::getPlaybackBeat() const
{
    return playbackBeatForUi();
}

double PluginProcessor::getLoopPlaybackBeat() const
{
    return getPlaybackBeat();
}

void PluginProcessor::addPendingNoteOn (const PendingNoteOn& note)
{
    if (pendingNoteOnCount >= pendingNoteOns.size())
        return;

    pendingNoteOns[pendingNoteOnCount++] = note;
}

void PluginProcessor::emitLayeredGeneratedNote (const int midiChannel,
                                                const int note,
                                                const int velocity,
                                                const int sampleOffset,
                                                const int gateSamples,
                                                const int bufferSamples,
                                                juce::MidiBuffer& midiMessages)
{
    if (sampleOffset < 0 || sampleOffset >= bufferSamples)
        return;

    const auto clampedSampleOffset = juce::jmax (0, sampleOffset);
    emitGeneratedNoteOn (midiChannel, note, velocity, clampedSampleOffset, midiMessages);

    const auto samplesUntilOff = clampedSampleOffset + gateSamples;

    if (samplesUntilOff < bufferSamples)
    {
        emitGeneratedNoteOff (midiChannel, note, samplesUntilOff, midiMessages);
    }
    else if (pendingCombinedNoteOffCount < pendingCombinedNoteOffs.size())
    {
        pendingCombinedNoteOffs[pendingCombinedNoteOffCount++] =
            PendingCombinedNoteOff { midiChannel, note, samplesUntilOff - bufferSamples };
    }
    else
    {
        emitGeneratedNoteOff (midiChannel, note, bufferSamples - 1, midiMessages);
    }
}

void PluginProcessor::emitScheduledNoteOn (const int row,
                                           const int midiChannel,
                                           const int note,
                                           const int velocity,
                                           const int step,
                                           const int sampleOffset,
                                           const int gateSamples,
                                           const int bufferSamples,
                                           juce::MidiBuffer& midiMessages)
{
    if (row < 0 || row >= phraseRowCount || sampleOffset < 0 || sampleOffset >= bufferSamples)
        return;

    auto& pending = pendingNoteOffs[static_cast<size_t> (row)];

    if (pending.note >= 0)
    {
        emitGeneratedNoteOff (
            pending.channel,
            pending.note,
            juce::jlimit (0, bufferSamples - 1, juce::jmin (pending.samplesRemaining, sampleOffset)),
            midiMessages);
        pending.note = -1;
        pending.activeStep = -1;
        pending.samplesRemaining = 0;
    }

    emitGeneratedNoteOn (midiChannel, note, velocity, sampleOffset, midiMessages);

    const auto samplesUntilOff = sampleOffset + gateSamples;

    if (samplesUntilOff < bufferSamples)
    {
        emitGeneratedNoteOff (midiChannel, note, samplesUntilOff, midiMessages);
    }
    else
    {
        pending.channel = midiChannel;
        pending.note = note;
        pending.activeStep = step;
        pending.samplesRemaining = samplesUntilOff - bufferSamples;
    }
}

void PluginProcessor::processPhraseRowNoteAuditions (const int bufferSamples,
                                                     juce::MidiBuffer& midiMessages)
{
    const auto& activePattern =
        audioPatterns[static_cast<size_t> (clampPatternSlot (audioActivePatternSlot))];

    for (int row = 0; row < phraseRowCount; ++row)
    {
        auto& audition = phraseRowNoteAuditions[static_cast<size_t> (row)];

        if (audition.pending.exchange (0, std::memory_order_acq_rel) == 0)
            continue;

        auto& pending = pendingNoteOffs[static_cast<size_t> (row)];

        if (pending.note < 0 || pending.activeStep != audition.step)
            continue;

        const auto& audioRow = activePattern.sequencer.rows[static_cast<size_t> (row)];
        const auto stepIndex =
            static_cast<size_t> (juce::jlimit (0, maxPhraseStepsPerRow - 1, audition.step));

        if (stepIndex >= static_cast<size_t> (audioRow.stepCount))
            continue;

        const auto newNote = audition.note;
        const auto bandpassLow = activePattern.noteBandpassLowMidi;
        const auto bandpassHigh = activePattern.noteBandpassHighMidi;
        auto velocity = audioRow.velocity[stepIndex];

        const auto remainingGateSamples =
            pending.samplesRemaining < bufferSamples
                ? pending.samplesRemaining
                : pending.samplesRemaining + bufferSamples;

        if (newNote < bandpassLow || newNote > bandpassHigh || velocity <= 0
            || audioRow.stepMuted[stepIndex] != 0 || remainingGateSamples <= 0)
        {
            emitGeneratedNoteOff (pending.channel, pending.note, 0, midiMessages);
            pending.note = -1;
            pending.activeStep = -1;
            pending.samplesRemaining = 0;
            continue;
        }

        velocity = humanizeVelocityValue (
            velocity,
            velocityHumanizePercent.load (std::memory_order_relaxed),
            playbackRandomState);

        const auto outputNote = juce::jlimit (minMidiNote,
                                               maxMidiNote,
                                               newNote + activePattern.globalTransposeSemitones);

        emitScheduledNoteOn (row,
                             pending.channel,
                             outputNote,
                             velocity,
                             audition.step,
                             0,
                             remainingGateSamples,
                             bufferSamples,
                             midiMessages);
    }
}

void PluginProcessor::flushPendingNoteOns (const int bufferSamples, juce::MidiBuffer& midiMessages)
{
    if (pendingNoteOnCount == 0)
        return;

    size_t writeIndex = 0;

    for (size_t i = 0; i < pendingNoteOnCount; ++i)
    {
        auto pending = pendingNoteOns[i];

        if (pending.samplesRemaining < bufferSamples)
        {
            if (pending.layered != 0)
            {
                emitLayeredGeneratedNote (pending.channel,
                                          pending.note,
                                          pending.velocity,
                                          pending.samplesRemaining,
                                          pending.gateSamples,
                                          bufferSamples,
                                          midiMessages);
            }
            else
            {
                emitScheduledNoteOn (pending.row,
                                     pending.channel,
                                     pending.note,
                                     pending.velocity,
                                     pending.step,
                                     pending.samplesRemaining,
                                     pending.gateSamples,
                                     bufferSamples,
                                     midiMessages);
            }
        }
        else
        {
            pending.samplesRemaining -= bufferSamples;
            pendingNoteOns[writeIndex++] = pending;
        }
    }

    pendingNoteOnCount = writeIndex;
}

bool PluginProcessor::hasStandaloneTransport() const
{
    return wrapperType == wrapperType_Standalone;
}

void PluginProcessor::setStandaloneTransportPlaying (const bool shouldPlay)
{
    if (! hasStandaloneTransport())
        return;

    const auto nextPlaying = shouldPlay ? 1 : 0;
    const auto wasStandalonePlaying = standaloneTransportPlaying.exchange (
        nextPlaying,
        std::memory_order_relaxed);

    if (nextPlaying != 0 && wasStandalonePlaying == 0)
    {
        standaloneTransportPpqPosition.store (0.0, std::memory_order_relaxed);
        standaloneTransportResetRequested.store (1, std::memory_order_relaxed);
    }
}

bool PluginProcessor::isStandaloneTransportPlaying() const
{
    return standaloneTransportPlaying.load (std::memory_order_relaxed) != 0;
}

void PluginProcessor::setStandaloneTempoBpm (const double bpm)
{
    standaloneTempoBpm.store (clampStandaloneTempoBpm (bpm), std::memory_order_relaxed);
}

double PluginProcessor::getStandaloneTempoBpm() const
{
    return standaloneTempoBpm.load (std::memory_order_relaxed);
}

void PluginProcessor::setPhraseRowRecording (const int row)
{
    for (auto& keyHeld : recordingKeysHeld)
        keyHeld.store (0, std::memory_order_release);

    if (row >= 0 && row < phraseRowCount)
    {
        recordingRow.store (row, std::memory_order_release);
        recordingAwaitingFirstNote.store (1, std::memory_order_release);
    }
    else
    {
        recordingRow.store (-1, std::memory_order_release);
        recordingAwaitingFirstNote.store (0, std::memory_order_release);
    }

    recordQueueWrite.store (0, std::memory_order_release);
    recordQueueRead.store (0, std::memory_order_release);
}

int PluginProcessor::getPhraseRowRecording() const
{
    return recordingRow.load (std::memory_order_acquire);
}

void PluginProcessor::enqueueRecordedNote (const int midiNote)
{
    const auto clamped = juce::jlimit (0, 127, midiNote);
    const auto& pattern = modelPattern (getViewPatternSlot());

    if (clampScaleModeIndex (pattern.scaleModeIndex) != defaultScaleModeIndex
        && ! pitchClassInScale (clamped, pattern.scaleRoot, pattern.scaleModeIndex))
        return;

    const auto write = recordQueueWrite.load (std::memory_order_relaxed);
    const auto read = recordQueueRead.load (std::memory_order_acquire);

    if (write - read >= recordQueueCapacity)
        return;

    recordQueueNotes[static_cast<size_t> (write % recordQueueCapacity)] = clamped;
    recordQueueWrite.store (write + 1, std::memory_order_release);
}

bool PluginProcessor::tryDequeueRecordedNote (int& midiNoteOut)
{
    const auto read = recordQueueRead.load (std::memory_order_relaxed);
    const auto write = recordQueueWrite.load (std::memory_order_acquire);

    if (read >= write)
        return false;

    midiNoteOut = recordQueueNotes[static_cast<size_t> (read % recordQueueCapacity)];
    recordQueueRead.store (read + 1, std::memory_order_release);
    return true;
}

void PluginProcessor::appendRecordedNoteToModelRow (const int row, const int midiNote)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    auto& steps = modelRow (row);

    recordingAwaitingFirstNote.store (0, std::memory_order_release);

    if (steps.stepCount < maxPhraseStepsPerRow)
    {
        const auto index = static_cast<size_t> (steps.stepCount);
        steps.notes[index] = juce::jlimit (0, 127, midiNote);
        steps.timingMultiplier[index] = defaultStepTimingMultiplierIndex;
        steps.durationFraction[index] = defaultStepDurationFraction;
        steps.velocity[index] = defaultStepVelocity;
        steps.stepMuted[index] = 0;
        steps.stepSkipped[index] = 0;
        steps.probability[index] = defaultStepProbability;
        steps.cycle[index] = defaultStepCycle;
        steps.cycleOffset[index] = defaultStepCycleMask;
        ++steps.stepCount;
    }
    else
    {
        return;
    }

    rebuildRowTimingLayout (steps);
    resetStepCycleCountersForRow (row);
    publishRowToAudio (row);
}

juce::Array<int> PluginProcessor::drainPhraseRowRecordedNotes()
{
    juce::Array<int> drained;

    const auto row = recordingRow.load (std::memory_order_acquire);

    if (row < 0)
        return drained;

    int midiNote = 0;

    while (tryDequeueRecordedNote (midiNote))
    {
        drained.add (midiNote);
        appendRecordedNoteToModelRow (row, midiNote);
    }

    return drained;
}

juce::Array<int> PluginProcessor::getPhraseRowRecordingKeysHeld() const
{
    juce::Array<int> held;

    for (int note = 0; note < 128; ++note)
    {
        if (recordingKeysHeld[static_cast<size_t> (note)].load (std::memory_order_acquire) != 0)
            held.add (note);
    }

    return held;
}

void PluginProcessor::injectPhraseRowRecordedNote (const int midiNote)
{
    const auto row = recordingRow.load (std::memory_order_acquire);

    if (row < 0)
        return;

    const auto clamped = juce::jlimit (0, 127, midiNote);
    const auto& pattern = modelPattern (getViewPatternSlot());

    if (clampScaleModeIndex (pattern.scaleModeIndex) != defaultScaleModeIndex
        && ! pitchClassInScale (clamped, pattern.scaleRoot, pattern.scaleModeIndex))
        return;

    appendRecordedNoteToModelRow (row, clamped);
}

void PluginProcessor::handleIncomingControlNotes (juce::MidiBuffer& midiMessages)
{
    const auto armedRow = recordingRow.load (std::memory_order_acquire);

    std::unordered_map<int, int> noteOnCountBySample;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
            ++noteOnCountBySample[metadata.samplePosition];
    }

    juce::MidiBuffer filtered;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (armedRow >= 0 && message.isNoteOnOrOff())
        {
            const auto heldNote = message.getNoteNumber();

            if (heldNote >= 0 && heldNote < 128)
            {
                recordingKeysHeld[static_cast<size_t> (heldNote)].store (message.isNoteOn() ? 1 : 0,
                                                                          std::memory_order_release);
            }
        }

        if (message.isNoteOnOrOff())
        {
            if (armedRow >= 0)
            {
                if (! message.isNoteOn())
                {
                    filtered.addEvent (message, metadata.samplePosition);
                    continue;
                }

                const auto countAtSample =
                    noteOnCountBySample[metadata.samplePosition];

                if (countAtSample == 1)
                    enqueueRecordedNote (message.getNoteNumber());

                filtered.addEvent (message, metadata.samplePosition);
                continue;
            }

            const auto note = message.getNoteNumber();

            if (note >= 0 && note < patternSlotCount)
            {
                if (message.isNoteOn())
                {
                    patternOutputArmed.store (1, std::memory_order_release);
                    lastViewPatternSlot = note;
                    currentModelPatternSlot.store (note, std::memory_order_release);
                    deactivateLoopBraceForPatternSelection (note);
                    requestAudioPatternSlot (note);
                }
                continue;
            }

            if (note >= patternSlotCount && note < patternSlotCount + loopSlotCount)
            {
                if (message.isNoteOn())
                {
                    const auto loopSlot = note - patternSlotCount;

                    if (isLoopSlotAssigned (loopSlot))
                        selectLoopSlot (loopSlot);
                }
                continue;
            }

            if (note == midiMuteTriggerNote)
            {
                if (message.isNoteOn())
                    deactivatePatternOutput();

                continue;
            }
        }

        filtered.addEvent (message, metadata.samplePosition);
    }

    midiMessages.swapWith (filtered);
}

bool PluginProcessor::shouldApplyPendingPatternSwitch (const double ppqStart,
                                                       const double ppqEnd) const
{
    if (pendingAudioPatternSlot.load (std::memory_order_acquire) < 0)
        return false;

    const auto pulse = pulseQuartersForIndex (pulseIndex.load (std::memory_order_relaxed));

    if (pulse <= 0.0)
        return true;

    constexpr auto epsilon = 1.0e-9;
    const auto nextPulse = (std::floor ((ppqStart + epsilon) / pulse) + 1.0) * pulse;

    return nextPulse <= ppqEnd + epsilon;
}

void PluginProcessor::processTransportPlaybackRange (const double transportPpqStart,
                                                     const double transportPpqEnd,
                                                     const double bufferTransportStartPpq,
                                                     const int bufferSamples,
                                                     const double ppqPerSample,
                                                     juce::MidiBuffer& midiMessages,
                                                     const bool resetRowTriggersAtSegmentStart)
{
    const auto& loop = audioLoopBrace();
    const auto loopEnabled = loop.enabled != 0;
    const auto loopStart = loop.startQuarters;
    const auto loopEnd = loop.endQuarters;
    const auto loopLength = loopEnd - loopStart;

    if (loopEnabled && loopLength > 0.0)
    {
        constexpr auto epsilon = 1.0e-9;
        auto transportCursor = transportPpqStart;
        auto isFirstSegment = true;

        while (transportCursor < transportPpqEnd - epsilon)
        {
            const auto mappedStart = mapTransportToLoopSchedulePpq (transportCursor);
            const auto remainingInLoop = loopEnd - mappedStart;
            const auto segmentTransportEnd =
                juce::jmin (transportPpqEnd, transportCursor + remainingInLoop);
            const auto mappedEnd = mappedStart + (segmentTransportEnd - transportCursor);
            const auto wrappedToLoopStart = mappedStart <= loopStart + epsilon
                                            && transportCursor > transportPpqStart + epsilon;

            const auto resetForLoopBoundaryAtRangeStart =
                isFirstSegment && transportPpqStart > epsilon
                && mappedStart <= loopStart + epsilon;

            processScheduledRange (mappedStart,
                                   mappedEnd,
                                   transportCursor,
                                   bufferTransportStartPpq,
                                   bufferSamples,
                                   ppqPerSample,
                                   midiMessages,
                                   resetRowTriggersAtSegmentStart || resetForLoopBoundaryAtRangeStart
                                       || (wrappedToLoopStart && ! isFirstSegment));

            transportCursor = segmentTransportEnd;
            isFirstSegment = false;
        }
    }
    else
    {
        processScheduledRange (mapTransportToPatternSchedulePpq (transportPpqStart),
                               mapTransportToPatternSchedulePpq (transportPpqEnd),
                               transportPpqStart,
                               bufferTransportStartPpq,
                               bufferSamples,
                               ppqPerSample,
                               midiMessages,
                               resetRowTriggersAtSegmentStart);
    }
}

void PluginProcessor::processCombinedScheduledRange (const double schedulePpqStart,
                                                     const double schedulePpqEnd,
                                                     const double segmentTransportStartPpq,
                                                     const double bufferTransportStartPpq,
                                                     const int bufferSamples,
                                                     const double ppqPerSample,
                                                     juce::MidiBuffer& midiMessages,
                                                     const bool resetRowTriggersAtSegmentStart)
{
    constexpr auto epsilon = 1.0e-9;
    const auto& state = audioSequencer();
    const auto& activePattern =
        audioPatterns[static_cast<size_t> (clampPatternSlot (audioActivePatternSlot))];
    const auto scaleRoot = activePattern.scaleRoot;
    const auto scaleModeIndex = activePattern.scaleModeIndex;
    const auto emptyRowDefaultNote = defaultStepNoteForScaleRoot (scaleRoot);
    const auto modeMask = clampCombinationModeMask (state.combinationModeMask);
    const auto pulse = pulseQuartersForIndex (pulseIndex.load (std::memory_order_relaxed));
    const auto combinationGesturePulse =
        juce::jmax (pulse, combinationGesturePulseQuartersFloor);
    const auto swing = swingPercent.load (std::memory_order_relaxed);
    const auto velocityHumanize = velocityHumanizePercent.load (std::memory_order_relaxed);
    const auto timingHumanize = timingHumanizePercent.load (std::memory_order_relaxed);
    const auto swingSubdivision = swingSubdivisionIndex.load (std::memory_order_relaxed);

    std::array<int, phraseRowCount> activeRows {};
    auto activeRowCount = 0;

    for (int row = 0; row < phraseRowCount; ++row)
    {
        if (state.muted[static_cast<size_t> (row)] == 0
            && state.rows[static_cast<size_t> (row)].stepCount > 0
            && state.rows[static_cast<size_t> (row)].cycleLengthQuarters > 0.0)
        {
            activeRows[static_cast<size_t> (activeRowCount++)] = row;
        }
    }

    if (activeRowCount == 0)
        return;

    const auto compareCombinedEventsForWeave = [] (const CombinedNoteEvent& a,
                                                   const CombinedNoteEvent& b) {
        if (std::abs (a.ppq - b.ppq) > epsilon)
            return a.ppq < b.ppq;

        if (a.note != b.note)
            return a.note < b.note;

        if (a.row != b.row)
            return a.row < b.row;

        return a.step < b.step;
    };

    auto eventCount = static_cast<size_t> (0);
    const auto appendEvent = [&] (const CombinedNoteEvent& event) {
        if (eventCount >= combinedEvents.size())
            return false;

        combinedEvents[eventCount++] = event;
        return true;
    };

    const auto hocketModeEnabled =
        combinationModeEnabled (modeMask, combinationModeHocket) && activeRowCount > 1;
    const auto weaveModeEnabled =
        combinationModeEnabled (modeMask, combinationModeWeave) && activeRowCount > 1;
    const auto heldNoteDeOverlapLookbackEnabled = modeMask != 0;

    const auto patternRepeatLengthQuarters = [&] {
        auto repeatUnits = 0;

        for (int row = 0; row < phraseRowCount; ++row)
        {
            if (state.muted[static_cast<size_t> (row)] != 0)
                continue;

            const auto& rowSteps = state.rows[static_cast<size_t> (row)];

            if (rowSteps.cycleLengthQuarters <= 1.0e-9)
                continue;

            auto units = static_cast<int> (std::lround (
                rowSteps.cycleLengthQuarters / loopBraceSnapQuarters));
            units = juce::jmax (1, units);
            repeatUnits = repeatUnits == 0 ? units : lcmInt (repeatUnits, units);
        }

        return repeatUnits > 0
                   ? static_cast<double> (repeatUnits) * loopBraceSnapQuarters
                   : 0.0;
    };

    const auto emitPpqStart = schedulePpqStart;
    const auto emitPpqEnd = schedulePpqEnd;
    auto collectionPpqStart = emitPpqStart;
    auto collectionPpqEnd = emitPpqEnd;
    auto hocketLengthQuarters = emitPpqEnd;
    auto maxActiveRowCycleQuarters = 0.0;

    for (int rowIndex = 0; rowIndex < activeRowCount; ++rowIndex)
    {
        const auto row = activeRows[static_cast<size_t> (rowIndex)];
        maxActiveRowCycleQuarters = juce::jmax (
            maxActiveRowCycleQuarters,
            state.rows[static_cast<size_t> (row)].cycleLengthQuarters);
    }

    const auto patternRepeat = patternRepeatLengthQuarters();
    const auto weaveHashPpq = [&] (const double ppq) {
        const auto& loop = audioLoopBrace();
        const auto loopLength = loop.endQuarters - loop.startQuarters;

        if (loop.enabled != 0 && loopLength > epsilon)
            return loop.startQuarters + positiveMod (ppq - loop.startQuarters, loopLength);

        if (patternRepeat > epsilon)
            return positiveMod (ppq, patternRepeat);

        return ppq;
    };

    if (hocketModeEnabled)
    {
        const auto& loop = audioLoopBrace();
        collectionPpqStart = loop.enabled != 0 ? loop.startQuarters : 0.0;

        if (loop.enabled != 0)
        {
            collectionPpqEnd = loop.endQuarters;
            hocketLengthQuarters = loop.endQuarters;
        }
        else if (patternRepeat > epsilon)
        {
            collectionPpqEnd = juce::jmax (patternRepeat, emitPpqEnd + pulse);
            hocketLengthQuarters = collectionPpqEnd;
        }
        else
        {
            collectionPpqEnd = emitPpqEnd + pulse;
            hocketLengthQuarters = collectionPpqEnd;
        }
    }
    else if (weaveModeEnabled)
    {
        const auto& loop = audioLoopBrace();
        const auto lookbackQuarters =
            juce::jmax (combinationGesturePulse * 2.0, maxActiveRowCycleQuarters);
        const auto lowerBound = loop.enabled != 0 ? loop.startQuarters : 0.0;
        collectionPpqStart = juce::jmax (lowerBound, emitPpqStart - lookbackQuarters);
        collectionPpqEnd = emitPpqEnd;
    }
    else if (heldNoteDeOverlapLookbackEnabled)
    {
        const auto& loop = audioLoopBrace();
        const auto lookbackQuarters =
            juce::jmax (combinationGesturePulse * 2.0, maxActiveRowCycleQuarters);
        const auto lowerBound = loop.enabled != 0 ? loop.startQuarters : 0.0;
        collectionPpqStart = juce::jmax (lowerBound, emitPpqStart - lookbackQuarters);
        collectionPpqEnd = emitPpqEnd;
    }

    for (int row = 0; row < phraseRowCount; ++row)
    {
        if (state.muted[static_cast<size_t> (row)] != 0)
            continue;

        const auto& rowSteps = state.rows[static_cast<size_t> (row)];
        const auto stepCount = rowSteps.stepCount;
        const auto cycleLengthQuarters = rowSteps.cycleLengthQuarters;

        if (stepCount <= 0 || cycleLengthQuarters <= 0.0)
            continue;

        const auto offset =
            rowTimingOffsetForIndex (state.timingOffset[static_cast<size_t> (row)]) * pulse;
        auto& scratch = processScratch[static_cast<size_t> (row)];
        auto triggerCount = 0;
        std::array<int, maxPhraseStepsPerRow> stepTriggerCounts {};

        if (hocketModeEnabled || weaveModeEnabled || heldNoteDeOverlapLookbackEnabled)
        {
            for (int step = 0; step < stepCount; ++step)
            {
                if (rowSteps.stepSkipped[static_cast<size_t> (step)] != 0)
                    continue;

                const auto stepStartInCycle = rowSteps.stepStartQuarters[static_cast<size_t> (step)];
                const auto firstGlobalTrigger = static_cast<int> (std::ceil (
                    (0.0 - stepStartInCycle - offset - epsilon) / cycleLengthQuarters));
                const auto previousTrigger = static_cast<int> (std::floor (
                    (collectionPpqStart - stepStartInCycle - offset - epsilon) / cycleLengthQuarters));
                stepTriggerCounts[static_cast<size_t> (step)] =
                    juce::jmax (0, previousTrigger - firstGlobalTrigger + 1);
            }
        }

        for (int step = 0; step < stepCount; ++step)
        {
            if (rowSteps.stepSkipped[static_cast<size_t> (step)] != 0)
                continue;

            const auto stepStartInCycle = rowSteps.stepStartQuarters[static_cast<size_t> (step)];
            const auto nMin = static_cast<int> (std::ceil (
                (collectionPpqStart - stepStartInCycle - offset - epsilon) / cycleLengthQuarters));
            const auto nMax = static_cast<int> (std::floor (
                (collectionPpqEnd - stepStartInCycle - offset - epsilon) / cycleLengthQuarters));

            for (int cycle = nMin; cycle <= nMax; ++cycle)
            {
                const auto triggerPpq = static_cast<double> (cycle) * cycleLengthQuarters
                                        + stepStartInCycle + offset;

                if (triggerPpq < collectionPpqStart - epsilon || triggerPpq >= collectionPpqEnd - epsilon)
                    continue;

                if (triggerCount >= static_cast<int> (scratch.triggers.size()))
                    break;

                scratch.triggers[static_cast<size_t> (triggerCount)] = { triggerPpq, step };
                ++triggerCount;
            }
        }

        if (triggerCount == 0)
            continue;

        std::sort (scratch.triggers.begin(),
                   scratch.triggers.begin() + triggerCount,
                   [] (const ProcessScratch::StepTrigger& a, const ProcessScratch::StepTrigger& b) {
                       return a.ppq < b.ppq;
                   });

        auto& lastTrigger = lastEmittedTriggerPpq[static_cast<size_t> (row)];

        if (hocketModeEnabled || weaveModeEnabled || heldNoteDeOverlapLookbackEnabled)
            lastTrigger = collectionPpqStart - cycleLengthQuarters - 1.0;
        else if (resetRowTriggersAtSegmentStart)
            lastTrigger = schedulePpqStart - cycleLengthQuarters - 1.0;
        else if (schedulePpqStart + epsilon < lastTrigger)
            lastTrigger = schedulePpqStart - cycleLengthQuarters - 1.0;

        struct ActiveCombinedNote
        {
            double startPpq = 0.0;
            double endPpq = 0.0;
            int step = 0;
            int channel = 0;
            int note = 0;
            int velocity = 0;
            bool valid = false;
        };

        ActiveCombinedNote activeNote;
        auto collectionFull = false;

        const auto flushActiveNote = [&] (const double endTime) {
            if (! activeNote.valid || collectionFull)
                return;

            if (hocketModeEnabled && activeNote.startPpq >= collectionPpqEnd - epsilon)
            {
                activeNote.valid = false;
                return;
            }

            auto clippedEnd = juce::jmin (activeNote.endPpq, endTime);

            if (hocketModeEnabled)
                clippedEnd = juce::jmin (clippedEnd, collectionPpqEnd);

            if (clippedEnd <= activeNote.startPpq + epsilon)
            {
                activeNote.valid = false;
                return;
            }

            if (! appendEvent (CombinedNoteEvent { activeNote.startPpq,
                                                   clippedEnd - activeNote.startPpq,
                                                   row,
                                                   activeNote.step,
                                                   activeNote.channel,
                                                   activeNote.note,
                                                   activeNote.velocity }))
            {
                collectionFull = true;
                activeNote.valid = false;
                return;
            }

            activeNote.valid = false;
        };

        for (int triggerIndex = 0; triggerIndex < triggerCount; ++triggerIndex)
        {
            if (collectionFull)
                break;

            const auto trigger = scratch.triggers[static_cast<size_t> (triggerIndex)];

            if (trigger.ppq <= lastTrigger + epsilon)
                continue;

            lastTrigger = trigger.ppq;

            const auto step = trigger.step;
            int stepTriggerCount = 0;

            if (hocketModeEnabled || weaveModeEnabled || heldNoteDeOverlapLookbackEnabled)
            {
                stepTriggerCount = stepTriggerCounts[static_cast<size_t> (step)];
                stepTriggerCounts[static_cast<size_t> (step)] = stepTriggerCount + 1;
            }
            else
            {
                const auto stepStartInCycle = rowSteps.stepStartQuarters[static_cast<size_t> (step)];
                const auto firstGlobalTrigger = static_cast<int> (std::ceil (
                    (0.0 - stepStartInCycle - offset - epsilon) / cycleLengthQuarters));
                const auto cycleIndex = static_cast<int> (std::floor (
                    (trigger.ppq - stepStartInCycle - offset - epsilon) / cycleLengthQuarters));
                stepTriggerCount = cycleIndex - firstGlobalTrigger;
            }

            const auto cycle = clampStepCycle (rowSteps.cycle[static_cast<size_t> (step)]);
            const auto cycleMask =
                clampStepCycleMask (rowSteps.cycleOffset[static_cast<size_t> (step)], cycle);

            if (! cycleGateMatches (stepTriggerCount, cycle, cycleMask))
                continue;

            auto velocity = rowSteps.velocity[static_cast<size_t> (step)];

            if (velocity <= 0 || rowSteps.stepMuted[static_cast<size_t> (step)] != 0)
                continue;

            const auto probability =
                clampStepProbability (rowSteps.probability[static_cast<size_t> (step)]);

            if (probability <= 0)
                continue;

            if (hocketModeEnabled || weaveModeEnabled || heldNoteDeOverlapLookbackEnabled)
            {
                if (! probabilityPassesDeterministic (step, stepTriggerCount, probability))
                    continue;
            }
            else if (probability < 100
                     && nextRandomUnit (playbackRandomState) * 100.0f
                            >= static_cast<float> (probability))
            {
                continue;
            }

            const auto gateQuarters =
                rowSteps.stepLengthQuarters[static_cast<size_t> (step)]
                * rowSteps.durationFraction[static_cast<size_t> (step)];

            if (gateQuarters <= epsilon)
                continue;

            if (! hocketModeEnabled && ! weaveModeEnabled)
                velocity = humanizeVelocityValue (velocity, velocityHumanize, playbackRandomState);

            const auto swingDelay =
                swingDelayQuartersForPpq (trigger.ppq, pulse, swing, swingSubdivision);
            const auto noteStart = trigger.ppq + swingDelay;

            flushActiveNote (noteStart);

            if (collectionFull)
                break;

            activeNote.startPpq = noteStart;
            activeNote.endPpq = noteStart + gateQuarters;
            activeNote.step = step;
            activeNote.channel = state.midiChannel[static_cast<size_t> (row)];
            activeNote.note = rowSteps.notes[static_cast<size_t> (step)];
            activeNote.velocity = velocity;
            activeNote.valid = true;
        }

        if (hocketModeEnabled)
            flushActiveNote (collectionPpqEnd);
        else if (activeNote.valid && ! collectionFull)
        {
            const auto gateQuarters = activeNote.endPpq - activeNote.startPpq;

            if (gateQuarters > epsilon)
            {
                if (! appendEvent (CombinedNoteEvent { activeNote.startPpq,
                                                       gateQuarters,
                                                       row,
                                                       activeNote.step,
                                                       activeNote.channel,
                                                       activeNote.note,
                                                       activeNote.velocity }))
                    collectionFull = true;
            }
        }
    }

    if (eventCount == 0)
        return;

    std::sort (combinedEvents.begin(),
               combinedEvents.begin() + static_cast<std::ptrdiff_t> (eventCount),
               compareCombinedEventsForWeave);

    if (eventCount == 0)
        return;

    const auto copyFilteredEvents = [&] (const size_t count) {
        for (size_t index = 0; index < count; ++index)
            combinedEvents[index] = combinedWorkingEvents[index];
    };

    const auto sortCombinedEvents = [&] {
        std::sort (combinedEvents.begin(),
                   combinedEvents.begin() + static_cast<std::ptrdiff_t> (eventCount),
                   compareCombinedEventsForWeave);
    };

    const auto activeRowPosition = [&] (const int row) {
        for (int index = 0; index < activeRowCount; ++index)
        {
            if (activeRows[static_cast<size_t> (index)] == row)
                return index;
        }

        return 0;
    };

    const auto firstNoteForRow = [&] (const int row) {
        const auto& rowSteps = state.rows[static_cast<size_t> (row)];
        return rowSteps.stepCount > 0 ? rowSteps.notes[0] : emptyRowDefaultNote;
    };

    const auto eventStartCollides = [&] (const double ppq) {
        for (size_t index = 0; index < eventCount; ++index)
        {
            if (std::abs (combinedEvents[index].ppq - ppq) <= epsilon)
                return true;
        }

        return false;
    };

    struct RoundRobinWindow
    {
        int currentRow = 0;
        int nextRow = 0;
        bool overlap = false;
    };

    const auto roundRobinWindowForPpq = [&] (const double ppq) {
        const auto segmentLength = juce::jmax (epsilon, combinationGesturePulse);
        const auto segmentIndex =
            static_cast<int> (std::floor ((ppq + epsilon) / segmentLength));
        const auto currentIndex =
            ((segmentIndex % activeRowCount) + activeRowCount) % activeRowCount;
        const auto phase = positiveMod (ppq, segmentLength);
        const auto currentRow = activeRows[static_cast<size_t> (currentIndex)];
        const auto overlapLength = segmentLength * roundRobinOverlapFraction;
        const auto inOverlap =
            overlapLength > epsilon && phase >= segmentLength - overlapLength - epsilon;

        if (! inOverlap)
            return RoundRobinWindow { currentRow, currentRow, false };

        return RoundRobinWindow {
            currentRow,
            activeRows[static_cast<size_t> ((currentIndex + 1) % activeRowCount)],
            true
        };
    };

    if (combinationModeEnabled (modeMask, combinationModeCrossModulation) && activeRowCount > 1)
    {
        for (size_t index = 0; index < eventCount; ++index)
        {
            auto& event = combinedEvents[index];
            const auto position = activeRowPosition (event.row);
            const auto pitchRow = activeRows[static_cast<size_t> ((position + 1) % activeRowCount)];
            const auto velocityRow = activeRows[static_cast<size_t> ((position + 2) % activeRowCount)];
            const auto durationRow = activeRows[static_cast<size_t> ((position + 3) % activeRowCount)];
            const auto& pitchSteps = state.rows[static_cast<size_t> (pitchRow)];
            const auto& velocitySteps = state.rows[static_cast<size_t> (velocityRow)];
            const auto& durationSteps = state.rows[static_cast<size_t> (durationRow)];
            const auto pitchStep = event.step % juce::jmax (1, pitchSteps.stepCount);
            const auto velocityStep = event.step % juce::jmax (1, velocitySteps.stepCount);
            const auto durationStep = event.step % juce::jmax (1, durationSteps.stepCount);
            event.note = echoNoteFromModStep (event.note,
                                              firstNoteForRow (pitchRow),
                                              pitchSteps.notes[static_cast<size_t> (pitchStep)],
                                              scaleRoot,
                                              scaleModeIndex);
            event.velocity = juce::jlimit (
                1,
                127,
                velocitySteps.velocity[static_cast<size_t> (velocityStep)]);

            const auto durationGate =
                durationSteps.stepLengthQuarters[static_cast<size_t> (durationStep)]
                * durationSteps.durationFraction[static_cast<size_t> (durationStep)];

            if (durationGate > epsilon)
                event.gateQuarters = durationGate;
        }
    }

    if (combinationModeEnabled (modeMask, combinationModeCanon) && activeRowCount > 1)
    {
        const auto originalCount = eventCount;

        for (size_t index = 0; index < originalCount; ++index)
            combinedWorkingEvents[index] = combinedEvents[index];

        auto write = originalCount;
        const auto canonDelay = combinationGesturePulse / static_cast<double> (activeRowCount);

        for (size_t read = 0; read < originalCount && write < combinedWorkingEvents.size(); ++read)
        {
            const auto& source = combinedEvents[read];
            const auto position = activeRowPosition (source.row);
            const auto targetRow = activeRows[static_cast<size_t> ((position + 1) % activeRowCount)];
            const auto& targetSteps = state.rows[static_cast<size_t> (targetRow)];

            if (targetSteps.stepCount <= 0)
                continue;

            const auto targetStep = source.step % juce::jmax (1, targetSteps.stepCount);
            const auto sourceBase = firstNoteForRow (source.row);
            const auto targetBase = firstNoteForRow (targetRow);

            auto copy = source;
            copy.ppq = source.ppq + canonDelay;
            copy.note = transposeMidiByScaleDegrees (
                targetBase,
                scaleDegreeDelta (sourceBase, source.note, scaleRoot, scaleModeIndex),
                scaleRoot,
                scaleModeIndex);
            copy.velocity = juce::jlimit (
                1,
                127,
                static_cast<int> (std::lround (static_cast<double> (source.velocity) * 0.78)));
            copy.row = targetRow;
            copy.channel = state.midiChannel[static_cast<size_t> (targetRow)];
            copy.step = targetStep;
            combinedWorkingEvents[write++] = copy;
        }

        eventCount = write;
        copyFilteredEvents (eventCount);
        sortCombinedEvents();
    }

    if (combinationModeEnabled (modeMask, combinationModeRetroInversion) && activeRowCount > 1)
    {
        const auto originalCount = eventCount;

        for (size_t index = 0; index < originalCount; ++index)
            combinedWorkingEvents[index] = combinedEvents[index];

        auto write = originalCount;
        const auto mirrorDelay = combinationGesturePulse * 0.25;

        for (size_t read = 0; read < originalCount && write < combinedWorkingEvents.size(); ++read)
        {
            const auto& source = combinedEvents[read];
            const auto& sourceSteps = state.rows[static_cast<size_t> (source.row)];

            if (sourceSteps.stepCount <= 0)
                continue;

            const auto position = activeRowPosition (source.row);
            const auto targetRow = activeRows[static_cast<size_t> ((position + 1) % activeRowCount)];
            const auto& targetSteps = state.rows[static_cast<size_t> (targetRow)];

            if (targetSteps.stepCount <= 0)
                continue;

            const auto sourceStep = source.step % juce::jmax (1, sourceSteps.stepCount);
            const auto mirroredStep = sourceSteps.stepCount - 1 - sourceStep;
            const auto targetStep = mirroredStep % juce::jmax (1, targetSteps.stepCount);
            const auto sourceBase = firstNoteForRow (source.row);
            const auto targetBase = firstNoteForRow (targetRow);
            const auto mirroredSourceNote =
                sourceSteps.notes[static_cast<size_t> (mirroredStep)];
            const auto mirrorGate = source.gateQuarters * 0.7;

            if (mirrorGate <= epsilon)
                continue;

            auto copy = source;
            copy.ppq = source.ppq + mirrorDelay;
            copy.gateQuarters = mirrorGate;
            copy.note = transposeMidiByScaleDegrees (
                targetBase,
                -scaleDegreeDelta (sourceBase, mirroredSourceNote, scaleRoot, scaleModeIndex),
                scaleRoot,
                scaleModeIndex);
            copy.velocity = juce::jlimit (
                1,
                127,
                static_cast<int> (std::lround (static_cast<double> (source.velocity) * 0.68)));
            copy.row = targetRow;
            copy.channel = state.midiChannel[static_cast<size_t> (targetRow)];
            copy.step = targetStep;
            combinedWorkingEvents[write++] = copy;
        }

        eventCount = write;
        copyFilteredEvents (eventCount);
        sortCombinedEvents();
    }

    if (combinationModeEnabled (modeMask, combinationModeHocket) && activeRowCount > 1)
    {
        const auto sliceQuarters = pulse / static_cast<double> (activeRowCount);
        const auto minimumHocketSliceOverlap =
            sliceQuarters * hocketMinimumSliceOverlapFraction;

        if (sliceQuarters > epsilon)
        {
            auto minSlice = std::numeric_limits<int>::max();
            auto maxSlice = std::numeric_limits<int>::min();

            for (size_t index = 0; index < eventCount; ++index)
            {
                const auto& event = combinedEvents[index];
                const auto eventEnd = event.ppq + event.gateQuarters;

                if (eventEnd <= event.ppq + epsilon)
                    continue;

                const auto firstSlice = static_cast<int> (std::floor ((event.ppq + epsilon) / sliceQuarters));
                const auto lastSlice = static_cast<int> (std::ceil ((eventEnd - epsilon) / sliceQuarters)) - 1;

                minSlice = juce::jmin (minSlice, firstSlice);
                maxSlice = juce::jmax (maxSlice, lastSlice);
            }

            const auto scheduleFirstSlice =
                static_cast<int> (std::floor ((emitPpqStart + epsilon) / sliceQuarters));
            const auto scheduleLastSlice =
                static_cast<int> (std::ceil ((emitPpqEnd - epsilon) / sliceQuarters)) - 1;

            if (minSlice > maxSlice)
            {
                minSlice = scheduleFirstSlice;
                maxSlice = scheduleLastSlice;
            }
            else
            {
                minSlice = juce::jmin (minSlice, scheduleFirstSlice);
                maxSlice = juce::jmax (maxSlice, scheduleLastSlice);
            }

            auto write = static_cast<size_t> (0);

            if (minSlice <= maxSlice)
            {
                for (int slice = minSlice; slice <= maxSlice && write < combinedWorkingEvents.size(); ++slice)
                {
                    const auto sliceStart = static_cast<double> (slice) * sliceQuarters;
                    const auto sliceEnd = sliceStart + sliceQuarters;

                    if (sliceStart < emitPpqStart - epsilon)
                        continue;

                    if (sliceStart >= emitPpqEnd - epsilon)
                        continue;

                    if (sliceStart >= hocketLengthQuarters - epsilon)
                        continue;

                    const auto targetIndex = ((slice % activeRowCount) + activeRowCount) % activeRowCount;
                    const auto targetRow = activeRows[static_cast<size_t> (targetIndex)];
                    std::array<size_t, 256> candidateIndices {};
                    auto candidateCount = static_cast<size_t> (0);
                    auto totalWeight = 0;

                    for (size_t index = 0; index < eventCount; ++index)
                    {
                        const auto& event = combinedEvents[index];
                        const auto eventEnd = event.ppq + event.gateQuarters;
                        const auto start = juce::jmax (event.ppq, sliceStart);
                        const auto end = juce::jmin (eventEnd, sliceEnd, hocketLengthQuarters);
                        const auto overlap = end - start;

                        if (overlap <= epsilon
                            || overlap + epsilon < minimumHocketSliceOverlap)
                            continue;

                        const auto& sourceSteps = state.rows[static_cast<size_t> (event.row)];
                        const auto sourceStepIndex =
                            static_cast<size_t> (event.step % juce::jmax (1, sourceSteps.stepCount));
                        const auto pickVelocity =
                            sourceSteps.velocity[sourceStepIndex] > 0
                                ? sourceSteps.velocity[sourceStepIndex]
                                : event.velocity;

                        if (candidateCount >= candidateIndices.size())
                            break;

                        candidateIndices[candidateCount++] = index;
                        totalWeight += juce::jmax (1, pickVelocity);
                    }

                    if (candidateCount <= 0)
                        continue;

                    if (candidateCount == 1)
                    {
                        const auto& source = combinedEvents[candidateIndices[0]];
                        const auto sourceEnd = source.ppq + source.gateQuarters;
                        const auto start = juce::jmax (source.ppq, sliceStart);
                        const auto end = juce::jmin (sourceEnd, sliceEnd, hocketLengthQuarters);
                        const auto gate = juce::jmin (end - start, sliceQuarters * 0.85);

                        if (gate <= epsilon)
                            continue;

                        auto copy = source;
                        copy.ppq = start;
                        copy.gateQuarters = gate;
                        copy.row = targetRow;
                        copy.channel = state.midiChannel[static_cast<size_t> (targetRow)];
                        combinedWorkingEvents[write++] = copy;
                        continue;
                    }

                    std::sort (candidateIndices.begin(),
                               candidateIndices.begin() + static_cast<std::ptrdiff_t> (candidateCount),
                               [&] (const size_t a, const size_t b) {
                                   const auto& eventA = combinedEvents[a];
                                   const auto& eventB = combinedEvents[b];
                                   const auto startA = juce::jmax (eventA.ppq, sliceStart);
                                   const auto startB = juce::jmax (eventB.ppq, sliceStart);

                                   if (std::abs (startA - startB) > epsilon)
                                       return startA < startB;

                                   if (eventA.note != eventB.note)
                                       return eventA.note < eventB.note;

                                   if (eventA.row != eventB.row)
                                       return eventA.row < eventB.row;

                                   return eventA.step < eventB.step;
                               });

                    auto pick = static_cast<int> (
                        deterministicEventHash (targetRow, slice, sliceStart)
                        % static_cast<std::uint32_t> (juce::jmax (1, totalWeight)));
                    auto selected = candidateIndices[0];
                    auto found = false;

                    for (size_t candidateIndex = 0; candidateIndex < candidateCount; ++candidateIndex)
                    {
                        const auto index = candidateIndices[candidateIndex];
                        const auto& event = combinedEvents[index];
                        const auto& sourceSteps = state.rows[static_cast<size_t> (event.row)];
                        const auto sourceStepIndex =
                            static_cast<size_t> (event.step % juce::jmax (1, sourceSteps.stepCount));
                        const auto pickVelocity =
                            sourceSteps.velocity[sourceStepIndex] > 0
                                ? sourceSteps.velocity[sourceStepIndex]
                                : event.velocity;

                        pick -= juce::jmax (1, pickVelocity);

                        if (pick < 0)
                        {
                            selected = index;
                            found = true;
                            break;
                        }
                    }

                    if (! found)
                        continue;

                    const auto& source = combinedEvents[selected];
                    const auto sourceEnd = source.ppq + source.gateQuarters;
                    const auto start = juce::jmax (source.ppq, sliceStart);
                    const auto end = juce::jmin (sourceEnd, sliceEnd, hocketLengthQuarters);
                    const auto gate = juce::jmin (end - start, sliceQuarters * 0.85);

                    if (gate <= epsilon)
                        continue;

                    auto copy = source;
                    copy.ppq = start;
                    copy.gateQuarters = gate;
                    copy.row = targetRow;
                    copy.channel = state.midiChannel[static_cast<size_t> (targetRow)];
                    combinedWorkingEvents[write++] = copy;
                }
            }

            eventCount = write;
            copyFilteredEvents (eventCount);

            if (eventCount == 0)
                return;

            sortCombinedEvents();
        }
    }

    if (combinationModeEnabled (modeMask, combinationModeRoundRobin) && activeRowCount > 1)
    {
        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount;)
        {
            auto groupEnd = read + 1;

            while (groupEnd < eventCount
                   && std::abs (combinedEvents[groupEnd].ppq - combinedEvents[read].ppq) <= epsilon)
            {
                ++groupEnd;
            }

            const auto window = roundRobinWindowForPpq (combinedEvents[read].ppq);

            if (! window.overlap)
            {
                for (size_t index = read; index < groupEnd && write < combinedWorkingEvents.size(); ++index)
                {
                    if (combinedEvents[index].row == window.currentRow)
                        combinedWorkingEvents[write++] = combinedEvents[index];
                }

                read = groupEnd;
                continue;
            }

            auto eligibleCount = 0;
            auto totalWeight = 0;
            auto firstEligible = read;

            for (size_t index = read; index < groupEnd; ++index)
            {
                if (combinedEvents[index].row != window.currentRow
                    && combinedEvents[index].row != window.nextRow)
                {
                    continue;
                }

                if (eligibleCount == 0)
                    firstEligible = index;

                ++eligibleCount;
                totalWeight += juce::jmax (1, combinedEvents[index].velocity);
            }

            if (eligibleCount == 1)
            {
                if (write < combinedWorkingEvents.size())
                    combinedWorkingEvents[write++] = combinedEvents[firstEligible];

                read = groupEnd;
                continue;
            }

            if (eligibleCount > 1)
            {
                auto pick = static_cast<int> (
                    deterministicEventHash (combinedEvents[firstEligible].row,
                                            combinedEvents[firstEligible].step,
                                            combinedEvents[firstEligible].ppq)
                    % static_cast<std::uint32_t> (juce::jmax (1, totalWeight)));

                for (size_t index = read; index < groupEnd; ++index)
                {
                    if (combinedEvents[index].row != window.currentRow
                        && combinedEvents[index].row != window.nextRow)
                    {
                        continue;
                    }

                    pick -= juce::jmax (1, combinedEvents[index].velocity);

                    if (pick < 0)
                    {
                        if (write < combinedWorkingEvents.size())
                            combinedWorkingEvents[write++] = combinedEvents[index];

                        break;
                    }
                }
            }

            read = groupEnd;
        }

        eventCount = write;
        copyFilteredEvents (eventCount);

        if (eventCount == 0)
            return;

        sortCombinedEvents();
    }

    if (combinationModeEnabled (modeMask, combinationModeBloom) && activeRowCount > 1)
    {
        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount && write < combinedWorkingEvents.size(); ++read)
        {
            const auto& source = combinedEvents[read];
            combinedWorkingEvents[write++] = source;

            const auto position = activeRowPosition (source.row);
            const auto modRow = activeRows[static_cast<size_t> ((position + 1) % activeRowCount)];
            const auto& modSteps = state.rows[static_cast<size_t> (modRow)];

            if (modSteps.stepCount <= 0)
                continue;

            const auto modStep = source.step % juce::jmax (1, modSteps.stepCount);
            const auto previousModStep = (modStep + modSteps.stepCount - 1) % modSteps.stepCount;
            const auto movement = scaleDegreeDelta (
                modSteps.notes[static_cast<size_t> (previousModStep)],
                modSteps.notes[static_cast<size_t> (modStep)],
                scaleRoot,
                scaleModeIndex);
            const auto direction = movement < 0 ? -1 : 1;
            const auto sourceSupportsReturnBloom =
                source.gateQuarters >= combinationGesturePulse - epsilon;

            if (! isBloomGestureAnchor (source.ppq,
                                        source.gateQuarters,
                                        combinationGesturePulse))
                continue;

            const auto ornamentGate =
                juce::jmin (source.gateQuarters * 0.375, combinationGesturePulse * 0.25);

            if (ornamentGate <= epsilon)
                continue;

            const auto firstDelay = combinationGesturePulse * 0.25;
            const auto secondDelay = combinationGesturePulse * 0.5;

            const auto appendBloom = [&] (const int degreeDelta,
                                          const double delay,
                                          const double velocityScale) {
                if (write >= combinedWorkingEvents.size())
                    return;

                auto copy = source;
                copy.ppq = source.ppq + delay;

                copy.note = transposeMidiByScaleDegrees (source.note,
                                                         degreeDelta,
                                                         scaleRoot,
                                                         scaleModeIndex);

                if (copy.note == source.note)
                    return;

                copy.gateQuarters = ornamentGate;
                copy.velocity = juce::jlimit (
                    1,
                    127,
                    static_cast<int> (std::lround (static_cast<double> (source.velocity) * velocityScale)));
                combinedWorkingEvents[write++] = copy;
            };

            appendBloom (direction, firstDelay, 0.65);

            if (sourceSupportsReturnBloom && std::abs (movement) >= 2)
                appendBloom (-direction, secondDelay, 0.5);
        }

        eventCount = write;
        copyFilteredEvents (eventCount);

        if (eventCount == 0)
            return;

        sortCombinedEvents();
    }

    if (combinationModeEnabled (modeMask, combinationModeCounter) && activeRowCount > 1)
    {
        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount && write < combinedWorkingEvents.size(); ++read)
        {
            const auto& source = combinedEvents[read];
            combinedWorkingEvents[write++] = source;

            const auto position = activeRowPosition (source.row);
            const auto modRow = activeRows[static_cast<size_t> ((position + 1) % activeRowCount)];
            const auto& modSteps = state.rows[static_cast<size_t> (modRow)];

            if (modSteps.stepCount <= 0)
                continue;

            const auto modStep = (source.step + 1) % modSteps.stepCount;
            const auto modIndex = static_cast<size_t> (modStep);

            if (modSteps.stepSkipped[modIndex] != 0
                || modSteps.stepMuted[modIndex] != 0
                || modSteps.velocity[modIndex] <= 0)
                continue;

            auto counterDelay = combinationGesturePulse * 0.5;
            auto counterStart = source.ppq + counterDelay;

            if (eventStartCollides (counterStart))
            {
                counterDelay += combinationGesturePulse * 0.125;
                counterStart = source.ppq + counterDelay;

                if (eventStartCollides (counterStart))
                    continue;
            }

            const auto counterGate =
                juce::jmin (source.gateQuarters * 0.5, combinationGesturePulse * 0.375);

            if (counterGate <= epsilon)
                continue;

            auto copy = source;
            copy.ppq = counterStart;
            copy.gateQuarters = counterGate;
            copy.note = echoNoteFromModStep (source.note,
                                             firstNoteForRow (modRow),
                                             modSteps.notes[modIndex],
                                             scaleRoot,
                                             scaleModeIndex);
            copy.velocity = juce::jlimit (
                1,
                127,
                static_cast<int> (std::lround (
                    static_cast<double> (source.velocity + modSteps.velocity[modIndex]) * 0.31)));
            copy.step = modStep;
            combinedWorkingEvents[write++] = copy;
        }

        eventCount = write;
        copyFilteredEvents (eventCount);

        if (eventCount == 0)
            return;

        sortCombinedEvents();
    }

    if (combinationModeEnabled (modeMask, combinationModeMultiplyEcho) && activeRowCount > 1)
    {
        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount && write < combinedWorkingEvents.size(); ++read)
        {
            const auto& carrier = combinedEvents[read];
            const auto position = activeRowPosition (carrier.row);
            const auto modRow = activeRows[static_cast<size_t> ((position + 1) % activeRowCount)];
            const auto& modSteps = state.rows[static_cast<size_t> (modRow)];
            const auto modBaseNote = modSteps.stepCount > 0
                                         ? modSteps.notes[0]
                                         : emptyRowDefaultNote;

            for (int modStep = 0; modStep < modSteps.stepCount && write < combinedWorkingEvents.size(); ++modStep)
            {
                const auto modIndex = static_cast<size_t> (modStep);

                if (modSteps.stepSkipped[modIndex] != 0
                    || modSteps.stepMuted[modIndex] != 0
                    || modSteps.velocity[modIndex] <= 0)
                    continue;

                auto next = carrier;
                next.ppq = carrier.ppq + modSteps.stepStartQuarters[modIndex];

                if (next.ppq < schedulePpqStart - epsilon)
                    continue;

                const auto modGate =
                    modSteps.stepLengthQuarters[modIndex] * modSteps.durationFraction[modIndex];

                if (modGate <= epsilon)
                    continue;

                next.note = echoNoteFromModStep (carrier.note,
                                                 modBaseNote,
                                                 modSteps.notes[modIndex],
                                                 scaleRoot,
                                                 scaleModeIndex);
                next.velocity = juce::jlimit (1, 127, (carrier.velocity + modSteps.velocity[modIndex]) / 2);
                next.gateQuarters = juce::jmin (carrier.gateQuarters, modGate);
                next.step = modStep;
                combinedWorkingEvents[write++] = next;
            }
        }

        eventCount = write;
        copyFilteredEvents (eventCount);

        if (eventCount == 0)
            return;

        std::sort (combinedEvents.begin(),
                   combinedEvents.begin() + static_cast<std::ptrdiff_t> (eventCount),
                   compareCombinedEventsForWeave);
    }

    if (weaveModeEnabled)
    {
        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount;)
        {
            auto groupEnd = read + 1;

            while (groupEnd < eventCount
                   && std::abs (combinedEvents[groupEnd].ppq - combinedEvents[read].ppq) <= epsilon)
            {
                ++groupEnd;
            }

            if (groupEnd - read == 1)
            {
                if (write < combinedWorkingEvents.size())
                    combinedWorkingEvents[write++] = combinedEvents[read];
            }
            else
            {
                auto totalWeight = 0;

                for (size_t index = read; index < groupEnd; ++index)
                    totalWeight += juce::jmax (1, combinedEvents[index].velocity);

                auto pick = static_cast<int> (
                    deterministicEventHash (combinedEvents[read].row,
                                            combinedEvents[read].step,
                                            weaveHashPpq (combinedEvents[read].ppq))
                    % static_cast<std::uint32_t> (juce::jmax (1, totalWeight)));
                auto selected = read;

                for (size_t index = read; index < groupEnd; ++index)
                {
                    pick -= juce::jmax (1, combinedEvents[index].velocity);

                    if (pick < 0)
                    {
                        selected = index;
                        break;
                    }
                }

                if (write < combinedWorkingEvents.size())
                    combinedWorkingEvents[write++] = combinedEvents[selected];
            }

            read = groupEnd;
        }

        eventCount = write;
        copyFilteredEvents (eventCount);
    }

    {
        const auto downEnabled = activePattern.octavizerDown8vaEnabled != 0;
        const auto upEnabled = activePattern.octavizerUp8vaEnabled != 0;

        if ((downEnabled || upEnabled) && eventCount > 0)
        {
            const auto originalCount = eventCount;

            for (size_t index = 0; index < originalCount; ++index)
                combinedWorkingEvents[index] = combinedEvents[index];

            auto write = eventCount;

            for (size_t read = 0; read < originalCount; ++read)
            {
                const auto& event = combinedEvents[read];

                if (downEnabled)
                {
                    const auto shiftedNote = event.note - octavizerSemitoneShift;
                    const auto shiftedVelocity = octavizerOutputVelocity (
                        event.velocity,
                        activePattern.octavizerDown8vaRelativeVelocity);

                    if (shiftedNote >= minMidiNote && shiftedVelocity > 0
                        && write < combinedWorkingEvents.size())
                    {
                        auto copy = event;
                        copy.note = shiftedNote;
                        copy.velocity = shiftedVelocity;
                        combinedWorkingEvents[write++] = copy;
                    }
                }

                if (upEnabled)
                {
                    const auto shiftedNote = event.note + octavizerSemitoneShift;
                    const auto shiftedVelocity = octavizerOutputVelocity (
                        event.velocity,
                        activePattern.octavizerUp8vaRelativeVelocity);

                    if (shiftedNote <= maxMidiNote && shiftedVelocity > 0
                        && write < combinedWorkingEvents.size())
                    {
                        auto copy = event;
                        copy.note = shiftedNote;
                        copy.velocity = shiftedVelocity;
                        combinedWorkingEvents[write++] = copy;
                    }
                }
            }

            eventCount = write;
            copyFilteredEvents (eventCount);
        }
    }

    {
        const auto shimmerEnabled = activePattern.shimmerEnabled != 0;

        if (shimmerEnabled && eventCount > 0)
        {
            const auto shimmerDelayQuarters =
                stepTimingMultiplierForIndex (activePattern.shimmerDelayMultiplierIndex) * pulse;

            if (shimmerDelayQuarters > epsilon)
            {
                const auto originalCount = eventCount;

                for (size_t index = 0; index < originalCount; ++index)
                    combinedWorkingEvents[index] = combinedEvents[index];

                auto write = eventCount;

                for (size_t read = 0; read < originalCount; ++read)
                {
                    const auto& source = combinedEvents[read];

                    for (int tap = 1;; ++tap)
                    {
                        const auto tapVelocity = shimmerTapVelocity (source.velocity,
                                                                       tap,
                                                                       activePattern.shimmerFeedbackPercent,
                                                                       activePattern.shimmerMixPercent);

                        if (tapVelocity <= 0)
                            break;

                        const auto shiftedNote = source.note + tap * octavizerSemitoneShift;

                        if (shiftedNote > maxMidiNote)
                            break;

                        if (write >= combinedWorkingEvents.size())
                            break;

                        auto copy = source;
                        copy.ppq = source.ppq + static_cast<double> (tap) * shimmerDelayQuarters;
                        copy.note = shiftedNote;
                        copy.velocity = tapVelocity;
                        combinedWorkingEvents[write++] = copy;
                    }

                    if (write >= combinedWorkingEvents.size())
                        break;
                }

                eventCount = write;
                copyFilteredEvents (eventCount);

                std::sort (combinedEvents.begin(),
                           combinedEvents.begin() + static_cast<std::ptrdiff_t> (eventCount),
                           [] (const CombinedNoteEvent& a, const CombinedNoteEvent& b) {
                               if (std::abs (a.ppq - b.ppq) > 1.0e-9)
                                   return a.ppq < b.ppq;

                               return a.note < b.note;
                           });
            }
        }
    }

    {
        const auto bandpassLow = activePattern.noteBandpassLowMidi;
        const auto bandpassHigh = activePattern.noteBandpassHighMidi;
        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount; ++read)
        {
            const auto& event = combinedEvents[read];

            if (event.note < bandpassLow || event.note > bandpassHigh)
                continue;

            if (write < combinedWorkingEvents.size())
                combinedWorkingEvents[write++] = event;
        }

        eventCount = write;
        copyFilteredEvents (eventCount);
    }

    {
        const auto tiltAmount = activePattern.velocityTiltAmount;

        if (tiltAmount != 0)
        {
            const auto pivotMidi = activePattern.velocityTiltPivotMidi;

            for (size_t index = 0; index < eventCount; ++index)
            {
                auto& event = combinedEvents[index];
                event.velocity = velocityTiltOutputVelocity (event.velocity,
                                                             event.note,
                                                             pivotMidi,
                                                             tiltAmount);
            }
        }
    }

    {
        const auto transposeSemitones = activePattern.globalTransposeSemitones;

        if (transposeSemitones != 0)
        {
            for (size_t index = 0; index < eventCount; ++index)
            {
                auto& event = combinedEvents[index];
                event.note = juce::jlimit (minMidiNote,
                                           maxMidiNote,
                                           event.note + transposeSemitones);
            }
        }
    }

    if (modeMask != 0 && eventCount > 1)
    {
        std::sort (combinedEvents.begin(),
                   combinedEvents.begin() + static_cast<std::ptrdiff_t> (eventCount),
                   [] (const CombinedNoteEvent& a, const CombinedNoteEvent& b) {
                       if (a.channel != b.channel)
                           return a.channel < b.channel;

                       if (a.note != b.note)
                           return a.note < b.note;

                       if (std::abs (a.ppq - b.ppq) > 1.0e-9)
                           return a.ppq < b.ppq;

                       if (a.row != b.row)
                           return a.row < b.row;

                       return a.step < b.step;
                   });

        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount;)
        {
            auto merged = combinedEvents[read];
            auto mergedEnd = merged.ppq + merged.gateQuarters;
            auto groupEnd = read + 1;

            while (groupEnd < eventCount
                   && combinedEvents[groupEnd].channel == merged.channel
                   && combinedEvents[groupEnd].note == merged.note
                   && combinedEvents[groupEnd].ppq <= merged.ppq + unisonOverlapWindowQuarters + epsilon)
            {
                const auto& candidate = combinedEvents[groupEnd];
                merged.velocity = juce::jmax (merged.velocity, candidate.velocity);
                mergedEnd = juce::jmax (mergedEnd, candidate.ppq + candidate.gateQuarters);
                ++groupEnd;
            }

            merged.gateQuarters = juce::jmax (0.0, mergedEnd - merged.ppq);

            if (write < combinedWorkingEvents.size())
                combinedWorkingEvents[write++] = merged;

            read = groupEnd;
        }

        eventCount = write;
        copyFilteredEvents (eventCount);
        sortCombinedEvents();
    }

    if (weaveModeEnabled && eventCount > 1)
    {
        sortCombinedEvents();
        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount;)
        {
            auto groupEnd = read + 1;

            while (groupEnd < eventCount
                   && std::abs (combinedEvents[groupEnd].ppq - combinedEvents[read].ppq) <= epsilon)
            {
                ++groupEnd;
            }

            auto selected = read;

            if (groupEnd - read > 1)
            {
                auto totalWeight = 0;

                for (size_t index = read; index < groupEnd; ++index)
                    totalWeight += juce::jmax (1, combinedEvents[index].velocity);

                auto pick = static_cast<int> (
                    deterministicEventHash (combinedEvents[read].row,
                                            combinedEvents[read].step,
                                            weaveHashPpq (combinedEvents[read].ppq))
                    % static_cast<std::uint32_t> (juce::jmax (1, totalWeight)));

                for (size_t index = read; index < groupEnd; ++index)
                {
                    pick -= juce::jmax (1, combinedEvents[index].velocity);

                    if (pick < 0)
                    {
                        selected = index;
                        break;
                    }
                }
            }

            if (write < combinedWorkingEvents.size())
                combinedWorkingEvents[write++] = combinedEvents[selected];

            read = groupEnd;
        }

        for (size_t index = 0; index < write; ++index)
        {
            auto& event = combinedWorkingEvents[index];
            const auto eventEnd = event.ppq + event.gateQuarters;
            const auto clippedEnd =
                index + 1 < write ? juce::jmin (eventEnd, combinedWorkingEvents[index + 1].ppq)
                                  : eventEnd;
            event.gateQuarters = juce::jmax (0.0, clippedEnd - event.ppq);
        }

        auto compacted = static_cast<size_t> (0);

        for (size_t index = 0; index < write; ++index)
        {
            if (combinedWorkingEvents[index].gateQuarters <= epsilon)
                continue;

            combinedEvents[compacted++] = combinedWorkingEvents[index];
        }

        eventCount = compacted;
        sortCombinedEvents();
    }

    if (eventCount > 1)
    {
        std::sort (combinedEvents.begin(),
                   combinedEvents.begin() + static_cast<std::ptrdiff_t> (eventCount),
                   [] (const CombinedNoteEvent& a, const CombinedNoteEvent& b) {
                       if (a.channel != b.channel)
                           return a.channel < b.channel;

                       if (a.note != b.note)
                           return a.note < b.note;

                       if (std::abs (a.ppq - b.ppq) > 1.0e-9)
                           return a.ppq < b.ppq;

                       if (a.row != b.row)
                           return a.row < b.row;

                       return a.step < b.step;
                   });

        auto write = static_cast<size_t> (0);

        for (size_t read = 0; read < eventCount;)
        {
            auto merged = combinedEvents[read];
            auto mergedEnd = merged.ppq + merged.gateQuarters;
            auto next = read + 1;

            while (next < eventCount
                   && combinedEvents[next].channel == merged.channel
                   && combinedEvents[next].note == merged.note
                   && combinedEvents[next].ppq < mergedEnd - epsilon)
            {
                const auto& candidate = combinedEvents[next];
                mergedEnd = juce::jmax (mergedEnd, candidate.ppq + candidate.gateQuarters);
                ++next;
            }

            merged.gateQuarters = juce::jmax (0.0, mergedEnd - merged.ppq);

            if (write < combinedWorkingEvents.size())
                combinedWorkingEvents[write++] = merged;

            read = next;
        }

        eventCount = write;
        copyFilteredEvents (eventCount);
        sortCombinedEvents();
    }

    const auto extendPendingCombinedNoteOff = [&] (const CombinedNoteEvent& event) {
        const auto eventEndPpq = event.ppq + event.gateQuarters;

        if (eventEndPpq <= schedulePpqStart + epsilon)
            return;

        const auto eventEndTransportPpq =
            segmentTransportStartPpq + (eventEndPpq - schedulePpqStart);
        const auto samplesUntilOff = static_cast<int> (std::lround (
            (eventEndTransportPpq - bufferTransportStartPpq) / ppqPerSample));

        if (samplesUntilOff <= bufferSamples)
            return;

        const auto samplesRemainingAfterBuffer = samplesUntilOff - bufferSamples;

        for (size_t i = 0; i < pendingCombinedNoteOffCount; ++i)
        {
            auto& pending = pendingCombinedNoteOffs[i];

            if (pending.note < 0 || pending.channel != event.channel || pending.note != event.note)
                continue;

            pending.samplesRemaining =
                juce::jmax (pending.samplesRemaining, samplesRemainingAfterBuffer);
            return;
        }
    };

    for (size_t index = 0; index < eventCount; ++index)
    {
        auto event = combinedEvents[index];

        if (event.gateQuarters <= epsilon || event.velocity <= 0)
            continue;

        if (heldNoteDeOverlapLookbackEnabled
            && (event.ppq < schedulePpqStart - epsilon || event.ppq >= schedulePpqEnd - epsilon))
        {
            if (event.ppq < schedulePpqStart - epsilon)
                extendPendingCombinedNoteOff (event);

            continue;
        }

        const auto clippedEventPpq = juce::jmax (event.ppq, schedulePpqStart);
        const auto clippedGateQuarters =
            event.gateQuarters - (clippedEventPpq - event.ppq);

        if (clippedGateQuarters <= epsilon)
            continue;

        const auto stepLength = clippedGateQuarters;
        const auto timingRange = stepLength * timingHumanizeScale
                                 * (static_cast<double> (clampPercent (timingHumanize)) / 100.0);
        const auto timingOffset =
            timingRange > 0.0 ? (nextRandomUnitDouble (playbackRandomState) * 2.0 - 1.0) * timingRange
                              : 0.0;
        const auto delayQuarters = juce::jmax (0.0, timingOffset);
        const auto transportPpqAtNoteOn =
            segmentTransportStartPpq + (clippedEventPpq + delayQuarters - schedulePpqStart);
        const auto sampleOffset = static_cast<int> (std::lround (
            (transportPpqAtNoteOn - bufferTransportStartPpq) / ppqPerSample));
        const auto segmentTransportEndPpq =
            segmentTransportStartPpq + (schedulePpqEnd - schedulePpqStart);
        const auto bufferTransportEndPpq =
            bufferTransportStartPpq + static_cast<double> (bufferSamples) * ppqPerSample;
        const auto scheduleEndsBeforeBuffer =
            segmentTransportEndPpq < bufferTransportEndPpq - epsilon;
        const auto sustainAcrossLoop =
            shouldSustainGateAcrossLoopWrap (event.row,
                                             event.note,
                                             event.ppq,
                                             event.gateQuarters,
                                             schedulePpqEnd);
        const auto gateEndTransportPpq =
            scheduleEndsBeforeBuffer && ! sustainAcrossLoop
                ? juce::jmin (transportPpqAtNoteOn + clippedGateQuarters, segmentTransportEndPpq)
                : transportPpqAtNoteOn + clippedGateQuarters;
        const auto effectiveGateQuarters = gateEndTransportPpq - transportPpqAtNoteOn;

        if (effectiveGateQuarters <= epsilon)
            continue;

        const auto noteGateSamples = scheduleEndsBeforeBuffer && ! sustainAcrossLoop
                                         ? juce::jmax (
                                               1,
                                               static_cast<int> (std::lround (
                                                   effectiveGateQuarters / ppqPerSample)))
                                         : juce::jmax (
                                               1,
                                               static_cast<int> (std::lround (
                                                   clippedGateQuarters / ppqPerSample)));

        if (sampleOffset >= bufferSamples)
        {
            addPendingNoteOn (PendingNoteOn { event.row,
                                              event.step,
                                              event.channel,
                                              event.note,
                                              event.velocity,
                                              sampleOffset - bufferSamples,
                                              noteGateSamples,
                                              1 });
            continue;
        }

        if (weaveModeEnabled)
            flushActiveWeaveNotes (sampleOffset, midiMessages);

        emitLayeredGeneratedNote (event.channel,
                                  event.note,
                                  event.velocity,
                                  sampleOffset,
                                  noteGateSamples,
                                  bufferSamples,
                                  midiMessages);
    }
}

void PluginProcessor::processScheduledRange (const double schedulePpqStart,
                                             const double schedulePpqEnd,
                                             const double segmentTransportStartPpq,
                                             const double bufferTransportStartPpq,
                                             const int bufferSamples,
                                             const double ppqPerSample,
                                             juce::MidiBuffer& midiMessages,
                                             const bool resetRowTriggersAtSegmentStart)
{
    if (resetRowTriggersAtSegmentStart)
    {
        resetLastEmittedTriggers();

        const auto segmentSampleOffset = juce::jlimit (
            0,
            bufferSamples - 1,
            static_cast<int> (std::lround (
                (segmentTransportStartPpq - bufferTransportStartPpq) / ppqPerSample)));

        for (int row = 0; row < phraseRowCount; ++row)
        {
            auto& pending = pendingNoteOffs[static_cast<size_t> (row)];

            if (pending.note < 0)
                continue;

            if (shouldPreservePendingNoteAcrossLoopWrap (row, pending.note))
                continue;

            emitGeneratedNoteOff (pending.channel, pending.note, segmentSampleOffset, midiMessages);
            pending.note = -1;
            pending.activeStep = -1;
            pending.samplesRemaining = 0;
        }

        auto combinedWrite = static_cast<size_t> (0);

        for (size_t i = 0; i < pendingCombinedNoteOffCount; ++i)
        {
            auto pending = pendingCombinedNoteOffs[i];

            if (pending.note < 0)
                continue;

            auto preserve = false;

            for (int row = 0; row < phraseRowCount; ++row)
            {
                if (audioSequencer().midiChannel[static_cast<size_t> (row)] != pending.channel)
                    continue;

                if (shouldPreservePendingNoteAcrossLoopWrap (row, pending.note))
                {
                    preserve = true;
                    break;
                }
            }

            if (preserve)
            {
                pendingCombinedNoteOffs[combinedWrite++] = pending;
                continue;
            }

            emitGeneratedNoteOff (pending.channel, pending.note, segmentSampleOffset, midiMessages);
        }

        pendingCombinedNoteOffCount = combinedWrite;
    }

    processCombinedScheduledRange (schedulePpqStart,
                                   schedulePpqEnd,
                                   segmentTransportStartPpq,
                                   bufferTransportStartPpq,
                                   bufferSamples,
                                   ppqPerSample,
                                   midiMessages,
                                   resetRowTriggersAtSegmentStart);
}

void PluginProcessor::releaseResources()
{
    resetPlaybackMidiState();
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    const auto out = layouts.getMainOutputChannelSet();
    const auto in = layouts.getMainInputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    if (in.isDisabled())
        return true;

    return in == out;
  #else
    const auto out = layouts.getMainOutputChannelSet();
    const auto in = layouts.getMainInputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    if (in != out)
        return false;

    return true;
  #endif
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages)
{
    buffer.clear();

   #if ! JucePlugin_ProducesMidiOutput
    juce::ignoreUnused (midiMessages);
    return;
   #endif

    drainSequencerCommands();

    if (patternSlotParameter != nullptr)
    {
        const auto parameterPatternSlot =
            clampPatternSlot (static_cast<int> (patternSlotParameter->get()) - 1);

        if (parameterPatternSlot != lastObservedParameterPatternSlot)
        {
            lastObservedParameterPatternSlot = parameterPatternSlot;
            patternOutputArmed.store (1, std::memory_order_release);
            lastViewPatternSlot = parameterPatternSlot;
            currentModelPatternSlot.store (parameterPatternSlot, std::memory_order_release);
            deactivateLoopBraceForPatternSelection (parameterPatternSlot);
            requestAudioPatternSlot (parameterPatternSlot);
        }
    }

    handleIncomingControlNotes (midiMessages);
    drainSequencerCommands();
    applyMuteOutputSilence (midiMessages);

    const auto stopPlayback = [&] {
        if (wasPlaying)
        {
            flushActiveGeneratedNotes (0, midiMessages);

            for (int ch = 1; ch <= 16; ++ch)
                midiMessages.addEvent (juce::MidiMessage::allNotesOff (ch), 0);
        }

        resetPlaybackMidiState();
    };

    const bool wasPlayingBefore = wasPlaying;
    double ppqStart = 0.0;
    double bpm = 120.0;

    if (hasStandaloneTransport())
    {
        if (standaloneTransportResetRequested.exchange (0, std::memory_order_relaxed) != 0)
        {
            if (wasPlaying)
                flushActiveGeneratedNotes (0, midiMessages);

            resetPlaybackMidiState();
        }

        if (! isStandaloneTransportPlaying())
        {
            stopPlayback();
            return;
        }

        ppqStart = standaloneTransportPpqPosition.load (std::memory_order_relaxed);
        bpm = clampStandaloneTempoBpm (standaloneTempoBpm.load (std::memory_order_relaxed));
    }
    else
    {
        const auto* playHead = getPlayHead();

        if (playHead == nullptr)
            return;

        const auto position = playHead->getPosition();

        if (! position.hasValue() || ! position->getIsPlaying())
        {
            stopPlayback();
            return;
        }

        ppqStart = position->getPpqPosition().orFallback (0.0);
        bpm = position->getBpm().orFallback (120.0);
    }

    if (bpm <= 0.0)
        return;

    const bool justStartedPlayback = ! wasPlayingBefore;
    const auto previousPlaybackPpq = currentPlaybackPpq.load (std::memory_order_relaxed);

    wasPlaying = true;

    const auto bufferSamples = buffer.getNumSamples();

    for (int row = 0; row < phraseRowCount; ++row)
    {
        if (phraseRowFlushNoteOff[static_cast<size_t> (row)].exchange (0) == 0)
            continue;

        auto& pending = pendingNoteOffs[static_cast<size_t> (row)];

        if (pending.note >= 0)
        {
            emitGeneratedNoteOff (pending.channel, pending.note, 0, midiMessages);
            pending.note = -1;
            pending.activeStep = -1;
            pending.samplesRemaining = 0;
        }

        if (pendingCombinedNoteOffCount > 0)
        {
            flushPendingCombinedNoteOffsForChannel (
                audioSequencer().midiChannel[static_cast<size_t> (row)],
                0,
                midiMessages);
        }

        size_t writeIndex = 0;

        for (size_t i = 0; i < pendingNoteOnCount; ++i)
        {
            if (pendingNoteOns[i].row != row)
                pendingNoteOns[writeIndex++] = pendingNoteOns[i];
        }

        pendingNoteOnCount = writeIndex;
    }

    processPhraseRowNoteAuditions (bufferSamples, midiMessages);
    flushPendingNoteOns (bufferSamples, midiMessages);
    flushPendingCombinedNoteOffs (bufferSamples, midiMessages);

    for (auto& pending : pendingNoteOffs)
    {
        if (pending.note < 0)
            continue;

        if (pending.samplesRemaining < bufferSamples)
        {
            emitGeneratedNoteOff (pending.channel,
                                  pending.note,
                                  pending.samplesRemaining,
                                  midiMessages);
            pending.note = -1;
            pending.activeStep = -1;
            pending.samplesRemaining = 0;
        }
        else
        {
            pending.samplesRemaining -= bufferSamples;
        }
    }

    const auto ppqPerSample = (bpm / 60.0) / sampleRateHz;
    const auto ppqEnd = ppqStart + static_cast<double> (bufferSamples) * ppqPerSample;
    currentPlaybackPpq.store (ppqEnd, std::memory_order_relaxed);

    if (hasStandaloneTransport())
        standaloneTransportPpqPosition.store (ppqEnd, std::memory_order_relaxed);

    if (! isPatternOutputArmed())
        return;

    constexpr auto epsilon = 1.0e-9;

    auto loopAnchorChanged = false;
    auto patternAnchorChanged = false;

    if (audioLoopBrace().enabled != 0)
    {
        if (justStartedPlayback || loopScheduleReanchorRequested)
        {
            reanchorLoopScheduleAt (ppqStart);
            loopScheduleReanchorRequested = false;
            loopAnchorChanged = true;
        }
        else if (wasPlayingBefore && previousPlaybackPpq >= 0.0)
        {
            const auto expectedContinuousPpq =
                previousPlaybackPpq + static_cast<double> (bufferSamples) * ppqPerSample;
            const auto discontinuityThreshold =
                juce::jmax (ppqPerSample * static_cast<double> (bufferSamples) * 2.0,
                            0.25);

            if (std::abs (ppqStart - expectedContinuousPpq) > discontinuityThreshold)
            {
                reanchorLoopScheduleAt (ppqStart);
                loopAnchorChanged = true;
            }
        }
    }
    else
    {
        clearLoopScheduleAnchor();

        if (justStartedPlayback)
        {
            reanchorPatternScheduleAt (ppqStart);
            patternAnchorChanged = true;
        }
        else if (wasPlayingBefore && previousPlaybackPpq >= 0.0)
        {
            const auto expectedContinuousPpq =
                previousPlaybackPpq + static_cast<double> (bufferSamples) * ppqPerSample;
            const auto discontinuityThreshold =
                juce::jmax (ppqPerSample * static_cast<double> (bufferSamples) * 2.0,
                            0.25);

            if (std::abs (ppqStart - expectedContinuousPpq) > discontinuityThreshold)
            {
                reanchorPatternScheduleAt (ppqStart);
                patternAnchorChanged = true;
            }
        }
    }

    auto transportCursor = ppqStart;
    auto resetAtSegmentStart = audioLoopBrace().enabled != 0
                                   ? (justStartedPlayback || loopAnchorChanged)
                                   : (justStartedPlayback || patternAnchorChanged);

    const auto pendingLoopAtBufferStart =
        pendingAudioLoopSlot.load (std::memory_order_acquire);

    if (pendingLoopAtBufferStart >= 0)
    {
        const auto loopDownbeat =
            loopDownbeatTransportForSlot (pendingLoopAtBufferStart, ppqStart);

        if (loopDownbeat >= ppqStart - epsilon && loopDownbeat <= ppqEnd + epsilon)
        {
            flushPendingGeneratedNoteOffs (juce::jlimit (
                                               0,
                                               bufferSamples - 1,
                                               static_cast<int> (std::lround (
                                                   (loopDownbeat - ppqStart) / ppqPerSample))),
                                           midiMessages);
            applyAudioLoopSlot (pendingAudioLoopSlot.exchange (-1, std::memory_order_acq_rel),
                                loopDownbeat);
            transportCursor = loopDownbeat;
            resetAtSegmentStart = true;
        }
    }

    const auto sampleOffsetForTransportPpq = [&] (const double transportPpq) {
        return juce::jlimit (
            0,
            bufferSamples - 1,
            static_cast<int> (std::lround ((transportPpq - ppqStart) / ppqPerSample)));
    };

    while (transportCursor < ppqEnd - epsilon)
    {
        auto segmentEnd = ppqEnd;
        auto segmentEndsAtScheduledSwitch = false;
        const auto pendingPattern = pendingAudioPatternSlot.load (std::memory_order_acquire);
        const auto pendingLoop = pendingAudioLoopSlot.load (std::memory_order_acquire);
        const auto pendingLoopBraceEnable = pendingAudioLoopBraceEnablePatternSlot;

        if (pendingPattern >= 0 || pendingLoop >= 0 || pendingLoopBraceEnable >= 0)
        {
            const auto pulse = pulseQuartersForIndex (pulseIndex.load (std::memory_order_relaxed));
            auto switchPpq = pulse > 0.0
                                 ? std::ceil ((transportCursor - epsilon) / pulse) * pulse
                                 : transportCursor;

            if (pendingLoop >= 0)
            {
                const auto loopDownbeat = loopDownbeatTransportForSlot (pendingLoop, transportCursor);

                if (loopDownbeat >= transportCursor - epsilon && loopDownbeat <= switchPpq + epsilon)
                    switchPpq = loopDownbeat;
            }

            if (switchPpq <= transportCursor + epsilon)
            {
                flushPendingGeneratedNoteOffs (sampleOffsetForTransportPpq (transportCursor),
                                               midiMessages);

                if (pendingPattern >= 0)
                    applyAudioPatternSlot (pendingAudioPatternSlot.exchange (-1,
                                                                            std::memory_order_acq_rel),
                                           transportCursor);

                if (pendingLoop >= 0)
                    applyAudioLoopSlot (pendingAudioLoopSlot.exchange (-1, std::memory_order_acq_rel),
                                        transportCursor);

                if (pendingLoopBraceEnable >= 0)
                    applyAudioLoopBraceEnable (transportCursor);

                resetAtSegmentStart = true;
                continue;
            }

            if (switchPpq <= ppqEnd + epsilon)
            {
                segmentEnd = switchPpq;
                segmentEndsAtScheduledSwitch = true;
            }
        }

        processTransportPlaybackRange (transportCursor,
                                       segmentEnd,
                                       ppqStart,
                                       bufferSamples,
                                       ppqPerSample,
                                       midiMessages,
                                       resetAtSegmentStart);

        if (segmentEndsAtScheduledSwitch)
        {
            flushPendingGeneratedNoteOffs (sampleOffsetForTransportPpq (segmentEnd), midiMessages);

            const auto nextPattern = pendingAudioPatternSlot.exchange (-1, std::memory_order_acq_rel);

            if (nextPattern >= 0)
                applyAudioPatternSlot (nextPattern, segmentEnd);

            const auto nextLoop = pendingAudioLoopSlot.exchange (-1, std::memory_order_acq_rel);

            if (nextLoop >= 0)
                applyAudioLoopSlot (nextLoop, segmentEnd);

            if (pendingLoopBraceEnable >= 0)
                applyAudioLoopBraceEnable (segmentEnd);

            transportCursor = segmentEnd;

            if (segmentEnd >= ppqEnd - epsilon)
                break;

            resetAtSegmentStart = true;
            continue;
        }

        if (segmentEnd >= ppqEnd - epsilon)
            break;

        transportCursor = segmentEnd;
        resetAtSegmentStart = true;
    }
}

bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

#if JUCE_WEB_BROWSER
void PluginProcessor::setWebHostCursorHandler (std::function<void (const juce::String&)> handler)
{
    webHostCursorHandler = std::move (handler);
}

void PluginProcessor::notifyWebHostCursor (const juce::String& cursorName)
{
    if (webHostCursorHandler != nullptr)
        webHostCursorHandler (cursorName);
}

void PluginProcessor::setWebEditorFullscreenHandler (std::function<juce::var (int)> handler)
{
    webEditorFullscreenHandler = std::move (handler);
}

juce::var PluginProcessor::requestWebEditorFullscreen (const int mode)
{
    if (webEditorFullscreenHandler != nullptr)
        return webEditorFullscreenHandler (mode);

    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("enabled", 0);
    object->setProperty ("native", 0);
    object->setProperty ("available", 0);
    return juce::var (object.release());
}

void PluginProcessor::setWebEditorScaleMinimumHandler (std::function<juce::var (int, int)> handler)
{
    webEditorScaleMinimumHandler = std::move (handler);
}

juce::var PluginProcessor::requestWebEditorScaleMinimum (const int minWidth, const int minHeight)
{
    if (webEditorScaleMinimumHandler != nullptr)
        return webEditorScaleMinimumHandler (minWidth, minHeight);

    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("available", 0);
    return juce::var (object.release());
}
#endif

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("MidiPhrases");
    state.setProperty ("version", phraseStateVersion, nullptr);
    state.setProperty ("currentPatternSlot", getCurrentPatternSlot(), nullptr);
    state.setProperty ("lastViewPatternSlot", getViewPatternSlot(), nullptr);
    state.setProperty ("patternOutputArmed", isPatternOutputArmed() ? 1 : 0, nullptr);
    state.setProperty ("currentLoopSlot", getCurrentLoopSlot(), nullptr);
    state.setProperty ("pulseIndex", getPulseIndex(), nullptr);
    state.setProperty ("swingPercent", getSwingPercent(), nullptr);
    state.setProperty ("velocityHumanizePercent", getVelocityHumanizePercent(), nullptr);
    state.setProperty ("timingHumanizePercent", getTimingHumanizePercent(), nullptr);
    state.setProperty ("swingSubdivisionIndex", getSwingSubdivisionIndex(), nullptr);
    state.setProperty ("rowColorsEnabled", isRowColorsEnabled() ? 1 : 0, nullptr);
    state.setProperty ("standaloneTempoBpm", getStandaloneTempoBpm(), nullptr);
    state.setProperty ("projectName", projectName, nullptr);
    state.setProperty ("projectDescription", projectDescription, nullptr);
    state.setProperty ("projectCreatedAt", projectCreatedAt, nullptr);
    state.setProperty ("projectModifiedAt", projectModifiedAt, nullptr);
    state.setProperty ("projectThemeMode", projectThemeMode, nullptr);
    state.setProperty ("projectUiScalePercent", projectUiScalePercent, nullptr);
    state.setProperty ("projectStretchStepsToFit", projectStretchStepsToFit ? 1 : 0, nullptr);

    for (int patternSlot = 0; patternSlot < patternSlotCount; ++patternSlot)
    {
        juce::ValueTree patternTree ("Pattern");
        patternTree.setProperty ("index", patternSlot, nullptr);
        patternTree.setProperty ("loopBraceEnabled", isPatternLoopBraceEnabled (patternSlot), nullptr);
        patternTree.setProperty ("loopBraceStart", getPatternLoopBraceStartQuarters (patternSlot), nullptr);
        patternTree.setProperty ("loopBraceEnd", getPatternLoopBraceEndQuarters (patternSlot), nullptr);
        patternTree.setProperty ("combinationModeMask",
                                 getPatternCombinationModeMask (patternSlot),
                                 nullptr);
        patternTree.setProperty ("scaleRoot", getPatternScaleRoot (patternSlot), nullptr);
        patternTree.setProperty ("scaleModeIndex", getPatternScaleModeIndex (patternSlot), nullptr);
        patternTree.setProperty ("noteBandpassLowMidi",
                                 getPatternNoteBandpassLow (patternSlot),
                                 nullptr);
        patternTree.setProperty ("noteBandpassHighMidi",
                                 getPatternNoteBandpassHigh (patternSlot),
                                 nullptr);
        patternTree.setProperty ("velocityTiltPivotMidi",
                                 getPatternVelocityTiltPivotMidi (patternSlot),
                                 nullptr);
        patternTree.setProperty ("velocityTiltAmount",
                                 getPatternVelocityTiltAmount (patternSlot),
                                 nullptr);
        patternTree.setProperty ("globalTransposeSemitones",
                                 getPatternGlobalTransposeSemitones (patternSlot),
                                 nullptr);
        patternTree.setProperty ("octavizerDown8vaEnabled",
                                 isPatternOctavizerDown8vaEnabled (patternSlot) ? 1 : 0,
                                 nullptr);
        patternTree.setProperty ("octavizerUp8vaEnabled",
                                 isPatternOctavizerUp8vaEnabled (patternSlot) ? 1 : 0,
                                 nullptr);
        patternTree.setProperty ("octavizerDown8vaRelativeVelocity",
                                 getPatternOctavizerDown8vaRelativeVelocity (patternSlot),
                                 nullptr);
        patternTree.setProperty ("octavizerUp8vaRelativeVelocity",
                                 getPatternOctavizerUp8vaRelativeVelocity (patternSlot),
                                 nullptr);
        patternTree.setProperty ("shimmerEnabled",
                                 isPatternShimmerEnabled (patternSlot) ? 1 : 0,
                                 nullptr);
        patternTree.setProperty ("shimmerDelayMultiplierIndex",
                                 getPatternShimmerDelayMultiplierIndex (patternSlot),
                                 nullptr);
        patternTree.setProperty ("shimmerFeedbackPercent",
                                 getPatternShimmerFeedbackPercent (patternSlot),
                                 nullptr);
        patternTree.setProperty ("shimmerMixPercent",
                                 getPatternShimmerMixPercent (patternSlot),
                                 nullptr);
        patternTree.setProperty ("seedingRhythmStep",
                                 getPatternSeedingRhythmStep (patternSlot),
                                 nullptr);

        for (int seedingRow = 0; seedingRow < phraseRowCount; ++seedingRow)
        {
            const auto rowState = getPatternSeedingRowState (patternSlot, seedingRow);
            const auto prefix = "seedingRow" + juce::String (seedingRow);
            patternTree.setProperty (prefix + "PhraseLength", rowState.phraseLength, nullptr);
            patternTree.setProperty (prefix + "CenterMidi", rowState.centerMidi, nullptr);
            patternTree.setProperty (prefix + "RangeSemitones", rowState.rangeSemitones, nullptr);
            patternTree.setProperty (prefix + "Repetition", rowState.repetition, nullptr);
            patternTree.setProperty (prefix + "Complexity", rowState.complexity, nullptr);
            patternTree.setProperty (prefix + "Randomness", rowState.randomness, nullptr);
            patternTree.setProperty (prefix + "TimingMeanMultiplierIndex",
                                     rowState.timingMeanMultiplierIndex,
                                     nullptr);
            patternTree.setProperty (prefix + "TimingVariance", rowState.timingVariance, nullptr);
            patternTree.setProperty (prefix + "Symmetry", rowState.symmetry, nullptr);
            patternTree.setProperty (prefix + "Seed", rowState.seed, nullptr);
            patternTree.setProperty (prefix + "RepetitionSeed", rowState.repetitionSeed, nullptr);
            patternTree.setProperty (prefix + "ComplexitySeed", rowState.complexitySeed, nullptr);
            patternTree.setProperty (prefix + "RandomnessSeed", rowState.randomnessSeed, nullptr);
            patternTree.setProperty (prefix + "TimingVarianceSeed", rowState.timingVarianceSeed, nullptr);
            patternTree.setProperty (prefix + "Targeted",
                                     isPatternSeedingRowTargeted (patternSlot, seedingRow) ? 1 : 0,
                                     nullptr);
        }

        for (int row = 0; row < phraseRowCount; ++row)
        {
            juce::ValueTree rowTree ("Row");
            rowTree.setProperty ("index", row, nullptr);

            const auto stepCount = getPatternPhraseRowStepCount (patternSlot, row);
            rowTree.setProperty ("stepCount", stepCount, nullptr);

            for (int step = 0; step < stepCount; ++step)
            {
                const auto propName = "step" + juce::String (step);
                const auto durationPropName = "duration" + juce::String (step);
                const auto velocityPropName = "velocity" + juce::String (step);
                const auto stepMutedPropName = "stepMuted" + juce::String (step);
                const auto stepSkippedPropName = "stepSkipped" + juce::String (step);
                const auto probabilityPropName = "probability" + juce::String (step);
                const auto cyclePropName = "cycle" + juce::String (step);
                const auto cycleOffsetPropName = "cycleOffset" + juce::String (step);
                const auto timingMultiplierPropName = "timingMultiplier" + juce::String (step);
                rowTree.setProperty (propName,
                                     getPatternPhraseNote (patternSlot, row, step),
                                     nullptr);
                rowTree.setProperty (timingMultiplierPropName,
                                     getPatternPhraseStepTimingMultiplier (patternSlot, row, step),
                                     nullptr);
                rowTree.setProperty (durationPropName,
                                     getPatternPhraseStepDurationFraction (patternSlot, row, step),
                                     nullptr);
                rowTree.setProperty (velocityPropName,
                                     getPatternPhraseStepVelocity (patternSlot, row, step),
                                     nullptr);
                rowTree.setProperty (stepMutedPropName,
                                     isPatternPhraseStepMuted (patternSlot, row, step),
                                     nullptr);
                rowTree.setProperty (stepSkippedPropName,
                                     isPatternPhraseStepSkipped (patternSlot, row, step),
                                     nullptr);
                rowTree.setProperty (probabilityPropName,
                                     getPatternPhraseStepProbability (patternSlot, row, step),
                                     nullptr);
                rowTree.setProperty (cyclePropName,
                                     getPatternPhraseStepCycle (patternSlot, row, step),
                                     nullptr);
                rowTree.setProperty (cycleOffsetPropName,
                                     getPatternPhraseStepCycleOffset (patternSlot, row, step),
                                     nullptr);
            }

            rowTree.setProperty ("muted", isPatternPhraseRowMuted (patternSlot, row), nullptr);
            rowTree.setProperty ("timingOffset",
                                 getPatternPhraseRowTimingOffset (patternSlot, row),
                                 nullptr);
            rowTree.setProperty ("midiChannel",
                                 getPatternPhraseRowMidiChannel (patternSlot, row),
                                 nullptr);
            patternTree.appendChild (rowTree, nullptr);
        }

        state.appendChild (patternTree, nullptr);
    }

    for (int loopSlot = 0; loopSlot < loopSlotCount; ++loopSlot)
    {
        juce::ValueTree loopTree ("LoopSlot");
        loopTree.setProperty ("index", loopSlot, nullptr);
        const auto& loopSlotState = loopSlots[static_cast<size_t> (loopSlot)];
        loopTree.setProperty ("assigned", loopSlotState.assigned != 0, nullptr);
        loopTree.setProperty ("patternSlot", clampPatternSlot (loopSlotState.patternSlot), nullptr);
        loopTree.setProperty ("start", loopSlotState.startQuarters, nullptr);
        loopTree.setProperty ("end", loopSlotState.endQuarters, nullptr);
        state.appendChild (loopTree, nullptr);
    }

    if (auto xml = state.createXml())
    {
        juce::MemoryOutputStream stream;
        xml->writeTo (stream);
        destData.replaceAll (stream.getData(), stream.getDataSize());
    }
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const auto xml = juce::parseXML (juce::String::createStringFromData (data, sizeInBytes));

    if (xml == nullptr)
        return;

    const auto state = juce::ValueTree::fromXml (*xml);

    if (! state.isValid() || ! state.hasType ("MidiPhrases"))
        return;

    const auto stateVersion = static_cast<int> (state.getProperty ("version", 1));

    projectName = state.getProperty ("projectName", "Untitled Project").toString();
    projectDescription = state.getProperty ("projectDescription", juce::String()).toString();
    projectCreatedAt = state.getProperty ("projectCreatedAt", juce::String()).toString();
    projectModifiedAt = state.getProperty ("projectModifiedAt", juce::String()).toString();
    projectThemeMode = state.getProperty ("projectThemeMode", "dark").toString();
    projectUiScalePercent = juce::jlimit (
        50,
        100,
        static_cast<int> (state.getProperty ("projectUiScalePercent", 100)));
    projectStretchStepsToFit =
        static_cast<int> (state.getProperty ("projectStretchStepsToFit", 0)) != 0;

    for (auto& pattern : modelPatterns)
        initialisePatternDefaults (pattern);

    for (auto& loopSlot : loopSlots)
        loopSlot = {};

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto patternTree = state.getChild (i);

        if (! patternTree.hasType ("Pattern"))
            continue;

        const auto patternSlot = clampPatternSlot (
            static_cast<int> (patternTree.getProperty ("index", i)));
        auto& pattern = modelPattern (patternSlot);
        const auto patternScaleRoot = clampScaleRoot (
            static_cast<int> (patternTree.getProperty ("scaleRoot", defaultScaleRoot)));

        for (int rowIndex = 0; rowIndex < patternTree.getNumChildren(); ++rowIndex)
        {
            const auto rowTree = patternTree.getChild (rowIndex);

            if (! rowTree.hasType ("Row"))
                continue;

            const auto row = static_cast<int> (rowTree.getProperty ("index", rowIndex));

            if (row < 0 || row >= phraseRowCount)
                continue;

            const auto stepCount = juce::jlimit (
                0,
                maxPhraseStepsPerRow,
                static_cast<int> (rowTree.getProperty ("stepCount", defaultPhraseStepsPerRow)));

            auto& steps = pattern.sequencer.rows[static_cast<size_t> (row)];
            initialiseRowDefaults (steps, row, stepCount, patternScaleRoot);

            for (int step = 0; step < stepCount; ++step)
            {
                const auto propName = "step" + juce::String (step);
                const auto durationPropName = "duration" + juce::String (step);
                const auto velocityPropName = "velocity" + juce::String (step);
                const auto stepMutedPropName = "stepMuted" + juce::String (step);
                const auto stepSkippedPropName = "stepSkipped" + juce::String (step);
                const auto probabilityPropName = "probability" + juce::String (step);
                const auto cyclePropName = "cycle" + juce::String (step);
                const auto cycleOffsetPropName = "cycleOffset" + juce::String (step);
                const auto timingMultiplierPropName = "timingMultiplier" + juce::String (step);
                const auto index = static_cast<size_t> (step);
                steps.notes[index] = juce::jlimit (
                    0,
                    127,
                    static_cast<int> (rowTree.getProperty (
                        propName,
                        defaultStepNoteForScaleRoot (patternScaleRoot))));
                steps.timingMultiplier[index] = stepTimingMultiplierIndexFromState (
                    static_cast<int> (rowTree.getProperty (timingMultiplierPropName,
                                                           defaultStepTimingMultiplierIndex)),
                    stateVersion);
                steps.durationFraction[index] = durationFractionFromStateProperty (
                    rowTree.getProperty (durationPropName, defaultStepDurationFraction),
                    stateVersion);
                steps.velocity[index] = juce::jlimit (
                    0,
                    127,
                    static_cast<int> (rowTree.getProperty (velocityPropName, defaultStepVelocity)));
                steps.stepMuted[index] =
                    static_cast<bool> (rowTree.getProperty (stepMutedPropName, false)) ? 1 : 0;
                steps.stepSkipped[index] =
                    static_cast<bool> (rowTree.getProperty (stepSkippedPropName, false)) ? 1 : 0;

                if (steps.stepMuted[index] != 0 && steps.stepSkipped[index] != 0)
                    steps.stepMuted[index] = 0;

                steps.probability[index] = stepProbabilityFromStateProperty (
                    rowTree.getProperty (probabilityPropName, defaultStepProbability),
                    stateVersion);
                const auto cycle = clampStepCycle (
                    static_cast<int> (rowTree.getProperty (cyclePropName, defaultStepCycle)));
                steps.cycle[index] = cycle;
                const auto storedMask = static_cast<int> (
                    rowTree.getProperty (cycleOffsetPropName, defaultStepCycleMask));

                if (stateVersion < 17)
                {
                    steps.cycleOffset[index] = clampStepCycleMask (
                        cycleMaskFromLegacyOffset (storedMask, cycle),
                        cycle);
                }
                else
                {
                    steps.cycleOffset[index] = clampStepCycleMask (storedMask, cycle);
                }

                if (steps.cycleOffset[index] == 0)
                    steps.cycleOffset[index] = defaultStepCycleMask;
            }

            pattern.sequencer.muted[static_cast<size_t> (row)] =
                static_cast<bool> (rowTree.getProperty ("muted", row != 0)) ? 1 : 0;
            pattern.sequencer.timingOffset[static_cast<size_t> (row)] = rowTimingOffsetIndexFromState (
                static_cast<int> (rowTree.getProperty ("timingOffset", defaultRowTimingOffsetIndex)),
                stateVersion);
            pattern.sequencer.midiChannel[static_cast<size_t> (row)] = juce::jlimit (
                minPhraseRowMidiChannel,
                maxPhraseRowMidiChannel,
                static_cast<int> (
                    rowTree.getProperty ("midiChannel", defaultPhraseRowMidiChannelForRow (row))));
            rebuildRowTimingLayout (steps);
        }

        pattern.loopBrace.enabled =
            static_cast<bool> (patternTree.getProperty ("loopBraceEnabled", false)) ? 1 : 0;
        const auto storedLoopStart = static_cast<double> (
            patternTree.getProperty ("loopBraceStart", defaultLoopBraceStartQuarters));
        const auto storedLoopEnd = static_cast<double> (
            patternTree.getProperty ("loopBraceEnd", defaultLoopBraceEndQuarters));
        pattern.loopBrace.startQuarters = clampLoopBraceStart (storedLoopStart, storedLoopEnd);
        pattern.loopBrace.endQuarters = clampLoopBraceEnd (storedLoopEnd, pattern.loopBrace.startQuarters);
        pattern.sequencer.combinationModeMask = clampCombinationModeMask (
            static_cast<int> (patternTree.getProperty ("combinationModeMask", 0)));
        pattern.scaleRoot = clampScaleRoot (
            static_cast<int> (patternTree.getProperty ("scaleRoot", defaultScaleRoot)));
        pattern.scaleModeIndex = clampScaleModeIndex (
            static_cast<int> (patternTree.getProperty ("scaleModeIndex", defaultScaleModeIndex)));
        const auto bandpassBounds = clampNoteBandpassBounds (
            static_cast<int> (patternTree.getProperty ("noteBandpassLowMidi",
                                                       defaultNoteBandpassLowMidi)),
            static_cast<int> (patternTree.getProperty ("noteBandpassHighMidi",
                                                       defaultNoteBandpassHighMidi)));
        pattern.noteBandpassLowMidi = bandpassBounds.first;
        pattern.noteBandpassHighMidi = bandpassBounds.second;
        pattern.velocityTiltPivotMidi = clampVelocityTiltPivotMidi (
            static_cast<int> (patternTree.getProperty ("velocityTiltPivotMidi",
                                                       defaultVelocityTiltPivotMidi)));
        pattern.velocityTiltAmount = clampVelocityTiltAmount (
            static_cast<int> (patternTree.getProperty ("velocityTiltAmount",
                                                       defaultVelocityTiltAmount)));
        pattern.globalTransposeSemitones = clampGlobalTransposeSemitones (
            static_cast<int> (patternTree.getProperty ("globalTransposeSemitones",
                                                       defaultGlobalTransposeSemitones)));
        pattern.octavizerDown8vaEnabled =
            static_cast<int> (patternTree.getProperty ("octavizerDown8vaEnabled", 0)) != 0 ? 1 : 0;
        pattern.octavizerUp8vaEnabled =
            static_cast<int> (patternTree.getProperty ("octavizerUp8vaEnabled", 0)) != 0 ? 1 : 0;
        pattern.octavizerDown8vaRelativeVelocity = clampOctavizerRelativeVelocity (
            static_cast<int> (patternTree.getProperty ("octavizerDown8vaRelativeVelocity",
                                                       defaultOctavizerRelativeVelocity)));
        pattern.octavizerUp8vaRelativeVelocity = clampOctavizerRelativeVelocity (
            static_cast<int> (patternTree.getProperty ("octavizerUp8vaRelativeVelocity",
                                                       defaultOctavizerRelativeVelocity)));
        pattern.shimmerEnabled =
            static_cast<int> (patternTree.getProperty ("shimmerEnabled", 0)) != 0 ? 1 : 0;
        pattern.shimmerDelayMultiplierIndex = clampShimmerDelayMultiplierIndex (
            static_cast<int> (patternTree.getProperty ("shimmerDelayMultiplierIndex",
                                                       defaultStepTimingMultiplierIndex)));
        pattern.shimmerFeedbackPercent = clampShimmerFeedbackPercent (
            static_cast<int> (patternTree.getProperty ("shimmerFeedbackPercent",
                                                       defaultShimmerFeedbackPercent)));
        pattern.shimmerMixPercent = clampShimmerMixPercent (
            static_cast<int> (patternTree.getProperty ("shimmerMixPercent",
                                                       defaultShimmerMixPercent)));
        pattern.seedingRhythmStep = clampSeedingRhythmStep (
            static_cast<int> (patternTree.getProperty ("seedingRhythmStep",
                                                       defaultSeedingRhythmStep)));

        for (int seedingRow = 0; seedingRow < phraseRowCount; ++seedingRow)
        {
            const auto prefix = "seedingRow" + juce::String (seedingRow);
            auto& rowState = pattern.seedingRows[static_cast<size_t> (seedingRow)];
            rowState.phraseLength = clampSeedingPhraseLength (
                static_cast<int> (patternTree.getProperty (prefix + "PhraseLength",
                                                           defaultSeedingPhraseLength)));
            rowState.centerMidi = clampSeedingCenterMidi (
                static_cast<int> (patternTree.getProperty (prefix + "CenterMidi",
                                                           defaultSeedingCenterMidi)));
            rowState.rangeSemitones = clampSeedingRangeSemitones (
                static_cast<int> (patternTree.getProperty (prefix + "RangeSemitones",
                                                           defaultSeedingRangeSemitones)));
            rowState.repetition = clampSeedingPercent (
                static_cast<int> (patternTree.getProperty (prefix + "Repetition",
                                                         defaultSeedingRepetition)));
            rowState.complexity = clampSeedingPercent (
                static_cast<int> (patternTree.getProperty (prefix + "Complexity",
                                                          defaultSeedingComplexity)));
            rowState.randomness = clampSeedingPercent (
                static_cast<int> (patternTree.getProperty (prefix + "Randomness",
                                                          defaultSeedingRandomness)));
            rowState.timingMeanMultiplierIndex = clampSeedingTimingMeanMultiplierIndex (
                static_cast<int> (patternTree.getProperty (
                    prefix + "TimingMeanMultiplierIndex",
                    defaultSeedingTimingMeanMultiplierIndex)));
            rowState.timingVariance = clampSeedingPercent (
                static_cast<int> (patternTree.getProperty (prefix + "TimingVariance",
                                                          defaultSeedingTimingVariance)));
            rowState.symmetry =
                static_cast<int> (patternTree.getProperty (prefix + "Symmetry", 0)) != 0 ? 1 : 0;
            rowState.seed = clampSeedingSeed (
                static_cast<int> (patternTree.getProperty (prefix + "Seed", defaultSeedingSeed)));
            rowState.repetitionSeed = clampSeedingAspectSeed (
                static_cast<int> (patternTree.getProperty (prefix + "RepetitionSeed", 0)));
            rowState.complexitySeed = clampSeedingAspectSeed (
                static_cast<int> (patternTree.getProperty (prefix + "ComplexitySeed", 0)));
            rowState.randomnessSeed = clampSeedingAspectSeed (
                static_cast<int> (patternTree.getProperty (prefix + "RandomnessSeed", 0)));
            rowState.timingVarianceSeed = clampSeedingAspectSeed (
                static_cast<int> (patternTree.getProperty (prefix + "TimingVarianceSeed", 0)));
            pattern.seedingRowTargets[static_cast<size_t> (seedingRow)] =
                static_cast<int> (patternTree.getProperty (prefix + "Targeted", seedingRow == 0 ? 1 : 0)) != 0 ? 1 : 0;
        }
    }

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto loopTree = state.getChild (i);

        if (! loopTree.hasType ("LoopSlot"))
            continue;

        const auto loopSlot = clampLoopSlot (static_cast<int> (loopTree.getProperty ("index", i)));
        auto& loopSlotState = loopSlots[static_cast<size_t> (loopSlot)];
        loopSlotState.assigned =
            static_cast<bool> (loopTree.getProperty ("assigned", false)) ? 1 : 0;
        loopSlotState.patternSlot = clampPatternSlot (
            static_cast<int> (loopTree.getProperty ("patternSlot", 0)));

        const auto storedLoopStart =
            static_cast<double> (loopTree.getProperty ("start", defaultLoopBraceStartQuarters));
        const auto storedLoopEnd =
            static_cast<double> (loopTree.getProperty ("end", defaultLoopBraceEndQuarters));
        loopSlotState.startQuarters = clampLoopBraceStart (storedLoopStart, storedLoopEnd);
        loopSlotState.endQuarters = clampLoopBraceEnd (storedLoopEnd, loopSlotState.startQuarters);
    }

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto rowTree = state.getChild (i);

        if (! rowTree.hasType ("Row"))
            continue;

        const auto row = static_cast<int> (rowTree.getProperty ("index", i));

        if (row < 0 || row >= phraseRowCount)
            continue;

        const auto stepCount = juce::jlimit (
            0,
            maxPhraseStepsPerRow,
            static_cast<int> (rowTree.getProperty ("stepCount", defaultPhraseStepsPerRow)));

        auto& steps = modelRow (row);
        const auto patternScaleRoot = modelPattern (getViewPatternSlot()).scaleRoot;
        initialiseRowDefaults (steps, row, stepCount, patternScaleRoot);

        for (int step = 0; step < stepCount; ++step)
        {
            const auto propName = "step" + juce::String (step);
            const auto durationPropName = "duration" + juce::String (step);
            const auto velocityPropName = "velocity" + juce::String (step);
            const auto stepMutedPropName = "stepMuted" + juce::String (step);
            const auto stepSkippedPropName = "stepSkipped" + juce::String (step);
            const auto probabilityPropName = "probability" + juce::String (step);
            const auto cyclePropName = "cycle" + juce::String (step);
            const auto cycleOffsetPropName = "cycleOffset" + juce::String (step);
            const auto timingMultiplierPropName = "timingMultiplier" + juce::String (step);
            const auto note = static_cast<int> (rowTree.getProperty (
                propName,
                defaultStepNoteForScaleRoot (patternScaleRoot)));
            steps.notes[static_cast<size_t> (step)] = juce::jlimit (0, 127, note);
            steps.timingMultiplier[static_cast<size_t> (step)] = stepTimingMultiplierIndexFromState (
                static_cast<int> (rowTree.getProperty (timingMultiplierPropName,
                                                       defaultStepTimingMultiplierIndex)),
                stateVersion);
            steps.durationFraction[static_cast<size_t> (step)] = durationFractionFromStateProperty (
                rowTree.getProperty (durationPropName, defaultStepDurationFraction),
                stateVersion);
            steps.velocity[static_cast<size_t> (step)] = juce::jlimit (
                0,
                127,
                static_cast<int> (rowTree.getProperty (velocityPropName, defaultStepVelocity)));
            steps.stepMuted[static_cast<size_t> (step)] =
                static_cast<bool> (rowTree.getProperty (stepMutedPropName, false)) ? 1 : 0;
            steps.stepSkipped[static_cast<size_t> (step)] =
                static_cast<bool> (rowTree.getProperty (stepSkippedPropName, false)) ? 1 : 0;

            if (steps.stepMuted[static_cast<size_t> (step)] != 0
                && steps.stepSkipped[static_cast<size_t> (step)] != 0)
                steps.stepMuted[static_cast<size_t> (step)] = 0;

            if (stateVersion >= 8)
            {
                steps.probability[static_cast<size_t> (step)] = stepProbabilityFromStateProperty (
                    rowTree.getProperty (probabilityPropName, PluginProcessor::defaultStepProbability),
                    stateVersion);
                const auto cycle = clampStepCycle (
                    static_cast<int> (rowTree.getProperty (cyclePropName, PluginProcessor::defaultStepCycle)));
                steps.cycle[static_cast<size_t> (step)] = cycle;
                const auto storedMask = static_cast<int> (
                    rowTree.getProperty (cycleOffsetPropName, PluginProcessor::defaultStepCycleMask));

                if (stateVersion < 17)
                {
                    steps.cycleOffset[static_cast<size_t> (step)] = clampStepCycleMask (
                        cycleMaskFromLegacyOffset (storedMask, cycle),
                        cycle);
                }
                else
                {
                    steps.cycleOffset[static_cast<size_t> (step)] =
                        clampStepCycleMask (storedMask, cycle);
                }

                if (steps.cycleOffset[static_cast<size_t> (step)] == 0)
                    steps.cycleOffset[static_cast<size_t> (step)] = defaultStepCycleMask;
            }
        }

        modelSequencer().muted[static_cast<size_t> (row)] =
            static_cast<bool> (rowTree.getProperty ("muted", false)) ? 1 : 0;
        modelSequencer().timingOffset[static_cast<size_t> (row)] = rowTimingOffsetIndexFromState (
            static_cast<int> (rowTree.getProperty ("timingOffset", defaultRowTimingOffsetIndex)),
            stateVersion);
        modelSequencer().midiChannel[static_cast<size_t> (row)] = juce::jlimit (
            minPhraseRowMidiChannel,
            maxPhraseRowMidiChannel,
            static_cast<int> (
                rowTree.getProperty ("midiChannel", defaultPhraseRowMidiChannelForRow (row))));
        rebuildRowTimingLayout (steps);
        publishRowToAudio (row);

        SequencerCommand mutedCommand;
        mutedCommand.type = SequencerCommand::Type::SetRowMuted;
        mutedCommand.row = row;
        mutedCommand.intValue = modelSequencer().muted[static_cast<size_t> (row)];
        publishCommandToAudio (mutedCommand);

        SequencerCommand timingCommand;
        timingCommand.type = SequencerCommand::Type::SetRowTimingOffset;
        timingCommand.row = row;
        timingCommand.intValue = modelSequencer().timingOffset[static_cast<size_t> (row)];
        publishCommandToAudio (timingCommand);

        SequencerCommand channelCommand;
        channelCommand.type = SequencerCommand::Type::SetRowMidiChannel;
        channelCommand.row = row;
        channelCommand.intValue = modelSequencer().midiChannel[static_cast<size_t> (row)];
        publishCommandToAudio (channelCommand);
    }

    setPulseIndex (static_cast<int> (state.getProperty ("pulseIndex", defaultPulseIndex)));
    setSwingPercent (static_cast<int> (state.getProperty ("swingPercent", defaultSwingPercent)));
    setVelocityHumanizePercent (
        static_cast<int> (state.getProperty ("velocityHumanizePercent", defaultVelocityHumanizePercent)));
    setTimingHumanizePercent (
        static_cast<int> (state.getProperty ("timingHumanizePercent", defaultTimingHumanizePercent)));
    setSwingSubdivisionIndex (
        static_cast<int> (state.getProperty ("swingSubdivisionIndex", defaultSwingSubdivisionIndex)));
    setRowColorsEnabled (static_cast<int> (state.getProperty ("rowColorsEnabled", 1)) != 0);
    setStandaloneTempoBpm (
        static_cast<double> (state.getProperty ("standaloneTempoBpm", 120.0)));

    const auto storedPatternSlot = static_cast<int> (state.getProperty ("currentPatternSlot", 0));
    lastViewPatternSlot = clampPatternSlot (
        static_cast<int> (state.getProperty ("lastViewPatternSlot", storedPatternSlot >= 0
                                                                  ? storedPatternSlot
                                                                  : 0)));

    const auto outputArmed = static_cast<int> (state.getProperty ("patternOutputArmed", 1)) != 0;

    if (storedPatternSlot < 0)
    {
        patternOutputArmed.store (0, std::memory_order_release);
        currentModelPatternSlot.store (-1, std::memory_order_release);
        currentLoopSlot.store (-1, std::memory_order_release);
        audioActiveLoopSlot.store (-1, std::memory_order_release);
        pendingAudioPatternSlot.store (-1, std::memory_order_release);
        pendingAudioLoopSlot.store (-1, std::memory_order_release);
        clearPendingAudioLoopBraceEnable();
        lastObservedParameterPatternSlot = lastViewPatternSlot;
    }
    else
    {
        const auto activePatternSlot = clampPatternSlot (storedPatternSlot);
        patternOutputArmed.store (outputArmed ? 1 : 0, std::memory_order_release);
        lastViewPatternSlot = activePatternSlot;
        currentModelPatternSlot.store (activePatternSlot, std::memory_order_release);
        lastObservedParameterPatternSlot = activePatternSlot;
    }

    if (state.hasProperty ("loopBraceStart") || state.hasProperty ("loopBraceEnd")
        || state.hasProperty ("loopBraceEnabled"))
    {
        const auto storedLoopStart = static_cast<double> (state.getProperty (
            "loopBraceStart",
            defaultLoopBraceStartQuarters));
        const auto storedLoopEnd = static_cast<double> (state.getProperty (
            "loopBraceEnd",
            defaultLoopBraceEndQuarters));
        auto& loop = modelLoopBrace();
        loop.startQuarters = clampLoopBraceStart (storedLoopStart, storedLoopEnd);
        loop.endQuarters = clampLoopBraceEnd (storedLoopEnd, loop.startQuarters);
        loop.enabled = static_cast<bool> (state.getProperty ("loopBraceEnabled", false)) ? 1 : 0;
    }

    for (int pattern = 0; pattern < patternSlotCount; ++pattern)
        publishPatternToAudio (pattern);

    const auto storedLoopSlot = static_cast<int> (state.getProperty ("currentLoopSlot", -1));
    auto restoredLoopSlot = false;

    if (storedPatternSlot >= 0
        && storedLoopSlot >= 0 && storedLoopSlot < loopSlotCount
        && loopSlots[static_cast<size_t> (storedLoopSlot)].assigned != 0)
    {
        const auto& loopSlotState = loopSlots[static_cast<size_t> (storedLoopSlot)];
        const auto activePatternSlot = clampPatternSlot (loopSlotState.patternSlot);

        const auto loopStart = clampLoopBraceStart (loopSlotState.startQuarters,
                                                    loopSlotState.endQuarters);
        const auto loopEnd = clampLoopBraceEnd (loopSlotState.endQuarters, loopStart);
        auto& loopBrace = modelPatterns[static_cast<size_t> (activePatternSlot)].loopBrace;
        loopBrace.enabled = 1;
        loopBrace.startQuarters = loopStart;
        loopBrace.endQuarters = loopEnd;

        currentModelPatternSlot.store (activePatternSlot, std::memory_order_release);
        currentLoopSlot.store (storedLoopSlot, std::memory_order_release);
        publishPatternToAudio (activePatternSlot);

        if (outputArmed)
            requestAudioLoopSlot (storedLoopSlot);

        restoredLoopSlot = true;
    }

    if (storedPatternSlot >= 0 && ! restoredLoopSlot && outputArmed)
        requestAudioPatternSlot (clampPatternSlot (storedPatternSlot));

    if (storedPatternSlot >= 0 && patternSlotParameter != nullptr
        && patternSlotParameter->get() != clampPatternSlot (storedPatternSlot) + 1)
    {
        patternSlotParameter->setValueNotifyingHost (
            patternSlotParameter->convertTo0to1 (clampPatternSlot (storedPatternSlot) + 1));
    }
}

void PluginProcessor::setProjectMetadata (const juce::String& name,
                                          const juce::String& description,
                                          const juce::String& createdAt,
                                          const juce::String& modifiedAt,
                                          const juce::String& themeMode,
                                          const int uiScalePercent,
                                          const bool stretchToFit)
{
    projectName = name.trim().isNotEmpty() ? name.trim() : juce::String ("Untitled Project");
    projectDescription = description.trim();
    projectCreatedAt = createdAt;
    projectModifiedAt = modifiedAt;
    projectThemeMode = themeMode == "light" || themeMode == "alt" ? themeMode : juce::String ("dark");
    projectUiScalePercent = juce::jlimit (50, 100, uiScalePercent);
    projectStretchStepsToFit = stretchToFit;
}

void PluginProcessor::setProjectUiScalePercent (const int uiScalePercent)
{
    projectUiScalePercent = juce::jlimit (50, 100, uiScalePercent);
}

void PluginProcessor::resetProject()
{
    for (auto& pattern : modelPatterns)
        initialisePatternDefaults (pattern);

    for (auto& loopSlot : loopSlots)
        loopSlot = {};

    pulseIndex.store (defaultPulseIndex, std::memory_order_relaxed);
    swingPercent.store (defaultSwingPercent, std::memory_order_relaxed);
    velocityHumanizePercent.store (defaultVelocityHumanizePercent, std::memory_order_relaxed);
    timingHumanizePercent.store (defaultTimingHumanizePercent, std::memory_order_relaxed);
    swingSubdivisionIndex.store (defaultSwingSubdivisionIndex, std::memory_order_relaxed);
    rowColorsEnabled.store (1, std::memory_order_relaxed);
    standaloneTempoBpm.store (120.0, std::memory_order_relaxed);
    standaloneTransportPlaying.store (0, std::memory_order_relaxed);
    standaloneTransportPpqPosition.store (0.0, std::memory_order_relaxed);
    standaloneTransportResetRequested.store (1, std::memory_order_release);

    projectName = "Untitled Project";
    projectDescription.clear();
    projectCreatedAt.clear();
    projectModifiedAt.clear();
    projectThemeMode = "dark";
    projectUiScalePercent = 100;
    projectStretchStepsToFit = false;

    lastViewPatternSlot = 0;
    currentModelPatternSlot.store (0, std::memory_order_release);
    patternOutputArmed.store (1, std::memory_order_release);
    currentLoopSlot.store (-1, std::memory_order_release);
    audioActiveLoopSlot.store (-1, std::memory_order_release);
    pendingAudioLoopSlot.store (-1, std::memory_order_release);
    clearPendingAudioLoopBraceEnable();

    for (int pattern = 0; pattern < patternSlotCount; ++pattern)
        publishPatternToAudio (pattern);

    requestAudioPatternSlot (0);

    if (patternSlotParameter != nullptr && patternSlotParameter->get() != 1)
        patternSlotParameter->setValueNotifyingHost (patternSlotParameter->convertTo0to1 (1));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
