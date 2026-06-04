#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace
{
constexpr double rowTimingOffsetValues[] = { -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75 };
constexpr double pulseQuartersTable[] = { 0.5, 1.0, 2.0, 4.0 };
constexpr double swingSubdivisionValues[] = { 0.25, 0.5, 1.0 };
constexpr double timingHumanizeScale = 0.2;
constexpr int phraseStateVersion = 12;

int clampStepProbability (const int probability)
{
    return juce::jlimit (0, 100, probability);
}

int clampStepCycle (const int cycle)
{
    return juce::jlimit (PluginProcessor::minStepCycle, PluginProcessor::maxStepCycle, cycle);
}

int clampStepCycleOffset (const int cycleOffset, const int cycle)
{
    return juce::jlimit (0, clampStepCycle (cycle) - 1, cycleOffset);
}

bool cycleGateMatches (const int count, const int cycle, const int cycleOffset)
{
    return count % cycle == cycleOffset;
}

float nextRandomUnit (std::uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    return static_cast<float> (state & 0x00FFFFFFu) / static_cast<float> (0x01000000u);
}

double nextRandomUnitDouble (std::uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;

    return static_cast<double> (state) / static_cast<double> (UINT32_MAX);
}

int clampPercent (const int percent)
{
    return juce::jlimit (0, 100, percent);
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
        pending.samplesRemaining = 0;
    }
}

void PluginProcessor::resetPendingNoteOns()
{
    pendingNoteOnCount = 0;
}

void PluginProcessor::resetActiveGeneratedNotes()
{
    for (auto& channelNotes : activeGeneratedNoteCounts)
        channelNotes.fill (0);
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

int PluginProcessor::defaultNoteForRow (int row)
{
    juce::ignoreUnused (row);
    return defaultStepNote;
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
    return modelPattern (getCurrentPatternSlot()).sequencer;
}

const PluginProcessor::SequencerState& PluginProcessor::modelSequencer() const
{
    return modelPattern (getCurrentPatternSlot()).sequencer;
}

const PluginProcessor::SequencerState& PluginProcessor::audioSequencer() const
{
    return audioPatterns[static_cast<size_t> (clampPatternSlot (audioActivePatternSlot))].sequencer;
}

PluginProcessor::LoopBraceState& PluginProcessor::modelLoopBrace()
{
    return modelPattern (getCurrentPatternSlot()).loopBrace;
}

const PluginProcessor::LoopBraceState& PluginProcessor::modelLoopBrace() const
{
    return modelPattern (getCurrentPatternSlot()).loopBrace;
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

void PluginProcessor::initialiseRowDefaults (PhraseRowSteps& steps, const int row, const int stepCount)
{
    steps.stepCount = juce::jlimit (0, maxPhraseStepsPerRow, stepCount);

    for (int step = 0; step < maxPhraseStepsPerRow; ++step)
    {
        steps.notes[static_cast<size_t> (step)] = defaultNoteForRow (row);
        steps.timingMultiplier[static_cast<size_t> (step)] = defaultStepTimingMultiplierIndex;
        steps.durationFraction[static_cast<size_t> (step)] = defaultStepDurationFraction;
        steps.velocity[static_cast<size_t> (step)] = defaultStepVelocity;
        steps.stepMuted[static_cast<size_t> (step)] = 0;
        steps.stepSkipped[static_cast<size_t> (step)] = 0;
        steps.probability[static_cast<size_t> (step)] = PluginProcessor::defaultStepProbability;
        steps.cycle[static_cast<size_t> (step)] = PluginProcessor::defaultStepCycle;
        steps.cycleOffset[static_cast<size_t> (step)] = PluginProcessor::PluginProcessor::defaultStepCycleOffset;
    }

    rebuildRowTimingLayout (steps);
}

void PluginProcessor::initialisePatternDefaults (PatternState& pattern)
{
    for (int row = 0; row < phraseRowCount; ++row)
    {
        initialiseRowDefaults (pattern.sequencer.rows[static_cast<size_t> (row)],
                               row,
                               defaultPhraseStepsPerRow);

        pattern.sequencer.muted[static_cast<size_t> (row)] = row == 0 ? 0 : 1;
        pattern.sequencer.timingOffset[static_cast<size_t> (row)] = defaultRowTimingOffsetIndex;
        pattern.sequencer.midiChannel[static_cast<size_t> (row)] = defaultPhraseRowMidiChannel;
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
        queuedCommand.patternSlot = getCurrentPatternSlot();

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
    command.patternSlot = getCurrentPatternSlot();
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
            && command.type != SequencerCommand::Type::SetLoopBraceEnd)
            return;
    }

    auto& row = state.rows[static_cast<size_t> (juce::jlimit (0, phraseRowCount - 1, command.row))];
    const auto step = command.step;
    const auto index = static_cast<size_t> (juce::jlimit (0, maxPhraseStepsPerRow - 1, step));

    switch (command.type)
    {
        case SequencerCommand::Type::SetNote:
            if (isValidAudioStep (state, command.row, step))
                row.notes[index] = command.intValue;
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
                    clampStepCycleOffset (row.cycleOffset[index], row.cycle[index]);
            }
            break;

        case SequencerCommand::Type::SetStepCycleOffset:
            if (isValidAudioStep (state, command.row, step))
                row.cycleOffset[index] =
                    clampStepCycleOffset (command.intValue, row.cycle[index]);
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

                row.notes[index] = defaultNoteForRow (command.row);
                row.timingMultiplier[index] = defaultStepTimingMultiplierIndex;
                row.durationFraction[index] = defaultStepDurationFraction;
                row.velocity[index] = defaultStepVelocity;
                row.stepMuted[index] = 0;
                row.stepSkipped[index] = 0;
                row.probability[index] = PluginProcessor::defaultStepProbability;
                row.cycle[index] = PluginProcessor::defaultStepCycle;
                row.cycleOffset[index] = PluginProcessor::defaultStepCycleOffset;
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
            pattern.loopBrace.enabled = command.intValue != 0 ? 1 : 0;
            break;

        case SequencerCommand::Type::SetLoopBraceStart:
            pattern.loopBrace.startQuarters =
                clampLoopBraceStart (command.doubleValue, pattern.loopBrace.endQuarters);
            break;

        case SequencerCommand::Type::SetLoopBraceEnd:
            pattern.loopBrace.endQuarters =
                clampLoopBraceEnd (command.doubleValue, pattern.loopBrace.startQuarters);
            break;

        case SequencerCommand::Type::ReplacePattern:
            pattern = command.patternState;
            for (auto& patternRow : pattern.sequencer.rows)
                rebuildRowTimingLayout (patternRow);
            resetStepCycleCounters();
            break;
    }

    if (patternSlot == audioActivePatternSlot
        && (command.type == SequencerCommand::Type::ReplacePattern
            || command.type == SequencerCommand::Type::ReplaceRow
            || command.type == SequencerCommand::Type::SetRowMuted
            || command.type == SequencerCommand::Type::SetRowMidiChannel))
    {
        for (auto& flush : phraseRowFlushNoteOff)
            flush.store (1);
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
    steps.cycleOffset[static_cast<size_t> (step)] = PluginProcessor::defaultStepCycleOffset;
    rebuildRowTimingLayout (steps);
    publishRowToAudio (row);
}

double PluginProcessor::rowTimingOffsetForIndex (const int offsetIndex)
{
    const auto index = juce::jlimit (0, rowTimingOffsetCount - 1, offsetIndex);
    return rowTimingOffsetValues[static_cast<size_t> (index)];
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
    resetLastEmittedTriggers();
    wasPlaying = false;
    resetPendingNoteOffs();
    resetPendingNoteOns();
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
        return defaultPhraseRowMidiChannel;

    return modelSequencer().midiChannel[static_cast<size_t> (row)];
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
    const auto previousOffset = steps.cycleOffset[index];
    steps.cycle[index] = value;
    steps.cycleOffset[index] = clampStepCycleOffset (previousOffset, value);

    SequencerCommand command;
    command.type = SequencerCommand::Type::SetStepCycle;
    command.row = row;
    command.step = step;
    command.intValue = value;
    publishCommandToAudio (command);

    if (steps.cycleOffset[index] != previousOffset)
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

void PluginProcessor::setPhraseStepCycleOffset (const int row, const int step, const int cycleOffset)
{
    if (! isValidStep (row, step))
        return;

    auto& steps = modelRow (row);
    const auto index = static_cast<size_t> (step);
    const auto value = clampStepCycleOffset (cycleOffset, steps.cycle[index]);
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
        return PluginProcessor::defaultStepCycleOffset;

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
    initialiseRowDefaults (steps, row, juce::jlimit (0, maxPhraseStepsPerRow, stepCount));

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
        steps.cycleOffset[index] = clampStepCycleOffset (cycleOffset[index], steps.cycle[index]);
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
    steps.cycleOffset[insertIndex] = PluginProcessor::defaultStepCycleOffset;
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
            return loop.startQuarters + positiveMod (currentPpq - loop.startQuarters, loopLength);
    }

    return currentPpq;
}

void PluginProcessor::setLoopBraceEnabled (const bool enabled)
{
    modelLoopBrace().enabled = enabled ? 1 : 0;
    publishLoopBraceCommandToAudio (SequencerCommand::Type::SetLoopBraceEnabled,
                                    getCurrentPatternSlot());
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
                                    getCurrentPatternSlot());
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
                                    getCurrentPatternSlot());
}

double PluginProcessor::getLoopBraceEndQuarters() const
{
    return modelLoopBrace().endQuarters;
}

void PluginProcessor::requestAudioPatternSlot (const int patternSlot)
{
    pendingAudioPatternSlot.store (clampPatternSlot (patternSlot), std::memory_order_release);
}

void PluginProcessor::applyAudioPatternSlot (const int patternSlot)
{
    const auto slot = clampPatternSlot (patternSlot);

    if (audioActivePatternSlot == slot)
    {
        pendingAudioPatternSlot.store (-1, std::memory_order_release);
        return;
    }

    audioActivePatternSlot = slot;
    pendingAudioPatternSlot.store (-1, std::memory_order_release);
    currentLoopSlot.store (-1, std::memory_order_release);
    resetLastEmittedTriggers();
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetStepCycleCounters();
}

void PluginProcessor::setCurrentPatternSlot (const int patternSlot)
{
    const auto slot = clampPatternSlot (patternSlot);
    currentModelPatternSlot.store (slot, std::memory_order_release);
    currentLoopSlot.store (-1, std::memory_order_release);
    requestAudioPatternSlot (slot);

    if (patternSlotParameter != nullptr && patternSlotParameter->get() != slot + 1)
        patternSlotParameter->setValueNotifyingHost (patternSlotParameter->convertTo0to1 (slot + 1));

    lastObservedParameterPatternSlot = slot;
}

int PluginProcessor::getCurrentPatternSlot() const
{
    return clampPatternSlot (currentModelPatternSlot.load (std::memory_order_acquire));
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

    if (slot == getCurrentPatternSlot())
        currentLoopSlot.store (-1, std::memory_order_release);
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
        return defaultStepNote;

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
        return defaultPhraseRowMidiChannel;

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

    return defaultStepCycleOffset;
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
    const auto patternSlot = getCurrentPatternSlot();
    const auto& loopBrace = modelPattern (patternSlot).loopBrace;
    auto& loopSlotState = loopSlots[static_cast<size_t> (slot)];

    loopSlotState.assigned = 1;
    loopSlotState.patternSlot = patternSlot;
    loopSlotState.startQuarters = loopBrace.startQuarters;
    loopSlotState.endQuarters = loopBrace.endQuarters;
}

void PluginProcessor::requestAudioLoopSlot (const int loopSlot)
{
    const auto slot = clampLoopSlot (loopSlot);

    if (loopSlots[static_cast<size_t> (slot)].assigned == 0)
        return;

    pendingAudioLoopSlot.store (slot, std::memory_order_release);
}

void PluginProcessor::applyAudioLoopSlot (const int loopSlot)
{
    const auto slot = clampLoopSlot (loopSlot);
    const auto& loopSlotState = loopSlots[static_cast<size_t> (slot)];

    if (loopSlotState.assigned == 0)
        return;

    audioActivePatternSlot = clampPatternSlot (loopSlotState.patternSlot);

    auto& loopBrace = audioPatterns[static_cast<size_t> (audioActivePatternSlot)].loopBrace;
    loopBrace.enabled = 1;
    loopBrace.startQuarters = clampLoopBraceStart (loopSlotState.startQuarters,
                                                   loopSlotState.endQuarters);
    loopBrace.endQuarters = clampLoopBraceEnd (loopSlotState.endQuarters,
                                               loopBrace.startQuarters);

    pendingAudioLoopSlot.store (-1, std::memory_order_release);
    currentLoopSlot.store (slot, std::memory_order_release);
    resetLastEmittedTriggers();
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetStepCycleCounters();
}

void PluginProcessor::selectLoopSlot (const int loopSlot)
{
    const auto slot = clampLoopSlot (loopSlot);
    const auto& loopSlotState = loopSlots[static_cast<size_t> (slot)];

    if (loopSlotState.assigned == 0)
        return;

    const auto patternSlot = clampPatternSlot (loopSlotState.patternSlot);
    const auto loopStart = clampLoopBraceStart (loopSlotState.startQuarters,
                                                loopSlotState.endQuarters);
    const auto loopEnd = clampLoopBraceEnd (loopSlotState.endQuarters, loopStart);

    currentModelPatternSlot.store (patternSlot, std::memory_order_release);

    auto& loopBrace = modelPatterns[static_cast<size_t> (patternSlot)].loopBrace;
    loopBrace.enabled = 1;
    loopBrace.startQuarters = loopStart;
    loopBrace.endQuarters = loopEnd;

    currentLoopSlot.store (slot, std::memory_order_release);
    publishPatternToAudio (patternSlot);

    if (patternSlotParameter != nullptr && patternSlotParameter->get() != patternSlot + 1)
        patternSlotParameter->setValueNotifyingHost (
            patternSlotParameter->convertTo0to1 (patternSlot + 1));

    lastObservedParameterPatternSlot = patternSlot;

    requestAudioLoopSlot (slot);
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

void PluginProcessor::emitScheduledNoteOn (const int row,
                                           const int midiChannel,
                                           const int note,
                                           const int velocity,
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
        pending.samplesRemaining = samplesUntilOff - bufferSamples;
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
            emitScheduledNoteOn (pending.row,
                                 pending.channel,
                                 pending.note,
                                 pending.velocity,
                                 pending.samplesRemaining,
                                 pending.gateSamples,
                                 bufferSamples,
                                 midiMessages);
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
    if (! hasStandaloneTransport())
        return;

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
    const auto write = recordQueueWrite.load (std::memory_order_relaxed);
    const auto read = recordQueueRead.load (std::memory_order_acquire);

    if (write - read >= recordQueueCapacity)
        return;

    recordQueueNotes[static_cast<size_t> (write % recordQueueCapacity)] =
        juce::jlimit (0, 127, midiNote);
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

    if (recordingAwaitingFirstNote.exchange (0) != 0)
    {
        initialiseRowDefaults (steps, row, 1);
        steps.notes[0] = juce::jlimit (0, 127, midiNote);
    }
    else if (steps.stepCount < maxPhraseStepsPerRow)
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
        steps.cycleOffset[index] = defaultStepCycleOffset;
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

    appendRecordedNoteToModelRow (row, midiNote);
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
                    currentModelPatternSlot.store (note, std::memory_order_release);
                    currentLoopSlot.store (-1, std::memory_order_release);
                    requestAudioPatternSlot (note);
                }
                continue;
            }

            if (note >= patternSlotCount && note < patternSlotCount + loopSlotCount)
            {
                if (message.isNoteOn())
                    requestAudioLoopSlot (note - patternSlotCount);
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
            const auto mappedStart = loopStart + positiveMod (transportCursor - loopStart, loopLength);
            const auto remainingInLoop = loopEnd - mappedStart;
            const auto segmentTransportEnd =
                juce::jmin (transportPpqEnd, transportCursor + remainingInLoop);
            const auto mappedEnd = mappedStart + (segmentTransportEnd - transportCursor);
            const auto wrappedToLoopStart = mappedStart <= loopStart + epsilon
                                            && transportCursor > transportPpqStart + epsilon;

            if (wrappedToLoopStart)
            {
                const auto pendingLoop = pendingAudioLoopSlot.exchange (-1, std::memory_order_acq_rel);

                if (pendingLoop >= 0)
                {
                    const auto loopSwitchSampleOffset = juce::jlimit (
                        0,
                        bufferSamples - 1,
                        static_cast<int> (std::lround (
                            (transportCursor - bufferTransportStartPpq) / ppqPerSample)));
                    flushPendingGeneratedNoteOffs (loopSwitchSampleOffset, midiMessages);
                    applyAudioLoopSlot (pendingLoop);
                    processTransportPlaybackRange (transportCursor,
                                                   transportPpqEnd,
                                                   bufferTransportStartPpq,
                                                   bufferSamples,
                                                   ppqPerSample,
                                                   midiMessages,
                                                   true);
                    return;
                }
            }

            processScheduledRange (mappedStart,
                                   mappedEnd,
                                   transportCursor,
                                   bufferTransportStartPpq,
                                   bufferSamples,
                                   ppqPerSample,
                                   midiMessages,
                                   resetRowTriggersAtSegmentStart
                                       || (wrappedToLoopStart && ! isFirstSegment));

            transportCursor = segmentTransportEnd;
            isFirstSegment = false;
        }
    }
    else
    {
        const auto pendingLoop = pendingAudioLoopSlot.exchange (-1, std::memory_order_acq_rel);

        if (pendingLoop >= 0)
        {
            const auto loopSwitchSampleOffset = juce::jlimit (
                0,
                bufferSamples - 1,
                static_cast<int> (std::lround (
                    (transportPpqStart - bufferTransportStartPpq) / ppqPerSample)));
            flushPendingGeneratedNoteOffs (loopSwitchSampleOffset, midiMessages);
            applyAudioLoopSlot (pendingLoop);
        }

        processScheduledRange (transportPpqStart,
                               transportPpqEnd,
                               transportPpqStart,
                               bufferTransportStartPpq,
                               bufferSamples,
                               ppqPerSample,
                               midiMessages,
                               resetRowTriggersAtSegmentStart);
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
    constexpr auto epsilon = 1.0e-9;

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

            emitGeneratedNoteOff (pending.channel, pending.note, segmentSampleOffset, midiMessages);
            pending.note = -1;
            pending.samplesRemaining = 0;
        }
    }

    for (int row = 0; row < phraseRowCount; ++row)
    {
        const auto& state = audioSequencer();

        if (state.muted[static_cast<size_t> (row)] != 0)
            continue;

        const auto midiChannel = state.midiChannel[static_cast<size_t> (row)];
        const auto pulse = pulseQuartersForIndex (pulseIndex.load (std::memory_order_relaxed));
        const auto swing = swingPercent.load (std::memory_order_relaxed);
        const auto velocityHumanize = velocityHumanizePercent.load (std::memory_order_relaxed);
        const auto timingHumanize = timingHumanizePercent.load (std::memory_order_relaxed);
        const auto swingSubdivision = swingSubdivisionIndex.load (std::memory_order_relaxed);
        const auto offset = rowTimingOffsetForIndex (state.timingOffset[static_cast<size_t> (row)])
                            * pulse;

        const auto stepCount = state.rows[static_cast<size_t> (row)].stepCount;

        if (stepCount <= 0)
            continue;

        const auto& rowSteps = state.rows[static_cast<size_t> (row)];
        auto& scratch = processScratch[static_cast<size_t> (row)];

        const auto cycleLengthQuarters = rowSteps.cycleLengthQuarters;

        if (cycleLengthQuarters <= 0.0)
            continue;

        auto triggerCount = 0;

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

        if (resetRowTriggersAtSegmentStart)
        {
            lastTrigger = schedulePpqStart - cycleLengthQuarters - 1.0;
        }
        else if (schedulePpqStart < lastTrigger - cycleLengthQuarters - epsilon)
        {
            lastTrigger = schedulePpqStart - cycleLengthQuarters - 1.0;
        }

        for (int triggerIndex = 0; triggerIndex < triggerCount; ++triggerIndex)
        {
            const auto triggerPpq = scratch.triggers[static_cast<size_t> (triggerIndex)].ppq;

            if (triggerPpq <= lastTrigger + epsilon)
                continue;

            lastTrigger = triggerPpq;

            const auto slot = scratch.triggers[static_cast<size_t> (triggerIndex)].step;

            const auto note = rowSteps.notes[static_cast<size_t> (slot)];
            auto velocity = rowSteps.velocity[static_cast<size_t> (slot)];

            if (velocity <= 0 || rowSteps.stepMuted[static_cast<size_t> (slot)] != 0)
                continue;

            const auto cycle = clampStepCycle (rowSteps.cycle[static_cast<size_t> (slot)]);
            const auto cycleOffset =
                clampStepCycleOffset (rowSteps.cycleOffset[static_cast<size_t> (slot)], cycle);

            if (cycle > 1)
            {
                auto& counter = stepCycleCounters[static_cast<size_t> (row)][static_cast<size_t> (slot)];
                const auto count = static_cast<int> (counter++);

                if (! cycleGateMatches (count, cycle, cycleOffset))
                    continue;
            }

            const auto probability =
                clampStepProbability (rowSteps.probability[static_cast<size_t> (slot)]);

            if (probability <= 0)
                continue;

            if (probability < 100
                && nextRandomUnit (playbackRandomState) * 100.0f
                       >= static_cast<float> (probability))
                continue;

            const auto stepLength = rowSteps.stepLengthQuarters[static_cast<size_t> (slot)];
            const auto durationFraction =
                rowSteps.durationFraction[static_cast<size_t> (slot)];

            if (durationFraction <= 0.0)
                continue;

            const auto gateQuarters = stepLength * durationFraction;

            if (gateQuarters <= epsilon)
                continue;

            velocity = humanizeVelocityValue (velocity, velocityHumanize, playbackRandomState);

            const auto swingDelay =
                swingDelayQuartersForPpq (triggerPpq, pulse, swing, swingSubdivision);
            const auto timingRange = stepLength * timingHumanizeScale
                                     * (static_cast<double> (clampPercent (timingHumanize)) / 100.0);
            const auto timingOffset =
                timingRange > 0.0 ? (nextRandomUnitDouble (playbackRandomState) * 2.0 - 1.0) * timingRange
                                  : 0.0;
            const auto delayQuarters = juce::jmax (0.0, swingDelay + timingOffset);
            const auto transportPpqAtNoteOn =
                segmentTransportStartPpq + (triggerPpq + delayQuarters - schedulePpqStart);
            const auto sampleOffset = static_cast<int> (std::lround (
                (transportPpqAtNoteOn - bufferTransportStartPpq) / ppqPerSample));
            const auto segmentTransportEndPpq =
                segmentTransportStartPpq + (schedulePpqEnd - schedulePpqStart);
            const auto bufferTransportEndPpq =
                bufferTransportStartPpq + static_cast<double> (bufferSamples) * ppqPerSample;
            const auto scheduleEndsBeforeBuffer =
                segmentTransportEndPpq < bufferTransportEndPpq - epsilon;

            const auto gateEndTransportPpq = scheduleEndsBeforeBuffer
                                                 ? juce::jmin (transportPpqAtNoteOn + gateQuarters,
                                                               segmentTransportEndPpq)
                                                 : transportPpqAtNoteOn + gateQuarters;
            const auto effectiveGateQuarters = gateEndTransportPpq - transportPpqAtNoteOn;

            if (effectiveGateQuarters <= epsilon)
                continue;

            const auto noteGateSamples = scheduleEndsBeforeBuffer
                                             ? juce::jmax (
                                                   1,
                                                   static_cast<int> (std::lround (
                                                       effectiveGateQuarters / ppqPerSample)))
                                             : juce::jmax (
                                                   1,
                                                   static_cast<int> (std::lround (
                                                       gateQuarters / ppqPerSample)));

            if (sampleOffset < bufferSamples)
            {
                emitScheduledNoteOn (row,
                                     midiChannel,
                                     note,
                                     velocity,
                                     juce::jmax (0, sampleOffset),
                                     noteGateSamples,
                                     bufferSamples,
                                     midiMessages);
            }
            else
            {
                addPendingNoteOn (PendingNoteOn { row,
                                                  midiChannel,
                                                  note,
                                                  velocity,
                                                  sampleOffset - bufferSamples,
                                                  noteGateSamples });
            }
        }
    }
}

void PluginProcessor::releaseResources()
{
    resetPendingNoteOffs();
    resetPendingNoteOns();
    resetActiveGeneratedNotes();
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
            currentModelPatternSlot.store (parameterPatternSlot, std::memory_order_release);
            currentLoopSlot.store (-1, std::memory_order_release);
            requestAudioPatternSlot (parameterPatternSlot);
        }
    }

    handleIncomingControlNotes (midiMessages);

    const auto stopPlayback = [&] {
        if (wasPlaying)
        {
            flushActiveGeneratedNotes (0, midiMessages);

            for (int ch = 1; ch <= 16; ++ch)
                midiMessages.addEvent (juce::MidiMessage::allNotesOff (ch), 0);
        }

        wasPlaying = false;
        resetLastEmittedTriggers();
        resetPendingNoteOffs();
        resetPendingNoteOns();
        currentPlaybackPpq.store (-1.0, std::memory_order_relaxed);
    };

    double ppqStart = 0.0;
    double bpm = 120.0;

    if (hasStandaloneTransport())
    {
        if (standaloneTransportResetRequested.exchange (0, std::memory_order_relaxed) != 0)
        {
            if (wasPlaying)
                flushActiveGeneratedNotes (0, midiMessages);

            wasPlaying = false;
            resetLastEmittedTriggers();
            resetPendingNoteOffs();
            resetPendingNoteOns();
            currentPlaybackPpq.store (-1.0, std::memory_order_relaxed);
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
            pending.samplesRemaining = 0;
        }

        size_t writeIndex = 0;

        for (size_t i = 0; i < pendingNoteOnCount; ++i)
        {
            if (pendingNoteOns[i].row != row)
                pendingNoteOns[writeIndex++] = pendingNoteOns[i];
        }

        pendingNoteOnCount = writeIndex;
    }

    flushPendingNoteOns (bufferSamples, midiMessages);

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

    constexpr auto epsilon = 1.0e-9;
    auto transportCursor = ppqStart;
    auto resetAtSegmentStart = false;

    const auto sampleOffsetForTransportPpq = [&] (const double transportPpq) {
        return juce::jlimit (
            0,
            bufferSamples - 1,
            static_cast<int> (std::lround ((transportPpq - ppqStart) / ppqPerSample)));
    };

    while (transportCursor < ppqEnd - epsilon)
    {
        auto segmentEnd = ppqEnd;
        auto segmentEndsAtPatternSwitch = false;
        const auto pendingPattern = pendingAudioPatternSlot.load (std::memory_order_acquire);

        if (pendingPattern >= 0)
        {
            const auto pulse = pulseQuartersForIndex (pulseIndex.load (std::memory_order_relaxed));
            const auto switchPpq = pulse > 0.0
                                       ? std::ceil ((transportCursor - epsilon) / pulse) * pulse
                                       : transportCursor;

            if (switchPpq <= transportCursor + epsilon)
            {
                flushPendingGeneratedNoteOffs (sampleOffsetForTransportPpq (transportCursor),
                                               midiMessages);
                applyAudioPatternSlot (pendingPattern);
                resetAtSegmentStart = true;
                continue;
            }

            if (switchPpq <= ppqEnd + epsilon)
            {
                segmentEnd = switchPpq;
                segmentEndsAtPatternSwitch = true;
            }
        }

        processTransportPlaybackRange (transportCursor,
                                       segmentEnd,
                                       ppqStart,
                                       bufferSamples,
                                       ppqPerSample,
                                       midiMessages,
                                       resetAtSegmentStart);

        if (segmentEndsAtPatternSwitch)
        {
            const auto nextPattern = pendingAudioPatternSlot.exchange (-1, std::memory_order_acq_rel);

            if (nextPattern >= 0)
            {
                flushPendingGeneratedNoteOffs (sampleOffsetForTransportPpq (segmentEnd),
                                               midiMessages);
                applyAudioPatternSlot (nextPattern);
            }

            if (segmentEnd >= ppqEnd - epsilon)
                break;

            transportCursor = segmentEnd;
            resetAtSegmentStart = true;
            continue;
        }

        if (segmentEnd >= ppqEnd - epsilon)
            break;

        const auto nextPattern = pendingAudioPatternSlot.exchange (-1, std::memory_order_acq_rel);

        if (nextPattern >= 0)
        {
            flushPendingGeneratedNoteOffs (sampleOffsetForTransportPpq (segmentEnd),
                                           midiMessages);
            applyAudioPatternSlot (nextPattern);
        }

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

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("MidiPhrases");
    state.setProperty ("version", phraseStateVersion, nullptr);
    state.setProperty ("currentPatternSlot", getCurrentPatternSlot(), nullptr);
    state.setProperty ("currentLoopSlot", getCurrentLoopSlot(), nullptr);
    state.setProperty ("pulseIndex", getPulseIndex(), nullptr);
    state.setProperty ("swingPercent", getSwingPercent(), nullptr);
    state.setProperty ("velocityHumanizePercent", getVelocityHumanizePercent(), nullptr);
    state.setProperty ("timingHumanizePercent", getTimingHumanizePercent(), nullptr);
    state.setProperty ("swingSubdivisionIndex", getSwingSubdivisionIndex(), nullptr);

    for (int patternSlot = 0; patternSlot < patternSlotCount; ++patternSlot)
    {
        juce::ValueTree patternTree ("Pattern");
        patternTree.setProperty ("index", patternSlot, nullptr);
        patternTree.setProperty ("loopBraceEnabled", isPatternLoopBraceEnabled (patternSlot), nullptr);
        patternTree.setProperty ("loopBraceStart", getPatternLoopBraceStartQuarters (patternSlot), nullptr);
        patternTree.setProperty ("loopBraceEnd", getPatternLoopBraceEndQuarters (patternSlot), nullptr);

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
            initialiseRowDefaults (steps, row, stepCount);

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
                    static_cast<int> (rowTree.getProperty (propName, defaultNoteForRow (row))));
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

                steps.probability[index] = clampStepProbability (static_cast<int> (
                    rowTree.getProperty (probabilityPropName, defaultStepProbability)));
                const auto cycle = clampStepCycle (
                    static_cast<int> (rowTree.getProperty (cyclePropName, defaultStepCycle)));
                steps.cycle[index] = cycle;
                steps.cycleOffset[index] = clampStepCycleOffset (
                    static_cast<int> (rowTree.getProperty (cycleOffsetPropName, defaultStepCycleOffset)),
                    cycle);
            }

            pattern.sequencer.muted[static_cast<size_t> (row)] =
                static_cast<bool> (rowTree.getProperty ("muted", row != 0)) ? 1 : 0;
            pattern.sequencer.timingOffset[static_cast<size_t> (row)] = juce::jlimit (
                0,
                rowTimingOffsetCount - 1,
                static_cast<int> (rowTree.getProperty ("timingOffset", defaultRowTimingOffsetIndex)));
            pattern.sequencer.midiChannel[static_cast<size_t> (row)] = juce::jlimit (
                minPhraseRowMidiChannel,
                maxPhraseRowMidiChannel,
                static_cast<int> (rowTree.getProperty ("midiChannel", defaultPhraseRowMidiChannel)));
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
        initialiseRowDefaults (steps, row, stepCount);

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
            const auto note = static_cast<int> (rowTree.getProperty (propName, defaultNoteForRow (row)));
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
                steps.probability[static_cast<size_t> (step)] = clampStepProbability (
                    static_cast<int> (rowTree.getProperty (probabilityPropName, PluginProcessor::defaultStepProbability)));
                const auto cycle = clampStepCycle (
                    static_cast<int> (rowTree.getProperty (cyclePropName, PluginProcessor::defaultStepCycle)));
                steps.cycle[static_cast<size_t> (step)] = cycle;
                steps.cycleOffset[static_cast<size_t> (step)] = clampStepCycleOffset (
                    static_cast<int> (rowTree.getProperty (cycleOffsetPropName, PluginProcessor::defaultStepCycleOffset)),
                    cycle);
            }
        }

        modelSequencer().muted[static_cast<size_t> (row)] =
            static_cast<bool> (rowTree.getProperty ("muted", false)) ? 1 : 0;
        modelSequencer().timingOffset[static_cast<size_t> (row)] = juce::jlimit (
            0,
            rowTimingOffsetCount - 1,
            static_cast<int> (rowTree.getProperty ("timingOffset", defaultRowTimingOffsetIndex)));
        modelSequencer().midiChannel[static_cast<size_t> (row)] = juce::jlimit (
            minPhraseRowMidiChannel,
            maxPhraseRowMidiChannel,
            static_cast<int> (rowTree.getProperty ("midiChannel", defaultPhraseRowMidiChannel)));
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

    auto activePatternSlot = clampPatternSlot (
        static_cast<int> (state.getProperty ("currentPatternSlot", 0)));
    currentModelPatternSlot.store (activePatternSlot, std::memory_order_release);
    lastObservedParameterPatternSlot = activePatternSlot;

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

    if (storedLoopSlot >= 0 && storedLoopSlot < loopSlotCount
        && loopSlots[static_cast<size_t> (storedLoopSlot)].assigned != 0)
    {
        const auto& loopSlotState = loopSlots[static_cast<size_t> (storedLoopSlot)];
        activePatternSlot = clampPatternSlot (loopSlotState.patternSlot);

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
        requestAudioLoopSlot (storedLoopSlot);
        restoredLoopSlot = true;
    }

    if (! restoredLoopSlot)
        requestAudioPatternSlot (activePatternSlot);

    lastObservedParameterPatternSlot = activePatternSlot;

    if (patternSlotParameter != nullptr && patternSlotParameter->get() != activePatternSlot + 1)
        patternSlotParameter->setValueNotifyingHost (
            patternSlotParameter->convertTo0to1 (activePatternSlot + 1));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
