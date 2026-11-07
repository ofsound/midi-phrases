#include "PhrasesCycleEditor.h"

namespace phrases::ui
{
    CycleEditor::CycleEditor (Controller& c, Palette& p) : controller (c), palette (p)
    {
        setName ("Step cycle");
        setTitle ("Step cycle pattern and length");
        setWantsKeyboardFocus (true);
        setTooltip ("Click a beat to toggle or extend the cycle. Drag an active beat to move it.");
    }

    const Step* CycleEditor::inspected() const
    {
        if (row < 0 || row >= static_cast<int> (controller.rows.size()) || step < 0)
            return nullptr;
        const auto& steps = controller.rows[static_cast<size_t> (row)].steps;
        return static_cast<size_t> (step) < steps.size() ? &steps[static_cast<size_t> (step)] : nullptr;
    }

    void CycleEditor::setInspectedStep (int nextRow, int nextStep)
    {
        if (row != nextRow || step != nextStep)
        {
            cancelEdit();
            row = nextRow;
            step = nextStep;
        }
        setVisible (inspected() != nullptr);
        repaint();
    }

    Rect CycleEditor::beatBounds (int index) const
    {
        constexpr float gap = 4.0f, inset = 8.0f;
        const float width = std::max (1.0f, (getWidth() - 2 * inset - (beatCount - 1) * gap) / beatCount);
        return { inset + static_cast<float> (index) * (width + gap), 28, width, 42 };
    }

    Rect CycleEditor::handleBounds (int cycle) const
    {
        const auto last = beatBounds (juce::jlimit (1, beatCount, cycle) - 1);
        return { last.getRight() - 5.0f, last.getY() - 3.0f, 12.0f, last.getHeight() + 6.0f };
    }

    int CycleEditor::beatAt (juce::Point<float> point) const
    {
        for (int i = 0; i < beatCount; ++i)
            if (beatBounds (i).contains (point))
                return i;
        return -1;
    }

    void CycleEditor::paint (juce::Graphics& g)
    {
        const auto* value = inspected();
        if (value == nullptr)
            return;

        const auto accent = palette.rows[static_cast<size_t> (row)];
        const int length = juce::jlimit (1, beatCount, value->cycle);
        const auto range = beatBounds (0).getUnion (beatBounds (length - 1)).expanded (3);
        g.setColour (accent.withAlpha (0.12f));
        g.fillRoundedRectangle (range, 6);
        g.setColour (accent.withAlpha (0.55f));
        g.drawRoundedRectangle (range, 6, 1);

        text (g, "Cycle  " + juce::String (value->cycle), { 8, 0, getWidth() - 16.0f, 20 },
              palette.text, 13, true, juce::Justification::centredLeft);
        const int mask = value->mask == 0 ? 1 : value->mask;
        const bool showMove = movingBeat && dropBeat >= 0 && dropBeat != pressedBeat;
        for (int i = 0; i < beatCount; ++i)
        {
            const bool inside = i < length;
            const bool active = showMove && i == dropBeat
                ? true : inside && (mask & (1 << i)) != 0 && (!showMove || i != pressedBeat);
            auto background = active ? accent : inside ? palette.raised : palette.app;
            if (i == hoveredBeat)
                background = background.brighter (0.15f);
            const auto bounds = beatBounds (i);
            g.setColour (background);
            g.fillRoundedRectangle (bounds, 4);
            if (!inside)
            {
                g.setColour (palette.borderSubtle);
                g.drawRoundedRectangle (bounds.reduced (0.5f), 4, 1);
            }
            text (g, juce::String (i + 1), bounds, active ? palette.app : inside ? palette.text : palette.faint, 13, true);
        }

        const auto handle = handleBounds (length);
        if (handleHovered || resizing || hasKeyboardFocus (true))
        {
            g.setColour (palette.app.withAlpha (0.85f));
            g.fillRoundedRectangle (handle, 4);
        }
        g.setColour (handleHovered || resizing ? accent : palette.text);
        for (int dot = -2; dot <= 2; ++dot)
            g.fillEllipse (handle.getCentreX() - 0.8f, handle.getCentreY() + static_cast<float> (dot) * 4.0f - 0.8f, 1.6f, 1.6f);
    }

    void CycleEditor::mouseMove (const juce::MouseEvent& e)
    {
        const auto* value = inspected();
        handleHovered = value != nullptr && handleBounds (value->cycle).contains (e.position);
        hoveredBeat = handleHovered ? -1 : beatAt (e.position);
        setMouseCursor (handleHovered ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::PointingHandCursor);
        setTooltip (handleHovered ? "Drag horizontally to set cycle length. Left and Right arrow keys also adjust it."
                                  : "Click to toggle a beat or extend the cycle. Drag an active beat to move it.");
        repaint();
    }

    void CycleEditor::mouseExit (const juce::MouseEvent&)
    {
        hoveredBeat = -1;
        handleHovered = false;
        repaint();
    }

    bool CycleEditor::beginEdit()
    {
        if (inspected() == nullptr || !controller.begin())
            return false;
        if (inspected() == nullptr)
        {
            controller.commit();
            return false;
        }
        if (!controller.hasSelection())
            controller.selection[static_cast<size_t> (row)].set (static_cast<size_t> (step));
        baseline = controller.rows;
        selection = controller.selection;
        return true;
    }

    bool CycleEditor::canEdit() const
    {
        return controller.gestureActive() && controller.selection == selection && inspected() != nullptr;
    }

    void CycleEditor::mouseDown (const juce::MouseEvent& e)
    {
        const auto* value = inspected();
        if (value == nullptr || !e.mods.isLeftButtonDown())
            return;

        const bool onHandle = handleBounds (value->cycle).contains (e.position);
        const int beat = beatAt (e.position);
        if ((!onHandle && beat < 0) || !beginEdit())
            return;
        if (isShowing())
            grabKeyboardFocus();

        resizing = onHandle;
        pressedBeat = onHandle ? -1 : beat;
        lastLength = inspected()->cycle;
        repaint();
    }

    void CycleEditor::resizeCycle (int length)
    {
        if (length == lastLength || !canEdit())
            return;
        lastLength = length;
        controller.rows = baseline;
        controller.editSelected ([length] (Step& target) {
            target.cycle = length;
            target.mask &= (1 << length) - 1;
            if (target.mask == 0)
                target.mask = 1;
        });
        repaint();
    }

    void CycleEditor::mouseDrag (const juce::MouseEvent& e)
    {
        if ((!resizing && pressedBeat < 0) || !canEdit())
            return;
        if (resizing)
        {
            if (e.getDistanceFromDragStart() < 3)
                return;
            const float stride = beatBounds (1).getX() - beatBounds (0).getX();
            const auto position = (e.position.x - beatBounds (0).getX()) / stride;
            resizeCycle (juce::jlimit (1, beatCount, static_cast<int> (std::ceil (position))));
            return;
        }

        const auto& value = baseline[static_cast<size_t> (row)].steps[static_cast<size_t> (step)];
        const int mask = value.mask == 0 ? 1 : value.mask;
        if (pressedBeat < 0 || pressedBeat >= value.cycle || (mask & (1 << pressedBeat)) == 0
            || e.getDistanceFromDragStart() < 4)
            return;
        movingBeat = true;
        const int target = beatAt (e.position);
        dropBeat = target < value.cycle ? target : -1;
        repaint();
    }

    void CycleEditor::mouseUp (const juce::MouseEvent& e)
    {
        if (!resizing && pressedBeat < 0)
            return;
        if (canEdit() && pressedBeat >= 0)
        {
            const int from = pressedBeat, to = beatAt (e.position);
            if (movingBeat && to >= 0 && to < inspected()->cycle && to != from)
            {
                controller.editSelected ([from, to] (Step& target) {
                    const int mask = target.mask == 0 ? 1 : target.mask;
                    if (from < target.cycle && to < target.cycle && (mask & (1 << from)) != 0)
                        target.mask = (mask & ~(1 << from)) | (1 << to);
                });
            }
            else if (!movingBeat && to == from && e.getDistanceFromDragStart() < 4)
            {
                const auto value = *inspected();
                const int length = std::max (value.cycle, from + 1);
                const int mask = value.mask == 0 ? 1 : value.mask;
                const int toggled = from < value.cycle ? mask ^ (1 << from) : mask;
                controller.editSelected ([length, toggled] (Step& target) {
                    target.cycle = length;
                    target.mask = toggled != 0 ? toggled : 1;
                });
            }
        }
        finishEdit();
    }

    bool CycleEditor::keyPressed (const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey && (resizing || pressedBeat >= 0))
        {
            cancelEdit();
            return true;
        }
        const int delta = key == juce::KeyPress::leftKey ? -1 : key == juce::KeyPress::rightKey ? 1 : 0;
        const auto* value = inspected();
        if (delta == 0 || resizing || pressedBeat >= 0 || value == nullptr
            || (delta > 0 && value->cycle >= beatCount) || !beginEdit())
            return false;
        resizeCycle (juce::jlimit (1, beatCount, inspected()->cycle + delta));
        finishEdit();
        return true;
    }

    void CycleEditor::finishEdit()
    {
        resetPointer();
        controller.commit();
        if (changed)
            changed();
        repaint();
    }

    void CycleEditor::cancelEdit()
    {
        if (resizing || pressedBeat >= 0)
        {
            resetPointer();
            controller.cancel();
            if (changed)
                changed();
        }
    }

    void CycleEditor::resetPointer()
    {
        pressedBeat = dropBeat = lastLength = -1;
        resizing = movingBeat = false;
    }
}
