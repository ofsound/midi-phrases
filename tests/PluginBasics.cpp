#include "helpers/test_helpers.h"
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

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

    SECTION ("step timing multiplier")
    {
        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 0)
               == PluginProcessor::defaultStepTimingMultiplierIndex);

        testPlugin.setPhraseStepTimingMultiplier (0, 1, 0);
        testPlugin.setPhraseStepTimingMultiplier (2, 3, 4);

        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 1) == 0);
        CHECK (testPlugin.getPhraseStepTimingMultiplier (2, 3) == 4);
        CHECK (PluginProcessor::stepTimingMultiplierForIndex (0)
               < PluginProcessor::stepTimingMultiplierForIndex (4));

        testPlugin.setPhraseStepTimingMultiplier (0, 0, 99);
        CHECK (testPlugin.getPhraseStepTimingMultiplier (0, 0)
               == PluginProcessor::stepTimingMultiplierCount - 1);
    }

    SECTION ("step duration fraction")
    {
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0)
               == PluginProcessor::defaultStepDurationFractionIndex);

        testPlugin.setPhraseStepDurationFraction (0, 1, 0);
        testPlugin.setPhraseStepDurationFraction (2, 3, 2);

        CHECK (testPlugin.getPhraseStepDurationFraction (0, 1) == 0);
        CHECK (testPlugin.getPhraseStepDurationFraction (2, 3) == 2);
        CHECK (PluginProcessor::stepDurationFractionForIndex (0)
               < PluginProcessor::stepDurationFractionForIndex (3));

        testPlugin.setPhraseStepDurationFraction (0, 0, 99);
        CHECK (testPlugin.getPhraseStepDurationFraction (0, 0)
               == PluginProcessor::stepDurationFractionCount - 1);
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

    SECTION ("remove phrase step")
    {
        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::phraseStepCount);

        testPlugin.setPhraseNote (0, 0, 55);
        testPlugin.setPhraseNote (0, 1, 66);
        testPlugin.setPhraseNote (0, 2, 77);

        testPlugin.removePhraseStep (0, 1);

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::phraseStepCount - 1);
        CHECK (testPlugin.getPhraseNote (0, 0) == 55);
        CHECK (testPlugin.getPhraseNote (0, 1) == 77);
    }

    SECTION ("insert phrase step")
    {
        testPlugin.removePhraseStep (0, 3);
        testPlugin.removePhraseStep (0, 2);

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::phraseStepCount - 2);

        testPlugin.setPhraseNote (0, 0, 55);
        testPlugin.setPhraseNote (0, 1, 77);

        testPlugin.insertPhraseStep (0, 1);

        CHECK (testPlugin.getPhraseRowStepCount (0) == PluginProcessor::phraseStepCount - 1);
        CHECK (testPlugin.getPhraseNote (0, 0) == 55);
        CHECK (testPlugin.getPhraseNote (0, 1) == 60);
        CHECK (testPlugin.getPhraseNote (0, 2) == 77);
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
