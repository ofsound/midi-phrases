#include "PhrasesPreview.h"
namespace phrases::ui
{
    std::vector<PreviewNote> renderPreview (PluginProcessor& source, double quarters)
    {
        juce::MemoryBlock state;
        source.getStateInformation (state);
        auto renderer = std::make_unique<PluginProcessor>();
        renderer->setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        renderer->setCurrentPatternSlot (source.getViewPatternSlot());
        renderer->setLoopBraceEnabled (false);
        renderer->setStandaloneTransportPlaying (false);
        struct Clock : juce::AudioPlayHead
        {
            PositionInfo info;
            juce::Optional<PositionInfo> getPosition() const override { return info; }
        } clock;
        clock.info.setBpm (120);
        clock.info.setIsPlaying (true);
        clock.info.setPpqPosition (0);
        renderer->setPlayHead (&clock);
        constexpr int block = 128;
        constexpr double rate = 8000;
        renderer->prepareToPlay (rate, block);
        juce::AudioBuffer<float> audio (2, block);
        juce::MidiBuffer midi;
        std::vector<PreviewNote> output;
        output.reserve (2048);
        std::array<std::array<std::vector<size_t>, 128>, 16> active;
        const int samples = static_cast<int> (juce::jlimit (1.0, 128.0, quarters) * rate * .5);
        for (int start = 0; start < samples; start += block)
        {
            midi.clear();
            audio.clear();
            clock.info.setPpqPosition (start * 2.0 / rate);
            renderer->processBlock (audio, midi);
            for (const auto event : midi)
            {
                const auto message = event.getMessage();
                if (!message.isNoteOnOrOff())
                    continue;
                auto& held = active[static_cast<size_t> (message.getChannel() - 1)][static_cast<size_t> (message.getNoteNumber())];
                const double beat = (start + event.samplePosition) * 2.0 / rate;
                if (beat >= quarters)
                    continue;
                if (message.isNoteOn())
                {
                    int row = 0;
                    for (int r = 0; r < 4; ++r)
                        if (source.getPhraseRowMidiChannel (r) == message.getChannel())
                        {
                            row = r;
                            break;
                        }
                    held.push_back (output.size());
                    output.push_back ({ beat, quarters, message.getNoteNumber(), message.getVelocity(), row });
                }
                else if (!held.empty())
                {
                    output[held.front()].end = beat;
                    held.erase (held.begin());
                }
            }
        }
        renderer->releaseResources();
        renderer->setPlayHead (nullptr);
        return output;
    }
    float Preview::beatX (double beat) const { return 44 + static_cast<float> ((beat - firstQuarter) * pixelsPerQuarter); }
    double Preview::xBeat (float x) const { return firstQuarter + (x - 44) / pixelsPerQuarter; }
    Rect Preview::noteBounds (double start, double end, int note) const
    {
        const float height = std::max (1.0f, (getHeight() - 30.0f) / static_cast<float> (highNote - lowNote + 1));
        return { beatX (start), 30 + static_cast<float> (highNote - note) * height + 1, std::max (2.0f, beatX (end) - beatX (start) - 1), std::max (1.0f, height - 2) };
    }
    void Preview::refresh()
    {
        double longest = 0;
        for (const auto& row : c.rows)
        {
            if (row.muted)
                continue;
            double duration = 0;
            for (const auto& step : row.steps)
                if (!step.skipped)
                    duration += (step.timing + 1) * .25;
            longest = std::max (longest, duration * PluginProcessor::pulseQuartersForIndex (c.p.getPulseIndex()));
        }
        visibleQuarters = std::max (8.0, std::ceil (longest / 4) * 4);
        if (c.p.isCombinationModeEnabled (PluginProcessor::combinationModeHocket))
            visibleQuarters = std::max (32.0, visibleQuarters * 4);
        visibleQuarters = std::min (128.0, visibleQuarters);
        notes = renderPreview (c.p, visibleQuarters);
        fit();
        repaint();
    }
    void Preview::fit()
    {
        lowNote = 48;
        highNote = 72;
        if (!notes.empty())
        {
            lowNote = 127;
            highNote = 0;
            for (const auto& event : notes)
            {
                lowNote = std::min (lowNote, event.note);
                highNote = std::max (highNote, event.note);
            }
            lowNote = std::max (0, lowNote - 2);
            highNote = std::min (127, highNote + 2);
            while (highNote - lowNote + 1 < 5)
            {
                if (lowNote > 0)
                    --lowNote;
                if (highNote - lowNote + 1 < 5 && highNote < 127)
                    ++highNote;
            }
        }
        repaint();
    }
    void Preview::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (p.app);
        g.fillRoundedRectangle (bounds, 12);
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip;
        clip.addRoundedRectangle (bounds, 12);
        g.reduceClipRegion (clip);
        const float h = std::max (1.0f, (getHeight() - 30.0f) / static_cast<float> (highNote - lowNote + 1));
        for (int n = lowNote; n <= highNote; ++n)
        {
            const auto pitch = n % 12;
            const bool black = pitch == 1 || pitch == 3 || pitch == 6 || pitch == 8 || pitch == 10;
            const auto y = 30 + static_cast<float> (highNote - n) * h;
            const float rollRight = std::min (static_cast<float> (getWidth()), beatX (visibleQuarters));
            g.setColour (black ? p.app : p.raised.withAlpha (.7f));
            g.fillRect (Rect (0, y, rollRight, h));
            g.setColour (p.border.withAlpha (.35f));
            g.drawHorizontalLine (juce::roundToInt (y), 0, rollRight);
            if (pitch == 0)
                text (g, noteName (n), { 0, y, 40, h }, p.muted, 10, false, juce::Justification::centredRight);
        }
        for (int beat = static_cast<int> (std::floor (firstQuarter)); beat <= firstQuarter + visibleQuarters; ++beat)
        {
            const float x = beatX (beat);
            g.setColour ((beat % 4 == 0 ? p.muted : p.border).withAlpha (.45f));
            g.drawVerticalLine (juce::roundToInt (x), 0, static_cast<float> (getHeight()));
            if (beat % 4 == 0)
                text (g, juce::String (beat), { x + 4, 0, 30, 28 }, p.faint, 10, false, juce::Justification::centredLeft);
        }
        const float start = beatX (c.p.getLoopBraceStartQuarters()), end = beatX (c.p.getLoopBraceEndQuarters());
        const auto loopColour = c.p.isLoopBraceEnabled() ? p.accent : p.muted;
        g.setColour (loopColour.withAlpha (.25f));
        g.fillRect (Rect (start, 6, std::max (2.0f, end - start), 8));
        g.setColour (loopColour);
        g.fillRoundedRectangle (Rect (start, 1, 9, 16), 3);
        g.fillRoundedRectangle (Rect (end - 9, 1, 9, 16), 3);
        rowNotes.clear();
        slots.clear();
        if (editingRow >= 0)
        {
            const auto& row = c.rows[static_cast<size_t> (editingRow)];
            double beat = (row.offset - 8) * .25 * PluginProcessor::pulseQuartersForIndex (c.p.getPulseIndex());
            for (size_t s = 0; s < row.steps.size(); ++s)
            {
                const auto& step = row.steps[s];
                const double length = (step.timing + 1) * .25 * PluginProcessor::pulseQuartersForIndex (c.p.getPulseIndex());
                const auto b = noteBounds (beat, beat + length * step.duration, step.note);
                rowNotes.emplace_back (b, static_cast<int> (s));
                const auto span = Rect (beatX (beat), 30, static_cast<float> (length * pixelsPerQuarter), getHeight() - 30.0f);
                slots.emplace_back (span, static_cast<int> (s));
                const auto accent = p.rows[static_cast<size_t> (editingRow)];
                g.setColour (accent.withAlpha (step.muted || step.skipped ? .2f : .2f + .8f * step.velocity / 127.0f));
                g.fillRoundedRectangle (b, 2);
                g.setColour (c.selection[static_cast<size_t> (editingRow)][s] ? p.text : accent.brighter (.2f));
                g.drawRoundedRectangle (b, 2, 1);
                if (drawVelocity)
                {
                    const auto bar = span.withTop (span.getBottom() - span.getHeight() * step.velocity / 127.0f).reduced (1, 0);
                    g.setColour (accent.withAlpha (.45f));
                    g.fillRect (bar);
                    g.setColour (accent);
                    g.drawHorizontalLine (juce::roundToInt (bar.getY()), bar.getX(), bar.getRight());
                }
                beat += length;
            }
        }
        else
            for (const auto& event : notes)
            {
                const auto b = noteBounds (event.start, event.end, event.note);
                const auto accent = p.rows[static_cast<size_t> (event.row)];
                g.setColour (accent.withAlpha (.25f + .6f * event.velocity / 127.0f));
                g.fillRoundedRectangle (b, 2);
                g.setColour (accent.brighter (.25f).withAlpha (.55f));
                g.drawRoundedRectangle (b, 2, 1);
            }
        const double playback = c.p.getPlaybackBeat();
        if (playback >= 0)
        {
            g.setColour (p.accent);
            g.drawVerticalLine (juce::roundToInt (beatX (playback)), 0, static_cast<float> (getHeight()));
        }
        g.setColour (p.border);
        g.drawRoundedRectangle (bounds.reduced (.5f), 12, 1);
    }
    void Preview::mouseDown (const juce::MouseEvent& e)
    {
        if (isShowing())
            grabKeyboardFocus();
        origin = e.position;
        initialStart = c.p.getLoopBraceStartQuarters();
        initialEnd = c.p.getLoopBraceEndQuarters();
        dragKind = 0;
        dragStep = -1;
        baseline = readRows (c.p);
        lastStroke = e.position;
        moveInstead = false;
        if (editingRow >= 0 && drawVelocity && e.position.y >= 30)
        {
            c.begin();
            dragKind = 6;
            mouseDrag (e);
            return;
        }
        if (e.position.y < 30 && editingRow < 0)
        {
            dragKind = std::abs (e.position.x - beatX (initialStart)) < 12 ? 1 : std::abs (e.position.x - beatX (initialEnd)) < 12 ? 2
                                                                                                                                   : 3;
            c.begin();
        }
        else if (editingRow >= 0)
            for (const auto& [b, step] : rowNotes)
            {
                if (!b.expanded (2).contains (e.position))
                    continue;
                dragStep = step;
                initialStep = c.rows[static_cast<size_t> (editingRow)].steps[static_cast<size_t> (step)];
                dragKind = e.position.x > b.getRight() - 7 ? 5 : e.position.x < b.getX() + 5 && step > 0 ? 7
                                                                                                         : 4;
                if (!e.mods.isShiftDown())
                    c.clearSelection();
                c.selection[static_cast<size_t> (editingRow)].set (static_cast<size_t> (step));
                c.begin();
                break;
            }
        repaint();
    }
    void Preview::mouseDrag (const juce::MouseEvent& e)
    {
        if (!c.gestureActive())
            return;
        const double delta = std::round ((xBeat (e.position.x) - xBeat (origin.x)) * 2) / 2;
        if (dragKind == 1)
            c.p.setLoopBraceStartQuarters (juce::jlimit (0.0, initialEnd - .5, initialStart + delta));
        if (dragKind == 2)
            c.p.setLoopBraceEndQuarters (std::max (initialStart + .5, initialEnd + delta));
        if (dragKind == 3)
        {
            const double start = std::max (0.0, initialStart + delta);
            c.p.setLoopBraceEndQuarters (start + initialEnd - initialStart);
            c.p.setLoopBraceStartQuarters (start);
        }
        if (dragKind == 6)
        {
            const float left = std::min (lastStroke.x, e.position.x), right = std::max (lastStroke.x, e.position.x);
            for (const auto& [bounds, step] : slots)
            {
                if (bounds.getRight() < left || bounds.getX() > right)
                    continue;
                const float x = juce::jlimit (left, right, bounds.getCentreX());
                const float t = std::abs (e.position.x - lastStroke.x) < .001f ? 1 : (x - lastStroke.x) / (e.position.x - lastStroke.x);
                const float y = lastStroke.y + t * (e.position.y - lastStroke.y);
                c.p.setPhraseStepVelocity (editingRow, step, juce::jlimit (0, 127, juce::roundToInt ((1 - (y - 30) / std::max (1, getHeight() - 30)) * 127)));
            }
            lastStroke = e.position;
        }
        if (dragKind == 4)
        {
            const float h = (getHeight() - 30.0f) / static_cast<float> (highNote - lowNote + 1);
            const int degrees = juce::roundToInt ((origin.y - e.position.y) / h);
            c.rows = baseline;
            c.editSelected ([&] (Step& step) { step.note = scaleNote (snapNote (step.note, c.p.getPatternScaleRoot (c.p.getViewPatternSlot()), c.p.getPatternScaleModeIndex (c.p.getViewPatternSlot())), degrees, c.p.getPatternScaleRoot (c.p.getViewPatternSlot()), c.p.getPatternScaleModeIndex (c.p.getViewPatternSlot())); });
            if (std::abs (e.position.x - origin.x) > std::abs (e.position.y - origin.y) && std::abs (e.position.x - origin.x) >= 4)
            {
                moveInstead = true;
                if (dragStep > 0 && !e.mods.isAltDown())
                {
                    const int previous = baseline[static_cast<size_t> (editingRow)].steps[static_cast<size_t> (dragStep - 1)].timing;
                    const double unit = .25 * PluginProcessor::pulseQuartersForIndex (c.p.getPulseIndex()) * pixelsPerQuarter;
                    const int next = previous + juce::roundToInt ((e.position.x - origin.x) / unit);
                    const bool crossed = origin.x - e.position.x >= (previous + 1) * unit;
                    if (!crossed && next <= 15)
                    {
                        c.p.setPhraseStepTimingMultiplier (editingRow, dragStep - 1, std::max (0, next));
                        moveInstead = false;
                    }
                }
            }
        }
        if (dragKind == 7 && dragStep > 0)
        {
            const double unit = .25 * PluginProcessor::pulseQuartersForIndex (c.p.getPulseIndex()) * pixelsPerQuarter;
            const int previous = baseline[static_cast<size_t> (editingRow)].steps[static_cast<size_t> (dragStep - 1)].timing;
            c.p.setPhraseStepTimingMultiplier (editingRow, dragStep - 1, juce::jlimit (0, 15, previous + juce::roundToInt ((e.position.x - origin.x) / unit)));
        }
        if (dragKind == 5)
        {
            const double length = (initialStep.timing + 1) * .25 * PluginProcessor::pulseQuartersForIndex (c.p.getPulseIndex());
            c.p.setPhraseStepDurationFraction (editingRow, dragStep, juce::jlimit (0.0, 1.0, initialStep.duration + (xBeat (e.position.x) - xBeat (origin.x)) / length));
        }
        c.rows = readRows (c.p);
        repaint();
    }
    void Preview::mouseUp (const juce::MouseEvent& e)
    {
        if (moveInstead && c.gestureActive() && editingRow >= 0)
        {
            int insertion = 0;
            for (const auto& [bounds, step] : slots)
                if (e.position.x >= bounds.getCentreX())
                    insertion = step + 1;
            c.moveSelected (editingRow, dragStep, editingRow, insertion, e.mods.isAltDown());
        }
        c.commit();
        dragKind = 0;
        if (changed)
            changed();
    }
    void Preview::mouseDoubleClick (const juce::MouseEvent& e)
    {
        if (editingRow >= 0)
            for (const auto& [b, step] : rowNotes)
                if (b.contains (e.position))
                {
                    if (inspect)
                        inspect (editingRow, step);
                    return;
                }
        if (editingRow >= 0 && e.position.y >= 30)
        {
            int insertion = 0;
            for (const auto& [bounds, step] : slots)
                if (e.position.x >= bounds.getRight())
                    insertion = step + 1;
            const float h = (getHeight() - 30.0f) / static_cast<float> (highNote - lowNote + 1);
            const int note = snapNote (highNote - static_cast<int> ((e.position.y - 30) / h), c.p.getPatternScaleRoot (c.p.getViewPatternSlot()), c.p.getPatternScaleModeIndex (c.p.getViewPatternSlot()));
            c.begin();
            c.p.insertPhraseStep (editingRow, insertion);
            c.p.setPhraseNote (editingRow, insertion, note);
            c.commit();
            if (changed)
                changed();
            return;
        }
        fit();
    }
    void Preview::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
    {
        if (e.mods.isCommandDown())
            pixelsPerQuarter = juce::jlimit (7.0, 224.0, pixelsPerQuarter * (wheel.deltaY > 0 ? 1.1 : .9));
        else if (e.mods.isShiftDown() || std::abs (wheel.deltaX) > std::abs (wheel.deltaY))
            firstQuarter = std::max (0.0, firstQuarter - (wheel.deltaX + wheel.deltaY) * visibleQuarters);
        else
        {
            const int delta = wheel.deltaY > 0 ? 1 : -1;
            if (lowNote + delta >= 0 && highNote + delta <= 127)
            {
                lowNote += delta;
                highNote += delta;
            }
        }
        repaint();
    }
}
