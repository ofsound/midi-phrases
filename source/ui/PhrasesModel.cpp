#include "PhrasesModel.h"

namespace phrases::ui
{
    Rows readRows (const PluginProcessor& p)
    {
        Rows result;
        for (int r = 0; r < 4; ++r)
        {
            auto& row = result[static_cast<size_t> (r)];
            row.offset = p.getPhraseRowTimingOffset (r);
            row.channel = p.getPhraseRowMidiChannel (r);
            row.muted = p.isPhraseRowMuted (r);
            for (int s = 0; s < p.getPhraseRowStepCount (r); ++s)
                row.steps.push_back ({ p.getPhraseNote (r, s), p.getPhraseStepTimingMultiplier (r, s), p.getPhraseStepVelocity (r, s), p.getPhraseStepProbability (r, s), p.getPhraseStepCycle (r, s), p.getPhraseStepCycleOffset (r, s), p.getPhraseStepDurationFraction (r, s), p.isPhraseStepMuted (r, s), p.isPhraseStepSkipped (r, s) });
        }
        return result;
    }
    void writeRow (PluginProcessor& p, int r, const Row& row)
    {
        std::array<int, 64> notes {}, timing {}, velocity {}, muted {}, skipped {}, probability {}, cycle {}, mask {};
        std::array<double, 64> duration {};
        const auto count = std::min (row.steps.size(), size_t { 64 });
        for (size_t s = 0; s < count; ++s)
        {
            const auto& step = row.steps[s];
            notes[s] = step.note;
            timing[s] = step.timing;
            velocity[s] = step.velocity;
            muted[s] = step.muted;
            skipped[s] = step.skipped;
            probability[s] = step.probability;
            cycle[s] = step.cycle;
            mask[s] = step.mask;
            duration[s] = step.duration;
        }
        p.replacePhraseRowSteps (r, static_cast<int> (count), notes, timing, duration, velocity, muted, skipped, probability, cycle, mask);
        if (p.isPhraseRowMuted (r) != row.muted)
            p.setPhraseRowMuted (r, row.muted);
        if (p.getPhraseRowTimingOffset (r) != row.offset)
            p.setPhraseRowTimingOffset (r, row.offset);
        if (p.getPhraseRowMidiChannel (r) != row.channel)
            p.setPhraseRowMidiChannel (r, row.channel);
    }

    bool stepActive (const Row& row, size_t index, double beat, int pulse, int swing, int subdivision)
    {
        constexpr double epsilon = 1.0e-8;
        if (beat < 0 || row.muted || index >= row.steps.size())
            return false;
        const auto& step = row.steps[index];
        if (step.skipped || step.muted || step.velocity <= 0 || step.duration <= 0 || step.probability <= 0)
            return false;
        const double unit = .25 * PluginProcessor::pulseQuartersForIndex (pulse);
        double start = 0, cycle = 0;
        for (size_t s = 0; s < row.steps.size(); ++s)
        {
            if (s == index)
                start = cycle;
            if (!row.steps[s].skipped)
                cycle += (row.steps[s].timing + 1) * unit;
        }
        if (cycle <= 0)
            return false;
        start += (row.offset - 8) * unit;
        if (beat - start < -epsilon)
            return false;
        const double iteration = std::floor ((beat - start + epsilon) / cycle);
        const auto count = static_cast<int64_t> (iteration - std::ceil ((-start - epsilon) / cycle));
        const int phase = static_cast<int> ((count % step.cycle + step.cycle) % step.cycle);
        if (step.cycle > 1 && (phase >= 30 || ((step.mask == 0 ? 1 : step.mask) & (1 << phase)) == 0))
            return false;
        const uint32_t hash = static_cast<uint32_t> (index) * 2654435761u + static_cast<uint32_t> (count) * 1597334677u;
        if (step.probability < 100 && hash % 100 >= static_cast<uint32_t> (step.probability))
            return false;
        double onset = iteration * cycle + start;
        const double swingUnit = PluginProcessor::pulseQuartersForIndex (pulse) * PluginProcessor::swingSubdivisionForIndex (subdivision);
        if (static_cast<int64_t> (std::floor ((onset + epsilon) / swingUnit)) % 2 != 0)
            onset += swingUnit * .5 * swing / 100;
        return beat >= onset - epsilon && beat < onset + (step.timing + 1) * unit * step.duration - epsilon;
    }

    namespace
    {
        const std::array<std::vector<int>, 16> intervals { { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 }, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 9, 10 }, { 0, 1, 3, 5, 7, 8, 10 }, { 0, 2, 4, 6, 7, 9, 11 }, { 0, 2, 4, 5, 7, 9, 10 }, { 0, 1, 3, 5, 6, 8, 10 }, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 2, 3, 5, 7, 9, 11 }, { 0, 2, 4, 7, 9 }, { 0, 3, 5, 7, 10 }, { 0, 3, 5, 6, 7, 10 }, { 0, 2, 4, 6, 8, 10 }, { 0, 2, 3, 5, 6, 8, 9, 11 }, { 0, 1, 3, 4, 6, 7, 9, 10 } } };
        bool inScale (int note, int root, int mode)
        {
            const auto& values = intervals[static_cast<size_t> (juce::jlimit (0, 15, mode))];
            return std::find (values.begin(), values.end(), (note - root + 132) % 12) != values.end();
        }
    }
    int scaleNote (int note, int degrees, int root, int mode)
    {
        const int direction = degrees < 0 ? -1 : 1;
        note = juce::jlimit (0, 127, note);
        for (int left = std::abs (degrees); left > 0 && note + direction >= 0 && note + direction <= 127;)
        {
            note += direction;
            if (inScale (note, root, mode))
                --left;
        }
        return note;
    }
    int snapNote (int note, int root, int mode)
    {
        note = juce::jlimit (0, 127, note);
        for (int d = 0; d <= 12; ++d)
        {
            if (note - d >= 0 && inScale (note - d, root, mode))
                return note - d;
            if (note + d <= 127 && inScale (note + d, root, mode))
                return note + d;
        }
        return note;
    }
    juce::String noteName (int note)
    {
        static constexpr const char* names[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return juce::String (names[juce::jlimit (0, 127, note) % 12]) + juce::String (note / 12 - 2);
    }
    juce::MemoryBlock Controller::state()
    {
        juce::MemoryBlock b;
        p.getStateInformation (b);
        return b;
    }
    bool Controller::begin()
    {
        refresh();
        if (gesture)
            return false;
        before = state();
        gesturePattern = p.getViewPatternSlot();
        generation = p.getDocumentGeneration();
        gesture = true;
        rows = readRows (p);
        return true;
    }
    bool Controller::commit()
    {
        if (!gesture)
            return false;
        gesture = false;
        auto after = state();
        rows = readRows (p);
        if (generation != p.getDocumentGeneration() || after == before)
            return false;
        undoHistory.push_back ({ before, std::move (after) });
        if (undoHistory.size() > 100)
            undoHistory.pop_front();
        redoHistory.clear();
        return true;
    }
    void Controller::cancel()
    {
        if (!gesture)
            return;
        gesture = false;
        if (generation == p.getDocumentGeneration() && gesturePattern == p.getViewPatternSlot() && readRows (p) == rows)
            p.setStateInformation (before.getData(), static_cast<int> (before.getSize()));
        knownGeneration = p.getDocumentGeneration();
        rows = readRows (p);
    }
    namespace
    {
        // Merge only the fields owned by an undo entry. Rows are atomic because inserting
        // a step changes the meaning of every later index. Unrelated recording and project
        // metadata survive undo, while edits to the same row cause the entry to be rejected.
        bool applyHistory (juce::ValueTree current, const juce::ValueTree& expected, const juce::ValueTree& desired)
        {
            if (!current.isValid() || current.getType() != expected.getType() || expected.getType() != desired.getType())
                return false;
            if (expected.isEquivalentTo (desired))
                return true;
            if (expected.hasType ("Row"))
            {
                if (!current.isEquivalentTo (expected))
                    return false;
                current.copyPropertiesAndChildrenFrom (desired, nullptr);
                return true;
            }
            juce::Array<juce::Identifier> properties;
            for (int i = 0; i < expected.getNumProperties(); ++i)
                properties.addIfNotAlreadyThere (expected.getPropertyName (i));
            for (int i = 0; i < desired.getNumProperties(); ++i)
                properties.addIfNotAlreadyThere (desired.getPropertyName (i));
            for (const auto& key : properties)
            {
                if (expected[key] == desired[key] && expected.hasProperty (key) == desired.hasProperty (key))
                    continue;
                if (current[key] != expected[key] || current.hasProperty (key) != expected.hasProperty (key))
                    return false;
                if (desired.hasProperty (key))
                    current.setProperty (key, desired[key], nullptr);
                else
                    current.removeProperty (key, nullptr);
            }
            if (current.getNumChildren() != expected.getNumChildren() || expected.getNumChildren() != desired.getNumChildren())
                return false;
            for (int i = 0; i < expected.getNumChildren(); ++i)
                if (!applyHistory (current.getChild (i), expected.getChild (i), desired.getChild (i)))
                    return false;
            return true;
        }
        juce::ValueTree historyTree (const juce::MemoryBlock& data)
        {
            return juce::ValueTree::fromXml (juce::String::createStringFromData (data.getData(), static_cast<int> (data.getSize())));
        }
    }
    bool Controller::restore (std::deque<Entry>& from, std::deque<Entry>& to, bool forward)
    {
        if (gesture || from.empty())
            return false;
        if (knownGeneration != p.getDocumentGeneration())
        {
            documentChanged();
            return false;
        }
        const auto& entry = from.back();
        auto merged = historyTree (state());
        if (!applyHistory (merged, historyTree (forward ? entry.before : entry.after), historyTree (forward ? entry.after : entry.before)))
        {
            documentChanged();
            return false;
        }
        const auto xml = merged.toXmlString();
        p.setStateInformation (xml.toRawUTF8(), static_cast<int> (xml.getNumBytesAsUTF8()));
        knownGeneration = p.getDocumentGeneration();
        to.push_back (entry);
        from.pop_back();
        rows = readRows (p);
        clearSelection();
        return true;
    }
    bool Controller::undo() { return restore (undoHistory, redoHistory, false); }
    bool Controller::redo() { return restore (redoHistory, undoHistory, true); }
    bool Controller::refresh()
    {
        if (knownGeneration != p.getDocumentGeneration())
        {
            documentChanged();
            return true;
        }
        auto latest = readRows (p);
        if (rows == latest)
            return false;
        // Changes not produced by our gesture (recording, host restore) invalidate stale drafts.
        if (gesture)
        {
            gesture = false;
            undoHistory.clear();
            redoHistory.clear();
        }
        rows = std::move (latest);
        return true;
    }
    void Controller::documentChanged()
    {
        knownGeneration = p.getDocumentGeneration();
        gesture = false;
        undoHistory.clear();
        redoHistory.clear();
        clearSelection();
        rows = readRows (p);
    }
    void Controller::selectAll (int row)
    {
        for (int r = 0; r < 4; ++r)
            if (row < 0 || r == row)
                for (size_t s = 0; s < rows[static_cast<size_t> (r)].steps.size(); ++s)
                    selection[static_cast<size_t> (r)].set (s);
    }
    bool Controller::hasSelection() const
    {
        for (const auto& row : selection)
            if (row.any())
                return true;
        return false;
    }
    void Controller::editSelected (const std::function<void (Step&)>& edit, bool allIfEmpty)
    {
        if (!gestureActive())
            return;
        const bool all = allIfEmpty && !hasSelection();
        for (size_t r = 0; r < 4; ++r)
        {
            bool changed = false;
            for (size_t s = 0; s < rows[r].steps.size(); ++s)
                if (all || selection[r][s])
                {
                    edit (rows[r].steps[s]);
                    changed = true;
                }
            if (changed)
                writeRow (p, static_cast<int> (r), rows[r]);
        }
    }
    void Controller::removeSelected()
    {
        begin();
        for (size_t r = 0; r < 4; ++r)
        {
            for (int s = static_cast<int> (rows[r].steps.size()) - 1; s >= 0; --s)
                if (selection[r][static_cast<size_t> (s)])
                    rows[r].steps.erase (rows[r].steps.begin() + s);
            writeRow (p, static_cast<int> (r), rows[r]);
        }
        clearSelection();
        commit();
    }
    void Controller::moveSelected (int sourceRow, int sourceStep, int targetRow, int insertion, bool copy)
    {
        if (targetRow < 0 || targetRow >= 4 || sourceRow < 0 || sourceRow >= 4 || sourceStep < 0)
            return;
        if (!selection[static_cast<size_t> (sourceRow)][static_cast<size_t> (sourceStep)])
        {
            clearSelection();
            selection[static_cast<size_t> (sourceRow)].set (static_cast<size_t> (sourceStep));
        }
        auto draft = rows;
        std::vector<Step> moved;
        auto& target = draft[static_cast<size_t> (targetRow)].steps;
        insertion = juce::jlimit (0, static_cast<int> (target.size()), insertion);
        int removedBefore = 0;
        for (size_t r = 0; r < 4; ++r)
        {
            for (size_t s = 0; s < rows[r].steps.size(); ++s)
                if (selection[r][s])
                {
                    moved.push_back (rows[r].steps[s]);
                    if (!copy && static_cast<int> (r) == targetRow && static_cast<int> (s) < insertion)
                        ++removedBefore;
                }
            if (!copy)
                for (int s = static_cast<int> (draft[r].steps.size()) - 1; s >= 0; --s)
                    if (selection[r][static_cast<size_t> (s)])
                        draft[r].steps.erase (draft[r].steps.begin() + s);
        }
        if (target.size() + moved.size() > 64)
            return;
        insertion -= removedBefore;
        target.insert (target.begin() + insertion, moved.begin(), moved.end());
        begin();
        rows = std::move (draft);
        clearSelection();
        for (size_t s = 0; s < moved.size(); ++s)
            selection[static_cast<size_t> (targetRow)].set (static_cast<size_t> (insertion) + s);
        for (int r = 0; r < 4; ++r)
            writeRow (p, r, rows[static_cast<size_t> (r)]);
        commit();
    }
}
