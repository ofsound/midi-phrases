#include "PhrasesPreview.h"
namespace phrases::ui
{
    namespace
    {
        constexpr double openNoteEnd = std::numeric_limits<double>::infinity();

        void applyNoteEdge (std::vector<PreviewNote>& notes, std::vector<size_t>& openNotes,
                            const PluginProcessor::PlaybackEvent& event)
        {
            if (event.velocity > 0)
            {
                if (event.row >= 0 && event.row < PluginProcessor::phraseRowCount)
                {
                    openNotes.push_back (notes.size());
                    notes.push_back ({ event.transportBeat, openNoteEnd, event.note, event.velocity, event.row, event.channel, event.step });
                }
                return;
            }

            const auto held = std::find_if (openNotes.begin(), openNotes.end(), [&](size_t index) {
                const auto& note = notes[index];
                return note.channel == event.channel && note.note == event.note;
            });
            if (held != openNotes.end())
            {
                notes[*held].end = event.transportBeat;
                openNotes.erase (held);
            }
        }

        // The scheduler can enqueue a future note-off before an earlier note-on
        // in the same block. Match the order in which the MIDI buffer plays them.
        template <typename Position, typename Edge>
        void visitPlaybackBlocks (std::vector<PluginProcessor::PlaybackEvent>& events, Position position, Edge edge)
        {
            for (auto block = events.begin(); block != events.end();)
            {
                jassert (block->channel == 0);
                position (*block);
                const auto first = std::next (block);
                const auto end = std::find_if (first, events.end(), [](const auto& event) { return event.channel == 0; });
                std::stable_sort (first, end, [](const auto& a, const auto& b) { return a.transportBeat < b.transportBeat; });
                for (auto event = first; event != end; ++event)
                    edge (*event);
                block = end;
            }
        }
    }

    std::vector<PreviewNote> renderPreview (PluginProcessor& source, double endQuarter, double firstQuarter)
    {
        const bool looping = source.isLoopBraceEnabled();
        const double loopStart = looping ? source.getLoopBraceStartQuarters() : 0;
        const double startBeat = std::max (loopStart, firstQuarter - 16.0);
        const double endBeat = std::min ({ endQuarter, std::max (loopStart, firstQuarter) + 256.0,
                                          looping ? source.getLoopBraceEndQuarters() : endQuarter });
        if (endBeat <= startBeat)
            return {};

        juce::MemoryBlock state;
        source.getStateInformation (state);
        auto renderer = std::make_unique<PluginProcessor>();
        renderer->setStateInformation (state.getData(), static_cast<int> (state.getSize()));
        const int pattern = source.getViewPatternSlot();
        renderer->setCurrentPatternSlot (pattern);
        renderer->setPlaybackMonitoring (true);
        struct Clock : juce::AudioPlayHead
        {
            PositionInfo info;
            juce::Optional<PositionInfo> getPosition() const override { return info; }
        } clock;
        const double bpm = source.hasStandaloneTransport() ? source.getStandaloneTempoBpm() : 120.0;
        clock.info.setBpm (bpm);
        clock.info.setIsPlaying (true);
        renderer->setPlayHead (&clock);
        constexpr int block = 128;
        constexpr double rate = 8000;
        const double ppqPerSample = bpm / (60.0 * rate);
        renderer->prepareToPlay (rate, block);
        renderer->setOfflinePreviewStartBeat (startBeat);
        juce::AudioBuffer<float> audio (2, block);
        juce::MidiBuffer midi;
        std::vector<PreviewNote> output;
        std::vector<size_t> openNotes;
        output.reserve (2048);
        std::vector<PluginProcessor::PlaybackEvent> events;
        const int samples = static_cast<int> (std::ceil ((endBeat - startBeat) / ppqPerSample));
        for (int start = 0; start < samples; start += block)
        {
            midi.clear();
            clock.info.setPpqPosition (startBeat + start * ppqPerSample);
            renderer->processBlock (audio, midi);
            renderer->drainPlaybackEvents (events);
            visitPlaybackBlocks (events, [](const auto&) {}, [&](const auto& event) {
                if (event.transportBeat < endBeat && (event.velocity == 0 || event.pattern == pattern))
                    applyNoteEdge (output, openNotes, event);
            });
        }
        renderer->releaseResources();
        renderer->setPlayHead (nullptr);
        for (auto& note : output)
            note.end = std::min (note.end, endBeat);
        std::erase_if (output, [firstQuarter](const auto& note) { return note.end <= firstQuarter; });
        return output;
    }

    Preview::Preview (Controller& controller, Palette& palette) : c (controller), p (palette)
    {
        setWantsKeyboardFocus (true);
        c.p.drainPlaybackEvents (playbackEvents);
        c.p.setPlaybackMonitoring (true);
    }

    Preview::~Preview()
    {
        c.p.setPlaybackMonitoring (false);
    }

    bool Preview::isLive() const
    {
        return playbackPosition.scheduleBeat >= 0 && playbackPosition.pattern == c.p.getViewPatternSlot();
    }

    double Preview::playheadBeat() const
    {
        return isLive() ? playbackPosition.scheduleBeat : -1;
    }

    double Preview::viewStart() const
    {
        return isLive() ? liveFirstQuarter : firstQuarter;
    }

    void Preview::updatePlayback()
    {
        playingSteps = {};
        c.p.drainPlaybackEvents (playbackEvents);
        if (c.p.getPlaybackBeat() < 0)
        {
            playbackPosition = {};
            liveNotes.clear();
            openLiveNotes.clear();
            if (useExtraForecast)
                updateForecast();
            return;
        }

        const bool wasLive = isLive();
        const bool looping = c.p.isLoopBraceEnabled();
        visitPlaybackBlocks (playbackEvents, [&](const auto& position) {
            const double originChange = (position.transportBeat - position.scheduleBeat)
                - (playbackPosition.transportBeat - playbackPosition.scheduleBeat);
            if (position.reset || position.pattern != playbackPosition.pattern
                || (!looping && std::abs (originChange) > .001))
            {
                liveNotes.clear();
                openLiveNotes.clear();
            }
            playbackPosition = position;
        }, [&](const auto& event) {
            if (event.velocity == 0 || event.pattern == playbackPosition.pattern)
            {
                applyNoteEdge (liveNotes, openLiveNotes, event);
                if (event.velocity > 0)
                {
                    lowNote = std::min (lowNote, event.note);
                    highNote = std::max (highNote, event.note);
                }
            }
        });

        if (!isLive())
            return;
        if (!wasLive)
        {
            liveFirstQuarter = firstQuarter;
            followPlayback = true;
        }

        const double beat = playbackPosition.scheduleBeat;
        const double pageWidth = std::max (1.0, (getWidth() - pitchGutterWidth) / pixelsPerQuarter());
        if (followPlayback && (beat < liveFirstQuarter || beat >= liveFirstQuarter + pageWidth))
        {
            const double start = looping ? c.p.getLoopBraceStartQuarters() : 0;
            liveFirstQuarter = start + std::floor (std::max (0.0, beat - start) / pageWidth) * pageWidth;
        }

        updateForecast();

        const double now = playbackPosition.transportBeat;
        constexpr double flashLength = .22;
        constexpr size_t maxLiveNotes = 8192;
        const auto previousSize = liveNotes.size();
        std::erase_if (liveNotes, [oldest = now - std::max (16.0, 2 * pageWidth)](const auto& note) { return note.end < oldest; });
        if (liveNotes.size() > maxLiveNotes)
            liveNotes.erase (liveNotes.begin(), liveNotes.end() - static_cast<std::ptrdiff_t> (maxLiveNotes));
        if (previousSize != liveNotes.size())
        {
            openLiveNotes.clear();
            for (size_t i = 0; i < liveNotes.size(); ++i)
                if (std::isinf (liveNotes[i].end))
                    openLiveNotes.push_back (i);
        }
        for (const auto& note : liveNotes)
        {
            if (note.step < 0 || note.step >= PluginProcessor::maxPhraseStepsPerRow || note.start > now)
                continue;
            auto& activity = playingSteps[static_cast<size_t> (note.row)][static_cast<size_t> (note.step)];
            activity.sounding |= now < note.end;
            activity.flash = std::max (activity.flash, static_cast<float> (1 - (now - note.start) / flashLength));
        }
    }
    double Preview::fittedPixelsPerQuarter() const
    {
        return std::max (28.0, (getWidth() - static_cast<double> (pitchGutterWidth)) / visibleQuarters);
    }
    double Preview::pixelsPerQuarter() const
    {
        return manualPixelsPerQuarter.value_or (fittedPixelsPerQuarter());
    }
    float Preview::beatX (double beat) const { return pitchGutterWidth + static_cast<float> ((beat - viewStart()) * pixelsPerQuarter()); }
    double Preview::xBeat (float x) const { return viewStart() + (x - pitchGutterWidth) / pixelsPerQuarter(); }
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
        const bool looping = c.p.isLoopBraceEnabled();
        const double start = looping ? c.p.getLoopBraceStartQuarters() : 0;
        if (looping)
            visibleQuarters = std::max (visibleQuarters, c.p.getLoopBraceEndQuarters() - start);
        visibleQuarters = std::min (128.0, visibleQuarters);
        notes = renderPreview (c.p, start + visibleQuarters);
        forecastStart = forecastEnd = 0;
        if (firstQuarter + visibleQuarters <= start)
            firstQuarter = start;
        fitPitch();
        updateForecast();
        repaint();
    }

    void Preview::updateForecast()
    {
        const bool looping = c.p.isLoopBraceEnabled();
        const double start = looping ? c.p.getLoopBraceStartQuarters() : 0;
        const double left = std::max (start, viewStart());
        const double right = looping ? std::min (c.p.getLoopBraceEndQuarters(), xBeat (static_cast<float> (getWidth())))
                                     : xBeat (static_cast<float> (getWidth()));
        useExtraForecast = right > start + visibleQuarters + .001 && right > left;
        if (!useExtraForecast || (left >= forecastStart && right <= forecastEnd))
            return;

        forecastStart = left;
        forecastEnd = right + visibleQuarters;
        extraForecast = renderPreview (c.p, forecastEnd, forecastStart);
        for (const auto& note : extraForecast)
        {
            lowNote = std::min (lowNote, note.note);
            highNote = std::max (highNote, note.note);
        }
    }

    void Preview::fit()
    {
        firstQuarter = c.p.isLoopBraceEnabled() ? c.p.getLoopBraceStartQuarters() : 0;
        liveFirstQuarter = firstQuarter;
        followPlayback = true;
        manualPixelsPerQuarter.reset();
        fitPitch();
        updateForecast();
        repaint();
    }
    void Preview::fitPitch()
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
            const float rollRight = static_cast<float> (getWidth());
            g.setColour (black ? p.app : p.raised.withAlpha (.7f));
            g.fillRect (Rect (0, y, rollRight, h));
            g.setColour (p.border.withAlpha (.35f));
            g.drawHorizontalLine (juce::roundToInt (y), 0, rollRight);
            if (pitch == 0)
                text (g, noteName (n), { 0, y, 40, h }, p.muted, 10, false, juce::Justification::centredRight);
        }
        g.reduceClipRegion (juce::Rectangle<int> (juce::roundToInt (pitchGutterWidth), 0, getWidth(), getHeight()));
        for (int beat = static_cast<int> (std::ceil (viewStart())); beat <= xBeat (static_cast<float> (getWidth())); ++beat)
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
        const double beat = playheadBeat();
        const bool live = beat >= 0;
        const double originBeat = live ? playbackPosition.transportBeat - beat : 0;
        const double left = viewStart();
        const double right = xBeat (static_cast<float> (getWidth()));
        const double loopStart = c.p.isLoopBraceEnabled() ? c.p.getLoopBraceStartQuarters() : 0;
        const auto drawNote = [&](const PreviewNote& event, double startBeat, double endBeat, bool active) {
            startBeat = std::max (startBeat, loopStart);
            if (endBeat <= left || startBeat >= right || startBeat > endBeat)
                return;
            const auto b = noteBounds (std::max (left, startBeat), std::min (right, endBeat), event.note);
            const auto accent = p.rows[static_cast<size_t> (event.row)];
            g.setColour (active ? accent.brighter (.35f) : accent.withAlpha (.25f + .6f * event.velocity / 127.0f));
            g.fillRoundedRectangle (b, 2);
            g.setColour (accent.brighter (.25f).withAlpha (.55f));
            g.drawRoundedRectangle (b, 2, 1);
        };
        const auto& forecast = upcomingNotes();
        constexpr double matchingTolerance = .002;
        for (const auto& note : forecast)
            if (!live || note.start > beat + matchingTolerance)
                drawNote (note, note.start, note.end, false);

        if (live)
            for (const auto& note : liveNotes)
            {
                const double onset = note.start - originBeat;
                const bool active = playbackPosition.transportBeat < note.end;
                double endBeat = active ? beat : note.end - originBeat;
                if (active)
                {
                    const auto prediction = std::find_if (forecast.begin(), forecast.end(), [&](const auto& next) {
                        return next.channel == note.channel && next.note == note.note && next.row == note.row
                            && std::abs (next.start - onset) < matchingTolerance;
                    });
                    if (prediction != forecast.end())
                        endBeat = std::max (endBeat, prediction->end);
                }
                drawNote (note, onset, endBeat, active);
            }
        if (live && beat >= left && beat <= right)
        {
            g.setColour (p.accent);
            g.drawVerticalLine (juce::roundToInt (beatX (beat)), 0, static_cast<float> (getHeight()));
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
        {
            const auto maxScale = std::max (224.0, fittedPixelsPerQuarter());
            manualPixelsPerQuarter = juce::jlimit (7.0, maxScale, pixelsPerQuarter() * (wheel.deltaY > 0 ? 1.1 : .9));
        }
        else if (e.mods.isShiftDown() || std::abs (wheel.deltaX) > std::abs (wheel.deltaY))
        {
            auto& start = isLive() ? liveFirstQuarter : firstQuarter;
            start = std::max (0.0, start - (wheel.deltaX + wheel.deltaY) * visibleQuarters);
            followPlayback = false;
        }
        else
        {
            const int delta = wheel.deltaY > 0 ? 1 : -1;
            if (lowNote + delta >= 0 && highNote + delta <= 127)
            {
                lowNote += delta;
                highNote += delta;
            }
        }
        updateForecast();
        repaint();
    }
}
