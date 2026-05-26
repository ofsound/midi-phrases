#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

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
    for (auto& note : phraseNotes)
        note.store (60);
}

PluginProcessor::~PluginProcessor()
{
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
    pendingNoteOffNote = -1;
    pendingNoteOffSamples = 0;
}

void PluginProcessor::setPhraseNote (int index, int noteNumber)
{
    if (index < 0 || index >= phraseNoteCount)
        return;

    phraseNotes[static_cast<size_t> (index)].store (juce::jlimit (0, 127, noteNumber));
}

void PluginProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
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
        pendingNoteOffNote = -1;
        pendingNoteOffSamples = 0;
        return;
    }

    wasPlaying = true;

    if (pendingNoteOffNote >= 0)
    {
        if (pendingNoteOffSamples < buffer.getNumSamples())
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, pendingNoteOffNote),
                                   pendingNoteOffSamples);
            pendingNoteOffNote = -1;
            pendingNoteOffSamples = 0;
        }
        else
        {
            pendingNoteOffSamples -= buffer.getNumSamples();
        }
    }

    const auto ppqStart = position->getPpqPosition().orFallback (0.0);
    const auto bpm = position->getBpm().orFallback (120.0);
    const auto ppqPerSample = (bpm / 60.0) / sampleRateHz;
    const auto ppqEnd = ppqStart + static_cast<double> (buffer.getNumSamples()) * ppqPerSample;

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

        const auto slot = ((quarter % phraseNoteCount) + phraseNoteCount) % phraseNoteCount;
        const auto note = phraseNotes[static_cast<size_t> (slot)].load();
        const auto sampleOffset = juce::jlimit (
            0,
            buffer.getNumSamples() - 1,
            static_cast<int> (std::lround ((static_cast<double> (quarter) - ppqStart) / ppqPerSample)));

        if (pendingNoteOffNote >= 0)
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, pendingNoteOffNote), sampleOffset);
            pendingNoteOffNote = -1;
            pendingNoteOffSamples = 0;
        }

        midiMessages.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)),
                               sampleOffset);

        const auto samplesUntilOff = sampleOffset + noteGateSamples;

        if (samplesUntilOff < buffer.getNumSamples())
        {
            midiMessages.addEvent (juce::MidiMessage::noteOff (1, note), samplesUntilOff);
        }
        else
        {
            pendingNoteOffNote = note;
            pendingNoteOffSamples = samplesUntilOff - buffer.getNumSamples();
        }
    }
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
