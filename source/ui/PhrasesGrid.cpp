#include "PhrasesGrid.h"
#include <span>
namespace phrases::ui
{
    namespace
    {
        constexpr float stepBarHeight = 36.0f;
        constexpr float lengthLabelHeight = 12.0f;
        constexpr float lengthLabelGap = 2.0f;

        juce::String lengthRatio (int timing)
        {
            auto length = juce::String ((timing + 1) * .25, 2).trimCharactersAtEnd ("0").trimCharactersAtEnd (".");
            return length.startsWith ("0.") ? length.substring (1) : length;
        }

        int draggedPitch (int note, int steps, bool octaves, int root, int mode)
        {
            return octaves ? juce::jlimit (0, 127, note + steps * 12) : scaleNote (note, steps, root, mode);
        }

        Rect durationArea (Rect card)
        {
            return card.withHeight (stepBarHeight).reduced (std::min (10.0f, card.getWidth() * .25f), 5.0f);
        }

        struct StepDetails
        {
            Rect remove, skip, mute, velocity, length, probability, cycle;
        };

        StepDetails stepDetails (Rect card)
        {
            auto body = card.withTrimmedTop (stepBarHeight).reduced (4, 0);
            auto actions = body.removeFromLeft (std::min (20.0f, body.getWidth() * .38f));
            body = body.withHeight (std::min (60.0f, body.getHeight()));
            const float actionHeight = actions.getHeight() / 3;
            StepDetails detail;
            detail.remove = actions.removeFromTop (actionHeight);
            detail.skip = actions.removeFromTop (actionHeight);
            detail.mute = actions;

            body.removeFromLeft (4);
            if (body.getWidth() >= 76)
            {
                body = body.reduced (5, 3);
                const float summaryHeight = body.getHeight() / 3;
                auto summary = body.removeFromTop (summaryHeight);
                detail.length = summary.removeFromRight (28);
                detail.velocity = summary;
                auto rhythm = body.removeFromBottom (summaryHeight);
                detail.probability = rhythm.removeFromLeft (32);
                detail.cycle = rhythm.removeFromRight (74.5f);
            }
            else
            {
                const float lineHeight = body.getHeight() / 4;
                detail.velocity = body.removeFromTop (lineHeight);
                detail.length = body.removeFromTop (lineHeight);
                detail.probability = body.removeFromTop (lineHeight);
                detail.cycle = body;
            }
            return detail;
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
        icons = { icon (RowDisableIcon), icon (RowEditPencilIcon), icon (SaplingIcon), icon (StepMuteIcon), icon (StepSkipIcon) };
    }
    void Grid::drawIcon (juce::Graphics& g, size_t index, Rect b, juce::Colour ink)
    {
        if (auto* d = icons[index].get())
        {
            d->replaceColour (iconInks[index], ink); iconInks[index] = ink;
            d->drawWithin (g, b, juce::RectanglePlacement::centred, 1);
        }
    }
    void Grid::drawStepBar (juce::Graphics& g, Rect bar, const Step& step, juce::Colour accent, bool durationHot, bool playing, float flash)
    {
        g.setColour (p.surface.interpolatedWith (p.app, .12f));
        g.fillRoundedRectangle (bar, 8);
        g.setColour (accent.withAlpha (step.skipped ? .12f : .22f + .78f * step.velocity / 127.0f));
        g.fillRoundedRectangle (bar.reduced (1), 6);
        if (playing)
        {
            g.setColour (accent.withAlpha (.72f));
            g.fillRoundedRectangle (bar, 8);
        }
        if (!step.muted)
        {
            const auto area = durationArea (bar);
            const float edge = durationEdge (area, step);
            g.setColour (p.app.withAlpha (.52f));
            g.fillRoundedRectangle (area, 3);
            const auto durationFill = accent.brighter (.2f).withAlpha (.35f + .6f * step.velocity / 127.0f);
            g.setColour (playing ? accent.interpolatedWith (juce::Colours::white, .18f + .40f * flash) : durationFill);
            g.fillRoundedRectangle (area.withRight (edge), 3);
            const auto grip = Rect (edge - 3, area.getY() + 1, 6, area.getHeight() - 2)
                                  .getIntersection (bar.reduced (1));
            g.setColour (p.app.withAlpha (durationHot ? .9f : .65f));
            g.fillRoundedRectangle (grip, 2);
            g.setColour (durationHot ? accent : p.text.withAlpha (.8f));
            for (int dot = -2; dot <= 2; ++dot)
                g.fillEllipse (grip.getCentreX() - .8f, grip.getCentreY() + static_cast<float> (dot) * 4.0f - .8f, 1.6f, 1.6f);
        }
        if (bar.getWidth() > 22)
            text (g, noteName (step.note), bar.reduced (3), p.text, 14, true);
    }
    void Grid::resized() { updateLayout(); }
    void Grid::setVelocityMode (bool enabled)
    {
        if (velocityMode == enabled)
            return;
        velocityMode = enabled;
        updateLayout();
    }
    void Grid::setVisibleRowCount (int count)
    {
        if (count == visibleRowCount)
            return;
        visibleRowCount = juce::jlimit (1, static_cast<int> (cells.size()), count);
        for (size_t row = static_cast<size_t> (visibleRowCount); row < cells.size(); ++row)
        {
            cells[row].clear();
            c.selection[row].reset();
        }
        hover = {};
        updateLayout();
    }
    void Grid::selectVisibleSteps()
    {
        for (int row = 0; row < visibleRowCount; ++row)
            c.selectAll (row);
        repaint();
    }
    void Grid::updateLayout()
    {
        int minimumOffset = 8;
        const auto visibleRows = std::span (c.rows).first (static_cast<size_t> (visibleRowCount));
        for (const auto& row : visibleRows)
            minimumOffset = std::min (minimumOffset, row.offset);
        constexpr float stepsLeft = 135;
        constexpr float addButtonSpace = 74;
        float contentRight = 0;
        for (const auto& row : visibleRows)
        {
            int columns = row.offset - minimumOffset;
            for (const auto& step : row.steps)
                columns += step.timing + 1;
            contentRight = std::max (contentRight, stepsLeft + static_cast<float> (columns) * unit + addButtonSpace);
        }
        maxScrollX = std::max (0.0f, contentRight - static_cast<float> (getWidth()));
        scrollX = juce::jlimit (0.0f, maxScrollX, scrollX);
        const float extraHeight = rowHeight() - 104.0f;
        for (size_t r = 0; r < visibleRows.size(); ++r)
        {
            cells[r].clear();
            float x = stepsLeft + static_cast<float> (c.rows[r].offset - minimumOffset) * unit - scrollX;
            const auto& steps = c.rows[r].steps;
            const float y = rowTop (r) + 4;
            int low = 127, high = 0;
            if (compact)
                for (const auto& step : steps)
                {
                    low = std::min (low, step.note);
                    high = std::max (high, step.note);
                }
            const float pitchCentre = (low + high) * .5f;
            const float pitchTravel = 60.0f + extraHeight;
            const float pitchScale = pitchTravel / std::max (6, high - low);
            const bool pitchCards = compact && !velocityMode;
            const bool anchored = pitchCards && compactPitchDrag && c.gestureActive() && static_cast<int> (r) == down.row
                && down.step >= 0 && static_cast<size_t> (down.step) < steps.size() && steps.size() == baseline[r].steps.size();
            float anchorTop = 0, aboveScale = 0, belowScale = 0;
            float anchorNote = 0;
            std::array<float, PluginProcessor::maxPhraseStepsPerRow> displayPitches;
            if (anchored)
            {
                anchorTop = juce::jlimit (y, y + pitchTravel, grabbedNoteTop + pointer.y - origin.y);
                const auto& originalSteps = baseline[r].steps;
                const int slot = c.p.getViewPatternSlot();
                const int root = c.p.getPatternScaleRoot (slot), mode = c.p.getPatternScaleModeIndex (slot);
                const int lowerStep = static_cast<int> (std::floor (compactPitchSteps));
                const float fraction = compactPitchSteps - static_cast<float> (lowerStep);
                float lowest = 127, highest = 0;
                // Interpolate the layout between scale steps while playback uses the snapped pitch.
                for (size_t s = 0; s < steps.size(); ++s)
                {
                    float pitch = static_cast<float> (steps[s].note);
                    if (c.selection[r][s])
                    {
                        const int note = originalSteps[s].note;
                        const auto from = draggedPitch (note, lowerStep, compactPitchOctaves, root, mode);
                        const auto to = draggedPitch (note, lowerStep + 1, compactPitchOctaves, root, mode);
                        pitch = std::lerp (static_cast<float> (from), static_cast<float> (to), fraction);
                    }
                    displayPitches[s] = pitch;
                    lowest = std::min (lowest, pitch);
                    highest = std::max (highest, pitch);
                }
                anchorNote = displayPitches[static_cast<size_t> (down.step)];
                const auto extremes = std::ranges::minmax_element (originalSteps, {}, &Step::note);
                const float originalScale = pitchTravel / std::max (6, extremes.max->note - extremes.min->note);
                aboveScale = std::min (originalScale, (anchorTop - y) / std::max (1.0f, highest - anchorNote));
                belowScale = std::min (originalScale, (y + pitchTravel - anchorTop) / std::max (1.0f, anchorNote - lowest));
            }
            for (size_t s = 0; s < steps.size(); ++s)
            {
                const auto& step = steps[s];
                const float width = static_cast<float> (step.timing + 1) * unit;
                const float gap = std::min (16.0f, width * .14f);
                float top = y;
                if (anchored)
                {
                    const float pitch = displayPitches[s];
                    top = anchorTop + (anchorNote - pitch) * (pitch > anchorNote ? aboveScale : belowScale);
                }
                else if (pitchCards)
                    top += pitchTravel * .5f + (pitchCentre - step.note) * pitchScale;
                cells[r].push_back ({ x + gap * .5f, top, std::max (2.0f, width - gap), pitchCards ? stepBarHeight : rowHeight() - 8.0f });
                x += width;
            }
            addBounds[r] = { x + 4, y + 20 + extraHeight * .5f, 54, 54 };
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
        const bool hasSelection = std::any_of (c.selection.begin(), c.selection.end(), [] (const auto& selectedSteps) { return selectedSteps.any(); });
        for (size_t r = 0; r < static_cast<size_t> (visibleRowCount); ++r)
        {
            const auto& row = c.rows[r];
            const float top = rowTop (r);
            const float y = top + (rowHeight() - 104.0f) * .5f;
            const auto accent = c.p.isRowColorsEnabled() ? p.rows[r] : p.rows[0];
            g.setColour (p.app.interpolatedWith (p.surface, .55f));
            g.fillRect (Rect (0, top, 123, rowHeight()));
            if (hover.row == static_cast<int> (r) && hover.step < 0 && hover.part != Part::none && hover.part != Part::add)
            {
                g.setColour (accent.withAlpha (0.07f));
                g.fillRect (Rect (0, top, 123, rowHeight()));
            }
            g.setColour (p.border.withAlpha (.5f));
            g.drawLine (123, top, 123, top + rowHeight());
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
            const bool drawingVelocity = velocityMode;
            const Rect velocityButton (49, y + 69, 26, 26);
            if (drawingVelocity)
            {
                g.setColour (accent.withAlpha (0.23f));
                g.fillRoundedRectangle (velocityButton, 4);
            }
            const auto velocityInk = drawingVelocity || headerHot (Part::rowVelocity) ? accent : p.faint;
            text (g, "V", velocityButton, velocityInk, 15, true);
            drawIcon (g, 2, { 80, y + 74, 14, 14 }, headerHot (Part::seed) ? accent : p.faint);
            juce::Graphics::ScopedSaveState clip (g);
            const float labelInset = compact && !drawingVelocity ? compactLabelInset : 0.0f;
            g.reduceClipRegion (Rect (128, top - labelInset, static_cast<float> (getWidth() - 128), rowHeight() + labelInset).toNearestInt());
            juce::Path selectedPath;
            for (size_t s = 0; s < cells[r].size(); ++s)
                if (c.selection[r][s])
                    selectedPath.addRoundedRectangle (cells[r][s].withHeight (stepBarHeight).reduced (1), 7);
            if (!selectedPath.isEmpty())
            {
                auto& glow = selectionGlows[r];
                glow.setColor (accent.withAlpha (.80f));
                glow.setRadius (12);
                glow.render (g, selectedPath);
            }
            for (size_t s = 0; s < cells[r].size(); ++s)
            {
                const auto b = cells[r][s];
                const auto& step = row.steps[s];
                const bool selected = c.selection[r][s];
                const auto bar = b.withHeight (stepBarHeight);
                const auto body = b.withTop (bar.getBottom() - 4);
                const bool detailed = !compact && !drawingVelocity;
                const auto face = drawingVelocity ? b : bar;
                const auto activity = playbackSteps[r][s];
                const float flash = activity.flash;
                const bool playing = activity.sounding || flash > 0;
                if (playing && g.clipRegionIntersects (face.expanded (14).getSmallestIntegerContainer()))
                {
                    juce::Path path;
                    path.addRoundedRectangle (face.reduced (1), 8);
                    auto& glow = playbackGlows[r];
                    glow.setRadius (12);
                    glow.setColor (accent.withAlpha (.34f + .56f * flash));
                    glow.render (g, path);
                }
                g.setColour (selected ? p.surface.interpolatedWith (accent, .40f) : p.surface.interpolatedWith (p.app, .12f));
                g.fillRoundedRectangle (detailed ? body : b, 8);
                if (playing && (detailed || drawingVelocity))
                {
                    g.setColour (accent.withAlpha (.72f));
                    g.fillRoundedRectangle (detailed ? body : b, 8);
                }
                if (drawingVelocity)
                {
                    const auto area = b.reduced (2);
                    const auto level = area.getBottom() - area.getHeight() * step.velocity / 127.0f;
                    const auto fill = area.withTop (level);
                    g.setColour (accent.withAlpha (.42f));
                    g.fillRoundedRectangle (fill, 5);
                    g.setColour (accent);
                    g.drawLine (area.getX(), level, area.getRight(), level, 1.5f);
                    if (b.getWidth() >= 24)
                        text (g, juce::String (step.velocity), b.withHeight (22), p.text, 12, true);
                }
                else
                {
                    if (detailed)
                    {
                        juce::Graphics::ScopedSaveState detailClip (g);
                        g.reduceClipRegion (b.getSmallestIntegerContainer());
                        const auto detail = stepDetails (b);
                        const auto hot = [&](Part part) {
                            return hover.row == static_cast<int> (r) && hover.step == static_cast<int> (s) && hover.part == part;
                        };
                        g.setColour (p.text.withAlpha (.15f));
                        g.drawVerticalLine (juce::roundToInt (detail.remove.getRight()), detail.remove.getY(), detail.mute.getBottom());
                        for (const auto button : { detail.skip, detail.mute })
                            g.drawHorizontalLine (juce::roundToInt (button.getY()), button.getX(), button.getRight());
                        text (g, juce::String::fromUTF8 ("×"), detail.remove, hot (Part::remove) ? p.danger : p.secondary, 24, true);
                        drawIcon (g, 4, detail.skip.reduced (2), step.skipped || hot (Part::skip) ? p.text : p.muted);
                        drawIcon (g, 3, detail.mute.reduced (2), step.muted || hot (Part::mute) ? p.text : p.muted);
                        text (g, juce::String (step.velocity), detail.velocity, p.text, 11, true, juce::Justification::centredLeft);
                        const auto lengthAlign = detail.length.getX() > detail.velocity.getX() ? juce::Justification::centredRight : juce::Justification::centredLeft;
                        text (g, lengthRatio (step.timing), detail.length, p.text, 12, true, lengthAlign);
                        text (g, juce::String (step.probability) + "%", detail.probability, p.text, 10, true, juce::Justification::centredLeft);
                        constexpr float pipGap = 1.5f;
                        const float pipWidth = std::min (8.0f, (detail.cycle.getWidth() - 7 * pipGap) / 8);
                        if (pipWidth >= 1.0f)
                            for (int i = 0; i < 8; ++i)
                            {
                                const bool inside = i < step.cycle;
                                const bool on = inside && ((step.mask == 0 ? 1 : step.mask) & (1 << i)) != 0;
                                g.setColour (on ? p.text : inside ? p.subtle : p.app);
                                g.fillRoundedRectangle (detail.cycle.getX() + static_cast<float> (i) * (pipWidth + pipGap),
                                                        detail.cycle.getCentreY() - 3, pipWidth, 6, 1);
                            }
                    }
                    else
                    {
                        const auto label = bar.withY (bar.getY() - lengthLabelHeight - lengthLabelGap).withHeight (lengthLabelHeight);
                        text (g, lengthRatio (step.timing), label, p.text, 11, true, juce::Justification::centredRight);
                    }

                    const bool durationHot = (hover.part == Part::duration && hover.row == static_cast<int> (r) && hover.step == static_cast<int> (s))
                        || (dragging && down.part == Part::duration && down.row == static_cast<int> (r) && down.step == static_cast<int> (s));
                    drawStepBar (g, bar, step, accent, durationHot, playing, flash);
                }
                const bool inspected = inspectedRow == static_cast<int> (r) && inspectedStep == static_cast<int> (s);
                const auto decorate = [&] (Rect bounds, bool outline = true) {
                    if (row.muted || step.skipped)
                    {
                        g.setColour (p.app.withAlpha (.5f));
                        g.fillRoundedRectangle (bounds, 8);
                    }
                    if (!outline)
                        return;
                    g.setColour (inspected || selected ? accent : p.border);
                    g.drawRoundedRectangle (bounds.reduced (1), 8, inspected ? 2.5f : 2);
                    if (inspected)
                    {
                        g.setColour (accent.withAlpha (0.35f));
                        g.drawRoundedRectangle (bounds.expanded (2), 10, 3);
                    }
                };
                if (detailed)
                {
                    juce::Graphics::ScopedSaveState bodyClip (g);
                    g.reduceClipRegion (b.expanded (5).withTop (bar.getBottom()).getSmallestIntegerContainer());
                    decorate (body, false);
                }
                decorate (face);
                if (selected)
                {
                    g.setColour (p.text.withAlpha (.95f));
                    g.drawRoundedRectangle (face.reduced (1.5f), 7, 2.5f);
                }
                if ((hasSelection || inspectedRow >= 0) && !selected && !inspected && !playing)
                {
                    g.setColour (p.app.withAlpha (.22f));
                    g.fillRoundedRectangle (b, 8);
                }
                if (step.muted)
                {
                    g.setColour (p.faint.withAlpha (.45f));
                    g.drawLine (face.getX() + 2, face.getBottom() - 3, face.getRight() - 2, face.getY() + 3, 1);
                }
                if (playing)
                {
                    g.setColour (accent.interpolatedWith (juce::Colours::white, .40f + .50f * flash));
                    g.drawRoundedRectangle (face.reduced (1), 8, 2.0f + flash);
                }
            }
            if (hover.row == static_cast<int> (r) && hover.part == Part::resize)
            {
                g.setColour (accent);
                const auto drawHandle = [&] (Rect card, float edge) {
                    g.fillRoundedRectangle (edge - 2.0f, card.getY() + stepBarHeight * .5f - 14.0f, 4.0f, 28.0f, 2.0f);
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
        if (dragging && down.part == Part::move && !compactEdited)
        {
            g.setColour (p.accent);
            g.drawVerticalLine (juce::roundToInt (pointer.x), pointer.y - 30, pointer.y + 30);
        }
    }
    Grid::Hit Grid::hit (juce::Point<float> point, bool selecting)
    {
        for (int r = 0; r < visibleRowCount; ++r)
        {
            const float top = rowTop (static_cast<size_t> (r));
            if (point.y < top || point.y >= top + rowHeight())
                continue;
            if (point.x < 123)
            {
                const float y = top + (rowHeight() - 104.0f) * .5f;
                if (point.y < y + 16 || point.y >= y + 104 || point.x < 15 || point.x > 110)
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
            if (velocityMode && !cells[index].empty())
            {
                if (point.x >= 128 && cells[index].front().getUnion (cells[index].back()).contains (point))
                    return { Part::drawVelocity, r, -1 };
                return { Part::none, r, -1 };
            }
            for (size_t s = 0; s < cells[index].size(); ++s)
            {
                const auto b = cells[index][s];
                const int step = static_cast<int> (s);
                if (s + 1 < cells[index].size())
                {
                    const auto next = cells[index][s + 1];
                    if (point.x >= b.getRight() && point.x < next.getX()
                        && point.y >= std::min (b.getY(), next.getY())
                        && point.y < std::max (b.getBottom(), next.getBottom()))
                        return { Part::resize, r, step };
                }
                const float edgeSlop = compact ? 4.0f : 9.0f;
                if (!b.expanded (edgeSlop - 1, 0).contains (point))
                    continue;
                const float resizeZone = b.contains (point) ? 4.0f : edgeSlop;
                if (std::abs (point.x - b.getX()) < resizeZone)
                    return { Part::resize, r, step - 1 };
                if (std::abs (point.x - b.getRight()) < resizeZone)
                    return { Part::resize, r, step };
                const auto bar = b.withHeight (stepBarHeight);
                if (bar.contains (point) && s < c.rows[index].steps.size())
                {
                    const auto area = durationArea (b);
                    const auto& value = c.rows[index].steps[s];
                    if (!value.muted && area.expanded (4, 3).contains (point)
                        && std::abs (point.x - durationEdge (area, value)) <= 4)
                        return { Part::duration, r, step };
                    if (!compact && std::abs (point.x - bar.getCentreX()) <= std::min (25.0f, bar.getWidth() * .25f))
                        return { Part::note, r, step };
                    return { Part::move, r, step };
                }
                if (compact)
                    return { Part::move, r, step };
                const auto detail = stepDetails (b);
                if (detail.remove.contains (point))
                    return { Part::remove, r, step };
                if (detail.skip.contains (point))
                    return { Part::skip, r, step };
                if (detail.mute.contains (point))
                    return { Part::mute, r, step };
                if (detail.velocity.contains (point))
                    return { Part::velocity, r, step };
                return { Part::move, r, step };
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
        const auto directHit = hit (e.position);
        const bool octaveDrag = directHit.part == Part::note || (compact && directHit.part == Part::move);
        shiftOctaveGesture = e.mods.isShiftDown() && octaveDrag;
        additiveMarquee = e.mods.isShiftDown() && e.position.x >= 123 && !octaveDrag;
        down = hit (e.position, additiveMarquee);
        origin = pointer = e.position;
        dragging = compactEdited = compactPitchDrag = false;
        grabbedNoteTop = stepBounds (down.row, down.step).getY();
        lastVelocityPoint.reset();
        baseline = readRows (c.p);
        c.rows = baseline;
        selectionBefore = c.selection;
        marquee = down.part == Part::none;
        if (down.step >= 0 && down.part != Part::add && !shiftOctaveGesture)
        {
            const auto r = static_cast<size_t> (down.row), s = static_cast<size_t> (down.step);
            if (additiveMarquee)
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
        if (!lastVelocityPoint || !c.gestureActive() || !velocityMode || down.row >= visibleRowCount)
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
        if (!dragging && e.getDistanceFromDragStart() > 3)
        {
            dragging = true;
            if (shiftOctaveGesture && down.step >= 0)
            {
                const auto row = static_cast<size_t> (down.row), step = static_cast<size_t> (down.step);
                if (!c.selection[row][step])
                {
                    c.clearSelection();
                    c.selection[row].set (step);
                }
            }
        }
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
                if (compactPitchDrag || std::abs (dy) >= std::abs (dx))
                {
                    compactPitchDrag = true;
                    compactPitchOctaves = e.mods.isShiftDown();
                    compactPitchSteps = dy / (compactPitchOctaves ? 24.0f : 8.0f);
                    const int degrees = juce::roundToInt (compactPitchSteps);
                    const int slot = c.p.getViewPatternSlot();
                    const int root = c.p.getPatternScaleRoot (slot), mode = c.p.getPatternScaleModeIndex (slot);
                    c.editSelected ([&] (Step& s) { s.note = draggedPitch (s.note, degrees, compactPitchOctaves, root, mode); });
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
        if (down.part == Part::duration)
        {
            const float width = durationArea (stepBounds (down.row, down.step)).getWidth();
            const double delta = (pointer.x - origin.x) / std::max (1.0f, width);
            bool edited = false;
            for (size_t r = 0; r < c.rows.size(); ++r)
                for (size_t s = 0; s < c.rows[r].steps.size(); ++s)
                {
                    if (!c.selection[r][s])
                        continue;
                    auto& step = c.rows[r].steps[s];
                    double duration = juce::jlimit (0.0, 1.0, baseline[r].steps[s].duration + delta);
                    if (e.mods.isShiftDown())
                        duration = std::round (duration * 4) / 4;
                    if (juce::approximatelyEqual (step.duration, duration))
                        continue;
                    c.p.setPhraseStepDurationFraction (static_cast<int> (r), static_cast<int> (s), duration);
                    step.duration = duration;
                    edited = true;
                }
            if (edited)
            {
                repaint();
                if (changed)
                    changed();
            }
            return;
        }
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
        const int pattern = c.p.getViewPatternSlot(), root = c.p.getPatternScaleRoot (pattern), mode = c.p.getPatternScaleModeIndex (pattern);
        c.editSelected ([&] (Step& step) {
            if (down.part == Part::note)
                step.note = e.mods.isShiftDown() ? juce::jlimit (0, 127, step.note + juce::roundToInt ((origin.y - pointer.y) / 24.0f) * 12) : scaleNote (step.note, delta, root, mode);
            else if (down.part == Part::velocity)
                step.velocity = juce::jlimit (0, 127, step.velocity + delta);
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
        compactPitchDrag = false;
        if (downGeneration != c.p.getDocumentGeneration() || downPattern != c.p.getViewPatternSlot()
            || (!marquee && !c.gestureActive()))
        {
            marquee = dragging = false;
            c.refresh();
            updateLayout();
            return;
        }
        if (shiftOctaveGesture && !dragging)
            c.selection[static_cast<size_t> (h.row)].flip (static_cast<size_t> (h.step));
        else if (h.part == Part::drawVelocity)
        {
            if (!lastVelocityPoint)
                return;
            down = h;
            drawVelocityStroke (e.position);
            down = {};
            lastVelocityPoint.reset();
        }
        else if (h.part == Part::none && !dragging
            && !additiveMarquee && !e.mods.isShiftDown() && !e.mods.isPopupMenu())
        {
            c.clearSelection();
            if (inspectedRow >= 0 && inspect)
                inspect (-1, -1);
            if (!compact && changeCompactMode)
            {
                changedViewOnClick = true;
                changeCompactMode (true);
            }
        }
        else if (dragging && h.part == Part::move && !compactEdited)
        {
            const int target = juce::jlimit (0, visibleRowCount - 1, static_cast<int> ((e.position.y - rowsTop()) / rowStride()));
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
            const bool inspectorTarget = h.part == Part::move || h.part == Part::duration
                || h.part == Part::note || h.part == Part::velocity || h.part == Part::resize;
            const auto target = h.part == Part::resize ? hit (origin, true) : h;
            if (inspect && inspectorTarget && target.step >= 0
                && !e.mods.isShiftDown() && !e.mods.isPopupMenu())
            {
                if (target.row == inspectedRow && target.step == inspectedStep)
                {
                    inspect (-1, -1);
                    if (!compact && changeCompactMode)
                    {
                        changedViewOnClick = true;
                        changeCompactMode (true);
                    }
                }
                else
                {
                    if (compact && changeCompactMode)
                    {
                        changedViewOnClick = true;
                        changeCompactMode (false);
                    }
                    c.clearSelection();
                    c.selection[static_cast<size_t> (target.row)].set (static_cast<size_t> (target.step));
                    inspect (target.row, target.step);
                }
            }

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
                        toggleVelocity();
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
                case Part::move:
                case Part::duration:
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
            selectVisibleSteps();
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
        const auto directHit = hit (position);
        const bool octaveDrag = directHit.part == Part::note || (compact && directHit.part == Part::move);
        const bool selecting = mods.isShiftDown() && position.x >= 123 && !octaveDrag;
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
            setTooltip ("Toggle velocity drawing for all rows.");
        else if (h.part == Part::drawVelocity)
            setTooltip ("Drag across the row to draw velocities.");
        else if (h.part == Part::duration)
            setTooltip ("Click to inspect the step. Drag horizontally to adjust note duration.");
        else if (h.part == Part::remove)
            setTooltip ("Delete this step.");
        else if (h.part == Part::skip)
            setTooltip ("Toggle skip for the selected steps.");
        else if (h.part == Part::mute)
            setTooltip ("Toggle mute for the selected steps.");
        else if (h.part == Part::move && compact)
            setTooltip ("Click to inspect the step. Drag vertically to change pitch; Shift-drag changes octaves.");
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
        const float delta = std::abs (wheel.deltaX) >= std::abs (wheel.deltaY) ? wheel.deltaX : wheel.deltaY;
        const float next = juce::jlimit (0.0f, maxScrollX, scrollX - delta * (wheel.isSmooth ? 512.0f : 320.0f));
        if (juce::approximatelyEqual (next, scrollX))
            return;
        scrollX = next;
        updateLayout();
        updateHover (e.position, e.mods);
    }
    void Grid::cancelGesture()
    {
        c.cancel();
        down = {};
        lastVelocityPoint.reset();
        marquee = dragging = false;
        shiftOctaveGesture = false;
        compactPitchDrag = false;
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
            selectVisibleSteps();
            return true;
        }
        return false;
    }
}
