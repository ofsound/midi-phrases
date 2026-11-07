#pragma once
#include "PhrasesModel.h"
#include "PhrasesStyle.h"
#include <melatonin_blur/melatonin_blur.h>

namespace phrases::ui
{
    class Grid : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        Grid (Controller&, Palette&);
        std::function<void (int, int)> inspect;
        std::function<void (int)> seedRow, recordRow;
        std::function<void()> toggleVelocity;
        std::function<void()> changed;
        std::function<void (bool)> changeCompactMode;
        bool compact = false;
        PlaybackSteps playbackSteps {};
        int inspectedRow = -1, inspectedStep = -1;
        bool isDraggingDuration() const { return dragging && down.part == Part::duration; }
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void modifierKeysChanged (const juce::ModifierKeys&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        bool keyPressed (const juce::KeyPress&) override;
        Rect stepBounds (int row, int step) const;
        void updateLayout();
        void cancelGesture();
        int getVisibleRowCount() const { return visibleRowCount; }
        void setVisibleRowCount (int count);
        void selectVisibleSteps();
        bool isVelocityMode() const { return velocityMode; }
        void setVelocityMode (bool enabled);
    private:
        Controller& c;
        Palette& p;
        std::array<std::vector<Rect>, 4> cells;
        std::array<Rect, 4> addBounds;
        float scrollX = 0, maxScrollX = 0;
        std::array<std::unique_ptr<juce::Drawable>, 5> icons;
        std::array<juce::Colour, 5> iconInks {juce::Colours::white, juce::Colours::white, juce::Colours::white, juce::Colours::white, juce::Colours::white};
        enum class Part { none,
            header,
            power,
            offset,
            record,
            rowVelocity,
            drawVelocity,
            seed,
            add,
            move,
            duration,
            note,
            velocity,
            skip,
            mute,
            remove,
            resize };
        struct Hit
        {
            Part part = Part::none;
            int row = -1, step = -1;
        };
        Hit hit (juce::Point<float>, bool selecting = false);
        Hit down;
        Rows baseline;
        Selection selectionBefore;
        juce::Point<float> origin, pointer;
        std::optional<juce::Point<float>> lastVelocityPoint;
        uint64_t downGeneration = 0;
        int downPattern = -1;
        bool dragging = false, marquee = false, compactEdited = false;
        bool compactPitchDrag = false;
        bool compactPitchOctaves = false;
        float compactPitchSteps = 0;
        float grabbedNoteTop = 0;
        bool additiveMarquee = false;
        bool shiftOctaveGesture = false;
        bool changedViewOnClick = false;
        bool velocityMode = false;
        int visibleRowCount = 4;
        static constexpr float compactLabelInset = 10;
        static constexpr float rowsTop() { return 24; }
        float rowStride() const { return static_cast<float> (std::max (456, getHeight() - 40)) / static_cast<float> (visibleRowCount); }
        float rowTop (size_t row) const { return rowsTop() + static_cast<float> (row) * rowStride(); }
        float rowHeight() const { return rowStride() - 10.0f; }
        static constexpr float unit = 58.0f;
        Hit hover;
        std::array<melatonin::DropShadow, 4> selectionGlows;
        std::array<melatonin::DropShadow, 4> playbackGlows;
        void drawIcon (juce::Graphics&, size_t, Rect, juce::Colour);
        void drawStepBar (juce::Graphics&, Rect, const Step&, juce::Colour, bool durationHot, bool playing, float flash);
        void drawVelocityStroke (juce::Point<float>);
        void updateHover (juce::Point<float>, juce::ModifierKeys);
        void finish();
    };
}
