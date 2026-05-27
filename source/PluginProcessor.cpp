#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int defaultRowNotes[] = { 60, 64, 67, 72 };
constexpr double rowTimingOffsetValues[] = { -0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75 };
constexpr double stepTimingMultiplierValues[] = { 0.25, 0.5, 1.0, 2.0, 4.0 };
constexpr double stepDurationFractionValues[] = { 0.25, 0.5, 0.75, 0.99 };
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
    for (auto& rowGates : phraseStepGateStartPpq)
    {
        for (auto& gateStart : rowGates)
            gateStart.store (-1.0, std::memory_order_relaxed);
    }

    for (auto& rowGates : phraseStepGateEndPpq)
    {
        for (auto& gateEnd : rowGates)
            gateEnd.store (-1.0, std::memory_order_relaxed);
    }
}

void PluginProcessor::resetPhraseStepGateEndsForRow (const int row)
{
    if (row < 0 || row >= phraseRowCount)
        return;

    for (int step = 0; step < phraseStepCount; ++step)
    {
        phraseStepGateStartPpq[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
            -1.0,
            std::memory_order_relaxed);
        phraseStepGateEndPpq[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
            -1.0,
            std::memory_order_relaxed);
    }
}

PluginProcessor::BusesProperties PluginProcessor::createBusesProperties()
{
  #if JucePlugin_IsMidiEffect
    // Dummy stereo out only (silent). Logic AU stays in the MIDI FX slot (aumi).
    // Ableton: stereo *in* classifies the VST3 as an audio effect (right of the instrument);
    // out-only matches JUCE's MidiLogger demo and loads in the instrument area instead.
    return BusesProperties().withOutput ("Out", juce::AudioChannelSet::stereo(), true);
  #else
    return BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                            .withOutput ("Output", juce::AudioChannelSet::stereo(), true);
  #endif
}

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (createBusesProperties())
{
    for (int row = 0; row < phraseRowCount; ++row)
    {
        const auto defaultNote = defaultNoteForRow (row);

        for (int step = 0; step < phraseStepCount; ++step)
        {
            phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (step)].store (defaultNote);
            phraseStepTimingMultiplier[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
                defaultStepTimingMultiplierIndex);
            phraseStepDurationFraction[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
                defaultStepDurationFractionIndex);
            phraseStepVelocity[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
                defaultStepVelocity);
        }

        phraseRowMuted[static_cast<size_t> (row)].store (0);
        phraseRowTimingOffset[static_cast<size_t> (row)].store (defaultRowTimingOffsetIndex);
        phraseRowFlushNoteOff[static_cast<size_t> (row)].store (0);
        phraseRowStepCount[static_cast<size_t> (row)].store (phraseStepCount);
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

void PluginProcessor::resetPhraseStepToDefaults (const int row, const int step)
{
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return;

    phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (step)].store (defaultNoteForRow (row));
    phraseStepTimingMultiplier[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
        defaultStepTimingMultiplierIndex);
    phraseStepDurationFraction[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
        defaultStepDurationFractionIndex);
    phraseStepVelocity[static_cast<size_t> (row)][static_cast<size_t> (step)].store (defaultStepVelocity);
    phraseStepGateStartPpq[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
        -1.0,
        std::memory_order_relaxed);
    phraseStepGateEndPpq[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
        -1.0,
        std::memory_order_relaxed);
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

double PluginProcessor::stepDurationFractionForIndex (const int fractionIndex)
{
    const auto index = juce::jlimit (0, stepDurationFractionCount - 1, fractionIndex);
    return stepDurationFractionValues[static_cast<size_t> (index)];
}

//==============================================================================
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
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

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    sampleRateHz = sampleRate;
    resetLastEmittedTriggers();
    wasPlaying = false;
    resetPendingNoteOffs();
}

void PluginProcessor::setPhraseNote (int row, int step, int noteNumber)
{
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return;

    phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
        juce::jlimit (0, 127, noteNumber));
}

int PluginProcessor::getPhraseNote (int row, int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return 60;

    return phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (step)].load();
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
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return;

    phraseStepTimingMultiplier[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
        juce::jlimit (0, stepTimingMultiplierCount - 1, multiplierIndex));
}

int PluginProcessor::getPhraseStepTimingMultiplier (const int row, const int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return defaultStepTimingMultiplierIndex;

    return phraseStepTimingMultiplier[static_cast<size_t> (row)][static_cast<size_t> (step)].load();
}

void PluginProcessor::setPhraseStepDurationFraction (const int row,
                                                   const int step,
                                                   const int fractionIndex)
{
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return;

    phraseStepDurationFraction[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
        juce::jlimit (0, stepDurationFractionCount - 1, fractionIndex));
}

int PluginProcessor::getPhraseStepDurationFraction (const int row, const int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return defaultStepDurationFractionIndex;

    return phraseStepDurationFraction[static_cast<size_t> (row)][static_cast<size_t> (step)].load();
}

void PluginProcessor::setPhraseStepVelocity (const int row, const int step, const int velocity)
{
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return;

    phraseStepVelocity[static_cast<size_t> (row)][static_cast<size_t> (step)].store (
        juce::jlimit (0, 127, velocity));
}

int PluginProcessor::getPhraseStepVelocity (const int row, const int step) const
{
    if (row < 0 || row >= phraseRowCount || step < 0 || step >= phraseStepCount)
        return defaultStepVelocity;

    return phraseStepVelocity[static_cast<size_t> (row)][static_cast<size_t> (step)].load();
}

int PluginProcessor::getPhraseRowStepCount (const int row) const
{
    if (row < 0 || row >= phraseRowCount)
        return 0;

    return juce::jlimit (0, phraseStepCount, phraseRowStepCount[static_cast<size_t> (row)].load());
}

void PluginProcessor::removePhraseStep (const int row, const int step)
{
    if (row < 0 || row >= phraseRowCount || step < 0)
        return;

    const auto count = getPhraseRowStepCount (row);

    if (step >= count)
        return;

    for (int index = step; index < count - 1; ++index)
    {
        const auto nextIndex = index + 1;

        phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (index)].store (
            phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (nextIndex)].load());
        phraseStepTimingMultiplier[static_cast<size_t> (row)][static_cast<size_t> (index)].store (
            phraseStepTimingMultiplier[static_cast<size_t> (row)][static_cast<size_t> (nextIndex)].load());
        phraseStepDurationFraction[static_cast<size_t> (row)][static_cast<size_t> (index)].store (
            phraseStepDurationFraction[static_cast<size_t> (row)][static_cast<size_t> (nextIndex)].load());
        phraseStepVelocity[static_cast<size_t> (row)][static_cast<size_t> (index)].store (
            phraseStepVelocity[static_cast<size_t> (row)][static_cast<size_t> (nextIndex)].load());
    }

    resetPhraseStepToDefaults (row, count - 1);
    phraseRowStepCount[static_cast<size_t> (row)].store (count - 1);
    resetPhraseStepGateEndsForRow (row);
}

juce::Array<juce::var> PluginProcessor::getPhraseStepPlaybackActivity() const
{
    juce::Array<juce::var> rows;

    const auto currentPpq = currentPlaybackPpq.load (std::memory_order_relaxed);
    const auto isPlaying = currentPpq >= 0.0;

    for (int row = 0; row < phraseRowCount; ++row)
    {
        juce::Array<juce::var> steps;

        const auto stepCount = getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            auto active = false;

            if (isPlaying)
            {
                const auto gateStart = phraseStepGateStartPpq[static_cast<size_t> (row)][static_cast<size_t> (step)].load (
                    std::memory_order_relaxed);
                const auto gateEnd = phraseStepGateEndPpq[static_cast<size_t> (row)][static_cast<size_t> (step)].load (
                    std::memory_order_relaxed);

                active = gateEnd > gateStart + 1.0e-9
                         && currentPpq >= gateStart - 1.0e-9
                         && currentPpq < gateEnd - 1.0e-9;
            }

            steps.add (active);
        }

        rows.add (steps);
    }

    return rows;
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

    for (int row = 0; row < phraseRowCount; ++row)
    {
        if (phraseRowMuted[static_cast<size_t> (row)].load (std::memory_order_relaxed) != 0)
            continue;

        const auto offset = rowTimingOffsetForIndex (
            phraseRowTimingOffset[static_cast<size_t> (row)].load (std::memory_order_relaxed));

        const auto stepCount = getPhraseRowStepCount (row);

        if (stepCount <= 0)
            continue;

        double stepLengthQuarters[phraseStepCount] {};
        double stepStartQuarters[phraseStepCount] {};
        auto cycleLengthQuarters = 0.0;

        for (int step = 0; step < stepCount; ++step)
        {
            stepStartQuarters[step] = cycleLengthQuarters;
            stepLengthQuarters[step] = stepTimingMultiplierForIndex (
                phraseStepTimingMultiplier[static_cast<size_t> (row)][static_cast<size_t> (step)].load (
                    std::memory_order_relaxed));
            cycleLengthQuarters += stepLengthQuarters[step];
        }

        if (cycleLengthQuarters <= 0.0)
            continue;

        struct StepTrigger
        {
            double ppq = 0.0;
            int step = 0;
        };

        std::array<StepTrigger, phraseStepCount * 8> triggers {};
        auto triggerCount = 0;

        for (int step = 0; step < stepCount; ++step)
        {
            const auto stepStartInCycle = stepStartQuarters[step];
            const auto nMin = static_cast<int> (std::ceil (
                (ppqStart - stepStartInCycle - offset - 1.0e-9) / cycleLengthQuarters));
            const auto nMax = static_cast<int> (std::floor (
                (ppqEnd - stepStartInCycle - offset - 1.0e-9) / cycleLengthQuarters));

            for (int cycle = nMin; cycle <= nMax; ++cycle)
            {
                const auto triggerPpq = static_cast<double> (cycle) * cycleLengthQuarters
                                        + stepStartInCycle + offset;

                if (triggerPpq < ppqStart - 1.0e-9 || triggerPpq >= ppqEnd + 1.0e-9)
                    continue;

                if (triggerCount >= static_cast<int> (triggers.size()))
                    break;

                triggers[static_cast<size_t> (triggerCount)] = { triggerPpq, step };
                ++triggerCount;
            }
        }

        if (triggerCount == 0)
            continue;

        std::sort (triggers.begin(),
                   triggers.begin() + triggerCount,
                   [] (const StepTrigger& a, const StepTrigger& b) { return a.ppq < b.ppq; });

        auto& lastTrigger = lastEmittedTriggerPpq[static_cast<size_t> (row)];

        if (ppqStart < lastTrigger - cycleLengthQuarters - 1.0e-9)
        {
            lastTrigger = ppqStart - cycleLengthQuarters - 1.0;
            resetPhraseStepGateEndsForRow (row);
        }

        for (int triggerIndex = 0; triggerIndex < triggerCount; ++triggerIndex)
        {
            const auto triggerPpq = triggers[static_cast<size_t> (triggerIndex)].ppq;

            if (triggerPpq <= lastTrigger + 1.0e-9)
                continue;

            lastTrigger = triggerPpq;

            const auto slot = triggers[static_cast<size_t> (triggerIndex)].step;
            const auto sampleOffset = juce::jlimit (
                0,
                bufferSamples - 1,
                static_cast<int> (std::lround ((triggerPpq - ppqStart) / ppqPerSample)));

            auto& pending = pendingNoteOffs[static_cast<size_t> (row)];

            if (pending.note >= 0)
            {
                midiMessages.addEvent (juce::MidiMessage::noteOff (1, pending.note), sampleOffset);
                pending.note = -1;
                pending.samplesRemaining = 0;
            }

            const auto note = phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (slot)].load();
            const auto velocity = phraseStepVelocity[static_cast<size_t> (row)][static_cast<size_t> (slot)].load (
                std::memory_order_relaxed);

            if (velocity <= 0)
                continue;

            const auto stepLength = stepLengthQuarters[slot];
            const auto durationFraction = stepDurationFractionForIndex (
                phraseStepDurationFraction[static_cast<size_t> (row)][static_cast<size_t> (slot)].load (
                    std::memory_order_relaxed));
            const auto gateQuarters = stepLength * durationFraction;
            const auto noteGateSamples = juce::jmax (
                1,
                static_cast<int> (std::lround (gateQuarters / ppqPerSample)));

            midiMessages.addEvent (
                juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (velocity)),
                sampleOffset);

            phraseStepGateStartPpq[static_cast<size_t> (row)][static_cast<size_t> (slot)].store (
                triggerPpq,
                std::memory_order_relaxed);
            phraseStepGateEndPpq[static_cast<size_t> (row)][static_cast<size_t> (slot)].store (
                triggerPpq + gateQuarters,
                std::memory_order_relaxed);

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

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("MidiPhrases");

    for (int row = 0; row < phraseRowCount; ++row)
    {
        juce::ValueTree rowTree ("Row");
        rowTree.setProperty ("index", row, nullptr);

        for (int step = 0; step < phraseStepCount; ++step)
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
        rowTree.setProperty ("stepCount", getPhraseRowStepCount (row), nullptr);
        state.appendChild (rowTree, nullptr);
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

    for (int i = 0; i < state.getNumChildren(); ++i)
    {
        const auto rowTree = state.getChild (i);

        if (! rowTree.hasType ("Row"))
            continue;

        const auto row = static_cast<int> (rowTree.getProperty ("index", i));
        const auto stepCount = juce::jlimit (
            0,
            phraseStepCount,
            static_cast<int> (rowTree.getProperty ("stepCount", phraseStepCount)));

        for (int step = 0; step < phraseStepCount; ++step)
        {
            if (step >= stepCount)
            {
                resetPhraseStepToDefaults (row, step);
                continue;
            }

            const auto propName = "step" + juce::String (step);
            const auto durationPropName = "duration" + juce::String (step);
            const auto velocityPropName = "velocity" + juce::String (step);
            const auto timingMultiplierPropName = "timingMultiplier" + juce::String (step);
            const auto note = static_cast<int> (rowTree.getProperty (propName, defaultNoteForRow (row)));
            setPhraseNote (row, step, note);
            setPhraseStepTimingMultiplier (row,
                                             step,
                                             static_cast<int> (rowTree.getProperty (
                                                 timingMultiplierPropName,
                                                 defaultStepTimingMultiplierIndex)));
            setPhraseStepDurationFraction (row,
                                             step,
                                             static_cast<int> (rowTree.getProperty (
                                                 durationPropName,
                                                 defaultStepDurationFractionIndex)));
            setPhraseStepVelocity (row,
                                   step,
                                   static_cast<int> (rowTree.getProperty (velocityPropName,
                                                                          defaultStepVelocity)));
        }

        phraseRowStepCount[static_cast<size_t> (row)].store (stepCount);

        setPhraseRowMuted (row, static_cast<bool> (rowTree.getProperty ("muted", false)));
        setPhraseRowTimingOffset (row,
                                  static_cast<int> (rowTree.getProperty ("timingOffset",
                                                                         defaultRowTimingOffsetIndex)));
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
