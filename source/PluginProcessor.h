#pragma once

#include <array>
#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>

#if (MSVC)
#include "ipps.h"
#endif

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static constexpr int phraseRowCount = 4;
    static constexpr int phraseStepCount = 4;

    void setPhraseNote (int row, int step, int noteNumber);
    int getPhraseNote (int row, int step) const;

private:
    static BusesProperties createBusesProperties();

    static constexpr double noteGateSeconds = 0.1;

    static int defaultNoteForRow (int row);
    void resetPendingNoteOffs();

    struct PendingNoteOff
    {
        int note = -1;
        int samplesRemaining = 0;
    };

    std::array<std::array<std::atomic<int>, phraseStepCount>, phraseRowCount> phraseNotes {};
    std::array<PendingNoteOff, phraseRowCount> pendingNoteOffs {};
    double sampleRateHz = 44100.0;
    int lastEmittedQuarter = -1;
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
