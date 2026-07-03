#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
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

#if JUCE_WEB_BROWSER
    void setWebHostCursorHandler (std::function<void (const juce::String&)> handler);
    void notifyWebHostCursor (const juce::String& cursorName);
    void setWebEditorFullscreenHandler (std::function<juce::var (int)> handler);
    juce::var requestWebEditorFullscreen (int mode);
    void setWebEditorScaleMinimumHandler (std::function<juce::var (int, int)> handler);
    juce::var requestWebEditorScaleMinimum (int minWidth, int minHeight);
#endif

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

    void setProjectMetadata (const juce::String& name,
                             const juce::String& description,
                             const juce::String& createdAt,
                             const juce::String& modifiedAt,
                             const juce::String& themeMode,
                             int uiScalePercent,
                             bool stretchStepsToFit);
    void resetProject();
    const juce::String& getProjectName() const noexcept { return projectName; }
    const juce::String& getProjectDescription() const noexcept { return projectDescription; }
    const juce::String& getProjectCreatedAt() const noexcept { return projectCreatedAt; }
    const juce::String& getProjectModifiedAt() const noexcept { return projectModifiedAt; }
    const juce::String& getProjectThemeMode() const noexcept { return projectThemeMode; }
    int getProjectUiScalePercent() const noexcept { return projectUiScalePercent; }
    void setProjectUiScalePercent (int uiScalePercent);
    bool getProjectStretchStepsToFit() const noexcept { return projectStretchStepsToFit; }

    static constexpr int phraseRowCount = 4;
    static constexpr int defaultPhraseStepsPerRow = 0;
    static constexpr int maxPhraseStepsPerRow = 64;
    static constexpr int patternSlotCount = 8;
    static constexpr int loopSlotCount = 8;
    static constexpr int midiMuteTriggerNote = patternSlotCount + loopSlotCount;
    static constexpr int combinationModeCount = 9;
    static constexpr int combinationModeCrossModulation = 0;
    static constexpr int combinationModeTendril = 1;
    static constexpr int combinationModeMultiplyEcho = 3;
    static constexpr int combinationModeWeave = 4;
    static constexpr int combinationModeHocket = 6;
    static constexpr int combinationModeCanon = 7;
    static constexpr int combinationModeRetroInversion = 8;
    static constexpr int combinationModeValidMask =
        (1 << combinationModeCrossModulation)
        | (1 << combinationModeTendril)
        | (1 << combinationModeMultiplyEcho)
        | (1 << combinationModeWeave)
        | (1 << combinationModeHocket)
        | (1 << combinationModeCanon)
        | (1 << combinationModeRetroInversion);

    void setPhraseNote (int row, int step, int noteNumber);
    int getPhraseNote (int row, int step) const;

    void setCombinationModeEnabled (int modeIndex, bool enabled);
    bool isCombinationModeEnabled (int modeIndex) const;
    int getCombinationModeMask() const;
    int getPatternCombinationModeMask (int patternSlot) const;

    static constexpr int scaleRootCount = 12;
    static constexpr int defaultScaleRoot = 0;
    static constexpr int scaleModeCount = 16;
    static constexpr int defaultScaleModeIndex = 0;

    void setPatternScale (int root, int modeIndex);
    int getPatternScaleRoot (int patternSlot) const;
    int getPatternScaleModeIndex (int patternSlot) const;

    static constexpr int minMidiNote = 0;
    static constexpr int maxMidiNote = 127;
    static constexpr int defaultNoteBandpassLowMidi = 36;  // C1
    static constexpr int defaultNoteBandpassHighMidi = 108; // C7

    void setPatternNoteBandpass (int lowMidi, int highMidi);
    int getPatternNoteBandpassLow (int patternSlot) const;
    int getPatternNoteBandpassHigh (int patternSlot) const;

    static constexpr int defaultVelocityTiltPivotMidi = 60;
    static constexpr int defaultVelocityTiltAmount = 0;
    static constexpr int minVelocityTiltAmount = -48;
    static constexpr int maxVelocityTiltAmount = 48;

    void setPatternVelocityTiltPivotMidi (int pivotMidi);
    void setPatternVelocityTiltAmount (int amount);
    int getPatternVelocityTiltPivotMidi (int patternSlot) const;
    int getPatternVelocityTiltAmount (int patternSlot) const;

    static constexpr int defaultGlobalTransposeSemitones = 0;
    static constexpr int minGlobalTransposeSemitones = -48;
    static constexpr int maxGlobalTransposeSemitones = 48;

    void setPatternGlobalTransposeSemitones (int semitones);
    int getPatternGlobalTransposeSemitones (int patternSlot) const;

    static constexpr int defaultOctavizerRelativeVelocity = 0;
    static constexpr int minOctavizerRelativeVelocity = -127;
    static constexpr int maxOctavizerRelativeVelocity = 127;
    static constexpr int octavizerSemitoneShift = 12;

    void setPatternOctavizerDown8vaEnabled (bool enabled);
    void setPatternOctavizerUp8vaEnabled (bool enabled);
    void setPatternOctavizerDown8vaRelativeVelocity (int relativeVelocity);
    void setPatternOctavizerUp8vaRelativeVelocity (int relativeVelocity);
    bool isPatternOctavizerDown8vaEnabled (int patternSlot) const;
    bool isPatternOctavizerUp8vaEnabled (int patternSlot) const;
    int getPatternOctavizerDown8vaRelativeVelocity (int patternSlot) const;
    int getPatternOctavizerUp8vaRelativeVelocity (int patternSlot) const;

    static constexpr int maxPercentValue = 99;
    static constexpr int maxStepProbabilityValue = 100;

    static constexpr int defaultShimmerFeedbackPercent = 10;
    static constexpr int defaultShimmerMixPercent = 20;
    static constexpr int minShimmerFeedbackPercent = 0;
    static constexpr int maxShimmerFeedbackPercent = maxPercentValue;
    static constexpr int minShimmerMixPercent = 0;
    static constexpr int maxShimmerMixPercent = maxPercentValue;

    void setPatternShimmerEnabled (bool enabled);
    void setPatternShimmerDelayMultiplierIndex (int multiplierIndex);
    void setPatternShimmerFeedbackPercent (int feedbackPercent);
    void setPatternShimmerMixPercent (int mixPercent);
    bool isPatternShimmerEnabled (int patternSlot) const;
    int getPatternShimmerDelayMultiplierIndex (int patternSlot) const;
    int getPatternShimmerFeedbackPercent (int patternSlot) const;
    int getPatternShimmerMixPercent (int patternSlot) const;

    static constexpr int defaultSeedingPhraseLength = 3;
    static constexpr int minSeedingPhraseLength = 2;
    static constexpr int maxSeedingPhraseLength = 16;
    static constexpr int defaultSeedingCenterMidi = -1;
    static constexpr int defaultSeedingRangeSemitones = 24;
    static constexpr int minSeedingRangeSemitones = 2;
    static constexpr int maxSeedingRangeSemitones = 48;
    static constexpr int defaultSeedingRepetition = 45;
    static constexpr int defaultSeedingComplexity = 50;
    static constexpr int defaultSeedingRandomness = 0;
    static constexpr int defaultSeedingTimingMeanMultiplierIndex = 3;
    static constexpr int minSeedingTimingMeanMultiplierIndex = 1;
    static constexpr int maxSeedingTimingMeanMultiplierIndex = 15;
    static constexpr int defaultSeedingTimingVariance = 50;
    static constexpr int defaultSeedingSeed = 1;
    static constexpr int defaultSeedingRhythmStep = 0;
    static constexpr int minSeedingRhythmStep = 0;
    static constexpr int maxSeedingRhythmStep = 7;

    struct SeedingRowState
    {
        int phraseLength = defaultSeedingPhraseLength;
        int centerMidi = defaultSeedingCenterMidi;
        int rangeSemitones = defaultSeedingRangeSemitones;
        int repetition = defaultSeedingRepetition;
        int complexity = defaultSeedingComplexity;
        int randomness = defaultSeedingRandomness;
        int timingMeanMultiplierIndex = defaultSeedingTimingMeanMultiplierIndex;
        int timingVariance = defaultSeedingTimingVariance;
        int symmetry = 0;
        int seed = defaultSeedingSeed;
        int repetitionSeed = 0;
        int complexitySeed = 0;
        int randomnessSeed = 0;
        int timingVarianceSeed = 0;
    };

    void setPatternSeedModeState (int rhythmStep,
                                  const std::array<SeedingRowState, phraseRowCount>& rowSettings,
                                  const std::array<int, phraseRowCount>& rowTargets);
    int getPatternSeedingRhythmStep (int patternSlot) const;
    SeedingRowState getPatternSeedingRowState (int patternSlot, int row) const;
    bool isPatternSeedingRowTargeted (int patternSlot, int row) const;

    void setPhraseRowMuted (int row, bool muted);
    bool isPhraseRowMuted (int row) const;

    void reversePhraseRowSteps (int row);
    void reorderPhraseRowSteps (int row,
                                 const std::array<int, maxPhraseStepsPerRow>& stepOrder,
                                 int orderSize);

    static constexpr int rowTimingOffsetCount = 17;
    static constexpr int defaultRowTimingOffsetIndex = 8;
    static constexpr double rowTimingOffsetMinQuarters = -2.0;
    static constexpr double rowTimingOffsetQuarterStep = 0.25;

    void setPhraseRowTimingOffset (int row, int offsetIndex);
    int getPhraseRowTimingOffset (int row) const;
    static double rowTimingOffsetForIndex (int offsetIndex);

    static constexpr int minPhraseRowMidiChannel = 1;
    static constexpr int maxPhraseRowMidiChannel = 16;
    static constexpr int defaultPhraseRowMidiChannel = 1;
    static constexpr std::array<int, phraseRowCount> defaultPhraseRowMidiChannels { 1, 2, 3, 4 };
    static int defaultPhraseRowMidiChannelForRow (int row);

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
    static constexpr int defaultStepOctave = 3;
    static constexpr int defaultStepNote = 60; // C3 (key center C at defaultStepOctave)

    static int defaultStepNoteForScaleRoot (int scaleRoot);

    void setPhraseStepVelocity (int row, int step, int velocity);
    int getPhraseStepVelocity (int row, int step) const;

    void setPhraseStepMuted (int row, int step, bool muted);
    bool isPhraseStepMuted (int row, int step) const;

    void setPhraseStepSkipped (int row, int step, bool skipped);
    bool isPhraseStepSkipped (int row, int step) const;

    static constexpr int defaultStepProbability = maxStepProbabilityValue;
    static constexpr int minStepCycle = 1;
    static constexpr int maxStepCycle = 64;
    static constexpr int defaultStepCycle = 1;
    static constexpr int defaultStepCycleMask = 1;

    void setPhraseStepProbability (int row, int step, int probability);
    int getPhraseStepProbability (int row, int step) const;

    void setPhraseStepCycle (int row, int step, int cycle);
    int getPhraseStepCycle (int row, int step) const;

    void setPhraseStepCycleOffset (int row, int step, int cycleMask);
    int getPhraseStepCycleOffset (int row, int step) const;

    void removePhraseStep (int row, int step);
    void insertPhraseStep (int row, int step);
    void duplicatePhraseStep (int row, int step);
    void movePhraseStep (int row, int fromStep, int toStep);
    int getPhraseRowStepCount (int row) const;
    void replacePhraseRowSteps (int row,
                                 int stepCount,
                                 const std::array<int, maxPhraseStepsPerRow>& notes,
                                 const std::array<int, maxPhraseStepsPerRow>& timingMultiplier,
                                 const std::array<double, maxPhraseStepsPerRow>& durationFraction,
                                 const std::array<int, maxPhraseStepsPerRow>& velocity,
                                 const std::array<int, maxPhraseStepsPerRow>& stepMuted,
                                 const std::array<int, maxPhraseStepsPerRow>& stepSkipped,
                                 const std::array<int, maxPhraseStepsPerRow>& probability,
                                 const std::array<int, maxPhraseStepsPerRow>& cycle,
                                 const std::array<int, maxPhraseStepsPerRow>& cycleOffset);

    /** Arm row for MIDI capture ({@code row} -1 disarms). Only one row at a time. */
    void setPhraseRowRecording (int row);
    int getPhraseRowRecording() const;

    /** Drains captured notes since the last call and applies them to the armed row. */
    juce::Array<int> drainPhraseRowRecordedNotes();

    /** MIDI note numbers currently held on a controller while recording (for keyboard UI). */
    juce::Array<int> getPhraseRowRecordingKeysHeld() const;

    /** Appends one captured step from the on-screen keyboard (message thread). */
    void injectPhraseRowRecordedNote (int midiNote);

    static constexpr int defaultLoopBraceStartQuarters = 0;
    static constexpr int defaultLoopBraceEndQuarters = 8;
    static constexpr double loopBraceSnapQuarters = 0.5;

    void setLoopBraceEnabled (bool enabled);
    bool isLoopBraceEnabled() const;

    void setLoopBraceStartQuarters (double startQuarters);
    double getLoopBraceStartQuarters() const;

    void setLoopBraceEndQuarters (double endQuarters);
    double getLoopBraceEndQuarters() const;

    void setCurrentPatternSlot (int patternSlot);
    int getCurrentPatternSlot() const;
    /** Pattern slot used for UI editing when output is disarmed (-1 selection). */
    int getViewPatternSlot() const;
    void deactivatePatternOutput();
    bool isPatternOutputArmed() const;
    int getAudioPatternSlot() const;
    void clearPatternSlot (int patternSlot);
    void copyPatternSlot (int sourcePatternSlot, int destinationPatternSlot);

    int getPatternPhraseRowStepCount (int patternSlot, int row) const;
    int getPatternPhraseNote (int patternSlot, int row, int step) const;
    bool isPatternPhraseRowMuted (int patternSlot, int row) const;
    int getPatternPhraseRowTimingOffset (int patternSlot, int row) const;
    int getPatternPhraseRowMidiChannel (int patternSlot, int row) const;
    int getPatternPhraseStepTimingMultiplier (int patternSlot, int row, int step) const;
    double getPatternPhraseStepDurationFraction (int patternSlot, int row, int step) const;
    int getPatternPhraseStepVelocity (int patternSlot, int row, int step) const;
    bool isPatternPhraseStepMuted (int patternSlot, int row, int step) const;
    bool isPatternPhraseStepSkipped (int patternSlot, int row, int step) const;
    int getPatternPhraseStepProbability (int patternSlot, int row, int step) const;
    int getPatternPhraseStepCycle (int patternSlot, int row, int step) const;
    int getPatternPhraseStepCycleOffset (int patternSlot, int row, int step) const;
    bool isPatternLoopBraceEnabled (int patternSlot) const;
    double getPatternLoopBraceStartQuarters (int patternSlot) const;
    double getPatternLoopBraceEndQuarters (int patternSlot) const;

    void saveCurrentBraceToLoopSlot (int loopSlot);
    void selectLoopSlot (int loopSlot);
    int getCurrentLoopSlot() const;
    bool isLoopSlotAssigned (int loopSlot) const;
    int getLoopSlotPatternSlot (int loopSlot) const;

    double getLoopPlaybackBeat() const;

    /** Beat used for UI playhead and step highlighting; -1 when not playing. */
    double getPlaybackBeat() const;

    bool hasStandaloneTransport() const;
    void setStandaloneTransportPlaying (bool shouldPlay);
    bool isStandaloneTransportPlaying() const;
    void setStandaloneTempoBpm (double bpm);
    double getStandaloneTempoBpm() const;

    bool hasActiveGeneratedNotes() const;
    void appendGeneratedNotePanicMessages (juce::MidiBuffer& midiMessages);

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

    void setRowColorsEnabled (bool enabled);
    bool isRowColorsEnabled() const;

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
        int combinationModeMask = 0;
    };

    struct LoopBraceState
    {
        int enabled = 0;
        double startQuarters = defaultLoopBraceStartQuarters;
        double endQuarters = defaultLoopBraceEndQuarters;
    };

    struct PatternState
    {
        SequencerState sequencer {};
        LoopBraceState loopBrace {};
        int scaleRoot = defaultScaleRoot;
        int scaleModeIndex = defaultScaleModeIndex;
        int noteBandpassLowMidi = defaultNoteBandpassLowMidi;
        int noteBandpassHighMidi = defaultNoteBandpassHighMidi;
        int velocityTiltPivotMidi = defaultVelocityTiltPivotMidi;
        int velocityTiltAmount = defaultVelocityTiltAmount;
        int globalTransposeSemitones = defaultGlobalTransposeSemitones;
        int octavizerDown8vaEnabled = 0;
        int octavizerUp8vaEnabled = 0;
        int octavizerDown8vaRelativeVelocity = defaultOctavizerRelativeVelocity;
        int octavizerUp8vaRelativeVelocity = defaultOctavizerRelativeVelocity;
        int shimmerEnabled = 0;
        int shimmerDelayMultiplierIndex = defaultStepTimingMultiplierIndex;
        int shimmerFeedbackPercent = defaultShimmerFeedbackPercent;
        int shimmerMixPercent = defaultShimmerMixPercent;
        int seedingRhythmStep = defaultSeedingRhythmStep;
        std::array<SeedingRowState, phraseRowCount> seedingRows {};
        std::array<int, phraseRowCount> seedingRowTargets { 1, 0, 0, 0 };
    };

    struct LoopSlotState
    {
        int assigned = 0;
        int patternSlot = 0;
        double startQuarters = defaultLoopBraceStartQuarters;
        double endQuarters = defaultLoopBraceEndQuarters;
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
            ReplaceRow,
            SetLoopBraceEnabled,
            SetLoopBraceStart,
            SetLoopBraceEnd,
            SetCombinationModeMask,
            SetPatternScale,
            SetPatternNoteBandpass,
            SetPatternVelocityTiltPivotMidi,
            SetPatternVelocityTiltAmount,
            SetPatternGlobalTransposeSemitones,
            SetPatternOctavizerDown8vaEnabled,
            SetPatternOctavizerUp8vaEnabled,
            SetPatternOctavizerDown8vaRelativeVelocity,
            SetPatternOctavizerUp8vaRelativeVelocity,
            SetPatternShimmerEnabled,
            SetPatternShimmerDelayMultiplierIndex,
            SetPatternShimmerFeedbackPercent,
            SetPatternShimmerMixPercent,
            ReplacePattern
        };

        Type type = Type::SetNote;
        int patternSlot = -1;
        int row = 0;
        int step = 0;
        int toStep = 0;
        int intValue = 0;
        double doubleValue = 0.0;
        PhraseRowSteps rowState {};
        PatternState patternState {};
    };

    struct PendingNoteOn;

    static BusesProperties createBusesProperties();

    int defaultNoteForRow (int row) const;
    void resetPhraseStepToDefaults (int row, int step);
    void resetPendingNoteOffs();
    void resetPendingNoteOns();
    void resetPendingCombinedNoteOffs();
    void resetActiveGeneratedNotes();
    void resetLastEmittedTriggers();
    void resetStepCycleCounters();
    void resetStepCycleCountersForRow (int row);
    bool isValidStep (int row, int step) const;
    bool isValidAudioStep (const SequencerState& state, int row, int step) const;
    void emitGeneratedNoteOn (int midiChannel,
                              int note,
                              int velocity,
                              int sampleOffset,
                              juce::MidiBuffer& midiMessages);
    void emitGeneratedNoteOff (int midiChannel,
                               int note,
                               int sampleOffset,
                               juce::MidiBuffer& midiMessages);
    void flushPendingGeneratedNoteOffs (int sampleOffset, juce::MidiBuffer& midiMessages);
    void flushPendingCombinedNoteOffs (int bufferSamples, juce::MidiBuffer& midiMessages);
    void flushActiveGeneratedNotes (int sampleOffset, juce::MidiBuffer& midiMessages);
    void flushActiveWeaveNotes (int sampleOffset, juce::MidiBuffer& midiMessages);
    void resetPlaybackMidiState();
    void addPendingNoteOn (const PendingNoteOn& note);
    void emitScheduledNoteOn (int row,
                              int midiChannel,
                              int note,
                              int velocity,
                              int step,
                              int sampleOffset,
                              int gateSamples,
                              int bufferSamples,
                              juce::MidiBuffer& midiMessages);
    void processPhraseRowNoteAuditions (int bufferSamples, juce::MidiBuffer& midiMessages);
    void emitLayeredGeneratedNote (int midiChannel,
                                   int note,
                                   int velocity,
                                   int sampleOffset,
                                   int gateSamples,
                                   int bufferSamples,
                                   juce::MidiBuffer& midiMessages);
    void flushPendingNoteOns (int bufferSamples, juce::MidiBuffer& midiMessages);
    void initialiseRowDefaults (PhraseRowSteps& steps, int row, int stepCount, int scaleRoot);
    void rebuildRowTimingLayout (PhraseRowSteps& steps);
    void reverseRowSteps (PhraseRowSteps& steps);
    bool applyRowStepOrder (PhraseRowSteps& steps,
                            const std::array<int, maxPhraseStepsPerRow>& stepOrder,
                            int orderSize);
    void publishCommandToAudio (const SequencerCommand& command);
    void publishRowToAudio (int row);
    void enqueueRecordedNote (int midiNote);
    bool tryDequeueRecordedNote (int& midiNoteOut);
    void appendRecordedNoteToModelRow (int row, int midiNote);
    void drainSequencerCommands();
    void applySequencerCommand (const SequencerCommand& command);
    PhraseRowSteps& modelRow (int row);
    const PhraseRowSteps& modelRow (int row) const;
    PatternState& modelPattern (int patternSlot);
    const PatternState& modelPattern (int patternSlot) const;
    SequencerState& modelSequencer();
    const SequencerState& modelSequencer() const;
    const SequencerState& audioSequencer() const;
    LoopBraceState& modelLoopBrace();
    const LoopBraceState& modelLoopBrace() const;
    const LoopBraceState& audioLoopBrace() const;
    const PhraseRowSteps* patternRowForStep (int patternSlot, int row, int step) const;
    int clampPatternSlot (int patternSlot) const;
    int clampLoopSlot (int loopSlot) const;
    void initialisePatternDefaults (PatternState& pattern);
    void publishPatternToAudio (int patternSlot);
    void publishLoopBraceCommandToAudio (SequencerCommand::Type type, int patternSlot);
    void deactivateLoopBraceForPatternSelection (int patternSlot);
    void requestAudioPatternSlot (int patternSlot);
    void applyAudioPatternSlot (int patternSlot, double reanchorTransportPpq = -1.0);
    void requestAudioLoopSlot (int loopSlot);
    void applyAudioLoopSlot (int loopSlot, double reanchorTransportPpq);
    void requestAudioLoopBraceEnable (int patternSlot);
    void clearPendingAudioLoopBraceEnable();
    void applyAudioLoopBraceEnable (double reanchorTransportPpq);
    void resolvePendingAudioLoopBraceEnableForStoppedPlayback();
    bool isAudioLoopSlotApplied (int loopSlot) const;
    double loopDownbeatTransportForSlot (int loopSlot, double transportPpq) const;
    void reanchorLoopScheduleAt (double transportPpq);
    void clearLoopScheduleAnchor();
    double mapTransportToLoopSchedulePpq (double transportPpq) const;
    void reanchorPatternScheduleAt (double transportPpq);
    void clearPatternScheduleAnchor();
    double mapTransportToPatternSchedulePpq (double transportPpq) const;
    void applyMuteOutputSilence (juce::MidiBuffer& midiMessages);
    void handleIncomingControlNotes (juce::MidiBuffer& midiMessages);
    bool shouldApplyPendingPatternSwitch (double ppqStart, double ppqEnd) const;
    void processTransportPlaybackRange (double transportPpqStart,
                                        double transportPpqEnd,
                                        double bufferTransportStartPpq,
                                        int bufferSamples,
                                        double ppqPerSample,
                                        juce::MidiBuffer& midiMessages,
                                        bool resetRowTriggersAtSegmentStart);

    double playbackBeatForUi() const;

    struct RowTriggerHit
    {
        double ppq = 0.0;
        int step = -1;
        int midiNote = -1;
        bool valid = false;
    };

    RowTriggerHit findEarliestRowTriggerInMappedRange (int row,
                                                       double schedulePpqStart,
                                                       double schedulePpqEnd) const;
    bool rowHasConflictingPitchBeforeMappedPpq (int row,
                                                int midiNote,
                                                double mappedPpqExclusiveEnd) const;
    bool shouldSustainGateAcrossLoopWrap (int row,
                                          int midiNote,
                                          double mappedTriggerPpq,
                                          double gateQuarters,
                                          double mappedScheduleEnd) const;
    bool shouldPreservePendingNoteAcrossLoopWrap (int row, int midiNote) const;
    void flushPendingCombinedNoteOffsForChannel (int midiChannel,
                                                 int sampleOffset,
                                                 juce::MidiBuffer& midiMessages);

    void processScheduledRange (double schedulePpqStart,
                                double schedulePpqEnd,
                                double segmentTransportStartPpq,
                                double bufferTransportStartPpq,
                                int bufferSamples,
                                double ppqPerSample,
                                juce::MidiBuffer& midiMessages,
                                bool resetRowTriggersAtSegmentStart);
    void processCombinedScheduledRange (double schedulePpqStart,
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
        int activeStep = -1;
        int samplesRemaining = 0;
    };

    struct PhraseRowNoteAudition
    {
        std::atomic<int> pending { 0 };
        int step = -1;
        int note = 60;
    };

    struct PendingNoteOn
    {
        int row = 0;
        int step = -1;
        int channel = 1;
        int note = -1;
        int velocity = 0;
        int samplesRemaining = 0;
        int gateSamples = 0;
        int layered = 0;
    };

    struct PendingCombinedNoteOff
    {
        int channel = 1;
        int note = -1;
        int samplesRemaining = 0;
    };

    struct CombinedNoteEvent
    {
        double ppq = 0.0;
        double gateQuarters = 0.0;
        int row = 0;
        int step = 0;
        int channel = 1;
        int note = 60;
        int velocity = defaultStepVelocity;
        bool extendedByHeldOverlap = false;
        bool carrierOnlyForCombinationFollowers = false;
    };

    static constexpr size_t sequencerCommandQueueCapacity = 1024;
    static constexpr size_t pendingNoteOnCapacity = static_cast<size_t> (phraseRowCount) * 16;
    static constexpr size_t pendingCombinedNoteOffCapacity = 256;
    static constexpr size_t combinedEventCapacity = 1024;

    std::array<PatternState, patternSlotCount> modelPatterns {};
    std::array<PatternState, patternSlotCount> audioPatterns {};
    std::array<LoopSlotState, loopSlotCount> loopSlots {};
    std::unique_ptr<std::array<SequencerCommand, sequencerCommandQueueCapacity>> sequencerCommandQueue {};
    std::atomic<size_t> sequencerCommandWriteIndex { 0 };
    std::atomic<size_t> sequencerCommandReadIndex { 0 };
    std::array<std::atomic<int>, phraseRowCount> phraseRowFlushNoteOff {};
    std::array<PhraseRowNoteAudition, phraseRowCount> phraseRowNoteAuditions {};
    std::array<PendingNoteOff, phraseRowCount> pendingNoteOffs {};
    std::array<PendingNoteOn, pendingNoteOnCapacity> pendingNoteOns {};
    std::array<PendingCombinedNoteOff, pendingCombinedNoteOffCapacity> pendingCombinedNoteOffs {};
    size_t pendingNoteOnCount = 0;
    size_t pendingCombinedNoteOffCount = 0;
    std::array<std::array<int, 128>, 16> activeGeneratedNoteCounts {};
    std::array<double, phraseRowCount> lastEmittedTriggerPpq {};
    std::array<ProcessScratch, phraseRowCount> processScratch {};
    std::array<CombinedNoteEvent, combinedEventCapacity> combinedEvents {};
    std::array<CombinedNoteEvent, combinedEventCapacity> combinedWorkingEvents {};
    std::array<std::array<std::uint32_t, maxPhraseStepsPerRow>, phraseRowCount> stepCycleCounters {};
    std::uint32_t playbackRandomState = 0xA5C3F17Du;
    std::atomic<double> currentPlaybackPpq { -1.0 };
    juce::AudioParameterInt* patternSlotParameter = nullptr;
    std::atomic<int> currentModelPatternSlot { 0 };
    int lastViewPatternSlot = 0;
    int audioActivePatternSlot = 0;
    int lastObservedParameterPatternSlot = 0;
    std::atomic<int> patternOutputArmed { 1 };
    std::atomic<int> muteFlushRequested { 0 };
    std::atomic<int> pendingAudioPatternSlot { -1 };
    std::atomic<int> currentLoopSlot { -1 };
    std::atomic<int> audioActiveLoopSlot { -1 };
    std::atomic<int> pendingAudioLoopSlot { -1 };
    int pendingAudioLoopBraceEnablePatternSlot = -1;
    std::atomic<int> pulseIndex { defaultPulseIndex };
    std::atomic<int> swingPercent { defaultSwingPercent };
    std::atomic<int> velocityHumanizePercent { defaultVelocityHumanizePercent };
    std::atomic<int> timingHumanizePercent { defaultTimingHumanizePercent };
    std::atomic<int> swingSubdivisionIndex { defaultSwingSubdivisionIndex };
    std::atomic<int> rowColorsEnabled { 1 };
    std::atomic<int> standaloneTransportPlaying { 0 };
    std::atomic<int> standaloneTransportResetRequested { 0 };
    std::atomic<double> standaloneTempoBpm { 120.0 };
    std::atomic<double> standaloneTransportPpqPosition { 0.0 };
    juce::String projectName { "Untitled Project" };
    juce::String projectDescription;
    juce::String projectCreatedAt;
    juce::String projectModifiedAt;
    juce::String projectThemeMode { "dark" };
    int projectUiScalePercent = 100;
    bool projectStretchStepsToFit = false;
    double sampleRateHz = 44100.0;
    bool wasPlaying = false;
    double loopScheduleAnchorTransportPpq = -1.0;
    double patternScheduleAnchorTransportPpq = -1.0;
    bool loopScheduleReanchorRequested = false;

    static constexpr int recordQueueCapacity = maxPhraseStepsPerRow;
    std::array<int, recordQueueCapacity> recordQueueNotes {};
    std::atomic<int> recordQueueWrite { 0 };
    std::atomic<int> recordQueueRead { 0 };
    std::atomic<int> recordingRow { -1 };
    std::atomic<int> recordingAwaitingFirstNote { 0 };
    std::array<std::atomic<int>, 128> recordingKeysHeld {};

#if JUCE_WEB_BROWSER
    std::function<void (const juce::String&)> webHostCursorHandler;
    std::function<juce::var (int)> webEditorFullscreenHandler;
    std::function<juce::var (int, int)> webEditorScaleMinimumHandler;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
