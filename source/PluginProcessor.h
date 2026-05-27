#pragma once

#include <array>
#include <atomic>
#include <vector>

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
    static constexpr int defaultPhraseStepsPerRow = 4;

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

    void removePhraseStep (int row, int step);
    void insertPhraseStep (int row, int step);
    void movePhraseStep (int row, int fromStep, int toStep);
    int getPhraseRowStepCount (int row) const;

    juce::Array<juce::var> getPhraseStepPlaybackActivity() const;

private:
    struct PhraseRowSteps
    {
        std::vector<int> notes;
        std::vector<int> timingMultiplier;
        std::vector<int> durationFraction;
        std::vector<int> velocity;
        std::vector<double> gateStartPpq;
        std::vector<double> gateEndPpq;
    };

    struct ProcessScratch
    {
        std::vector<double> stepLengthQuarters;
        std::vector<double> stepStartQuarters;

        struct StepTrigger
        {
            double ppq = 0.0;
            int step = 0;
        };

        std::vector<StepTrigger> triggers;
    };

    static BusesProperties createBusesProperties();

    static int defaultNoteForRow (int row);
    void resetPhraseStepToDefaults (int row, int step);
    void resetPendingNoteOffs();
    void resetLastEmittedTriggers();
    void resetPhraseStepGateEnds();
    void resetPhraseStepGateEndsForRow (int row);
    void syncRowGateStorage (int row);
    void syncProcessScratch (int row);
    bool isValidStep (int row, int step) const;

    struct PendingNoteOff
    {
        int note = -1;
        int samplesRemaining = 0;
    };

    std::array<PhraseRowSteps, phraseRowCount> phraseRows {};
    std::array<std::atomic<int>, phraseRowCount> phraseRowStepCount {};
    std::array<std::atomic<int>, phraseRowCount> phraseRowMuted {};
    std::array<std::atomic<int>, phraseRowCount> phraseRowTimingOffset {};
    std::array<std::atomic<int>, phraseRowCount> phraseRowFlushNoteOff {};
    std::array<PendingNoteOff, phraseRowCount> pendingNoteOffs {};
    std::array<double, phraseRowCount> lastEmittedTriggerPpq {};
    std::array<ProcessScratch, phraseRowCount> processScratch {};
    std::atomic<double> currentPlaybackPpq { -1.0 };
    double sampleRateHz = 44100.0;
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
