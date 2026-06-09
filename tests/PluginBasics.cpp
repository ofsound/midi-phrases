#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cmath>

TEST_CASE ("one is equal to one", "[dummy]")
{
    REQUIRE (1 == 1);
}

namespace
{
void ensurePhraseRowStepCount (PluginProcessor& plugin, const int row, const int stepCount)
{
    while (plugin.getPhraseRowStepCount (row) < stepCount)
        plugin.insertPhraseStep (row, plugin.getPhraseRowStepCount (row));

    while (plugin.getPhraseRowStepCount (row) > stepCount)
        plugin.removePhraseStep (row, plugin.getPhraseRowStepCount (row) - 1);
}
}

TEST_CASE ("Echo mode follows pattern scale", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeMultiplyEcho, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, false);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 2);
    ensurePhraseRowStepCount (testPlugin, 2, 2);

    testPlugin.setPhraseNote (0, 0, 57); // A3
    testPlugin.setPhraseNote (1, 0, 53); // F3
    testPlugin.setPhraseNote (1, 1, 48); // C3
    testPlugin.setPhraseNote (2, 0, 64); // E4
    testPlugin.setPhraseNote (2, 1, 65); // F4

    testPlugin.setPhraseStepTimingMultiplier (0, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepTimingMultiplier (1, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepTimingMultiplier (1, 1, PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepTimingMultiplier (2, 0, 1);
    testPlugin.setPhraseStepTimingMultiplier (2, 1, 2);
    testPlugin.setPhraseRowTimingOffset (2, 4);

    for (int row = 0; row < 3; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
            testPlugin.setPhraseStepVelocity (row, step, 100);
        }
    }

    testPlugin.setPatternScale (0, 1); // C major — after pulse publish so audio thread sees it
    CHECK (testPlugin.getPatternScaleModeIndex (0) == 1);

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    struct PlayHeadMock : juce::AudioPlayHead
    {
        juce::AudioPlayHead::PositionInfo info;

        juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
        {
            return info;
        }
    } playHead;

    playHead.info.setBpm (120.0);
    testPlugin.setPlayHead (&playHead);

    std::array<int, 128> noteOnCounts {};

    for (int block = 0; block < 700; ++block)
    {
        midi.clear();
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                      * (120.0 / 60.0) / sampleRate);
        testPlugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn())
                ++noteOnCounts[static_cast<size_t> (message.getNoteNumber())];
        }

        if (playHead.info.getPpqPosition() >= 8.0)
            break;
    }

    CHECK (noteOnCounts[50] > 0); // D3 scale-degree echo from C3 + (E4 -> F4)
    CHECK (noteOnCounts[49] == 0); // C#3 would be chromatic, not in C major
    CHECK (noteOnCounts[52] > 0); // E3 = A3 shifted by F3 -> C3 scale degrees
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Plugin instance", "[instance]")
{
    PluginProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
            Catch::Matchers::Equals ("MIDI Phrases"));
    }

    SECTION ("fresh phrase state")
    {
        for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
        {
            CHECK (testPlugin.getPhraseRowStepCount (row) == 0);
            CHECK (testPlugin.isPhraseRowMuted (row) == (row != 0));
        }
    }

    SECTION ("row mute")
    {
        CHECK_FALSE (testPlugin.isPhraseRowMuted (0));
        CHECK (testPlugin.isPhraseRowMuted (1));
        CHECK (testPlugin.isPhraseRowMuted (2));
        CHECK (testPlugin.isPhraseRowMuted (3));

        testPlugin.setPhraseRowMuted (0, true);
        testPlugin.setPhraseRowMuted (1, false);

        CHECK (testPlugin.isPhraseRowMuted (0));
        CHECK_FALSE (testPlugin.isPhraseRowMuted (1));

        testPlugin.setPhraseRowMuted (0, false);
        CHECK_FALSE (testPlugin.isPhraseRowMuted (0));
    }

    SECTION ("unmuting another row does not choke active row playback")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setPhraseRowMuted (0, false);

        for (int row = 1; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, true);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        playHead.info.setPpqPosition (0.0);
        testPlugin.processBlock (buffer, midi);

        auto rowZeroNoteOn = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                rowZeroNoteOn = true;
        }

        CHECK (rowZeroNoteOn);

        midi.clear();
        playHead.info.setPpqPosition (0.25);
        testPlugin.processBlock (buffer, midi);

        testPlugin.setPhraseRowMuted (1, false);

        midi.clear();
        playHead.info.setPpqPosition (0.3);
        testPlugin.processBlock (buffer, midi);

        auto rowZeroNoteOff = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                rowZeroNoteOff = true;
        }

        CHECK_FALSE (rowZeroNoteOff);
        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("row step order reverse")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 4);

        for (int step = 0; step < testPlugin.getPhraseRowStepCount (0); ++step)
        {
            testPlugin.setPhraseNote (0, step, 60 + step);
            testPlugin.setPhraseStepVelocity (0, step, 20 + step);
            testPlugin.setPhraseStepDurationFraction (0, step, 0.25 * static_cast<double> (step + 1));
            testPlugin.setPhraseStepProbability (0, step, 50 + step);
            testPlugin.setPhraseStepCycle (0, step, 4 + step);
            testPlugin.setPhraseStepCycleOffset (0, step, step);
        }

        testPlugin.reversePhraseRowSteps (0);

        CHECK (testPlugin.getPhraseNote (0, 0) == 63);
        CHECK (testPlugin.getPhraseNote (0, 3) == 60);
        CHECK (testPlugin.getPhraseStepVelocity (0, 0) == 23);
        CHECK (testPlugin.getPhraseStepVelocity (0, 3) == 20);
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0) == Catch::Approx (1.0));
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 3) == Catch::Approx (0.25));
        CHECK (testPlugin.getPhraseStepProbability (0, 0) == 53);
        CHECK (testPlugin.getPhraseStepProbability (0, 3) == 50);
        CHECK (testPlugin.getPhraseStepCycle (0, 0) == 7);
        CHECK (testPlugin.getPhraseStepCycle (0, 3) == 4);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 0) == 3);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 3) == 0);

        testPlugin.reversePhraseRowSteps (0);
        CHECK (testPlugin.getPhraseNote (0, 0) == 60);
        CHECK (testPlugin.getPhraseNote (0, 3) == 63);
    }

    SECTION ("row step order permutation")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 4);

        for (int step = 0; step < testPlugin.getPhraseRowStepCount (0); ++step)
        {
            testPlugin.setPhraseNote (0, step, 60 + step);
            testPlugin.setPhraseStepVelocity (0, step, 30 + step);
            testPlugin.setPhraseStepDurationFraction (0, step, 0.25 * static_cast<double> (step + 1));
        }

        std::array<int, PluginProcessor::maxPhraseStepsPerRow> stepOrder {};
        stepOrder[0] = 2;
        stepOrder[1] = 0;
        stepOrder[2] = 3;
        stepOrder[3] = 1;
        testPlugin.reorderPhraseRowSteps (0, stepOrder, 4);

        CHECK (testPlugin.getPhraseNote (0, 0) == 62);
        CHECK (testPlugin.getPhraseNote (0, 1) == 60);
        CHECK (testPlugin.getPhraseNote (0, 2) == 63);
        CHECK (testPlugin.getPhraseNote (0, 3) == 61);
        CHECK (testPlugin.getPhraseStepVelocity (0, 0) == 32);
        CHECK (testPlugin.getPhraseStepVelocity (0, 3) == 31);
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0) == Catch::Approx (0.75));
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 3) == Catch::Approx (0.5));

        stepOrder[0] = 0;
        stepOrder[1] = 0;
        stepOrder[2] = 2;
        stepOrder[3] = 3;
        testPlugin.reorderPhraseRowSteps (0, stepOrder, 4);
        CHECK (testPlugin.getPhraseNote (0, 0) == 62);
        CHECK (testPlugin.getPhraseNote (0, 1) == 60);
        CHECK (testPlugin.getPhraseNote (0, 2) == 63);
        CHECK (testPlugin.getPhraseNote (0, 3) == 61);
    }

    SECTION ("pulse")
    {
        CHECK (testPlugin.getPulseIndex() == PluginProcessor::defaultPulseIndex);
        CHECK (PluginProcessor::pulseQuartersForIndex (0) == Catch::Approx (0.5));
        CHECK (PluginProcessor::pulseQuartersForIndex (1) == Catch::Approx (1.0));
        CHECK (PluginProcessor::pulseQuartersForIndex (2) == Catch::Approx (2.0));
        CHECK (PluginProcessor::pulseQuartersForIndex (3) == Catch::Approx (4.0));

        testPlugin.setPulseIndex (0);
        CHECK (testPlugin.getPulseIndex() == 0);

        testPlugin.setPulseIndex (99);
        CHECK (testPlugin.getPulseIndex() == PluginProcessor::pulseCount - 1);

        testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    }

    SECTION ("swing and humanize controls")
    {
        CHECK (testPlugin.getSwingPercent() == PluginProcessor::defaultSwingPercent);
        CHECK (testPlugin.getVelocityHumanizePercent()
               == PluginProcessor::defaultVelocityHumanizePercent);
        CHECK (testPlugin.getTimingHumanizePercent()
               == PluginProcessor::defaultTimingHumanizePercent);
        CHECK (testPlugin.getSwingSubdivisionIndex()
               == PluginProcessor::defaultSwingSubdivisionIndex);
        CHECK (PluginProcessor::swingSubdivisionForIndex (0) == Catch::Approx (0.25));
        CHECK (PluginProcessor::swingSubdivisionForIndex (1) == Catch::Approx (0.5));
        CHECK (PluginProcessor::swingSubdivisionForIndex (2) == Catch::Approx (1.0));

        testPlugin.setSwingPercent (125);
        testPlugin.setVelocityHumanizePercent (64);
        testPlugin.setTimingHumanizePercent (-3);
        testPlugin.setSwingSubdivisionIndex (99);

        CHECK (testPlugin.getSwingPercent() == 100);
        CHECK (testPlugin.getVelocityHumanizePercent() == 64);
        CHECK (testPlugin.getTimingHumanizePercent() == 0);
        CHECK (testPlugin.getSwingSubdivisionIndex()
               == PluginProcessor::swingSubdivisionCount - 1);
    }

    SECTION ("row color UI state")
    {
        CHECK_FALSE (testPlugin.isRowColorsEnabled());

        testPlugin.setRowColorsEnabled (true);
        CHECK (testPlugin.isRowColorsEnabled());

        testPlugin.setRowColorsEnabled (false);
        CHECK_FALSE (testPlugin.isRowColorsEnabled());
    }

    SECTION ("combination modes are pattern state")
    {
        CHECK (testPlugin.getCombinationModeMask() == 0);

        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, true);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeMultiplyEcho, true);

        const auto expectedMask =
            (1 << PluginProcessor::combinationModeWeave)
            | (1 << PluginProcessor::combinationModeMultiplyEcho);
        CHECK (testPlugin.getCombinationModeMask() == expectedMask);
        CHECK (testPlugin.getPatternCombinationModeMask (0) == expectedMask);

        testPlugin.setCurrentPatternSlot (1);
        CHECK (testPlugin.getCombinationModeMask() == 0);

        testPlugin.copyPatternSlot (0, 1);
        CHECK (testPlugin.getPatternCombinationModeMask (1) == expectedMask);

        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, false);
        CHECK_FALSE (testPlugin.isCombinationModeEnabled (PluginProcessor::combinationModeWeave));
    }

    SECTION ("multiply echo mode emits interval-shifted echoes")
    {
        testPlugin.prepareToPlay (1000.0, 512);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeMultiplyEcho, true);
        testPlugin.setPhraseRowMuted (0, false);
        testPlugin.setPhraseRowMuted (1, false);
        testPlugin.setPhraseRowMuted (2, false);
        testPlugin.setPhraseRowMuted (3, true);

        ensurePhraseRowStepCount (testPlugin, 0, 1);
        ensurePhraseRowStepCount (testPlugin, 1, 2);
        ensurePhraseRowStepCount (testPlugin, 2, 2);

        testPlugin.setPhraseNote (0, 0, 57); // A3
        testPlugin.setPhraseNote (1, 0, 53); // F3
        testPlugin.setPhraseNote (1, 1, 48); // C3
        testPlugin.setPhraseNote (2, 0, 64); // E4
        testPlugin.setPhraseNote (2, 1, 65); // F4

        testPlugin.setPhraseStepTimingMultiplier (0, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepTimingMultiplier (1, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepTimingMultiplier (1, 1, PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepTimingMultiplier (2, 0, 1); // 0.5x
        testPlugin.setPhraseStepTimingMultiplier (2, 1, 2); // 0.75x
        testPlugin.setPhraseRowTimingOffset (2, 4); // +0.25 quarters

        for (int row = 0; row < 3; ++row)
        {
            const auto stepCount = testPlugin.getPhraseRowStepCount (row);

            for (int step = 0; step < stepCount; ++step)
            {
                testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
                testPlugin.setPhraseStepVelocity (row, step, 100);
            }
        }

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (120.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);
        testPlugin.processBlock (buffer, midi);

        std::array<int, 128> noteOnCounts {};

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn())
                ++noteOnCounts[static_cast<size_t> (message.getNoteNumber())];
        }

        CHECK (noteOnCounts[57] > 0); // A3 carrier echo
        CHECK (noteOnCounts[53] > 0); // F3 from row 1 carriers
        CHECK (noteOnCounts[64] > 0); // E4 from row 2
        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("multiply echo mode emits bass interval echoes over eight quarters")
    {
        constexpr double sampleRate = 44100.0;
        constexpr int blockSize = 512;

        testPlugin.prepareToPlay (sampleRate, blockSize);
        testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeMultiplyEcho, true);
        testPlugin.setPhraseRowMuted (0, false);
        testPlugin.setPhraseRowMuted (1, false);
        testPlugin.setPhraseRowMuted (2, false);
        testPlugin.setPhraseRowMuted (3, true);

        ensurePhraseRowStepCount (testPlugin, 0, 1);
        ensurePhraseRowStepCount (testPlugin, 1, 2);
        ensurePhraseRowStepCount (testPlugin, 2, 2);

        testPlugin.setPhraseNote (0, 0, 57); // A3
        testPlugin.setPhraseNote (1, 0, 53); // F3
        testPlugin.setPhraseNote (1, 1, 48); // C3
        testPlugin.setPhraseNote (2, 0, 64); // E4
        testPlugin.setPhraseNote (2, 1, 65); // F4

        testPlugin.setPhraseStepTimingMultiplier (0, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepTimingMultiplier (1, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepTimingMultiplier (1, 1, PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepTimingMultiplier (2, 0, 1); // 0.5x
        testPlugin.setPhraseStepTimingMultiplier (2, 1, 2); // 0.75x
        testPlugin.setPhraseRowTimingOffset (2, 4); // +0.25 quarters

        for (int row = 0; row < 3; ++row)
        {
            const auto stepCount = testPlugin.getPhraseRowStepCount (row);

            for (int step = 0; step < stepCount; ++step)
            {
                testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
                testPlugin.setPhraseStepVelocity (row, step, 100);
            }
        }

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (120.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        std::array<int, 128> noteOnCounts {};

        for (int block = 0; block < 700; ++block)
        {
            midi.clear();
            playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                          * (120.0 / 60.0) / sampleRate);
            testPlugin.processBlock (buffer, midi);

            for (const auto metadata : midi)
            {
                const auto message = metadata.getMessage();

                if (message.isNoteOn())
                    ++noteOnCounts[static_cast<size_t> (message.getNoteNumber())];
            }

            if (playHead.info.getPpqPosition() >= 8.0)
                break;
        }

        CHECK (noteOnCounts[48] > 0); // C3 bass carrier / echo
        CHECK (noteOnCounts[49] > 0); // C#3 interval echo
        CHECK (noteOnCounts[52] > 0); // E3 = A3 + (C3 - F3) interval echo
        CHECK (noteOnCounts[53] > 0); // F3
        CHECK (noteOnCounts[57] > 0); // A3
        CHECK (noteOnCounts[64] > 0); // E4

        auto totalNoteOns = 0;

        for (const auto count : noteOnCounts)
            totalNoteOns += count;

        CHECK (totalNoteOns >= 30); // preview schedules ~44 note-ons over eight quarters
        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("multiply echo mode notes sustain for step gate length")
    {
        constexpr double sampleRate = 44100.0;
        constexpr int blockSize = 512;

        testPlugin.prepareToPlay (sampleRate, blockSize);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeMultiplyEcho, true);
        testPlugin.setPhraseRowMuted (0, false);
        testPlugin.setPhraseRowMuted (1, false);
        testPlugin.setPhraseRowMuted (2, true);
        testPlugin.setPhraseRowMuted (3, true);

        ensurePhraseRowStepCount (testPlugin, 0, 1);
        ensurePhraseRowStepCount (testPlugin, 1, 1);

        testPlugin.setPhraseNote (0, 0, 57); // A3
        testPlugin.setPhraseNote (1, 0, 53); // F3
        testPlugin.setPhraseStepTimingMultiplier (0, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepTimingMultiplier (1, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setPhraseStepDurationFraction (1, 0, 1.0);
        testPlugin.setPhraseStepVelocity (0, 0, 100);
        testPlugin.setPhraseStepVelocity (1, 0, 100);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (120.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        auto absoluteSample = 0;
        auto noteOnAbsoluteSample = -1;
        auto noteOffAbsoluteSample = -1;
        auto trackedPitch = -1;

        for (int block = 0; block < 96; ++block)
        {
            midi.clear();
            playHead.info.setPpqPosition (static_cast<double> (absoluteSample)
                                          * (120.0 / 60.0) / sampleRate);
            testPlugin.processBlock (buffer, midi);

            for (const auto metadata : midi)
            {
                const auto message = metadata.getMessage();
                const auto eventSample = absoluteSample + metadata.samplePosition;

                if (message.isNoteOn() && noteOnAbsoluteSample < 0)
                {
                    noteOnAbsoluteSample = eventSample;
                    trackedPitch = message.getNoteNumber();
                }

                if (message.isNoteOff() && trackedPitch >= 0
                    && message.getNoteNumber() == trackedPitch && noteOffAbsoluteSample < 0)
                {
                    noteOffAbsoluteSample = eventSample;
                }
            }

            absoluteSample += blockSize;

            if (noteOnAbsoluteSample >= 0 && noteOffAbsoluteSample >= 0)
                break;
        }

        const auto samplesPerQuarter = static_cast<int> (std::lround (sampleRate * 60.0 / 120.0));
        const auto minimumGateSamples = samplesPerQuarter / 4; // 0.25x shortest mod step gate

        CHECK (noteOnAbsoluteSample >= 0);
        CHECK (noteOffAbsoluteSample > noteOnAbsoluteSample);
        CHECK ((noteOffAbsoluteSample - noteOnAbsoluteSample) >= minimumGateSamples);
        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("loop brace snaps to eighth notes")
    {
        CHECK (testPlugin.getLoopBraceStartQuarters()
               == Catch::Approx (PluginProcessor::defaultLoopBraceStartQuarters));
        CHECK (testPlugin.getLoopBraceEndQuarters()
               == Catch::Approx (PluginProcessor::defaultLoopBraceEndQuarters));

        testPlugin.setLoopBraceStartQuarters (0.49);
        testPlugin.setLoopBraceEndQuarters (3.26);

        CHECK (testPlugin.getLoopBraceStartQuarters() == Catch::Approx (0.5));
        CHECK (testPlugin.getLoopBraceEndQuarters() == Catch::Approx (3.5));

        testPlugin.setLoopBraceStartQuarters (3.4);
        CHECK (testPlugin.getLoopBraceStartQuarters() == Catch::Approx (3.0));

        testPlugin.setLoopBraceEndQuarters (3.24);
        CHECK (testPlugin.getLoopBraceEndQuarters() == Catch::Approx (3.5));
    }

    SECTION ("selecting a pattern clears loop slot and disables loop brace")
    {
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.saveCurrentBraceToLoopSlot (0);
        testPlugin.selectLoopSlot (0);

        CHECK (testPlugin.getCurrentLoopSlot() == 0);
        CHECK (testPlugin.isLoopBraceEnabled());

        testPlugin.setCurrentPatternSlot (2);

        CHECK (testPlugin.getCurrentLoopSlot() < 0);
        CHECK_FALSE (testPlugin.isLoopBraceEnabled());
        CHECK_FALSE (testPlugin.isPatternLoopBraceEnabled (2));
    }

    SECTION ("selecting pattern that owns active loop keeps loop slot selected")
    {
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.saveCurrentBraceToLoopSlot (0);
        testPlugin.selectLoopSlot (0);

        testPlugin.setCurrentPatternSlot (0);

        CHECK (testPlugin.getCurrentLoopSlot() == 0);
        CHECK (testPlugin.isLoopBraceEnabled());
    }

    SECTION ("selecting pattern outside active loop owner switches audio immediately")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.saveCurrentBraceToLoopSlot (0);
        testPlugin.selectLoopSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setCurrentPatternSlot (1);

        CHECK (testPlugin.getCurrentLoopSlot() < 0);
        CHECK (testPlugin.getCurrentPatternSlot() == 1);
        CHECK (testPlugin.getAudioPatternSlot() == 1);
        CHECK_FALSE (testPlugin.isPatternLoopBraceEnabled (1));
    }

    SECTION ("loop wrap keeps full gate on repeated passes")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (1.0);
        testPlugin.setLoopBraceEnabled (true);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);

        for (int row = 1; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, true);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        playHead.info.setPpqPosition (0.0);
        testPlugin.processBlock (buffer, midi);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        auto chokedNoteOffAtWrapSample = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                chokedNoteOffAtWrapSample = metadata.samplePosition;
        }

        CHECK (chokedNoteOffAtWrapSample < 0);

        midi.clear();
        playHead.info.setPpqPosition (1.05);
        testPlugin.processBlock (buffer, midi);

        auto earlyNoteOffSample = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                earlyNoteOffSample = metadata.samplePosition;
        }

        CHECK (earlyNoteOffSample < 0);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("loop play from project start maps to brace left edge")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setLoopBraceStartQuarters (8.0);
        testPlugin.setLoopBraceEndQuarters (16.0);
        testPlugin.setLoopBraceEnabled (true);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);

        for (int row = 1; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, true);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        playHead.info.setPpqPosition (0.0);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getPlaybackBeat() == Catch::Approx (8.0).margin (0.05));

        auto noteOnAtBraceStart = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                noteOnAtBraceStart = true;
        }

        CHECK (noteOnAtBraceStart);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("offset loop brace plays from left edge when transport is before brace")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setLoopBraceStartQuarters (8.0);
        testPlugin.setLoopBraceEndQuarters (16.0);
        testPlugin.setLoopBraceEnabled (true);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);

        for (int row = 1; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, true);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        playHead.info.setPpqPosition (0.0);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getPlaybackBeat() == Catch::Approx (8.0).margin (0.05));
        CHECK (midi.getNumEvents() > 0);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("short loop re-triggers after wrap when buffer starts on loop boundary")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 4);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (1.0);
        testPlugin.setLoopBraceEnabled (true);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);

        for (int row = 1; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, true);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        playHead.info.setPpqPosition (0.0);
        testPlugin.processBlock (buffer, midi);

        midi.clear();
        playHead.info.setPpqPosition (1.0);
        testPlugin.processBlock (buffer, midi);

        auto chokedNoteOffAtWrapSample = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                chokedNoteOffAtWrapSample = metadata.samplePosition;
        }

        CHECK (chokedNoteOffAtWrapSample < 0);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("loop brace state loads fractional ranges beyond defaults")
    {
        juce::ValueTree state ("MidiPhrases");
        state.setProperty ("version", 10, nullptr);
        state.setProperty ("loopBraceStart", 20.5, nullptr);
        state.setProperty ("loopBraceEnd", 28.5, nullptr);
        state.setProperty ("loopBraceEnabled", true, nullptr);

        juce::MemoryBlock block;

        if (auto xml = state.createXml())
        {
            juce::MemoryOutputStream stream (block, true);
            xml->writeTo (stream);
        }

        testPlugin.setStateInformation (block.getData(), static_cast<int> (block.getSize()));

        CHECK (testPlugin.isLoopBraceEnabled());
        CHECK (testPlugin.getLoopBraceStartQuarters() == Catch::Approx (20.5));
        CHECK (testPlugin.getLoopBraceEndQuarters() == Catch::Approx (28.5));
    }

    SECTION ("saving current brace to loop slot selects that loop")
    {
        testPlugin.setCurrentPatternSlot (1);
        testPlugin.setLoopBraceStartQuarters (2.0);
        testPlugin.setLoopBraceEndQuarters (4.0);
        testPlugin.saveCurrentBraceToLoopSlot (3);

        CHECK (testPlugin.getCurrentLoopSlot() == 3);
        CHECK (testPlugin.isLoopSlotAssigned (3));
        CHECK (testPlugin.getLoopSlotPatternSlot (3) == 1);
        CHECK (testPlugin.getCurrentPatternSlot() == 1);
        CHECK (testPlugin.isLoopBraceEnabled());
        CHECK (testPlugin.getLoopBraceStartQuarters() == Catch::Approx (2.0));
        CHECK (testPlugin.getLoopBraceEndQuarters() == Catch::Approx (4.0));
    }

    SECTION ("loop slots recall their own saved brace ranges")
    {
        testPlugin.setLoopBraceStartQuarters (0.5);
        testPlugin.setLoopBraceEndQuarters (2.0);
        testPlugin.saveCurrentBraceToLoopSlot (0);

        testPlugin.setCurrentPatternSlot (1);
        testPlugin.setLoopBraceEndQuarters (6.0);
        testPlugin.setLoopBraceStartQuarters (4.0);
        testPlugin.saveCurrentBraceToLoopSlot (1);

        juce::MemoryBlock state;
        testPlugin.getStateInformation (state);

        PluginProcessor reloaded;
        reloaded.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

        reloaded.selectLoopSlot (0);
        CHECK (reloaded.getCurrentPatternSlot() == 0);
        CHECK (reloaded.getCurrentLoopSlot() == 0);
        CHECK (reloaded.isLoopBraceEnabled());
        CHECK (reloaded.getLoopBraceStartQuarters() == Catch::Approx (0.5));
        CHECK (reloaded.getLoopBraceEndQuarters() == Catch::Approx (2.0));

        reloaded.selectLoopSlot (1);
        CHECK (reloaded.getCurrentPatternSlot() == 1);
        CHECK (reloaded.getCurrentLoopSlot() == 1);
        CHECK (reloaded.isLoopBraceEnabled());
        CHECK (reloaded.getLoopBraceStartQuarters() == Catch::Approx (4.0));
        CHECK (reloaded.getLoopBraceEndQuarters() == Catch::Approx (6.0));
    }

    SECTION ("copy pattern slot duplicates pattern content without copying loop slot assignments")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 3);
        ensurePhraseRowStepCount (testPlugin, 1, 2);

        testPlugin.setPhraseRowMuted (0, true);
        testPlugin.setPhraseRowMuted (1, false);
        testPlugin.setPhraseRowTimingOffset (0, 1);
        testPlugin.setPhraseRowTimingOffset (1, 6);
        testPlugin.setPhraseRowMidiChannel (0, 5);
        testPlugin.setPhraseRowMidiChannel (1, 12);

        testPlugin.setPhraseNote (0, 0, 48);
        testPlugin.setPhraseStepVelocity (0, 0, 42);
        testPlugin.setPhraseStepTimingMultiplier (0, 0, 8);
        testPlugin.setPhraseStepDurationFraction (0, 0, 0.5);
        testPlugin.setPhraseStepMuted (0, 0, true);
        testPlugin.setPhraseStepProbability (0, 0, 25);
        testPlugin.setPhraseStepCycle (0, 0, 4);
        testPlugin.setPhraseStepCycleOffset (0, 0, 2);

        testPlugin.setPhraseNote (1, 1, 72);
        testPlugin.setPhraseStepVelocity (1, 1, 96);
        testPlugin.setPhraseStepSkipped (1, 1, true);
        testPlugin.setLoopBraceStartQuarters (1.0);
        testPlugin.setLoopBraceEndQuarters (5.0);
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setPatternScale (6, 7);
        testPlugin.saveCurrentBraceToLoopSlot (0);

        testPlugin.setCurrentPatternSlot (2);
        testPlugin.setLoopBraceStartQuarters (6.0);
        testPlugin.setLoopBraceEndQuarters (7.0);
        testPlugin.saveCurrentBraceToLoopSlot (1);

        testPlugin.copyPatternSlot (0, 1);

        CHECK (testPlugin.getCurrentPatternSlot() == 2);
        CHECK (testPlugin.getLoopSlotPatternSlot (0) == 0);
        CHECK (testPlugin.getLoopSlotPatternSlot (1) == 2);

        CHECK (testPlugin.getPatternPhraseRowStepCount (1, 0) == 3);
        CHECK (testPlugin.getPatternPhraseRowStepCount (1, 1) == 2);
        CHECK (testPlugin.isPatternPhraseRowMuted (1, 0));
        CHECK_FALSE (testPlugin.isPatternPhraseRowMuted (1, 1));
        CHECK (testPlugin.getPatternPhraseRowTimingOffset (1, 0) == 1);
        CHECK (testPlugin.getPatternPhraseRowTimingOffset (1, 1) == 6);
        CHECK (testPlugin.getPatternPhraseRowMidiChannel (1, 0) == 5);
        CHECK (testPlugin.getPatternPhraseRowMidiChannel (1, 1) == 12);

        CHECK (testPlugin.getPatternPhraseNote (1, 0, 0) == 48);
        CHECK (testPlugin.getPatternPhraseStepVelocity (1, 0, 0) == 42);
        CHECK (testPlugin.getPatternPhraseStepTimingMultiplier (1, 0, 0) == 8);
        CHECK (testPlugin.getPatternPhraseStepDurationFraction (1, 0, 0)
               == Catch::Approx (0.5));
        CHECK (testPlugin.isPatternPhraseStepMuted (1, 0, 0));
        CHECK (testPlugin.getPatternPhraseStepProbability (1, 0, 0) == 25);
        CHECK (testPlugin.getPatternPhraseStepCycle (1, 0, 0) == 4);
        CHECK (testPlugin.getPatternPhraseStepCycleOffset (1, 0, 0) == 2);

        CHECK (testPlugin.getPatternPhraseNote (1, 1, 1) == 72);
        CHECK (testPlugin.getPatternPhraseStepVelocity (1, 1, 1) == 96);
        CHECK (testPlugin.isPatternPhraseStepSkipped (1, 1, 1));
        CHECK (testPlugin.isPatternLoopBraceEnabled (1));
        CHECK (testPlugin.getPatternLoopBraceStartQuarters (1) == Catch::Approx (1.0));
        CHECK (testPlugin.getPatternLoopBraceEndQuarters (1) == Catch::Approx (5.0));
        CHECK (testPlugin.getPatternScaleRoot (1) == 6);
        CHECK (testPlugin.getPatternScaleModeIndex (1) == 7);

        testPlugin.copyPatternSlot (1, 1);
        CHECK (testPlugin.getPatternPhraseNote (1, 0, 0) == 48);
        CHECK (testPlugin.getPatternLoopBraceEndQuarters (1) == Catch::Approx (5.0));
        CHECK (testPlugin.getPatternScaleRoot (1) == 6);
    }

    SECTION ("pattern scale mode is pattern state")
    {
        CHECK (testPlugin.getPatternScaleRoot (0) == PluginProcessor::defaultScaleRoot);
        CHECK (testPlugin.getPatternScaleModeIndex (0) == PluginProcessor::defaultScaleModeIndex);

        testPlugin.setPatternScale (3, 5);

        CHECK (testPlugin.getPatternScaleRoot (0) == 3);
        CHECK (testPlugin.getPatternScaleModeIndex (0) == 5);

        testPlugin.setPatternScale (-10, 99);

        CHECK (testPlugin.getPatternScaleRoot (0) == 0);
        CHECK (testPlugin.getPatternScaleModeIndex (0) == PluginProcessor::scaleModeCount - 1);

        testPlugin.setPatternScale (11, 13);

        juce::MemoryBlock state;
        testPlugin.getStateInformation (state);

        PluginProcessor reloaded;
        reloaded.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

        CHECK (reloaded.getPatternScaleRoot (0) == 11);
        CHECK (reloaded.getPatternScaleModeIndex (0) == 13);

        reloaded.clearPatternSlot (0);
        CHECK (reloaded.getPatternScaleRoot (0) == PluginProcessor::defaultScaleRoot);
        CHECK (reloaded.getPatternScaleModeIndex (0) == PluginProcessor::defaultScaleModeIndex);
    }

    SECTION ("insert step after scale change outputs key-center pitch")
    {
        testPlugin.prepareToPlay (44100.0, 512);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setPatternScale (4, 2); // E natural minor
        testPlugin.setPhraseRowMuted (0, false);

        for (int row = 1; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, true);

        testPlugin.insertPhraseStep (0, 0);

        CHECK (testPlugin.getPhraseNote (0, 0) == 64);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (120.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);
        testPlugin.processBlock (buffer, midi);

        int noteOnPitch = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn())
                noteOnPitch = message.getNoteNumber();
        }

        CHECK (noteOnPitch == 64);
        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("phrase rows with different notes output different pitches")
    {
        testPlugin.prepareToPlay (44100.0, 512);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setPatternScale (4, 2); // E natural minor
        testPlugin.setPhraseRowMuted (0, false);
        testPlugin.setPhraseRowMuted (1, false);
        testPlugin.setPhraseRowMuted (2, true);
        testPlugin.setPhraseRowMuted (3, true);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        ensurePhraseRowStepCount (testPlugin, 1, 1);
        testPlugin.setPhraseNote (0, 0, 64); // E3
        testPlugin.setPhraseNote (1, 0, 60); // C3
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setPhraseStepDurationFraction (1, 0, 1.0);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (120.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);
        testPlugin.processBlock (buffer, midi);

        std::array<int, 128> noteOnCounts {};

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn())
                ++noteOnCounts[static_cast<size_t> (message.getNoteNumber())];
        }

        CHECK (noteOnCounts[60] == 1);
        CHECK (noteOnCounts[64] == 1);
        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("row timing offset")
    {
        CHECK (testPlugin.getPhraseRowTimingOffset (0) == PluginProcessor::defaultRowTimingOffsetIndex);

        testPlugin.setPhraseRowTimingOffset (0, 1);
        testPlugin.setPhraseRowTimingOffset (1, 6);

        CHECK (testPlugin.getPhraseRowTimingOffset (0) == 1);
        CHECK (testPlugin.getPhraseRowTimingOffset (1) == 6);
        CHECK (PluginProcessor::rowTimingOffsetForIndex (0)
               < PluginProcessor::rowTimingOffsetForIndex (6));

        testPlugin.setPhraseRowTimingOffset (0, 99);
        CHECK (testPlugin.getPhraseRowTimingOffset (0) == PluginProcessor::rowTimingOffsetCount - 1);
    }

    SECTION ("row MIDI channel")
    {
        for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
            CHECK (testPlugin.getPhraseRowMidiChannel (row)
                   == PluginProcessor::defaultPhraseRowMidiChannelForRow (row));

        testPlugin.setPhraseRowMidiChannel (0, 5);
        testPlugin.setPhraseRowMidiChannel (1, 16);

        CHECK (testPlugin.getPhraseRowMidiChannel (0) == 5);
        CHECK (testPlugin.getPhraseRowMidiChannel (1) == 16);

        testPlugin.setPhraseRowMidiChannel (0, 0);
        CHECK (testPlugin.getPhraseRowMidiChannel (0) == PluginProcessor::minPhraseRowMidiChannel);

        testPlugin.setPhraseRowMidiChannel (0, 99);
        CHECK (testPlugin.getPhraseRowMidiChannel (0) == PluginProcessor::maxPhraseRowMidiChannel);
    }

    SECTION ("step timing multiplier")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);
        ensurePhraseRowStepCount (testPlugin, 2, 4);

        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 0)
               == PluginProcessor::defaultStepTimingMultiplierIndex);

        testPlugin.setPhraseStepTimingMultiplier (0, 1, 0);
        testPlugin.setPhraseStepTimingMultiplier (2, 3, 15);

        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 1) == 0);
        CHECK (testPlugin.getPhraseStepTimingMultiplier (2, 3) == 15);
        CHECK (PluginProcessor::stepTimingMultiplierForIndex (0)
               == Catch::Approx (0.25));
        CHECK (PluginProcessor::stepTimingMultiplierForIndex (3)
               == Catch::Approx (1.0));
        CHECK (PluginProcessor::stepTimingMultiplierForIndex (15)
               == Catch::Approx (4.0));
        CHECK (PluginProcessor::stepTimingMultiplierForIndex (0)
               < PluginProcessor::stepTimingMultiplierForIndex (4));

        testPlugin.setPhraseStepTimingMultiplier (0, 0, 99);
        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 0)
               == PluginProcessor::stepTimingMultiplierCount - 1);
    }

    SECTION ("step duration fraction")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);
        ensurePhraseRowStepCount (testPlugin, 2, 4);

        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0)
               == Catch::Approx (PluginProcessor::defaultStepDurationFraction));

        testPlugin.setPhraseStepDurationFraction (0, 1, 0.0);
        testPlugin.setPhraseStepDurationFraction (2, 3, 0.5);

        CHECK (testPlugin.getPhraseStepDurationFraction (0, 1) == Catch::Approx (0.0));
        CHECK (testPlugin.getPhraseStepDurationFraction (2, 3) == Catch::Approx (0.5));
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0)
               > testPlugin.getPhraseStepDurationFraction (0, 1));

        testPlugin.setPhraseStepDurationFraction (0, 0, 1.5);
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0) == Catch::Approx (1.0));

        testPlugin.setPhraseStepDurationFraction (0, 0, -0.25);
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0) == Catch::Approx (0.0));

        testPlugin.setPhraseStepDurationFraction (0, 0, 0.33);
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0) == Catch::Approx (0.33));
    }

    SECTION ("step duration state version 2 loads continuous fractions")
    {
        juce::ValueTree state ("MidiPhrases");
        state.setProperty ("version", 2, nullptr);

        juce::ValueTree rowTree ("Row");
        rowTree.setProperty ("index", 0, nullptr);
        rowTree.setProperty ("stepCount", 4, nullptr);
        rowTree.setProperty ("duration0", 0.75, nullptr);
        rowTree.setProperty ("duration1", 1.0, nullptr);
        rowTree.setProperty ("duration2", 0.33, nullptr);
        rowTree.setProperty ("duration3", 0.5, nullptr);
        state.appendChild (rowTree, nullptr);

        juce::MemoryBlock destData;

        if (auto xml = state.createXml())
        {
            juce::MemoryOutputStream stream;
            xml->writeTo (stream);
            destData.replaceAll (stream.getData(), stream.getDataSize());
        }

        PluginProcessor reloaded;
        reloaded.setStateInformation (destData.getData(), static_cast<int> (destData.getSize()));

        CHECK (reloaded.getPhraseStepDurationFraction (0, 0) == Catch::Approx (0.75));
        CHECK (reloaded.getPhraseStepDurationFraction (0, 1) == Catch::Approx (1.0));
        CHECK (reloaded.getPhraseStepDurationFraction (0, 2) == Catch::Approx (0.33));
        CHECK (reloaded.getPhraseStepDurationFraction (0, 3) == Catch::Approx (0.5));
    }

    SECTION ("note gate spans audio blocks")
    {
        testPlugin.prepareToPlay (44100.0, 512);
        ensurePhraseRowStepCount (testPlugin, 0, 1);

        for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
        {
            for (int step = 0; step < testPlugin.getPhraseRowStepCount (row); ++step)
            {
                if (row > 0)
                    testPlugin.setPhraseStepVelocity (row, step, 0);

                testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
                testPlugin.setPhraseStepTimingMultiplier (row, step, 3);
            }
        }

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (120.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        int noteOnSample = -1;
        int noteOffSample = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn())
                noteOnSample = metadata.samplePosition;

            if (message.isNoteOff())
                noteOffSample = metadata.samplePosition;
        }

        CHECK (noteOnSample == 0);
        CHECK (noteOffSample < 0);

        midi.clear();
        testPlugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff())
                noteOffSample = metadata.samplePosition;
        }

        CHECK (noteOffSample < 0);

        const auto ppqPerSample = (120.0 / 60.0) / 44100.0;
        const auto expectedGateSamples =
            static_cast<int> (std::lround (1.0 / ppqPerSample));
        int blocksProcessed = 1;

        while (noteOffSample < 0 && blocksProcessed < 64)
        {
            const auto ppq = playHead.info.getPpqPosition().orFallback (0.0)
                             + static_cast<double> (512) * (120.0 / 60.0) / 44100.0;
            playHead.info.setPpqPosition (ppq);
            midi.clear();
            testPlugin.processBlock (buffer, midi);
            ++blocksProcessed;

            for (const auto metadata : midi)
            {
                const auto message = metadata.getMessage();

                if (message.isNoteOff())
                    noteOffSample = metadata.samplePosition;
            }
        }

        CHECK (noteOffSample >= 0);
        CHECK (blocksProcessed > 1);
        CHECK (noteOffSample + blocksProcessed * 512
               == Catch::Approx (expectedGateSamples).margin (2));

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("row order reverse makes former rightmost step play first")
    {
        testPlugin.prepareToPlay (44100.0, 512);

        for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, row != 0);

        ensurePhraseRowStepCount (testPlugin, 0, 2);

        for (int step = 0; step < testPlugin.getPhraseRowStepCount (0); ++step)
        {
            testPlugin.setPhraseStepVelocity (0, step, 100);
            testPlugin.setPhraseStepDurationFraction (0, step, 1.0);
            testPlugin.setPhraseStepTimingMultiplier (
                0,
                step,
                PluginProcessor::defaultStepTimingMultiplierIndex);
        }

        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseNote (0, 1, 72);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (120.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        auto firstNoteOn = [&midi] {
            for (const auto metadata : midi)
            {
                const auto message = metadata.getMessage();

                if (message.isNoteOn())
                    return message.getNoteNumber();
            }

            return -1;
        };

        testPlugin.reversePhraseRowSteps (0);
        testPlugin.processBlock (buffer, midi);
        CHECK (firstNoteOn() == 72);

        testPlugin.reversePhraseRowSteps (0);
        midi.clear();
        playHead.info.setPpqPosition (8.0);
        testPlugin.processBlock (buffer, midi);
        CHECK (firstNoteOn() == 60);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("message-thread edits are applied by the next audio block")
    {
        testPlugin.prepareToPlay (44100.0, 512);

        for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
        {
            for (int step = 0; step < testPlugin.getPhraseRowStepCount (row); ++step)
                testPlugin.setPhraseStepVelocity (row, step, 0);
        }

        ensurePhraseRowStepCount (testPlugin, 0, 1);

        testPlugin.setPhraseNote (0, 0, 73);
        testPlugin.setPhraseStepVelocity (0, 0, 100);

        juce::AudioBuffer<float> buffer (2, 512);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (120.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        auto emittedEditedNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 73)
                emittedEditedNote = true;
        }

        CHECK (emittedEditedNote);
        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI pattern trigger updates UI slot immediately and audio slot on pulse")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setCurrentPatternSlot (2);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setCurrentPatternSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.25);
        testPlugin.setPlayHead (&playHead);

        midi.addEvent (juce::MidiMessage::noteOn (1, 2, static_cast<juce::uint8> (100)), 0);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentPatternSlot() == 2);
        CHECK (testPlugin.getAudioPatternSlot() == 0);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentPatternSlot() == 2);
        CHECK (testPlugin.getAudioPatternSlot() == 2);

        auto emittedSwitchedPatternNote = false;
        auto emittedOriginalPatternNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedSwitchedPatternNote = true;

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedOriginalPatternNote = true;
        }

        CHECK (emittedSwitchedPatternNote);
        CHECK_FALSE (emittedOriginalPatternNote);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI pattern trigger note-offs are consumed")
    {
        testPlugin.prepareToPlay (1000.0, 100);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.25);
        testPlugin.setPlayHead (&playHead);

        midi.addEvent (juce::MidiMessage::noteOn (1, 2, static_cast<juce::uint8> (100)), 0);
        midi.addEvent (juce::MidiMessage::noteOff (1, 2), 10);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentPatternSlot() == 2);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            const auto isLeakedTriggerNote =
                message.isNoteOnOrOff() && message.getNoteNumber() == 2;
            CHECK_FALSE (isLeakedTriggerNote);
        }

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("pending pattern switch applies when plugin block starts on pulse")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setCurrentPatternSlot (2);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setCurrentPatternSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (1.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.setCurrentPatternSlot (2);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getAudioPatternSlot() == 2);

        auto emittedSwitchedPatternNote = false;
        auto emittedOriginalPatternNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedSwitchedPatternNote = true;

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedOriginalPatternNote = true;
        }

        CHECK (emittedSwitchedPatternNote);
        CHECK_FALSE (emittedOriginalPatternNote);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI pattern trigger first beat keeps full gate after pulse switch")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setCurrentPatternSlot (2);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setCurrentPatternSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.25);
        testPlugin.setPlayHead (&playHead);

        midi.addEvent (juce::MidiMessage::noteOn (1, 2, static_cast<juce::uint8> (100)), 0);
        testPlugin.processBlock (buffer, midi);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        auto switchedNoteOnSample = -1;
        auto switchedNoteOffSample = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                switchedNoteOnSample = metadata.samplePosition;

            if (message.isNoteOff() && message.getNoteNumber() == 72)
                switchedNoteOffSample = metadata.samplePosition;
        }

        CHECK (switchedNoteOnSample >= 0);
        CHECK (switchedNoteOffSample < 0);

        midi.clear();
        playHead.info.setPpqPosition (1.05);
        testPlugin.processBlock (buffer, midi);

        auto earlyNoteOffSample = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 72)
                earlyNoteOffSample = metadata.samplePosition;
        }

        CHECK (earlyNoteOffSample < 0);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("pattern switch emits note-off for active long notes")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 4.0);
        testPlugin.setCurrentPatternSlot (2);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setCurrentPatternSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        auto emittedInitialNote = false;
        auto emittedInitialNoteOff = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedInitialNote = true;

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                emittedInitialNoteOff = true;
        }

        CHECK (emittedInitialNote);
        CHECK_FALSE (emittedInitialNoteOff);

        testPlugin.setCurrentPatternSlot (2);
        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        auto emittedSwitchNoteOff = false;
        auto emittedSwitchedPatternNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                emittedSwitchNoteOff = true;

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedSwitchedPatternNote = true;
        }

        CHECK (emittedSwitchNoteOff);
        CHECK (emittedSwitchedPatternNote);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("loop to pattern switch emits note-off for active long notes")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 4.0);
        testPlugin.setCurrentPatternSlot (1);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.saveCurrentBraceToLoopSlot (0);
        testPlugin.selectLoopSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        auto emittedInitialNote = false;
        auto emittedInitialNoteOff = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedInitialNote = true;

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                emittedInitialNoteOff = true;
        }

        CHECK (emittedInitialNote);
        CHECK_FALSE (emittedInitialNoteOff);

        testPlugin.setCurrentPatternSlot (1);
        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        auto emittedSwitchNoteOff = false;
        auto emittedSwitchedPatternNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                emittedSwitchNoteOff = true;

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedSwitchedPatternNote = true;
        }

        CHECK (emittedSwitchNoteOff);
        CHECK (emittedSwitchedPatternNote);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI pattern trigger from loop emits note-off for active long notes")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 4.0);
        testPlugin.setCurrentPatternSlot (1);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.saveCurrentBraceToLoopSlot (0);
        testPlugin.selectLoopSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        auto emittedInitialNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedInitialNote = true;
        }

        CHECK (emittedInitialNote);

        midi.clear();
        playHead.info.setPpqPosition (0.25);
        midi.addEvent (juce::MidiMessage::noteOn (1, 1, static_cast<juce::uint8> (100)), 0);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentPatternSlot() == 1);
        CHECK (testPlugin.getAudioPatternSlot() == 0);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        auto emittedSwitchNoteOff = false;
        auto emittedSwitchedPatternNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                emittedSwitchNoteOff = true;

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedSwitchedPatternNote = true;
        }

        CHECK (emittedSwitchNoteOff);
        CHECK (emittedSwitchedPatternNote);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("loop slot switch applies on pulse boundary like pattern switch")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 4.0);
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (4.0);
        testPlugin.saveCurrentBraceToLoopSlot (0);

        testPlugin.setCurrentPatternSlot (1);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (2.0);
        testPlugin.saveCurrentBraceToLoopSlot (1);

        testPlugin.selectLoopSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        auto emittedInitialNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedInitialNote = true;
        }

        CHECK (emittedInitialNote);
        CHECK (testPlugin.getAudioPatternSlot() == 0);

        testPlugin.selectLoopSlot (1);
        midi.clear();
        playHead.info.setPpqPosition (0.5);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getAudioPatternSlot() == 0);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        auto emittedSwitchNoteOff = false;
        auto emittedSwitchedLoopNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                emittedSwitchNoteOff = true;

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedSwitchedLoopNote = true;
        }

        CHECK (emittedSwitchNoteOff);
        CHECK (emittedSwitchedLoopNote);
        CHECK (testPlugin.getAudioPatternSlot() == 1);
        CHECK (testPlugin.getCurrentLoopSlot() == 1);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI switch to new loop on downbeat plays loop start note")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (4.0);
        testPlugin.saveCurrentBraceToLoopSlot (0);

        testPlugin.setCurrentPatternSlot (1);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (4.0);
        testPlugin.saveCurrentBraceToLoopSlot (1);

        testPlugin.selectLoopSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        playHead.info.setPpqPosition (0.0);
        testPlugin.processBlock (buffer, midi);

        midi.clear();
        playHead.info.setPpqPosition (0.25);
        midi.addEvent (juce::MidiMessage::noteOn (1,
                                                  PluginProcessor::patternSlotCount + 1,
                                                  static_cast<juce::uint8> (100)),
                       0);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentLoopSlot() == 1);
        CHECK (testPlugin.getAudioPatternSlot() == 0);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        auto emittedLoopStartNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedLoopStartNote = true;
        }

        CHECK (emittedLoopStartNote);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI retrigger of active loop on downbeat keeps full gate")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (1.0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);

        for (int row = 1; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, true);

        testPlugin.saveCurrentBraceToLoopSlot (0);
        testPlugin.selectLoopSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        testPlugin.setPlayHead (&playHead);

        playHead.info.setPpqPosition (0.0);
        testPlugin.processBlock (buffer, midi);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        midi.addEvent (juce::MidiMessage::noteOn (1,
                                                  PluginProcessor::patternSlotCount,
                                                  static_cast<juce::uint8> (100)),
                       0);
        testPlugin.processBlock (buffer, midi);

        auto chokedNoteOffAtPulseSample = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                chokedNoteOffAtPulseSample = metadata.samplePosition;
        }

        CHECK (chokedNoteOffAtPulseSample < 0);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI loop trigger matches selectLoopSlot and enables loop brace")
    {
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (4.0);
        testPlugin.saveCurrentBraceToLoopSlot (0);

        testPlugin.setCurrentPatternSlot (1);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (2.0);
        testPlugin.saveCurrentBraceToLoopSlot (1);

        testPlugin.selectLoopSlot (0);
        CHECK (testPlugin.getCurrentLoopSlot() == 0);
        CHECK (testPlugin.isPatternLoopBraceEnabled (0));

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1,
                                                  PluginProcessor::patternSlotCount + 1,
                                                  static_cast<juce::uint8> (100)),
                       0);

        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentLoopSlot() == 1);
        CHECK (testPlugin.getCurrentPatternSlot() == 1);
        CHECK (testPlugin.isLoopBraceEnabled());
        CHECK (testPlugin.isPatternLoopBraceEnabled (1));
        CHECK (testPlugin.getPatternLoopBraceEndQuarters (1) == Catch::Approx (2.0));
    }

    SECTION ("pattern switch at normal quarter boundary does not add duplicate note-offs")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setCurrentPatternSlot (2);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setCurrentPatternSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        testPlugin.setCurrentPatternSlot (2);
        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        auto noteOffCount = 0;
        auto emittedSwitchedPatternNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                ++noteOffCount;

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedSwitchedPatternNote = true;
        }

        CHECK (noteOffCount == 1);
        CHECK (emittedSwitchedPatternNote);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("transport stop emits explicit note-off for active long notes")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 4.0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        midi.clear();
        playHead.info.setIsPlaying (false);
        testPlugin.processBlock (buffer, midi);

        auto emittedExplicitNoteOff = false;
        auto emittedAllNotesOff = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                emittedExplicitNoteOff = true;

            if (message.isAllNotesOff())
                emittedAllNotesOff = true;
        }

        CHECK (emittedExplicitNoteOff);
        CHECK (emittedAllNotesOff);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("swing delays selected pulse subdivisions")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setSwingPercent (100);
        testPlugin.setSwingSubdivisionIndex (2);

        for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, row != 0);

        ensurePhraseRowStepCount (testPlugin, 0, 2);

        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseNote (0, 1, 62);

        for (int step = 0; step < testPlugin.getPhraseRowStepCount (0); ++step)
        {
            testPlugin.setPhraseStepVelocity (0, step, 100);
            testPlugin.setPhraseStepDurationFraction (0, step, 0.1);
            testPlugin.setPhraseStepTimingMultiplier (
                0,
                step,
                PluginProcessor::defaultStepTimingMultiplierIndex);
        }

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (600.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.99);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        int swungNoteOnSample = -1;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 62)
                swungNoteOnSample = metadata.samplePosition;
        }

        CHECK (swungNoteOnSample == Catch::Approx (51).margin (1));
        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("step velocity")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);
        ensurePhraseRowStepCount (testPlugin, 2, 4);

        CHECK (testPlugin.getPhraseStepVelocity (0, 0) == PluginProcessor::defaultStepVelocity);

        testPlugin.setPhraseStepVelocity (0, 1, 64);
        testPlugin.setPhraseStepVelocity (2, 3, 127);

        CHECK (testPlugin.getPhraseStepVelocity (0, 1) == 64);
        CHECK (testPlugin.getPhraseStepVelocity (2, 3) == 127);

        testPlugin.setPhraseStepVelocity (0, 0, 200);
        CHECK (testPlugin.getPhraseStepVelocity (0, 0) == 127);

        testPlugin.setPhraseStepVelocity (0, 0, -5);
        CHECK (testPlugin.getPhraseStepVelocity (0, 0) == 0);
    }

    SECTION ("step velocity zero is stored for silent steps")
    {
        ensurePhraseRowStepCount (testPlugin, 1, 3);

        testPlugin.setPhraseStepVelocity (1, 2, 0);
        CHECK (testPlugin.getPhraseStepVelocity (1, 2) == 0);
    }

    SECTION ("step mute")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);
        ensurePhraseRowStepCount (testPlugin, 2, 4);

        CHECK_FALSE (testPlugin.isPhraseStepMuted (0, 0));

        testPlugin.setPhraseStepMuted (0, 1, true);
        testPlugin.setPhraseStepMuted (2, 3, true);

        CHECK (testPlugin.isPhraseStepMuted (0, 1));
        CHECK (testPlugin.isPhraseStepMuted (2, 3));

        testPlugin.setPhraseStepMuted (0, 1, false);
        CHECK_FALSE (testPlugin.isPhraseStepMuted (0, 1));
    }

    SECTION ("step skip")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);
        ensurePhraseRowStepCount (testPlugin, 2, 4);

        CHECK_FALSE (testPlugin.isPhraseStepSkipped (0, 0));

        testPlugin.setPhraseStepSkipped (0, 1, true);
        testPlugin.setPhraseStepSkipped (2, 3, true);

        CHECK (testPlugin.isPhraseStepSkipped (0, 1));
        CHECK (testPlugin.isPhraseStepSkipped (2, 3));

        testPlugin.setPhraseStepSkipped (0, 1, false);
        CHECK_FALSE (testPlugin.isPhraseStepSkipped (0, 1));
    }

    SECTION ("step mute and skip are mutually exclusive")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);

        testPlugin.setPhraseStepMuted (0, 1, true);
        CHECK (testPlugin.isPhraseStepMuted (0, 1));
        CHECK_FALSE (testPlugin.isPhraseStepSkipped (0, 1));

        testPlugin.setPhraseStepSkipped (0, 1, true);
        CHECK (testPlugin.isPhraseStepSkipped (0, 1));
        CHECK_FALSE (testPlugin.isPhraseStepMuted (0, 1));

        testPlugin.setPhraseStepMuted (0, 1, true);
        CHECK (testPlugin.isPhraseStepMuted (0, 1));
        CHECK_FALSE (testPlugin.isPhraseStepSkipped (0, 1));
    }

    SECTION ("step probability")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);

        CHECK (testPlugin.getPhraseStepProbability (0, 0) == PluginProcessor::defaultStepProbability);

        testPlugin.setPhraseStepProbability (0, 1, 50);
        CHECK (testPlugin.getPhraseStepProbability (0, 1) == 50);

        testPlugin.setPhraseStepProbability (0, 1, 200);
        CHECK (testPlugin.getPhraseStepProbability (0, 1) == 100);
    }

    SECTION ("step cycle")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);

        CHECK (testPlugin.getPhraseStepCycle (0, 0) == PluginProcessor::defaultStepCycle);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 0) == PluginProcessor::defaultStepCycleOffset);

        testPlugin.setPhraseStepCycle (0, 1, 3);
        testPlugin.setPhraseStepCycleOffset (0, 1, 2);

        CHECK (testPlugin.getPhraseStepCycle (0, 1) == 3);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 1) == 2);

        testPlugin.setPhraseStepCycle (0, 1, 2);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 1) == 1);
    }

    SECTION ("remove phrase step")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 3);
        CHECK (testPlugin.getPhraseRowStepCount (0) == 3);

        testPlugin.setPhraseNote (0, 0, 55);
        testPlugin.setPhraseNote (0, 1, 66);
        testPlugin.setPhraseNote (0, 2, 77);

        testPlugin.removePhraseStep (0, 1);

        CHECK (testPlugin.getPhraseRowStepCount (0) == 2);
        CHECK (testPlugin.getPhraseNote (0, 0) == 55);
        CHECK (testPlugin.getPhraseNote (0, 1) == 77);
    }

    SECTION ("insert phrase step")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);
        CHECK (testPlugin.getPhraseRowStepCount (0) == 2);

        testPlugin.setPhraseNote (0, 0, 55);
        testPlugin.setPhraseNote (0, 1, 77);

        testPlugin.insertPhraseStep (0, 1);

        CHECK (testPlugin.getPhraseRowStepCount (0) == 3);
        CHECK (testPlugin.getPhraseNote (0, 0) == 55);
        CHECK (testPlugin.getPhraseNote (0, 1) == 60);
        CHECK (testPlugin.getPhraseNote (0, 2) == 77);
    }

    SECTION ("duplicate phrase step")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);

        testPlugin.setPhraseNote (0, 0, 55);
        testPlugin.setPhraseStepVelocity (0, 0, 42);
        testPlugin.setPhraseStepTimingMultiplier (0, 0, 8);
        testPlugin.setPhraseStepMuted (0, 0, true);
        testPlugin.setPhraseStepSkipped (0, 0, true);
        testPlugin.setPhraseStepProbability (0, 0, 25);
        testPlugin.setPhraseStepCycle (0, 0, 4);
        testPlugin.setPhraseStepCycleOffset (0, 0, 2);
        testPlugin.setPhraseNote (0, 1, 77);

        testPlugin.duplicatePhraseStep (0, 1);

        CHECK (testPlugin.getPhraseRowStepCount (0) == 3);
        CHECK (testPlugin.getPhraseNote (0, 0) == 55);
        CHECK (testPlugin.getPhraseNote (0, 1) == 55);
        CHECK (testPlugin.getPhraseNote (0, 2) == 77);
        CHECK (testPlugin.getPhraseStepVelocity (0, 1) == 42);
        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 1) == 8);
        CHECK_FALSE (testPlugin.isPhraseStepMuted (0, 1));
        CHECK (testPlugin.isPhraseStepSkipped (0, 1));
        CHECK (testPlugin.getPhraseStepProbability (0, 1) == 25);
        CHECK (testPlugin.getPhraseStepCycle (0, 1) == 4);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 1) == 2);
    }

    SECTION ("move phrase step")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 4);

        testPlugin.setPhraseNote (0, 0, 55);
        testPlugin.setPhraseNote (0, 1, 66);
        testPlugin.setPhraseNote (0, 2, 77);
        testPlugin.setPhraseNote (0, 3, 88);
        testPlugin.setPhraseStepVelocity (0, 1, 42);

        testPlugin.movePhraseStep (0, 1, 3);

        CHECK (testPlugin.getPhraseNote (0, 0) == 55);
        CHECK (testPlugin.getPhraseNote (0, 1) == 77);
        CHECK (testPlugin.getPhraseNote (0, 2) == 88);
        CHECK (testPlugin.getPhraseNote (0, 3) == 66);
        CHECK (testPlugin.getPhraseStepVelocity (0, 3) == 42);

        testPlugin.movePhraseStep (0, 3, 0);

        CHECK (testPlugin.getPhraseNote (0, 0) == 66);
        CHECK (testPlugin.getPhraseNote (0, 1) == 55);
        CHECK (testPlugin.getPhraseNote (0, 2) == 77);
        CHECK (testPlugin.getPhraseNote (0, 3) == 88);
    }

    SECTION ("replace phrase row steps")
    {
        std::array<int, PluginProcessor::maxPhraseStepsPerRow> notes {};
        std::array<int, PluginProcessor::maxPhraseStepsPerRow> timingMultiplier {};
        std::array<double, PluginProcessor::maxPhraseStepsPerRow> durationFraction {};
        std::array<int, PluginProcessor::maxPhraseStepsPerRow> velocity {};
        std::array<int, PluginProcessor::maxPhraseStepsPerRow> stepMuted {};
        std::array<int, PluginProcessor::maxPhraseStepsPerRow> stepSkipped {};
        std::array<int, PluginProcessor::maxPhraseStepsPerRow> probability {};
        std::array<int, PluginProcessor::maxPhraseStepsPerRow> cycle {};
        std::array<int, PluginProcessor::maxPhraseStepsPerRow> cycleOffset {};

        notes[0] = 44;
        notes[1] = 155;
        timingMultiplier[0] = 4;
        timingMultiplier[1] = 99;
        durationFraction[0] = 0.5;
        durationFraction[1] = 2.0;
        velocity[0] = 64;
        velocity[1] = 200;
        stepMuted[0] = 1;
        stepMuted[1] = 1;
        stepSkipped[1] = 1;
        probability[0] = 80;
        probability[1] = -10;
        cycle[0] = 4;
        cycle[1] = 100;
        cycleOffset[0] = 2;
        cycleOffset[1] = 100;

        testPlugin.replacePhraseRowSteps (0,
                                          2,
                                          notes,
                                          timingMultiplier,
                                          durationFraction,
                                          velocity,
                                          stepMuted,
                                          stepSkipped,
                                          probability,
                                          cycle,
                                          cycleOffset);

        CHECK (testPlugin.getPhraseRowStepCount (0) == 2);
        CHECK (testPlugin.getPhraseNote (0, 0) == 44);
        CHECK (testPlugin.getPhraseNote (0, 1) == 127);
        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 0) == 4);
        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 1) == 15);
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0) == 0.5);
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 1) == 1.0);
        CHECK (testPlugin.getPhraseStepVelocity (0, 0) == 64);
        CHECK (testPlugin.getPhraseStepVelocity (0, 1) == 127);
        CHECK (testPlugin.isPhraseStepMuted (0, 0));
        CHECK_FALSE (testPlugin.isPhraseStepMuted (0, 1));
        CHECK (testPlugin.isPhraseStepSkipped (0, 1));
        CHECK (testPlugin.getPhraseStepProbability (0, 0) == 80);
        CHECK (testPlugin.getPhraseStepProbability (0, 1) == 0);
        CHECK (testPlugin.getPhraseStepCycle (0, 0) == 4);
        CHECK (testPlugin.getPhraseStepCycle (0, 1) == PluginProcessor::maxStepCycle);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 0) == 2);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 1) == PluginProcessor::maxStepCycle - 1);
    }

    SECTION ("phrase row MIDI recording")
    {
        testPlugin.prepareToPlay (44100.0, 512);

        ensurePhraseRowStepCount (testPlugin, 0, 4);
        testPlugin.setPhraseNote (0, 0, 48);
        testPlugin.setPhraseNote (0, 1, 50);
        testPlugin.setPhraseStepVelocity (0, 0, 42);

        testPlugin.setPhraseRowRecording (0);
        CHECK (testPlugin.getPhraseRowRecording() == 0);

        const auto processNoteOns = [&] (const std::initializer_list<int> notes) {
            juce::MidiBuffer midi;
            int sample = 0;

            for (const auto note : notes)
            {
                midi.addEvent (juce::MidiMessage::noteOn (1, note, static_cast<juce::uint8> (100)), sample);
                ++sample;
            }

            juce::AudioBuffer<float> buffer (2, 512);
            buffer.clear();
            testPlugin.processBlock (buffer, midi);
        };

        processNoteOns ({ 60, 64, 67 });
        auto drained = testPlugin.drainPhraseRowRecordedNotes();
        REQUIRE (drained.size() == 3);
        CHECK (drained[0] == 60);
        CHECK (drained[1] == 64);
        CHECK (drained[2] == 67);

        CHECK (testPlugin.getPhraseRowStepCount (0) == 7);
        CHECK (testPlugin.getPhraseNote (0, 0) == 48);
        CHECK (testPlugin.getPhraseNote (0, 1) == 50);
        CHECK (testPlugin.getPhraseStepVelocity (0, 0) == 42);
        CHECK (testPlugin.getPhraseNote (0, 4) == 60);
        CHECK (testPlugin.getPhraseNote (0, 5) == 64);
        CHECK (testPlugin.getPhraseNote (0, 6) == 67);
        CHECK (testPlugin.getPhraseStepVelocity (0, 4) == PluginProcessor::defaultStepVelocity);
        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 6)
               == PluginProcessor::defaultStepTimingMultiplierIndex);

        {
            juce::MidiBuffer chordMidi;
            chordMidi.addEvent (juce::MidiMessage::noteOn (1, 72, static_cast<juce::uint8> (100)), 0);
            chordMidi.addEvent (juce::MidiMessage::noteOn (1, 74, static_cast<juce::uint8> (100)), 0);
            juce::AudioBuffer<float> buffer (2, 512);
            buffer.clear();
            testPlugin.processBlock (buffer, chordMidi);
        }

        drained = testPlugin.drainPhraseRowRecordedNotes();
        CHECK (drained.isEmpty());
        CHECK (testPlugin.getPhraseRowStepCount (0) == 7);

        testPlugin.setPhraseRowRecording (1);
        CHECK (testPlugin.getPhraseRowRecording() == 1);
    }

    SECTION ("insert phrase step beyond default row length")
    {
        for (int i = 0; i < 3; ++i)
            testPlugin.insertPhraseStep (0, PluginProcessor::defaultPhraseStepsPerRow);

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::defaultPhraseStepsPerRow + 3);
    }

    SECTION ("phrase rows are capped at fixed audio-thread capacity")
    {
        for (int i = 0; i < PluginProcessor::maxPhraseStepsPerRow * 2; ++i)
            testPlugin.insertPhraseStep (0, testPlugin.getPhraseRowStepCount (0));

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::maxPhraseStepsPerRow);
    }

    SECTION ("MIDI mute trigger note 16 stops output immediately")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 4.0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        auto emittedInitialNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedInitialNote = true;
        }

        CHECK (emittedInitialNote);

        midi.clear();
        playHead.info.setPpqPosition (0.25);
        midi.addEvent (juce::MidiMessage::noteOn (1,
                                                  PluginProcessor::midiMuteTriggerNote,
                                                  static_cast<juce::uint8> (100)),
                       0);
        testPlugin.processBlock (buffer, midi);

        auto emittedMuteNoteOff = false;
        auto emittedAfterMuteNoteOn = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOff() && message.getNoteNumber() == 60)
                emittedMuteNoteOff = true;

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedAfterMuteNoteOn = true;
        }

        CHECK (emittedMuteNoteOff);
        CHECK_FALSE (emittedAfterMuteNoteOn);
        CHECK (testPlugin.getCurrentPatternSlot() == -1);
        CHECK_FALSE (testPlugin.isPatternOutputArmed());

        midi.clear();
        playHead.info.setPpqPosition (0.75);
        testPlugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOnOrOff())
                CHECK_FALSE (message.getNoteNumber() == 60);
        }

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI mute trigger note-offs are consumed")
    {
        testPlugin.prepareToPlay (1000.0, 100);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.25);
        testPlugin.setPlayHead (&playHead);

        midi.addEvent (juce::MidiMessage::noteOn (1,
                                                  PluginProcessor::midiMuteTriggerNote,
                                                  static_cast<juce::uint8> (100)),
                       0);
        midi.addEvent (juce::MidiMessage::noteOff (1, PluginProcessor::midiMuteTriggerNote), 10);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentPatternSlot() == -1);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            const auto isLeakedTriggerNote = message.isNoteOnOrOff()
                                             && message.getNoteNumber()
                                                    == PluginProcessor::midiMuteTriggerNote;
            CHECK_FALSE (isLeakedTriggerNote);
        }

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("MIDI mute trigger re-arms on pattern select")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setCurrentPatternSlot (2);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setCurrentPatternSlot (0);

        juce::AudioBuffer<float> buffer (2, 100);
        juce::MidiBuffer midi;

        struct PlayHeadMock : juce::AudioPlayHead
        {
            juce::AudioPlayHead::PositionInfo info;

            juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override
            {
                return info;
            }
        } playHead;

        playHead.info.setBpm (60.0);
        playHead.info.setIsPlaying (true);
        playHead.info.setPpqPosition (0.25);
        testPlugin.setPlayHead (&playHead);

        midi.addEvent (juce::MidiMessage::noteOn (1,
                                                  PluginProcessor::midiMuteTriggerNote,
                                                  static_cast<juce::uint8> (100)),
                       0);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentPatternSlot() == -1);

        midi.clear();
        midi.addEvent (juce::MidiMessage::noteOn (1, 2, static_cast<juce::uint8> (100)), 0);
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getCurrentPatternSlot() == 2);
        CHECK (testPlugin.isPatternOutputArmed());

        auto emittedSwitchedPatternNote = false;
        auto emittedOriginalPatternNote = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn() && message.getNoteNumber() == 72)
                emittedSwitchedPatternNote = true;

            if (message.isNoteOn() && message.getNoteNumber() == 60)
                emittedOriginalPatternNote = true;
        }

        CHECK (emittedSwitchedPatternNote);
        CHECK_FALSE (emittedOriginalPatternNote);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("muted state round-trips through plugin state")
    {
        testPlugin.setCurrentPatternSlot (2);
        testPlugin.deactivatePatternOutput();
        testPlugin.setRowColorsEnabled (true);

        CHECK (testPlugin.getCurrentPatternSlot() == -1);
        CHECK_FALSE (testPlugin.isPatternOutputArmed());
        CHECK (testPlugin.getViewPatternSlot() == 2);

        juce::MemoryBlock state;
        testPlugin.getStateInformation (state);

        PluginProcessor reloaded;
        reloaded.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

        CHECK (reloaded.getCurrentPatternSlot() == -1);
        CHECK_FALSE (reloaded.isPatternOutputArmed());
        CHECK (reloaded.getViewPatternSlot() == 2);
        CHECK (reloaded.isRowColorsEnabled());
    }

    SECTION ("state load clamps phrase rows to fixed audio-thread capacity")
    {
        juce::ValueTree state ("MidiPhrases");
        state.setProperty ("version", 3, nullptr);

        juce::ValueTree rowTree ("Row");
        rowTree.setProperty ("index", 0, nullptr);
        rowTree.setProperty ("stepCount", PluginProcessor::maxPhraseStepsPerRow + 10, nullptr);

        for (int step = 0; step < PluginProcessor::maxPhraseStepsPerRow + 10; ++step)
            rowTree.setProperty ("step" + juce::String (step), 72, nullptr);

        state.appendChild (rowTree, nullptr);

        juce::MemoryBlock destData;

        if (auto xml = state.createXml())
        {
            juce::MemoryOutputStream stream;
            xml->writeTo (stream);
            destData.replaceAll (stream.getData(), stream.getDataSize());
        }

        PluginProcessor reloaded;
        reloaded.setStateInformation (destData.getData(), static_cast<int> (destData.getSize()));

        CHECK (reloaded.getPhraseRowStepCount (0) == PluginProcessor::maxPhraseStepsPerRow);
        CHECK (reloaded.getPhraseNote (0, PluginProcessor::maxPhraseStepsPerRow - 1) == 72);
    }
}


#ifdef PAMPLEJUCE_IPP
    #include <ipp.h>

TEST_CASE ("IPP version", "[ipp]")
{
    #if defined(__APPLE__)
        // macOS uses 2021.9.1 from pip wheel (only x86_64 version available)
        CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2021.9.1 (r0x7e208212)"));
    #else
        CHECK_THAT (ippsGetLibVersion()->Version, Catch::Matchers::Equals ("2026.0.0 (r0xa7ad6ebc)"));
    #endif
}
#endif
