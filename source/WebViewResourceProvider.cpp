#include "WebViewResourceProvider.h"

#include "BinaryData.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <cstring>
#include <memory>

#if JUCE_WEB_BROWSER

namespace
{
double varToDouble (const juce::var& value)
{
    if (value.isBool())
        return static_cast<bool> (value) ? 1.0 : 0.0;

    if (value.isInt())
        return static_cast<double> (static_cast<int> (value));

    if (value.isInt64())
        return static_cast<double> (static_cast<juce::int64> (value));

    if (value.isDouble())
        return static_cast<double> (value);

    return static_cast<double> (value);
}

int varToInt (const juce::var& value)
{
    if (value.isBool())
        return static_cast<bool> (value) ? 1 : 0;

    if (value.isInt())
        return static_cast<int> (value);

    if (value.isInt64())
        return static_cast<int> (value);

    if (value.isDouble())
        return static_cast<int> (static_cast<double> (value));

    return static_cast<int> (value);
}

PluginProcessor::SeedingRowState seedingRowStateFromVar (const juce::var& value)
{
    PluginProcessor::SeedingRowState rowState;

    if (auto* object = value.getDynamicObject())
    {
        rowState.phraseLength = varToInt (object->getProperty ("phraseLength"));
        rowState.centerMidi = object->hasProperty ("centerMidi")
                                  ? varToInt (object->getProperty ("centerMidi"))
                                  : PluginProcessor::defaultSeedingCenterMidi;
        rowState.rangeSemitones = varToInt (object->getProperty ("rangeSemitones"));
        rowState.repetition = varToInt (object->getProperty ("repetition"));
        rowState.complexity = varToInt (object->getProperty ("complexity"));
        rowState.randomness = varToInt (object->getProperty ("randomness"));
        rowState.timingMeanMultiplierIndex = object->hasProperty ("timingMeanMultiplierIndex")
                                                 ? varToInt (object->getProperty ("timingMeanMultiplierIndex"))
                                                 : PluginProcessor::defaultSeedingTimingMeanMultiplierIndex;
        rowState.timingVariance = object->hasProperty ("timingVariance")
                                      ? varToInt (object->getProperty ("timingVariance"))
                                      : PluginProcessor::defaultSeedingTimingVariance;
        rowState.symmetry = varToInt (object->getProperty ("symmetry"));
        rowState.seed = varToInt (object->getProperty ("seed"));
    }

    return rowState;
}

juce::var createSeedModeStateVar (const PluginProcessor& processor, const int patternSlot)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("seedingRhythmStep", processor.getPatternSeedingRhythmStep (patternSlot));

    juce::Array<juce::var> seedingRowSettings;
    juce::Array<juce::var> seedingRowTargets;

    for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
    {
        const auto rowState = processor.getPatternSeedingRowState (patternSlot, row);
        auto rowObject = std::make_unique<juce::DynamicObject>();
        rowObject->setProperty ("phraseLength", rowState.phraseLength);
        rowObject->setProperty ("centerMidi", rowState.centerMidi);
        rowObject->setProperty ("rangeSemitones", rowState.rangeSemitones);
        rowObject->setProperty ("repetition", rowState.repetition);
        rowObject->setProperty ("complexity", rowState.complexity);
        rowObject->setProperty ("randomness", rowState.randomness);
        rowObject->setProperty ("timingMeanMultiplierIndex", rowState.timingMeanMultiplierIndex);
        rowObject->setProperty ("timingVariance", rowState.timingVariance);
        rowObject->setProperty ("symmetry", rowState.symmetry);
        rowObject->setProperty ("seed", rowState.seed);
        seedingRowSettings.add (juce::var (rowObject.release()));
        seedingRowTargets.add (processor.isPatternSeedingRowTargeted (patternSlot, row) ? 1 : 0);
    }

    object->setProperty ("seedingRowSettings", seedingRowSettings);
    object->setProperty ("seedingRowTargets", seedingRowTargets);
    return juce::var (object.release());
}

void copyIntArray (const juce::var& value,
                   std::array<int, PluginProcessor::maxPhraseStepsPerRow>& target)
{
    if (! value.isArray())
        return;

    const auto* values = value.getArray();
    const auto count = juce::jmin (values->size(), PluginProcessor::maxPhraseStepsPerRow);

    for (int index = 0; index < count; ++index)
        target[static_cast<size_t> (index)] = varToInt ((*values)[index]);
}

void copyDoubleArray (const juce::var& value,
                      std::array<double, PluginProcessor::maxPhraseStepsPerRow>& target)
{
    if (! value.isArray())
        return;

    const auto* values = value.getArray();
    const auto count = juce::jmin (values->size(), PluginProcessor::maxPhraseStepsPerRow);

    for (int index = 0; index < count; ++index)
        target[static_cast<size_t> (index)] = varToDouble ((*values)[index]);
}

std::unique_ptr<juce::ZipFile> uiZip;

juce::ZipFile* getUiZip()
{
    if (BinaryData::ui_zipSize == 0)
        return nullptr;

    if (uiZip == nullptr)
    {
        auto stream = std::make_unique<juce::MemoryInputStream> (BinaryData::ui_zip,
                                                                 BinaryData::ui_zipSize,
                                                                 false);
        uiZip = std::make_unique<juce::ZipFile> (std::move (stream));
    }

    return uiZip.get();
}

juce::String normalisePath (juce::String path)
{
    if (path.isEmpty() || path == "/")
        return "index.html";

    if (path.startsWithChar ('/'))
        return path.substring (1);

    return path;
}

juce::String mimeTypeForPath (const juce::String& path)
{
    if (path.endsWithIgnoreCase (".html"))
        return "text/html";

    if (path.endsWithIgnoreCase (".js"))
        return "text/javascript";

    if (path.endsWithIgnoreCase (".css"))
        return "text/css";

    if (path.endsWithIgnoreCase (".svg"))
        return "image/svg+xml";

    if (path.endsWithIgnoreCase (".png"))
        return "image/png";

    if (path.endsWithIgnoreCase (".woff2"))
        return "font/woff2";

    return "application/octet-stream";
}

int patternSlotForNativeDefault (const PluginProcessor& processor)
{
    const auto current = processor.getCurrentPatternSlot();

    if (current >= 0)
        return current;

    return processor.getViewPatternSlot();
}

juce::var createPatternStateVar (PluginProcessor& processor, const int patternSlot)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("patternSlot", patternSlot);

    juce::Array<juce::var> phraseRows;
    juce::Array<juce::var> phraseRowMuted;
    juce::Array<juce::var> phraseRowTimingOffset;
    juce::Array<juce::var> phraseRowMidiChannel;
    juce::Array<juce::var> phraseStepDurationFraction;
    juce::Array<juce::var> phraseStepTimingMultiplier;
    juce::Array<juce::var> phraseStepVelocity;
    juce::Array<juce::var> phraseStepMuted;
    juce::Array<juce::var> phraseStepSkipped;
    juce::Array<juce::var> phraseStepProbability;
    juce::Array<juce::var> phraseStepCycle;
    juce::Array<juce::var> phraseStepCycleOffset;
    for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
    {
        juce::Array<juce::var> steps;
        juce::Array<juce::var> stepDurations;
        juce::Array<juce::var> stepTimingMultipliers;
        juce::Array<juce::var> stepVelocities;
        juce::Array<juce::var> stepMutedFlags;
        juce::Array<juce::var> stepSkippedFlags;
        juce::Array<juce::var> stepProbabilityValues;
        juce::Array<juce::var> stepCycleValues;
        juce::Array<juce::var> stepCycleOffsetValues;

        for (int step = 0; step < processor.getPatternPhraseRowStepCount (patternSlot, row); ++step)
        {
            steps.add (processor.getPatternPhraseNote (patternSlot, row, step));
            stepTimingMultipliers.add (
                processor.getPatternPhraseStepTimingMultiplier (patternSlot, row, step));
            stepDurations.add (
                processor.getPatternPhraseStepDurationFraction (patternSlot, row, step));
            stepVelocities.add (processor.getPatternPhraseStepVelocity (patternSlot, row, step));
            stepMutedFlags.add (
                processor.isPatternPhraseStepMuted (patternSlot, row, step));
            stepSkippedFlags.add (
                processor.isPatternPhraseStepSkipped (patternSlot, row, step));
            stepProbabilityValues.add (
                processor.getPatternPhraseStepProbability (patternSlot, row, step));
            stepCycleValues.add (processor.getPatternPhraseStepCycle (patternSlot, row, step));
            stepCycleOffsetValues.add (
                processor.getPatternPhraseStepCycleOffset (patternSlot, row, step));
        }

        phraseRows.add (steps);
        phraseRowMuted.add (processor.isPatternPhraseRowMuted (patternSlot, row));
        phraseRowTimingOffset.add (processor.getPatternPhraseRowTimingOffset (patternSlot, row));
        phraseRowMidiChannel.add (processor.getPatternPhraseRowMidiChannel (patternSlot, row));
        phraseStepTimingMultiplier.add (stepTimingMultipliers);
        phraseStepDurationFraction.add (stepDurations);
        phraseStepVelocity.add (stepVelocities);
        phraseStepMuted.add (stepMutedFlags);
        phraseStepSkipped.add (stepSkippedFlags);
        phraseStepProbability.add (stepProbabilityValues);
        phraseStepCycle.add (stepCycleValues);
        phraseStepCycleOffset.add (stepCycleOffsetValues);
    }

    object->setProperty ("phraseNotes", phraseRows);
    object->setProperty ("phraseRowMuted", phraseRowMuted);
    object->setProperty ("phraseRowTimingOffset", phraseRowTimingOffset);
    object->setProperty ("phraseRowMidiChannel", phraseRowMidiChannel);
    object->setProperty ("phraseStepDurationFraction", phraseStepDurationFraction);
    object->setProperty ("phraseStepTimingMultiplier", phraseStepTimingMultiplier);
    object->setProperty ("phraseStepVelocity", phraseStepVelocity);
    object->setProperty ("phraseStepMuted", phraseStepMuted);
    object->setProperty ("phraseStepSkipped", phraseStepSkipped);
    object->setProperty ("phraseStepProbability", phraseStepProbability);
    object->setProperty ("phraseStepCycle", phraseStepCycle);
    object->setProperty ("phraseStepCycleOffset", phraseStepCycleOffset);
    object->setProperty ("loopBraceEnabled",
                         processor.isPatternLoopBraceEnabled (patternSlot) ? 1 : 0);
    object->setProperty ("loopBraceStart",
                         processor.getPatternLoopBraceStartQuarters (patternSlot));
    object->setProperty ("loopBraceEnd",
                         processor.getPatternLoopBraceEndQuarters (patternSlot));
    object->setProperty ("combinationModeMask",
                         processor.getPatternCombinationModeMask (patternSlot));
    object->setProperty ("scaleRoot", processor.getPatternScaleRoot (patternSlot));
    object->setProperty ("scaleModeIndex", processor.getPatternScaleModeIndex (patternSlot));
    object->setProperty ("noteBandpassLowMidi",
                         processor.getPatternNoteBandpassLow (patternSlot));
    object->setProperty ("noteBandpassHighMidi",
                         processor.getPatternNoteBandpassHigh (patternSlot));
    object->setProperty ("velocityTiltPivotMidi",
                         processor.getPatternVelocityTiltPivotMidi (patternSlot));
    object->setProperty ("velocityTiltAmount",
                         processor.getPatternVelocityTiltAmount (patternSlot));
    object->setProperty ("globalTransposeSemitones",
                         processor.getPatternGlobalTransposeSemitones (patternSlot));
    object->setProperty ("octavizerDown8vaEnabled",
                         processor.isPatternOctavizerDown8vaEnabled (patternSlot) ? 1 : 0);
    object->setProperty ("octavizerUp8vaEnabled",
                         processor.isPatternOctavizerUp8vaEnabled (patternSlot) ? 1 : 0);
    object->setProperty ("octavizerDown8vaRelativeVelocity",
                         processor.getPatternOctavizerDown8vaRelativeVelocity (patternSlot));
    object->setProperty ("octavizerUp8vaRelativeVelocity",
                         processor.getPatternOctavizerUp8vaRelativeVelocity (patternSlot));
    object->setProperty ("shimmerEnabled", processor.isPatternShimmerEnabled (patternSlot) ? 1 : 0);
    object->setProperty ("shimmerDelayMultiplierIndex",
                         processor.getPatternShimmerDelayMultiplierIndex (patternSlot));
    object->setProperty ("shimmerFeedbackPercent",
                         processor.getPatternShimmerFeedbackPercent (patternSlot));
    object->setProperty ("shimmerMixPercent", processor.getPatternShimmerMixPercent (patternSlot));
    object->setProperty ("seedingRhythmStep", processor.getPatternSeedingRhythmStep (patternSlot));

    juce::Array<juce::var> seedingRowSettings;
    juce::Array<juce::var> seedingRowTargets;

    for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
    {
        const auto rowState = processor.getPatternSeedingRowState (patternSlot, row);
        auto rowObject = std::make_unique<juce::DynamicObject>();
        rowObject->setProperty ("phraseLength", rowState.phraseLength);
        rowObject->setProperty ("centerMidi", rowState.centerMidi);
        rowObject->setProperty ("rangeSemitones", rowState.rangeSemitones);
        rowObject->setProperty ("repetition", rowState.repetition);
        rowObject->setProperty ("complexity", rowState.complexity);
        rowObject->setProperty ("randomness", rowState.randomness);
        rowObject->setProperty ("timingMeanMultiplierIndex", rowState.timingMeanMultiplierIndex);
        rowObject->setProperty ("timingVariance", rowState.timingVariance);
        rowObject->setProperty ("symmetry", rowState.symmetry);
        rowObject->setProperty ("seed", rowState.seed);
        seedingRowSettings.add (juce::var (rowObject.release()));
        seedingRowTargets.add (processor.isPatternSeedingRowTargeted (patternSlot, row) ? 1 : 0);
    }

    object->setProperty ("seedingRowSettings", seedingRowSettings);
    object->setProperty ("seedingRowTargets", seedingRowTargets);

    return juce::var (object.release());
}

juce::var createCurrentSlotStateVar (PluginProcessor& processor)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty ("currentPatternSlot", processor.getCurrentPatternSlot());
    object->setProperty ("viewPatternSlot", processor.getViewPatternSlot());
    object->setProperty ("patternOutputArmed", processor.isPatternOutputArmed() ? 1 : 0);
    object->setProperty ("currentLoopSlot", processor.getCurrentLoopSlot());
    return juce::var (object.release());
}

juce::var createProjectStateVar (PluginProcessor& processor, const PluginEditor& editor)
{
    auto object = std::make_unique<juce::DynamicObject>();
    const auto patternSlot = patternSlotForNativeDefault (processor);
    object->setProperty ("patternState", createPatternStateVar (processor, patternSlot));
    object->setProperty ("currentPatternSlot", processor.getCurrentPatternSlot());
    object->setProperty ("viewPatternSlot", processor.getViewPatternSlot());
    object->setProperty ("patternOutputArmed", processor.isPatternOutputArmed() ? 1 : 0);
    object->setProperty ("currentLoopSlot", processor.getCurrentLoopSlot());
    object->setProperty ("pulseIndex", processor.getPulseIndex());
    object->setProperty ("swingPercent", processor.getSwingPercent());
    object->setProperty ("velocityHumanizePercent", processor.getVelocityHumanizePercent());
    object->setProperty ("timingHumanizePercent", processor.getTimingHumanizePercent());
    object->setProperty ("swingSubdivisionIndex", processor.getSwingSubdivisionIndex());
    object->setProperty ("rowColorsEnabled", processor.isRowColorsEnabled() ? 1 : 0);
    object->setProperty ("standaloneTempoBpm", processor.getStandaloneTempoBpm());
    object->setProperty ("projectName", processor.getProjectName());
    object->setProperty ("projectDescription", processor.getProjectDescription());
    object->setProperty ("projectCreatedAt", processor.getProjectCreatedAt());
    object->setProperty ("projectModifiedAt", processor.getProjectModifiedAt());
    object->setProperty ("projectThemeMode", processor.getProjectThemeMode());
    object->setProperty ("projectUiScalePercent", processor.getProjectUiScalePercent());
    object->setProperty ("projectStretchStepsToFit",
                         processor.getProjectStretchStepsToFit() ? 1 : 0);
    object->setProperty ("projectFileName", editor.getCurrentProjectFileName());
    object->setProperty ("hasPreviousProject", editor.hasPreviousProject() ? 1 : 0);
    object->setProperty ("hasNextProject", editor.hasNextProject() ? 1 : 0);

    juce::Array<juce::var> loopAssigned;
    juce::Array<juce::var> loopPattern;
    for (int loopSlot = 0; loopSlot < PluginProcessor::loopSlotCount; ++loopSlot)
    {
        loopAssigned.add (processor.isLoopSlotAssigned (loopSlot) ? 1 : 0);
        loopPattern.add (processor.getLoopSlotPatternSlot (loopSlot));
    }
    object->setProperty ("loopSlotAssigned", loopAssigned);
    object->setProperty ("loopSlotPattern", loopPattern);
    return juce::var (object.release());
}

} // namespace

std::optional<juce::WebBrowserComponent::Resource> WebViewResources::getResource (const juce::String& url)
{
    auto* zip = getUiZip();

    if (zip == nullptr)
        return std::nullopt;

    const auto path = normalisePath (url);
    const auto entryIndex = zip->getIndexOfFileName (path);

    if (entryIndex < 0)
        return std::nullopt;

    std::unique_ptr<juce::InputStream> stream (zip->createStreamForEntry (entryIndex));

    if (stream == nullptr)
        return std::nullopt;

    juce::MemoryBlock data;
    stream->readIntoMemoryBlock (data);

    juce::WebBrowserComponent::Resource resource;
    resource.mimeType = mimeTypeForPath (path);
    resource.data.resize (data.getSize());
    std::memcpy (resource.data.data(), data.getData(), data.getSize());

    return resource;
}

juce::WebBrowserComponent::Options WebViewResources::makeBrowserOptions (PluginProcessor& processor,
                                                                         PluginEditor& editor)
{
    using Options = juce::WebBrowserComponent::Options;

    juce::Array<juce::var> phraseRows;
    juce::Array<juce::var> phraseRowMuted;
    juce::Array<juce::var> phraseRowTimingOffset;
    juce::Array<juce::var> phraseRowMidiChannel;
    juce::Array<juce::var> phraseStepDurationFraction;
    juce::Array<juce::var> phraseStepTimingMultiplier;
    juce::Array<juce::var> phraseStepVelocity;
    juce::Array<juce::var> phraseStepMuted;
    juce::Array<juce::var> phraseStepSkipped;
    juce::Array<juce::var> phraseStepProbability;
    juce::Array<juce::var> phraseStepCycle;
    juce::Array<juce::var> phraseStepCycleOffset;
    juce::Array<juce::var> loopSlotAssigned;
    juce::Array<juce::var> loopSlotPattern;

    for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
    {
        juce::Array<juce::var> steps;
        juce::Array<juce::var> stepDurations;
        juce::Array<juce::var> stepTimingMultipliers;
        juce::Array<juce::var> stepVelocities;
        juce::Array<juce::var> stepMutedFlags;
        juce::Array<juce::var> stepSkippedFlags;
        juce::Array<juce::var> stepProbabilityValues;
        juce::Array<juce::var> stepCycleValues;
        juce::Array<juce::var> stepCycleOffsetValues;

        for (int step = 0; step < processor.getPhraseRowStepCount (row); ++step)
        {
            steps.add (processor.getPhraseNote (row, step));
            stepTimingMultipliers.add (processor.getPhraseStepTimingMultiplier (row, step));
            stepDurations.add (processor.getPhraseStepDurationFraction (row, step));
            stepVelocities.add (processor.getPhraseStepVelocity (row, step));
            stepMutedFlags.add (processor.isPhraseStepMuted (row, step));
            stepSkippedFlags.add (processor.isPhraseStepSkipped (row, step));
            stepProbabilityValues.add (processor.getPhraseStepProbability (row, step));
            stepCycleValues.add (processor.getPhraseStepCycle (row, step));
            stepCycleOffsetValues.add (processor.getPhraseStepCycleOffset (row, step));
        }

        phraseRows.add (steps);
        phraseRowMuted.add (processor.isPhraseRowMuted (row));
        phraseRowTimingOffset.add (processor.getPhraseRowTimingOffset (row));
        phraseRowMidiChannel.add (processor.getPhraseRowMidiChannel (row));
        phraseStepTimingMultiplier.add (stepTimingMultipliers);
        phraseStepDurationFraction.add (stepDurations);
        phraseStepVelocity.add (stepVelocities);
        phraseStepMuted.add (stepMutedFlags);
        phraseStepSkipped.add (stepSkippedFlags);
        phraseStepProbability.add (stepProbabilityValues);
        phraseStepCycle.add (stepCycleValues);
        phraseStepCycleOffset.add (stepCycleOffsetValues);
    }

    for (int loopSlot = 0; loopSlot < PluginProcessor::loopSlotCount; ++loopSlot)
    {
        loopSlotAssigned.add (processor.isLoopSlotAssigned (loopSlot) ? 1 : 0);
        loopSlotPattern.add (processor.getLoopSlotPatternSlot (loopSlot));
    }

    auto options = Options{}
                       .withNativeIntegrationEnabled()
                       .withInitialisationData ("pluginName", juce::var { PRODUCT_NAME_WITHOUT_VERSION })
                       .withInitialisationData ("currentPatternSlot", processor.getCurrentPatternSlot())
                       .withInitialisationData ("viewPatternSlot", processor.getViewPatternSlot())
                       .withInitialisationData ("patternOutputArmed",
                                                processor.isPatternOutputArmed() ? 1 : 0)
                       .withInitialisationData ("currentLoopSlot", processor.getCurrentLoopSlot())
                       .withInitialisationData ("loopSlotAssigned", loopSlotAssigned)
                       .withInitialisationData ("loopSlotPattern", loopSlotPattern)
                       .withInitialisationData ("phraseNotes", phraseRows)
                       .withInitialisationData ("phraseRowMuted", phraseRowMuted)
                       .withInitialisationData ("phraseRowTimingOffset", phraseRowTimingOffset)
                       .withInitialisationData ("phraseRowMidiChannel", phraseRowMidiChannel)
                       .withInitialisationData ("phraseStepDurationFraction", phraseStepDurationFraction)
                       .withInitialisationData ("phraseStepTimingMultiplier", phraseStepTimingMultiplier)
                       .withInitialisationData ("phraseStepVelocity", phraseStepVelocity)
                       .withInitialisationData ("phraseStepMuted", phraseStepMuted)
                       .withInitialisationData ("phraseStepSkipped", phraseStepSkipped)
                       .withInitialisationData ("phraseStepProbability", phraseStepProbability)
                       .withInitialisationData ("phraseStepCycle", phraseStepCycle)
                       .withInitialisationData ("phraseStepCycleOffset", phraseStepCycleOffset)
                       .withInitialisationData ("pulseIndex", processor.getPulseIndex())
                       .withInitialisationData ("swingPercent", processor.getSwingPercent())
                       .withInitialisationData ("velocityHumanizePercent",
                                                processor.getVelocityHumanizePercent())
                       .withInitialisationData ("timingHumanizePercent",
                                                processor.getTimingHumanizePercent())
                       .withInitialisationData ("swingSubdivisionIndex",
                                                processor.getSwingSubdivisionIndex())
                       .withInitialisationData ("rowColorsEnabled",
                                                processor.isRowColorsEnabled() ? 1 : 0)
                       .withInitialisationData ("combinationModeMask",
                                                processor.getCombinationModeMask())
                       .withInitialisationData ("scaleRoot",
                                                processor.getPatternScaleRoot (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("scaleModeIndex",
                                                processor.getPatternScaleModeIndex (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("noteBandpassLowMidi",
                                                processor.getPatternNoteBandpassLow (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("noteBandpassHighMidi",
                                                processor.getPatternNoteBandpassHigh (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("velocityTiltPivotMidi",
                                                processor.getPatternVelocityTiltPivotMidi (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("velocityTiltAmount",
                                                processor.getPatternVelocityTiltAmount (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("globalTransposeSemitones",
                                                processor.getPatternGlobalTransposeSemitones (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("octavizerDown8vaEnabled",
                                                processor.isPatternOctavizerDown8vaEnabled (
                                                    patternSlotForNativeDefault (processor))
                                                    ? 1
                                                    : 0)
                       .withInitialisationData ("octavizerUp8vaEnabled",
                                                processor.isPatternOctavizerUp8vaEnabled (
                                                    patternSlotForNativeDefault (processor))
                                                    ? 1
                                                    : 0)
                       .withInitialisationData ("octavizerDown8vaRelativeVelocity",
                                                processor.getPatternOctavizerDown8vaRelativeVelocity (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("octavizerUp8vaRelativeVelocity",
                                                processor.getPatternOctavizerUp8vaRelativeVelocity (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("shimmerEnabled",
                                                processor.isPatternShimmerEnabled (
                                                    patternSlotForNativeDefault (processor))
                                                    ? 1
                                                    : 0)
                       .withInitialisationData ("shimmerDelayMultiplierIndex",
                                                processor.getPatternShimmerDelayMultiplierIndex (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("shimmerFeedbackPercent",
                                                processor.getPatternShimmerFeedbackPercent (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("shimmerMixPercent",
                                                processor.getPatternShimmerMixPercent (
                                                    patternSlotForNativeDefault (processor)))
                       .withInitialisationData ("loopBraceEnabled",
                                                processor.isLoopBraceEnabled() ? 1 : 0)
                       .withInitialisationData ("loopBraceStart",
                                                processor.getLoopBraceStartQuarters())
                       .withInitialisationData ("loopBraceEnd",
                                                processor.getLoopBraceEndQuarters())
                       .withInitialisationData ("standaloneTransportAvailable",
                                                processor.hasStandaloneTransport() ? 1 : 0)
                       .withInitialisationData ("standaloneTransportPlaying",
                                                processor.isStandaloneTransportPlaying() ? 1 : 0)
                       .withInitialisationData ("standaloneTempoBpm",
                                                processor.getStandaloneTempoBpm())
                       .withInitialisationData ("projectName", processor.getProjectName())
                       .withInitialisationData ("projectDescription", processor.getProjectDescription())
                       .withInitialisationData ("projectCreatedAt", processor.getProjectCreatedAt())
                       .withInitialisationData ("projectModifiedAt", processor.getProjectModifiedAt())
                       .withInitialisationData ("projectThemeMode", processor.getProjectThemeMode())
                       .withInitialisationData ("projectUiScalePercent",
                                                processor.getProjectUiScalePercent())
                       .withInitialisationData ("projectStretchStepsToFit",
                                                processor.getProjectStretchStepsToFit() ? 1 : 0)
                       .withInitialisationData ("projectFileName", editor.getCurrentProjectFileName())
                       .withInitialisationData ("hasPreviousProject",
                                                editor.hasPreviousProject() ? 1 : 0)
                       .withInitialisationData ("hasNextProject", editor.hasNextProject() ? 1 : 0)
                       .withNativeFunction (
                           "getProjectState",
                           [&processor, &editor] (const juce::Array<juce::var>&,
                                                  juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               complete (createProjectStateVar (processor, editor));
                           })
                       .withNativeFunction (
                           "newProject",
                           [&editor] (const juce::Array<juce::var>&,
                                      juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               editor.createNewProject (std::move (complete));
                           })
                       .withNativeFunction (
                           "saveProject",
                           [&editor] (const juce::Array<juce::var>& args,
                                      juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               editor.showSaveProjectDialog (args, std::move (complete));
                           })
                       .withNativeFunction (
                           "loadProject",
                           [&editor] (const juce::Array<juce::var>&,
                                      juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               editor.showLoadProjectDialog (std::move (complete));
                           })
                       .withNativeFunction (
                           "cycleProject",
                           [&editor] (const juce::Array<juce::var>& args,
                                      juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               editor.cycleProject (args.size() > 0 ? varToInt (args[0]) : 1,
                                                    std::move (complete));
                           })
                       .withNativeFunction (
                           "getEditorFullscreenState",
                           [&processor] (const juce::Array<juce::var>&,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               complete (processor.requestWebEditorFullscreen (-1));
                           })
                       .withNativeFunction (
                           "setEditorFullscreen",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               complete (processor.requestWebEditorFullscreen (
                                   args.size() >= 1 ? varToInt (args[0]) : 1));
                           })
                       .withNativeFunction (
                           "setEditorScaleMinimum",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               complete (processor.requestWebEditorScaleMinimum (
                                   args.size() >= 1 ? varToInt (args[0]) : 1690,
                                   args.size() >= 2 ? varToInt (args[1]) : 980));
                           })
                       .withNativeFunction (
                           "setProjectUiScalePercent",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               processor.setProjectUiScalePercent (
                                   args.size() >= 1 ? varToInt (args[0]) : 100);
                               complete (processor.getProjectUiScalePercent());
                           })
                       .withNativeFunction (
                           "getPatternSlotState",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               complete (createPatternStateVar (
                                   processor,
                                   args.size() >= 1 ? varToInt (args[0])
                                                   : patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "getCurrentSlotState",
                           [&processor] (const juce::Array<juce::var>&,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               complete (createCurrentSlotStateVar (processor));
                           })
                       .withNativeFunction (
                           "setCurrentPatternSlot",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setCurrentPatternSlot (varToInt (args[0]));

                               complete (createPatternStateVar (processor,
                                                                patternSlotForNativeDefault (
                                                                    processor)));
                           })
                       .withNativeFunction (
                           "deactivatePatternOutput",
                           [&processor] (const juce::Array<juce::var>&,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               processor.deactivatePatternOutput();
                               complete (createCurrentSlotStateVar (processor));
                           })
                       .withNativeFunction (
                           "clearPatternSlot",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.clearPatternSlot (varToInt (args[0]));

                               complete (createPatternStateVar (
                                   processor,
                                   args.size() >= 1 ? varToInt (args[0])
                                                    : patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "copyPatternSlot",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                                   processor.copyPatternSlot (varToInt (args[0]),
                                                              varToInt (args[1]));

                               complete (createPatternStateVar (
                                   processor,
                                   args.size() >= 2 ? varToInt (args[1])
                                                    : patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "saveCurrentBraceToLoopSlot",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.saveCurrentBraceToLoopSlot (varToInt (args[0]));

                               complete (createPatternStateVar (processor,
                                                                patternSlotForNativeDefault (
                                                                    processor)));
                           })
                       .withNativeFunction (
                           "selectLoopSlot",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.selectLoopSlot (varToInt (args[0]));

                               complete (createPatternStateVar (processor,
                                                                patternSlotForNativeDefault (
                                                                    processor)));
                           })
                       .withNativeFunction (
                           "setPhraseNote",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseNote (varToInt (args[0]),
                                                            varToInt (args[1]),
                                                            varToInt (args[2]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPatternScale",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                                   processor.setPatternScale (varToInt (args[0]),
                                                              varToInt (args[1]));

                               complete (createPatternStateVar (processor,
                                                                patternSlotForNativeDefault (
                                                                    processor)));
                           })
                       .withNativeFunction (
                           "setPatternNoteBandpass",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                                   processor.setPatternNoteBandpass (varToInt (args[0]),
                                                                     varToInt (args[1]));

                               juce::Array<juce::var> result;
                               const auto patternSlot = patternSlotForNativeDefault (processor);
                               result.add (processor.getPatternNoteBandpassLow (patternSlot));
                               result.add (processor.getPatternNoteBandpassHigh (patternSlot));
                               complete (result);
                           })
                       .withNativeFunction (
                           "setPatternVelocityTiltPivotMidi",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternVelocityTiltPivotMidi (varToInt (args[0]));

                               complete (processor.getPatternVelocityTiltPivotMidi (
                                   patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPatternVelocityTiltAmount",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternVelocityTiltAmount (varToInt (args[0]));

                               complete (processor.getPatternVelocityTiltAmount (
                                   patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPatternGlobalTransposeSemitones",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternGlobalTransposeSemitones (varToInt (args[0]));

                               complete (processor.getPatternGlobalTransposeSemitones (
                                   patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPatternOctavizerDown8vaEnabled",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternOctavizerDown8vaEnabled (varToInt (args[0]) != 0);

                               complete (processor.isPatternOctavizerDown8vaEnabled (
                                             patternSlotForNativeDefault (processor))
                                         ? 1
                                         : 0);
                           })
                       .withNativeFunction (
                           "setPatternOctavizerUp8vaEnabled",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternOctavizerUp8vaEnabled (varToInt (args[0]) != 0);

                               complete (processor.isPatternOctavizerUp8vaEnabled (
                                             patternSlotForNativeDefault (processor))
                                         ? 1
                                         : 0);
                           })
                       .withNativeFunction (
                           "setPatternOctavizerDown8vaRelativeVelocity",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternOctavizerDown8vaRelativeVelocity (varToInt (args[0]));

                               complete (processor.getPatternOctavizerDown8vaRelativeVelocity (
                                   patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPatternOctavizerUp8vaRelativeVelocity",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternOctavizerUp8vaRelativeVelocity (varToInt (args[0]));

                               complete (processor.getPatternOctavizerUp8vaRelativeVelocity (
                                   patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPatternShimmerEnabled",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternShimmerEnabled (varToInt (args[0]) != 0);

                               complete (processor.isPatternShimmerEnabled (
                                             patternSlotForNativeDefault (processor))
                                         ? 1
                                         : 0);
                           })
                       .withNativeFunction (
                           "setPatternShimmerDelayMultiplierIndex",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternShimmerDelayMultiplierIndex (varToInt (args[0]));

                               complete (processor.getPatternShimmerDelayMultiplierIndex (
                                   patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPatternShimmerFeedbackPercent",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternShimmerFeedbackPercent (varToInt (args[0]));

                               complete (processor.getPatternShimmerFeedbackPercent (
                                   patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPatternShimmerMixPercent",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPatternShimmerMixPercent (varToInt (args[0]));

                               complete (processor.getPatternShimmerMixPercent (
                                   patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPatternSeedModeState",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   const auto rhythmStep = varToInt (args[0]);
                                   std::array<PluginProcessor::SeedingRowState, PluginProcessor::phraseRowCount>
                                       rowSettings {};
                                   std::array<int, PluginProcessor::phraseRowCount> rowTargets {};

                                   if (auto* rowSettingsValues = args[1].getArray())
                                   {
                                       for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
                                       {
                                           if (row < rowSettingsValues->size())
                                               rowSettings[static_cast<size_t> (row)] =
                                                   seedingRowStateFromVar ((*rowSettingsValues)[row]);
                                       }
                                   }

                                   if (auto* rowTargetsValues = args[2].getArray())
                                   {
                                       for (int row = 0; row < PluginProcessor::phraseRowCount; ++row)
                                       {
                                           rowTargets[static_cast<size_t> (row)] =
                                               row < rowTargetsValues->size()
                                                   ? varToInt ((*rowTargetsValues)[row])
                                                   : 0;
                                       }
                                   }

                                   processor.setPatternSeedModeState (rhythmStep, rowSettings, rowTargets);
                               }

                               complete (createSeedModeStateVar (processor,
                                                               patternSlotForNativeDefault (processor)));
                           })
                       .withNativeFunction (
                           "setPhraseRowMuted",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                               {
                                   processor.setPhraseRowMuted (varToInt (args[0]),
                                                                varToInt (args[1]) != 0);
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "reversePhraseRowSteps",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.reversePhraseRowSteps (varToInt (args[0]));

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "reorderPhraseRowSteps",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2 && args[1].isArray())
                               {
                                   std::array<int, PluginProcessor::maxPhraseStepsPerRow> stepOrder {};
                                   const auto* orderValues = args[1].getArray();
                                   const auto orderSize = juce::jmin (
                                       orderValues->size(),
                                       PluginProcessor::maxPhraseStepsPerRow);

                                   for (int index = 0; index < orderSize; ++index)
                                       stepOrder[static_cast<size_t> (index)] =
                                           varToInt ((*orderValues)[index]);

                                   processor.reorderPhraseRowSteps (varToInt (args[0]),
                                                                     stepOrder,
                                                                     orderSize);
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "replacePhraseRow",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 10 && args[1].isArray())
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

                                   copyIntArray (args[1], notes);
                                   copyIntArray (args[2], timingMultiplier);
                                   copyDoubleArray (args[3], durationFraction);
                                   copyIntArray (args[4], velocity);
                                   copyIntArray (args[5], stepMuted);
                                   copyIntArray (args[6], stepSkipped);
                                   copyIntArray (args[7], probability);
                                   copyIntArray (args[8], cycle);
                                   copyIntArray (args[9], cycleOffset);

                                   processor.replacePhraseRowSteps (varToInt (args[0]),
                                                                     args[1].getArray()->size(),
                                                                     notes,
                                                                     timingMultiplier,
                                                                     durationFraction,
                                                                     velocity,
                                                                     stepMuted,
                                                                     stepSkipped,
                                                                     probability,
                                                                     cycle,
                                                                     cycleOffset);
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseRowTimingOffset",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                               {
                                   processor.setPhraseRowTimingOffset (varToInt (args[0]),
                                                                       varToInt (args[1]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseRowMidiChannel",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                               {
                                   processor.setPhraseRowMidiChannel (varToInt (args[0]),
                                                                      varToInt (args[1]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseRowRecording",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPhraseRowRecording (varToInt (args[0]));

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "drainPhraseRowRecordedNotes",
                           [&processor] (const juce::Array<juce::var>&,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               juce::Array<juce::var> notes;

                               for (const auto note : processor.drainPhraseRowRecordedNotes())
                                   notes.add (note);

                               complete (notes);
                           })
                       .withNativeFunction (
                           "getPhraseRowRecordingKeysHeld",
                           [&processor] (const juce::Array<juce::var>&,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               juce::Array<juce::var> keys;

                               for (const auto note : processor.getPhraseRowRecordingKeysHeld())
                                   keys.add (note);

                               complete (keys);
                           })
                       .withNativeFunction (
                           "injectPhraseRowRecordedNote",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.injectPhraseRowRecordedNote (varToInt (args[0]));

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseStepTimingMultiplier",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseStepTimingMultiplier (varToInt (args[0]),
                                                                            varToInt (args[1]),
                                                                            varToInt (args[2]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseStepDurationFraction",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseStepDurationFraction (varToInt (args[0]),
                                                                            varToInt (args[1]),
                                                                            varToDouble (args[2]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseStepVelocity",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseStepVelocity (varToInt (args[0]),
                                                                    varToInt (args[1]),
                                                                    varToInt (args[2]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseStepMuted",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseStepMuted (varToInt (args[0]),
                                                                 varToInt (args[1]),
                                                                 varToInt (args[2]) != 0);
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseStepSkipped",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseStepSkipped (varToInt (args[0]),
                                                                   varToInt (args[1]),
                                                                   varToInt (args[2]) != 0);
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseStepProbability",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseStepProbability (varToInt (args[0]),
                                                                       varToInt (args[1]),
                                                                       varToInt (args[2]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseStepCycle",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseStepCycle (varToInt (args[0]),
                                                                 varToInt (args[1]),
                                                                 varToInt (args[2]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPhraseStepCycleOffset",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.setPhraseStepCycleOffset (varToInt (args[0]),
                                                                       varToInt (args[1]),
                                                                       varToInt (args[2]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "removePhraseStep",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                               {
                                   processor.removePhraseStep (varToInt (args[0]),
                                                               varToInt (args[1]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "insertPhraseStep",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                               {
                                   processor.insertPhraseStep (varToInt (args[0]),
                                                                 varToInt (args[1]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "duplicatePhraseStep",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                               {
                                   processor.duplicatePhraseStep (varToInt (args[0]),
                                                                  varToInt (args[1]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "movePhraseStep",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 3)
                               {
                                   processor.movePhraseStep (varToInt (args[0]),
                                                             varToInt (args[1]),
                                                             varToInt (args[2]));
                               }

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setPulseIndex",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setPulseIndex (varToInt (args[0]));

                               complete (processor.getPulseIndex());
                           })
                       .withNativeFunction (
                           "setSwingPercent",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setSwingPercent (varToInt (args[0]));

                               complete (processor.getSwingPercent());
                           })
                       .withNativeFunction (
                           "setVelocityHumanizePercent",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setVelocityHumanizePercent (varToInt (args[0]));

                               complete (processor.getVelocityHumanizePercent());
                           })
                       .withNativeFunction (
                           "setTimingHumanizePercent",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setTimingHumanizePercent (varToInt (args[0]));

                               complete (processor.getTimingHumanizePercent());
                           })
                       .withNativeFunction (
                           "setSwingSubdivisionIndex",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setSwingSubdivisionIndex (varToInt (args[0]));

                               complete (processor.getSwingSubdivisionIndex());
                           })
                       .withNativeFunction (
                           "setRowColorsEnabled",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setRowColorsEnabled (varToInt (args[0]) != 0);

                               complete (processor.isRowColorsEnabled() ? 1 : 0);
                           })
                       .withNativeFunction (
                           "setCombinationModeEnabled",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 2)
                               {
                                   processor.setCombinationModeEnabled (varToInt (args[0]),
                                                                        varToInt (args[1]) != 0);
                               }

                               complete (processor.getCombinationModeMask());
                           })
                       .withNativeFunction (
                           "setLoopBraceEnabled",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setLoopBraceEnabled (varToInt (args[0]) != 0);

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setLoopBraceStart",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setLoopBraceStartQuarters (varToDouble (args[0]));

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setLoopBraceEnd",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setLoopBraceEndQuarters (varToDouble (args[0]));

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "getLoopPlaybackBeat",
                           [&processor] (const juce::Array<juce::var>&,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               complete (processor.getLoopPlaybackBeat());
                           })
                       .withNativeFunction (
                           "getPlaybackBeat",
                           [&processor] (const juce::Array<juce::var>&,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               complete (processor.getPlaybackBeat());
                           })
                       .withNativeFunction (
                           "setStandaloneTransportPlaying",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setStandaloneTransportPlaying (varToInt (args[0]) != 0);

                               complete (juce::var {});
                           })
                       .withNativeFunction (
                           "setStandaloneTempoBpm",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.setStandaloneTempoBpm (varToDouble (args[0]));

                               complete (processor.getStandaloneTempoBpm());
                           })
                       .withNativeFunction (
                           "setHostCursor",
                           [&processor] (const juce::Array<juce::var>& args,
                                         juce::WebBrowserComponent::NativeFunctionCompletion complete) {
                               if (args.size() >= 1)
                                   processor.notifyWebHostCursor (args[0].toString());

                               complete (juce::var {});
                           })
                       .withUserScript (juce::String { R"(
                           document.documentElement.classList.add('juce-ready');
                       )" });

#if JUCE_WEB_BROWSER_RESOURCE_PROVIDER_AVAILABLE
    options = options.withResourceProvider (
        [] (const juce::String& url) { return getResource (url); },
        juce::URL { "http://localhost:5174" }.getOrigin());
#endif

#if JUCE_WINDOWS
    options = options.withBackend (Options::Backend::webview2)
                     .withWinWebView2Options (
                         Options::WinWebView2{}
                             .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)));
#endif

    return options;
}

#endif
