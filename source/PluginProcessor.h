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
    static constexpr int defaultPhraseStepsPerRow = 4;
    static constexpr int maxPhraseStepsPerRow = 64;

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

    static constexpr double defaultStepDurationFraction = 1.0;

    void setPhraseStepDurationFraction (int row, int step, double fraction);
    double getPhraseStepDurationFraction (int row, int step) const;

    static constexpr int defaultStepVelocity = 100;

    void setPhraseStepVelocity (int row, int step, int velocity);
    int getPhraseStepVelocity (int row, int step) const;

    void removePhraseStep (int row, int step);
    void insertPhraseStep (int row, int step);
    void movePhraseStep (int row, int fromStep, int toStep);
    int getPhraseRowStepCount (int row) const;

    static constexpr int defaultLoopBraceStartQuarters = 0;
    static constexpr int defaultLoopBraceEndQuarters = 8;

    void setLoopBraceEnabled (bool enabled);
    bool isLoopBraceEnabled() const;

    void setLoopBraceStartQuarters (int startQuarters);
    int getLoopBraceStartQuarters() const;

    void setLoopBraceEndQuarters (int endQuarters);
    int getLoopBraceEndQuarters() const;

    double getLoopPlaybackBeat() const;

    /** Beat used for UI playhead and step highlighting; -1 when not playing. */
    double getPlaybackBeat() const;

private:
    struct PhraseRowSteps
    {
        int stepCount = defaultPhraseStepsPerRow;
        std::array<int, maxPhraseStepsPerRow> notes {};
        std::array<int, maxPhraseStepsPerRow> timingMultiplier {};
        std::array<double, maxPhraseStepsPerRow> durationFraction {};
        std::array<int, maxPhraseStepsPerRow> velocity {};
        std::array<double, maxPhraseStepsPerRow> stepLengthQuarters {};
        std::array<double, maxPhraseStepsPerRow> stepStartQuarters {};
        double cycleLengthQuarters = 0.0;
    };

    struct ProcessScratch
    {
        struct StepTrigger
        {
            double ppq = 0.0;
            int step = 0;
        };

        static constexpr int maxTriggers = maxPhraseStepsPerRow * 8;
        std::array<StepTrigger, maxTriggers> triggers {};
    };

    struct SequencerState
    {
        std::array<PhraseRowSteps, phraseRowCount> rows {};
        std::array<int, phraseRowCount> muted {};
        std::array<int, phraseRowCount> timingOffset {};
    };

    struct SequencerCommand
    {
        enum class Type
        {
            SetNote,
            SetRowMuted,
            SetRowTimingOffset,
            SetStepTimingMultiplier,
            SetStepDurationFraction,
            SetStepVelocity,
            RemoveStep,
            InsertStep,
            MoveStep,
            ReplaceRow
        };

        Type type = Type::SetNote;
        int row = 0;
        int step = 0;
        int toStep = 0;
        int intValue = 0;
        double doubleValue = 0.0;
        PhraseRowSteps rowState {};
    };

    static BusesProperties createBusesProperties();

    static int defaultNoteForRow (int row);
    void resetPhraseStepToDefaults (int row, int step);
    void resetPendingNoteOffs();
    void resetLastEmittedTriggers();
    bool isValidStep (int row, int step) const;
    bool isValidAudioStep (const SequencerState& state, int row, int step) const;
    void initialiseRowDefaults (PhraseRowSteps& steps, int row, int stepCount);
    void rebuildRowTimingLayout (PhraseRowSteps& steps);
    void publishCommandToAudio (const SequencerCommand& command);
    void publishRowToAudio (int row);
    void drainSequencerCommands();
    void applySequencerCommand (const SequencerCommand& command);
    PhraseRowSteps& modelRow (int row);
    const PhraseRowSteps& modelRow (int row) const;

    double playbackBeatForUi() const;

    void processScheduledRange (double schedulePpqStart,
                                double schedulePpqEnd,
                                double segmentTransportStartPpq,
                                double bufferTransportStartPpq,
                                int bufferSamples,
                                double ppqPerSample,
                                juce::MidiBuffer& midiMessages,
                                bool resetRowTriggersAtSegmentStart);

    struct PendingNoteOff
    {
        int note = -1;
        int samplesRemaining = 0;
    };

    static constexpr size_t sequencerCommandQueueCapacity = 1024;

    SequencerState modelState {};
    SequencerState audioState {};
    std::array<SequencerCommand, sequencerCommandQueueCapacity> sequencerCommandQueue {};
    std::atomic<size_t> sequencerCommandWriteIndex { 0 };
    std::atomic<size_t> sequencerCommandReadIndex { 0 };
    std::array<std::atomic<int>, phraseRowCount> phraseRowFlushNoteOff {};
    std::array<PendingNoteOff, phraseRowCount> pendingNoteOffs {};
    std::array<double, phraseRowCount> lastEmittedTriggerPpq {};
    std::array<ProcessScratch, phraseRowCount> processScratch {};
    std::atomic<double> currentPlaybackPpq { -1.0 };
    std::atomic<int> loopBraceEnabled { 0 };
    std::atomic<int> loopBraceStartQuarters { defaultLoopBraceStartQuarters };
    std::atomic<int> loopBraceEndQuarters { defaultLoopBraceEndQuarters };
    double sampleRateHz = 44100.0;
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
