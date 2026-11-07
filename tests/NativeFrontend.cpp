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
TEST_CASE ("The logo retains all three interior tracks through the end of each letter", "[native][render]")
{
    Palette palette;
    juce::Image rendered (juce::Image::RGB, 270 * 4, 32 * 4, true);
    juce::Graphics graphics (rendered);
    graphics.addTransform (juce::AffineTransform::scale (4.0f));
    logo (graphics, {}, palette);

    // The first I has three tracks centered at x = 33, 35, and 37.
    for (const int x : { 33, 35, 37 })
    {
        CHECK (rendered.getPixelAt (x * 4, 22 * 4 + 1).getBrightness() < 0.4f);
        CHECK (rendered.getPixelAt (x * 4, 23 * 4 + 2).getBrightness() > 0.8f);
        CHECK (rendered.getPixelAt (x * 4, 25 * 4).getBrightness() < 0.4f);
    }
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
        for (const auto& variant : { Variant { "alt", "alt", 100, false }, Variant { "compact", "dark", 100, true }, Variant { "70", "dark", 70, false }, Variant { "50", "dark", 50, false } })
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
        wrapper.setSize (PluginEditor::designWidth * percent / 100, PluginEditor::designHeight * percent / 100);
        auto* native = dynamic_cast<NativeEditor*> (wrapper.getChildComponent (0));
        REQUIRE (native != nullptr);
        auto& editor = *native;
        editor.preview.refresh();
        editor.preview.fit();
        const auto first = editor.grid.stepBounds (0, 0);
        const auto second = editor.grid.stepBounds (0, 1);
        CHECK (first.getHeight() == Catch::Approx (108));
        CHECK (second.getX() > first.getRight());
        CHECK (second.getRight() < PluginEditor::designWidth);
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
            editor.toggleVelocityMode();
            capture ("row-velocity");
            editor.toggleVelocityMode();
            editor.showSeed();
            capture ("seed");
            editor.showSeed();
            editor.grid.compact = true;
            editor.grid.updateLayout();
            capture ("compact");
            editor.grid.compact = false;
            editor.grid.updateLayout();
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
TEST_CASE ("Standalone tempo arrows step in both directions", "[native]")
{
    PluginProcessor processor;
    processor.setStandaloneTransportAvailableForTests (true);
    processor.setStandaloneTempoBpm (120);
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    auto* up = findControl (editor, "Increase tempo by 1 BPM");
    auto* down = findControl (editor, "Decrease tempo by 1 BPM");
    auto* tempo = findControl (editor, "Tempo");
    auto* play = findControl (editor, "Play or stop (Space)");
    REQUIRE (up != nullptr);
    REQUIRE (down != nullptr);
    REQUIRE (tempo != nullptr);
    REQUIRE (play != nullptr);
    CHECK (play->getY() == tempo->getY());
    CHECK (tempo->getY() == 42);
    CHECK (tempo->getBottom() < editor.grid.getY());
    CHECK (up->isVisible());
    CHECK (down->isVisible());
    CHECK_FALSE (up->getBounds().intersects (down->getBounds()));
    CHECK (up->getY() == tempo->getY());
    CHECK (down->getBottom() == tempo->getBottom());
    const auto click = [] (Control& control) {
        const auto point = control.getLocalBounds().getCentre().toFloat();
        const auto event = pointerEvent (control, point, point);
        control.mouseDown (event);
        control.mouseUp (event);
    };
    click (*up);
    CHECK (processor.getStandaloneTempoBpm() == Catch::Approx (121));
    click (*down);
    CHECK (processor.getStandaloneTempoBpm() == Catch::Approx (120));
}

TEST_CASE ("Desktop standalone starts smaller and gives the removed header space to the rows", "[native][scale]")
{
   #if ! JUCE_IOS
    PluginProcessor processor;
    PluginEditor plugin (processor);
    const int pluginWidth = plugin.getWidth();
    processor.setStandaloneTransportAvailableForTests (true);
    PluginEditor standalone (processor);
    CHECK (standalone.getWidth() == Catch::Approx (std::max (400.0, pluginWidth * .75)).margin (4));
    CHECK (standalone.getWidth() * 3 == standalone.getHeight() * 4);
    const auto* pluginView = dynamic_cast<NativeEditor*> (plugin.getChildComponent (0));
    const auto* standaloneView = dynamic_cast<NativeEditor*> (standalone.getChildComponent (0));
    REQUIRE (pluginView != nullptr);
    REQUIRE (standaloneView != nullptr);
    CHECK (standaloneView->grid.getY() == pluginView->grid.getY());
    CHECK (standaloneView->grid.getHeight() == pluginView->grid.getHeight() + 48);
   #endif
}

TEST_CASE ("Seed slider accents follow the targeted row", "[native][interaction]")
{
    PluginProcessor processor;
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    editor.showSeed (1);
    auto* steps = findControl (editor, "Seed Steps");
    auto* target = findControl (editor, "Target seed row 3");
    REQUIRE (steps != nullptr);
    REQUIRE (target != nullptr);
    const auto trackColour = [&] {
        juce::Image image (juce::Image::RGB, steps->getWidth(), steps->getHeight(), true);
        juce::Graphics graphics (image);
        steps->paint (graphics);
        return image.getPixelAt (12, steps->getHeight() / 2);
    };
    CHECK (trackColour() == editor.palette.rows[1]);
    const auto clickTarget = [&] {
        const auto centre = target->getLocalBounds().getCentre().toFloat();
        const auto event = pointerEvent (*target, centre, centre);
        target->mouseDown (event);
        target->mouseUp (event);
    };
    clickTarget();
    CHECK (trackColour() == editor.palette.rows[2]);
    clickTarget();
    CHECK (trackColour() == editor.palette.rows[1]);
}

TEST_CASE ("Legacy light projects open in dark and the theme toggle has two positions", "[native]")
{
    PluginProcessor processor;
    constexpr char legacyState[] = R"xml(<MidiPhrases projectThemeMode="light"/>)xml";
    processor.setStateInformation (legacyState, static_cast<int> (sizeof (legacyState) - 1));
    CHECK (processor.getProjectThemeMode() == "dark");

    NativeEditor editor (processor);
    auto* toggle = findControl (editor, "Change theme");
    REQUIRE (toggle != nullptr);
    const auto click = pointerEvent (*toggle, { 10, 10 }, { 10, 10 });
    toggle->mouseDown (click);
    toggle->mouseUp (click);
    CHECK (processor.getProjectThemeMode() == "alt");
    toggle->mouseDown (click);
    toggle->mouseUp (click);
    CHECK (processor.getProjectThemeMode() == "dark");
}

TEST_CASE ("Numbered slots recall their own loop brace", "[native]")
{
    PluginProcessor processor;
    NativeEditor editor (processor);
    auto* first = findControl (editor, "Select slot 1");
    auto* last = findControl (editor, "Select slot 16");
    REQUIRE (first != nullptr);
    REQUIRE (last != nullptr);
    CHECK (findControl (editor, "Change slot types") == nullptr);
    CHECK (first->caption == "1");
    CHECK (last->caption == "16");
    const auto click = [] (Control& control) {
        const auto event = pointerEvent (control, { 10, 10 }, { 10, 10 });
        control.mouseDown (event);
        control.mouseUp (event);
    };

    click (*last);
    REQUIRE (processor.getCurrentPatternSlot() == 15);
    CHECK_FALSE (processor.isLoopBraceEnabled());
    processor.setLoopBraceStartQuarters (1.0);
    processor.setLoopBraceEndQuarters (4.0);
    processor.setLoopBraceEnabled (true);
    click (*first);
    CHECK_FALSE (processor.isLoopBraceEnabled());
    click (*last);
    CHECK (processor.isLoopBraceEnabled());
    CHECK (processor.getLoopBraceStartQuarters() == Catch::Approx (1.0));
    CHECK (processor.getLoopBraceEndQuarters() == Catch::Approx (4.0));
}

TEST_CASE ("Saved loop slots migrate to numbered slots with enabled braces", "[native]")
{
    PluginProcessor processor;
    constexpr char legacyState[] = R"xml(<MidiPhrases version="28" currentPatternSlot="0" currentLoopSlot="0"><LoopSlot index="0" assigned="1" patternSlot="0" start="1" end="4"/></MidiPhrases>)xml";
    processor.setStateInformation (legacyState, static_cast<int> (sizeof (legacyState) - 1));
    CHECK (processor.getCurrentPatternSlot() == 8);
    CHECK (processor.isLoopBraceEnabled());
    CHECK (processor.getLoopBraceStartQuarters() == Catch::Approx (1.0));
    CHECK (processor.getLoopBraceEndQuarters() == Catch::Approx (4.0));
    CHECK_FALSE (processor.isPatternLoopBraceEnabled (9));
    CHECK (PluginProcessor::midiMuteTriggerNote == 16);
}

TEST_CASE ("Compact cards show pitch and let duration move independently of timing", "[native]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    processor.insertPhraseStep (0, 1);
    processor.setPhraseNote (0, 0, 55);
    processor.setPhraseNote (0, 1, 67);
    processor.setPhraseStepTimingMultiplier (0, 1, 5);
    processor.insertPhraseStep (1, 0);
    processor.setPhraseRowTimingOffset (1, 12);
    processor.setPhraseStepDurationFraction (0, 0, .5);
    Controller controller (processor);
    Palette palette;
    palette.set ("dark");
    Grid grid (controller, palette);
    grid.setSize (900, 500);
    const auto detailedFirst = grid.stepBounds (0, 0);
    const auto detailedSecond = grid.stepBounds (0, 1);
    const auto detailedOffset = grid.stepBounds (1, 0);
    const auto render = [&] {
        juce::Image snapshot (juce::Image::RGB, grid.getWidth(), grid.getHeight(), true);
        juce::Graphics graphics (snapshot);
        grid.paint (graphics);
        return snapshot;
    };
    const auto detailedImage = render();
    grid.compact = true;
    grid.updateLayout();

    const auto card = grid.stepBounds (0, 0);
    const auto second = grid.stepBounds (0, 1);
    const auto offset = grid.stepBounds (1, 0);
    const auto compactImage = render();
    for (int y = 0; y < grid.getHeight(); ++y)
        CHECK (compactImage.getPixelAt (10, y) == detailedImage.getPixelAt (10, y));
    for (const float fraction : { .25f, .5f, .75f })
    {
        const int x = juce::roundToInt (card.getX() + card.getWidth() * fraction);
        for (const int y : { 8, 17, 28 })
            CHECK (compactImage.getPixelAt (x, juce::roundToInt (card.getY()) + y)
                   == detailedImage.getPixelAt (x, juce::roundToInt (detailedFirst.getY()) + y));
    }
    CHECK (card.getX() == Catch::Approx (detailedFirst.getX()));
    CHECK (card.getWidth() == Catch::Approx (detailedFirst.getWidth()));
    CHECK (second.getX() == Catch::Approx (detailedSecond.getX()));
    CHECK (second.getWidth() == Catch::Approx (detailedSecond.getWidth()));
    CHECK (offset.getX() == Catch::Approx (detailedOffset.getX()));
    CHECK (offset.getWidth() == Catch::Approx (detailedOffset.getWidth()));
    CHECK (card.getHeight() < 96.0f);
    CHECK (second.getY() < card.getY());
    CHECK (card.getBottom() <= 135.0f);
    const int timing = controller.rows[0].steps[0].timing;
    const auto edge = card.getCentre();
    const auto target = edge.translated ((card.getWidth() - 20.0f) * .25f, 0);
    grid.mouseDown (pointerEvent (grid, edge, edge));
    grid.mouseDrag (pointerEvent (grid, target, edge, true));
    grid.mouseUp (pointerEvent (grid, target, edge, true));
    CHECK (controller.rows[0].steps[0].duration == Catch::Approx (.75));
    CHECK (controller.rows[0].steps[0].timing == timing);
    REQUIRE (controller.undo());
    CHECK (controller.rows[0].steps[0].duration == Catch::Approx (.5));

    processor.setPhraseStepDurationFraction (0, 0, 1.0);
    controller.refresh();
    const juce::Point<float> fullDuration { card.getRight() - 10.0f, card.getCentreY() };
    const auto shorter = fullDuration.translated (-(card.getWidth() - 20.0f) * .25f, 0);
    grid.mouseDown (pointerEvent (grid, fullDuration, fullDuration));
    grid.mouseDrag (pointerEvent (grid, shorter, fullDuration, true));
    grid.mouseUp (pointerEvent (grid, shorter, fullDuration, true));
    CHECK (controller.rows[0].steps[0].duration == Catch::Approx (.75));
    CHECK (controller.rows[0].steps[0].timing == timing);

    const juce::Point<float> rightEdge { card.getRight() - 1, card.getCentreY() };
    const auto wider = rightEdge.translated (card.getWidth() * .5f, 0);
    grid.mouseDown (pointerEvent (grid, rightEdge, rightEdge));
    grid.mouseDrag (pointerEvent (grid, wider, rightEdge, true));
    grid.mouseUp (pointerEvent (grid, wider, rightEdge, true));
    CHECK (controller.rows[0].steps[0].duration == Catch::Approx (.75));
    CHECK (controller.rows[0].steps[0].timing > timing);
}

TEST_CASE ("A compact duration drag updates the preview and MIDI before release", "[native][interaction]")
{
    PluginProcessor processor;
    processor.setStandaloneTransportAvailableForTests (true);
    processor.setStandaloneTempoBpm (120);
    processor.insertPhraseStep (0, 0);
    processor.setPhraseStepDurationFraction (0, 0, .5);
    constexpr int sampleRate = 8000, blockSize = 128;
    processor.prepareToPlay (sampleRate, blockSize);
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    REQUIRE (editor.grid.compact);
    const auto card = editor.grid.stepBounds (0, 0);
    const auto grip = card.getCentre();
    const auto target = grip.translated ((card.getWidth() - 20.0f) * .25f, 0);
    editor.grid.mouseDown (pointerEvent (editor.grid, grip, grip));
    editor.grid.mouseDrag (pointerEvent (editor.grid, target, grip, true));
    REQUIRE (editor.controller.gestureActive());
    CHECK (processor.getPhraseStepDurationFraction (0, 0) == Catch::Approx (.75));

    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (editor.controller.gestureActive());
    REQUIRE_FALSE (editor.preview.events().empty());
    const auto& note = editor.preview.events().front();
    const double duration = .75 * (processor.getPhraseStepTimingMultiplier (0, 0) + 1) * .25
        * PluginProcessor::pulseQuartersForIndex (processor.getPulseIndex());
    CHECK (note.end - note.start == Catch::Approx (duration).margin (2.0 / sampleRate));

    processor.setStandaloneTransportPlaying (true);
    juce::AudioBuffer<float> audio (2, blockSize);
    juce::MidiBuffer midi;
    int noteOn = -1, noteOff = -1;
    for (int start = 0; start < sampleRate && noteOff < 0; start += blockSize)
    {
        midi.clear();
        processor.processBlock (audio, midi);
        for (const auto event : midi)
        {
            if (event.getMessage().isNoteOn() && noteOn < 0)
                noteOn = start + event.samplePosition;
            if (event.getMessage().isNoteOff() && noteOn >= 0 && noteOff < 0)
                noteOff = start + event.samplePosition;
        }
    }
    REQUIRE (noteOn >= 0);
    REQUIRE (noteOff > noteOn);
    CHECK ((noteOff - noteOn) * 2.0 / sampleRate == Catch::Approx (duration).margin (2.0 / sampleRate));
    editor.grid.mouseUp (pointerEvent (editor.grid, target, grip, true));
    processor.releaseResources();
}

TEST_CASE ("The detailed step bar uses the same duration drag as compact mode", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    processor.setPhraseStepDurationFraction (0, 0, .5);
    Controller controller (processor);
    Palette palette;
    Grid grid (controller, palette);
    grid.setSize (900, 500);
    const auto card = grid.stepBounds (0, 0);
    const juce::Point<float> grip { card.getCentreX(), card.getY() + 18 };
    const auto target = grip.translated ((card.getWidth() - 20.0f) * .25f, 0);
    grid.mouseDown (pointerEvent (grid, grip, grip));
    grid.mouseDrag (pointerEvent (grid, target, grip, true));
    grid.mouseUp (pointerEvent (grid, target, grip, true));
    CHECK (processor.getPhraseStepDurationFraction (0, 0) == Catch::Approx (.75));
    CHECK (processor.getPhraseStepTimingMultiplier (0, 0) == 3);
}

TEST_CASE ("Any row V toggles velocity drawing across the grid", "[native][interaction]")
{
    PluginProcessor processor;
    for (int step = 0; step < 3; ++step)
        processor.insertPhraseStep (0, step);
    processor.insertPhraseStep (1, 0);
    processor.setLoopBraceEnabled (true);
    processor.setLoopBraceEndQuarters (6);
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    auto& grid = editor.grid;
    const auto previewBounds = editor.preview.getBounds();
    const auto originalFirst = grid.stepBounds (0, 0);
    const juce::Point<float> v { 62, 106 };
    grid.mouseDown (pointerEvent (grid, v, v));
    grid.mouseUp (pointerEvent (grid, v, v));
    REQUIRE (grid.isVelocityMode());
    CHECK (editor.preview.isVisible());
    CHECK (editor.preview.getBounds() == previewBounds);
    CHECK (grid.stepBounds (0, 0).getX() == Catch::Approx (originalFirst.getX()));
    CHECK (grid.stepBounds (0, 0).getWidth() == Catch::Approx (originalFirst.getWidth()));

    const auto first = grid.stepBounds (0, 0).reduced (2);
    const auto last = grid.stepBounds (0, 2).reduced (2);
    const juce::Point<float> start { first.getCentreX(), first.getY() };
    const juce::Point<float> end { last.getCentreX(), last.getBottom() };
    grid.mouseDown (pointerEvent (grid, start, start));
    grid.mouseDrag (pointerEvent (grid, end, start, true));
    grid.mouseUp (pointerEvent (grid, end, start, true));
    CHECK (processor.getPhraseStepVelocity (0, 0) == 127);
    CHECK (processor.getPhraseStepVelocity (0, 1) == Catch::Approx (64).margin (1));
    CHECK (processor.getPhraseStepVelocity (0, 2) == 0);
    CHECK (processor.getPhraseStepVelocity (1, 0) == 100);
    CHECK (processor.isLoopBraceEnabled());
    CHECK (processor.getLoopBraceEndQuarters() == Catch::Approx (6));
    REQUIRE (editor.controller.undo());
    CHECK (processor.getPhraseStepVelocity (0, 2) == 100);

    const auto secondRowStep = grid.stepBounds (1, 0).reduced (2);
    const juce::Point<float> bottom { secondRowStep.getCentreX(), secondRowStep.getBottom() };
    grid.mouseDown (pointerEvent (grid, bottom, bottom));
    grid.mouseUp (pointerEvent (grid, bottom, bottom));
    CHECK (processor.getPhraseStepVelocity (1, 0) == 0);

    const auto rowStride = grid.stepBounds (1, 0).getY() - grid.stepBounds (0, 0).getY();
    const auto emptyRowV = v.translated (0, 2 * rowStride);
    grid.mouseDown (pointerEvent (grid, emptyRowV, emptyRowV));
    grid.mouseUp (pointerEvent (grid, emptyRowV, emptyRowV));
    CHECK_FALSE (grid.isVelocityMode());
}

TEST_CASE ("Save and Save As dispatch separate project actions", "[native][interaction]")
{
    PluginProcessor processor;
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    std::vector<int> actions;
    editor.projectAction = [&] (int action) { actions.push_back (action); };
    for (auto name : { "SAVE", "SAVE AS" })
    {
        auto* control = findControl (editor, name);
        REQUIRE (control != nullptr);
        const auto point = control->getLocalBounds().getCentre().toFloat();
        const auto event = pointerEvent (*control, point, point);
        control->mouseDown (event);
        control->mouseUp (event);
    }
    CHECK (actions == std::vector<int> { 2, 5 });
}

TEST_CASE ("Main piano roll fits the pitches rendered by playback", "[native]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    processor.setPhraseNote (0, 0, 60);
    processor.insertPhraseStep (1, 0);
    processor.setPhraseNote (1, 0, 24);
    processor.insertPhraseStep (1, 1);
    processor.setPhraseNote (1, 1, 31);
    processor.setPhraseRowMuted (1, true);
    Controller controller (processor);
    Palette palette;
    Preview preview (controller, palette);
    preview.refresh();
    CHECK (preview.lowNote == 58);
    CHECK (preview.highNote == 62);


}

TEST_CASE ("Viewport fit scales the design into the host view", "[native][scale]")
{
    const auto wide = PluginEditor::fitDesignToAvailable (2000.0f, 1000.0f, static_cast<float> (PluginEditor::designWidth), static_cast<float> (PluginEditor::designHeight));
    CHECK (wide.scale == Catch::Approx (1000.0f / PluginEditor::designHeight));
    CHECK (wide.y == Catch::Approx (0.0f).margin (0.01f));
    CHECK (wide.x == Catch::Approx ((2000.0f - PluginEditor::designWidth * wide.scale) * 0.5f));

    const auto tall = PluginEditor::fitDesignToAvailable (820.0f, 1180.0f, static_cast<float> (PluginEditor::designWidth), static_cast<float> (PluginEditor::designHeight));
    CHECK (tall.scale == Catch::Approx (820.0f / PluginEditor::designWidth));
    CHECK (tall.x == Catch::Approx (0.0f).margin (0.01f));
    CHECK (tall.y == Catch::Approx ((1180.0f - PluginEditor::designHeight * tall.scale) * 0.5f));

    const auto empty = PluginEditor::fitDesignToAvailable (0.0f, 800.0f, static_cast<float> (PluginEditor::designWidth), static_cast<float> (PluginEditor::designHeight));
    CHECK (empty.scale == Catch::Approx (1.0f));

    PluginProcessor processor;
    processor.setProjectUiScalePercent (70);
    PluginEditor editor (processor);
    CHECK (editor.getWidth() * 3 == editor.getHeight() * 4);
   #if ! JUCE_IOS
    REQUIRE (editor.getConstrainer() != nullptr);
    CHECK (editor.getConstrainer()->getFixedAspectRatio() == Catch::Approx (4.0 / 3.0));
   #endif
    editor.setSize (2000, 1000);
    auto* native = dynamic_cast<NativeEditor*> (editor.getChildComponent (0));
    REQUIRE (native != nullptr);
    CHECK (native->getWidth() == PluginEditor::designWidth);
    CHECK (native->getHeight() == PluginEditor::designHeight);
    const auto origin = juce::Point<float>().transformedBy (native->getTransform());
    const auto corner = juce::Point<float> (static_cast<float> (PluginEditor::designWidth), static_cast<float> (PluginEditor::designHeight)).transformedBy (native->getTransform());
    CHECK (corner.y - origin.y == Catch::Approx (1000.0f).margin (0.5f));
    CHECK (origin.y == Catch::Approx (0.0f).margin (0.5f));
    CHECK (origin.x == Catch::Approx ((2000.0f - (corner.x - origin.x)) * 0.5f).margin (0.5f));
    CHECK (processor.getProjectUiScalePercent() == 70);

    CHECK (findControl (*native, "UI scale percent") == nullptr);
    CHECK_FALSE (native->controller.canUndo());

    processor.setStandaloneTransportAvailableForTests (true);
    PluginEditor standalone (processor);
    standalone.setSize (1024, 768);
    auto* standaloneView = dynamic_cast<NativeEditor*> (standalone.getChildComponent (0));
    REQUIRE (standaloneView != nullptr);
    CHECK (standaloneView->getHeight() == PluginEditor::designHeight);
    const auto canvasCorner = juce::Point<float> (PluginEditor::designWidth, PluginEditor::designHeight)
                                  .transformedBy (standaloneView->getTransform());
    CHECK (canvasCorner.x == Catch::Approx (1024.0f));
    CHECK (canvasCorner.y == Catch::Approx (768.0f));
}
TEST_CASE ("Native repeated relative drags stay anchored and undo together", "[native][interaction]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    p.insertPhraseStep (0, 1);
    NativeEditor editor (p);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    auto* field = findControl (editor, "Bulk Len");
    REQUIRE (field != nullptr);
    CHECK (editor.grid.getRight() == editor.getWidth() * 4 / 5);
    CHECK (field->getX() >= editor.grid.getRight());
    CHECK (field->getWidth() == field->getHeight());
    CHECK (editor.grid.getBottom() - field->getBottom() == 24);
    auto* newProject = findControl (editor, "NEW");
    REQUIRE (newProject != nullptr);
    CHECK (newProject->getBottom() <= editor.grid.getY());
    CHECK_FALSE (field->isEnabled());

    editor.controller.selection[0].set (0);
    editor.synchronise();
    CHECK (field->isEnabled());
    const juce::Point<float> start { 16, 16 };
    field->mouseDown (pointerEvent (*field, start, start));
    field->mouseDrag (pointerEvent (*field, { 16, 8 }, start, true));
    CHECK (p.getPhraseStepTimingMultiplier (0, 0) == 4);
    field->mouseDrag (pointerEvent (*field, { 16, 0 }, start, true));
    CHECK (p.getPhraseStepTimingMultiplier (0, 0) == 5);
    field->mouseDrag (pointerEvent (*field, { 16, 8 }, start, true));
    CHECK (p.getPhraseStepTimingMultiplier (0, 0) == 4);
    field->mouseUp (pointerEvent (*field, { 16, 8 }, start, true));
    CHECK (p.getPhraseStepTimingMultiplier (0, 1) == 3);
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

TEST_CASE ("Horizontal scrolling pans the whole grid from gaps and shorter rows", "[native][interaction]")
{
    PluginProcessor processor;
    for (int step = 0; step < 4; ++step)
        processor.insertPhraseStep (0, step);
    processor.insertPhraseStep (1, 0);
    Controller controller (processor);
    Palette palette;
    Grid grid (controller, palette);
    grid.compact = true;
    grid.setSize (900, 500);
    const auto first = grid.stepBounds (0, 0);
    const auto shortRow = grid.stepBounds (1, 0);
    const juce::Point<float> gap { 400, 10 };
    grid.mouseWheelMove (pointerEvent (grid, gap, gap), { -.25f, 0, false, true, false });
    const float distance = first.getX() - grid.stepBounds (0, 0).getX();
    CHECK (distance > 0);
    CHECK (shortRow.getX() - grid.stepBounds (1, 0).getX() == Catch::Approx (distance));
    const auto lowerRow = grid.stepBounds (1, 0).getCentre();
    grid.mouseWheelMove (pointerEvent (grid, lowerRow, lowerRow), { 0, -.25f, false, false, false });
    CHECK (first.getX() - grid.stepBounds (0, 0).getX() > distance);
    grid.mouseWheelMove (pointerEvent (grid, gap, gap), { 10, 0, false, true, false });
    CHECK (grid.stepBounds (0, 0) == first);
    CHECK (grid.stepBounds (1, 0) == shortRow);
}

TEST_CASE ("The full gap between compact steps is a single resize boundary", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    processor.insertPhraseStep (0, 1);
    processor.setPhraseNote (0, 0, 48);
    processor.setPhraseNote (0, 1, 72);
    Controller controller (processor);
    Palette palette;
    Grid grid (controller, palette);
    grid.compact = true;
    grid.setSize (900, 500);
    const auto first = grid.stepBounds (0, 0);
    const auto second = grid.stepBounds (0, 1);
    REQUIRE (second.getBottom() < first.getY());
    REQUIRE (second.getX() - first.getRight() > 8.0f);
    const juce::Point<float> gap { (first.getRight() + second.getX()) * .5f,
                                  (second.getBottom() + first.getY()) * .5f };
    const int timing = processor.getPhraseStepTimingMultiplier (0, 0);
    const int nextTiming = processor.getPhraseStepTimingMultiplier (0, 1);
    const float quarterStepWidth = (second.getCentreX() - first.getCentreX()) / static_cast<float> (timing + 1);
    const auto target = gap.translated (quarterStepWidth, 0);
    grid.mouseDown (pointerEvent (grid, gap, gap));
    grid.mouseDrag (pointerEvent (grid, target, gap, true));
    grid.mouseUp (pointerEvent (grid, target, gap, true));
    CHECK (processor.getPhraseStepTimingMultiplier (0, 0) == timing + 1);
    CHECK (processor.getPhraseStepTimingMultiplier (0, 1) == nextTiming);
    CHECK (grid.stepBounds (0, 1).getWidth() == Catch::Approx (second.getWidth()));
    CHECK (grid.stepBounds (0, 1).getX() - second.getX() == Catch::Approx (quarterStepWidth));
    grid.setSize (760, 500);
    CHECK (grid.stepBounds (0, 1).getWidth() == Catch::Approx (second.getWidth()));
    REQUIRE (controller.undo());
    grid.updateLayout();
    grid.mouseDoubleClick (pointerEvent (grid, gap, gap));
    CHECK (processor.getPhraseRowStepCount (0) == 3);
}

TEST_CASE ("Native boundary duplication and compact pitch drags preserve step data", "[native][interaction]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    p.setPhraseNote (0, 0, 64);
    p.setPhraseStepProbability (0, 0, 43);
    NativeEditor editor (p);
    editor.setSize (1690, 980);
    REQUIRE (editor.grid.compact);
    const auto body = editor.grid.stepBounds (0, 0).getCentre();
    editor.grid.mouseDown (pointerEvent (editor.grid, body, body));
    editor.grid.mouseUp (pointerEvent (editor.grid, body, body));
    CHECK_FALSE (editor.grid.compact);
    CHECK_FALSE (p.getProjectStretchStepsToFit());
    CHECK_FALSE (editor.preview.isVisible());
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
TEST_CASE ("Detailed step actions share one vertical column", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    SECTION ("Standard card") { processor.setPhraseStepTimingMultiplier (0, 0, 3); }
    SECTION ("Shortest card") { processor.setPhraseStepTimingMultiplier (0, 0, 0); }
    Controller controller (processor);
    Palette palette;
    Grid grid (controller, palette);
    grid.setSize (900, 500);
    grid.setVisibleRowCount (1);
    const auto card = grid.stepBounds (0, 0);
    const auto corner = card.getTopLeft();
    constexpr float barHeight = 36.0f;
    const float buttonHeight = (card.getHeight() - barHeight) / 3.0f;
    const auto click = [&](float y) {
        const auto point = corner.translated (14, y);
        const auto event = pointerEvent (grid, point, point);
        grid.mouseDown (event);
        grid.mouseUp (event);
    };
    click (barHeight + buttonHeight * 1.5f);
    CHECK (processor.isPhraseStepSkipped (0, 0));
    click (barHeight + buttonHeight * 1.5f);
    CHECK_FALSE (processor.isPhraseStepSkipped (0, 0));
    click (card.getHeight() - 2.0f);
    CHECK (processor.isPhraseStepMuted (0, 0));
    click (barHeight + buttonHeight * .5f);
    CHECK (processor.getPhraseRowStepCount (0) == 0);
}

TEST_CASE ("Shift takes priority over step editing and adds to the marquee selection", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    processor.insertPhraseStep (0, 1);
    processor.insertPhraseStep (1, 0);
    processor.setPhraseStepDurationFraction (0, 0, .5);
    Controller controller (processor);
    controller.selection[1].set (0);
    const auto original = controller.rows;
    Palette palette;
    Grid grid (controller, palette);
    grid.setSize (900, 500);
    const int shift = juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier;
    juce::Point<float> start;

    SECTION ("Compact duration grip")
    {
        grid.compact = true;
        grid.updateLayout();
        start = grid.stepBounds (0, 0).getCentre();
    }
    SECTION ("Detailed delete control")
    {
        start = grid.stepBounds (0, 0).getTopLeft().translated (12, 48);
        grid.mouseDown (pointerEvent (grid, start, start, false, shift));
        grid.mouseUp (pointerEvent (grid, start, start, false, shift));
        CHECK (processor.getPhraseRowStepCount (0) == 2);
        CHECK (controller.selection[0][0]);
    }
    SECTION ("Velocity drawing")
    {
        grid.setVelocityMode (true);
        start = grid.stepBounds (0, 0).getCentre();
    }

    const auto end = grid.stepBounds (0, 1).getCentre().translated (0, 8);
    grid.mouseDown (pointerEvent (grid, start, start, false, shift));
    grid.mouseDrag (pointerEvent (grid, end, start, true));
    grid.mouseUp (pointerEvent (grid, end, start, true));
    CHECK (controller.selection[0][0]);
    CHECK (controller.selection[0][1]);
    CHECK (controller.selection[1][0]);
    CHECK (readRows (processor) == original);
    CHECK_FALSE (controller.canUndo());
}

TEST_CASE ("Clicking the detailed grid background clears selection and returns to compact", "[native][interaction]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    p.setProjectMetadata ("", "", "", "", "dark", 100, false);
    NativeEditor editor (p);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    auto& grid = editor.grid;
    REQUIRE_FALSE (grid.compact);
    editor.showInspector (0, 0);
    const auto card = grid.stepBounds (0, 0);
    const juce::Point<float> background { static_cast<float> (grid.getWidth() - 5), card.getCentreY() + 10 };
    grid.mouseDown (pointerEvent (grid, background, background));
    grid.mouseDrag (pointerEvent (grid, card.getCentre(), background, true));
    grid.mouseUp (pointerEvent (grid, card.getCentre(), background, true));
    CHECK_FALSE (grid.compact);
    CHECK (editor.controller.hasSelection());

    grid.mouseDown (pointerEvent (grid, background, background));
    grid.mouseUp (pointerEvent (grid, background, background));
    CHECK (grid.compact);
    CHECK (p.getProjectStretchStepsToFit());
    CHECK_FALSE (editor.controller.hasSelection());
    CHECK (grid.inspectedStep == -1);
}

TEST_CASE ("Native scale dialog commits one undoable edit", "[native][interaction]")
{
    PluginProcessor p;
    p.insertPhraseStep (0, 0);
    NativeEditor editor (p);
    editor.setSize (1690, 980);
    auto* scale = findControl (editor, "Scale mode");
    REQUIRE (scale != nullptr);
    CHECK (scale->getY() == editor.grid.getY());
    CHECK (scale->getRight() == editor.grid.getRight() - 16);
    CHECK (scale->getBottom() <= editor.grid.getY() + editor.grid.stepBounds (0, 0).getY());
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
    CHECK (done->getY() > scale->getBottom());
    CHECK (root->getX() < scale->getX());
    CHECK (done->getRight() < scale->getRight());
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

TEST_CASE ("The piano roll follows the live timeline beyond its first window", "[native][schedule]")
{
    PluginProcessor processor;
    processor.setStandaloneTransportAvailableForTests (true);
    processor.setStandaloneTempoBpm (120);
    const int timing[] { 3, 1, 2, 3, 3 };
    for (int step = 0; step < 5; ++step)
    {
        processor.insertPhraseStep (0, step);
        processor.setPhraseStepTimingMultiplier (0, step, timing[step]);
        processor.setPhraseNote (0, step, 55 + step);
    }
    processor.insertPhraseStep (1, 0);
    processor.setPhraseRowMuted (1, false);
    processor.setPhraseRowTimingOffset (1, 10);
    processor.setPhraseStepTimingMultiplier (1, 0, 2);
    Controller controller (processor);
    Palette palette;
    palette.set ("dark");
    Preview preview (controller, palette);
    preview.setSize (900, 300);
    preview.refresh();
    const auto upcomingPixel = [&] {
        juce::Image image (juce::Image::RGB, preview.getWidth(), preview.getHeight(), true);
        juce::Graphics graphics (image);
        preview.paint (graphics);
        const double lane = 270.0 / (preview.highNote - preview.lowNote + 1);
        return image.getPixelAt (178, juce::roundToInt (30 + (preview.highNote - 56 + .5) * lane));
    };
    const auto forecastInk = upcomingPixel();
    REQUIRE (forecastInk != palette.app);

    constexpr int sampleRate = 8000, blockSize = 128;
    processor.prepareToPlay (sampleRate, blockSize);
    processor.setStandaloneTransportPlaying (true);
    juce::AudioBuffer<float> audio (2, blockSize);
    juce::MidiBuffer midi;
    int liveOns = 0;
    for (int start = 0; start < 34600; start += blockSize)
    {
        midi.clear();
        processor.processBlock (audio, midi);
        preview.updatePlayback();
        if (start == 0)
            CHECK (upcomingPixel() == forecastInk);
        for (const auto event : midi)
        {
            const auto message = event.getMessage();
            if (!message.isNoteOn())
                continue;
            ++liveOns;
            const double onset = (start + event.samplePosition) * 2.0 / sampleRate;
            CHECK (std::ranges::any_of (preview.playbackNotes(), [&](const auto& note) {
                return std::abs (note.start - onset) < 1.0e-8 && note.note == message.getNoteNumber()
                    && note.channel == message.getChannel() && note.velocity == message.getVelocity();
            }));
        }
    }
    CHECK (preview.playheadBeat() > 8.5);
    CHECK (std::ranges::any_of (preview.upcomingNotes(), [](const auto& note) {
        return std::abs (note.start - 9.5) < .001 && note.row == 0 && note.step == 1;
    }));
    CHECK (preview.playbackNotes().size() == static_cast<size_t> (liveOns));
    CHECK (std::ranges::any_of (preview.playbackNotes(), [](const auto& note) {
        return std::abs (note.start - 8.5) < 1.0e-8 && note.row == 0 && note.step == 0;
    }));
    CHECK (preview.stepPlayback()[0][0].sounding);
    processor.setStandaloneTransportPlaying (false);
    processor.processBlock (audio, midi);
    preview.updatePlayback();
    CHECK (preview.playheadBeat() < 0);
    CHECK_FALSE (preview.stepPlayback()[0][0].sounding);

    const auto later = renderPreview (processor, 40, 32);
    CHECK (std::ranges::any_of (later, [](const auto& note) {
        return std::abs (note.start - 34.0) < .001 && note.row == 0 && note.step == 0;
    }));
}

TEST_CASE ("The stopped piano roll honors a loop brace offset and clips its boundary", "[native][schedule]")
{
    PluginProcessor processor;
    processor.setPhraseRowMuted (0, false);
    for (int i = 0; i < 4; ++i)
    {
        processor.insertPhraseStep (0, i);
        processor.setPhraseNote (0, i, 60 + i);
    }
    processor.setLoopBraceEndQuarters (2.5);
    processor.setLoopBraceStartQuarters (.5);
    processor.setLoopBraceEnabled (true);
    const auto notes = renderPreview (processor, 8);
    REQUIRE (notes.size() == 2);
    CHECK (notes.front().start == Catch::Approx (1));
    CHECK (notes.back().start == Catch::Approx (2));
    CHECK (notes.back().end == Catch::Approx (2.5));
}

TEST_CASE ("Grid flashes follow emitted MIDI including unisons on independent channels", "[native][schedule]")
{
    for (const bool sameChannel : { false, true })
    {
        CAPTURE (sameChannel);
        PluginProcessor processor;
        processor.setStandaloneTransportAvailableForTests (true);
        processor.setStandaloneTempoBpm (120);
        for (int row = 0; row < 2; ++row)
        {
            processor.insertPhraseStep (row, 0);
            processor.setPhraseRowMuted (row, false);
            processor.setPhraseRowMidiChannel (row, sameChannel ? 1 : row + 1);
            processor.setPhraseNote (row, 0, 60);
        }
        processor.setPhraseRowTimingOffset (1, 10);
        Controller controller (processor);
        Palette palette;
        Preview preview (controller, palette);
        constexpr int sampleRate = 8000, blockSize = 128;
        processor.prepareToPlay (sampleRate, blockSize);
        processor.setStandaloneTransportPlaying (true);
        juce::AudioBuffer<float> audio (2, blockSize);
        juce::MidiBuffer midi;
        for (int start = 0; start < 2048; start += blockSize)
        {
            midi.clear();
            processor.processBlock (audio, midi);
            preview.updatePlayback();
        }
        CHECK (preview.stepPlayback()[0][0].sounding);
        CHECK (preview.stepPlayback()[1][0].sounding == !sameChannel);
        CHECK (preview.playbackNotes().size() == (sameChannel ? 1u : 2u));
    }
}

TEST_CASE ("Held unisons and loop cutoffs are independent of audio block boundaries", "[native][schedule]")
{
    for (const bool loop : { false, true })
    for (const int blockSize : { 32, 128, 512 })
    {
        CAPTURE (loop, blockSize);
        PluginProcessor processor;
        processor.setStandaloneTransportAvailableForTests (true);
        processor.setStandaloneTempoBpm (120);
        if (!loop)
        {
            processor.insertPhraseStep (0, 0);
            processor.setPhraseNote (0, 0, 60);
            processor.setPhraseStepDurationFraction (0, 0, .751);
            processor.insertPhraseStep (0, 1);
            processor.setPhraseNote (0, 1, 62);
            processor.insertPhraseStep (1, 0);
            processor.setPhraseRowMuted (1, false);
            processor.setPhraseRowMidiChannel (1, 1);
            processor.setPhraseNote (1, 0, 60);
            processor.setPhraseRowTimingOffset (1, 11);
            processor.setPhraseStepDurationFraction (1, 0, .5);
        }
        else
        {
            processor.insertPhraseStep (0, 0);
            processor.setPhraseNote (0, 0, 62);
            processor.insertPhraseStep (0, 1);
            processor.setPhraseNote (0, 1, 60);
            processor.setPhraseStepDurationFraction (0, 1, .51);
            processor.setLoopBraceEndQuarters (1.5);
            processor.setLoopBraceEnabled (true);
        }

        constexpr int sampleRate = 8000;
        processor.prepareToPlay (sampleRate, blockSize);
        processor.setStandaloneTransportPlaying (true);
        juce::AudioBuffer<float> audio (2, blockSize);
        juce::MidiBuffer midi;
        int noteOns = 0;
        double firstOff = -1;
        for (int start = 0; start < 6400 && firstOff < 0; start += blockSize)
        {
            midi.clear();
            processor.processBlock (audio, midi);
            for (const auto event : midi)
            {
                const auto msg = event.getMessage();
                if (!msg.isNoteOnOrOff() || msg.getNoteNumber() != 60)
                    continue;
                if (msg.isNoteOff())
                {
                    firstOff = (start + event.samplePosition) * 2.0 / sampleRate;
                    break;
                }
                if (msg.isNoteOn())
                    ++noteOns;
            }
        }
        CHECK (noteOns == 1);
        CHECK (firstOff == Catch::Approx (loop ? 1.5 : 1.25).margin (2.0 / sampleRate));
    }
}

TEST_CASE ("Forecast and DAW MIDI agree with swing, cycles, probability and humanization", "[native][schedule]")
{
    for (const int blockSize : { 64, 333, 512 })
    {
        CAPTURE (blockSize);
        PluginProcessor processor;
        processor.setSwingPercent (67);
        processor.setSwingSubdivisionIndex (1);
        processor.setVelocityHumanizePercent (30);
        processor.setTimingHumanizePercent (70);
        for (int step = 0; step < 3; ++step)
        {
            processor.insertPhraseStep (0, step);
            processor.setPhraseNote (0, step, 60 + step);
            processor.setPhraseStepTimingMultiplier (0, step, step + 1);
            processor.setPhraseStepDurationFraction (0, step, .65);
        }
        processor.setPhraseStepCycle (0, 0, 3);
        processor.setPhraseStepCycleOffset (0, 0, 5);
        processor.setPhraseStepProbability (0, 1, 70);
        const auto forecast = renderPreview (processor, 8);
        REQUIRE_FALSE (forecast.empty());
        struct Clock : juce::AudioPlayHead
        {
            PositionInfo info;
            juce::Optional<PositionInfo> getPosition() const override { return info; }
        } clock;
        clock.info.setBpm (120);
        clock.info.setIsPlaying (true);
        processor.setPlayHead (&clock);
        constexpr int sampleRate = 48000;
        processor.prepareToPlay (sampleRate, blockSize);
        juce::AudioBuffer<float> audio (2, blockSize);
        juce::MidiBuffer midi;
        size_t noteOns = 0, noteOffs = 0;
        for (int start = 0; start < sampleRate * 4; start += blockSize)
        {
            midi.clear();
            clock.info.setPpqPosition (start * 2.0 / sampleRate);
            processor.processBlock (audio, midi);
            for (const auto event : midi)
            {
                const auto msg = event.getMessage();
                const double beat = (start + event.samplePosition) * 2.0 / sampleRate;
                if (!msg.isNoteOnOrOff() || beat >= 8)
                    continue;
                if (msg.isNoteOn())
                {
                    ++noteOns;
                    CHECK (std::ranges::any_of (forecast, [&](const auto& note) {
                        return note.note == msg.getNoteNumber() && note.velocity == msg.getVelocity()
                            && std::abs (note.start - beat) < .001;
                    }));
                }
                else
                {
                    ++noteOffs;
                    CHECK (std::ranges::any_of (forecast, [&](const auto& note) {
                        return note.note == msg.getNoteNumber() && std::abs (note.end - beat) < .001;
                    }));
                }
            }
        }
        CHECK (noteOns == forecast.size());
        CHECK (noteOffs == static_cast<size_t> (std::ranges::count_if (forecast, [](const auto& note) { return note.end < 8; })));
        processor.setPlayHead (nullptr);
    }
}

TEST_CASE ("Step actions use the accent of a single selected row", "[native][interaction]")
{
    PluginProcessor processor;
    processor.setRowColorsEnabled (true);
    processor.insertPhraseStep (0, 0);
    processor.insertPhraseStep (1, 0);
    NativeEditor editor (processor);
    auto* input = findControl (editor, "Bulk Pitch");
    auto* action = findControl (editor, "Skip selected steps");
    REQUIRE (input != nullptr);
    REQUIRE (action != nullptr);
    REQUIRE (input->accentRow);
    REQUIRE (action->accentRow);

    editor.controller.selection[0].set (0);
    editor.synchronise();
    CHECK (input->accentRow() == 0);
    editor.controller.selection[0].reset();
    editor.controller.selection[1].set (0);
    editor.synchronise();
    CHECK (input->accentRow() == 1);
    CHECK (input->accentWhenIdle);
    CHECK (action->accentRow() == 1);

    editor.controller.selection[0].set (0);
    editor.synchronise();
    CHECK (input->accentRow() == -1);
    CHECK_FALSE (input->accentWhenIdle);
    CHECK (action->accentRow() == -1);
}

TEST_CASE ("The footer row count expands the visible rows without editing the pattern", "[native][interaction]")
{
    PluginProcessor processor;
    for (int row = 0; row < 4; ++row)
        processor.insertPhraseStep (row, 0);
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    editor.grid.compact = false;
    editor.grid.updateLayout();
    auto* count = findControl (editor, "Visible phrase rows");
    auto* drum = findControl (editor, "Drum row pitch lock");
    auto* modeSwitch = findControl (editor, "Mode switch timing");
    REQUIRE (modeSwitch != nullptr);
    REQUIRE (count != nullptr);
    REQUIRE (drum != nullptr);
    CHECK (modeSwitch->getY() == drum->getY());
    CHECK (modeSwitch->getRight() < drum->getX());
    CHECK (modeSwitch->display().endsWith ("Every pulse"));
    CHECK (processor.isCombinationChangePhaseContinue());
    CHECK (drum->getY() == count->getY());
    CHECK (drum->getRight() < count->getX());
    const auto first = editor.grid.stepBounds (0, 0);
    const auto last = editor.grid.stepBounds (3, 0);
    const auto before = readRows (processor);
    editor.showInspector (3, 0);
    CHECK (drum->isVisible());
    CHECK (modeSwitch->isVisible());
    editor.grid.selectVisibleSteps();

    const auto drag = [&] (float distance) {
        const auto start = count->getLocalBounds().getCentre().toFloat();
        const auto end = start.translated (0, distance);
        count->mouseDown (pointerEvent (*count, start, start));
        count->mouseDrag (pointerEvent (*count, end, start, true));
        count->mouseUp (pointerEvent (*count, end, start, true));
    };
    drag (24);
    CHECK (editor.grid.getVisibleRowCount() == 2);
    CHECK (editor.grid.stepBounds (0, 0).getY() == Catch::Approx (first.getY()));
    CHECK (editor.grid.stepBounds (0, 0).getHeight() == Catch::Approx (editor.grid.stepBounds (1, 0).getHeight()));
    CHECK (editor.grid.stepBounds (1, 0).getBottom() == Catch::Approx (last.getBottom()));
    CHECK (editor.grid.stepBounds (2, 0).isEmpty());
    CHECK (editor.grid.stepBounds (3, 0).isEmpty());
    CHECK (editor.preview.isVisible());
    editor.grid.selectVisibleSteps();
    CHECK (editor.controller.selection[0][0]);
    CHECK_FALSE (editor.controller.selection[3][0]);

    drag (36);
    CHECK (editor.grid.getVisibleRowCount() == 1);
    CHECK (editor.grid.stepBounds (0, 0).getBottom() == Catch::Approx (last.getBottom()));
    drag (-60);
    CHECK (editor.grid.getVisibleRowCount() == 4);
    CHECK (editor.grid.stepBounds (0, 0) == first);
    CHECK (editor.grid.stepBounds (3, 0) == last);
    CHECK (readRows (processor) == before);
    CHECK_FALSE (editor.controller.canUndo());
}

TEST_CASE ("The processing strip appears only with the piano roll", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    auto* mode = findControl (editor, "Cross-Mod");
    auto* octave = findControl (editor, "Up octave mix");
    auto* filter = findControl (editor, "Bandpass range");
    auto* fit = findControl (editor, "Fit notes");
    auto* close = findControl (editor, "Close inspector");
    REQUIRE (mode != nullptr);
    REQUIRE (octave != nullptr);
    REQUIRE (filter != nullptr);
    REQUIRE (fit != nullptr);
    REQUIRE (close != nullptr);
    const auto previewBounds = editor.preview.getBounds();
    const int stripTop = editor.grid.getBottom();
    const auto expectPianoRoll = [&] {
        CHECK (editor.preview.isVisible());
        CHECK (editor.preview.getBounds() == previewBounds);
        CHECK (mode->isVisible());
        CHECK (octave->isVisible());
        CHECK (filter->isVisible());
        CHECK (fit->isVisible());
    };
    const auto expectExpandedPanel = [&] {
        CHECK_FALSE (editor.preview.isVisible());
        CHECK_FALSE (mode->isVisible());
        CHECK_FALSE (octave->isVisible());
        CHECK_FALSE (filter->isVisible());
        CHECK_FALSE (fit->isVisible());
        for (auto* child : editor.getChildren())
            if (auto* keyboard = dynamic_cast<Keyboard*> (child))
            {
                CHECK (keyboard->isVisible());
                if (close->isVisible())
                {
                    CHECK (keyboard->getY() > stripTop + 104);
                    CHECK (keyboard->getX() > 270);
                    CHECK (keyboard->getRight() < editor.getWidth() - 24);
                    CHECK (keyboard->getBottom() < editor.getHeight() - 40);
                }
                else
                    CHECK (keyboard->getY() == stripTop + 98);
                return;
            }
        FAIL ("Missing panel keyboard");
    };

    expectPianoRoll();
    const auto clickGrid = [&] (juce::Point<float> position) {
        editor.grid.mouseDown (pointerEvent (editor.grid, position, position));
        editor.grid.mouseUp (pointerEvent (editor.grid, position, position));
    };
    clickGrid (editor.grid.stepBounds (0, 0).getCentre());
    expectExpandedPanel();
    CHECK (close->isVisible());
    CHECK (close->getY() == stripTop + 8);
    clickGrid (editor.grid.stepBounds (0, 0).getCentre());
    CHECK_FALSE (close->isVisible());
    clickGrid ({ 200, 10 });
    expectPianoRoll();

    processor.setPhraseRowRecording (0, 16);
    editor.resized();
    expectExpandedPanel();
    CHECK_FALSE (close->isVisible());
    processor.setPhraseRowRecording (-1);
    editor.resized();
    expectPianoRoll();
}

TEST_CASE ("The step cycle extends, moves a beat on release, and resizes as one edit", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    editor.showInspector (0, 0);
    auto* cycle = [&]() -> CycleEditor* {
        for (auto* child : editor.getChildren())
            if (auto* found = dynamic_cast<CycleEditor*> (child))
                return found;
        return nullptr;
    }();
    REQUIRE (cycle != nullptr);
    REQUIRE (cycle->isVisible());
    const juce::Point<float> first { 29, 49 }, third { 121, 49 };
    cycle->mouseDown (pointerEvent (*cycle, third, third));
    cycle->mouseUp (pointerEvent (*cycle, third, third));
    CHECK (processor.getPhraseStepCycle (0, 0) == 3);
    CHECK (processor.getPhraseStepCycleOffset (0, 0) == 1);

    cycle->mouseDown (pointerEvent (*cycle, first, first));
    cycle->mouseDrag (pointerEvent (*cycle, third, first, true));
    CHECK (processor.getPhraseStepCycleOffset (0, 0) == 1);
    cycle->mouseUp (pointerEvent (*cycle, third, first, true));
    CHECK (processor.getPhraseStepCycleOffset (0, 0) == 4);

    const juce::Point<float> handle { 143, 49 }, fifthEdge { 233, 49 };
    cycle->mouseDown (pointerEvent (*cycle, handle, handle));
    cycle->mouseDrag (pointerEvent (*cycle, fifthEdge, handle, true));
    CHECK (processor.getPhraseStepCycle (0, 0) == 5);
    cycle->mouseUp (pointerEvent (*cycle, fifthEdge, handle, true));
    REQUIRE (editor.controller.undo());
    CHECK (processor.getPhraseStepCycle (0, 0) == 3);
    CHECK (processor.getPhraseStepCycleOffset (0, 0) == 4);
}

TEST_CASE ("Clicking the inspected step again closes the inspector and restores compact view", "[native][interaction]")
{
    PluginProcessor processor;
    processor.setProjectMetadata ("", "", "", "", "dark", 100, true);
    processor.insertPhraseStep (0, 0);
    NativeEditor editor (processor);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    auto& grid = editor.grid;
    REQUIRE (editor.preview.isVisible());
    const auto point = grid.stepBounds (0, 0).getCentre();
    grid.mouseDown (pointerEvent (grid, point, point));
    grid.mouseUp (pointerEvent (grid, point, point));
    REQUIRE (grid.inspectedStep == 0);
    CHECK_FALSE (editor.preview.isVisible());
    CHECK_FALSE (grid.compact);

    grid.mouseDown (pointerEvent (grid, point, point));
    grid.mouseUp (pointerEvent (grid, point, point));
    CHECK (grid.inspectedStep == -1);
    CHECK (grid.compact);
    CHECK (editor.preview.isVisible());
    CHECK (editor.controller.selection[0][0]);
}

TEST_CASE ("The group delete button removes selected steps in one undoable edit", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    processor.insertPhraseStep (0, 1);
    processor.insertPhraseStep (1, 0);
    NativeEditor editor (processor);
    auto* remove = findControl (editor, "Delete selected steps");
    REQUIRE (remove != nullptr);
    CHECK_FALSE (remove->isEnabled());

    editor.controller.selection[0].set (1);
    editor.controller.selection[1].set (0);
    editor.synchronise();
    REQUIRE (remove->isEnabled());
    const auto point = remove->getLocalBounds().getCentre().toFloat();
    remove->mouseDown (pointerEvent (*remove, point, point));
    remove->mouseUp (pointerEvent (*remove, point, point));
    CHECK (processor.getPhraseRowStepCount (0) == 1);
    CHECK (processor.getPhraseRowStepCount (1) == 0);
    CHECK_FALSE (remove->isEnabled());
    REQUIRE (editor.controller.undo());
    CHECK (processor.getPhraseRowStepCount (0) == 2);
    CHECK (processor.getPhraseRowStepCount (1) == 1);
}

TEST_CASE ("Clicking another step retargets the open inspector and clears the previous selection", "[native][interaction]")
{
    PluginProcessor p;
    p.setProjectMetadata ("", "", "", "", "dark", 100, false);
    p.insertPhraseStep (0, 0);
    p.insertPhraseStep (1, 0);
    p.setPhraseNote (1, 0, 62);
    NativeEditor editor (p);
    editor.setSize (PluginEditor::designWidth, PluginEditor::designHeight);
    editor.showInspector (0, 0);
    editor.controller.selectAll();
    const auto target = editor.grid.stepBounds (1, 0).getCentre();
    const auto click = [] (juce::Component& component, juce::Point<float> point) {
        const auto event = pointerEvent (component, point, point);
        component.mouseDown (event);
        component.mouseUp (event);
    };
    click (editor.grid, target);
    CHECK (editor.grid.inspectedRow == 1);
    CHECK (editor.grid.inspectedStep == 0);
    CHECK_FALSE (editor.controller.selection[0][0]);
    CHECK (editor.controller.selection[1][0]);

    auto* note = findControl (editor, "Step note");
    REQUIRE (note != nullptr);
    CHECK (note->isVisible());
    CHECK (note->display() == "D3");
    auto* skip = findControl (editor, "Skip inspected step");
    auto* mute = findControl (editor, "Mute inspected step");
    auto* remove = findControl (editor, "Delete inspected steps");
    auto* velocity = findControl (editor, "Step velocity");
    REQUIRE (skip != nullptr);
    REQUIRE (mute != nullptr);
    REQUIRE (remove != nullptr);
    REQUIRE (velocity != nullptr);
    CHECK (remove->getWidth() == 226);
    CHECK (remove->getBottom() < skip->getY());
    CHECK (skip->getBottom() < mute->getY());
    CHECK (note->getRight() < velocity->getX());
    CHECK (findControl (editor, "Step duration") == nullptr);
    CHECK (findControl (editor, "Step multiplier") == nullptr);
    click (*skip, skip->getLocalBounds().getCentre().toFloat());
    CHECK_FALSE (p.isPhraseStepSkipped (0, 0));
    CHECK (p.isPhraseStepSkipped (1, 0));
    click (*remove, remove->getLocalBounds().getCentre().toFloat());
    CHECK (p.getPhraseRowStepCount (1) == 0);
    CHECK (editor.preview.isVisible());
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
    CHECK (editor.grid.inspectedStep == 0);
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
    auto* source = findControl (editor, "Select slot 1");
    auto* destination = findControl (editor, "Select slot 2");
    REQUIRE (source != nullptr);
    REQUIRE (destination != nullptr);
    const int shift = juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier;
    source->mouseDown (pointerEvent (*source, { 4, 4 }, { 4, 4 }, false, shift));
    source->mouseUp (pointerEvent (*source, { 4, 4 }, { 4, 4 }, false, shift));
    REQUIRE (source->copySource);
    CHECK (source->copySource());
    REQUIRE (destination->copyTarget);
    CHECK (destination->copyTarget());
    auto* clear = findControl (editor, "Clear selected slot");
    REQUIRE (clear != nullptr);
    clear->mouseDown (pointerEvent (*clear, { 4, 4 }, { 4, 4 }));
    clear->mouseUp (pointerEvent (*clear, { 4, 4 }, { 4, 4 }));
    CHECK_FALSE (source->copySource());
}

TEST_CASE ("Playback display advances and stops with and without the loop brace", "[native][schedule]")
{
    for (const bool loop : { false, true })
    {
        INFO ("loop=" << loop);
        PluginProcessor processor;
        processor.setStandaloneTransportAvailableForTests (true);
        processor.setCurrentPatternSlot (0);
        processor.insertPhraseStep (0, 0);
        processor.setLoopBraceStartQuarters (0);
        processor.setLoopBraceEndQuarters (1);
        processor.setLoopBraceEnabled (loop);
        processor.prepareToPlay (8000, 128);
        processor.setStandaloneTransportPlaying (true);
        juce::AudioBuffer<float> audio (2, 128);
        juce::MidiBuffer midi;
        for (int block = 0; block < 80; ++block)
        {
            midi.clear();
            processor.processBlock (audio, midi);
            const double elapsed = (block + 1) * 128.0 * 2.0 / 8000.0;
            CHECK (processor.getPlaybackBeat() == Catch::Approx (loop ? std::fmod (elapsed, 1.0) : elapsed).margin (1.e-7));
        }
        processor.setStandaloneTransportPlaying (false);
        processor.processBlock (audio, midi);
        CHECK (processor.getPlaybackBeat() < 0);
        processor.releaseResources();
    }
}

TEST_CASE ("Shift-click toggles note bodies in both grid views", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    processor.insertPhraseStep (1, 0);
    Controller controller (processor);
    Palette palette;
    Grid grid (controller, palette);
    for (const bool compact : { false, true })
    {
        CAPTURE (compact);
        grid.compact = compact;
        grid.setSize (900, 500);
        grid.updateLayout();
        controller.selection[0].set (0);
        const auto point = grid.stepBounds (1, 0).withHeight (36).getCentre();
        const int shift = juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier;
        const auto event = pointerEvent (grid, point, point, false, shift);
        grid.mouseDown (event);
        grid.mouseUp (event);
        CHECK (controller.selection[0][0]);
        CHECK (controller.selection[1][0]);
        grid.mouseDown (event);
        grid.mouseUp (event);
        CHECK (controller.selection[0][0]);
        CHECK_FALSE (controller.selection[1][0]);
        const int command = juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::commandModifier;
        const auto commandClick = pointerEvent (grid, point, point, false, command);
        grid.mouseDown (commandClick);
        grid.mouseUp (commandClick);
        CHECK_FALSE (controller.selection[0][0]);
        CHECK (controller.selection[1][0]);
        controller.clearSelection();
    }
}

TEST_CASE ("Compact pitch drags anchor the grabbed note while the row adjusts", "[native][interaction]")
{
    PluginProcessor processor;
    processor.setPatternScale (0, 0);
    for (int step = 0; step < 3; ++step)
        processor.insertPhraseStep (0, step);
    processor.setPhraseNote (0, 0, 60);
    processor.setPhraseNote (0, 1, 64);
    processor.setPhraseNote (0, 2, 67);
    Controller controller (processor);
    Palette palette;
    Grid grid (controller, palette);
    grid.compact = true;
    grid.setSize (900, 500);
    const auto start = grid.stepBounds (0, 1).getCentre();
    const auto top = grid.stepBounds (0, 2).getY();
    const auto bottom = grid.stepBounds (0, 0).getBottom();
    grid.mouseDown (pointerEvent (grid, start, start));

    grid.mouseDrag (pointerEvent (grid, start.translated (0, -10), start, true));
    const float neighborBefore = grid.stepBounds (0, 2).getY();
    CHECK (processor.getPhraseNote (0, 1) == 65);
    grid.mouseDrag (pointerEvent (grid, start.translated (0, -11), start, true));
    const float neighborBetween = grid.stepBounds (0, 2).getY();
    CHECK (processor.getPhraseNote (0, 1) == 65);
    CHECK (std::abs (neighborBetween - neighborBefore) > .01f);
    grid.mouseDrag (pointerEvent (grid, start.translated (0, -12), start, true));
    CHECK (processor.getPhraseNote (0, 1) == 66);
    CHECK (std::abs (grid.stepBounds (0, 2).getY() - neighborBetween) < 1.0f);

    const auto higher = start.translated (0, -16);
    grid.mouseDrag (pointerEvent (grid, higher, start, true));
    CHECK (grid.stepBounds (0, 1).getCentreY() == Catch::Approx (higher.y));
    CHECK (processor.getPhraseNote (0, 1) == 66);
    CHECK (grid.stepBounds (0, 2).getY() < grid.stepBounds (0, 1).getY());
    CHECK (grid.stepBounds (0, 2).getY() >= top - .01f);
    CHECK (grid.stepBounds (0, 0).getBottom() <= bottom + .01f);

    const auto outside = start.translated (80, -48);
    grid.mouseDrag (pointerEvent (grid, outside, start, true));
    CHECK (grid.stepBounds (0, 1).getY() == Catch::Approx (top));
    CHECK (processor.getPhraseNote (0, 1) == 70);
    grid.mouseUp (pointerEvent (grid, outside, start, true));
    CHECK (processor.getPhraseNote (0, 0) == 60);
    CHECK (processor.getPhraseNote (0, 2) == 67);
    REQUIRE (controller.undo());
    CHECK (processor.getPhraseNote (0, 1) == 64);
}

TEST_CASE ("Shift drag on a detailed pitch changes octaves without deselecting the step", "[native][interaction]")
{
    PluginProcessor processor;
    processor.insertPhraseStep (0, 0);
    Controller controller (processor);
    controller.selection[0].set (0);
    Palette palette;
    Grid grid (controller, palette);
    grid.setSize (900, 500);
    const auto start = grid.stepBounds (0, 0).withHeight (36).getCentre();
    const auto end = start.translated (0, -24);
    const int shift = juce::ModifierKeys::leftButtonModifier | juce::ModifierKeys::shiftModifier;
    const int original = processor.getPhraseNote (0, 0);
    grid.mouseDown (pointerEvent (grid, start, start, false, shift));
    grid.mouseDrag (pointerEvent (grid, end, start, true, shift));
    grid.mouseUp (pointerEvent (grid, end, start, true, shift));
    CHECK (processor.getPhraseNote (0, 0) == original + 12);
    CHECK (controller.selection[0][0]);
    REQUIRE (controller.undo());
    CHECK (processor.getPhraseNote (0, 0) == original);
}
