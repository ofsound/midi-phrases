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

TEST_CASE ("Plugin instance", "[instance]")
{
    PluginProcessor testPlugin;

    SECTION ("name")
    {
        CHECK_THAT (testPlugin.getName().toStdString(),
            Catch::Matchers::Equals ("MIDI Phrases"));
    }

    SECTION ("row mute")
    {
        CHECK_FALSE (testPlugin.isPhraseRowMuted (0));

        testPlugin.setPhraseRowMuted (0, true);
        testPlugin.setPhraseRowMuted (1, false);

        CHECK (testPlugin.isPhraseRowMuted (0));
        CHECK_FALSE (testPlugin.isPhraseRowMuted (1));

        testPlugin.setPhraseRowMuted (0, false);
        CHECK_FALSE (testPlugin.isPhraseRowMuted (0));
    }

    SECTION ("row reverse")
    {
        CHECK_FALSE (testPlugin.isPhraseRowReversed (0));

        testPlugin.setPhraseRowReversed (0, true);
        testPlugin.setPhraseRowReversed (1, false);

        CHECK (testPlugin.isPhraseRowReversed (0));
        CHECK_FALSE (testPlugin.isPhraseRowReversed (1));

        testPlugin.setPhraseRowReversed (0, false);
        CHECK_FALSE (testPlugin.isPhraseRowReversed (0));
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
        CHECK (testPlugin.getPhraseRowMidiChannel (0) == PluginProcessor::defaultPhraseRowMidiChannel);

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
        rowTree.setProperty ("stepCount", PluginProcessor::defaultPhraseStepsPerRow, nullptr);
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

    SECTION ("row reverse plays rightmost step first")
    {
        testPlugin.prepareToPlay (44100.0, 512);

        for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, row != 0);

        while (testPlugin.getPhraseRowStepCount (0) > 2)
            testPlugin.removePhraseStep (0, testPlugin.getPhraseRowStepCount (0) - 1);

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

        testPlugin.setPhraseRowReversed (0, true);
        testPlugin.processBlock (buffer, midi);
        CHECK (firstNoteOn() == 72);

        testPlugin.setPhraseRowReversed (0, false);
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

    SECTION ("swing delays selected pulse subdivisions")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setSwingPercent (100);
        testPlugin.setSwingSubdivisionIndex (2);

        for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
            testPlugin.setPhraseRowMuted (row, row != 0);

        while (testPlugin.getPhraseRowStepCount (0) > 2)
            testPlugin.removePhraseStep (0, testPlugin.getPhraseRowStepCount (0) - 1);

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
        testPlugin.setPhraseStepVelocity (1, 2, 0);
        CHECK (testPlugin.getPhraseStepVelocity (1, 2) == 0);
    }

    SECTION ("step mute")
    {
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
        CHECK (testPlugin.getPhraseStepProbability (0, 0) == PluginProcessor::defaultStepProbability);

        testPlugin.setPhraseStepProbability (0, 1, 50);
        CHECK (testPlugin.getPhraseStepProbability (0, 1) == 50);

        testPlugin.setPhraseStepProbability (0, 1, 200);
        CHECK (testPlugin.getPhraseStepProbability (0, 1) == 100);
    }

    SECTION ("step cycle")
    {
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
        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::defaultPhraseStepsPerRow);

        testPlugin.setPhraseNote (0, 0, 55);
        testPlugin.setPhraseNote (0, 1, 66);
        testPlugin.setPhraseNote (0, 2, 77);

        testPlugin.removePhraseStep (0, 1);

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::defaultPhraseStepsPerRow - 1);
        CHECK (testPlugin.getPhraseNote (0, 0) == 55);
        CHECK (testPlugin.getPhraseNote (0, 1) == 77);
    }

    SECTION ("insert phrase step")
    {
        testPlugin.removePhraseStep (0, 3);
        testPlugin.removePhraseStep (0, 2);

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::defaultPhraseStepsPerRow - 2);

        testPlugin.setPhraseNote (0, 0, 55);
        testPlugin.setPhraseNote (0, 1, 77);

        testPlugin.insertPhraseStep (0, 1);

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::defaultPhraseStepsPerRow - 1);
        CHECK (testPlugin.getPhraseNote (0, 0) == 55);
        CHECK (testPlugin.getPhraseNote (0, 1) == 60);
        CHECK (testPlugin.getPhraseNote (0, 2) == 77);
    }

    SECTION ("duplicate phrase step")
    {
        testPlugin.removePhraseStep (0, 3);
        testPlugin.removePhraseStep (0, 2);

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

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::defaultPhraseStepsPerRow - 1);
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
