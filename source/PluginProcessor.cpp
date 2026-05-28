#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int defaultRowNotes[] = { 60, 64, 67, 72 };
constexpr double rowTimingOffsetValues[] = { -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75 };
constexpr double stepTimingMultiplierValues[] = { 0.25, 0.5, 1.0, 2.0, 4.0 };
constexpr int phraseStateVersion = 3;
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
} // namespace

void PluginProcessor::resetPendingNoteOffs()
{
    for (auto& pending : pendingNoteOffs)
    {
        pending.note = -1;
        pending.samplesRemaining = 0;
    }
}

void PluginProcessor::resetLastEmittedTriggers()
{
    for (auto& lastTrigger : lastEmittedTriggerPpq)
        lastTrigger = -1.0;
}

void PluginProcessor::resetPhraseStepGateEnds()
{
    for (auto& rowSteps : phraseRows)
    {
        for (auto& gateStart : rowSteps.gateStartPpq)
            gateStart = -1.0;

        for (auto& gateEnd : rowSteps.gateEndPpq)
            gateEnd = -1.0;
    }
}

void PluginProcessor::syncRowGateStorage (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    const auto count = static_cast<size_t> (getPhraseRowStepCount (row));
    auto& steps = phraseRows[static_cast<size_t> (row)];
    steps.gateStartPpq.assign (count, -1.0);
    steps.gateEndPpq.assign (count, -1.0);
}

void PluginProcessor::syncProcessScratch (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    const auto count = static_cast<size_t> (getPhraseRowStepCount (row));
    auto& scratch = processScratch[static_cast<size_t> (row)];
    scratch.stepLengthQuarters.resize (count);
    scratch.stepStartQuarters.resize (count);
    scratch.triggers.resize (std::max (count * 8, size_t { 8 }));
}

void PluginProcessor::resetPhraseStepGateEndsForRow (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    auto& steps = phraseRows[static_cast<size_t> (row)];

    for (auto& gateStart : steps.gateStartPpq)
        gateStart = -1.0;

    for (auto& gateEnd : steps.gateEndPpq)
        gateEnd = -1.0;
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
    for (int row = 0; row < phraseRowCount; ++row)
    {
        auto& steps = phraseRows[static_cast<size_t> (row)];
        steps.notes.assign (defaultPhraseStepsPerRow, defaultNoteForRow (row));
        steps.timingMultiplier.assign (defaultPhraseStepsPerRow, defaultStepTimingMultiplierIndex);
        steps.durationFraction.assign (defaultPhraseStepsPerRow, defaultStepDurationFraction);
        steps.velocity.assign (defaultPhraseStepsPerRow, defaultStepVelocity);

        phraseRowMuted[static_cast<size_t> (row)].store (0);
        phraseRowTimingOffset[static_cast<size_t> (row)].store (defaultRowTimingOffsetIndex);
        phraseRowFlushNoteOff[static_cast<size_t> (row)].store (0);
        phraseRowStepCount[static_cast<size_t> (row)].store (defaultPhraseStepsPerRow);

        syncRowGateStorage (row);
        syncProcessScratch (row);
    }

    resetLastEmittedTriggers();
}

PluginProcessor::~PluginProcessor()
{
}

int PluginProcessor::defaultNoteForRow (int row)
{
    if (row < 0 || row >= phraseRowCount)
        return 60;

    return defaultRowNotes[row];
}

bool PluginProcessor::isValidStep (const int row, const int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return false;

    return step < getPhraseRowStepCount (row);
}

void PluginProcessor::resetPhraseStepToDefaults (const int row, const int step)
{
    if (! isValidStep (row, step))
        return;

    auto& steps = phraseRows[static_cast<size_t> (row)];
    steps.notes[static_cast<size_t> (step)] = defaultNoteForRow (row);
    steps.timingMultiplier[static_cast<size_t> (step)] = defaultStepTimingMultiplierIndex;
    steps.durationFraction[static_cast<size_t> (step)] = defaultStepDurationFraction;
    steps.velocity[static_cast<size_t> (step)] = defaultStepVelocity;
    steps.gateStartPpq[static_cast<size_t> (step)] = -1.0;
    steps.gateEndPpq[static_cast<size_t> (step)] = -1.0;
}

double PluginProcessor::rowTimingOffsetForIndex (const int offsetIndex)
{
    const auto index = juce::jlimit (0, rowTimingOffsetCount - 1, offsetIndex);
    return rowTimingOffsetValues[static_cast<size_t> (index)];
}

double PluginProcessor::stepTimingMultiplierForIndex (const int multiplierIndex)
{
    const auto index = juce::jlimit (0, stepTimingMultiplierCount - 1, multiplierIndex);
    return stepTimingMultiplierValues[static_cast<size_t> (index)];
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

    for (int row = 0; row < phraseRowCount; ++row)
        syncProcessScratch (row);
}

void PluginProcessor::setPhraseNote (int row, int step, int noteNumber)
{
    if (! isValidStep (row, step))
        return;

    phraseRows[static_cast<size_t> (row)].notes[static_cast<size_t> (step)] =
        juce::jlimit (0, 127, noteNumber);
}

int PluginProcessor::getPhraseNote (int row, int step) const
{
    if (! isValidStep (row, step))
        return 60;

    return phraseRows[static_cast<size_t> (row)].notes[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseRowMuted (int row, bool muted)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    phraseRowMuted[static_cast<size_t> (row)].store (muted ? 1 : 0);

    if (muted)
        phraseRowFlushNoteOff[static_cast<size_t> (row)].store (1);
}

bool PluginProcessor::isPhraseRowMuted (int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return false;

    return phraseRowMuted[static_cast<size_t> (row)].load() != 0;
}

void PluginProcessor::setPhraseRowTimingOffset (const int row, const int offsetIndex)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    phraseRowTimingOffset[static_cast<size_t> (row)].store (
        juce::jlimit (0, rowTimingOffsetCount - 1, offsetIndex));
}

int PluginProcessor::getPhraseRowTimingOffset (const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return defaultRowTimingOffsetIndex;

    return phraseRowTimingOffset[static_cast<size_t> (row)].load();
}

void PluginProcessor::setPhraseStepTimingMultiplier (const int row,
                                                    const int step,
                                                    const int multiplierIndex)
{
    if (! isValidStep (row, step))
        return;

    phraseRows[static_cast<size_t> (row)].timingMultiplier[static_cast<size_t> (step)] =
        juce::jlimit (0, stepTimingMultiplierCount - 1, multiplierIndex);
}

int PluginProcessor::getPhraseStepTimingMultiplier (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return defaultStepTimingMultiplierIndex;

    return phraseRows[static_cast<size_t> (row)].timingMultiplier[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseStepDurationFraction (const int row,
                                                   const int step,
                                                   const double fraction)
{
    if (! isValidStep (row, step))
        return;

    phraseRows[static_cast<size_t> (row)].durationFraction[static_cast<size_t> (step)] =
        clampStepDurationFraction (fraction);
}

double PluginProcessor::getPhraseStepDurationFraction (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return defaultStepDurationFraction;

    return phraseRows[static_cast<size_t> (row)].durationFraction[static_cast<size_t> (step)];
}

void PluginProcessor::setPhraseStepVelocity (const int row, const int step, const int velocity)
{
    if (! isValidStep (row, step))
        return;

    phraseRows[static_cast<size_t> (row)].velocity[static_cast<size_t> (step)] =
        juce::jlimit (0, 127, velocity);
}

int PluginProcessor::getPhraseStepVelocity (const int row, const int step) const
{
    if (! isValidStep (row, step))
        return defaultStepVelocity;

    return phraseRows[static_cast<size_t> (row)].velocity[static_cast<size_t> (step)];
}

int PluginProcessor::getPhraseRowStepCount (const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return 0;

    return juce::jmax (0, phraseRowStepCount[static_cast<size_t> (row)].load());
}

void PluginProcessor::removePhraseStep (const int row, const int step)
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return;

    const auto count = getPhraseRowStepCount (row);

    if (step >= count)
        return;

    auto& steps = phraseRows[static_cast<size_t> (row)];
    const auto index = static_cast<size_t> (step);

    steps.notes.erase (steps.notes.begin() + static_cast<std::ptrdiff_t> (index));
    steps.timingMultiplier.erase (steps.timingMultiplier.begin()
                                  + static_cast<std::ptrdiff_t> (index));
    steps.durationFraction.erase (steps.durationFraction.begin()
                                  + static_cast<std::ptrdiff_t> (index));
    steps.velocity.erase (steps.velocity.begin() + static_cast<std::ptrdiff_t> (index));

    phraseRowStepCount[static_cast<size_t> (row)].store (count - 1);
    syncRowGateStorage (row);
    syncProcessScratch (row);
    resetPhraseStepGateEndsForRow (row);
}

void PluginProcessor::insertPhraseStep (const int row, const int step)
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return;

    const auto count = getPhraseRowStepCount (row);

    if (step > count)
        return;

    auto& steps = phraseRows[static_cast<size_t> (row)];
    const auto index = static_cast<size_t> (step);

    steps.notes.insert (steps.notes.begin() + static_cast<std::ptrdiff_t> (index),
                        defaultNoteForRow (row));
    steps.timingMultiplier.insert (steps.timingMultiplier.begin()
                                   + static_cast<std::ptrdiff_t> (index),
                                   defaultStepTimingMultiplierIndex);
    steps.durationFraction.insert (steps.durationFraction.begin()
                                   + static_cast<std::ptrdiff_t> (index),
                                   defaultStepDurationFraction);
    steps.velocity.insert (steps.velocity.begin() + static_cast<std::ptrdiff_t> (index),
                           defaultStepVelocity);

    phraseRowStepCount[static_cast<size_t> (row)].store (count + 1);
    syncRowGateStorage (row);
    syncProcessScratch (row);
    resetPhraseStepGateEndsForRow (row);
}

void PluginProcessor::movePhraseStep (const int row, const int fromStep, const int toStep)
{
    if (row < 0 || row >= phraseRowCount || fromStep < 0 || toStep < 0)
        return;

    const auto count = getPhraseRowStepCount (row);

    if (fromStep >= count || toStep >= count || fromStep == toStep)
        return;

    auto& steps = phraseRows[static_cast<size_t> (row)];
    const auto note = steps.notes[static_cast<size_t> (fromStep)];
    const auto timingMultiplier = steps.timingMultiplier[static_cast<size_t> (fromStep)];
    const auto durationFraction = steps.durationFraction[static_cast<size_t> (fromStep)];
    const auto velocity = steps.velocity[static_cast<size_t> (fromStep)];

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
        }
    }

    steps.notes[static_cast<size_t> (toStep)] = note;
    steps.timingMultiplier[static_cast<size_t> (toStep)] = timingMultiplier;
    steps.durationFraction[static_cast<size_t> (toStep)] = durationFraction;
    steps.velocity[static_cast<size_t> (toStep)] = velocity;
    resetPhraseStepGateEndsForRow (row);
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

bool PluginProcessor::isPhraseStepActiveAtPlaybackBeat (const int row,
                                                      const int step,
                                                      const double playbackBeat) const
{
    constexpr auto epsilon = 1.0e-9;

    if (! isValidStep (row, step))
        return false;

    if (phraseRowMuted[static_cast<size_t> (row)].load (std::memory_order_relaxed) != 0)
        return false;

    const auto& rowSteps = phraseRows[static_cast<size_t> (row)];

    if (rowSteps.velocity[static_cast<size_t> (step)] <= 0)
        return false;

    const auto durationFraction = rowSteps.durationFraction[static_cast<size_t> (step)];

    if (durationFraction <= 0.0)
        return false;

    const auto stepCount = getPhraseRowStepCount (row);

    if (stepCount <= 0)
        return false;

    const auto offset = rowTimingOffsetForIndex (
        phraseRowTimingOffset[static_cast<size_t> (row)].load (std::memory_order_relaxed));

    auto cycleLengthQuarters = 0.0;
    auto stepStartInCycle = 0.0;
    auto stepLengthQuarters = 0.0;

    for (int index = 0; index < stepCount; ++index)
    {
        const auto length = stepTimingMultiplierForIndex (
            rowSteps.timingMultiplier[static_cast<size_t> (index)]);

        if (index == step)
        {
            stepStartInCycle = cycleLengthQuarters;
            stepLengthQuarters = length;
        }

        cycleLengthQuarters += length;
    }

    if (cycleLengthQuarters <= 0.0 || stepLengthQuarters <= 0.0)
        return false;

    const auto relativeBeat = playbackBeat - stepStartInCycle - offset;

    if (relativeBeat < -epsilon)
        return false;

    const auto cycleIndex = static_cast<int> (std::floor ((relativeBeat + epsilon) / cycleLengthQuarters));
    const auto triggerPpq = static_cast<double> (cycleIndex) * cycleLengthQuarters + stepStartInCycle
                            + offset;
    const auto gateEndPpq = triggerPpq + stepLengthQuarters * durationFraction;

    return playbackBeat >= triggerPpq - epsilon && playbackBeat < gateEndPpq - epsilon;
}

juce::Array<juce::var> PluginProcessor::getPhraseStepPlaybackActivity() const
{
    juce::Array<juce::var> rows;
    const auto playbackBeat = playbackBeatForUi();
    const auto isPlaying = playbackBeat >= 0.0;

    for (int row = 0; row < phraseRowCount; ++row)
    {
        juce::Array<juce::var> steps;

        const auto stepCount = getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            const auto active = isPlaying
                                && isPhraseStepActiveAtPlaybackBeat (row, step, playbackBeat);
            steps.add (active);
        }

        rows.add (steps);
    }

    return rows;
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
        resetPhraseStepGateEnds();

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

            midiMessages.addEvent (juce::MidiMessage::noteOff (1, pending.note), segmentSampleOffset);
            pending.note = -1;
            pending.samplesRemaining = 0;
        }
    }

    for (int row = 0; row < phraseRowCount; ++row)
    {
        if (phraseRowMuted[static_cast<size_t> (row)].load (std::memory_order_relaxed) != 0)
            continue;

        const auto offset = rowTimingOffsetForIndex (
            phraseRowTimingOffset[static_cast<size_t> (row)].load (std::memory_order_relaxed));

        const auto stepCount = getPhraseRowStepCount (row);

        if (stepCount <= 0)
            continue;

        auto& rowSteps = phraseRows[static_cast<size_t> (row)];
        auto& scratch = processScratch[static_cast<size_t> (row)];

        if (scratch.stepLengthQuarters.size() < static_cast<size_t> (stepCount)
            || scratch.stepStartQuarters.size() < static_cast<size_t> (stepCount))
            continue;

        auto cycleLengthQuarters = 0.0;

        for (int step = 0; step < stepCount; ++step)
        {
            scratch.stepStartQuarters[static_cast<size_t> (step)] = cycleLengthQuarters;
            scratch.stepLengthQuarters[static_cast<size_t> (step)] = stepTimingMultiplierForIndex (
                rowSteps.timingMultiplier[static_cast<size_t> (step)]);
            cycleLengthQuarters += scratch.stepLengthQuarters[static_cast<size_t> (step)];
        }

        if (cycleLengthQuarters <= 0.0)
            continue;

        auto triggerCount = 0;

        for (int step = 0; step < stepCount; ++step)
        {
            const auto stepStartInCycle = scratch.stepStartQuarters[static_cast<size_t> (step)];
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
            resetPhraseStepGateEndsForRow (row);
        }

        for (int triggerIndex = 0; triggerIndex < triggerCount; ++triggerIndex)
        {
            const auto triggerPpq = scratch.triggers[static_cast<size_t> (triggerIndex)].ppq;

            if (triggerPpq <= lastTrigger + epsilon)
                continue;

            lastTrigger = triggerPpq;

            const auto slot = scratch.triggers[static_cast<size_t> (triggerIndex)].step;
            const auto transportPpqAtTrigger =
                segmentTransportStartPpq + (triggerPpq - schedulePpqStart);
            const auto sampleOffset = juce::jlimit (
                0,
                bufferSamples - 1,
                static_cast<int> (std::lround (
                    (transportPpqAtTrigger - bufferTransportStartPpq) / ppqPerSample)));

            auto& pending = pendingNoteOffs[static_cast<size_t> (row)];

            if (pending.note >= 0)
            {
                midiMessages.addEvent (juce::MidiMessage::noteOff (1, pending.note), sampleOffset);
                pending.note = -1;
                pending.samplesRemaining = 0;
            }

            const auto note = rowSteps.notes[static_cast<size_t> (slot)];
            const auto velocity = rowSteps.velocity[static_cast<size_t> (slot)];

            if (velocity <= 0)
                continue;

            const auto stepLength = scratch.stepLengthQuarters[static_cast<size_t> (slot)];
            const auto durationFraction =
                rowSteps.durationFraction[static_cast<size_t> (slot)];

            if (durationFraction <= 0.0)
                continue;

            const auto gateQuarters = stepLength * durationFraction;

            if (gateQuarters <= epsilon)
                continue;

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

            midiMessages.addEvent (
                juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (velocity)),
                sampleOffset);

            rowSteps.gateStartPpq[static_cast<size_t> (slot)] = triggerPpq;
            rowSteps.gateEndPpq[static_cast<size_t> (slot)] = gateEndPpq;

            const auto samplesUntilOff = sampleOffset + noteGateSamples;

            if (samplesUntilOff < bufferSamples)
            {
                midiMessages.addEvent (juce::MidiMessage::noteOff (1, note), samplesUntilOff);
            }
            else
            {
                pending.note = note;
                pending.samplesRemaining = samplesUntilOff - bufferSamples;
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

    const auto* playHead = getPlayHead();

    if (playHead == nullptr)
        return;

    const auto position = playHead->getPosition();

    if (! position.hasValue() || ! position->getIsPlaying())
    {
        if (wasPlaying)
        {
            for (int ch = 1; ch <= 16; ++ch)
                midiMessages.addEvent (juce::MidiMessage::allNotesOff (ch), 0);
        }

        wasPlaying = false;
        resetLastEmittedTriggers();
        resetPendingNoteOffs();
        resetPhraseStepGateEnds();
        currentPlaybackPpq.store (-1.0, std::memory_order_relaxed);
        return;
    }

    wasPlaying = true;

    const auto bufferSamples = buffer.getNumSamples();

    for (int row = 0; row < phraseRowCount; ++row)
    {
        if (phraseRowFlushNoteOff[static_cast<size_t> (row)].exchange (0) == 0)
            continue;

        auto& pending = pendingNoteOffs[static_cast<size_t> (row)];

        if (pending.note >= 0)
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, pending.note), 0);
            pending.note = -1;
            pending.samplesRemaining = 0;
        }
    }

    for (auto& pending : pendingNoteOffs)
    {
        if (pending.note < 0)
            continue;

        if (pending.samplesRemaining < bufferSamples)
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, pending.note),
                                   pending.samplesRemaining);
            pending.note = -1;
            pending.samplesRemaining = 0;
        }
        else
        {
            pending.samplesRemaining -= bufferSamples;
        }
    }

    const auto ppqStart = position->getPpqPosition().orFallback (0.0);
    const auto bpm = position->getBpm().orFallback (120.0);
    const auto ppqPerSample = (bpm / 60.0) / sampleRateHz;
    const auto ppqEnd = ppqStart + static_cast<double> (bufferSamples) * ppqPerSample;
    currentPlaybackPpq.store (ppqEnd, std::memory_order_relaxed);

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
            const auto timingMultiplierPropName = "timingMultiplier" + juce::String (step);
            rowTree.setProperty (propName, getPhraseNote (row, step), nullptr);
            rowTree.setProperty (timingMultiplierPropName, getPhraseStepTimingMultiplier (row, step), nullptr);
            rowTree.setProperty (durationPropName, getPhraseStepDurationFraction (row, step), nullptr);
            rowTree.setProperty (velocityPropName, getPhraseStepVelocity (row, step), nullptr);
        }

        rowTree.setProperty ("muted", isPhraseRowMuted (row), nullptr);
        rowTree.setProperty ("timingOffset", getPhraseRowTimingOffset (row), nullptr);
        state.appendChild (rowTree, nullptr);
    }

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

        const auto stepCount = juce::jmax (
            0,
            static_cast<int> (rowTree.getProperty ("stepCount", defaultPhraseStepsPerRow)));

        auto& steps = phraseRows[static_cast<size_t> (row)];
        steps.notes.resize (static_cast<size_t> (stepCount));
        steps.timingMultiplier.resize (static_cast<size_t> (stepCount));
        steps.durationFraction.resize (static_cast<size_t> (stepCount));
        steps.velocity.resize (static_cast<size_t> (stepCount));

        for (int step = 0; step < stepCount; ++step)
        {
            const auto propName = "step" + juce::String (step);
            const auto durationPropName = "duration" + juce::String (step);
            const auto velocityPropName = "velocity" + juce::String (step);
            const auto timingMultiplierPropName = "timingMultiplier" + juce::String (step);
            const auto note = static_cast<int> (rowTree.getProperty (propName, defaultNoteForRow (row)));
            steps.notes[static_cast<size_t> (step)] = juce::jlimit (0, 127, note);
            steps.timingMultiplier[static_cast<size_t> (step)] = juce::jlimit (
                0,
                stepTimingMultiplierCount - 1,
                static_cast<int> (rowTree.getProperty (timingMultiplierPropName,
                                                         defaultStepTimingMultiplierIndex)));
            steps.durationFraction[static_cast<size_t> (step)] = durationFractionFromStateProperty (
                rowTree.getProperty (durationPropName, defaultStepDurationFraction),
                stateVersion);
            steps.velocity[static_cast<size_t> (step)] = juce::jlimit (
                0,
                127,
                static_cast<int> (rowTree.getProperty (velocityPropName, defaultStepVelocity)));
        }

        phraseRowStepCount[static_cast<size_t> (row)].store (stepCount);
        syncRowGateStorage (row);
        syncProcessScratch (row);

        setPhraseRowMuted (row, static_cast<bool> (rowTree.getProperty ("muted", false)));
        setPhraseRowTimingOffset (row,
                                  static_cast<int> (rowTree.getProperty ("timingOffset",
                                                                         defaultRowTimingOffsetIndex)));
    }

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
