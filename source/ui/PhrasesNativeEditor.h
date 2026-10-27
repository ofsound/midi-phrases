#pragma once
#include "PhrasesGrid.h"
#include "PhrasesPreview.h"
#include "PhrasesSeed.h"
#include <map>

namespace phrases::ui
{
    class Keyboard : public juce::Component
    {
    public:
        explicit Keyboard (Palette& palette) : p (palette) {}
        Palette& p;
        int low = 36, high = 96, selected = -1;
        int scaleRoot = 0, scaleMode = 0;
        bool showScale = false;
        bool scrolling = false;
        int rowAccent = -1;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        std::function<void (int)> noteOn, noteOff;
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseExit (const juce::MouseEvent&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        ~Keyboard() override { release(); }
        void release();
        void centerOn (int note);
        bool accepts (int note) const;
        Rect boundsFor (int note);

    private:
        struct Key
        {
            Rect bounds;
            int note = 0;
            bool black = false;
        };
        std::vector<Key> keys;
        int held = -1;
        int hoverNote = -1;
        int centerNote = -1;
        float scrollLeft = 0, keyWidth = 12, keysTop = 0, keysH = 1;
        bool hoverDown = false, hoverUp = false, octaveHold = false;
        Rect octaveDown, octaveUp;
        void layout();
        void buildKeys();
        void scrollBy (float delta);
        void press (juce::Point<float>);
    };

    class NativeEditor : public juce::Component, private juce::Timer, private juce::KeyListener
    {
    public:
        explicit NativeEditor (PluginProcessor&);
        ~NativeEditor() override;
        Controller controller;
        Palette palette;
        Grid grid;
        Preview preview;
        std::function<void (int)> projectAction;
        std::function<void()> scaleChanged, fullscreenChanged;
        bool fullscreen = false;
        void paint (juce::Graphics&) override;
        void resized() override;
        void mouseDown (const juce::MouseEvent&) override;
        void synchronise();
        void applyTheme (const juce::String& mode);
        void documentChanged();
        void showInspector (int row, int step);
        void showRowEditor (int row);
        void showSeed (int row = -1);
        bool keyPressed (const juce::KeyPress&) override;
        bool keyStateChanged (bool) override;
        void focusLost (FocusChangeType) override;

    private:
        struct Placement
        {
            Control* control;
            int region;
            Rect bounds;
            juce::String label;
        };
        std::vector<std::unique_ptr<Control>> controls;
        std::vector<Placement> placements;
        Keyboard keyboard { palette };
        juce::Label projectName, projectDescription;
        std::unique_ptr<juce::Component> scalePicker;
        juce::TooltipWindow tooltips { this, 600 };
        int inspectorRow = -1, inspectorStep = -1, seedTarget = 0, copyPattern = -1, clearPattern = -1, keyboardOctave = 0;
        float viewThumbFrom = 0;
        double viewThumbStart = 0;
        bool viewThumbMoving = false;
        float viewThumbPosition() const;
        void repaintPatternSlots();
        int knownPattern = -1;
        uint64_t knownDocument = 0;
        bool wasPlaying = false;
        bool seedOpen = false, previewDirty = true;
        std::array<int, 17> heldKeys {};
        juce::String theme;
        double top() const { return controller.p.hasStandaloneTransport() ? 48.0 : 0.0; }
        int slot() const { return controller.p.getViewPatternSlot(); }
        Control& button (juce::String name, juce::String caption, int region, Rect, std::function<void (juce::ModifierKeys)>, const char* art = nullptr);
        Control& parameter (juce::String name, juce::String label, int region, Rect, double min, double max, double reset, std::function<double()>, std::function<void (double)>, std::function<juce::String()> display = {});
        void buildControls();
        void buildInspector();
        void buildSeed();
        void openScale();
        void updateMetadata();
        void edited();
        void record (int);
        void bulkOrder (int operation);
        void seedGenerate (bool allRhythm = false);
        void sliderAppearance (Control&);
        void reshuffleSeed (int aspect);
        void editStep (const std::function<void (Step&)>&);
        Step currentStep() const;
        void timerCallback() override;
        void releaseKeys();
        bool keyPressed (const juce::KeyPress& key, juce::Component*) override { return keyPressed (key); }
        bool keyStateChanged (bool down, juce::Component*) override { return keyStateChanged (down); }
    };
}
