#include "PluginEditor.h"
#include "ui/PhrasesNativeEditor.h"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace phrases::ui;

TEST_CASE ("Native edits undo atomically and reject stale history", "[native]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    Controller c (p);
    c.begin();
    p.setPhraseNote (0, 0, 65);
    p.setPhraseStepVelocity (0, 0, 72);
    REQUIRE (c.commit());
    REQUIRE (c.undo());
    CHECK (p.getPhraseNote (0, 0) == 60);
    CHECK (p.getPhraseStepVelocity (0, 0) == 100);
    REQUIRE (c.redo());
    CHECK (p.getPhraseNote (0, 0) == 65);
    CHECK (p.getPhraseStepVelocity (0, 0) == 72);
    p.setPhraseNote (0, 0, 77);
    CHECK_FALSE (c.undo());
    CHECK (p.getPhraseNote (0, 0) == 77);
}
TEST_CASE ("Native selection copies all step attributes between rows within capacity", "[native]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    p.setPhraseNote (0, 0, 68);
    p.setPhraseStepProbability (0, 0, 42);
    p.setPhraseStepCycle (0, 0, 5);
    Controller c (p);
    c.selectAll (0);
    c.moveSelected (0, 0, 1, 0, true);
    CHECK (p.getPhraseRowStepCount (0) == 1);
    REQUIRE (p.getPhraseRowStepCount (1) == 1);
    CHECK (p.getPhraseNote (1, 0) == 68);
    CHECK (p.getPhraseStepProbability (1, 0) == 42);
    CHECK (p.getPhraseStepCycle (1, 0) == 5);
    REQUIRE (c.undo());
    CHECK (p.getPhraseRowStepCount (1) == 0);
    c.begin();
    c.selectAll (0);
    c.editSelected ([] (Step& s) { s.note = 50; });
    c.cancel();
    CHECK (p.getPhraseNote (0, 0) == 68);
}
TEST_CASE ("Native seed generation matches original JavaScript fixtures", "[native][seed]")
{
    const auto file = juce::File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("docs/native-ui-reference/seeding-fixtures.json");
    const auto fixtures = juce::JSON::parse (file);
    REQUIRE (fixtures.isArray());
    for (const auto& fixture : *fixtures.getArray())
    {
        std::array<PluginProcessor::SeedingRowState, 4> settings;
        for (int r = 0; r < 4; ++r)
        {
            const auto row = fixture["settings"][r];
            auto& s = settings[static_cast<size_t> (r)];
            s.phraseLength = row["phraseLength"];
            s.centerMidi = row["centerMidi"];
            s.rangeSemitones = row["rangeSemitones"];
            s.repetition = row["repetition"];
            s.complexity = row["complexity"];
            s.randomness = row["randomness"];
            s.timingMeanMultiplierIndex = row["timingMeanMultiplierIndex"];
            s.timingVariance = row["timingVariance"];
            s.symmetry = static_cast<bool> (row["symmetry"]);
            s.seed = row["seed"];
            s.repetitionSeed = row["repetitionSeed"];
            s.complexitySeed = row["complexitySeed"];
            s.randomnessSeed = row["randomnessSeed"];
            s.timingVarianceSeed = row["timingVarianceSeed"];
        }
        const auto result = generate (settings, fixture["rhythm"], fixture["root"], fixture["mode"]);
        const auto expected = fixture["expected"];
        for (int r = 0; r < 4; ++r)
        {
            INFO ("seed=" << settings[0].seed << " mode=" << static_cast<int> (fixture["mode"]) << " rhythm=" << static_cast<int> (fixture["rhythm"]) << " row=" << r);
            const auto& row = result[static_cast<size_t> (r)];
            CHECK (row.offset == static_cast<int> (expected["rowTimingOffset"][r]));
            REQUIRE (row.steps.size() == static_cast<size_t> (expected["notes"][r].size()));
            for (int i = 0; i < static_cast<int> (row.steps.size()); ++i)
            {
                const auto& step = row.steps[static_cast<size_t> (i)];
                INFO ("step=" << i);
                CHECK (step.note == static_cast<int> (expected["notes"][r][i]));
                CHECK (step.timing == static_cast<int> (expected["stepTimingMultiplier"][r][i]));
                CHECK (step.duration == Catch::Approx (static_cast<double> (expected["stepDurationFraction"][r][i])));
                CHECK (step.velocity == static_cast<int> (expected["stepVelocity"][r][i]));
                CHECK (step.skipped == static_cast<bool> (expected["stepSkipped"][r][i]));
                CHECK (step.probability == static_cast<int> (expected["stepProbability"][r][i]));
            }
        }
    }
}
TEST_CASE ("Native preview uses isolated engine without changing live project", "[native]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    p.setPhraseNote (0, 0, 64);
    p.setPhraseStepDurationFraction (0, 0, .5);
    juce::MemoryBlock before, after;
    p.getStateInformation (before);
    const auto notes = renderPreview (p, 4);
    p.getStateInformation (after);
    CHECK (before == after);
    REQUIRE (notes.size() == 4);
    CHECK (notes.front().note == 64);
    CHECK (notes.front().start == Catch::Approx (0));
    CHECK (notes.front().end == Catch::Approx (.5));
    p.setPatternGlobalTransposeSemitones (12);
    const auto shifted = renderPreview (p, 4);
    REQUIRE_FALSE (shifted.empty());
    CHECK (shifted.front().note == 76);
}
TEST_CASE ("Native editor reference renders with shared layout at supported scales", "[native][render]")
{
    PluginProcessor p;
    p.setStandaloneTransportAvailableForTests (true);
    p.setCurrentPatternSlot (1);
    p.setPatternScale (0, 2);
    const int pitches[3][4] { { 62, 56, 0, 0 }, { 67, 0, 0, 0 }, { 48, 51, 48, 48 } };
    const int lengths[3][4] { { 3, 3, 0, 0 }, { 1, 0, 0, 0 }, { 5, 8, 6, 9 } };
    const int velocities[3][4] { { 68, 94, 0, 0 }, { 92, 0, 0, 0 }, { 77, 112, 73, 48 } };
    const double durations[3][4] { { .79, .79, 0, 0 }, { .79, 0, 0, 0 }, { .79, .7276506, .82, .76 } };
    const int counts[] { 2, 1, 4 };
    for (int r = 0; r < 3; ++r)
        for (int s = 0; s < counts[r]; ++s)
        {
            p.insertPhraseStep (r, s);
            p.setPhraseNote (r, s, pitches[r][s]);
            p.setPhraseStepTimingMultiplier (r, s, lengths[r][s]);
            p.setPhraseStepVelocity (r, s, velocities[r][s]);
            p.setPhraseStepDurationFraction (r, s, durations[r][s]);
        }
    p.setPhraseRowTimingOffset (0, 16);
    p.setPhraseRowTimingOffset (1, 14);
    p.setPhraseRowTimingOffset (3, 12);
    for (int r = 0; r < 4; ++r)
        p.setPhraseRowMuted (r, r == 3);
    p.setProjectMetadata ("A5", {}, "2026-10-09T12:00:00", "2026-10-09T12:00:00", "dark", 100, false);
    const auto output = juce::SystemStats::getEnvironmentVariable ("MIDI_PHRASES_NATIVE_CAPTURE", {});
    if (output.isNotEmpty())
    {
        juce::MemoryBlock fixture;
        p.getStateInformation (fixture);
        REQUIRE (juce::File (output).createDirectory());
        REQUIRE (juce::File (output).getChildFile ("fixture.midiphrases").replaceWithData (fixture.getData(), fixture.getSize()));
        REQUIRE (juce::File (output).getChildFile ("fixture-state.txt").replaceWithText (fixture.toBase64Encoding()));
        struct Variant
        {
            const char* name;
            const char* theme;
            int scale;
            bool compact;
        };
        for (const auto& variant : { Variant { "light", "light", 100, false }, Variant { "alt", "alt", 100, false }, Variant { "compact", "dark", 100, true }, Variant { "70", "dark", 70, false }, Variant { "50", "dark", 50, false } })
        {
            p.setProjectMetadata ("A5", {}, "2026-10-09T12:00:00", "2026-10-09T12:00:00", variant.theme, variant.scale, variant.compact);
            p.setProjectUiScalePercent (variant.scale);
            juce::MemoryBlock state;
            p.getStateInformation (state);
            REQUIRE (juce::File (output).getChildFile ("fixture-" + juce::String (variant.name) + "-state.txt").replaceWithText (state.toBase64Encoding()));
        }
        p.setProjectMetadata ("A5", {}, "2026-10-09T12:00:00", "2026-10-09T12:00:00", "dark", 100, false);
        p.setProjectUiScalePercent (100);
    }
    for (int pass = 0; pass < 3; ++pass)
    {
        const int percent = pass == 0 ? 100 : pass == 1 ? 70
                                                        : 50;
        p.setProjectUiScalePercent (percent);
        PluginEditor wrapper (p);
        auto* native = dynamic_cast<NativeEditor*> (wrapper.getChildComponent (0));
        REQUIRE (native != nullptr);
        auto& editor = *native;
        editor.preview.refresh();
        editor.preview.fit();
        const auto first = editor.grid.stepBounds (0, 0);
        const auto second = editor.grid.stepBounds (0, 1);
        CHECK (first.getHeight() == Catch::Approx (96));
        CHECK (second.getX() > first.getRight());
        CHECK (second.getRight() < 1690);
        const float scale = pass == 0 ? 1.0f : pass == 1 ? .7f
                                                         : .5f;
        const auto screenshot = wrapper.createComponentSnapshot (wrapper.getLocalBounds());
        CHECK (screenshot.isValid());
        if (output.isNotEmpty())
        {
            const auto directory = juce::File (output);
            REQUIRE (directory.createDirectory());
            auto stream = directory.getChildFile ("native-dark-" + juce::String (juce::roundToInt (scale * 100)) + ".png").createOutputStream();
            REQUIRE (stream != nullptr);
            stream->setPosition (0);
            stream->truncate();
            REQUIRE (juce::PNGImageFormat().writeImageToStream (screenshot, *stream));
        }
        if (pass == 0 && output.isNotEmpty())
        {
            auto capture = [&] (const juce::String& name) {
                for (const float factor : { 1.0f, 2.0f })
                {
                    const auto file = juce::File (output).getChildFile ("native-" + name + (factor > 1 ? "@2x" : "") + ".png");
                    auto stream = file.createOutputStream();
                    REQUIRE (stream != nullptr);
                    stream->setPosition (0);
                    stream->truncate();
                    REQUIRE (juce::PNGImageFormat().writeImageToStream (wrapper.createComponentSnapshot (wrapper.getLocalBounds(), true, factor), *stream));
                }
            };
            capture ("dark-100");
            editor.showInspector (0, 0);
            capture ("inspector");
            editor.showInspector (0, 0);
            editor.showRowEditor (2);
            capture ("row-roll");
            editor.preview.drawVelocity = true;
            capture ("row-velocity");
            editor.showRowEditor (2);
            editor.showSeed();
            capture ("seed");
            editor.showSeed();
            editor.grid.compact = true;
            editor.grid.updateLayout();
            capture ("compact");
            editor.grid.compact = false;
            editor.grid.updateLayout();
            editor.applyTheme ("light");
            capture ("light");
            editor.applyTheme ("alt");
            capture ("alt");
            editor.applyTheme ("dark");
        }
    }
}

namespace
{
    Control* findControl (NativeEditor& editor, const juce::String& name)
    {
        for (auto* child : editor.getChildren())
            if (child->getName() == name)
                return dynamic_cast<Control*> (child);
        return nullptr;
    }
    juce::MouseEvent pointerEvent (juce::Component& target, juce::Point<float> at, juce::Point<float> start, bool dragged = false, int modifiers = juce::ModifierKeys::leftButtonModifier)
    {
        const auto now = juce::Time::getCurrentTime();
        return { juce::Desktop::getInstance().getMainMouseSource(), at, juce::ModifierKeys (modifiers), 1, 0, 0, 0, 0, &target, &target, now, start, now, 1, dragged };
    }
}
TEST_CASE ("Native repeated relative drags stay anchored and undo together", "[native][interaction]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    NativeEditor editor (p);
    editor.setSize (1690, 980);
    auto* field = findControl (editor, "Bulk Len");
    REQUIRE (field != nullptr);
    const juce::Point<float> start { 16, 16 };
    field->mouseDown (pointerEvent (*field, start, start));
    field->mouseDrag (pointerEvent (*field, { 16, 8 }, start, true));
    CHECK (p.getPhraseStepTimingMultiplier (0, 0) == 4);
    field->mouseDrag (pointerEvent (*field, { 16, 0 }, start, true));
    CHECK (p.getPhraseStepTimingMultiplier (0, 0) == 5);
    field->mouseDrag (pointerEvent (*field, { 16, 8 }, start, true));
    CHECK (p.getPhraseStepTimingMultiplier (0, 0) == 4);
    field->mouseUp (pointerEvent (*field, { 16, 8 }, start, true));
    REQUIRE (editor.controller.undo());
    CHECK (p.getPhraseStepTimingMultiplier (0, 0) == 3);
}
TEST_CASE ("Native gestures cannot overwrite a restored project", "[native][interaction]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    NativeEditor editor (p);
    editor.setSize (1690, 980);
    auto* field = findControl (editor, "Swing");
    REQUIRE (field != nullptr);
    const juce::Point<float> start { 15, 15 };
    field->mouseDown (pointerEvent (*field, start, start));
    PluginProcessor replacement;
    replacement.setSwingPercent (37);
    juce::MemoryBlock state;
    replacement.getStateInformation (state);
    p.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    field->mouseDrag (pointerEvent (*field, { 15, -30 }, start, true));
    field->mouseUp (pointerEvent (*field, { 15, -30 }, start, true));
    CHECK (p.getSwingPercent() == 37);
    CHECK_FALSE (editor.controller.canUndo());
    p.insertPhraseStep (0, 0);
    editor.controller.refresh();
    editor.grid.updateLayout();
    const auto point = editor.grid.stepBounds (0, 0).getTopLeft() + juce::Point<float> (10, 10);
    editor.grid.mouseDown (pointerEvent (editor.grid, point, point));
    p.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    editor.grid.mouseUp (pointerEvent (editor.grid, point, point));
    CHECK (p.getPhraseRowStepCount (0) == 0);
    CHECK (p.getSwingPercent() == 37);
}
TEST_CASE ("Native undo preserves independent recording and metadata", "[native]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    Controller c (p);
    c.begin();
    p.setPhraseNote (0, 0, 67);
    REQUIRE (c.commit());
    p.insertPhraseStep (1, 0);
    p.setPhraseNote (1, 0, 44);
    p.setProjectMetadata ("Saved name", "Description", {}, {}, "dark", 100, false);
    REQUIRE (c.undo());
    CHECK (p.getPhraseNote (0, 0) == 60);
    CHECK (p.getPhraseNote (1, 0) == 44);
    CHECK (p.getProjectName() == "Saved name");
    REQUIRE (c.redo());
    CHECK (p.getPhraseNote (0, 0) == 67);
    CHECK (p.getPhraseNote (1, 0) == 44);
}

TEST_CASE ("Native boundary duplication and compact pitch drags preserve step data", "[native][interaction]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    p.setPhraseNote (0, 0, 64);
    p.setPhraseStepProbability (0, 0, 43);
    NativeEditor editor (p);
    editor.setSize (1690, 980);
    auto edge = editor.grid.stepBounds (0, 0).getTopRight() + juce::Point<float> (0, 50);
    editor.grid.mouseDoubleClick (pointerEvent (editor.grid, edge, edge, false, juce::ModifierKeys::altModifier));
    REQUIRE (p.getPhraseRowStepCount (0) == 2);
    CHECK (p.getPhraseNote (0, 1) == 64);
    CHECK (p.getPhraseStepProbability (0, 1) == 43);
    editor.grid.compact = true;
    editor.grid.updateLayout();
    const auto middle = editor.grid.stepBounds (0, 0).getCentre();
    editor.grid.mouseDown (pointerEvent (editor.grid, middle, middle));
    editor.grid.mouseDrag (pointerEvent (editor.grid, middle.translated (0, -16), middle, true));
    editor.grid.mouseUp (pointerEvent (editor.grid, middle.translated (0, -16), middle, true));
    CHECK (p.getPhraseNote (0, 0) == 66);
    CHECK (p.getPhraseStepProbability (0, 0) == 43);
    REQUIRE (editor.controller.undo());
    CHECK (p.getPhraseNote (0, 0) == 64);
}
TEST_CASE ("Native scale dialog commits one undoable edit", "[native][interaction]")
{
    PluginProcessor p;
    NativeEditor editor (p);
    editor.setSize (1690, 980);
    auto* scale = findControl (editor, "Scale mode");
    REQUIRE (scale != nullptr);
    scale->begin();
    scale->action ({});
    scale->end();
    Control* root = nullptr;
    Control* done = nullptr;
    for (auto* child : editor.getChildren())
        for (auto* nested : child->getChildren())
        {
            if (nested->getName() == "Root D3")
                root = dynamic_cast<Control*> (nested);
            if (nested->getName() == "Done")
                done = dynamic_cast<Control*> (nested);
        }
    REQUIRE (root != nullptr);
    REQUIRE (done != nullptr);
    root->action ({});
    CHECK (p.getPatternScaleRoot (0) == 2);
    done->action ({});
    juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    REQUIRE (editor.controller.undo());
    CHECK (p.getPatternScaleRoot (0) == 0);
}
TEST_CASE ("Native copy respects fixed row capacity", "[native]")
{
    PluginProcessor p;
    for (int s = 0; s < 64; ++s)
        p.insertPhraseStep (0, s);
    p.insertPhraseStep (1, 0);
    Controller c (p);
    c.selectAll (1);
    const auto before = readRows (p);
    c.moveSelected (1, 0, 0, 32, true);
    CHECK (readRows (p) == before);
    CHECK_FALSE (c.canUndo());
}

TEST_CASE ("Native playback highlights match the original JavaScript timing fixtures", "[native][schedule]")
{
    const auto file = juce::File (__FILE__).getParentDirectory().getParentDirectory().getChildFile ("docs/native-ui-reference/active-step-fixtures.json");
    const auto fixtures = juce::JSON::parse (file);
    REQUIRE (fixtures.isArray());
    for (const auto& fixture : *fixtures.getArray())
    {
        Row row;
        row.offset = fixture["row"]["offset"];
        row.muted = false;
        for (const auto& step : *fixture["row"]["steps"].getArray())
            row.steps.push_back ({ step["note"], step["timing"], step["velocity"], step["probability"], step["cycle"], step["mask"], step["duration"], step["muted"], step["skipped"] });
        for (const auto& example : *fixture["cases"].getArray())
        {
            INFO ("pulse " << static_cast<int> (fixture["pulse"]) << " offset " << row.offset << " swing " << static_cast<int> (fixture["swing"]) << " step " << static_cast<int> (example[0]) << " beat " << static_cast<double> (example[1]));
            CHECK (stepActive (row, static_cast<size_t> (static_cast<int> (example[0])), example[1], fixture["pulse"], fixture["swing"], 1) == static_cast<bool> (example[2]));
        }
    }
}

TEST_CASE ("Inspector keyboard scrolls the full piano and blocks out-of-scale keys", "[native][keyboard]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    p.setPhraseNote (0, 0, 60);
    p.setPatternScale (0, 1);
    NativeEditor editor (p);
    editor.setSize (1690, 1044);
    editor.showInspector (0, 0);
    Keyboard* keys = nullptr;
    for (auto* child : editor.getChildren())
        if (auto* found = dynamic_cast<Keyboard*> (child))
            keys = found;
    REQUIRE (keys != nullptr);
    CHECK (keys->scrolling);
    CHECK (keys->accepts (0));
    CHECK (keys->accepts (60));
    CHECK_FALSE (keys->accepts (61));
    CHECK (keys->boundsFor (0).getWidth() > 0);
    CHECK (keys->boundsFor (127).getRight() > keys->boundsFor (0).getX());
    CHECK (editor.grid.flipsActive());
    const float before = keys->boundsFor (0).getX();
    keys->mouseDown (pointerEvent (*keys, { 20, 20 }, { 20, 20 }));
    keys->mouseUp (pointerEvent (*keys, { 20, 20 }, { 20, 20 }));
    CHECK (keys->boundsFor (0).getX() > before);
    const auto sharp = keys->boundsFor (61).getCentre();
    keys->mouseDown (pointerEvent (*keys, sharp, sharp));
    keys->mouseUp (pointerEvent (*keys, sharp, sharp));
    CHECK (p.getPhraseNote (0, 0) == 60);
    const auto natural = keys->boundsFor (62).getCentre();
    keys->mouseDown (pointerEvent (*keys, natural, natural));
    keys->mouseUp (pointerEvent (*keys, natural, natural));
    CHECK (p.getPhraseNote (0, 0) == 62);
    auto* source = findControl (editor, "Select pattern 1");
    auto* destination = findControl (editor, "Select pattern 2");
    REQUIRE (source != nullptr);
    REQUIRE (destination != nullptr);
    const int shift = juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier;
    source->mouseDown (pointerEvent (*source, { 4, 4 }, { 4, 4 }, false, shift));
    source->mouseUp (pointerEvent (*source, { 4, 4 }, { 4, 4 }, false, shift));
    REQUIRE (source->copySource);
    CHECK (source->copySource());
    REQUIRE (destination->copyTarget);
    CHECK (destination->copyTarget());
    auto* clear = findControl (editor, "Clear selected pattern");
    REQUIRE (clear != nullptr);
    clear->mouseDown (pointerEvent (*clear, { 4, 4 }, { 4, 4 }));
    clear->mouseUp (pointerEvent (*clear, { 4, 4 }, { 4, 4 }));
    CHECK_FALSE (source->copySource());
}
