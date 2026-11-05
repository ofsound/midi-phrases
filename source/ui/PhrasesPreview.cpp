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
    float Preview::pitchLaneHeight() const
    {
        return std::max (1.0f, getHeight() - 30.0f) / static_cast<float> (highNote - lowNote + 1);
    }
    Rect Preview::noteBounds (double start, double end, int note) const
    {
        const float lane = pitchLaneHeight();
        const float height = std::max (1.0f, lane - 2);
        return { beatX (start), 30 + static_cast<float> (highNote - note) * lane + (lane - height) * .5f, std::max (2.0f, beatX (end) - beatX (start) - 1), height };
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
    }
    void Preview::fit()
    {
        int low = 128, high = -1;
        for (const auto& event : notes)
        {
            low = std::min (low, event.note);
            high = std::max (high, event.note);
        }

        lowNote = 48;
        highNote = 72;
        if (high >= low)
        {
            lowNote = std::max (0, low - 1);
            highNote = std::min (127, high + 1);
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
        const float h = pitchLaneHeight();
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
        const double playback = c.p.getPlaybackBeat();
        // The overview shows one repeating window of the composition.
        const double displayBeat = playback < 0 ? -1.0 : std::fmod (playback, visibleQuarters);
        for (const auto& event : notes)
        {
            const bool active = displayBeat >= event.start && displayBeat < event.end;
            const auto b = noteBounds (event.start, event.end, event.note);
            const auto accent = p.rows[static_cast<size_t> (event.row)];
            g.setColour (active ? accent.brighter (.35f) : accent.withAlpha (.25f + .6f * event.velocity / 127.0f));
            g.fillRoundedRectangle (b, 2);
            g.setColour (accent.brighter (.25f).withAlpha (.55f));
            g.drawRoundedRectangle (b, 2, 1);
        }
        if (playback >= 0)
        {
            g.setColour (p.accent);
            g.drawVerticalLine (juce::roundToInt (beatX (displayBeat)), 0, static_cast<float> (getHeight()));
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
        if (e.position.y < 30)
        {
            dragKind = std::abs (e.position.x - beatX (initialStart)) < 12 ? 1 : std::abs (e.position.x - beatX (initialEnd)) < 12 ? 2
                                                                                                                                   : 3;
            c.begin();
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
        repaint();
    }
    void Preview::mouseUp (const juce::MouseEvent&)
    {
        c.commit();
        dragKind = 0;
        if (changed)
            changed();
    }
    void Preview::mouseDoubleClick (const juce::MouseEvent&)
    {
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
