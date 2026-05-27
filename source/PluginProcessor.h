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

    void setPhraseRowMuted (int row, bool muted);
    bool isPhraseRowMuted (int row) const;

    static constexpr int rowTimingOffsetCount = 7;
    static constexpr int defaultRowTimingOffsetIndex = 3;

    void setPhraseRowTimingOffset (int row, int offsetIndex);
    int getPhraseRowTimingOffset (int row) const;
    static double rowTimingOffsetForIndex (int offsetIndex);

    static constexpr int stepTimingMultiplierCount = 5;
    static constexpr int defaultStepTimingMultiplierIndex = 2;

    void setPhraseStepTimingMultiplier (int row, int step, int multiplierIndex);
    int getPhraseStepTimingMultiplier (int row, int step) const;
    static double stepTimingMultiplierForIndex (int multiplierIndex);

    static constexpr int stepDurationFractionCount = 4;
    static constexpr int defaultStepDurationFractionIndex = 3;

    void setPhraseStepDurationFraction (int row, int step, int fractionIndex);
    int getPhraseStepDurationFraction (int row, int step) const;
    static double stepDurationFractionForIndex (int fractionIndex);

    static constexpr int defaultStepVelocity = 100;

    void setPhraseStepVelocity (int row, int step, int velocity);
    int getPhraseStepVelocity (int row, int step) const;

private:
    static BusesProperties createBusesProperties();

    static int defaultNoteForRow (int row);
    void resetPendingNoteOffs();
    void resetLastEmittedTriggers();

    struct PendingNoteOff
    {
        int note = -1;
        int samplesRemaining = 0;
    };

    std::array<std::array<std::atomic<int>, phraseStepCount>, phraseRowCount> phraseNotes {};
    std::array<std::array<std::atomic<int>, phraseStepCount>, phraseRowCount> phraseStepTimingMultiplier {};
    std::array<std::array<std::atomic<int>, phraseStepCount>, phraseRowCount> phraseStepDurationFraction {};
    std::array<std::array<std::atomic<int>, phraseStepCount>, phraseRowCount> phraseStepVelocity {};
    std::array<std::atomic<int>, phraseRowCount> phraseRowMuted {};
    std::array<std::atomic<int>, phraseRowCount> phraseRowTimingOffset {};
    std::array<std::atomic<int>, phraseRowCount> phraseRowFlushNoteOff {};
    std::array<PendingNoteOff, phraseRowCount> pendingNoteOffs {};
    std::array<double, phraseRowCount> lastEmittedTriggerPpq {};
    double sampleRateHz = 44100.0;
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
