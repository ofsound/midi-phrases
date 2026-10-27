#include "PhrasesNativeEditor.h"
namespace phrases::ui
{
    namespace
    {
        juce::String number (double value)
        {
            if (std::abs (value - std::round (value)) < 1.0e-8)
                return juce::String (static_cast<int> (std::round (value)));
            auto result = juce::String (value, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
            return result.startsWith ("0.") ? result.substring (1) : result;
        }
        bool blackKey (int note)
        {
            const int n = note % 12;
            return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
        }
        int whiteKeys (int first, int last)
        {
            int count = 0;
            for (int note = first; note <= last; ++note)
                if (!blackKey (note))
                    ++count;
            return count;
        }
        int whiteIndex (int midi)
        {
            int index = 0;
            for (int note = 0; note < midi; ++note)
                if (!blackKey (note))
                    ++index;
            return index;
        }
    }
    void Keyboard::layout()
    {
        const float width = static_cast<float> (getWidth());
        const float height = static_cast<float> (getHeight());
        if (width <= 1.0f || height <= 1.0f)
            return;
        if (scrolling)
        {
            keysTop = 52.0f;
            keysH = std::max (1.0f, height - keysTop);
            keyWidth = keysH * (23.0f / 150.0f);
            octaveDown = { 0, 2, 40, 40 };
            octaveUp = { width - 40.0f, 2, 40, 40 };
            if (centerNote >= 0)
            {
                constexpr int windowSemitones = 36;
                const int maxOffset = std::max (0, (127 - windowSemitones + 1) / 12);
                const int offset = juce::jlimit (0, maxOffset, juce::roundToInt ((centerNote - (windowSemitones - 1) / 2.0) / 12.0));
                scrollLeft = static_cast<float> (whiteIndex (offset * 12)) * keyWidth;
                centerNote = -1;
            }
            const float content = static_cast<float> (whiteKeys (0, 127)) * keyWidth;
            scrollLeft = juce::jlimit (0.0f, std::max (0.0f, content - width), scrollLeft);
        }
        else
        {
            keysTop = 0;
            keysH = height;
            keyWidth = width / static_cast<float> (std::max (1, whiteKeys (low, high)));
            scrollLeft = 0;
        }
    }
    void Keyboard::buildKeys()
    {
        layout();
        keys.clear();
        const int first = scrolling ? 0 : low;
        const int last = scrolling ? 127 : high;
        int index = 0;
        for (int note = first; note <= last; ++note)
        {
            if (blackKey (note))
                continue;
            const float x = static_cast<float> (index++) * keyWidth - scrollLeft;
            keys.push_back ({ { x, keysTop, scrolling ? keyWidth : keyWidth - 1.0f, keysH }, note, false });
        }
        index = 0;
        for (int note = first; note <= last; ++note)
        {
            if (!blackKey (note))
            {
                ++index;
                continue;
            }
            const float widthRatio = scrolling ? 0.52f : 0.6f;
            const float blackWidth = keyWidth * widthRatio;
            const float center = static_cast<float> (index) * keyWidth - scrollLeft;
            keys.push_back ({ { center - blackWidth * 0.5f, keysTop, blackWidth, keysH * (scrolling ? 0.58f : 0.62f) }, note, true });
        }
    }
    void Keyboard::resized() { layout(); }
    void Keyboard::centerOn (int note)
    {
        centerNote = juce::jlimit (0, 127, note);
        layout();
        repaint();
    }
    bool Keyboard::accepts (int note) const
    {
        if (!scrolling || showScale || scaleMode == 0)
            return true;
        return snapNote (note, scaleRoot, scaleMode) == note;
    }
    Rect Keyboard::boundsFor (int note)
    {
        buildKeys();
        for (const auto& key : keys)
            if (key.note == note)
                return key.bounds;
        return {};
    }
    void Keyboard::scrollBy (float delta)
    {
        scrollLeft += delta;
        layout();
        repaint();
    }
    void Keyboard::paint (juce::Graphics& g)
    {
        buildKeys();
        const auto accent = rowAccent >= 0 ? (scrolling ? p.rowStrong[static_cast<size_t> (rowAccent)] : p.rows[static_cast<size_t> (rowAccent)]) : (scrolling ? p.strong : p.accent);
        if (scrolling)
        {
            auto paintOctave = [&] (Rect bounds, const juce::String& caption, bool hot) {
                face (g, bounds, p, false, 6);
                if (hot)
                {
                    g.setColour (p.borderStrong);
                    g.drawRoundedRectangle (bounds.reduced (0.5f), 6, 1.4f);
                }
                text (g, caption, bounds, hot ? p.text : p.secondary, 16, true);
            };
            paintOctave (octaveDown, juce::String::fromUTF8 ("−"), hoverDown);
            paintOctave (octaveUp, "+", hoverUp);
        }
        juce::Graphics::ScopedSaveState clip (g);
        if (scrolling)
            g.reduceClipRegion (juce::Rectangle<int> (0, juce::roundToInt (keysTop), getWidth(), juce::roundToInt (keysH)));
        for (const auto& key : keys)
        {
            if (key.black)
                continue;
            const bool chosen = key.note == selected || key.note == held;
            const bool enabled = accepts (key.note);
            juce::Colour top = colour (0xffffff);
            juce::Colour bottom = colour (0xececec);
            if (!scrolling)
            {
                top = chosen ? accent : colour (0xd4d4d8);
                bottom = chosen ? accent.darker (0.15f) : colour (0xf4f4f5);
            }
            else if (!enabled)
            {
                top = top.interpolatedWith (p.app, 0.65f);
                bottom = bottom.interpolatedWith (p.app, 0.65f);
            }
            else if (chosen)
            {
                top = bottom = accent;
            }
            else if (key.note == hoverNote)
            {
                top = top.interpolatedWith (juce::Colours::black, 0.02f);
                bottom = bottom.interpolatedWith (juce::Colours::black, 0.04f);
            }
            if (scrolling && chosen && key.note == held)
            {
                top = top.darker (0.05f);
                bottom = bottom.darker (0.08f);
            }
            g.setGradientFill (juce::ColourGradient (top, key.bounds.getTopLeft(), bottom, key.bounds.getBottomLeft(), false));
            if (scrolling)
                g.fillRect (key.bounds);
            else
                g.fillRoundedRectangle (key.bounds, 3);
            if (scrolling && chosen)
            {
                g.setColour (accent.withAlpha (0.45f));
                g.drawRect (key.bounds.expanded (1.5f), 2.0f);
            }
            g.setColour (scrolling ? juce::Colours::black.withAlpha (enabled ? 0.18f : 0.08f) : p.border);
            if (scrolling)
            {
                g.drawLine (key.bounds.getRight() - 0.5f, key.bounds.getY(), key.bounds.getRight() - 0.5f, key.bounds.getBottom());
                g.drawLine (key.bounds.getX(), key.bounds.getBottom() - 0.5f, key.bounds.getRight(), key.bounds.getBottom() - 0.5f);
            }
            else
                g.drawRoundedRectangle (key.bounds, 3, 1);
            if (scrolling && key.note % 12 == 0)
                text (g, noteName (key.note), key.bounds.withY (key.bounds.getBottom() - 18).withHeight (14), p.inverse, 11, Weight::bold);
            else if (!scrolling && key.note % 12 == 0)
                text (g, noteName (key.note), key.bounds.withY (key.bounds.getBottom() - 24).withHeight (20), colour (0x52525b), 11);
            if (!scrolling && showScale && snapNote (key.note, scaleRoot, scaleMode) == key.note)
            {
                g.setColour (p.accent);
                g.fillEllipse (key.bounds.getCentreX() - 4, key.bounds.getBottom() - 13, 8, 8);
            }
        }
        if (scrolling)
            for (const auto& key : keys)
                if (key.black)
                {
                    g.setColour (colour (0xffffff));
                    g.fillRect (key.bounds.getCentreX() - 1.0f, keysTop, 2.0f, keysH);
                }
        for (auto key = keys.rbegin(); key != keys.rend(); ++key)
        {
            if (!key->black)
                continue;
            const bool chosen = key->note == selected || key->note == held;
            const bool enabled = accepts (key->note);
            juce::Colour top = colour (0x181818);
            juce::Colour bottom = colour (0x0c0c0c);
            if (!scrolling)
            {
                top = chosen ? accent.darker (0.15f) : colour (0x18181b);
                bottom = chosen ? accent : colour (0x3f3f46);
            }
            else if (!enabled)
            {
                top = top.interpolatedWith (juce::Colours::white, 0.65f);
                bottom = bottom.interpolatedWith (juce::Colours::white, 0.65f);
            }
            else if (chosen)
            {
                top = bottom = accent;
            }
            else if (key->note == hoverNote)
            {
                top = top.brighter (0.10f);
                bottom = bottom.brighter (0.10f);
            }
            if (scrolling && chosen && key->note == held)
            {
                top = top.brighter (0.25f);
                bottom = bottom.brighter (0.25f);
            }
            g.setGradientFill (juce::ColourGradient (top, key->bounds.getTopLeft(), bottom, key->bounds.getBottomLeft(), false));
            g.fillRoundedRectangle (key->bounds, scrolling ? 4.0f : 2.0f);
            g.setColour (scrolling ? juce::Colours::black.withAlpha (enabled ? 0.50f : 0.25f) : colour (0x09090b));
            g.drawRoundedRectangle (key->bounds, scrolling ? 4.0f : 2.0f, 1);
            if (scrolling && chosen)
            {
                g.setColour (accent.withAlpha (0.55f));
                g.drawRoundedRectangle (key->bounds.expanded (1.5f), 5.0f, 2.0f);
            }
            if (!scrolling && showScale && snapNote (key->note, scaleRoot, scaleMode) == key->note)
            {
                g.setColour (p.accent);
                g.fillEllipse (key->bounds.getCentreX() - 4, key->bounds.getBottom() - 13, 8, 8);
            }
        }
    }
    void Keyboard::press (juce::Point<float> position)
    {
        buildKeys();
        for (auto key = keys.rbegin(); key != keys.rend(); ++key)
            if (key->bounds.contains (position))
            {
                if (!accepts (key->note))
                    return;
                if (held == key->note)
                    return;
                release();
                held = key->note;
                if (noteOn)
                    noteOn (held);
                repaint();
                return;
            }
        release();
    }
    void Keyboard::release()
    {
        if (held >= 0 && noteOff)
            noteOff (held);
        held = -1;
        repaint();
    }
    void Keyboard::mouseMove (const juce::MouseEvent& e)
    {
        buildKeys();
        const bool down = scrolling && octaveDown.contains (e.position);
        const bool up = scrolling && octaveUp.contains (e.position);
        int next = -1;
        if (!down && !up)
            for (auto key = keys.rbegin(); key != keys.rend(); ++key)
                if (key->bounds.contains (e.position) && accepts (key->note))
                {
                    next = key->note;
                    break;
                }
        if (next != hoverNote || down != hoverDown || up != hoverUp)
        {
            hoverNote = next;
            hoverDown = down;
            hoverUp = up;
            repaint();
        }
        setMouseCursor (next >= 0 || down || up ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }
    void Keyboard::mouseExit (const juce::MouseEvent&)
    {
        hoverNote = -1;
        hoverDown = hoverUp = false;
        repaint();
    }
    void Keyboard::mouseDown (const juce::MouseEvent& e)
    {
        if (scrolling && octaveDown.contains (e.position))
        {
            octaveHold = true;
            scrollBy (-7.0f * keyWidth);
            return;
        }
        if (scrolling && octaveUp.contains (e.position))
        {
            octaveHold = true;
            scrollBy (7.0f * keyWidth);
            return;
        }
        octaveHold = false;
        press (e.position);
    }
    void Keyboard::mouseDrag (const juce::MouseEvent& e)
    {
        if (!octaveHold)
            press (e.position);
    }
    void Keyboard::mouseUp (const juce::MouseEvent&)
    {
        octaveHold = false;
        release();
    }

    void Keyboard::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
    {
        if (showScale)
            return;
        if (scrolling)
        {
            const float delta = std::abs (wheel.deltaX) > std::abs (wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
            scrollBy (-delta * std::max (48.0f, keyWidth * 5.0f));
            return;
        }
        const int span = high - low;
        low = juce::jlimit (0, 127 - span, low + ((wheel.deltaX + wheel.deltaY) > 0 ? 12 : -12));
        high = low + span;
        repaint();
    }
    NativeEditor::NativeEditor (PluginProcessor& processor) : controller (processor), grid (controller, palette), preview (controller, palette)
    {
        heldKeys.fill (-1);
        setWantsKeyboardFocus (true);
        setOpaque (true);
        theme = processor.getProjectThemeMode();
        palette.set (theme);
        grid.compact = processor.getProjectStretchStepsToFit();
        addAndMakeVisible (grid);
        addAndMakeVisible (preview);
        addChildComponent (keyboard);
        grid.addKeyListener (this);
        preview.addKeyListener (this);
        keyboard.addKeyListener (this);
        grid.inspect = [this] (int r, int s) { showInspector (r, s); };
        grid.editRow = [this] (int r) { showRowEditor (r); };
        grid.seedRow = [this] (int r) { showSeed (r); };
        grid.recordRow = [this] (int r) { record (r); };
        grid.changed = [this] { edited(); };
        preview.changed = [this] { edited(); };
        preview.inspect = grid.inspect;
        keyboard.noteOn = [this] (int note) {
            if (controller.p.getPhraseRowRecording() >= 0)
                controller.p.capturePhraseRowRecordedNoteOn (note, 100);
            else if (inspectorRow >= 0)
            {
                controller.begin();
                editStep ([note] (Step& s) { s.note = note; });
                controller.commit();
                edited();
            }
            else
                controller.p.playStandaloneKeyboardNoteOn (note, 100);
        };
        keyboard.noteOff = [this] (int note) {
            if (controller.p.getPhraseRowRecording() >= 0)
                controller.p.capturePhraseRowRecordedNoteOff (note);
            else
                controller.p.playStandaloneKeyboardNoteOff (note);
        };
        for (auto* label : { &projectName, &projectDescription })
        {
            addAndMakeVisible (*label);
            label->setEditable (true);
            label->setFont (font (14, label == &projectName));
            label->onTextChange = [this] { updateMetadata(); };
            label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        }
        projectName.setTitle ("Project name");
        projectDescription.setTitle ("Project description");
        buildControls();
        buildInspector();
        buildSeed();
        addMouseListener (this, true);
        for (auto& placement : placements)
            if (placement.region == 6 && placement.control->change)
                sliderAppearance (*placement.control);
        knownDocument = processor.getDocumentGeneration();
        knownPattern = slot();
        synchronise();
        startTimerHz (30);
    }
    NativeEditor::~NativeEditor()
    {
        stopTimer();
        releaseKeys();
        keyboard.release();
    }
    Control& NativeEditor::button (juce::String name, juce::String caption, int region, Rect b, std::function<void (juce::ModifierKeys)> callback, const char* art)
    {
        auto item = std::make_unique<Control> (palette, std::move (name));
        auto& result = *item;
        result.caption = std::move (caption);
        result.action = [this, invoke = std::move (callback)] (auto mods) { if (controller.gestureActive() && invoke) invoke (mods); };
        if (art)
            result.artwork = icon (art);
        result.begin = [this] { controller.begin(); };
        result.end = [this] { if (! scalePicker) controller.commit(); edited(); };
        addAndMakeVisible (result);
        result.addKeyListener (this);
        placements.push_back ({ &result, region, b, {} });
        controls.push_back (std::move (item));
        return result;
    }
    Control& NativeEditor::parameter (juce::String name, juce::String label, int region, Rect b, double min, double max, double reset, std::function<double()> get, std::function<void (double)> set, std::function<juce::String()> display)
    {
        auto& result = button (std::move (name), {}, region, b, {});
        placements.back().label = std::move (label);
        result.minimum = min;
        result.maximum = max;
        result.reset = reset;
        result.value = std::move (get);
        result.change = [this, setter = std::move (set)] (double v) { if (! controller.gestureActive()) return; setter (v); controller.rows = readRows (controller.p); previewDirty = true; grid.updateLayout(); repaint(); };
        result.display = display ? std::move (display) : [getter = result.value] { return number (getter()); };
        return result;
    }
    void NativeEditor::buildControls()
    {
        auto& p = controller.p;
        auto& view = button ("Toggle compact steps", {}, 0, { 297, 40, 65, 32 }, [this] (auto) {
            viewThumbFrom = viewThumbPosition();
            viewThumbStart = juce::Time::getMillisecondCounterHiRes();
            viewThumbMoving = true;
            grid.compact = ! grid.compact;
            updateMetadata();
            grid.updateLayout();
        });
        view.radius = 16;
        view.customPaint = [this] (juce::Graphics& g, Rect b) {
            const float outerRadius = b.getHeight() * 0.5f;
            const auto ringTop = palette.light ? palette.borderSubtle : palette.borderSubtle;
            const auto ringBottom = palette.light ? palette.borderSubtle : palette.border;
            g.setGradientFill (juce::ColourGradient (ringTop, b.getTopLeft(), ringBottom, b.getBottomLeft(), false));
            g.fillRoundedRectangle (b, outerRadius);
            const float insetX = b.getWidth() * (10.5f / 117.6f);
            const float insetY = b.getHeight() * (7.0f / 57.6f);
            const auto track = b.reduced (insetX, insetY);
            {
                juce::Graphics::ScopedSaveState lane (g);
                juce::Path ring;
                ring.addRoundedRectangle (b, outerRadius);
                ring.addRoundedRectangle (track, track.getHeight() * 0.5f);
                ring.setUsingNonZeroWinding (false);
                g.reduceClipRegion (ring);
                g.setColour (palette.app.withAlpha (palette.light ? 0.28f : 0.62f));
                for (float x = b.getX() + 3.0f; x < b.getRight() - 3.0f; x += 2.5f)
                    for (float y = b.getY() + 1.5f; y < b.getBottom() - 1.5f; y += 3.1f)
                        g.fillRect (x, y, 0.65f, 1.35f);
                g.setColour (palette.app.withAlpha (palette.light ? 0.16f : 0.26f));
                for (int band = 1; band <= 3; ++band)
                    g.drawHorizontalLine (juce::roundToInt (b.getY() + b.getHeight() * static_cast<float> (band) / 4.0f), b.getX() + 4.0f, b.getRight() - 4.0f);
            }
            g.setColour (palette.light ? palette.surfaceMuted.interpolatedWith (palette.workspace, 0.42f) : juce::Colours::black.withAlpha (0.60f));
            g.fillRoundedRectangle (track, track.getHeight() * 0.5f);
            g.setColour (palette.border.withAlpha (palette.light ? 0.18f : 0.30f));
            g.drawRoundedRectangle (track.reduced (0.5f), track.getHeight() * 0.5f, 1);
            const float pad = 1.15f;
            const float gap = 2.1f;
            const auto bed = track.reduced (pad);
            const float thumbW = (bed.getWidth() - gap) * 0.5f;
            auto thumb = bed.withWidth (thumbW).translated ((thumbW + gap * 0.5f) * viewThumbPosition(), 0);
            const auto thumbTop = palette.accent.interpolatedWith (juce::Colours::white, 0.32f);
            const auto thumbBottom = palette.strong.interpolatedWith (palette.accent, 0.18f);
            g.setColour (palette.accent.withAlpha (0.35f));
            g.fillRoundedRectangle (thumb.expanded (1.2f), thumb.getHeight() * 0.5f);
            g.setGradientFill (juce::ColourGradient (thumbTop, thumb.getTopLeft(), thumbBottom, thumb.getBottomLeft(), false));
            g.fillRoundedRectangle (thumb, thumb.getHeight() * 0.5f);
            g.setColour (juce::Colours::white.withAlpha (0.36f));
            g.drawLine (thumb.getX() + 4.0f, thumb.getY() + 1.2f, thumb.getRight() - 4.0f, thumb.getY() + 1.2f, 1);
            auto paintIcon = [&] (int side, bool engaged) {
                auto cell = bed.withWidth (thumbW).translated (side == 0 ? 0 : thumbW + gap, 0);
                auto icon = cell.withSizeKeepingCentre (11.0f, 12.0f);
                g.setColour (engaged ? palette.app : palette.muted.withAlpha (0.55f));
                if (side == 0)
                {
                    g.drawRoundedRectangle (icon, 1.6f, 1.15f);
                    g.drawLine (icon.getX() + 1.6f, icon.getY() + 3.2f, icon.getRight() - 1.6f, icon.getY() + 3.2f, 1.1f);
                    g.drawLine (icon.getX() + 2.6f, icon.getY() + 6.0f, icon.getRight() - 2.6f, icon.getY() + 6.0f, 1.1f);
                    g.drawLine (icon.getX() + 2.6f, icon.getY() + 8.8f, icon.getRight() - 4.4f, icon.getY() + 8.8f, 1.1f);
                }
                else
                    g.fillRoundedRectangle (icon.reduced (0.4f), 2.2f);
            };
            paintIcon (0, !grid.compact);
            paintIcon (1, grid.compact);
        };
        button ("Undo", juce::String::fromUTF8 ("↶"), 0, { 374, 40, 32, 32 }, [this] (auto) { controller.cancel(); controller.undo(); }, UndoIcon);
        button ("Redo", juce::String::fromUTF8 ("↷"), 0, { 410, 40, 32, 32 }, [this] (auto) { controller.cancel(); controller.redo(); }, RedoIcon);
        auto& pulse = parameter ("Pulse length", "Pulse", 0, { 450, 40, 31, 32 }, 0, 3, 2, [&p] { return 3 - p.getPulseIndex(); }, [&p] (double v) { p.setPulseIndex (3 - static_cast<int> (v)); }, [&p] { const juce::StringArray values {juce::String::fromUTF8 ("½"), juce::String::fromUTF8 ("♩"), juce::String::fromUTF8 ("♪"), juce::String::fromUTF8 ("♬")}; return values[p.getPulseIndex()]; });
        pulse.pixelsPerUnit = 18;
        pulse.customPaint = [this, &p] (juce::Graphics& g, Rect b) {
            face (g, b, palette);
            juce::Graphics::ScopedSaveState save (g);
            g.addTransform (juce::AffineTransform::scale (.625f).translated (8, 5));
            g.setColour (palette.text);
            const int kind = p.getPulseIndex();
            const Rect head (3, 19.5f, 12, 9);
            if (kind <= 1)
                g.fillEllipse (head);
            else
                g.drawEllipse (kind == 2 ? head : Rect (3, 16.5f, 16, 11), 1.75f);
            if (kind != 3)
                g.drawLine (14.25f, 22.5f, 14.25f, 7, 2);
            if (kind == 0)
            {
                juce::Path flag;
                flag.startNewSubPath (14.25f, 7);
                flag.cubicTo (20.5f, 9.25f, 20.75f, 15.5f, 14.25f, 18.25f);
                g.strokePath (flag, juce::PathStrokeType (2));
            }
        };
        parameter ("Swing", "Swing", 0, { 486, 40, 31, 32 }, 0, 99, 0, [&p] { return p.getSwingPercent(); }, [&p] (double v) { p.setSwingPercent (static_cast<int> (v)); });
        parameter ("Swing subdivision", "Sub", 0, { 522, 40, 31, 32 }, 0, 2, 1, [&p] { return p.getSwingSubdivisionIndex(); }, [&p] (double v) { p.setSwingSubdivisionIndex (static_cast<int> (v)); }, [&p] { return number (PluginProcessor::swingSubdivisionForIndex (p.getSwingSubdivisionIndex())); });
        parameter ("Velocity humanize percent", "Vel %", 0, { 560, 40, 36, 32 }, 0, 99, 0, [&p] { return p.getVelocityHumanizePercent(); }, [&p] (double v) { p.setVelocityHumanizePercent (static_cast<int> (v)); });
        parameter ("Timing humanize percent", "Time %", 0, { 600, 40, 36, 32 }, 0, 99, 0, [&p] { return p.getTimingHumanizePercent(); }, [&p] (double v) { p.setTimingHumanizePercent (static_cast<int> (v)); });
        auto& scale = button ("Scale mode", {}, 0, { 666, 22, 100, 50 }, [this] (auto) { openScale(); });
        scale.display = [this, &p] { return noteName (p.getPatternScaleRoot (slot()) + 60).dropLastCharacters (1) + " " + scaleNames[p.getPatternScaleModeIndex (slot())]; };
        scale.customPaint = [this, &p] (juce::Graphics& g, Rect b) {
            g.setColour (palette.app.withAlpha (.55f));
            g.fillRoundedRectangle (b, 6);
            g.setColour (palette.border);
            g.drawRoundedRectangle (b.reduced (.5f), 6, 1);
            text (g, noteName (p.getPatternScaleRoot (slot()) + 60).dropLastCharacters (1), b.withHeight (32), palette.text, 22, true);
            text (g, scaleNames[p.getPatternScaleModeIndex (slot())], b.withY (31).withHeight (16), palette.accent, 12, true);
        };
        auto& seed = button ("Seed mode", {}, 0, { 774, 22, 48, 50 }, [this] (auto) { showSeed(); }, SaplingIcon);
        seed.active = [this] { return seedOpen; };
        seed.iconInset = 12;
        seed.translucent = true;
        seed.accentWhenIdle = true;
        seed.glow = true;
        const char* art[] { StepSkipIcon, StepMuteIcon, RowShiftOrderIcon, RowReverseOrderIcon, RowRandomizeOrderIcon, RowRandomizeOctaveIcon, RowRandomizeLengthIcon };
        const char* names[] { "Skip selected steps", "Mute selected steps", "Shift selected steps left by row", "Reverse selected steps by row", "Shuffle selected steps", "Randomize selected step octaves", "Randomize selected step lengths" };
        for (int i = 0; i < 7; ++i)
            button (names[i], {}, 0, { 832 + static_cast<float> (i) * 36, 40, 32, 32 }, [this, i] (auto) { bulkOrder (i); }, art[i]);
        const char* labels[] { "Len", "Dur %", "Vel %", "Pitch" };
        for (int i = 0; i < 4; ++i)
        {
            auto baseline = std::make_shared<Rows>();
            auto& field = parameter ("Bulk " + juce::String (labels[i]), labels[i], 0, { 1092 + static_cast<float> (i) * 37, 40, 33, 32 }, -99, 99, 0, [] { return 0.0; }, [this, i, baseline] (double v) {
            // Recompute from the start of the drag, including when a value hits its limit.
            controller.rows = *baseline;
            const int d = static_cast<int> (v);
            controller.editSelected ([this, i, d] (Step& s)
            {
                if (i == 0) s.timing = juce::jlimit (0, 15, s.timing + d);
                if (i == 1) s.duration = juce::jlimit (0.0, 1.0, s.duration + d / 100.0);
                if (i == 2) s.velocity = juce::jlimit (0, 127, s.velocity + juce::roundToInt (d * 1.27));
                if (i == 3) s.note = scaleNote (s.note, d, controller.p.getPatternScaleRoot (slot()), controller.p.getPatternScaleModeIndex (slot()));
            }, true); });
            field.begin = [this, baseline] { controller.begin(); *baseline = controller.rows; };
            field.pixelsPerUnit = 8;
        }
        for (int i = 0; i < 8; ++i)
        {
            auto& pattern = button ("Select pattern " + juce::String (i + 1), juce::String (i + 1), 0, { 1315 + static_cast<float> (i) * 37.5f, 12, 33.5f, 32 }, [this, &p, i] (auto mods) {
                if (mods.isShiftDown())
                {
                    copyPattern = copyPattern == i ? -1 : i;
                    clearPattern = -1;
                    repaintPatternSlots();
                    return;
                }
                if (copyPattern >= 0)
                {
                    p.copyPatternSlot (copyPattern, i);
                    copyPattern = -1;
                }
                p.setCurrentPatternSlot (i);
                clearPattern = -1;
                controller.clearSelection();
                inspectorRow = inspectorStep = -1;
                preview.editingRow = -1;
                keyboard.scrolling = false;
                grid.presentFlip (-1, -1);
                repaintPatternSlots();
            });
            pattern.active = [&p, i] { return p.getCurrentPatternSlot() == i; };
            pattern.copySource = [this, i] { return copyPattern == i; };
            pattern.copyTarget = [this, i] { return copyPattern >= 0 && copyPattern != i; };
            pattern.strongLabel = true;
            pattern.glow = true;
            pattern.radius = 3;
            auto& loop = button ("Select or Shift-save loop " + juce::String (i + 1), juce::String (i + 1), 0, { 1315 + static_cast<float> (i) * 37.5f, 49, 33.5f, 32 }, [&p, i] (auto mods) { if (mods.isShiftDown() || ! p.isLoopSlotAssigned (i)) p.saveCurrentBraceToLoopSlot (i); else p.selectLoopSlot (i); });
            loop.active = [&p, i] { return p.getCurrentLoopSlot() == i; };
            loop.strongLabel = true;
            loop.glow = true;
            loop.radius = 3;
            loop.dimmed = [&p, i] { return ! p.isLoopSlotAssigned (i); };;
        }
        auto& clear = button ("Clear selected pattern", juce::String::fromUTF8 ("×"), 0, { 1614, 12, 24, 32 }, [this, &p] (auto) {
            if (clearPattern != slot())
            {
                clearPattern = slot();
                copyPattern = -1;
                repaintPatternSlots();
                return;
            }
            clearPattern = -1;
            p.clearPatternSlot (slot());
            controller.clearSelection();
            repaintPatternSlots();
        });
        clear.bare = true;
        clear.customPaint = [this] (juce::Graphics& g, Rect b) {
            juce::Graphics::ScopedSaveState save (g);
            if (clearPattern == slot())
                g.setOpacity (1.0f - 0.7f * pulseAmount());
            text (g, juce::String::fromUTF8 ("×"), b, palette.danger, 20, true);
        };
        button ("Mute output", "M", 0, { 1640, 26, 44, 44 }, [&p] (auto) { p.deactivatePatternOutput(); }).active = [&p] { return !p.isPatternOutputArmed(); };
        constexpr int modes[] { 0, 7, 8, 1, 6, 3, 4 };
        const char* modesArt[] { crossMod, canon, retroInv, tendril, hocket, echo, weave };
        const char* modeNames[] { "Cross-Mod", "Canon", "Retro-Inv", "Tendril", "Hocket", "Echo", "Weave" };
        for (int i = 0; i < 7; ++i)
        {
            const int mode = modes[i];
            auto& control = button (modeNames[i], {}, 1, { 24 + static_cast<float> (i) * 65.5f, 7, 47, 47 }, [&p, mode] (auto) { p.setCombinationModeEnabled (mode, !p.isCombinationModeEnabled (mode)); }, modesArt[i]);
            control.processing = true;
            control.active = [&p, mode] { return p.isCombinationModeEnabled (mode); };
        }
        parameter ("Combination change quantize", {}, 1, { 472, 16, 52, 30 }, 0, 4, 2, [&p] { return p.getCombinationSyncDivisionIndex(); }, [&p] (double v) { p.setCombinationSyncDivisionIndex (static_cast<int> (v)); }, [&p] { return number (PluginProcessor::combinationSyncDivisionMultiplierForIndex (p.getCombinationSyncDivisionIndex())); });
        button ("Continue timeline on combination mode change", juce::String::fromUTF8 ("•"), 1, { 531, 23, 15, 15 }, [&p] (auto) { p.setCombinationChangePhaseContinue (!p.isCombinationChangePhaseContinue()); }).active = [&p] { return p.isCombinationChangePhaseContinue(); };
        auto& down = button ("Down one octave", "-8va", 1, { 605, 7, 47, 47 }, [this, &p] (auto) { p.setPatternOctavizerDown8vaEnabled (!p.isPatternOctavizerDown8vaEnabled (slot())); });
        down.processing = true;
        down.size = 11;
        down.active = [this, &p] { return p.isPatternOctavizerDown8vaEnabled (slot()); };
        parameter ("Down octave relative velocity", "Vel", 1, { 657, 21, 46, 19 }, -127, 127, 0, [this, &p] { return p.getPatternOctavizerDown8vaRelativeVelocity (slot()); }, [&p] (double v) { p.setPatternOctavizerDown8vaRelativeVelocity (static_cast<int> (v)); });
        auto& up = button ("Up one octave", "8va", 1, { 720, 7, 47, 47 }, [this, &p] (auto) { p.setPatternOctavizerUp8vaEnabled (!p.isPatternOctavizerUp8vaEnabled (slot())); });
        up.processing = true;
        up.size = 11;
        up.active = [this, &p] { return p.isPatternOctavizerUp8vaEnabled (slot()); };
        parameter ("Up octave relative velocity", "Vel", 1, { 772, 21, 46, 19 }, -127, 127, 0, [this, &p] { return p.getPatternOctavizerUp8vaRelativeVelocity (slot()); }, [&p] (double v) { p.setPatternOctavizerUp8vaRelativeVelocity (static_cast<int> (v)); });
        auto& shimmer = button ("Shimmer", {}, 1, { 835, 7, 47, 47 }, [this, &p] (auto) { p.setPatternShimmerEnabled (!p.isPatternShimmerEnabled (slot())); }, ShimmerIcon);
        shimmer.processing = true;
        shimmer.active = [this, &p] { return p.isPatternShimmerEnabled (slot()); };
        parameter ("Shimmer delay", "Del", 1, { 886, 21, 36, 19 }, 0, 15, 3, [this, &p] { return p.getPatternShimmerDelayMultiplierIndex (slot()); }, [&p] (double v) { p.setPatternShimmerDelayMultiplierIndex (static_cast<int> (v)); }, [this, &p] { return number ((p.getPatternShimmerDelayMultiplierIndex (slot()) + 1) * .25); });
        parameter ("Shimmer feedback", "Fbk", 1, { 926, 21, 46, 19 }, 0, 99, 10, [this, &p] { return p.getPatternShimmerFeedbackPercent (slot()); }, [&p] (double v) { p.setPatternShimmerFeedbackPercent (static_cast<int> (v)); }, [this, &p] { return juce::String (p.getPatternShimmerFeedbackPercent (slot())) + "%"; });
        parameter ("Shimmer mix", "Mix", 1, { 976, 21, 46, 19 }, 0, 99, 20, [this, &p] { return p.getPatternShimmerMixPercent (slot()); }, [&p] (double v) { p.setPatternShimmerMixPercent (static_cast<int> (v)); }, [this, &p] { return juce::String (p.getPatternShimmerMixPercent (slot())) + "%"; });
        parameter ("Bandpass low note", "Min", 1, { 1080, 21, 46, 19 }, 0, 127, 36, [this, &p] { return p.getPatternNoteBandpassLow (slot()); }, [this, &p] (double v) { p.setPatternNoteBandpass (static_cast<int> (v), p.getPatternNoteBandpassHigh (slot())); }, [this, &p] { return noteName (p.getPatternNoteBandpassLow (slot())); });
        auto& bandpass = parameter ("Bandpass range", {}, 1, { 1134, 17, 80, 28 }, 0, 127, 108, [this, &p] { return p.getPatternNoteBandpassHigh (slot()); }, [this, &p] (double v) { p.setPatternNoteBandpass (p.getPatternNoteBandpassLow (slot()), static_cast<int> (v)); });
        auto bandStart = std::make_shared<std::array<int, 3>>();
        bandpass.pointerStart = [this, &p, bandStart] (const auto& e) {
            const int low = p.getPatternNoteBandpassLow (slot()), high = p.getPatternNoteBandpassHigh (slot());
            const float note = (e.position.x - 5) / 70 * 127;
            *bandStart = { low, high, std::abs (note - low) <= std::abs (note - high) ? 0 : 1 };
        };
        bandpass.pointerDrag = [this, &p, bandStart] (const auto& e) {
            if (!controller.gestureActive())
                return;
            const int delta = juce::roundToInt (e.getDistanceFromDragStartX() * 127.0 / 70 * (e.mods.isShiftDown() ? .22 : 1));
            int low = (*bandStart)[0], high = (*bandStart)[1];
            if ((*bandStart)[2] == 0)
                low = juce::jlimit (0, high, low + delta);
            else
                high = juce::jlimit (low, 127, high + delta);
            p.setPatternNoteBandpass (low, high);
            previewDirty = true;
        };
        bandpass.customPaint = [this, &p] (juce::Graphics& g, Rect b) {
            const float low = 5 + p.getPatternNoteBandpassLow (slot()) / 127.0f * 70, high = 5 + p.getPatternNoteBandpassHigh (slot()) / 127.0f * 70;
            face (g, b.withHeight (12).withCentre (b.getCentre()), palette);
            g.setColour (palette.accent.withAlpha (.65f));
            g.fillRect (Rect (low, 10, high - low, 8));
            for (float x : { low, high })
            {
                face (g, { x - 4, 1, 8, 26 }, palette, false, 2);
                g.setColour (palette.muted);
                g.drawVerticalLine (juce::roundToInt (x), 5, 23);
            }
        };
        parameter ("Bandpass high note", "Max", 1, { 1223, 21, 46, 19 }, 0, 127, 108, [this, &p] { return p.getPatternNoteBandpassHigh (slot()); }, [this, &p] (double v) { p.setPatternNoteBandpass (p.getPatternNoteBandpassLow (slot()), static_cast<int> (v)); }, [this, &p] { return noteName (p.getPatternNoteBandpassHigh (slot())); });
        auto& tilt = parameter ("Velocity tilt", {}, 1, { 1317, 7, 77, 47 }, -48, 48, 0, [this, &p] { return p.getPatternVelocityTiltAmount (slot()); }, [&p] (double v) { p.setPatternVelocityTiltAmount (static_cast<int> (v)); });
        auto tiltStart = std::make_shared<std::array<int, 2>>();
        tilt.pointerStart = [this, &p, tiltStart] (const auto&) { *tiltStart = { p.getPatternVelocityTiltAmount (slot()), p.getPatternVelocityTiltPivotMidi (slot()) }; };
        tilt.pointerDrag = [this, &p, tiltStart] (const auto& e) {
            if (!controller.gestureActive())
                return;
            p.setPatternVelocityTiltAmount (juce::jlimit (-48, 48, (*tiltStart)[0] - e.getDistanceFromDragStartY() / 4));
            p.setPatternVelocityTiltPivotMidi (juce::jlimit (p.getPatternNoteBandpassLow (slot()), p.getPatternNoteBandpassHigh (slot()), (*tiltStart)[1] + e.getDistanceFromDragStartX() / 8));
            previewDirty = true;
        };
        tilt.customPaint = [this, &p] (juce::Graphics& g, Rect b) {
            face (g, b, palette, false, 1);
            b = b.reduced (5);
            g.setGradientFill (juce::ColourGradient (palette.subtle, b.getTopLeft(), palette.raised, b.getBottomLeft(), false));
            g.fillRect (b);
            const int low = p.getPatternNoteBandpassLow (slot()), high = p.getPatternNoteBandpassHigh (slot());
            const int pivot = juce::jlimit (low, high, p.getPatternVelocityTiltPivotMidi (slot()));
            juce::Path curve;
            for (int note = low; note <= high; ++note)
            {
                const float x = b.getX() + b.getWidth() * (note - low) / static_cast<float> (std::max (1, high - low));
                const float velocity = juce::jlimit (1.0f, 127.0f, 64.0f + (note - pivot) / 12.0f * p.getPatternVelocityTiltAmount (slot()));
                const float y = b.getBottom() - b.getHeight() * (velocity - 1) / 126;
                if (note == low)
                    curve.startNewSubPath (x, y);
                else
                    curve.lineTo (x, y);
            }
            g.setColour (palette.accent);
            g.strokePath (curve, juce::PathStrokeType (2));
            const float x = b.getX() + b.getWidth() * (pivot - low) / static_cast<float> (std::max (1, high - low));
            g.setColour (palette.text);
            g.fillEllipse (x - 4, b.getCentreY() - 4, 8, 8);
        };
        parameter ("Velocity tilt pivot", "Pivot", 1, { 1400, 21, 46, 19 }, 0, 127, 60, [this, &p] { return p.getPatternVelocityTiltPivotMidi (slot()); }, [&p] (double v) { p.setPatternVelocityTiltPivotMidi (static_cast<int> (v)); }, [this, &p] { return noteName (p.getPatternVelocityTiltPivotMidi (slot())); });
        button ("Transpose down octave", "-8va", 1, { 1495, 21, 34, 19 }, [this, &p] (auto) { p.setPatternGlobalTransposeSemitones (p.getPatternGlobalTransposeSemitones (slot()) - 12); }).size = 11;
        parameter ("Global transpose", "Semi", 1, { 1533, 21, 36, 19 }, -48, 48, 0, [this, &p] { return p.getPatternGlobalTransposeSemitones (slot()); }, [&p] (double v) { p.setPatternGlobalTransposeSemitones (static_cast<int> (v)); });
        button ("Transpose up octave", "8va", 1, { 1573, 21, 34, 19 }, [this, &p] (auto) { p.setPatternGlobalTransposeSemitones (p.getPatternGlobalTransposeSemitones (slot()) + 12); }).size = 11;
        auto& drum = button ("Drum row pitch lock", {}, 1, { 1620, 7, 47, 47 }, [this, &p] (auto) { p.setPatternDrumRowPitchLockEnabled (!p.isPatternDrumRowPitchLockEnabled (slot())); }, snare);
        drum.processing = true;
        drum.active = [this, &p] { return p.isPatternDrumRowPitchLockEnabled (slot()); };
        auto& loop = button ("Loop brace enabled", "LOOP OFF", 3, { 24, 8, 62, 21 }, [&p] (auto) { p.setLoopBraceEnabled (!p.isLoopBraceEnabled()); });
        loop.size = 11;
        loop.display = [&p] { return p.isLoopBraceEnabled() ? "LOOP ON" : "LOOP OFF"; };
        loop.active = [&p] { return p.isLoopBraceEnabled(); };
        button ("Fit notes", "FIT", 3, { 99, 8, 32, 21 }, [this] (auto) { preview.fit(); }).size = 11;
        button ("Close inspector or row editor", juce::String::fromUTF8 ("×"), 3, { 1640, 8, 24, 21 }, [this] (auto) { inspectorRow = inspectorStep = -1; preview.editingRow = -1; resized(); });
        const juce::String projectLabels[] { "NEW", "LOAD", "SAVE", juce::String::fromUTF8 ("◀"), juce::String::fromUTF8 ("▶") };
        const float projectX[] { 24, 98, 176, 264, 1483 };
        const float projectW[] { 66, 70, 70, 40, 40 };
        for (int i = 0; i < 5; ++i)
        {
            auto& projectButton = button (projectLabels[i], projectLabels[i], 2, { projectX[i], 6, projectW[i], 32 }, [this, i] (auto) { controller.cancel(); if (projectAction) projectAction (i); });
            projectButton.bare = true;
            projectButton.radius = 4;
            const bool arrow = i >= 3;
            projectButton.customPaint = [this, i, arrow, control = &projectButton] (juce::Graphics& g, Rect b) {
                g.setColour (palette.raised);
                g.fillRoundedRectangle (b, 4);
                g.setColour (control->isMouseOver() ? palette.accent.withAlpha (0.6f) : palette.borderSubtle);
                g.drawRoundedRectangle (b.reduced (0.5f), 4, 1);
                const juce::String label = i == 0 ? "NEW" : i == 1 ? "LOAD" : i == 2 ? "SAVE" : i == 3 ? juce::String::fromUTF8 ("◀") : juce::String::fromUTF8 ("▶");
                text (g, label, b, arrow ? palette.muted : palette.text, arrow ? 16.0f : 12.0f, !arrow, juce::Justification::centred, arrow ? 0.0f : 0.16f);
            };
        }
        button ("Change theme", {}, 2, { 1546, 6, 32, 32 }, [this] (auto) { theme = theme == "dark" ? "alt" : theme == "alt" ? "light" : "dark"; palette.set (theme); updateMetadata(); synchronise(); }, ThemeMoonIcon).bare = true;
        parameter ("UI scale percent", {}, 2, { 1582, 6, 46, 32 }, 50, 100, 100, [&p] { return p.getProjectUiScalePercent(); }, [this, &p] (double v) { p.setProjectUiScalePercent (static_cast<int> (v)); if (scaleChanged) scaleChanged(); }, [&p] { return juce::String (p.getProjectUiScalePercent()) + "%"; });
        button ("Fullscreen", {}, 2, { 1640, 6, 32, 32 }, [this] (auto) { if (fullscreenChanged) fullscreenChanged(); }, FullscreenIcon).bare = true;
        auto& play = button ("Toggle transport (Space)", "Play", 4, { 1472, 8, 64, 32 }, [&p] (auto) { p.setStandaloneTransportPlaying (!p.isStandaloneTransportPlaying()); });
        play.display = [&p] { return p.isStandaloneTransportPlaying() ? "Stop" : "Play"; };
        play.active = [&p] { return p.isStandaloneTransportPlaying(); };
        auto& tempo = parameter ("Tempo", "BPM", 4, { 1575, 8, 90, 32 }, 20, 300, 120, [&p] { return p.getStandaloneTempoBpm(); }, [&p] (double v) { p.setStandaloneTempoBpm (v); });
        tempo.customPaint = [this, &tempo] (juce::Graphics& g, Rect b) {
            face (g, b, palette);
            text (g, tempo.display ? tempo.display() : juce::String(), b.withTrimmedRight (22), palette.text, 14, true);
            const auto spinner = Rect (b.getRight() - 20, b.getCentreY() - 11, 14, 22);
            g.setColour (palette.raised);
            g.fillRoundedRectangle (spinner, 3);
            g.setColour (palette.border);
            g.drawRoundedRectangle (spinner.reduced (0.5f), 3, 1);
            g.setColour (palette.secondary);
            juce::Path chevrons;
            const float x = spinner.getCentreX();
            chevrons.addTriangle (x - 3.2f, spinner.getY() + 9, x + 3.2f, spinner.getY() + 9, x, spinner.getY() + 4);
            chevrons.addTriangle (x - 3.2f, spinner.getBottom() - 9, x + 3.2f, spinner.getBottom() - 9, x, spinner.getBottom() - 4);
            g.fillPath (chevrons);
        };
        button ("Edit notes on the piano roll", "Roll", 8, { 0, 8, 188, 32 }, [this] (auto) { preview.drawVelocity = false; preview.repaint(); }).active = [this] { return !preview.drawVelocity; };
        button ("Draw velocity shape across steps", "Velocity", 8, { 0, 48, 188, 32 }, [this] (auto) { preview.drawVelocity = true; preview.repaint(); }).active = [this] { return preview.drawVelocity; };
        parameter ("Recording cycle length in pulses", "Cycle length", 7, { 0, 36, 188, 32 }, 1, 64, 16, [&p] { return p.getPhraseRowRecordingCycleLength(); }, [&p] (double v) { p.setPhraseRowRecordingCycleLength (static_cast<int> (v)); });
        button ("Finish recording", "Finish", 7, { 0, 220, 188, 32 }, [this, &p] (auto) { if (p.getPhraseRowRecording() >= 0) record (p.getPhraseRowRecording()); });
    }

    Step NativeEditor::currentStep() const
    {
        if (inspectorRow < 0 || inspectorStep < 0)
            return {};
        const auto& steps = controller.rows[static_cast<size_t> (inspectorRow)].steps;
        return inspectorStep < static_cast<int> (steps.size()) ? steps[static_cast<size_t> (inspectorStep)] : Step {};
    }
    void NativeEditor::editStep (const std::function<void (Step&)>& change)
    {
        if (inspectorRow < 0 || inspectorStep < 0)
            return;
        if (!controller.hasSelection())
            controller.selection[static_cast<size_t> (inspectorRow)].set (static_cast<size_t> (inspectorStep));
        controller.editSelected (change);
        keyboard.selected = currentStep().note;
    }
    void NativeEditor::buildInspector()
    {
        auto& skip = button ("Skip inspected step", "Skip", 5, { 0, 8, 90, 28 }, [this] (auto) { const bool value = ! currentStep().skipped; editStep ([value] (Step& s) { s.skipped = value; }); }, StepSkipIcon);
        skip.active = [this] { return currentStep().skipped; };
        skip.iconInset = 6;
        auto& mute = button ("Mute inspected step", "Mute", 5, { 98, 8, 90, 28 }, [this] (auto) { const bool value = ! currentStep().muted; editStep ([value] (Step& s) { s.muted = value; }); }, StepMuteIcon);
        mute.active = [this] { return currentStep().muted; };
        mute.iconInset = 6;
        for (auto* toggle : { &skip, &mute })
        {
            auto lastInk = std::make_shared<juce::Colour> (juce::Colours::white);
            toggle->customPaint = [this, toggle, lastInk] (juce::Graphics& g, Rect b) {
                const bool on = toggle->active && toggle->active();
                face (g, b, palette, on, 6);
                const auto ink = on ? palette.primaryText : palette.secondary;
                if (toggle->artwork)
                {
                    toggle->artwork->replaceColour (*lastInk, ink);
                    *lastInk = ink;
                    toggle->artwork->drawWithin (g, b.withWidth (28).reduced (6, 5), juce::RectanglePlacement::centred, 1);
                }
                text (g, toggle->caption, b.withTrimmedLeft (22), ink, 12, true);
            };
        }
        parameter ("Step multiplier", "Multiplier", 5, { 0, 65, 188, 28 }, 0, 15, 3, [this] { return currentStep().timing; }, [this] (double v) { const int delta = static_cast<int> (v) - currentStep().timing; editStep ([delta] (Step& s) { s.timing = juce::jlimit (0, 15, s.timing + delta); }); }, [this] { return number ((currentStep().timing + 1) * .25); });
        parameter ("Step duration", "Duration %", 5, { 0, 115, 188, 28 }, 0, 100, 100, [this] { return currentStep().duration * 100; }, [this] (double v) { const double delta = v / 100 - currentStep().duration; editStep ([delta] (Step& s) { s.duration = juce::jlimit (0.0, 1.0, s.duration + delta); }); });
        parameter ("Step velocity", "Velocity", 5, { 0, 165, 188, 28 }, 0, 127, 100, [this] { return currentStep().velocity; }, [this] (double v) { const int delta = static_cast<int> (v) - currentStep().velocity; editStep ([delta] (Step& s) { s.velocity = juce::jlimit (0, 127, s.velocity + delta); }); });
        parameter ("Step probability", "Probability", 5, { 1130, 38, 300, 28 }, 0, 100, 100, [this] { return currentStep().probability; }, [this] (double v) { editStep ([v] (Step& s) { s.probability = static_cast<int> (v); }); }, [this] { return juce::String (currentStep().probability) + "%"; });
        parameter ("Step cycle length", {}, 5, { 670, 64, 300, 16 }, 1, 8, 1, [this] { return currentStep().cycle; }, [this] (double v) { editStep ([v] (Step& s) { s.cycle = static_cast<int> (v); }); });
        for (int i = 0; i < 8; ++i)
        {
            auto& cycle = button ("Cycle beat " + juce::String (i + 1), {}, 5, { 670 + static_cast<float> (i) * 36, 40, 36, 22 }, [this, i] (auto) { editStep ([i] (Step& s) { if (i < s.cycle) { s.mask ^= 1 << i; if (s.mask == 0) s.mask = 1; } }); });
            cycle.bare = true;
            cycle.active = [this, i] { return i < currentStep().cycle && (currentStep().mask & (1 << i)) != 0; };
            cycle.customPaint = [this, i] (juce::Graphics& g, Rect b) {
                const int length = std::max (1, currentStep().cycle);
                const bool inside = i < length;
                const bool on = inside && (currentStep().mask & (1 << i)) != 0;
                const auto accent = inspectorRow >= 0 ? palette.rows[static_cast<size_t> (inspectorRow)] : palette.accent;
                g.setColour (inside ? palette.surface : palette.app.withAlpha (0.55f));
                g.fillRect (b);
                if (on)
                {
                    g.setColour (accent);
                    g.fillRect (b.reduced (1, 1));
                }
                else if (inside)
                {
                    juce::Graphics::ScopedSaveState hatch (g);
                    g.reduceClipRegion (b.toNearestInt());
                    g.setColour (palette.muted.withAlpha (0.55f));
                    for (float x = b.getX() - b.getHeight(); x < b.getRight(); x += 4.0f)
                        g.drawLine (x, b.getBottom(), x + b.getHeight(), b.getY(), 1);
                }
                g.setColour (palette.border);
                g.drawRect (b, 1);
            };
        }
        parameter ("Step note", {}, 5, { 350, 24, 180, 56 }, 0, 127, 60, [this] { return currentStep().note; }, [this] (double v) { editStep ([this, v] (Step& s) { s.note = snapNote (static_cast<int> (v), controller.p.getPatternScaleRoot (slot()), controller.p.getPatternScaleModeIndex (slot())); }); }, [this] { return noteName (currentStep().note); });
        button ("Delete inspected steps", juce::String::fromUTF8 ("×  Delete"), 5, { 0, 240, 188, 28 }, [this] (auto) { controller.removeSelected(); inspectorRow = inspectorStep = -1; });
        for (auto& placement : placements)
        {
            auto& control = *placement.control;
            if (placement.region == 5 && control.change)
            {
                if (control.getName() == "Step cycle length")
                {
                    control.bare = true;
                    control.customPaint = [this, &control] (juce::Graphics& g, Rect b) {
                        const float length = static_cast<float> (std::max (1.0, control.value()));
                        g.setColour (palette.faint.withAlpha (0.65f));
                        g.fillRoundedRectangle ({ b.getX(), b.getCentreY() - 1.5f, b.getWidth() * length / 8.0f, 3 }, 1.5f);
                    };
                }
                else if (control.getName() != "Step note")
                    sliderAppearance (control);
                else
                {
                    control.size = 36;
                    control.bare = true;
                    control.customPaint = [this, &control] (juce::Graphics& g, Rect b) {
                        const auto accent = inspectorRow >= 0 ? palette.rows[static_cast<size_t> (inspectorRow)] : palette.accent;
                        text (g, control.display ? control.display() : juce::String(), b, accent, 36, Weight::bold);
                    };
                }
            }
        }
    }
    void NativeEditor::sliderAppearance (Control& control)
    {
        control.horizontal = control.absolute = true;
        control.customPaint = [this, &control] (juce::Graphics& g, Rect b) {
            const float trackWidth = std::max (1.0f, b.getWidth() - 45);
            const float ratio = static_cast<float> ((control.value() - control.minimum) / std::max (1.0, control.maximum - control.minimum));
            const auto accent = inspectorRow >= 0 ? palette.rows[static_cast<size_t> (inspectorRow)] : palette.accent;
            const auto track = Rect (3, b.getCentreY() - 3, trackWidth - 6, 6);
            g.setColour (palette.field);
            g.fillRoundedRectangle (track, 3);
            g.setColour (palette.border.withAlpha (0.78f));
            g.drawRoundedRectangle (track, 3, 1);
            g.setColour (accent);
            g.fillRoundedRectangle (track.withWidth (std::max (4.0f, track.getWidth() * ratio)), 3);
            const float thumbX = 3 + ratio * track.getWidth();
            const auto thumb = Rect (thumbX - 5, b.getCentreY() - 12, 10, 24);
            g.setGradientFill (juce::ColourGradient (palette.subtle, thumb.getTopLeft(), palette.field, thumb.getBottomRight(), false));
            g.fillRoundedRectangle (thumb, 2);
            g.setColour (palette.borderSubtle);
            g.drawRoundedRectangle (thumb, 2, 1);
            g.setColour (palette.text.withAlpha (0.49f));
            g.drawVerticalLine (juce::roundToInt (thumb.getCentreX()), thumb.getY() + 4, thumb.getBottom() - 4);
            text (g, control.display ? control.display() : number (control.value()), { trackWidth + 4, 0, 41, b.getHeight() }, accent, 14, Weight::bold, juce::Justification::centredRight);
        };
        control.pointerStart = [&control] (const auto& e) {
            control.change (std::clamp (std::round (control.minimum + (e.position.x - 3) / std::max (1, control.getWidth() - 51) * (control.maximum - control.minimum)), control.minimum, control.maximum));
        };
        control.pointerDrag = control.pointerStart;
        control.absolute = false;
    }
    void NativeEditor::buildSeed()
    {
        auto& p = controller.p;
        parameter ("Global rhythm overlap to interleave", "Global Overlap", 6, { 0, 24, 340, 26 }, 0, 7, 0, [this, &p] { return p.getPatternSeedingRhythmStep (slot()); }, [this, &p] (double v) {
        std::array<PluginProcessor::SeedingRowState, 4> settings; std::array<int, 4> targets;
        for (int r = 0; r < 4; ++r) { settings[static_cast<size_t> (r)] = p.getPatternSeedingRowState (slot(), r); targets[static_cast<size_t> (r)] = p.isPatternSeedingRowTargeted (slot(), r); }
        p.setPatternSeedModeState (static_cast<int> (v), settings, targets); seedGenerate (true); });
        for (int r = 0; r < 4; ++r)
        {
            auto& target = button ("Target seed row " + juce::String (r + 1), juce::String (r + 1), 6, { static_cast<float> (r) * 72, 61, 64, 30 }, [this, &p, r] (auto) {
                seedTarget = r;
                std::array<PluginProcessor::SeedingRowState, 4> settings;
                std::array<int, 4> targets;
                for (int i = 0; i < 4; ++i)
                {
                    settings[static_cast<size_t> (i)] = p.getPatternSeedingRowState (slot(), i);
                    targets[static_cast<size_t> (i)] = p.isPatternSeedingRowTargeted (slot(), i);
                }
                targets[static_cast<size_t> (r)] = 1 - targets[static_cast<size_t> (r)];
                p.setPatternSeedModeState (p.getPatternSeedingRhythmStep (slot()), settings, targets);
            });
            target.active = [this, &p, r] { return p.isPatternSeedingRowTargeted (slot(), r); };
        }
        const char* labels[] { "Steps", "Length Avg", "Length Var", "Center", "Range", "Repetition", "Complexity", "Randomness" };
        using Settings = PluginProcessor::SeedingRowState;
        const std::array<int Settings::*, 8> members { &Settings::phraseLength, &Settings::timingMeanMultiplierIndex, &Settings::timingVariance, &Settings::centerMidi, &Settings::rangeSemitones, &Settings::repetition, &Settings::complexity, &Settings::randomness };
        const int minimum[] { 2, 1, 0, 0, 2, 0, 0, 0 }, maximum[] { 16, 15, 100, 127, 48, 100, 100, 100 }, defaults[] { 4, 3, 50, 60, 24, 45, 50, 0 };
        for (size_t i = 0; i < members.size(); ++i)
        {
            const auto member = members[i];
            auto& seedParameter = parameter ("Seed " + juce::String (labels[i]), labels[i], 6, { 0, 117 + static_cast<float> (i) * 42, 340, 25 }, minimum[i], maximum[i], defaults[i], [this, &p, member] { const int v = p.getPatternSeedingRowState (slot(), seedTarget).*member; return member == &Settings::centerMidi && v < 0 ? 60 + p.getPatternScaleRoot (slot()) : v; }, [this, &p, member] (double v) {
            std::array<Settings, 4> settings; std::array<int, 4> targets;
            for (int r = 0; r < 4; ++r)
            {
                settings[static_cast<size_t> (r)] = p.getPatternSeedingRowState (slot(), r); targets[static_cast<size_t> (r)] = p.isPatternSeedingRowTargeted (slot(), r);
                if (targets[static_cast<size_t> (r)]) settings[static_cast<size_t> (r)].*member = member == &Settings::centerMidi ? snapNote (static_cast<int> (v), p.getPatternScaleRoot (slot()), p.getPatternScaleModeIndex (slot())) : static_cast<int> (v);
            }
            p.setPatternSeedModeState (p.getPatternSeedingRhythmStep (slot()), settings, targets); seedGenerate(); });
            if (i == 1) seedParameter.display = [this, &p] { return number ((p.getPatternSeedingRowState (slot(), seedTarget).timingMeanMultiplierIndex + 1) * .25); };
            if (i == 3) seedParameter.display = [this, &p] { const int note = p.getPatternSeedingRowState (slot(), seedTarget).centerMidi; return noteName (snapNote (note < 0 ? 60 + p.getPatternScaleRoot (slot()) : note, p.getPatternScaleRoot (slot()), p.getPatternScaleModeIndex (slot()))); };
            if (i == 4) seedParameter.display = [this, &p] { return juce::String (p.getPatternSeedingRowState (slot(), seedTarget).rangeSemitones) + " st"; };

        }
        button ("Seed symmetry", "Symmetry", 6, { 0, 460, 110, 28 }, [this, &p] (auto) {
            std::array<Settings, 4> settings;
            std::array<int, 4> targets;
            const int next = !p.getPatternSeedingRowState (slot(), seedTarget).symmetry;
            for (int r = 0; r < 4; ++r)
            {
                settings[static_cast<size_t> (r)] = p.getPatternSeedingRowState (slot(), r);
                targets[static_cast<size_t> (r)] = p.isPatternSeedingRowTargeted (slot(), r);
                if (targets[static_cast<size_t> (r)])
                    settings[static_cast<size_t> (r)].symmetry = next;
            }
            p.setPatternSeedModeState (p.getPatternSeedingRhythmStep (slot()), settings, targets);
            seedGenerate();
        }).active = [this, &p] { return p.getPatternSeedingRowState (slot(), seedTarget).symmetry != 0; };
        button ("Reseed targeted rows", "Reseed", 6, { 122, 460, 100, 28 }, [this, &p] (auto) {
            std::array<Settings, 4> settings;
            std::array<int, 4> targets;
            const int seed = juce::Random::getSystemRandom().nextInt (2147483646) + 1;
            for (int r = 0; r < 4; ++r)
            {
                auto& s = settings[static_cast<size_t> (r)];
                s = p.getPatternSeedingRowState (slot(), r);
                targets[static_cast<size_t> (r)] = p.isPatternSeedingRowTargeted (slot(), r);
                if (targets[static_cast<size_t> (r)])
                {
                    s.seed = seed;
                    s.repetitionSeed = s.complexitySeed = s.randomnessSeed = s.timingVarianceSeed = 0;
                }
            }
            p.setPatternSeedModeState (p.getPatternSeedingRhythmStep (slot()), settings, targets);
            seedGenerate();
        });
        button ("Randomize all seed settings", "All", 6, { 234, 460, 106, 28 }, [this] (auto) { reshuffleSeed (-1); });
        const char* aspectNames[] { "Length Var", "Repetition", "Complexity", "Randomness" };
        const int aspectRows[] { 2, 5, 6, 7 };
        for (int i = 0; i < 4; ++i)
        {
            auto& control = button ("Re-shuffle " + juce::String (aspectNames[i]), {}, 6, { 298, 117 + static_cast<float> (aspectRows[i]) * 42, 42, 25 }, [this, i] (auto) { reshuffleSeed (i); });
            control.bare = true;
            control.customPaint = [this, &control] (juce::Graphics& g, Rect b) { g.setColour (palette.workspace); g.fillRect (b); text (g, control.display(), b, palette.text, 12, true, juce::Justification::centredRight); };
            control.display = [this, &p, i] {
                const auto row = p.getPatternSeedingRowState (slot(), seedTarget);
                return juce::String (i == 0 ? row.timingVariance : i == 1 ? row.repetition
                                                               : i == 2   ? row.complexity
                                                                          : row.randomness);
            };
        }
    }
    void NativeEditor::reshuffleSeed (int aspect)
    {
        auto& p = controller.p;
        auto& random = juce::Random::getSystemRandom();
        std::array<PluginProcessor::SeedingRowState, 4> settings;
        std::array<int, 4> targets;
        for (int r = 0; r < 4; ++r)
        {
            auto& s = settings[static_cast<size_t> (r)];
            s = p.getPatternSeedingRowState (slot(), r);
            targets[static_cast<size_t> (r)] = p.isPatternSeedingRowTargeted (slot(), r);
            if (!targets[static_cast<size_t> (r)])
                continue;
            const int seed = random.nextInt (2147483646) + 1;
            if (aspect == 0)
                s.timingVarianceSeed = seed;
            if (aspect == 1)
                s.repetitionSeed = seed;
            if (aspect == 2)
                s.complexitySeed = seed;
            if (aspect == 3)
                s.randomnessSeed = seed;
            if (aspect < 0)
            {
                s.seed = seed;
                s.phraseLength = random.nextInt (15) + 2;
                s.centerMidi = snapNote (random.nextInt (49) + 36, p.getPatternScaleRoot (slot()), p.getPatternScaleModeIndex (slot()));
                s.rangeSemitones = random.nextInt (47) + 2;
                s.timingMeanMultiplierIndex = random.nextInt (15) + 1;
                s.timingVariance = random.nextInt (101);
                s.repetition = random.nextInt (101);
                s.complexity = random.nextInt (101);
                s.randomness = random.nextInt (101);
                s.symmetry = random.nextBool();
                s.repetitionSeed = s.complexitySeed = s.randomnessSeed = s.timingVarianceSeed = 0;
            }
        }
        p.setPatternSeedModeState (p.getPatternSeedingRhythmStep (slot()), settings, targets);
        seedGenerate();
    }
    void NativeEditor::seedGenerate (bool allRhythm)
    {
        auto& p = controller.p;
        std::array<PluginProcessor::SeedingRowState, 4> settings;
        for (int r = 0; r < 4; ++r)
            settings[static_cast<size_t> (r)] = p.getPatternSeedingRowState (slot(), r);
        auto generated = generate (settings, p.getPatternSeedingRhythmStep (slot()), p.getPatternScaleRoot (slot()), p.getPatternScaleModeIndex (slot()));
        auto merged = readRows (p);
        int earliest = 16;
        for (size_t r = 0; r < 4; ++r)
        {
            const bool target = p.isPatternSeedingRowTargeted (slot(), static_cast<int> (r));
            if (target)
                merged[r].steps = generated[r].steps;
            if (allRhythm && !target)
                for (size_t step = 0; step < merged[r].steps.size(); ++step)
                {
                    const auto rhythm = step < generated[r].steps.size() ? generated[r].steps[step] : Step {};
                    merged[r].steps[step].timing = rhythm.timing;
                    merged[r].steps[step].duration = rhythm.duration;
                }
            if (target || allRhythm)
                merged[r].offset = generated[r].offset;
            if (!merged[r].steps.empty())
                earliest = std::min (earliest, merged[r].offset);
        }
        for (int r = 0; r < 4; ++r)
        {
            auto& row = merged[static_cast<size_t> (r)];
            if (allRhythm && earliest != 16)
                row.offset = juce::jlimit (0, 16, row.offset - earliest + 8);
            if (allRhythm || p.isPatternSeedingRowTargeted (slot(), r))
                writeRow (p, r, row);
        }
        controller.rows = readRows (p);
        controller.clearSelection();
        edited();
    }
    void NativeEditor::bulkOrder (int operation)
    {
        auto& random = juce::Random::getSystemRandom();
        const bool all = !controller.hasSelection();
        for (size_t r = 0; r < 4; ++r)
        {
            auto& row = controller.rows[r];
            std::vector<size_t> indices;
            std::vector<Step> steps;
            for (size_t s = 0; s < row.steps.size(); ++s)
                if (all || controller.selection[r][s])
                {
                    indices.push_back (s);
                    steps.push_back (row.steps[s]);
                }
            if (steps.empty())
                continue;
            if (operation == 0)
            {
                const bool value = !steps[0].skipped;
                for (auto& step : steps)
                    step.skipped = value;
            }
            if (operation == 1)
            {
                const bool value = !steps[0].muted;
                for (auto& step : steps)
                    step.muted = value;
            }
            if (operation == 2)
                std::rotate (steps.begin(), steps.begin() + 1, steps.end());
            if (operation == 3)
                std::reverse (steps.begin(), steps.end());
            if (operation == 4)
                for (size_t i = steps.size() - 1; i > 0; --i)
                    std::swap (steps[i], steps[static_cast<size_t> (random.nextInt (static_cast<int> (i + 1)))]);
            if (operation == 5)
                for (auto& step : steps)
                    step.note = juce::jlimit (0, 127, step.note + (random.nextInt (5) - 2) * 12);
            if (operation == 6)
                for (auto& step : steps)
                    step.timing = random.nextInt (16);
            for (size_t i = 0; i < indices.size(); ++i)
                row.steps[indices[i]] = steps[i];
            writeRow (controller.p, static_cast<int> (r), row);
        }
    }
    void NativeEditor::record (int row)
    {
        auto& p = controller.p;
        releaseKeys();
        keyboard.release();
        if (p.getPhraseRowRecording() == row)
        {
            int updated = -1;
            p.finishPhraseRowRecordingCapture (updated);
            p.setPhraseRowRecording (-1);
        }
        else
            p.setPhraseRowRecording (row, p.getPatternRecordCycleLengthPulses (slot(), row));
        controller.documentChanged();
        inspectorRow = inspectorStep = -1;
        keyboard.scrolling = false;
        grid.presentFlip (-1, -1);
        preview.editingRow = -1;
        resized();
        edited();
    }
    void NativeEditor::showInspector (int row, int step)
    {
        if (inspectorRow == row && inspectorStep == step)
            inspectorRow = inspectorStep = -1;
        else
        {
            inspectorRow = row;
            inspectorStep = step;
        }
        preview.editingRow = -1;
        keyboard.scrolling = inspectorRow >= 0;
        keyboard.rowAccent = row;
        keyboard.scaleRoot = controller.p.getPatternScaleRoot (slot());
        keyboard.scaleMode = controller.p.getPatternScaleModeIndex (slot());
        if (inspectorRow >= 0)
            keyboard.selected = currentStep().note;
        grid.inspectedRow = inspectorRow;
        grid.inspectedStep = inspectorStep;
        grid.presentFlip (inspectorRow, inspectorStep);
        resized();
        if (inspectorRow >= 0)
            keyboard.centerOn (keyboard.selected);
        repaint();
    }
    void NativeEditor::showRowEditor (int row)
    {
        inspectorRow = inspectorStep = -1;
        keyboard.scrolling = false;
        grid.presentFlip (-1, -1);
        preview.editingRow = preview.editingRow == row ? -1 : row;
        resized();
        repaint();
    }
    void NativeEditor::showSeed (int row)
    {
        seedOpen = row >= 0 || !seedOpen;
        if (row >= 0)
        {
            seedTarget = row;
            std::array<PluginProcessor::SeedingRowState, 4> settings;
            std::array<int, 4> targets {};
            for (int r = 0; r < 4; ++r)
                settings[static_cast<size_t> (r)] = controller.p.getPatternSeedingRowState (slot(), r);
            targets[static_cast<size_t> (row)] = 1;
            controller.p.setPatternSeedModeState (controller.p.getPatternSeedingRhythmStep (slot()), settings, targets);
        }
        resized();
        repaint();
    }
    void NativeEditor::updateMetadata()
    {
        auto& p = controller.p;
        p.setProjectMetadata (projectName.getText(), projectDescription.getText(), p.getProjectCreatedAt(), p.getProjectModifiedAt(), theme, p.getProjectUiScalePercent(), grid.compact);
    }
    float NativeEditor::viewThumbPosition() const
    {
        const float target = grid.compact ? 1.0f : 0.0f;
        if (!viewThumbMoving)
            return target;
        const float t = juce::jlimit (0.0f, 1.0f, static_cast<float> ((juce::Time::getMillisecondCounterHiRes() - viewThumbStart) / 150.0));
        const float eased = t * t * (3.0f - 2.0f * t);
        return viewThumbFrom + (target - viewThumbFrom) * eased;
    }
    void NativeEditor::repaintPatternSlots()
    {
        for (const auto& control : controls)
            if (control->getName().startsWith ("Select pattern") || control->getName() == "Clear selected pattern")
                control->repaint();
    }
    void NativeEditor::mouseDown (const juce::MouseEvent& e)
    {
        if (clearPattern < 0 || e.eventComponent == nullptr)
            return;
        if (e.eventComponent->getName() == "Clear selected pattern")
            return;
        clearPattern = -1;
        repaintPatternSlots();
    }
    void NativeEditor::edited()
    {
        previewDirty = true;
        controller.rows = readRows (controller.p);
        if (inspectorRow >= 0)
        {
            keyboard.selected = currentStep().note;
            keyboard.scaleRoot = controller.p.getPatternScaleRoot (slot());
            keyboard.scaleMode = controller.p.getPatternScaleModeIndex (slot());
        }
        grid.updateLayout();
        resized();
        repaint();
    }
    void NativeEditor::documentChanged()
    {
        releaseKeys();
        controller.documentChanged();
        inspectorRow = inspectorStep = -1;
        keyboard.scrolling = false;
        grid.presentFlip (-1, -1);
        preview.editingRow = -1;
        synchronise();
        edited();
        if (scaleChanged)
            scaleChanged();
    }
    void NativeEditor::applyTheme (const juce::String& mode)
    {
        theme = mode;
        palette.set (theme);
        projectName.setColour (juce::Label::textColourId, palette.text);
        projectDescription.setColour (juce::Label::textColourId, palette.text);
        projectName.setFont (font (14, Weight::semibold, 0.025f));
        projectDescription.setFont (font (12));
        repaint();
    }
    void NativeEditor::synchronise()
    {
        auto& p = controller.p;
        theme = p.getProjectThemeMode();
        grid.compact = p.getProjectStretchStepsToFit();
        projectName.setText (p.getProjectName(), juce::dontSendNotification);
        projectDescription.setText (p.getProjectDescription(), juce::dontSendNotification);
        applyTheme (theme);
        for (const auto& control : controls)
        {
            if (control->getName() == "Undo")
                control->setEnabled (controller.canUndo());
            if (control->getName() == "Redo")
                control->setEnabled (controller.canRedo());
            control->repaint();
        }
        resized();
        repaint();
    }
    void NativeEditor::resized()
    {
        const float header = static_cast<float> (top()), processing = header + 98 + 496, bottom = processing + 60;
        const float extraWidth = std::max (0.0f, getWidth() - 1690.0f);
        const int gridX = seedOpen ? 400 : 0;
        grid.setBounds (gridX, static_cast<int> (header + 98), getWidth() - gridX, 496);
        preview.setBounds (preview.editingRow >= 0 ? 232 : 24, static_cast<int> (bottom + 39), getWidth() - (preview.editingRow >= 0 ? 256 : 48), std::max (20, getHeight() - static_cast<int> (bottom + 39) - 48));
        const bool recording = controller.p.getPhraseRowRecording() >= 0;
        preview.setVisible (inspectorRow < 0 && !recording);
        keyboard.setVisible (inspectorRow >= 0 || recording);
        keyboard.setBounds (232, static_cast<int> (bottom + 98), getWidth() - 256, std::max (80, getHeight() - static_cast<int> (bottom + 98) - 60));
        for (auto& placement : placements)
        {
            auto b = placement.bounds;
            bool visible = true;
            if (placement.region == 0)
            {
                b = b.translated (b.getX() >= 1260 ? extraWidth : 0, header + 2);
            }
            if (placement.region == 1)
            {
                b = b.translated (0, processing);
            }
            if (placement.region == 2)
            {
                b = b.translated (b.getX() >= 1480 ? extraWidth : 0, getHeight() - 46.0f);
            }
            if (placement.region == 3)
            {
                b = b.translated (b.getX() > 1600 ? extraWidth : 0, bottom);
                visible = placement.bounds.getX() > 1600 ? (inspectorRow >= 0 || preview.editingRow >= 0) : (inspectorRow < 0 && preview.editingRow < 0 && ! recording);
            }
            if (placement.region == 4)
            {
                b = b.translated (extraWidth, 0);
                visible = controller.p.hasStandaloneTransport();
            }
            if (placement.region == 5)
            {
                b = b.translated (24, bottom + 4);
                visible = inspectorRow >= 0;
            }
            if (placement.region == 6)
            {
                b = b.translated (24, header + 98);
                visible = seedOpen;
            }
            if (placement.region == 7)
            {
                b = b.translated (24, bottom + 4);
                visible = recording;
            }
            if (placement.region == 8)
            {
                b = b.translated (24, bottom + 4);
                visible = preview.editingRow >= 0;
            }
            placement.control->setVisible (visible);
            placement.control->setBounds (b.toNearestInt());
        }
        projectName.setBounds (318, getHeight() - 40, 156, 32);
        projectDescription.setBounds (474, getHeight() - 40, std::max (100, getWidth() - 850), 32);
        if (scalePicker)
            scalePicker->setBounds (getLocalBounds());
    }
    void NativeEditor::paint (juce::Graphics& g)
    {
        g.fillAll (palette.app);
        const float header = static_cast<float> (top()), processing = header + 594, bottom = processing + 60;
        rail (g, { 0, header, static_cast<float> (getWidth()), 98 }, palette);
        rail (g, { 0, processing, static_cast<float> (getWidth()), 60 }, palette);
        rail (g, { 0, getHeight() - 46.0f, static_cast<float> (getWidth()), 46 }, palette);
        text (g, "OFSOUND", { 24, header + 22, 150, 14 }, palette.accent, 14, Weight::bold, juce::Justification::centredLeft, 0.1f);
        logo (g, { 24, header + 42 }, palette);
        text (g, "Operation", { 832, header + 25, 150, 16 }, palette.text, 12, true, juce::Justification::centredLeft);
        const float extra = std::max (0.0f, getWidth() - 1690.0f);
        text (g, "Patterns:", { 1254 + extra, header + 14, 58, 32 }, palette.text, 12, true, juce::Justification::centredRight);
        text (g, "Loops:", { 1254 + extra, header + 51, 58, 32 }, palette.text, 12, true, juce::Justification::centredRight);
        for (const auto& placement : placements)
        {
            if (!placement.control->isVisible() || placement.label.isEmpty())
                continue;
            auto b = placement.control->getBounds().toFloat();
            if (placement.region == 1)
                b = b.withY (b.getBottom() + 2).withHeight (12);
            else if (placement.region == 4)
                b = b.withX (b.getX() - 35).withWidth (31);
            else
                b = b.withY (b.getY() - 18).withHeight (16).withWidth (b.getWidth() + 10);
            const bool transportLabel = placement.region == 4;
            text (g, placement.label, b, transportLabel ? palette.muted : palette.text, placement.region == 1 ? 9.0f : 12.0f, false, placement.region == 1 || transportLabel ? juce::Justification::centred : juce::Justification::centredLeft);
        }
        const float foot = getHeight() - 40.0f;
        const float nextX = 1483.0f + extra;
        const auto meta = Rect (312, foot, nextX - 8.0f - 312.0f, 32);
        g.setColour (palette.surface.withAlpha (0.80f));
        g.fillRoundedRectangle (meta, 4);
        g.setColour (palette.borderSubtle);
        g.drawRoundedRectangle (meta.reduced (0.5f), 4, 1);
        if (projectDescription.getText().isEmpty())
            text (g, juce::String::fromUTF8 ("Add a project description…"), projectDescription.getBounds().toFloat().reduced (5, 0), palette.faint, 12, false, juce::Justification::centredLeft);
        auto stamp = controller.p.getProjectModifiedAt();
        if (stamp.isEmpty())
            stamp = controller.p.getProjectCreatedAt();
        juce::String dateLabel = "UNSAVED";
        if (stamp.isNotEmpty())
        {
            const auto when = juce::Time::fromISO8601 (stamp);
            if (when.toMilliseconds() > 0)
                dateLabel = when.formatted ("%b %e, %Y").toUpperCase().replace ("  ", " ");
        }
        text (g, dateLabel, { nextX - 128.0f, foot, 112, 32 }, palette.text, 10.4f, true, juce::Justification::centredRight, 0.14f);
        if (inspectorRow < 0)
            text (g, "loop " + number (controller.p.getLoopBraceStartQuarters()) + juce::String::fromUTF8 ("–") + number (controller.p.getLoopBraceEndQuarters()) + " (" + number (controller.p.getLoopBraceEndQuarters() - controller.p.getLoopBraceStartQuarters()) + " beats)", { getWidth() - 250.0f, bottom + 8, 208, 21 }, palette.secondary, 12, true, juce::Justification::centredRight);
        if (controller.p.getPhraseRowRecording() >= 0)
            text (g, "Recording row " + juce::String (controller.p.getPhraseRowRecording() + 1) + juce::String::fromUTF8 (" · Play MIDI or the computer keyboard"), { 24, bottom + 70, 800, 30 }, palette.danger, 18, true, juce::Justification::centredLeft);
        if (inspectorRow >= 0)
            text (g, "Cycle", { 694, bottom + 24, 300, 16 }, palette.text, 12);
        if (copyPattern >= 0)
            text (g, "Copy pattern " + juce::String (copyPattern + 1) + ": choose destination", { 880, header + 78, 350, 16 }, palette.accent, 12);
    }
    void NativeEditor::timerCallback()
    {
        auto& p = controller.p;
        int recorded = -1;
        const bool recordingUpdate = p.drainPhraseRowRecordingUpdates (recorded);
        const auto captured = p.drainPhraseRowRecordedNotes();
        if (recordingUpdate || !captured.isEmpty())
        {
            controller.documentChanged();
            previewDirty = true;
        }
        if (knownDocument != p.getDocumentGeneration())
        {
            knownDocument = p.getDocumentGeneration();
            releaseKeys();
            keyboard.release();
            scalePicker.reset();
            inspectorRow = inspectorStep = -1;
            keyboard.scrolling = false;
            grid.presentFlip (-1, -1);
            preview.editingRow = -1;
            controller.refresh();
            synchronise();
            if (scaleChanged)
                scaleChanged();
            previewDirty = true;
        }
        if (knownPattern != slot())
        {
            knownPattern = slot();
            controller.clearSelection();
            inspectorRow = inspectorStep = -1;
            keyboard.scrolling = false;
            grid.presentFlip (-1, -1);
            preview.editingRow = -1;
            synchronise();
            previewDirty = true;
        }
        if (controller.refresh())
        {
            grid.updateLayout();
            previewDirty = true;
        }
        if (previewDirty && !controller.gestureActive())
        {
            previewDirty = false;
            preview.refresh();
            synchronise();
        }
        if (copyPattern >= 0 || clearPattern >= 0)
            repaintPatternSlots();
        if (viewThumbMoving)
        {
            if (juce::Time::getMillisecondCounterHiRes() - viewThumbStart >= 150.0)
                viewThumbMoving = false;
            for (const auto& control : controls)
                if (control->getName() == "Toggle compact steps")
                    control->repaint();
        }
        if (grid.flipsActive())
            grid.repaint();
        const bool playing = p.getPlaybackBeat() >= 0;
        if (playing || wasPlaying)
        {
            preview.repaint();
            grid.repaint();
        }
        wasPlaying = playing;
        keyStateChanged (false);
    }
    void NativeEditor::releaseKeys()
    {
        for (auto& note : heldKeys)
            if (note >= 0)
            {
                controller.p.capturePhraseRowRecordedNoteOff (note);
                controller.p.playStandaloneKeyboardNoteOff (note);
                note = -1;
            }
    }
    bool NativeEditor::keyPressed (const juce::KeyPress& key)
    {
        if (dynamic_cast<juce::TextEditor*> (juce::Component::getCurrentlyFocusedComponent()) != nullptr)
            return false;
        const int code = key.getKeyCode();
        auto& p = controller.p;
        if (key == juce::KeyPress::spaceKey && p.hasStandaloneTransport())
        {
            p.setStandaloneTransportPlaying (!p.isStandaloneTransportPlaying());
            synchronise();
            return true;
        }
        if (key.getModifiers().isCommandDown() && code == 'Z')
        {
            controller.cancel();
            if (key.getModifiers().isShiftDown())
                controller.redo();
            else
                controller.undo();
            edited();
            return true;
        }
        if (key == juce::KeyPress::escapeKey)
        {
            grid.cancelGesture();
            scalePicker.reset();
            inspectorRow = inspectorStep = -1;
            preview.editingRow = -1;
            copyPattern = -1;
            clearPattern = -1;
            releaseKeys();
            resized();
            repaint();
            return true;
        }
        if (key.getModifiers().isCommandDown() || key.getModifiers().isAltDown())
            return false;
        if (code == 'Z' || code == 'X')
        {
            keyboardOctave = juce::jlimit (-6, 4, keyboardOctave + (code == 'Z' ? -1 : 1));
            return true;
        }
        static constexpr const char* keys = "AWSEDFTGYHUJKOLP;";
        if (!p.hasStandaloneTransport() && p.getPhraseRowRecording() < 0)
            return false;
        for (int i = 0; i < 17; ++i)
            if (code == keys[i])
            {
                const int note = 60 + keyboardOctave * 12 + i;
                if (note < 0 || note > 127)
                    return true;
                if (heldKeys[static_cast<size_t> (i)] < 0)
                {
                    heldKeys[static_cast<size_t> (i)] = note;
                    if (p.getPhraseRowRecording() >= 0)
                        p.capturePhraseRowRecordedNoteOn (note, 100);
                    else
                        p.playStandaloneKeyboardNoteOn (note, 100);
                }
                return true;
            }
        return false;
    }
    bool NativeEditor::keyStateChanged (bool)
    {
        static constexpr const char* keys = "AWSEDFTGYHUJKOLP;";
        for (size_t i = 0; i < heldKeys.size(); ++i)
            if (heldKeys[i] >= 0 && !juce::KeyPress::isKeyCurrentlyDown (keys[i]))
            {
                controller.p.capturePhraseRowRecordedNoteOff (heldKeys[i]);
                controller.p.playStandaloneKeyboardNoteOff (heldKeys[i]);
                heldKeys[i] = -1;
            }
        return false;
    }
    void NativeEditor::focusLost (FocusChangeType)
    {
        if (!hasKeyboardFocus (true))
        {
            releaseKeys();
            keyboard.release();
        }
    }

    namespace
    {
        class ScalePicker : public juce::Component
        {
        public:
            ScalePicker (Palette& palette, PluginProcessor& processor, juce::Image background, std::function<void()> complete) : p (palette), engine (processor), done (std::move (complete)), backdrop (std::move (background)), scaleKeyboard (palette)
            {
                if (backdrop.isValid())
                    backdrop.getPixelData()->applyGaussianBlurEffect (14);
                addAndMakeVisible (scaleKeyboard);
                scaleKeyboard.setInterceptsMouseClicks (false, false);
                scaleKeyboard.showScale = true;
                generation = engine.getDocumentGeneration();
                const int slot = engine.getViewPatternSlot();
                root = engine.getPatternScaleRoot (slot);
                mode = engine.getPatternScaleModeIndex (slot);
                for (int i = 0; i < 29; ++i)
                {
                    auto item = std::make_unique<Control> (p, i < 12 ? "Root " + noteName (60 + i) : i < 28 ? scaleNames[i - 12]
                                                                                                            : "Done");
                    item->caption = i < 12 ? noteName (60 + i).dropLastCharacters (1) : i < 28 ? scaleNames[i - 12]
                                                                                               : "Done";
                    item->active = [this, i] { return i < 12 ? root == i : i < 28 && mode == i - 12; };
                    item->action = [this, i] (auto) {
                        if (generation != engine.getDocumentGeneration())
                            return;
                        if (i == 28)
                        {
                            if (done)
                                juce::MessageManager::callAsync (done);
                            return;
                        }
                        if (i < 12)
                            root = i;
                        else
                            mode = i - 12;
                        engine.setPatternScale (root, mode);
                        updateKeyboard();
                        repaint();
                        for (auto& control : controls)
                            control->repaint();
                    };
                    item->customPaint = [this, i] (juce::Graphics& g, Rect b) {
                        const bool selected = i < 12 ? root == i : i < 28 && mode == i - 12;
                        g.setColour (selected ? p.accent : p.surface);
                        g.fillRect (b);
                        g.setColour (selected ? p.accent : p.border);
                        g.drawRect (b, 1);
                        text (g, controls[static_cast<size_t> (i)]->caption, b.reduced (i < 12 || i == 28 ? 0.0f : 12.0f, 0), selected ? p.app : p.text, 14, true, i < 12 || i == 28 ? juce::Justification::centred : juce::Justification::centredLeft);
                    };
                    if (i == 28)
                        item->caption = juce::String::fromUTF8 ("×");
                    addAndMakeVisible (*item);
                    controls.push_back (std::move (item));
                }
            }
            void resized() override
            {
                panel = getLocalBounds().toFloat().withSizeKeepingCentre (928, 660);
                scaleKeyboard.setBounds (panel.reduced (24).withTop (panel.getY() + 476).toNearestInt());
                updateKeyboard();
                for (int i = 0; i < 12; ++i)
                    controls[static_cast<size_t> (i)]->setBounds (juce::Rectangle<float> (panel.getX() + 24 + static_cast<float> (i) * 74, panel.getY() + 112, 66, 44).toNearestInt());
                for (int i = 0; i < 16; ++i)
                    controls[static_cast<size_t> (i + 12)]->setBounds (juce::Rectangle<float> (panel.getX() + 24 + static_cast<float> (i % 4) * 222, panel.getY() + 240 + static_cast<float> (i / 4) * 56, 214, 48).toNearestInt());
                controls.back()->setBounds (juce::Rectangle<float> (panel.getRight() - 56, panel.getY() + 16, 32, 32).toNearestInt());
            }
            void paint (juce::Graphics& g) override
            {
                if (backdrop.isValid())
                    g.drawImage (backdrop, getLocalBounds().toFloat());
                g.fillAll (p.app.withAlpha (.45f));
                g.setColour (p.app.withAlpha (.88f));
                g.fillRect (panel);
                g.setColour (p.border);
                g.drawRect (panel, 1);
                text (g, "PATTERN " + juce::String (engine.getViewPatternSlot() + 1), panel.reduced (24).withHeight (16), p.muted, 12, true, juce::Justification::centredLeft);
                text (g, "Scale Mode", panel.reduced (24).withY (panel.getY() + 44).withHeight (32), p.text, 24, true, juce::Justification::centredLeft);
                g.setColour (p.border);
                g.drawHorizontalLine (juce::roundToInt (panel.getY() + 88), panel.getX(), panel.getRight());
                text (g, "MODE", panel.reduced (24).withY (panel.getY() + 180).withHeight (16), p.muted, 12, true, juce::Justification::centredLeft);
                text (g, scaleNames[mode], panel.reduced (24).withY (panel.getY() + 200).withHeight (24), p.text, 18, true, juce::Justification::centredLeft);
            }

        private:
            Palette& p;
            PluginProcessor& engine;
            std::function<void()> done;
            int root = 0, mode = 0;
            Rect panel;
            uint64_t generation = 0;
            juce::Image backdrop;
            Keyboard scaleKeyboard;
            void updateKeyboard()
            {
                scaleKeyboard.low = 60 + root;
                scaleKeyboard.high = 60 + root + 35;
                scaleKeyboard.scaleRoot = root;
                scaleKeyboard.scaleMode = mode;
                scaleKeyboard.repaint();
            }
            std::vector<std::unique_ptr<Control>> controls;
        };
    }
    void NativeEditor::openScale()
    {
        controller.commit();
        controller.begin();
        const juce::Component::SafePointer<NativeEditor> safe (this);
        scalePicker = std::make_unique<ScalePicker> (palette, controller.p, createComponentSnapshot (getLocalBounds()), [safe] {
            if (safe == nullptr)
                return;
            safe->controller.commit();
            safe->scalePicker.reset();
            safe->edited();
        });
        addAndMakeVisible (*scalePicker);
        scalePicker->setBounds (getLocalBounds());
        scalePicker->toFront (true);
    }
}
