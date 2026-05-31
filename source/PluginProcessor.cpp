#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double rowTimingOffsetValues[] = { -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75 };
constexpr double pulseQuartersTable[] = { 0.5, 1.0, 2.0, 4.0 };
constexpr double swingSubdivisionValues[] = { 0.25, 0.5, 1.0 };
constexpr double timingHumanizeScale = 0.2;
constexpr int phraseStateVersion = 9;

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

int clampLoopBraceStart (const int startQuarters, const int endQuarters)
{
    return juce::jmax (0, juce::jmin (startQuarters, endQuarters - 1));
}

int clampLoopBraceEnd (const int endQuarters, const int startQuarters)
{
    return juce::jmax (startQuarters + 1, endQuarters);
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
    sequencerCommandQueue =
        std::make_unique<std::array<SequencerCommand, sequencerCommandQueueCapacity>>();

    for (int row = 0; row < phraseRowCount; ++row)
    {
        initialiseRowDefaults (modelState.rows[static_cast<size_t> (row)], row, defaultPhraseStepsPerRow);
        initialiseRowDefaults (audioState.rows[static_cast<size_t> (row)], row, defaultPhraseStepsPerRow);
        modelState.muted[static_cast<size_t> (row)] = 0;
        modelState.timingOffset[static_cast<size_t> (row)] = defaultRowTimingOffsetIndex;
        modelState.midiChannel[static_cast<size_t> (row)] = defaultPhraseRowMidiChannel;
        audioState.muted[static_cast<size_t> (row)] = 0;
        audioState.timingOffset[static_cast<size_t> (row)] = defaultRowTimingOffsetIndex;
        audioState.midiChannel[static_cast<size_t> (row)] = defaultPhraseRowMidiChannel;
        phraseRowFlushNoteOff[static_cast<size_t> (row)].store (0);
    }

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

PluginProcessor::PhraseRowSteps& PluginProcessor::modelRow (const int row)
{
    return modelState.rows[static_cast<size_t> (row)];
}

const PluginProcessor::PhraseRowSteps& PluginProcessor::modelRow (const int row) const
{
    return modelState.rows[static_cast<size_t> (row)];
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
    const auto write = sequencerCommandWriteIndex.load (std::memory_order_relaxed);
    const auto nextWrite = (write + 1) % sequencerCommandQueueCapacity;

    if (nextWrite == sequencerCommandReadIndex.load (std::memory_order_acquire))
    {
        jassertfalse;
        return;
    }

    (*sequencerCommandQueue)[write] = command;
    sequencerCommandWriteIndex.store (nextWrite, std::memory_order_release);
}

void PluginProcessor::publishRowToAudio (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    SequencerCommand command;
    command.type = SequencerCommand::Type::ReplaceRow;
    command.row = row;
    command.rowState = modelRow (row);
    publishCommandToAudio (command);
}

void PluginProcessor::applySequencerCommand (const SequencerCommand& command)
{
    if (command.row < 0 || command.row >= phraseRowCount)
        return;

    auto& row = audioState.rows[static_cast<size_t> (command.row)];
    const auto step = command.step;
    const auto index = static_cast<size_t> (juce::jlimit (0, maxPhraseStepsPerRow - 1, step));

    switch (command.type)
    {
        case SequencerCommand::Type::SetNote:
            if (isValidAudioStep (audioState, command.row, step))
                row.notes[index] = command.intValue;
            break;

        case SequencerCommand::Type::SetRowMuted:
            audioState.muted[static_cast<size_t> (command.row)] = command.intValue != 0 ? 1 : 0;
            break;

        case SequencerCommand::Type::SetRowTimingOffset:
            audioState.timingOffset[static_cast<size_t> (command.row)] =
                juce::jlimit (0, rowTimingOffsetCount - 1, command.intValue);
            break;

        case SequencerCommand::Type::SetRowMidiChannel:
            audioState.midiChannel[static_cast<size_t> (command.row)] =
                juce::jlimit (minPhraseRowMidiChannel, maxPhraseRowMidiChannel, command.intValue);
            break;

        case SequencerCommand::Type::SetStepTimingMultiplier:
            if (isValidAudioStep (audioState, command.row, step))
            {
                row.timingMultiplier[index] =
                    juce::jlimit (0, stepTimingMultiplierCount - 1, command.intValue);
                rebuildRowTimingLayout (row);
            }
            break;

        case SequencerCommand::Type::SetStepDurationFraction:
            if (isValidAudioStep (audioState, command.row, step))
                row.durationFraction[index] = clampStepDurationFraction (command.doubleValue);
            break;

        case SequencerCommand::Type::SetStepVelocity:
            if (isValidAudioStep (audioState, command.row, step))
                row.velocity[index] = juce::jlimit (0, 127, command.intValue);
            break;

        case SequencerCommand::Type::SetStepMuted:
            if (isValidAudioStep (audioState, command.row, step))
            {
                row.stepMuted[index] = command.intValue != 0 ? 1 : 0;

                if (command.intValue != 0)
                    row.stepSkipped[index] = 0;
            }
            break;

        case SequencerCommand::Type::SetStepSkipped:
            if (isValidAudioStep (audioState, command.row, step))
            {
                if (command.intValue != 0)
                    row.stepMuted[index] = 0;

                row.stepSkipped[index] = command.intValue != 0 ? 1 : 0;
                rebuildRowTimingLayout (row);
            }
            break;

        case SequencerCommand::Type::SetStepProbability:
            if (isValidAudioStep (audioState, command.row, step))
                row.probability[index] = command.intValue;
            break;

        case SequencerCommand::Type::SetStepCycle:
            if (isValidAudioStep (audioState, command.row, step))
            {
                row.cycle[index] = command.intValue;
                row.cycleOffset[index] =
                    clampStepCycleOffset (row.cycleOffset[index], row.cycle[index]);
            }
            break;

        case SequencerCommand::Type::SetStepCycleOffset:
            if (isValidAudioStep (audioState, command.row, step))
                row.cycleOffset[index] =
                    clampStepCycleOffset (command.intValue, row.cycle[index]);
            break;

        case SequencerCommand::Type::RemoveStep:
            if (isValidAudioStep (audioState, command.row, step))
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
            if (isValidAudioStep (audioState, command.row, command.step)
                && isValidAudioStep (audioState, command.row, command.toStep)
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

    for (int row = 0; row < phraseRowCount; ++row)
    {
        rebuildRowTimingLayout (modelRow (row));
        publishRowToAudio (row);
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

    modelState.muted[static_cast<size_t> (row)] = muted ? 1 : 0;

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

    return modelState.muted[static_cast<size_t> (row)] != 0;
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

void PluginProcessor::setPhraseRowTimingOffset (const int row, const int offsetIndex)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    const auto value = juce::jlimit (0, rowTimingOffsetCount - 1, offsetIndex);
    modelState.timingOffset[static_cast<size_t> (row)] = value;

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

    return modelState.timingOffset[static_cast<size_t> (row)];
}

void PluginProcessor::setPhraseRowMidiChannel (const int row, const int channel)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    const auto value =
        juce::jlimit (minPhraseRowMidiChannel, maxPhraseRowMidiChannel, channel);
    modelState.midiChannel[static_cast<size_t> (row)] = value;
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

    return modelState.midiChannel[static_cast<size_t> (row)];
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

    if (loopBraceEnabled.load (std::memory_order_relaxed) != 0)
    {
        const auto loopStart = static_cast<double> (loopBraceStartQuarters.load (std::memory_order_relaxed));
        const auto loopEnd = static_cast<double> (loopBraceEndQuarters.load (std::memory_order_relaxed));
        const auto loopLength = loopEnd - loopStart;

        if (loopLength > 0.0)
            return loopStart + positiveMod (currentPpq - loopStart, loopLength);
    }

    return currentPpq;
}

void PluginProcessor::setLoopBraceEnabled (const bool enabled)
{
    loopBraceEnabled.store (enabled ? 1 : 0);
}

bool PluginProcessor::isLoopBraceEnabled() const
{
    return loopBraceEnabled.load() != 0;
}

void PluginProcessor::setLoopBraceStartQuarters (const int startQuarters)
{
    const auto end = loopBraceEndQuarters.load();
    loopBraceStartQuarters.store (clampLoopBraceStart (startQuarters, end));
}

int PluginProcessor::getLoopBraceStartQuarters() const
{
    return loopBraceStartQuarters.load();
}

void PluginProcessor::setLoopBraceEndQuarters (const int endQuarters)
{
    const auto start = loopBraceStartQuarters.load();
    loopBraceEndQuarters.store (clampLoopBraceEnd (endQuarters, start));
}

int PluginProcessor::getLoopBraceEndQuarters() const
{
    return loopBraceEndQuarters.load();
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
        midiMessages.addEvent (
            juce::MidiMessage::noteOff (pending.channel, pending.note),
            juce::jlimit (0, bufferSamples - 1, juce::jmin (pending.samplesRemaining, sampleOffset)));
        pending.note = -1;
        pending.samplesRemaining = 0;
    }

    midiMessages.addEvent (
        juce::MidiMessage::noteOn (midiChannel, note, static_cast<juce::uint8> (velocity)),
        sampleOffset);

    const auto samplesUntilOff = sampleOffset + gateSamples;

    if (samplesUntilOff < bufferSamples)
    {
        midiMessages.addEvent (juce::MidiMessage::noteOff (midiChannel, note), samplesUntilOff);
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

            midiMessages.addEvent (
                juce::MidiMessage::noteOff (pending.channel, pending.note),
                segmentSampleOffset);
            pending.note = -1;
            pending.samplesRemaining = 0;
        }
    }

    for (int row = 0; row < phraseRowCount; ++row)
    {
        if (audioState.muted[static_cast<size_t> (row)] != 0)
            continue;

        const auto midiChannel = audioState.midiChannel[static_cast<size_t> (row)];
        const auto pulse = pulseQuartersForIndex (pulseIndex.load (std::memory_order_relaxed));
        const auto swing = swingPercent.load (std::memory_order_relaxed);
        const auto velocityHumanize = velocityHumanizePercent.load (std::memory_order_relaxed);
        const auto timingHumanize = timingHumanizePercent.load (std::memory_order_relaxed);
        const auto swingSubdivision = swingSubdivisionIndex.load (std::memory_order_relaxed);
        const auto offset = rowTimingOffsetForIndex (audioState.timingOffset[static_cast<size_t> (row)])
                            * pulse;

        const auto stepCount = audioState.rows[static_cast<size_t> (row)].stepCount;

        if (stepCount <= 0)
            continue;

        const auto& rowSteps = audioState.rows[static_cast<size_t> (row)];
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

                if (triggerPpq < schedulePpqStart - epsilon || triggerPpq >= schedulePpqEnd + epsilon)
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
            const auto scheduleSpanQuarters = schedulePpqEnd - schedulePpqStart;
            const auto bufferSpanQuarters = static_cast<double> (bufferSamples) * ppqPerSample;
            const auto scheduleEndsBeforeBuffer =
                scheduleSpanQuarters < bufferSpanQuarters - epsilon;

            const auto gateEndPpq = scheduleEndsBeforeBuffer
                                        ? juce::jmin (triggerPpq + gateQuarters, schedulePpqEnd)
                                        : triggerPpq + gateQuarters;
            const auto effectiveGateQuarters = gateEndPpq - triggerPpq;

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

    const auto stopPlayback = [&] {
        if (wasPlaying)
        {
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
            midiMessages.addEvent (juce::MidiMessage::noteOff (pending.channel, pending.note), 0);
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
            midiMessages.addEvent (juce::MidiMessage::noteOff (pending.channel, pending.note),
                                   pending.samplesRemaining);
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

    const auto loopEnabled = loopBraceEnabled.load (std::memory_order_relaxed) != 0;
    const auto loopStart = static_cast<double> (loopBraceStartQuarters.load (std::memory_order_relaxed));
    const auto loopEnd = static_cast<double> (loopBraceEndQuarters.load (std::memory_order_relaxed));
    const auto loopLength = loopEnd - loopStart;

    if (loopEnabled && loopLength > 0.0)
    {
        constexpr auto epsilon = 1.0e-9;
        auto transportCursor = ppqStart;
        auto isFirstSegment = true;

        while (transportCursor < ppqEnd - epsilon)
        {
            const auto mappedStart = loopStart + positiveMod (transportCursor - loopStart, loopLength);
            const auto remainingInLoop = loopEnd - mappedStart;
            const auto segmentTransportEnd = juce::jmin (ppqEnd, transportCursor + remainingInLoop);
            const auto mappedEnd = mappedStart + (segmentTransportEnd - transportCursor);
            const auto wrappedToLoopStart = mappedStart <= loopStart + epsilon
                                            && transportCursor > ppqStart + epsilon;

            processScheduledRange (mappedStart,
                                   mappedEnd,
                                   transportCursor,
                                   ppqStart,
                                   bufferSamples,
                                   ppqPerSample,
                                   midiMessages,
                                   wrappedToLoopStart && ! isFirstSegment);

            transportCursor = segmentTransportEnd;
            isFirstSegment = false;
        }
    }
    else
    {
        processScheduledRange (ppqStart,
                               ppqEnd,
                               ppqStart,
                               ppqStart,
                               bufferSamples,
                               ppqPerSample,
                               midiMessages,
                               false);
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

    for (int row = 0; row < phraseRowCount; ++row)
    {
        juce::ValueTree rowTree ("Row");
        rowTree.setProperty ("index", row, nullptr);

        const auto stepCount = getPhraseRowStepCount (row);
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
            rowTree.setProperty (propName, getPhraseNote (row, step), nullptr);
            rowTree.setProperty (timingMultiplierPropName, getPhraseStepTimingMultiplier (row, step), nullptr);
            rowTree.setProperty (durationPropName, getPhraseStepDurationFraction (row, step), nullptr);
            rowTree.setProperty (velocityPropName, getPhraseStepVelocity (row, step), nullptr);
            rowTree.setProperty (stepMutedPropName, isPhraseStepMuted (row, step), nullptr);
            rowTree.setProperty (stepSkippedPropName, isPhraseStepSkipped (row, step), nullptr);
            rowTree.setProperty (probabilityPropName, getPhraseStepProbability (row, step), nullptr);
            rowTree.setProperty (cyclePropName, getPhraseStepCycle (row, step), nullptr);
            rowTree.setProperty (cycleOffsetPropName, getPhraseStepCycleOffset (row, step), nullptr);
        }

        rowTree.setProperty ("muted", isPhraseRowMuted (row), nullptr);
        rowTree.setProperty ("timingOffset", getPhraseRowTimingOffset (row), nullptr);
        rowTree.setProperty ("midiChannel", getPhraseRowMidiChannel (row), nullptr);
        state.appendChild (rowTree, nullptr);
    }

    state.setProperty ("pulseIndex", getPulseIndex(), nullptr);
    state.setProperty ("swingPercent", getSwingPercent(), nullptr);
    state.setProperty ("velocityHumanizePercent", getVelocityHumanizePercent(), nullptr);
    state.setProperty ("timingHumanizePercent", getTimingHumanizePercent(), nullptr);
    state.setProperty ("swingSubdivisionIndex", getSwingSubdivisionIndex(), nullptr);
    state.setProperty ("loopBraceEnabled", isLoopBraceEnabled(), nullptr);
    state.setProperty ("loopBraceStart", getLoopBraceStartQuarters(), nullptr);
    state.setProperty ("loopBraceEnd", getLoopBraceEndQuarters(), nullptr);

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

        modelState.muted[static_cast<size_t> (row)] =
            static_cast<bool> (rowTree.getProperty ("muted", false)) ? 1 : 0;
        modelState.timingOffset[static_cast<size_t> (row)] = juce::jlimit (
            0,
            rowTimingOffsetCount - 1,
            static_cast<int> (rowTree.getProperty ("timingOffset", defaultRowTimingOffsetIndex)));
        modelState.midiChannel[static_cast<size_t> (row)] = juce::jlimit (
            minPhraseRowMidiChannel,
            maxPhraseRowMidiChannel,
            static_cast<int> (rowTree.getProperty ("midiChannel", defaultPhraseRowMidiChannel)));
        rebuildRowTimingLayout (steps);
        publishRowToAudio (row);

        SequencerCommand mutedCommand;
        mutedCommand.type = SequencerCommand::Type::SetRowMuted;
        mutedCommand.row = row;
        mutedCommand.intValue = modelState.muted[static_cast<size_t> (row)];
        publishCommandToAudio (mutedCommand);

        SequencerCommand timingCommand;
        timingCommand.type = SequencerCommand::Type::SetRowTimingOffset;
        timingCommand.row = row;
        timingCommand.intValue = modelState.timingOffset[static_cast<size_t> (row)];
        publishCommandToAudio (timingCommand);

        SequencerCommand channelCommand;
        channelCommand.type = SequencerCommand::Type::SetRowMidiChannel;
        channelCommand.row = row;
        channelCommand.intValue = modelState.midiChannel[static_cast<size_t> (row)];
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

    setLoopBraceStartQuarters (static_cast<int> (state.getProperty ("loopBraceStart",
                                                                     defaultLoopBraceStartQuarters)));
    setLoopBraceEndQuarters (static_cast<int> (state.getProperty ("loopBraceEnd",
                                                                  defaultLoopBraceEndQuarters)));
    setLoopBraceEnabled (static_cast<bool> (state.getProperty ("loopBraceEnabled", false)));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
