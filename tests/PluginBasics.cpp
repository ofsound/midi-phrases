#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <cmath>
#include <limits>

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

std::array<int, 128> collectNoteOnsOverQuarters (PluginProcessor& plugin,
                                                 const double sampleRate,
                                                 const int blockSize,
                                                 const double quarters)
{
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
    plugin.setPlayHead (&playHead);

    std::array<int, 128> noteOnCounts {};

    for (int block = 0; block < 700; ++block)
    {
        midi.clear();
        playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                      * (120.0 / 60.0) / sampleRate);
        plugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn())
                ++noteOnCounts[static_cast<size_t> (message.getNoteNumber())];
        }

        if (playHead.info.getPpqPosition() >= quarters)
            break;
    }

    plugin.setPlayHead (nullptr);
    return noteOnCounts;
}

int findNoteOnSample (const juce::MidiBuffer& midiMessages, const int noteNumber)
{
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn() && message.getNoteNumber() == noteNumber)
            return metadata.samplePosition;
    }

    return -1;
}

int findNoteOnSampleOnChannel (const juce::MidiBuffer& midiMessages,
                               const int noteNumber,
                               const int channel)
{
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn()
            && message.getNoteNumber() == noteNumber
            && message.getChannel() == channel)
            return metadata.samplePosition;
    }

    return -1;
}

int findNoteOffSampleOnChannel (const juce::MidiBuffer& midiMessages,
                                const int noteNumber,
                                const int channel)
{
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOff()
            && message.getNoteNumber() == noteNumber
            && message.getChannel() == channel)
        {
            return metadata.samplePosition;
        }
    }

    return -1;
}

int countMidiMessagesForNoteOnChannel (const juce::MidiBuffer& midiMessages,
                                       const int noteNumber,
                                       const int channel,
                                       const bool noteOn)
{
    auto count = 0;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();
        const auto matchesType = noteOn ? message.isNoteOn() : message.isNoteOff();

        if (matchesType
            && message.getNoteNumber() == noteNumber
            && message.getChannel() == channel)
        {
            ++count;
        }
    }

    return count;
}

void configureFourRowOffsetHocketPattern (PluginProcessor& testPlugin)
{
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeHocket, true);

    for (int row = 0; row < 4; ++row)
    {
        testPlugin.setPhraseRowMuted (row, false);
        testPlugin.setPhraseRowTimingOffset (
            row,
            PluginProcessor::defaultRowTimingOffsetIndex + row);
    }

    ensurePhraseRowStepCount (testPlugin, 0, 3);
    ensurePhraseRowStepCount (testPlugin, 1, 3);
    ensurePhraseRowStepCount (testPlugin, 2, 4);
    ensurePhraseRowStepCount (testPlugin, 3, 3);

    const std::array<std::array<int, 4>, 4> notes { {
        { 53, 55, 55, 0 },
        { 57, 53, 58, 0 },
        { 55, 62, 57, 57 },
        { 50, 48, 48, 0 },
    } };

    for (int row = 0; row < 4; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
            testPlugin.setPhraseNote (row, step, notes[static_cast<size_t> (row)][static_cast<size_t> (step)]);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (
                row,
                step,
                PluginProcessor::defaultStepTimingMultiplierIndex);
            testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
            testPlugin.setPhraseStepVelocity (row, step, 100);
        }
    }
}

std::vector<std::pair<int, int>> collectHocketNoteOnEvents (PluginProcessor& testPlugin,
                                                            const double sampleRate,
                                                            const int blockSize,
                                                            const int blockCount)
{
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

    std::vector<std::pair<int, int>> events;
    const auto ppqPerSample = (120.0 / 60.0) / sampleRate;

    for (int block = 0; block < blockCount; ++block)
    {
        midi.clear();
        const auto blockStartPpq = static_cast<double> (block * blockSize) * ppqPerSample;
        playHead.info.setPpqPosition (blockStartPpq);
        testPlugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (! message.isNoteOn())
                continue;

            const auto eventPpq = blockStartPpq + static_cast<double> (metadata.samplePosition) * ppqPerSample;
            events.emplace_back (static_cast<int> (std::lround (eventPpq * 1000.0)),
                                 message.getNoteNumber());
        }
    }

    testPlugin.setPlayHead (nullptr);
    return events;
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
    testPlugin.setPhraseRowTimingOffset (2, 9);

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

TEST_CASE ("Cross-Mod mode follows pattern scale", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeCrossModulation, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 2);
    ensurePhraseRowStepCount (testPlugin, 1, 2);

    testPlugin.setPhraseNote (0, 0, 48); // C3
    testPlugin.setPhraseNote (0, 1, 48); // C3
    testPlugin.setPhraseNote (1, 0, 64); // E4
    testPlugin.setPhraseNote (1, 1, 65); // F4

    for (int row = 0; row < 2; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (row,
                                                        step,
                                                        PluginProcessor::defaultStepTimingMultiplierIndex);
            testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
            testPlugin.setPhraseStepVelocity (row, step, 100);
        }
    }

    testPlugin.setPatternScale (0, 1); // C major
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

    CHECK (noteOnCounts[50] > 0); // D3 scale-degree shift from C3 + (E4 -> F4)
    CHECK (noteOnCounts[49] == 0); // C#3 would be chromatic, not in C major
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Bloom mode adds scale-neighbor ornaments", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 2);

    testPlugin.setPhraseNote (0, 0, 60); // C4
    testPlugin.setPhraseNote (1, 0, 62); // D4
    testPlugin.setPhraseNote (1, 1, 67); // G4

    for (int row = 0; row < 2; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (row,
                                                        step,
                                                        PluginProcessor::defaultStepTimingMultiplierIndex);
            testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
            testPlugin.setPhraseStepVelocity (row, step, 100);
        }
    }

    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeBloom, true);

    const auto noteOnCounts = collectNoteOnsOverQuarters (testPlugin, sampleRate, blockSize, 2.0);

    CHECK (noteOnCounts[60] > 0); // source C4 stays
    CHECK (noteOnCounts[59] > 0); // lower scale-neighbor bloom
    CHECK (noteOnCounts[59] == 1); // short-pulse bloom stays sparse within the half-note gesture
    CHECK (noteOnCounts[62] > 0); // upper scale-neighbor bloom / row source
}

TEST_CASE ("Combination modes merge duplicate same-channel unison attacks", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 256;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeCrossModulation, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowMidiChannel (0, 1);
    testPlugin.setPhraseRowMidiChannel (1, 1);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 60);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 0.25);
        testPlugin.setPhraseStepVelocity (row, 0, row == 0 ? 72 : 104);
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
    playHead.info.setPpqPosition (0.0);
    testPlugin.setPlayHead (&playHead);

    testPlugin.processBlock (buffer, midi);

    CHECK (countMidiMessagesForNoteOnChannel (midi, 60, 1, true) == 1);
    CHECK (countMidiMessagesForNoteOnChannel (midi, 60, 1, false) == 1);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Combination modes suppress same-note retriggers while held", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 900;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeCrossModulation, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowMidiChannel (0, 1);
    testPlugin.setPhraseRowMidiChannel (1, 1);
    testPlugin.setPhraseRowTimingOffset (1, PluginProcessor::defaultRowTimingOffsetIndex + 2);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 60);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
        testPlugin.setPhraseStepCycle (row, 0, 2);
        testPlugin.setPhraseStepCycleOffset (row, 0, 1);
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
    playHead.info.setPpqPosition (0.0);
    testPlugin.setPlayHead (&playHead);

    testPlugin.processBlock (buffer, midi);

    std::vector<int> noteOnSamples;

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn() && message.getNoteNumber() == 60 && message.getChannel() == 1)
            noteOnSamples.push_back (metadata.samplePosition);
    }

    CHECK (noteOnSamples == std::vector<int> { 0 });
    CHECK (countMidiMessagesForNoteOnChannel (midi, 60, 1, true) == 1);
    CHECK (countMidiMessagesForNoteOnChannel (midi, 60, 1, false) == 1);
    CHECK (findNoteOnSampleOnChannel (midi, 60, 1) == 0);
    CHECK (findNoteOffSampleOnChannel (midi, 60, 1) == 750);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Combination modes extend held same-note suppression across audio blocks", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 128;
    constexpr auto ppqPerSample = (120.0 / 60.0) / sampleRate;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeCrossModulation, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowMidiChannel (0, 1);
    testPlugin.setPhraseRowMidiChannel (1, 1);
    testPlugin.setPhraseRowTimingOffset (1, PluginProcessor::defaultRowTimingOffsetIndex + 2);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 60);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
        testPlugin.setPhraseStepCycle (row, 0, 2);
        testPlugin.setPhraseStepCycleOffset (row, 0, 1);
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

    auto noteOnCount = 0;
    auto noteOffCount = 0;
    auto noteOffGlobalSample = -1;

    for (int block = 0; block < 7; ++block)
    {
        midi.clear();
        playHead.info.setPpqPosition (static_cast<double> (block * blockSize) * ppqPerSample);
        testPlugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.getNoteNumber() != 60 || message.getChannel() != 1)
                continue;

            if (message.isNoteOn())
                ++noteOnCount;
            else if (message.isNoteOff())
            {
                ++noteOffCount;
                noteOffGlobalSample = block * blockSize + metadata.samplePosition;
            }
        }
    }

    CHECK (noteOnCount == 1);
    CHECK (noteOffCount == 1);
    CHECK (noteOffGlobalSample == 750);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Combination modes merge overlapping unisons across channels", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeCrossModulation, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowMidiChannel (0, 1);
    testPlugin.setPhraseRowMidiChannel (1, 2);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 60);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
        testPlugin.setPhraseStepCycle (row, 0, 2);
        testPlugin.setPhraseStepCycleOffset (row, 0, 1);
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
    playHead.info.setPpqPosition (0.0);
    testPlugin.setPlayHead (&playHead);

    testPlugin.processBlock (buffer, midi);

    CHECK (countMidiMessagesForNoteOnChannel (midi, 60, 1, true) == 1);
    CHECK (countMidiMessagesForNoteOnChannel (midi, 60, 2, true) == 0);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Counter mode adds offbeat response notes", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 2);

    testPlugin.setPhraseNote (0, 0, 60); // C4
    testPlugin.setPhraseNote (1, 0, 62); // D4
    testPlugin.setPhraseNote (1, 1, 67); // G4

    for (int row = 0; row < 2; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (row,
                                                        step,
                                                        PluginProcessor::defaultStepTimingMultiplierIndex);
            testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
            testPlugin.setPhraseStepVelocity (row, step, 100);
        }
    }

    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeCounter, true);

    const auto noteOnCounts = collectNoteOnsOverQuarters (testPlugin, sampleRate, blockSize, 2.0);

    CHECK (noteOnCounts[60] > 0); // source C4 stays
    CHECK (noteOnCounts[65] > 0); // F4 answer from row 1's D -> G contour
}

TEST_CASE ("Round Robin mode gates rows with overlap note choice", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 2048;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeRoundRobin, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 67);
    testPlugin.setPhraseRowTimingOffset (0, 10); // +0.5 quarters, including transition overlap at 1.5
    testPlugin.setPhraseRowTimingOffset (1, 10);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (row,
                                                    0,
                                                    PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
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
    playHead.info.setPpqPosition (0.0);
    testPlugin.setPlayHead (&playHead);

    testPlugin.processBlock (buffer, midi);

    std::array<int, 128> noteOnCounts {};
    auto totalNoteOns = 0;

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            ++noteOnCounts[static_cast<size_t> (message.getNoteNumber())];
            ++totalNoteOns;
        }
    }

    CHECK (noteOnCounts[60] > 0);
    CHECK (noteOnCounts[67] > 0);
    CHECK (totalNoteOns == 4);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Weave mode trims emitted notes to the next selected attack", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowTimingOffset (0, PluginProcessor::defaultRowTimingOffsetIndex);
    testPlugin.setPhraseRowTimingOffset (1, PluginProcessor::defaultRowTimingOffsetIndex + 2);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 64);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
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
    playHead.info.setPpqPosition (0.0);
    testPlugin.setPlayHead (&playHead);

    testPlugin.processBlock (buffer, midi);

    CHECK (findNoteOnSampleOnChannel (midi, 60, 1) == 0);
    CHECK (findNoteOffSampleOnChannel (midi, 60, 1) == 250);
    CHECK (findNoteOnSampleOnChannel (midi, 64, 2) == 250);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Weave mode releases the previous note across audio block boundaries", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 128;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowTimingOffset (0, PluginProcessor::defaultRowTimingOffsetIndex);
    testPlugin.setPhraseRowTimingOffset (1, PluginProcessor::defaultRowTimingOffsetIndex + 2);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 64);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
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

    playHead.info.setPpqPosition (0.0);
    testPlugin.processBlock (buffer, midi);
    CHECK (findNoteOnSampleOnChannel (midi, 60, 1) == 0);
    CHECK (findNoteOffSampleOnChannel (midi, 60, 1) == -1);

    midi.clear();
    playHead.info.setPpqPosition (static_cast<double> (blockSize) * (120.0 / 60.0) / sampleRate);
    testPlugin.processBlock (buffer, midi);

    CHECK (findNoteOffSampleOnChannel (midi, 60, 1) == 122);
    CHECK (findNoteOnSampleOnChannel (midi, 64, 2) == 122);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Weave mode does not emit delayed followers from the pending queue", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 128;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeRetroInversion, true);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowTimingOffset (0, PluginProcessor::defaultRowTimingOffsetIndex);
    testPlugin.setPhraseRowTimingOffset (1, PluginProcessor::defaultRowTimingOffsetIndex + 2);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 67);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
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

    playHead.info.setPpqPosition (0.0);
    testPlugin.processBlock (buffer, midi);
    CHECK (findNoteOnSampleOnChannel (midi, 60, 1) == 0);

    midi.clear();
    playHead.info.setPpqPosition (static_cast<double> (blockSize) * (120.0 / 60.0) / sampleRate);
    testPlugin.processBlock (buffer, midi);

    CHECK (countMidiMessagesForNoteOnChannel (midi, 67, 2, true) == 1);
    CHECK (findNoteOnSampleOnChannel (midi, 67, 2) == 122);
    CHECK (findNoteOffSampleOnChannel (midi, 67, 2) == -1);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Weave mode keeps selected notes stable across phrase repeats", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 128;
    constexpr int blockCount = 10;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 64);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 0.5);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
    }

    const auto outputEvents = collectHocketNoteOnEvents (
        testPlugin,
        sampleRate,
        blockSize,
        blockCount);

    REQUIRE (outputEvents.size() >= 2);
    CHECK (outputEvents[0].first == 0);
    CHECK (outputEvents[1].first == 1000);
    CHECK (outputEvents[0].second == outputEvents[1].second);
    CHECK (outputEvents[0].second == 64);
}

TEST_CASE ("Hocket mode slices overlapping rows into interlocking handoffs", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 2048;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeHocket, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 67);

    for (int row = 0; row < 2; ++row)
    {
        testPlugin.setPhraseStepTimingMultiplier (row,
                                                    0,
                                                    PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
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
    playHead.info.setPpqPosition (0.0);
    testPlugin.setPlayHead (&playHead);

    testPlugin.processBlock (buffer, midi);

    std::array<int, blockSize> noteOnsBySample {};
    auto totalNoteOns = 0;

    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            ++noteOnsBySample[static_cast<size_t> (metadata.samplePosition)];
            ++totalNoteOns;
        }
    }

    CHECK (noteOnsBySample[0] == 1);
    CHECK (noteOnsBySample[250] == 1);
    CHECK (noteOnsBySample[500] == 1);
    CHECK (noteOnsBySample[750] == 1);

    for (const auto count : noteOnsBySample)
        CHECK (count <= 1);

    CHECK (totalNoteOns >= 4);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Hocket mode keeps four offset rows monophonic in audio output", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 256;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeHocket, true);

    for (int row = 0; row < 4; ++row)
    {
        testPlugin.setPhraseRowMuted (row, false);
        testPlugin.setPhraseRowTimingOffset (
            row,
            PluginProcessor::defaultRowTimingOffsetIndex + row);
        ensurePhraseRowStepCount (testPlugin, row, 1);
        testPlugin.setPhraseNote (row, 0, 60 + row * 2);
        testPlugin.setPhraseStepTimingMultiplier (
            row,
            0,
            PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
        testPlugin.setPhraseStepVelocity (row, 0, 100);
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

    auto maxSimultaneousNoteOns = 0;

    for (int block = 0; block < 16; ++block)
    {
        midi.clear();
        std::array<int, blockSize> noteOnsBySample {};
        playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                      * (120.0 / 60.0) / sampleRate);
        testPlugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (! message.isNoteOn())
                continue;

            ++noteOnsBySample[static_cast<size_t> (metadata.samplePosition)];
            maxSimultaneousNoteOns =
                juce::jmax (maxSimultaneousNoteOns,
                            noteOnsBySample[static_cast<size_t> (metadata.samplePosition)]);
        }
    }

    CHECK (maxSimultaneousNoteOns <= 1);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Hocket mode keeps four offset two-step rows monophonic in audio output", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 256;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeHocket, true);

    for (int row = 0; row < 4; ++row)
    {
        testPlugin.setPhraseRowMuted (row, false);
        testPlugin.setPhraseRowTimingOffset (
            row,
            PluginProcessor::defaultRowTimingOffsetIndex + row);
        ensurePhraseRowStepCount (testPlugin, row, 2);
        testPlugin.setPhraseNote (row, 0, 60 + row * 2);
        testPlugin.setPhraseNote (row, 1, 62 + row * 2);
        for (int step = 0; step < 2; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (
                row,
                step,
                PluginProcessor::defaultStepTimingMultiplierIndex);
            testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
            testPlugin.setPhraseStepVelocity (row, step, 100);
        }
    }

    juce::AudioBuffer<float> buffer (2, blockSize);
    juce::MidiBuffer midi;

    struct PlayHeadMock : juce::AudioPlayHead
    {
        juce::AudioPlayHead::PositionInfo info;
        juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override { return info; }
    } playHead;

    playHead.info.setBpm (120.0);
    playHead.info.setIsPlaying (true);
    testPlugin.setPlayHead (&playHead);

    auto maxSimultaneousNoteOns = 0;

    for (int block = 0; block < 32; ++block)
    {
        midi.clear();
        std::array<int, blockSize> noteOnsBySample {};
        playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                      * (120.0 / 60.0) / sampleRate);
        testPlugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            if (! message.isNoteOn()) continue;
            ++noteOnsBySample[static_cast<size_t> (metadata.samplePosition)];
            maxSimultaneousNoteOns = juce::jmax (
                maxSimultaneousNoteOns,
                noteOnsBySample[static_cast<size_t> (metadata.samplePosition)]);
        }
    }

    CHECK (maxSimultaneousNoteOns <= 1);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Hocket mode keeps four offset multi-step rows monophonic in audio output", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 256;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeHocket, true);

    for (int row = 0; row < 4; ++row)
    {
        testPlugin.setPhraseRowMuted (row, false);
        testPlugin.setPhraseRowTimingOffset (
            row,
            PluginProcessor::defaultRowTimingOffsetIndex + row);
    }

    ensurePhraseRowStepCount (testPlugin, 0, 3);
    ensurePhraseRowStepCount (testPlugin, 1, 3);
    ensurePhraseRowStepCount (testPlugin, 2, 4);
    ensurePhraseRowStepCount (testPlugin, 3, 3);

    const std::array<std::array<int, 4>, 4> notes { {
        { 53, 55, 55, 0 },
        { 57, 53, 58, 0 },
        { 55, 62, 57, 57 },
        { 50, 48, 48, 0 },
    } };

    for (int row = 0; row < 4; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
            testPlugin.setPhraseNote (row, step, notes[static_cast<size_t> (row)][static_cast<size_t> (step)]);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (
                row,
                step,
                PluginProcessor::defaultStepTimingMultiplierIndex);
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

    std::array<int, blockSize> noteOnsBySample {};
    auto maxSimultaneousNoteOns = 0;
    auto shortestAudibleGateSamples = std::numeric_limits<int>::max ();

    for (int block = 0; block < 32; ++block)
    {
        midi.clear();
        noteOnsBySample.fill (0);
        playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                      * (120.0 / 60.0) / sampleRate);
        testPlugin.processBlock (buffer, midi);

        std::array<int, blockSize> noteOnNotes {};
        noteOnNotes.fill (-1);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.isNoteOn())
            {
                const auto sample = metadata.samplePosition;
                ++noteOnsBySample[static_cast<size_t> (sample)];
                noteOnNotes[static_cast<size_t> (sample)] = message.getNoteNumber();
                maxSimultaneousNoteOns =
                    juce::jmax (maxSimultaneousNoteOns, noteOnsBySample[static_cast<size_t> (sample)]);
            }

            if (message.isNoteOff())
            {
                const auto sample = metadata.samplePosition;

                for (int onSample = 0; onSample < sample; ++onSample)
                {
                    if (noteOnNotes[static_cast<size_t> (onSample)] == message.getNoteNumber())
                    {
                        shortestAudibleGateSamples =
                            juce::jmin (shortestAudibleGateSamples, sample - onSample);
                        noteOnNotes[static_cast<size_t> (onSample)] = -1;
                    }
                }
            }
        }
    }

    for (const auto count : noteOnsBySample)
        CHECK (count <= 1);

    CHECK (maxSimultaneousNoteOns <= 1);
    CHECK (shortestAudibleGateSamples > 50);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Hocket mode matches output across block sizes", "[instance]")
{
    constexpr double sampleRate = 1000.0;
    constexpr int smallBlockSize = 256;
    constexpr int largeBlockSize = 2048;
    constexpr int blockCount = 32;

    PluginProcessor smallBlockPlugin;
    smallBlockPlugin.prepareToPlay (sampleRate, smallBlockSize);
    configureFourRowOffsetHocketPattern (smallBlockPlugin);
    const auto smallBlockEvents =
        collectHocketNoteOnEvents (smallBlockPlugin, sampleRate, smallBlockSize, blockCount);

    PluginProcessor largeBlockPlugin;
    largeBlockPlugin.prepareToPlay (sampleRate, largeBlockSize);
    configureFourRowOffsetHocketPattern (largeBlockPlugin);
    const auto largeBlockEvents =
        collectHocketNoteOnEvents (largeBlockPlugin, sampleRate, largeBlockSize, 4);

    REQUIRE (smallBlockEvents.size() > 0);
    REQUIRE (largeBlockEvents.size() > 0);

    const auto compareCount =
        juce::jmin (smallBlockEvents.size(), largeBlockEvents.size());

    for (size_t index = 0; index < static_cast<size_t> (compareCount); ++index)
    {
        CHECK (smallBlockEvents[index].first == largeBlockEvents[index].first);
        CHECK (smallBlockEvents[index].second == largeBlockEvents[index].second);
    }
}

TEST_CASE ("Hocket mode matches preview schedule for four offset rows", "[instance]")
{
    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 256;
    constexpr int blockCount = 32;

    PluginProcessor testPlugin;
    testPlugin.prepareToPlay (sampleRate, blockSize);
    configureFourRowOffsetHocketPattern (testPlugin);

    const auto outputEvents =
        collectHocketNoteOnEvents (testPlugin, sampleRate, blockSize, blockCount);

    std::vector<std::pair<int, int>> filteredEvents;
    filteredEvents.reserve (outputEvents.size());

    for (const auto& event : outputEvents)
    {
        if (event.first < 16000)
            filteredEvents.push_back (event);
    }

    const std::array<std::pair<int, int>, 64> expectedEvents { {
        { 0, 53 },
        { 250, 53 },
        { 500, 57 },
        { 750, 57 },
        { 1000, 55 },
        { 1250, 55 },
        { 1500, 62 },
        { 1750, 48 },
        { 2000, 48 },
        { 2250, 62 },
        { 2500, 58 },
        { 2750, 58 },
        { 3000, 53 },
        { 3250, 53 },
        { 3500, 57 },
        { 3750, 50 },
        { 4000, 50 },
        { 4250, 57 },
        { 4500, 50 },
        { 4750, 53 },
        { 5000, 55 },
        { 5250, 58 },
        { 5500, 48 },
        { 5750, 62 },
        { 6000, 62 },
        { 6250, 48 },
        { 6500, 57 },
        { 6750, 57 },
        { 7000, 57 },
        { 7250, 50 },
        { 7500, 55 },
        { 7750, 55 },
        { 8000, 53 },
        { 8250, 58 },
        { 8500, 48 },
        { 8750, 48 },
        { 9000, 55 },
        { 9250, 57 },
        { 9500, 48 },
        { 9750, 53 },
        { 10000, 55 },
        { 10250, 62 },
        { 10500, 50 },
        { 10750, 57 },
        { 11000, 48 },
        { 11250, 58 },
        { 11500, 55 },
        { 11750, 57 },
        { 12000, 58 },
        { 12250, 48 },
        { 12500, 55 },
        { 12750, 50 },
        { 13000, 55 },
        { 13250, 55 },
        { 13500, 55 },
        { 13750, 53 },
        { 14000, 55 },
        { 14250, 55 },
        { 14500, 55 },
        { 14750, 58 },
        { 15000, 48 },
        { 15250, 57 },
        { 15500, 57 },
        { 15750, 53 },
    } };

    REQUIRE (filteredEvents.size() == expectedEvents.size());

    for (size_t index = 0; index < expectedEvents.size(); ++index)
    {
        CHECK (filteredEvents[index].first == expectedEvents[index].first);
        CHECK (filteredEvents[index].second == expectedEvents[index].second);
    }
}

TEST_CASE ("Hocket mode drops tiny slice overlaps", "[instance]")
{
    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 512;
    constexpr int blockCount = 2;

    PluginProcessor testPlugin;
    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeHocket, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (1, 0, 67);
    testPlugin.setPhraseStepTimingMultiplier (
        0,
        0,
        PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepTimingMultiplier (
        1,
        0,
        PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepDurationFraction (0, 0, 0.2);
    testPlugin.setPhraseStepDurationFraction (1, 0, 1.0);
    testPlugin.setPhraseStepVelocity (0, 0, 100);
    testPlugin.setPhraseStepVelocity (1, 0, 0);

    const auto outputEvents =
        collectHocketNoteOnEvents (testPlugin, sampleRate, blockSize, blockCount);

    CHECK (outputEvents.empty());
}

TEST_CASE ("Hocket mode matches output across DAW-sized blocks", "[instance]")
{
    constexpr double sampleRate = 44100.0;
    constexpr int smallBlockSize = 512;
    constexpr int largeBlockSize = 4096;
    constexpr int blockCount = 128;

    PluginProcessor smallBlockPlugin;
    smallBlockPlugin.prepareToPlay (sampleRate, smallBlockSize);
    configureFourRowOffsetHocketPattern (smallBlockPlugin);
    const auto smallBlockEvents =
        collectHocketNoteOnEvents (smallBlockPlugin, sampleRate, smallBlockSize, blockCount);

    PluginProcessor largeBlockPlugin;
    largeBlockPlugin.prepareToPlay (sampleRate, largeBlockSize);
    configureFourRowOffsetHocketPattern (largeBlockPlugin);
    const auto largeBlockEvents =
        collectHocketNoteOnEvents (largeBlockPlugin, sampleRate, largeBlockSize, 16);

    REQUIRE (smallBlockEvents.size() > 0);
    REQUIRE (largeBlockEvents.size() > 0);

    const auto compareCount =
        juce::jmin (smallBlockEvents.size(), largeBlockEvents.size());

    for (size_t index = 0; index < static_cast<size_t> (compareCount); ++index)
    {
        CHECK (smallBlockEvents[index].first == largeBlockEvents[index].first);
        CHECK (smallBlockEvents[index].second == largeBlockEvents[index].second);
    }
}

TEST_CASE ("Canon mode adds delayed scale-aware followers", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 2048;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeCanon, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowMidiChannel (1, 12);

    ensurePhraseRowStepCount (testPlugin, 0, 2);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (0, 1, 64);
    testPlugin.setPhraseNote (1, 0, 67);

    for (int row = 0; row < 2; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (
                row,
                step,
                PluginProcessor::defaultStepTimingMultiplierIndex);
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
    playHead.info.setPpqPosition (0.0);
    testPlugin.setPlayHead (&playHead);

    testPlugin.processBlock (buffer, midi);

    CHECK (findNoteOnSampleOnChannel (midi, 71, 12) == 1000);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Canon mode emits followers scheduled on block boundary", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 1000;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeCanon, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowMidiChannel (1, 12);

    ensurePhraseRowStepCount (testPlugin, 0, 2);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (0, 1, 64);
    testPlugin.setPhraseNote (1, 0, 67);

    for (int row = 0; row < 2; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (
                row,
                step,
                PluginProcessor::defaultStepTimingMultiplierIndex);
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

    playHead.info.setPpqPosition (0.0);
    midi.clear();
    testPlugin.processBlock (buffer, midi);

    playHead.info.setPpqPosition (2.0);
    midi.clear();
    testPlugin.processBlock (buffer, midi);

    CHECK (findNoteOnSampleOnChannel (midi, 71, 12) == 0);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Retro-Inv mode adds reversed inverted followers", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 1000.0;
    constexpr int blockSize = 2048;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPatternScale (0, 1); // C major
    testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeRetroInversion, true);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, false);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    testPlugin.setPhraseRowMidiChannel (1, 12);

    ensurePhraseRowStepCount (testPlugin, 0, 3);
    ensurePhraseRowStepCount (testPlugin, 1, 1);

    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseNote (0, 1, 64);
    testPlugin.setPhraseNote (0, 2, 67);
    testPlugin.setPhraseNote (1, 0, 72);

    for (int row = 0; row < 2; ++row)
    {
        const auto stepCount = testPlugin.getPhraseRowStepCount (row);

        for (int step = 0; step < stepCount; ++step)
        {
            testPlugin.setPhraseStepTimingMultiplier (
                row,
                step,
                PluginProcessor::defaultStepTimingMultiplierIndex);
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
    playHead.info.setPpqPosition (0.0);
    testPlugin.setPlayHead (&playHead);

    testPlugin.processBlock (buffer, midi);

    CHECK (findNoteOnSampleOnChannel (midi, 65, 12) == 250);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Pattern note bandpass filters scheduled output", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, true);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 2);
    testPlugin.setPhraseNote (0, 0, 48); // C3
    testPlugin.setPhraseNote (0, 1, 60); // C4

    for (int step = 0; step < 2; ++step)
    {
        testPlugin.setPhraseStepTimingMultiplier (0,
                                                    step,
                                                    PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (0, step, 1.0);
        testPlugin.setPhraseStepVelocity (0, step, 100);
    }

    testPlugin.setPatternNoteBandpass (60, 60); // C4 only

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

    CHECK (noteOnCounts[60] > 0);
    CHECK (noteOnCounts[48] == 0);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Pattern velocity tilt adjusts filtered scheduled output", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, true);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 3);
    testPlugin.setPhraseNote (0, 0, 48); // C3
    testPlugin.setPhraseNote (0, 1, 60); // C4
    testPlugin.setPhraseNote (0, 2, 72); // C5

    for (int step = 0; step < 3; ++step)
    {
        testPlugin.setPhraseStepTimingMultiplier (0,
                                                    step,
                                                    PluginProcessor::defaultStepTimingMultiplierIndex);
        testPlugin.setPhraseStepDurationFraction (0, step, 1.0);
        testPlugin.setPhraseStepVelocity (0, step, 100);
    }

    testPlugin.setPatternNoteBandpass (60, 72);
    testPlugin.setPatternVelocityTiltPivotMidi (60);
    testPlugin.setPatternVelocityTiltAmount (12);

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
    std::array<int, 128> noteOnVelocities {};

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
            {
                const auto note = message.getNoteNumber();
                ++noteOnCounts[static_cast<size_t> (note)];
                noteOnVelocities[static_cast<size_t> (note)] = message.getVelocity();
            }
        }

        if (playHead.info.getPpqPosition() >= 8.0)
            break;
    }

    CHECK (noteOnCounts[48] == 0);
    CHECK (noteOnCounts[60] > 0);
    CHECK (noteOnCounts[72] > 0);
    CHECK (noteOnVelocities[60] == 100);
    CHECK (noteOnVelocities[72] == 112);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Pattern global transpose is the final scheduled pitch transform", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, true);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);
    ensurePhraseRowStepCount (testPlugin, 0, 1);
    testPlugin.setPhraseNote (0, 0, 60);
    testPlugin.setPhraseStepVelocity (0, 0, 100);
    testPlugin.setPatternNoteBandpass (60, 60);
    testPlugin.setPatternGlobalTransposeSemitones (5);

    const auto noteOnCounts = collectNoteOnsOverQuarters (testPlugin, sampleRate, blockSize, 2.0);

    CHECK (testPlugin.getPatternGlobalTransposeSemitones (0) == 5);
    CHECK (noteOnCounts[65] > 0);
    CHECK (noteOnCounts[60] == 0);
}

TEST_CASE ("Pattern octavizer duplicates scheduled notes by octave", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, true);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    testPlugin.setPhraseNote (0, 0, 60); // C4

    testPlugin.setPhraseStepTimingMultiplier (0,
                                                0,
                                                PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
    testPlugin.setPhraseStepVelocity (0, 0, 100);

    testPlugin.setPatternOctavizerDown8vaEnabled (true);
    testPlugin.setPatternOctavizerUp8vaEnabled (true);
    testPlugin.setPatternOctavizerDown8vaRelativeVelocity (-10);
    testPlugin.setPatternOctavizerUp8vaRelativeVelocity (10);

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
    std::array<int, 128> noteOnVelocities {};

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
            {
                const auto note = message.getNoteNumber();
                ++noteOnCounts[static_cast<size_t> (note)];
                noteOnVelocities[static_cast<size_t> (note)] = message.getVelocity();
            }
        }

        if (playHead.info.getPpqPosition() >= 8.0)
            break;
    }

    CHECK (noteOnCounts[60] > 0);
    CHECK (noteOnCounts[48] > 0);
    CHECK (noteOnCounts[72] > 0);
    CHECK (noteOnVelocities[60] == 100);
    CHECK (noteOnVelocities[48] == 90);
    CHECK (noteOnVelocities[72] == 110);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Pattern octavizer keeps original row note sounding", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, true);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    testPlugin.setPhraseNote (0, 0, 60);

    testPlugin.setPhraseStepTimingMultiplier (0,
                                                0,
                                                PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
    testPlugin.setPhraseStepVelocity (0, 0, 100);

    testPlugin.setPatternOctavizerDown8vaEnabled (true);
    testPlugin.setPatternOctavizerUp8vaEnabled (true);

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

    auto sawOriginalNoteOn = false;
    auto foundImmediateOriginalCutoff = false;

    for (int block = 0; block < 700; ++block)
    {
        midi.clear();
        playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                      * (120.0 / 60.0) / sampleRate);
        testPlugin.processBlock (buffer, midi);

        auto originalNoteOnSample = -1;
        auto sawOriginalNoteOnInBlock = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.getNoteNumber() != 60)
                continue;

            if (message.isNoteOn())
            {
                sawOriginalNoteOn = true;
                sawOriginalNoteOnInBlock = true;
                originalNoteOnSample = metadata.samplePosition;
            }
            else if (sawOriginalNoteOnInBlock
                     && metadata.samplePosition == originalNoteOnSample)
            {
                foundImmediateOriginalCutoff = true;
                break;
            }
        }

        if (foundImmediateOriginalCutoff)
            break;
    }

    CHECK (sawOriginalNoteOn);
    CHECK_FALSE (foundImmediateOriginalCutoff);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Pattern octavizer velocity tweaks do not flush sounding notes", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, true);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    testPlugin.setPhraseNote (0, 0, 60);

    testPlugin.setPhraseStepTimingMultiplier (0,
                                                0,
                                                PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
    testPlugin.setPhraseStepVelocity (0, 0, 100);
    testPlugin.setPatternOctavizerDown8vaEnabled (true);

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

    auto sawOriginalNoteOn = false;
    auto foundImmediateOriginalCutoff = false;

    for (int block = 0; block < 700; ++block)
    {
        if (block == 44)
            testPlugin.setPatternOctavizerDown8vaRelativeVelocity (-36);

        midi.clear();
        playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                      * (120.0 / 60.0) / sampleRate);
        testPlugin.processBlock (buffer, midi);

        auto originalNoteOnSample = -1;
        auto sawOriginalNoteOnInBlock = false;

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();

            if (message.getNoteNumber() != 60)
                continue;

            if (message.isNoteOn())
            {
                sawOriginalNoteOn = true;
                sawOriginalNoteOnInBlock = true;
                originalNoteOnSample = metadata.samplePosition;
            }
            else if (sawOriginalNoteOnInBlock
                     && metadata.samplePosition == originalNoteOnSample)
            {
                foundImmediateOriginalCutoff = true;
                break;
            }
        }

        if (foundImmediateOriginalCutoff)
            break;
    }

    CHECK (sawOriginalNoteOn);
    CHECK_FALSE (foundImmediateOriginalCutoff);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Pattern shimmer adds delayed octave-up taps", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, true);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    testPlugin.setPhraseNote (0, 0, 60); // C4

    testPlugin.setPhraseStepTimingMultiplier (0,
                                                0,
                                                PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
    testPlugin.setPhraseStepVelocity (0, 0, 100);

    testPlugin.setPatternShimmerEnabled (true);
    testPlugin.setPatternShimmerDelayMultiplierIndex (
        PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPatternShimmerFeedbackPercent (80);
    testPlugin.setPatternShimmerMixPercent (PluginProcessor::maxPercentValue);

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
    std::array<int, 128> noteOnVelocities {};

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
            {
                const auto note = message.getNoteNumber();
                ++noteOnCounts[static_cast<size_t> (note)];
                noteOnVelocities[static_cast<size_t> (note)] = message.getVelocity();
            }
        }

        if (playHead.info.getPpqPosition() >= 8.0)
            break;
    }

    CHECK (noteOnCounts[60] > 0);
    CHECK (noteOnCounts[72] > 0);
    CHECK (noteOnVelocities[60] == 100);
    CHECK (noteOnVelocities[72] == 79);
    testPlugin.setPlayHead (nullptr);
}

TEST_CASE ("Pattern note bandpass filters shimmer taps", "[instance]")
{
    PluginProcessor testPlugin;

    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 512;

    testPlugin.prepareToPlay (sampleRate, blockSize);
    testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
    testPlugin.setCurrentPatternSlot (0);
    testPlugin.setPhraseRowMuted (0, false);
    testPlugin.setPhraseRowMuted (1, true);
    testPlugin.setPhraseRowMuted (2, true);
    testPlugin.setPhraseRowMuted (3, true);

    ensurePhraseRowStepCount (testPlugin, 0, 1);
    testPlugin.setPhraseNote (0, 0, 48); // C3

    testPlugin.setPhraseStepTimingMultiplier (0,
                                                0,
                                                PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
    testPlugin.setPhraseStepVelocity (0, 0, 100);

    testPlugin.setPatternNoteBandpass (36, 57); // A#0..A3
    testPlugin.setPatternShimmerEnabled (true);
    testPlugin.setPatternShimmerDelayMultiplierIndex (
        PluginProcessor::defaultStepTimingMultiplierIndex);
    testPlugin.setPatternShimmerFeedbackPercent (80);
    testPlugin.setPatternShimmerMixPercent (PluginProcessor::maxPercentValue);

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

    CHECK (noteOnCounts[48] > 0);
    CHECK (noteOnCounts[60] == 0);
    CHECK (noteOnCounts[72] == 0);
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
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 3) == 1);

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

        CHECK (testPlugin.getSwingPercent() == PluginProcessor::maxPercentValue);
        CHECK (testPlugin.getVelocityHumanizePercent() == 64);
        CHECK (testPlugin.getTimingHumanizePercent() == 0);
        CHECK (testPlugin.getSwingSubdivisionIndex()
               == PluginProcessor::swingSubdivisionCount - 1);
    }

    SECTION ("row color UI state")
    {
        CHECK (testPlugin.isRowColorsEnabled());

        testPlugin.setRowColorsEnabled (false);
        CHECK_FALSE (testPlugin.isRowColorsEnabled());

        testPlugin.setRowColorsEnabled (true);
        CHECK (testPlugin.isRowColorsEnabled());
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

    SECTION ("weave mode picks same winner as preview for same-time collisions")
    {
        testPlugin.prepareToPlay (44100.0, 512);
        testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setVelocityHumanizePercent (0);
        testPlugin.setTimingHumanizePercent (0);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, true);

        for (int row = 0; row < 4; ++row)
        {
            testPlugin.setPhraseRowMuted (row, false);
            ensurePhraseRowStepCount (testPlugin, row, 1);
            testPlugin.setPhraseNote (row, 0, 60 + row * 2);
            testPlugin.setPhraseStepTimingMultiplier (
                row, 0, PluginProcessor::defaultStepTimingMultiplierIndex);
            testPlugin.setPhraseStepDurationFraction (row, 0, 1.0);
            testPlugin.setPhraseStepVelocity (row, 0, 100);
            testPlugin.setPhraseStepProbability (row, 0, 100);
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

        CHECK (noteOnCounts[66] == 1);
        CHECK (noteOnCounts[60] == 0);
        CHECK (noteOnCounts[62] == 0);
        CHECK (noteOnCounts[64] == 0);
        testPlugin.setPlayHead (nullptr);
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
        testPlugin.setPhraseRowTimingOffset (2, 9); // +0.25 quarters

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
        testPlugin.setPhraseRowTimingOffset (2, 9); // +0.25 quarters

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

    SECTION ("combination mode with timing humanize pairs note-offs with note-ons")
    {
        constexpr double sampleRate = 44100.0;
        constexpr int blockSize = 512;

        testPlugin.prepareToPlay (sampleRate, blockSize);
        testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setTimingHumanizePercent (100);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeMultiplyEcho, true);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, true);
        testPlugin.setPhraseRowMuted (0, false);
        testPlugin.setPhraseRowMuted (1, false);
        testPlugin.setPhraseRowMuted (2, true);
        testPlugin.setPhraseRowMuted (3, true);

        ensurePhraseRowStepCount (testPlugin, 0, 3);
        ensurePhraseRowStepCount (testPlugin, 1, 4);

        testPlugin.setPhraseNote (0, 0, 38); // D2
        testPlugin.setPhraseNote (0, 1, 41); // F2
        testPlugin.setPhraseNote (0, 2, 48); // C3
        testPlugin.setPhraseNote (1, 0, 53); // F3
        testPlugin.setPhraseNote (1, 1, 53);
        testPlugin.setPhraseNote (1, 2, 55); // G3
        testPlugin.setPhraseNote (1, 3, 50); // D3

        for (int row = 0; row < 2; ++row)
        {
            const auto stepCount = testPlugin.getPhraseRowStepCount (row);

            for (int step = 0; step < stepCount; ++step)
            {
                testPlugin.setPhraseStepTimingMultiplier (
                    row, step, PluginProcessor::defaultStepTimingMultiplierIndex);
                testPlugin.setPhraseStepDurationFraction (row, step, 1.0);
                testPlugin.setPhraseStepVelocity (row, step, 100);
                testPlugin.setPhraseStepProbability (row, step, 100);
            }
        }

        testPlugin.setPhraseStepTimingMultiplier (1, 0, 1);
        testPlugin.setPhraseStepTimingMultiplier (1, 1, 1);
        testPlugin.setPhraseStepTimingMultiplier (1, 2, 2);
        testPlugin.setPhraseStepTimingMultiplier (1, 3, 3);

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
        std::array<int, 128> noteOffCounts {};

        for (int block = 0; block < 1200; ++block)
        {
            midi.clear();
            playHead.info.setPpqPosition (static_cast<double> (block * blockSize)
                                          * (120.0 / 60.0) / sampleRate);
            testPlugin.processBlock (buffer, midi);

            for (const auto metadata : midi)
            {
                const auto message = metadata.getMessage();
                const auto note = message.getNoteNumber();

                if (message.isNoteOn())
                    ++noteOnCounts[static_cast<size_t> (note)];

                if (message.isNoteOff())
                    ++noteOffCounts[static_cast<size_t> (note)];
            }

            if (playHead.info.getPpqPosition() >= 12.0)
                break;
        }

        playHead.info.setIsPlaying (false);
        midi.clear();
        testPlugin.processBlock (buffer, midi);

        for (const auto metadata : midi)
        {
            const auto message = metadata.getMessage();
            const auto note = message.getNoteNumber();

            if (message.isNoteOn())
                ++noteOnCounts[static_cast<size_t> (note)];

            if (message.isNoteOff())
                ++noteOffCounts[static_cast<size_t> (note)];
        }

        for (int note = 0; note < 128; ++note)
        {
            if (noteOnCounts[static_cast<size_t> (note)] == 0)
                continue;

            CHECK (noteOffCounts[static_cast<size_t> (note)]
                   == noteOnCounts[static_cast<size_t> (note)]);
        }

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

    SECTION ("project metadata and display settings round-trip with processor state")
    {
        testPlugin.setProjectMetadata ("Night Drive",
                                       "Muted polyrhythms for the bridge",
                                       "2026-06-20T19:39:30-06:00",
                                       "2026-06-20T20:10:00-06:00",
                                       "alt",
                                       80,
                                       true);
        testPlugin.setStandaloneTempoBpm (137.5);

        juce::MemoryBlock state;
        testPlugin.getStateInformation (state);

        PluginProcessor reloaded;
        reloaded.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

        CHECK (reloaded.getProjectName() == "Night Drive");
        CHECK (reloaded.getProjectDescription() == "Muted polyrhythms for the bridge");
        CHECK (reloaded.getProjectCreatedAt() == "2026-06-20T19:39:30-06:00");
        CHECK (reloaded.getProjectModifiedAt() == "2026-06-20T20:10:00-06:00");
        CHECK (reloaded.getProjectThemeMode() == "alt");
        CHECK (reloaded.getProjectUiScalePercent() == 80);
        CHECK (reloaded.getProjectStretchStepsToFit());
        CHECK (reloaded.getStandaloneTempoBpm() == Catch::Approx (137.5));
    }

    SECTION ("new project resets every project-owned state group")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setPatternScale (9, 2);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeBloom, true);
        testPlugin.setLoopBraceStartQuarters (2.0);
        testPlugin.setLoopBraceEndQuarters (6.0);
        testPlugin.saveCurrentBraceToLoopSlot (3);
        testPlugin.setPulseIndex (3);
        testPlugin.setSwingPercent (60);
        testPlugin.setRowColorsEnabled (false);
        testPlugin.setStandaloneTempoBpm (148.0);
        testPlugin.setProjectMetadata ("Old Project", "Old description", "created", "modified", "alt", 70, true);

        testPlugin.resetProject();

        CHECK (testPlugin.getCurrentPatternSlot() == 0);
        CHECK (testPlugin.getViewPatternSlot() == 0);
        CHECK (testPlugin.getCurrentLoopSlot() == -1);
        CHECK_FALSE (testPlugin.isLoopSlotAssigned (3));
        CHECK (testPlugin.getPatternPhraseRowStepCount (0, 0) == 0);
        CHECK (testPlugin.getPatternScaleRoot (0) == PluginProcessor::defaultScaleRoot);
        CHECK (testPlugin.getPatternScaleModeIndex (0) == PluginProcessor::defaultScaleModeIndex);
        CHECK (testPlugin.getPatternCombinationModeMask (0) == 0);
        CHECK (testPlugin.getPulseIndex() == PluginProcessor::defaultPulseIndex);
        CHECK (testPlugin.getSwingPercent() == 0);
        CHECK (testPlugin.isRowColorsEnabled());
        CHECK (testPlugin.getStandaloneTempoBpm() == Catch::Approx (120.0));
        CHECK (testPlugin.getProjectName() == "Untitled Project");
        CHECK (testPlugin.getProjectDescription().isEmpty());
        CHECK (testPlugin.getProjectCreatedAt().isEmpty());
        CHECK (testPlugin.getProjectModifiedAt().isEmpty());
        CHECK (testPlugin.getProjectThemeMode() == "dark");
        CHECK (testPlugin.getProjectUiScalePercent() == 100);
        CHECK_FALSE (testPlugin.getProjectStretchStepsToFit());
    }

    SECTION ("copy pattern slot duplicates pattern content without copying loop slot assignments")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 3);
        ensurePhraseRowStepCount (testPlugin, 1, 2);

        testPlugin.setPhraseRowMuted (0, true);
        testPlugin.setPhraseRowMuted (1, false);
        testPlugin.setPhraseRowTimingOffset (0, 6);
        testPlugin.setPhraseRowTimingOffset (1, 11);
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
        CHECK (testPlugin.getPatternPhraseRowTimingOffset (1, 0) == 6);
        CHECK (testPlugin.getPatternPhraseRowTimingOffset (1, 1) == 11);
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

    SECTION ("global transpose is remembered per pattern and in plugin state")
    {
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setPatternGlobalTransposeSemitones (5);

        testPlugin.setCurrentPatternSlot (1);
        CHECK (testPlugin.getPatternGlobalTransposeSemitones (1)
               == PluginProcessor::defaultGlobalTransposeSemitones);
        testPlugin.setPatternGlobalTransposeSemitones (-12);

        juce::MemoryBlock state;
        testPlugin.getStateInformation (state);

        PluginProcessor reloaded;
        reloaded.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

        CHECK (reloaded.getPatternGlobalTransposeSemitones (0) == 5);
        CHECK (reloaded.getPatternGlobalTransposeSemitones (1) == -12);
    }

    SECTION ("clearPatternSlot resets per-pattern state")
    {
        testPlugin.setCurrentPatternSlot (0);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeWeave, true);
        testPlugin.setCombinationModeEnabled (PluginProcessor::combinationModeMultiplyEcho, true);
        testPlugin.setPatternNoteBandpass (48, 84);
        testPlugin.setPatternVelocityTiltPivotMidi (72);
        testPlugin.setPatternVelocityTiltAmount (40);
        testPlugin.setPatternGlobalTransposeSemitones (5);
        testPlugin.setPatternOctavizerDown8vaEnabled (true);
        testPlugin.setPatternOctavizerUp8vaEnabled (true);
        testPlugin.setPatternShimmerEnabled (true);
        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setLoopBraceStartQuarters (2.0);
        testPlugin.setLoopBraceEndQuarters (6.0);

        testPlugin.clearPatternSlot (0);

        CHECK (testPlugin.getPatternCombinationModeMask (0) == 0);
        CHECK (testPlugin.getPatternNoteBandpassLow (0) == PluginProcessor::defaultNoteBandpassLowMidi);
        CHECK (testPlugin.getPatternNoteBandpassHigh (0) == PluginProcessor::defaultNoteBandpassHighMidi);
        CHECK (testPlugin.getPatternVelocityTiltPivotMidi (0)
               == PluginProcessor::defaultVelocityTiltPivotMidi);
        CHECK (testPlugin.getPatternVelocityTiltAmount (0) == PluginProcessor::defaultVelocityTiltAmount);
        CHECK (testPlugin.getPatternGlobalTransposeSemitones (0)
               == PluginProcessor::defaultGlobalTransposeSemitones);
        CHECK_FALSE (testPlugin.isPatternOctavizerDown8vaEnabled (0));
        CHECK_FALSE (testPlugin.isPatternOctavizerUp8vaEnabled (0));
        CHECK_FALSE (testPlugin.isPatternShimmerEnabled (0));
        CHECK_FALSE (testPlugin.isPatternLoopBraceEnabled (0));
        CHECK (testPlugin.getPatternLoopBraceStartQuarters (0)
               == Catch::Approx (PluginProcessor::defaultLoopBraceStartQuarters));
        CHECK (testPlugin.getPatternLoopBraceEndQuarters (0)
               == Catch::Approx (PluginProcessor::defaultLoopBraceEndQuarters));
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

        testPlugin.setPhraseRowTimingOffset (0, 6);
        testPlugin.setPhraseRowTimingOffset (1, 11);

        CHECK (testPlugin.getPhraseRowTimingOffset (0) == 6);
        CHECK (testPlugin.getPhraseRowTimingOffset (1) == 11);
        CHECK (PluginProcessor::rowTimingOffsetForIndex (0)
               < PluginProcessor::rowTimingOffsetForIndex (PluginProcessor::rowTimingOffsetCount - 1));

        testPlugin.setPhraseRowTimingOffset (0, 99);
        CHECK (testPlugin.getPhraseRowTimingOffset (0) == PluginProcessor::rowTimingOffsetCount - 1);
    }

    SECTION ("legacy row timing offset indices migrate from version 22 state")
    {
        juce::ValueTree state ("MidiPhrases");
        state.setProperty ("version", 22, nullptr);

        juce::ValueTree patternTree ("Pattern");
        patternTree.setProperty ("index", 0, nullptr);

        juce::ValueTree rowTree ("Row");
        rowTree.setProperty ("index", 0, nullptr);
        rowTree.setProperty ("stepCount", 1, nullptr);
        rowTree.setProperty ("timingOffset", 6, nullptr);

        patternTree.appendChild (rowTree, nullptr);
        state.appendChild (patternTree, nullptr);

        juce::MemoryBlock destData;

        if (auto xml = state.createXml())
        {
            juce::MemoryOutputStream stream;
            xml->writeTo (stream);
            destData.replaceAll (stream.getData(), stream.getDataSize());
        }

        PluginProcessor reloaded;
        reloaded.setStateInformation (destData.getData(), static_cast<int> (destData.getSize()));

        CHECK (reloaded.getPhraseRowTimingOffset (0) == 11);
        CHECK (PluginProcessor::rowTimingOffsetForIndex (reloaded.getPhraseRowTimingOffset (0))
               == Catch::Approx (0.75));
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

        for (const auto ppq : { 0.35, 0.45, 0.55, 0.65, 0.75, 0.85 })
        {
            midi.clear();
            playHead.info.setPpqPosition (ppq);
            testPlugin.processBlock (buffer, midi);

            CHECK (testPlugin.getAudioPatternSlot() == 0);
        }

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

    SECTION ("pattern switch restarts schedule phase from beat zero")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 4);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setPhraseStepVelocity (0, 1, 0);
        testPlugin.setPhraseStepVelocity (0, 2, 0);
        testPlugin.setPhraseStepVelocity (0, 3, 0);

        testPlugin.setCurrentPatternSlot (2);
        ensurePhraseRowStepCount (testPlugin, 0, 4);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setPhraseStepDurationFraction (0, 0, 1.0);
        testPlugin.setPhraseStepVelocity (0, 1, 0);
        testPlugin.setPhraseStepVelocity (0, 2, 0);
        testPlugin.setPhraseStepVelocity (0, 3, 0);
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
        testPlugin.setPlayHead (&playHead);

        playHead.info.setPpqPosition (16.0);
        testPlugin.processBlock (buffer, midi);

        midi.clear();
        playHead.info.setPpqPosition (17.0);
        testPlugin.setCurrentPatternSlot (2);
        testPlugin.processBlock (buffer, midi);

        CHECK (testPlugin.getAudioPatternSlot() == 2);
        CHECK (testPlugin.getPlaybackBeat() == Catch::Approx (0.1).margin (0.05));

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

    SECTION ("direct loop brace enable while playing applies on next pulse")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 0.25);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (1.0);

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

        playHead.info.setPpqPosition (0.25);
        testPlugin.processBlock (buffer, midi);

        testPlugin.setLoopBraceEnabled (true);

        midi.clear();
        playHead.info.setPpqPosition (0.50);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.65);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.75);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.85);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) == 50);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("brace range edit followed by enable applies on next pulse")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 72);
        testPlugin.setPhraseStepDurationFraction (0, 0, 0.25);

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

        playHead.info.setPpqPosition (0.25);
        testPlugin.processBlock (buffer, midi);

        testPlugin.setLoopBraceStartQuarters (2.0);
        testPlugin.setLoopBraceEndQuarters (3.0);
        testPlugin.setLoopBraceEnabled (true);

        midi.clear();
        playHead.info.setPpqPosition (0.50);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 72) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.65);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 72) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.75);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 72) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.85);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 72) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 72) == 50);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("disabling loop brace clears pending pulse enable")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setPulseIndex (PluginProcessor::defaultPulseIndex);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 0.25);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (1.0);

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

        playHead.info.setPpqPosition (0.25);
        testPlugin.processBlock (buffer, midi);

        testPlugin.setLoopBraceEnabled (true);
        testPlugin.setLoopBraceEnabled (false);

        midi.clear();
        playHead.info.setPpqPosition (0.50);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.65);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.75);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.85);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        midi.clear();
        playHead.info.setPpqPosition (0.95);
        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) < 0);

        testPlugin.setPlayHead (nullptr);
    }

    SECTION ("loop brace enabled while stopped plays from loop start on playback")
    {
        testPlugin.prepareToPlay (1000.0, 100);
        testPlugin.setCurrentPatternSlot (0);
        ensurePhraseRowStepCount (testPlugin, 0, 1);
        testPlugin.setPhraseNote (0, 0, 60);
        testPlugin.setPhraseStepDurationFraction (0, 0, 0.25);
        testPlugin.setLoopBraceStartQuarters (0.0);
        testPlugin.setLoopBraceEndQuarters (1.0);
        testPlugin.setLoopBraceEnabled (true);

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
        playHead.info.setPpqPosition (4.25);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        CHECK (findNoteOnSample (midi, 60) == 0);

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

        for (const auto ppq : { 0.10, 0.20, 0.30, 0.40, 0.50, 0.60, 0.70, 0.80, 0.85 })
        {
            midi.clear();
            playHead.info.setPpqPosition (ppq);
            testPlugin.processBlock (buffer, midi);

            CHECK (testPlugin.getAudioPatternSlot() == 0);
        }

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

    SECTION ("shutdown panic API emits note-off for active generated notes")
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
        CHECK (testPlugin.hasActiveGeneratedNotes());

        midi.clear();
        testPlugin.appendGeneratedNotePanicMessages (midi);

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
        CHECK_FALSE (testPlugin.hasActiveGeneratedNotes());

        testPlugin.releaseResources();
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
        playHead.info.setPpqPosition (0.0);
        testPlugin.setPlayHead (&playHead);

        testPlugin.processBlock (buffer, midi);

        midi.clear();
        playHead.info.setPpqPosition (1.0);
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
        CHECK (testPlugin.getPhraseStepProbability (0, 1) == PluginProcessor::maxStepProbabilityValue);

        testPlugin.setPhraseStepProbability (0, 1, 100);
        CHECK (testPlugin.getPhraseStepProbability (0, 1) == 100);
    }

    SECTION ("step cycle")
    {
        ensurePhraseRowStepCount (testPlugin, 0, 2);

        CHECK (testPlugin.getPhraseStepCycle (0, 0) == PluginProcessor::defaultStepCycle);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 0) == PluginProcessor::defaultStepCycleMask);

        testPlugin.setPhraseStepCycle (0, 1, 3);
        testPlugin.setPhraseStepCycleOffset (0, 1, 0b110);

        CHECK (testPlugin.getPhraseStepCycle (0, 1) == 3);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 1) == 0b110);

        testPlugin.setPhraseStepCycle (0, 1, 2);
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 1) == 0b10);
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
        CHECK (testPlugin.getPhraseStepCycleOffset (0, 1) == 100);
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

        testPlugin.setPatternScale (0, 1); // C major
        testPlugin.setPhraseRowRecording (0);

        processNoteOns ({ 60, 61, 64 });
        drained = testPlugin.drainPhraseRowRecordedNotes();
        REQUIRE (drained.size() == 2);
        CHECK (drained[0] == 60);
        CHECK (drained[1] == 64);
        CHECK (testPlugin.getPhraseRowStepCount (0) == 9);
        CHECK (testPlugin.getPhraseNote (0, 7) == 60);
        CHECK (testPlugin.getPhraseNote (0, 8) == 64);

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

        for (const auto ppq : { 0.35, 0.45, 0.55, 0.65, 0.75, 0.85 })
        {
            midi.clear();
            playHead.info.setPpqPosition (ppq);
            testPlugin.processBlock (buffer, midi);
        }

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

    SECTION ("fresh pattern defaults seeding to row 1 only")
    {
        PluginProcessor freshPlugin;
        freshPlugin.setCurrentPatternSlot (0);

        CHECK (freshPlugin.isPatternSeedingRowTargeted (0, 0));
        CHECK_FALSE (freshPlugin.isPatternSeedingRowTargeted (0, 1));
        CHECK_FALSE (freshPlugin.isPatternSeedingRowTargeted (0, 2));
        CHECK_FALSE (freshPlugin.isPatternSeedingRowTargeted (0, 3));
    }

    SECTION ("seeding center note round-trips through plugin state")
    {
        testPlugin.setCurrentPatternSlot (1);

        std::array<PluginProcessor::SeedingRowState, PluginProcessor::phraseRowCount> rowSettings {};
        std::array<int, PluginProcessor::phraseRowCount> rowTargets { 1, 0, 1, 0 };
        rowSettings[0].centerMidi = 72;
        rowSettings[0].timingMeanMultiplierIndex = 7;
        rowSettings[0].timingVariance = 88;
        rowSettings[1].centerMidi = 200;
        rowSettings[1].timingMeanMultiplierIndex = 99;
        rowSettings[1].timingVariance = 500;
        rowSettings[2].centerMidi = -12;
        rowSettings[2].timingMeanMultiplierIndex = -99;
        rowSettings[2].timingVariance = -10;

        testPlugin.setPatternSeedModeState (PluginProcessor::defaultSeedingRhythmStep,
                                            rowSettings,
                                            rowTargets);

        juce::MemoryBlock state;
        testPlugin.getStateInformation (state);

        PluginProcessor reloaded;
        reloaded.setStateInformation (state.getData(), static_cast<int> (state.getSize()));

        CHECK (reloaded.getPatternSeedingRowState (1, 0).centerMidi == 72);
        CHECK (reloaded.getPatternSeedingRowState (1, 0).timingMeanMultiplierIndex == 7);
        CHECK (reloaded.getPatternSeedingRowState (1, 0).timingVariance == 88);
        CHECK (reloaded.getPatternSeedingRowState (1, 1).centerMidi == PluginProcessor::maxMidiNote);
        CHECK (reloaded.getPatternSeedingRowState (1, 1).timingMeanMultiplierIndex
               == PluginProcessor::maxSeedingTimingMeanMultiplierIndex);
        CHECK (reloaded.getPatternSeedingRowState (1, 1).timingVariance == PluginProcessor::maxPercentValue);
        CHECK (reloaded.getPatternSeedingRowState (1, 2).centerMidi == PluginProcessor::defaultSeedingCenterMidi);
        CHECK (reloaded.getPatternSeedingRowState (1, 2).timingMeanMultiplierIndex
               == PluginProcessor::minSeedingTimingMeanMultiplierIndex);
        CHECK (reloaded.getPatternSeedingRowState (1, 2).timingVariance == 0);
        CHECK (reloaded.isPatternSeedingRowTargeted (1, 0));
        CHECK_FALSE (reloaded.isPatternSeedingRowTargeted (1, 1));
    }

    SECTION ("legacy seeding state without center note loads default center sentinel")
    {
        juce::ValueTree state ("MidiPhrases");
        state.setProperty ("version", 20, nullptr);

        juce::ValueTree patternTree ("Pattern");
        patternTree.setProperty ("index", 0, nullptr);
        patternTree.setProperty ("seedingRow0PhraseLength", 7, nullptr);
        patternTree.setProperty ("seedingRow0RangeSemitones", 16, nullptr);
        patternTree.setProperty ("seedingRow0Targeted", 1, nullptr);
        state.appendChild (patternTree, nullptr);

        juce::MemoryBlock block;

        if (auto xml = state.createXml())
        {
            juce::MemoryOutputStream stream (block, true);
            xml->writeTo (stream);
        }

        PluginProcessor reloaded;
        reloaded.setStateInformation (block.getData(), static_cast<int> (block.getSize()));

        const auto rowState = reloaded.getPatternSeedingRowState (0, 0);
        CHECK (rowState.phraseLength == 7);
        CHECK (rowState.rangeSemitones == 16);
        CHECK (rowState.centerMidi == PluginProcessor::defaultSeedingCenterMidi);
        CHECK (rowState.timingMeanMultiplierIndex
               == PluginProcessor::defaultSeedingTimingMeanMultiplierIndex);
        CHECK (rowState.timingVariance == PluginProcessor::defaultSeedingTimingVariance);
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

    SECTION ("legacy state migrates full step probability to 100")
    {
        juce::ValueTree state ("MidiPhrases");
        state.setProperty ("version", 21, nullptr);

        juce::ValueTree patternTree ("Pattern");
        patternTree.setProperty ("index", 0, nullptr);

        juce::ValueTree rowTree ("Row");
        rowTree.setProperty ("index", 0, nullptr);
        rowTree.setProperty ("stepCount", 2, nullptr);
        rowTree.setProperty ("probability0", PluginProcessor::maxPercentValue, nullptr);
        rowTree.setProperty ("probability1", 42, nullptr);

        patternTree.appendChild (rowTree, nullptr);
        state.appendChild (patternTree, nullptr);

        juce::MemoryBlock destData;

        if (auto xml = state.createXml())
        {
            juce::MemoryOutputStream stream;
            xml->writeTo (stream);
            destData.replaceAll (stream.getData(), stream.getDataSize());
        }

        PluginProcessor reloaded;
        reloaded.setStateInformation (destData.getData(), static_cast<int> (destData.getSize()));

        CHECK (reloaded.getPhraseStepProbability (0, 0) == PluginProcessor::defaultStepProbability);
        CHECK (reloaded.getPhraseStepProbability (0, 1) == 42);
    }

    SECTION ("current state preserves explicit 99 step probability")
    {
        juce::ValueTree state ("MidiPhrases");
        state.setProperty ("version", 22, nullptr);

        juce::ValueTree patternTree ("Pattern");
        patternTree.setProperty ("index", 0, nullptr);

        juce::ValueTree rowTree ("Row");
        rowTree.setProperty ("index", 0, nullptr);
        rowTree.setProperty ("stepCount", 1, nullptr);
        rowTree.setProperty ("probability0", PluginProcessor::maxPercentValue, nullptr);

        patternTree.appendChild (rowTree, nullptr);
        state.appendChild (patternTree, nullptr);

        juce::MemoryBlock destData;

        if (auto xml = state.createXml())
        {
            juce::MemoryOutputStream stream;
            xml->writeTo (stream);
            destData.replaceAll (stream.getData(), stream.getDataSize());
        }

        PluginProcessor reloaded;
        reloaded.setStateInformation (destData.getData(), static_cast<int> (destData.getSize()));

        CHECK (reloaded.getPhraseStepProbability (0, 0) == PluginProcessor::maxPercentValue);
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
