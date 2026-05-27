#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr int defaultRowNotes[] = { 60, 64, 67, 72 };
} // namespace

void PluginProcessor::resetPendingNoteOffs()
{
    for (auto& pending : pendingNoteOffs)
    {
        pending.note = -1;
        pending.samplesRemaining = 0;
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
            phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (step)].store (defaultNote);
    }
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
    lastEmittedQuarter = -1;
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
        lastEmittedQuarter = -1;
        resetPendingNoteOffs();
        return;
    }

    wasPlaying = true;

    const auto bufferSamples = buffer.getNumSamples();

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

    const auto qStart = static_cast<int> (std::ceil (ppqStart - 1.0e-9));
    const auto qEnd = static_cast<int> (std::floor (ppqEnd + 1.0e-9));

    if (qEnd < lastEmittedQuarter)
        lastEmittedQuarter = qEnd - 1;

    const auto noteGateSamples = juce::jmax (1, static_cast<int> (sampleRateHz * noteGateSeconds));

    for (int quarter = qStart; quarter <= qEnd; ++quarter)
    {
        if (quarter <= lastEmittedQuarter)
            continue;

        lastEmittedQuarter = quarter;

        const auto slot = ((quarter % phraseStepCount) + phraseStepCount) % phraseStepCount;
        const auto sampleOffset = juce::jlimit (
            0,
            bufferSamples - 1,
            static_cast<int> (std::lround ((static_cast<double> (quarter) - ppqStart) / ppqPerSample)));

        for (int row = 0; row < phraseRowCount; ++row)
        {
            auto& pending = pendingNoteOffs[static_cast<size_t> (row)];

            if (pending.note >= 0)
            {
                midiMessages.addEvent (juce::MidiMessage::noteOff (1, pending.note), sampleOffset);
                pending.note = -1;
                pending.samplesRemaining = 0;
            }

            const auto note = phraseNotes[static_cast<size_t> (row)][static_cast<size_t> (slot)].load();

            midiMessages.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)),
                                   sampleOffset);

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
            rowTree.setProperty (propName, getPhraseNote (row, step), nullptr);
        }

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

        for (int step = 0; step < phraseStepCount; ++step)
        {
            const auto propName = "step" + juce::String (step);
            const auto note = static_cast<int> (rowTree.getProperty (propName, defaultNoteForRow (row)));
            setPhraseNote (row, step, note);
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
