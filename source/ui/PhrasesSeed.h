#pragma once
#include "PhrasesModel.h"
namespace phrases::ui
{
    // Pure message-thread generation, ported from seeding.js. No audio-thread state.
    Rows generate (const std::array<PluginProcessor::SeedingRowState, 4>& settings, int rhythm, int root, int mode);
}
