#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>

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
    static constexpr int defaultPhraseStepsPerRow = 1;
    static constexpr int maxPhraseStepsPerRow = 64;

    void setPhraseNote (int row, int step, int noteNumber);
    int getPhraseNote (int row, int step) const;

    void setPhraseRowMuted (int row, bool muted);
    bool isPhraseRowMuted (int row) const;

    void reversePhraseRowSteps (int row);
    void reorderPhraseRowSteps (int row,
                                 const std::array<int, maxPhraseStepsPerRow>& stepOrder,
                                 int orderSize);

    static constexpr int rowTimingOffsetCount = 7;
    static constexpr int defaultRowTimingOffsetIndex = 3;

    void setPhraseRowTimingOffset (int row, int offsetIndex);
    int getPhraseRowTimingOffset (int row) const;
    static double rowTimingOffsetForIndex (int offsetIndex);

    static constexpr int minPhraseRowMidiChannel = 1;
    static constexpr int maxPhraseRowMidiChannel = 16;
    static constexpr int defaultPhraseRowMidiChannel = 1;

    void setPhraseRowMidiChannel (int row, int channel);
    int getPhraseRowMidiChannel (int row) const;

    static constexpr int stepTimingMultiplierCount = 16;
    static constexpr int defaultStepTimingMultiplierIndex = 3;
    static constexpr double stepTimingMultiplierQuarterStep = 0.25;
    static constexpr double stepTimingMultiplierMin = 0.25;
    static constexpr double stepTimingMultiplierMax = 4.0;

    void setPhraseStepTimingMultiplier (int row, int step, int multiplierIndex);
    int getPhraseStepTimingMultiplier (int row, int step) const;
    static double stepTimingMultiplierForIndex (int multiplierIndex);

    static constexpr double defaultStepDurationFraction = 1.0;

    void setPhraseStepDurationFraction (int row, int step, double fraction);
    double getPhraseStepDurationFraction (int row, int step) const;

    static constexpr int defaultStepVelocity = 100;
    static constexpr int defaultStepNote = 60; // C3

    void setPhraseStepVelocity (int row, int step, int velocity);
    int getPhraseStepVelocity (int row, int step) const;

    void setPhraseStepMuted (int row, int step, bool muted);
    bool isPhraseStepMuted (int row, int step) const;

    void setPhraseStepSkipped (int row, int step, bool skipped);
    bool isPhraseStepSkipped (int row, int step) const;

    static constexpr int defaultStepProbability = 100;
    static constexpr int minStepCycle = 1;
    static constexpr int maxStepCycle = 64;
    static constexpr int defaultStepCycle = 1;
    static constexpr int defaultStepCycleOffset = 0;

    void setPhraseStepProbability (int row, int step, int probability);
    int getPhraseStepProbability (int row, int step) const;

    void setPhraseStepCycle (int row, int step, int cycle);
    int getPhraseStepCycle (int row, int step) const;

    void setPhraseStepCycleOffset (int row, int step, int cycleOffset);
    int getPhraseStepCycleOffset (int row, int step) const;

    void removePhraseStep (int row, int step);
    void insertPhraseStep (int row, int step);
    void duplicatePhraseStep (int row, int step);
    void movePhraseStep (int row, int fromStep, int toStep);
    int getPhraseRowStepCount (int row) const;

    static constexpr int defaultLoopBraceStartQuarters = 0;
    static constexpr int defaultLoopBraceEndQuarters = 8;
    static constexpr double loopBraceSnapQuarters = 0.5;

    void setLoopBraceEnabled (bool enabled);
    bool isLoopBraceEnabled() const;

    void setLoopBraceStartQuarters (double startQuarters);
    double getLoopBraceStartQuarters() const;

    void setLoopBraceEndQuarters (double endQuarters);
    double getLoopBraceEndQuarters() const;

    double getLoopPlaybackBeat() const;

    /** Beat used for UI playhead and step highlighting; -1 when not playing. */
    double getPlaybackBeat() const;

    bool hasStandaloneTransport() const;
    void setStandaloneTransportPlaying (bool shouldPlay);
    bool isStandaloneTransportPlaying() const;
    void setStandaloneTempoBpm (double bpm);
    double getStandaloneTempoBpm() const;

    static constexpr int pulseCount = 4;
    static constexpr int defaultPulseIndex = 1;

    void setPulseIndex (int pulseIndex);
    int getPulseIndex() const;
    static double pulseQuartersForIndex (int pulseIndex);

    static constexpr int defaultSwingPercent = 0;
    static constexpr int defaultVelocityHumanizePercent = 0;
    static constexpr int defaultTimingHumanizePercent = 0;
    static constexpr int swingSubdivisionCount = 3;
    static constexpr int defaultSwingSubdivisionIndex = 1;

    void setSwingPercent (int percent);
    int getSwingPercent() const;
    void setVelocityHumanizePercent (int percent);
    int getVelocityHumanizePercent() const;
    void setTimingHumanizePercent (int percent);
    int getTimingHumanizePercent() const;
    void setSwingSubdivisionIndex (int subdivisionIndex);
    int getSwingSubdivisionIndex() const;
    static double swingSubdivisionForIndex (int subdivisionIndex);

private:
    struct PhraseRowSteps
    {
        int stepCount = defaultPhraseStepsPerRow;
        std::array<int, maxPhraseStepsPerRow> notes {};
        std::array<int, maxPhraseStepsPerRow> timingMultiplier {};
        std::array<double, maxPhraseStepsPerRow> durationFraction {};
        std::array<int, maxPhraseStepsPerRow> velocity {};
        std::array<int, maxPhraseStepsPerRow> stepMuted {};
        std::array<int, maxPhraseStepsPerRow> stepSkipped {};
        std::array<int, maxPhraseStepsPerRow> probability {};
        std::array<int, maxPhraseStepsPerRow> cycle {};
        std::array<int, maxPhraseStepsPerRow> cycleOffset {};
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
        std::array<int, phraseRowCount> midiChannel {};
    };

    struct SequencerCommand
    {
        enum class Type
        {
            SetNote,
            SetRowMuted,
            SetRowTimingOffset,
            SetRowMidiChannel,
            SetStepTimingMultiplier,
            SetStepDurationFraction,
            SetStepVelocity,
            SetStepMuted,
            SetStepSkipped,
            SetStepProbability,
            SetStepCycle,
            SetStepCycleOffset,
            RemoveStep,
            InsertStep,
            DuplicateStep,
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

    struct PendingNoteOn;

    static BusesProperties createBusesProperties();

    static int defaultNoteForRow (int row);
    void resetPhraseStepToDefaults (int row, int step);
    void resetPendingNoteOffs();
    void resetPendingNoteOns();
    void resetLastEmittedTriggers();
    void resetStepCycleCounters();
    void resetStepCycleCountersForRow (int row);
    bool isValidStep (int row, int step) const;
    bool isValidAudioStep (const SequencerState& state, int row, int step) const;
    void addPendingNoteOn (const PendingNoteOn& note);
    void emitScheduledNoteOn (int row,
                              int midiChannel,
                              int note,
                              int velocity,
                              int sampleOffset,
                              int gateSamples,
                              int bufferSamples,
                              juce::MidiBuffer& midiMessages);
    void flushPendingNoteOns (int bufferSamples, juce::MidiBuffer& midiMessages);
    void initialiseRowDefaults (PhraseRowSteps& steps, int row, int stepCount);
    void rebuildRowTimingLayout (PhraseRowSteps& steps);
    void reverseRowSteps (PhraseRowSteps& steps);
    bool applyRowStepOrder (PhraseRowSteps& steps,
                            const std::array<int, maxPhraseStepsPerRow>& stepOrder,
                            int orderSize);
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
        int channel = 1;
        int note = -1;
        int samplesRemaining = 0;
    };

    struct PendingNoteOn
    {
        int row = 0;
        int channel = 1;
        int note = -1;
        int velocity = 0;
        int samplesRemaining = 0;
        int gateSamples = 0;
    };

    static constexpr size_t sequencerCommandQueueCapacity = 1024;
    static constexpr size_t pendingNoteOnCapacity = static_cast<size_t> (phraseRowCount) * 16;

    SequencerState modelState {};
    SequencerState audioState {};
    std::unique_ptr<std::array<SequencerCommand, sequencerCommandQueueCapacity>> sequencerCommandQueue {};
    std::atomic<size_t> sequencerCommandWriteIndex { 0 };
    std::atomic<size_t> sequencerCommandReadIndex { 0 };
    std::array<std::atomic<int>, phraseRowCount> phraseRowFlushNoteOff {};
    std::array<PendingNoteOff, phraseRowCount> pendingNoteOffs {};
    std::array<PendingNoteOn, pendingNoteOnCapacity> pendingNoteOns {};
    size_t pendingNoteOnCount = 0;
    std::array<double, phraseRowCount> lastEmittedTriggerPpq {};
    std::array<ProcessScratch, phraseRowCount> processScratch {};
    std::array<std::array<std::uint32_t, maxPhraseStepsPerRow>, phraseRowCount> stepCycleCounters {};
    std::uint32_t playbackRandomState = 0xA5C3F17Du;
    std::atomic<double> currentPlaybackPpq { -1.0 };
    std::atomic<int> pulseIndex { defaultPulseIndex };
    std::atomic<int> swingPercent { defaultSwingPercent };
    std::atomic<int> velocityHumanizePercent { defaultVelocityHumanizePercent };
    std::atomic<int> timingHumanizePercent { defaultTimingHumanizePercent };
    std::atomic<int> swingSubdivisionIndex { defaultSwingSubdivisionIndex };
    std::atomic<int> loopBraceEnabled { 0 };
    std::atomic<double> loopBraceStartQuarters { defaultLoopBraceStartQuarters };
    std::atomic<double> loopBraceEndQuarters { defaultLoopBraceEndQuarters };
    std::atomic<int> standaloneTransportPlaying { 0 };
    std::atomic<int> standaloneTransportResetRequested { 0 };
    std::atomic<double> standaloneTempoBpm { 120.0 };
    std::atomic<double> standaloneTransportPpqPosition { 0.0 };
    double sampleRateHz = 44100.0;
    bool wasPlaying = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
