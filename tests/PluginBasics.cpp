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
