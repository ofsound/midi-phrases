#pragma once
#include "PhrasesArtwork.h"
#include <BinaryData.h>
#include <juce_gui_extra/juce_gui_extra.h>

namespace phrases::ui
{
    using Rect = juce::Rectangle<float>;
    inline juce::Colour colour (juce::uint32 rgb) { return juce::Colour (rgb | 0xff000000); }

    // 180ms each way, matching mp-slot-copy-blink / mp-pattern-clear-blink.
    inline float pulseAmount()
    {
        const double phase = std::fmod (juce::Time::getMillisecondCounterHiRes(), 360.0) / 360.0;
        return static_cast<float> (0.5 - 0.5 * std::cos (phase * juce::MathConstants<double>::twoPi));
    }

    enum class Weight
    {
        regular,
        semibold,
        bold
    };

    struct Palette
    {
        juce::Colour app, workspace, shell, surface, raised, surfaceMuted, subtle, field, text, secondary, muted, faint, inverse, primaryText, border, borderSubtle, borderStrong, accent, strong;
        std::array<juce::Colour, 4> rows, rowSoft, rowStrong;
        juce::Colour danger = colour (0xf87171);
        bool light = false;
        void set (const juce::String& mode)
        {
            light = mode == "light";
            if (mode == "alt")
            {
                app = colour (0x0b0d0e);
                shell = colour (0x121719);
                surface = colour (0x192124);
                raised = colour (0x223036);
                surfaceMuted = colour (0x2d4148);
                subtle = colour (0x38535c);
                field = app;
                text = colour (0xf4fafb);
                secondary = colour (0xcde1e5);
                muted = colour (0x95b4bb);
                faint = colour (0x6f8f97);
                inverse = colour (0x160713);
                primaryText = colour (0xfff5fb);
                border = subtle;
                borderSubtle = colour (0x223036);
                borderStrong = colour (0x6f8f97);
                accent = colour (0xf472b6);
                strong = colour (0xec4899);
                danger = colour (0xfb7185);
                rows = { accent, colour (0x67e8f9), colour (0xfbbf24), colour (0xa78bfa) };
                rowSoft = { colour (0xf9a8d4), colour (0xa5f3fc), colour (0xfde68a), colour (0xddd6fe) };
                rowStrong = { strong, colour (0x22d3ee), colour (0xf59e0b), colour (0x8b5cf6) };
            }
            else
            {
                app = colour (light ? 0xf7f8fa : 0x09090b);
                shell = colour (light ? 0xe6e9ef : 0x0e0e10);
                surface = colour (light ? 0xfafbfc : 0x18181b);
                raised = colour (light ? 0xfefefe : 0x27272a);
                surfaceMuted = colour (light ? 0xf1f3f7 : 0x3f3f46);
                subtle = colour (light ? 0xe4e8ef : 0x52525b);
                field = colour (light ? 0xf8f9fb : 0x09090b);
                text = colour (light ? 0x0c0e12 : 0xfafafa);
                secondary = colour (light ? 0x2e3542 : 0xd4d4d8);
                muted = colour (light ? 0x505968 : 0xa1a1aa);
                faint = colour (light ? 0x707a8a : 0x71717a);
                inverse = colour (0x09090b);
                primaryText = light ? colour (0xf5faf7) : inverse;
                border = colour (light ? 0xa4b0c0 : 0x3f3f46);
                borderSubtle = colour (light ? 0xd1d7e1 : 0x27272a);
                borderStrong = colour (light ? 0x5e6b7d : 0x71717a);
                accent = colour (light ? 0x2a6e4a : 0x62f5b4);
                strong = colour (light ? 0x1a5236 : 0x2fd68f);
                danger = colour (light ? 0xd64545 : 0xf87171);
                rows = { colour (0x48a8c0), colour (0x4868c8), colour (0x7858c8), colour (0xa848c8) };
                rowSoft = { colour (0x78c0d8), colour (0x7898d8), colour (0x9888d8), colour (0xc888d8) };
                rowStrong = { colour (0x3890a8), colour (0x3a58a8), colour (0x6040a8), colour (0x8838a0) };
            }
            workspace = light ? juce::Colours::white : app;
        }
        Palette() { set ("dark"); }
    };

    inline juce::Font font (float size, Weight weight = Weight::regular, float trackingEm = 0)
    {
        static auto regular = juce::Typeface::createSystemTypefaceFor (BinaryData::FamiljenGroteskRegular_ttf, BinaryData::FamiljenGroteskRegular_ttfSize);
        static auto semibold = juce::Typeface::createSystemTypefaceFor (BinaryData::FamiljenGroteskSemibold_ttf, BinaryData::FamiljenGroteskSemibold_ttfSize);
        static auto bold = juce::Typeface::createSystemTypefaceFor (BinaryData::FamiljenGroteskBold_ttf, BinaryData::FamiljenGroteskBold_ttfSize);
        const auto face = weight == Weight::bold ? bold : weight == Weight::semibold ? semibold
                                                                                     : regular;
        auto result = juce::Font (juce::FontOptions (face).withHeight (size * 1.28f));
        result.setExtraKerningFactor (trackingEm / 1.28f);
        return result;
    }
    inline juce::Font font (float size, bool bold)
    {
        return font (size, bold ? Weight::semibold : Weight::regular);
    }

    inline void text (juce::Graphics& g, const juce::String& value, Rect bounds, juce::Colour c, float size, Weight weight, juce::Justification align = juce::Justification::centred, float trackingEm = 0)
    {
        g.setColour (c);
        g.setFont (font (size, weight, trackingEm));
        g.drawText (value, bounds, align, false);
    }
    inline void text (juce::Graphics& g, const juce::String& value, Rect bounds, juce::Colour c, float size = 14, bool bold = false, juce::Justification align = juce::Justification::centred, float trackingEm = 0)
    {
        text (g, value, bounds, c, size, bold ? Weight::semibold : Weight::regular, align, trackingEm);
    }

    inline void face (juce::Graphics& g, Rect bounds, const Palette& p, bool active = false, float radius = 6, bool muted = false)
    {
        const auto base = active ? p.accent.interpolatedWith (p.app, 0.12f) : p.field;
        const auto top = active ? p.accent.interpolatedWith (p.text, 0.24f) : muted ? p.borderSubtle.interpolatedWith (p.field, 0.56f)
                                                                                     : p.subtle.interpolatedWith (p.field, 0.5f);
        if (active)
        {
            g.setGradientFill (juce::ColourGradient (p.strong, bounds.getTopLeft(), base, bounds.getBottomLeft(), false));
            g.fillRoundedRectangle (bounds, radius);
            g.setColour (p.text.withAlpha (0.24f));
            g.fillRoundedRectangle (bounds.withHeight (bounds.getHeight() * 0.42f).reduced (1, 0), radius);
        }
        else
        {
            g.setGradientFill (juce::ColourGradient (top, bounds.getTopLeft(), base, bounds.getBottomLeft(), false));
            g.fillRoundedRectangle (bounds, radius);
        }
        g.setColour (active ? p.accent : muted ? p.borderSubtle : p.border);
        g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 1);
        g.setColour (p.text.withAlpha (active ? 0.32f : muted ? 0.05f : 0.10f));
        g.drawLine (bounds.getX() + radius, bounds.getY() + 1.5f, bounds.getRight() - radius, bounds.getY() + 1.5f);
        if (!active)
        {
            g.setColour (p.app.withAlpha (muted ? 0.60f : 0.55f));
            g.drawLine (bounds.getX() + radius, bounds.getBottom() - 1.5f, bounds.getRight() - radius, bounds.getBottom() - 1.5f);
        }
    }

    // Dash each subpath on its own so the pattern does not bridge the gaps between letters.
    inline void strokeDashed (juce::Graphics& g, const juce::Path& path, juce::Colour ink, float on, float off, float width)
    {
        juce::Path dashes;
        juce::PathFlatteningIterator it (path, {}, 0.25f);
        const float period = on + off;
        float phase = 0;
        int lastSub = -1;
        while (it.next())
        {
            if (static_cast<int> (it.subPathIndex) != lastSub)
                lastSub = static_cast<int> (it.subPathIndex);
            const juce::Point<float> a (it.x1, it.y1), b (it.x2, it.y2);
            const float len = a.getDistanceFrom (b);
            float walked = 0;
            while (walked < len - 0.0001f)
            {
                const float into = std::fmod (phase, period);
                const bool drawing = into < on;
                const float remain = drawing ? on - into : period - into;
                const float step = std::min (remain, len - walked);
                if (step <= 0.0001f)
                    break;
                if (drawing)
                {
                    const float t0 = walked / len;
                    const float t1 = (walked + step) / len;
                    dashes.startNewSubPath (a + (b - a) * t0);
                    dashes.lineTo (a + (b - a) * t1);
                }
                walked += step;
                phase += step;
            }
        }
        g.setColour (ink);
        g.strokePath (dashes, juce::PathStrokeType (width, juce::PathStrokeType::mitered, juce::PathStrokeType::butt));
    }

    inline std::unique_ptr<juce::Drawable> icon (const char* svg)
    {
        if (auto xml = juce::parseXML (svg))
        {
            // Drawable::drawWithin fits painted bounds. Preserve the SVG viewport
            // too, otherwise icons with internal whitespace become oversized.
            const auto view = juce::StringArray::fromTokens (xml->getStringAttribute ("viewBox"), " ,", "");
            if (view.size() == 4)
            {
                auto* bounds = xml->createNewChildElement ("rect");
                bounds->setAttribute ("x", view[0]); bounds->setAttribute ("y", view[1]);
                bounds->setAttribute ("width", view[2]); bounds->setAttribute ("height", view[3]);
                bounds->setAttribute ("fill", "#ffffff"); bounds->setAttribute ("opacity", "0");
            }
            return juce::Drawable::createFromSVG (*xml);
        }
        return {};
    }

    inline void rail (juce::Graphics& g, Rect bounds, const Palette& p)
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (bounds.toNearestInt());
        g.setColour (p.shell);
        g.fillRect (bounds);
        juce::Path grid;
        for (float y = bounds.getY() - 13; y < bounds.getBottom() + 13; y += 6.43f)
        {
            const int row = juce::roundToInt ((y - bounds.getY() + 13) / 6.43f);
            for (float x = (row % 2 == 0 ? 0.0f : 4.95f); x < bounds.getRight() + 10; x += 9.9f)
            {
                grid.startNewSubPath (x, y);
                grid.lineTo (x + 4.45f, y + 2.15f);
                grid.lineTo (x + 4.45f, y + 6.43f);
                grid.lineTo (x, y + 8.58f);
                grid.lineTo (x - 4.45f, y + 6.43f);
                grid.lineTo (x - 4.45f, y + 2.15f);
                grid.closeSubPath();
            }
        }
        g.setColour (p.muted.withAlpha (p.light ? 0.16f : 0.075f));
        g.strokePath (grid, juce::PathStrokeType (0.54f));
        g.setColour (p.text.withAlpha (0.07f));
        g.drawHorizontalLine (juce::roundToInt (bounds.getY()), bounds.getX(), bounds.getRight());
        g.drawHorizontalLine (juce::roundToInt (bounds.getBottom() - 1), bounds.getX(), bounds.getRight());
    }

    inline void logo (juce::Graphics& g, juce::Point<float> origin, const Palette& p)
    {
        for (size_t i = 0; i < std::size (logoPaths); ++i)
        {
            auto path = juce::Drawable::parseSVGPath (logoPaths[i]);
            const auto transform = juce::AffineTransform::translation (origin.x + (i >= 4 ? 86.72f : 0), origin.y);
            g.setColour (p.accent);
            g.strokePath (path, juce::PathStrokeType (10, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), transform);
            g.setColour (p.text);
            g.strokePath (path, juce::PathStrokeType (8, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), transform);
            g.setColour (p.app.withAlpha (0.86f));
            for (float shift : { -2.0f, 0.0f, 2.0f })
            {
                auto dashed = path;
                dashed.applyTransform (transform.translated (shift, 0));
                strokeDashed (g, dashed, p.app.withAlpha (0.86f), 1.2f, 1.6f, 0.65f);
            }
        }
    }

    // A native, accessible control with the original vertical-drag / double-click contract.
    class Control : public juce::Component, public juce::SettableTooltipClient
    {
    public:
        explicit Control (Palette& palette, juce::String title) : p (palette)
        {
            setTitle (title);
            setName (title);
            setTooltip (title);
            setWantsKeyboardFocus (true);
            setMouseCursor (juce::MouseCursor::PointingHandCursor);
        }
        Palette& p;
        juce::String caption;
        std::unique_ptr<juce::Drawable> artwork;
        std::function<juce::String()> display;
        std::function<void (juce::Graphics&, Rect)> customPaint;
        std::function<bool()> active;
        std::function<void (juce::ModifierKeys)> action;
        std::function<double()> value;
        std::function<void (double)> change;
        std::function<void()> begin, end;
        std::function<void (const juce::MouseEvent&)> pointerStart, pointerDrag;
        double minimum = 0, maximum = 127, interval = 1, reset = 0, pixelsPerUnit = 4;
        float size = 14, radius = 6, iconInset = 8, tracking = 0;
        bool bare = false, processing = false, horizontal = false, absolute = false;
        bool accentWhenIdle = false, strongLabel = false, glow = false, translucent = false;
        std::function<bool()> dimmed, copySource, copyTarget;
        void paint (juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            if (customPaint)
            {
                customPaint (g, b);
                return;
            }
            const bool on = active && active();
            const bool quiet = (!isEnabled() || (dimmed && dimmed())) && !on;
            if (copySource && copySource())
            {
                g.setColour (p.accent.interpolatedWith (juce::Colours::black, 0.75f * pulseAmount()));
                g.fillRoundedRectangle (b, radius);
                g.setColour (p.accent);
                g.drawRoundedRectangle (b.reduced (0.5f), radius, 1);
                text (g, display ? display() : caption, b.reduced (3, 0), p.primaryText, size, true, juce::Justification::centred, tracking);
                return;
            }
            if (!bare)
            {
                if (translucent && !on)
                {
                    g.setColour (p.surface.withAlpha (0.30f));
                    g.fillRoundedRectangle (b, radius);
                    g.setColour (isMouseOver() ? p.borderStrong : p.border);
                    g.drawRoundedRectangle (b.reduced (0.5f), radius, 1);
                }
                else
                {
                    if (on && glow)
                    {
                        g.setColour (p.accent.withAlpha (0.18f));
                        g.drawRoundedRectangle (b.expanded (1.2f), radius + 1.0f, 2.6f);
                        g.setColour (p.accent.withAlpha (0.08f));
                        g.drawRoundedRectangle (b.expanded (4.0f), radius + 3.0f, 6.0f);
                    }
                    face (g, b, p, on, processing ? 1.0f : radius, quiet);
                }
            }
            if (processing)
            {
                b = b.reduced (5.1f);
                g.setGradientFill (juce::ColourGradient (p.text.withAlpha (0.12f), b.getTopLeft(), p.app.withAlpha (0.25f), b.getBottomLeft(), false));
                g.fillRect (b);
            }
            const auto idle = accentWhenIdle ? p.accent : (strongLabel ? p.text : p.secondary);
            const bool targetHover = copyTarget && copyTarget() && isMouseOver();
            const auto ink = quiet ? p.faint : (on ? p.primaryText : (targetHover ? p.text : (isMouseOver() ? p.accent : idle)));
            if (artwork)
            {
                artwork->replaceColour (lastInk, ink);
                lastInk = ink;
                artwork->drawWithin (g, b.reduced (processing ? 6.0f : iconInset), juce::RectanglePlacement::centred, 1);
            }
            else
                text (g, display ? display() : caption, b.reduced (3, 0), ink, size, true, juce::Justification::centred, tracking);
            if (targetHover && !on)
            {
                g.setColour (p.accent);
                g.drawRoundedRectangle (b.reduced (0.5f), radius, 1.5f);
                g.setColour (p.accent.withAlpha (0.40f));
                g.drawRoundedRectangle (b.expanded (1.5f), radius + 1.0f, 2.5f);
            }
            if (hasKeyboardFocus (true))
            {
                g.setColour (p.accent);
                g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1), radius, 1);
            }
        }
        void mouseDown (const juce::MouseEvent& e) override
        {
            if (isShowing())
                grabKeyboardFocus();
            origin = value ? value() : 0;
            dragged = false;
            if (begin)
                begin();
            if (e.mods.isPopupMenu())
            {
                if (end)
                    end();
                return;
            }
            if (pointerStart)
                pointerStart (e);
            if (absolute && change)
                assign (minimum + e.position.x / getWidth() * (maximum - minimum));
        }
        void mouseDrag (const juce::MouseEvent& e) override
        {
            if (!change && !pointerDrag)
                return;
            dragged = dragged || e.getDistanceFromDragStart() > 2;
            if (dragged)
            {
                if (pointerDrag)
                {
                    pointerDrag (e);
                    repaint();
                    return;
                }
                if (absolute)
                    assign (minimum + e.position.x / getWidth() * (maximum - minimum));
                else
                    assign (origin + (horizontal ? e.getDistanceFromDragStartX() : -e.getDistanceFromDragStartY()) / pixelsPerUnit * (e.mods.isShiftDown() ? 0.25 : 1.0));
            }
        }
        void mouseUp (const juce::MouseEvent& e) override
        {
            if (!dragged && !e.mods.isPopupMenu() && action)
                action (e.mods);
            if (end)
                end();
            repaint();
        }
        void mouseDoubleClick (const juce::MouseEvent&) override
        {
            if (change)
            {
                if (begin)
                    begin();
                assign (reset);
                if (end)
                    end();
            }
        }
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheel) override
        {
            if (!change || !value)
                return;
            if (begin)
                begin();
            assign (value() + (wheel.deltaY > 0 ? interval : -interval));
            if (end)
                end();
        }
        bool keyPressed (const juce::KeyPress& key) override
        {
            if (key == juce::KeyPress::returnKey || key == juce::KeyPress::spaceKey)
            {
                if (begin)
                    begin();
                if (action)
                    action (key.getModifiers());
                if (end)
                    end();
                return true;
            }
            if (change && value && (key == juce::KeyPress::upKey || key == juce::KeyPress::downKey))
            {
                if (begin)
                    begin();
                assign (value() + (key == juce::KeyPress::upKey ? interval : -interval));
                if (end)
                    end();
                return true;
            }
            return false;
        }
        std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;
        void mouseEnter (const juce::MouseEvent&) override { repaint(); }
        void mouseExit (const juce::MouseEvent&) override { repaint(); }
        void focusGained (FocusChangeType) override { repaint(); }
        void focusLost (FocusChangeType) override { repaint(); }

    private:
        juce::Colour lastInk = juce::Colours::white;
        double origin = 0;
        bool dragged = false;
        void assign (double v)
        {
            change (juce::jlimit (minimum, maximum, std::round (v / interval) * interval));
            repaint();
        }
    };
    inline std::unique_ptr<juce::AccessibilityHandler> Control::createAccessibilityHandler()
    {
        class Value : public juce::AccessibilityValueInterface
        {
        public:
            explicit Value (Control& owner) : c (owner) {}
            bool isReadOnly() const override { return !c.isEnabled() || !c.change; }
            double getCurrentValue() const override { return c.value ? c.value() : 0; }
            juce::String getCurrentValueAsString() const override { return c.display ? c.display() : juce::String (getCurrentValue()); }
            void setValue (double v) override
            {
                if (isReadOnly())
                    return;
                if (c.begin)
                    c.begin();
                c.change (juce::jlimit (c.minimum, c.maximum, std::round (v / c.interval) * c.interval));
                if (c.end)
                    c.end();
                c.repaint();
            }
            void setValueAsString (const juce::String& v) override { setValue (v.getDoubleValue()); }
            AccessibleValueRange getRange() const override { return { { c.minimum, c.maximum }, c.interval }; }

        private:
            Control& c;
        };
        class Handler : public juce::AccessibilityHandler
        {
        public:
            explicit Handler (Control& owner)
                : AccessibilityHandler (owner, owner.value ? juce::AccessibilityRole::slider : juce::AccessibilityRole::button, juce::AccessibilityActions().addAction (juce::AccessibilityActionType::press, [&owner] {
                      if (!owner.isEnabled())
                          return;
                      if (owner.begin)
                          owner.begin();
                      if (owner.action)
                          owner.action ({});
                      if (owner.end)
                          owner.end();
                      owner.repaint();
                  }),
                      Interfaces { owner.value ? std::make_unique<Value> (owner) : nullptr }),
                  c (owner) {}
            juce::AccessibleState getCurrentState() const override
            {
                auto state = AccessibilityHandler::getCurrentState();
                if (c.active)
                {
                    state = state.withCheckable();
                    if (c.active())
                        state = state.withChecked();
                }
                return state;
            }
            juce::String getHelp() const override { return c.getTooltip(); }

        private:
            Control& c;
        };
        return std::make_unique<Handler> (*this);
    }

}
