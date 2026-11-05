#include "PhrasesGrid.h"
namespace phrases::ui
{
    namespace
    {
        Rect compactDurationArea (Rect card)
        {
            return card.reduced (std::min (10.0f, card.getWidth() * .25f), 5.0f);
        }

        float durationEdge (Rect area, const Step& step)
        {
            return area.getX() + area.getWidth() * static_cast<float> (step.duration);
        }
    }

    Grid::Grid (Controller& controller, Palette& palette) : c (controller), p (palette)
    {
        setWantsKeyboardFocus (true);
        setTitle ("Phrase grid");
        icons = { icon (RowDisableIcon), icon (RowEditPencilIcon), icon (SaplingIcon), icon (StepGearIcon), icon (StepMuteIcon), icon (StepSkipIcon) };
    }
    void Grid::drawIcon (juce::Graphics& g, size_t index, Rect b, juce::Colour ink)
    {
        if (auto* d = icons[index].get())
        {
            d->replaceColour (iconInks[index], ink); iconInks[index] = ink;
            d->drawWithin (g, b, juce::RectanglePlacement::centred, 1);
        }
    }
    void Grid::resized() { updateLayout(); }
    void Grid::setVelocityRow (int row)
    {
        if (row >= 0 && (row >= static_cast<int> (c.rows.size()) || c.rows[static_cast<size_t> (row)].steps.empty()))
            row = -1;
        if (velocityRow == row)
            return;
        velocityRow = row;
        updateLayout();
    }
    void Grid::updateLayout()
    {
        int minimumOffset = 8;
        float widest = 0;
        for (const auto& row : c.rows)
            minimumOffset = std::min (minimumOffset, row.offset);
        for (const auto& row : c.rows)
        {
            float columns = static_cast<float> (row.offset - minimumOffset);
            for (const auto& step : row.steps)
                columns += static_cast<float> (step.timing + 1);
            widest = std::max (widest, columns);
        }
        const float fit = widest > 0 ? (getWidth() - 48.0f - 127.0f - 8.0f - 4.0f - 70.0f) / (widest * 76.5f - 8.0f + 16.0f / 3.0f) : 1;
        unit = std::max (1.0f, 76.5f * fit);
        for (size_t r = 0; r < 4; ++r)
        {
            cells[r].clear();
            float x = 135 + static_cast<float> (c.rows[r].offset - minimumOffset) * unit - scroll[r];
            const float y = 28 + static_cast<float> (r) * 114;
            const auto& steps = c.rows[r].steps;
            int low = 127, high = 0;
            if (compact)
                for (const auto& step : steps)
                {
                    low = std::min (low, step.note);
                    high = std::max (high, step.note);
                }
            const float pitchCentre = (low + high) * .5f;
            const float pitchScale = 60.0f / std::max (6, high - low);
            for (const auto& step : steps)
            {
                const float width = static_cast<float> (step.timing + 1) * unit;
                const float gap = std::min (16.0f, width * .3f);
                const bool pitchCard = compact && static_cast<int> (r) != velocityRow;
                const float top = pitchCard ? y + 30.0f + (pitchCentre - step.note) * pitchScale : y;
                cells[r].push_back ({ x + gap * .5f, top, std::max (2.0f, width - gap), pitchCard ? 36.0f : 96.0f });
                x += width;
            }
            addBounds[r] = { x + 4, y + 20, 54, 54 };
        }
        repaint();
    }
    Rect Grid::stepBounds (int r, int s) const
    {
        if (r < 0 || r > 3 || s < 0 || s >= static_cast<int> (cells[static_cast<size_t> (r)].size()))
            return {};
        return cells[static_cast<size_t> (r)][static_cast<size_t> (s)];
    }
    void Grid::paint (juce::Graphics& g)
    {
        g.fillAll (p.workspace);
        for (size_t r = 0; r < 4; ++r)
        {
            const auto& row = c.rows[r];
            const float y = 24 + static_cast<float> (r) * 114;
            const auto accent = c.p.isRowColorsEnabled() ? p.rows[r] : p.rows[0];
            g.setColour (p.app.interpolatedWith (p.surface, .55f));
            g.fillRect (Rect (0, y, 123, 104));
            if (hover.row == static_cast<int> (r) && hover.step < 0 && hover.part != Part::none && hover.part != Part::add)
            {
                g.setColour (accent.withAlpha (0.07f));
                g.fillRect (Rect (0, y, 123, 104));
            }
            g.setColour (p.border.withAlpha (.5f));
            g.drawLine (123, y, 123, y + 104);
            const auto headerHot = [&] (Part part) { return hover.row == static_cast<int> (r) && hover.part == part; };
            drawIcon (g, 0, { 23, y + 22, 35, 35 }, headerHot (Part::power) ? accent.brighter (0.15f) : (row.muted ? p.faint : accent));
            const auto badge = Rect (66, y + 24, 28, 28);
            g.setColour (p.surface);
            g.fillEllipse (badge);
            g.setColour (row.muted ? p.borderSubtle : p.borderStrong);
            g.drawEllipse (badge.reduced (0.75f), 1.25f);
            const auto angle = static_cast<float> (row.offset - 8) / 8 * 2.4f;
            g.setColour (row.muted ? p.subtle : accent);
            g.fillEllipse (badge.getCentreX() + std::sin (angle) * 13 - 2, badge.getCentreY() - std::cos (angle) * 13 - 2, 4, 4);
            auto offsetLabel = juce::String ((row.offset - 8) * .25, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
            if (offsetLabel.startsWith ("0."))
                offsetLabel = offsetLabel.substring (1);
            text (g, offsetLabel, badge, row.muted ? p.muted : p.text, 9, true);
            if (row.muted)
            {
                g.setColour (p.app.withAlpha (0.50f));
                g.fillEllipse (badge);
            }
            g.setColour (c.p.getPhraseRowRecording() == static_cast<int> (r) || headerHot (Part::record) ? p.danger : p.faint);
            g.fillEllipse (26, y + 74, 15, 15);
            const bool drawingVelocity = static_cast<int> (r) == velocityRow;
            const Rect velocityButton (49, y + 69, 26, 26);
            if (drawingVelocity)
            {
                g.setColour (accent.withAlpha (0.23f));
                g.fillRoundedRectangle (velocityButton, 4);
            }
            const auto velocityInk = drawingVelocity || headerHot (Part::rowVelocity) ? accent : p.faint;
            text (g, "V", velocityButton, velocityInk.withMultipliedAlpha (row.steps.empty() ? .45f : 1.0f), 15, true);
            drawIcon (g, 2, { 80, y + 74, 14, 14 }, headerHot (Part::seed) ? accent : p.faint);
            juce::Graphics::ScopedSaveState clip (g);
            g.reduceClipRegion (juce::Rectangle<int> (128, static_cast<int> (y), getWidth() - 128, 104));
            for (size_t s = 0; s < cells[r].size(); ++s)
            {
                const auto b = cells[r][s];
                const auto& step = row.steps[s];
                const bool selected = c.selection[r][s];
                g.setColour (p.surface.interpolatedWith (p.app, .12f));
                g.fillRoundedRectangle (b, 8);
                if (drawingVelocity)
                {
                    const auto area = b.reduced (2);
                    const auto level = area.getBottom() - area.getHeight() * step.velocity / 127.0f;
                    const auto bar = area.withTop (level);
                    g.setColour (accent.withAlpha (.42f));
                    g.fillRoundedRectangle (bar, 5);
                    g.setColour (accent);
                    g.drawLine (area.getX(), level, area.getRight(), level, 1.5f);
                    if (b.getWidth() >= 24)
                        text (g, juce::String (step.velocity), b.withHeight (22), p.text, 12, true);
                }
                else if (compact)
                {
                    g.setColour (accent.withAlpha (step.skipped ? .12f : .22f + .78f * step.velocity / 127.0f));
                    g.fillRoundedRectangle (b.reduced (1), 6);
                    if (!step.muted)
                    {
                        const auto area = compactDurationArea (b);
                        const float edge = durationEdge (area, step);
                        g.setColour (p.app.withAlpha (.52f));
                        g.fillRoundedRectangle (area, 3);
                        g.setColour (accent.brighter (.2f).withAlpha (.35f + .6f * step.velocity / 127.0f));
                        g.fillRoundedRectangle (area.withRight (edge), 3);
                        const bool active = (hover.part == Part::duration && hover.row == static_cast<int> (r) && hover.step == static_cast<int> (s))
                            || (dragging && down.part == Part::duration && down.row == static_cast<int> (r) && down.step == static_cast<int> (s));
                        const auto grip = Rect (edge - 3, area.getY() + 1, 6, area.getHeight() - 2)
                                              .getIntersection (b.reduced (1));
                        g.setColour (p.app.withAlpha (active ? .9f : .65f));
                        g.fillRoundedRectangle (grip, 2);
                        g.setColour (active ? accent : p.text.withAlpha (.8f));
                        for (int dot = -2; dot <= 2; ++dot)
                            g.fillEllipse (grip.getCentreX() - .8f, grip.getCentreY() + static_cast<float> (dot) * 4.0f - .8f, 1.6f, 1.6f);
                    }
                    if (b.getWidth() > 22)
                        text (g, noteName (step.note), b.reduced (3), p.text, 14, true);
                }
                else
                {
                    juce::Graphics::ScopedSaveState card (g);
                    juce::Path shell;
                    shell.addRoundedRectangle (b.reduced (2), 6);
                    g.reduceClipRegion (shell);
                    const auto header = b.withHeight (23);
                    g.setColour (p.text.withAlpha (.10f));
                    g.fillRect (header);
                    text (g, juce::String::fromUTF8 ("×"), header.withWidth (20), hover.row == static_cast<int> (r) && hover.step == static_cast<int> (s) && hover.part == Part::remove ? p.danger : p.secondary, 22, true);
                    auto lengthText = juce::String ((step.timing + 1) * .25, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
                    if (lengthText.startsWith ("0."))
                        lengthText = lengthText.substring (1);
                    text (g, lengthText, header.reduced (6, 0), p.muted, 12, true, juce::Justification::centredRight);
                    const auto bar = Rect (b.getX() + 2, b.getY() + 23, b.getWidth() - 4, 17);
                    g.setGradientFill (juce::ColourGradient (p.subtle, bar.getTopLeft(), p.raised, bar.getBottomLeft(), false));
                    g.fillRect (bar);
                    if (!step.muted)
                    {
                        g.setGradientFill (juce::ColourGradient (accent.brighter (.35f).withAlpha (.2f + .8f * step.velocity / 127.0f), bar.getTopLeft(), accent.withAlpha (.2f + .8f * step.velocity / 127.0f), bar.getBottomLeft(), false));
                        g.fillRect (bar.withWidth (bar.getWidth() * static_cast<float> (step.duration)));
                    }
                    text (g, noteName (step.note), { b.getX() + 6, b.getY() + 42, 30, 22 }, p.text, 14, true, juce::Justification::centredLeft);
                    text (g, juce::String (step.velocity), { b.getX() + 38, b.getY() + 44, 30, 20 }, p.text, 11, true, juce::Justification::centredLeft);
                    if (b.getWidth() >= 28)
                    {
                        const float chanceWidth = std::min (32.0f, b.getWidth() - 12);
                        text (g, juce::String (step.probability) + "%", { b.getX() + 6, b.getY() + 63, chanceWidth, 13 },
                              accent, 10, true, juce::Justification::centredLeft);
                        const float pipWidth = std::min (8.0f, (b.getWidth() - chanceWidth - 24.0f) / 8.0f);
                        if (pipWidth >= 1.0f)
                            for (int i = 0; i < 8; ++i)
                            {
                                const bool inside = i < step.cycle;
                                const bool on = inside && ((step.mask == 0 ? 1 : step.mask) & (1 << i)) != 0;
                                g.setColour (on ? accent : inside ? p.subtle : p.app);
                                g.fillRect (Rect (b.getX() + chanceWidth + 10.0f + static_cast<float> (i) * (pipWidth + 1.0f),
                                                  b.getY() + 67, pipWidth, 6));
                            }
                    }
                    drawIcon (g, 5, { b.getX() + 2, b.getBottom() - 20, 18, 18 }, step.skipped ? accent : p.muted);
                    drawIcon (g, 4, { b.getX() + 23, b.getBottom() - 20, 18, 18 }, step.muted ? accent : p.muted);
                    drawIcon (g, 3, { b.getRight() - 20, b.getBottom() - 20, 16, 16 }, inspectedRow == static_cast<int> (r) && inspectedStep == static_cast<int> (s) ? accent : p.muted);
                }
                if (row.muted || step.skipped)
                {
                    g.setColour (p.app.withAlpha (.5f));
                    g.fillRoundedRectangle (b, 8);
                }
                const bool playing = stepActive (row, s, c.p.getPlaybackBeat(), c.p.getPulseIndex(), c.p.getSwingPercent(), c.p.getSwingSubdivisionIndex());
                if (playing)
                {
                    g.setColour (accent.withAlpha (.16f));
                    g.fillRoundedRectangle (b, 8);
                }
                const bool inspected = inspectedRow == static_cast<int> (r) && inspectedStep == static_cast<int> (s);
                if (inspectedRow >= 0 && !inspected)
                {
                    g.setColour (p.app.withAlpha (0.45f));
                    g.fillRoundedRectangle (b, 8);
                }
                g.setColour (inspected || selected || playing ? accent : p.border);
                g.drawRoundedRectangle (b.reduced (1), 8, inspected ? 2.5f : 2);
                if (inspected)
                {
                    g.setColour (accent.withAlpha (0.35f));
                    g.drawRoundedRectangle (b.expanded (2), 10, 3);
                }
                if (step.muted)
                {
                    g.setColour (p.faint.withAlpha (.45f));
                    g.drawLine (b.getX() + 2, b.getBottom() - 3, b.getRight() - 2, b.getY() + 3, 1);
                }
            }
            if (hover.row == static_cast<int> (r) && hover.part == Part::resize)
            {
                g.setColour (accent);
                const auto drawHandle = [&] (Rect card, float edge) {
                    g.fillRoundedRectangle (edge - 2.0f, card.getCentreY() - 14.0f, 4.0f, 28.0f, 2.0f);
                };
                if (hover.step >= 0 && hover.step < static_cast<int> (cells[r].size()))
                {
                    const auto card = cells[r][static_cast<size_t> (hover.step)];
                    drawHandle (card, card.getRight());
                }
                const int next = hover.step + 1;
                if (next >= 0 && next < static_cast<int> (cells[r].size()))
                {
                    const auto card = cells[r][static_cast<size_t> (next)];
                    drawHandle (card, card.getX());
                }
            }
            if (row.steps.size() < 64)
                text (g, "+", addBounds[r], row.muted ? p.faint : accent, 32, true);
        }
        if (marquee && dragging)
        {
            const auto box = Rect (origin, pointer);
            g.setColour (p.accent.withAlpha (.1f));
            g.fillRect (box);
            g.setColour (p.accent);
            g.drawRect (box, 1);
        }
        if (dragging && down.part == Part::move)
        {
            g.setColour (p.accent);
            g.drawVerticalLine (juce::roundToInt (pointer.x), pointer.y - 30, pointer.y + 30);
        }
    }
    Grid::Hit Grid::hit (juce::Point<float> point, bool selecting)
    {
        for (int r = 0; r < 4; ++r)
        {
            const float y = 24 + static_cast<float> (r) * 114;
            if (point.y < y || point.y >= y + 104)
                continue;
            if (point.x < 123)
            {
                if (point.y < y + 16 || point.x < 15 || point.x > 110)
                    return { Part::header, r, -1 };
                if (point.y >= y + 68)
                    return { point.x < 47 ? Part::record : point.x < 75 ? Part::rowVelocity
                                                                        : Part::seed,
                        r,
                        -1 };
                return { point.x < 61 ? Part::power : Part::offset, r, -1 };
            }
            const auto index = static_cast<size_t> (r);
            if (selecting)
            {
                for (size_t s = 0; s < cells[index].size(); ++s)
                    if (cells[index][s].contains (point))
                        return { Part::none, r, static_cast<int> (s) };
                return { Part::none, r, -1 };
            }
            if (addBounds[index].contains (point))
                return { Part::add, r, static_cast<int> (cells[index].size()) };
            if (r == velocityRow && !cells[index].empty())
            {
                if (point.x >= 128 && cells[index].front().getUnion (cells[index].back()).contains (point))
                    return { Part::drawVelocity, r, -1 };
                return { Part::none, r, -1 };
            }
            for (size_t s = 0; s < cells[index].size(); ++s)
            {
                const auto b = cells[index][s];
                if (!b.expanded (compact ? 3.0f : 8.0f, 0).contains (point))
                    continue;
                const int step = static_cast<int> (s);
                if (std::abs (point.x - b.getX()) < (compact ? 4 : 9))
                    return { Part::resize, r, step - 1 };
                if (std::abs (point.x - b.getRight()) < (compact ? 4 : 9))
                    return { Part::resize, r, step };
                if (compact && s < c.rows[index].steps.size() && b.contains (point))
                {
                    const auto area = compactDurationArea (b);
                    const auto& value = c.rows[index].steps[s];
                    if (!value.muted && area.expanded (4, 3).contains (point)
                        && std::abs (point.x - durationEdge (area, value)) <= 4)
                        return { Part::duration, r, step };
                }
                if (compact)
                    return { Part::move, r, step };
                const auto dy = point.y - b.getY(), dx = point.x - b.getX();
                if (dy < 23)
                    return { dx < 20 ? Part::remove : Part::move, r, step };
                if (dy < 40)
                    return { Part::duration, r, step };
                if (dy >= 64 && dy < 76)
                    return { Part::gear, r, step };
                if (dy >= 76)
                    return { point.x >= b.getRight() - 22 ? Part::gear : dx < 24 ? Part::skip
                                                                     : dx < 45   ? Part::mute
                                                                                 : Part::move,
                        r,
                        step };
                return { dx < 38 ? Part::note : dx < 72 ? Part::velocity
                                                        : Part::move,
                    r,
                    step };
            }
            return { Part::none, r, -1 };
        }
        return {};
    }
    void Grid::mouseDown (const juce::MouseEvent& e)
    {
        if (e.getNumberOfClicks() == 1)
            changedViewOnClick = false;
        if (c.refresh())
            updateLayout();
        downGeneration = c.p.getDocumentGeneration();
        downPattern = c.p.getViewPatternSlot();
        if (isShowing())
            grabKeyboardFocus();
        additiveMarquee = e.mods.isShiftDown() && e.position.x >= 123;
        down = hit (e.position, additiveMarquee);
        origin = pointer = e.position;
        dragging = compactEdited = false;
        lastVelocityPoint.reset();
        baseline = readRows (c.p);
        c.rows = baseline;
        selectionBefore = c.selection;
        marquee = down.part == Part::none;
        if (down.step >= 0 && down.part != Part::add)
        {
            const auto r = static_cast<size_t> (down.row), s = static_cast<size_t> (down.step);
            if (e.mods.isShiftDown() || e.mods.isCommandDown())
                c.selection[r].flip (s);
            else if (!c.selection[r][s])
            {
                c.clearSelection();
                c.selection[r].set (s);
            }
        }
        else if (marquee && !e.mods.isShiftDown())
            c.clearSelection();
        if (!marquee && c.begin() && down.part == Part::drawVelocity)
        {
            lastVelocityPoint = pointer;
            drawVelocityStroke (pointer);
        }
        repaint();
    }
    void Grid::drawVelocityStroke (juce::Point<float> point)
    {
        if (!lastVelocityPoint || !c.gestureActive() || down.row != velocityRow)
            return;

        const auto previous = *lastVelocityPoint;
        lastVelocityPoint = point;
        const float left = std::min (previous.x, point.x), right = std::max (previous.x, point.x);
        const float dx = point.x - previous.x;
        const auto index = static_cast<size_t> (down.row);
        auto& steps = c.rows[index].steps;
        for (size_t s = 0; s < std::min (cells[index].size(), steps.size()); ++s)
        {
            const auto card = cells[index][s];
            if (card.getRight() < std::max (left, 128.0f) || card.getX() > std::min (right, static_cast<float> (getWidth())))
                continue;

            const float x = juce::jlimit (left, right, card.getCentreX());
            const float t = std::abs (dx) < .001f ? 1.0f : (x - previous.x) / dx;
            const float y = previous.y + (point.y - previous.y) * t;
            const auto track = card.reduced (2);
            const int velocity = juce::jlimit (0, 127, juce::roundToInt ((track.getBottom() - y) * 127.0f / track.getHeight()));
            if (steps[s].velocity != velocity)
            {
                c.p.setPhraseStepVelocity (down.row, static_cast<int> (s), velocity);
                steps[s].velocity = velocity;
            }
        }
        repaint();
    }
    void Grid::mouseDrag (const juce::MouseEvent& e)
    {
        pointer = e.position;
        if (down.part == Part::drawVelocity)
        {
            drawVelocityStroke (pointer);
            return;
        }
        dragging = dragging || e.getDistanceFromDragStart() > 3;
        if (!dragging)
            return;
        if (marquee)
        {
            c.selection = additiveMarquee || e.mods.isShiftDown() ? selectionBefore : Selection {};
            const auto area = Rect (origin, pointer);
            for (size_t r = 0; r < 4; ++r)
                for (size_t s = 0; s < cells[r].size(); ++s)
                    if (area.intersects (cells[r][s]))
                        c.selection[r].set (s);
            repaint();
            return;
        }
        if (down.part == Part::move || down.part == Part::add)
        {
            if (compact && down.part == Part::move && c.gestureActive() && down.step >= 0)
            {
                const float dx = pointer.x - origin.x, dy = origin.y - pointer.y;
                compactEdited = false;
                c.rows = baseline;
                if (std::abs (dy) >= std::abs (dx))
                {
                    const int degrees = juce::roundToInt (dy / 8);
                    c.editSelected ([&] (Step& s) { s.note = e.mods.isShiftDown() ? juce::jlimit (0, 127, s.note + degrees * 12) : scaleNote (s.note, degrees, c.p.getPatternScaleRoot (c.p.getViewPatternSlot()), c.p.getPatternScaleModeIndex (c.p.getViewPatternSlot())); });
                    compactEdited = true;
                }
                else if (down.step > 0 && !e.mods.isAltDown() && std::abs (dy) < 80)
                {
                    auto& previous = c.rows[static_cast<size_t> (down.row)].steps[static_cast<size_t> (down.step - 1)];
                    const int next = previous.timing + juce::roundToInt (dx / unit);
                    if (dx > -(previous.timing + 1) * unit && next <= 15)
                    {
                        previous.timing = std::max (0, next);
                        compactEdited = true;
                    }
                    for (int r = 0; r < 4; ++r)
                        writeRow (c.p, r, c.rows[static_cast<size_t> (r)]);
                }
                updateLayout();
                if (changed)
                    changed();
            }
            repaint();
            return;
        }
        if (!c.gestureActive())
            return;
        const int delta = juce::roundToInt ((origin.y - pointer.y) / 4);
        c.rows = baseline;
        if (down.part == Part::offset)
        {
            auto& row = c.rows[static_cast<size_t> (down.row)];
            row.offset = juce::jlimit (0, 16, row.offset + delta);
            c.p.setPhraseRowTimingOffset (down.row, row.offset);
            updateLayout();
            return;
        }
        if (down.step < 0)
            return;
        const auto b = stepBounds (down.row, down.step);
        const int pattern = c.p.getViewPatternSlot(), root = c.p.getPatternScaleRoot (pattern), mode = c.p.getPatternScaleModeIndex (pattern);
        c.editSelected ([&] (Step& step) {
            if (down.part == Part::note)
                step.note = e.mods.isShiftDown() ? juce::jlimit (0, 127, step.note + delta * 12) : scaleNote (step.note, delta, root, mode);
            else if (down.part == Part::velocity)
                step.velocity = juce::jlimit (0, 127, step.velocity + delta);
            else if (down.part == Part::duration)
            {
                const float width = compact ? compactDurationArea (b).getWidth() : b.getWidth();
                step.duration = juce::jlimit (0.0, 1.0, step.duration + (pointer.x - origin.x) / std::max (1.0f, width));
                if (e.mods.isShiftDown())
                    step.duration = std::round (step.duration * 4) / 4;
            }
            else if (down.part == Part::resize)
                step.timing = juce::jlimit (0, 15, step.timing + juce::roundToInt ((pointer.x - origin.x) / unit));
        });
        updateLayout();
        if (changed)
            changed();
    }
    void Grid::finish()
    {
        c.commit();
        updateLayout();
        if (changed)
            changed();
    }
    void Grid::mouseUp (const juce::MouseEvent& e)
    {
        const auto h = down;
        down = {};
        if (downGeneration != c.p.getDocumentGeneration() || downPattern != c.p.getViewPatternSlot()
            || (!marquee && !c.gestureActive()))
        {
            marquee = dragging = false;
            c.refresh();
            updateLayout();
            return;
        }
        if (h.part == Part::drawVelocity)
        {
            if (!lastVelocityPoint)
                return;
            down = h;
            drawVelocityStroke (e.position);
            down = {};
            lastVelocityPoint.reset();
        }
        else if (h.part == Part::none && !dragging && !compact && changeCompactMode
            && !additiveMarquee && !e.mods.isShiftDown() && !e.mods.isCommandDown() && !e.mods.isPopupMenu())
        {
            c.clearSelection();
            changedViewOnClick = true;
            changeCompactMode (true);
        }
        else if (dragging && h.part == Part::move && !compactEdited)
        {
            const int target = juce::jlimit (0, 3, static_cast<int> ((e.position.y - 24) / 114));
            int insertion = 0;
            for (const auto& b : cells[static_cast<size_t> (target)])
                if (e.position.x > b.getCentreX())
                    ++insertion;
            c.moveSelected (h.row, h.step, target, insertion, e.mods.isAltDown());
        }
        else if (h.part == Part::add)
        {
            c.p.insertPhraseStep (h.row, h.step);
            if (dragging)
                c.p.setPhraseStepTimingMultiplier (h.row, h.step, juce::jlimit (0, 7, 3 + juce::roundToInt ((origin.y - e.position.y) / 16)));
        }
        else if (!dragging && h.row >= 0)
        {
            switch (h.part)
            {
                case Part::header:
                    if (!e.mods.isShiftDown())
                        c.clearSelection();
                    c.selectAll (h.row);
                    break;
                case Part::power:
                    if (e.mods.isShiftDown())
                        for (int r = 0; r < 4; ++r)
                            c.p.setPhraseRowMuted (r, r != h.row);
                    else
                        c.p.setPhraseRowMuted (h.row, !c.p.isPhraseRowMuted (h.row));
                    break;
                case Part::record:
                    if (recordRow)
                        recordRow (h.row);
                    break;
                case Part::rowVelocity:
                    if (toggleVelocity)
                        toggleVelocity (h.row);
                    break;
                case Part::seed:
                    if (seedRow)
                        seedRow (h.row);
                    break;
                case Part::remove:
                    c.p.removePhraseStep (h.row, h.step);
                    c.clearSelection();
                    break;
                case Part::skip:
                    c.editSelected ([value = !c.p.isPhraseStepSkipped (h.row, h.step)] (Step& s) { s.skipped = value; });
                    break;
                case Part::mute:
                    c.editSelected ([value = !c.p.isPhraseStepMuted (h.row, h.step)] (Step& s) { s.muted = value; });
                    break;
                case Part::gear:
                    if (inspect)
                        inspect (h.row, h.step);
                    break;
                case Part::move:
                case Part::duration:
                    if (compact && changeCompactMode && !e.mods.isShiftDown() && !e.mods.isCommandDown())
                    {
                        changedViewOnClick = true;
                        changeCompactMode (false);
                    }
                    break;
                case Part::none:
                case Part::drawVelocity:
                case Part::offset:
                case Part::add:
                case Part::note:
                case Part::velocity:
                case Part::resize:
                    break;
            }
        }
        marquee = dragging = false;
        finish();
    }
    void Grid::mouseDoubleClick (const juce::MouseEvent& e)
    {
        // The first click may have changed the controls under the pointer.
        if (changedViewOnClick && e.getNumberOfClicks() > 1)
            return;
        if (e.position.x >= 123 && (additiveMarquee || e.mods.isShiftDown()))
            return;
        const auto h = hit (e.position);
        if (h.part == Part::drawVelocity)
            return;
        c.begin();
        if (h.part == Part::resize && h.step >= -1)
        {
            if (e.mods.isAltDown())
            {
                if (h.step >= 0)
                    c.p.duplicatePhraseStep (h.row, h.step + 1);
            }
            else
                c.p.insertPhraseStep (h.row, h.step + 1);
        }
        else if (h.part == Part::duration)
            c.p.setPhraseStepDurationFraction (h.row, h.step, 1);
        else if (h.part == Part::velocity)
            c.p.setPhraseStepVelocity (h.row, h.step, 100);
        else if (h.part == Part::note)
            c.p.setPhraseNote (h.row, h.step, 60 + c.p.getPatternScaleRoot (c.p.getViewPatternSlot()));
        else if (h.part == Part::offset)
            c.p.setPhraseRowTimingOffset (h.row, 8);
        else if (h.part == Part::none)
            c.selectAll();
        finish();
    }
    void Grid::mouseMove (const juce::MouseEvent& e)
    {
        updateHover (e.position, e.mods);
    }
    void Grid::modifierKeysChanged (const juce::ModifierKeys& mods)
    {
        if (isMouseOver() && !mods.isAnyMouseButtonDown())
            updateHover (getMouseXYRelative().toFloat(), mods);
        juce::Component::modifierKeysChanged (mods);
    }
    void Grid::updateHover (juce::Point<float> position, juce::ModifierKeys mods)
    {
        const bool selecting = mods.isShiftDown() && position.x >= 123;
        const auto h = hit (position, selecting);
        if (h.part != hover.part || h.row != hover.row || h.step != hover.step)
        {
            hover = h;
            repaint();
        }
        if (selecting)
        {
            setMouseCursor (juce::MouseCursor::CrosshairCursor);
            setTooltip ("Drag to add steps to the selection. Shift-click toggles a step.");
            return;
        }
        setMouseCursor (h.part == Part::resize || h.part == Part::duration ? juce::MouseCursor::LeftRightResizeCursor : h.part == Part::note || h.part == Part::velocity || h.part == Part::offset ? juce::MouseCursor::UpDownResizeCursor
                                                                                     : h.part == Part::move                                                      ? juce::MouseCursor::DraggingHandCursor
                                                                                                                                                                 : juce::MouseCursor::PointingHandCursor);
        if (h.part == Part::rowVelocity)
            setTooltip ("Toggle velocity drawing in this row.");
        else if (h.part == Part::drawVelocity)
            setTooltip ("Drag across the row to draw velocities.");
        else if (h.part == Part::duration)
            setTooltip ("Drag horizontally to adjust note duration.");
        else if (h.part == Part::gear)
            setTooltip ("Open step inspector.");
        else if (h.part == Part::move && compact)
            setTooltip ("Click for detailed steps. Shift-drag selects steps.");
        else
            setTooltip (h.step >= 0 && h.part != Part::add
                            ? "Row " + juce::String (h.row + 1) + ", step " + juce::String (h.step + 1) + ". Drag to edit; Shift-drag selects; Option-drag copies."
                            : "Drag the background to select steps.");
    }
    void Grid::mouseExit (const juce::MouseEvent&)
    {
        if (hover.part == Part::none)
            return;
        hover = {};
        repaint();
    }
    void Grid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        const auto h = hit (e.position);
        if (h.row < 0)
            return;
        auto& position = scroll[static_cast<size_t> (h.row)];
        position = std::max (0.0f, position - (std::abs (wheel.deltaX) > .0001f ? wheel.deltaX : wheel.deltaY) * 100);
        updateLayout();
    }
    void Grid::cancelGesture()
    {
        c.cancel();
        down = {};
        lastVelocityPoint.reset();
        marquee = dragging = false;
        updateLayout();
        if (changed)
            changed();
    }
    bool Grid::keyPressed (const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            cancelGesture();
            return true;
        }
        if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
        {
            c.removeSelected();
            updateLayout();
            if (changed)
                changed();
            return true;
        }
        if (key.getModifiers().isCommandDown() && key.getKeyCode() == 'A')
        {
            c.selectAll();
            repaint();
            return true;
        }
        return false;
    }
}
