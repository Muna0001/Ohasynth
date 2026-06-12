#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

namespace oha {

// Juno-style palette
namespace col {
    const juce::Colour panel      { 0xff2b2b30 };
    const juce::Colour panel2     { 0xff232327 };
    const juce::Colour edge       { 0xff1a1a1d };
    const juce::Colour cream      { 0xffece5d5 };
    const juce::Colour creamDim   { 0xffb9b2a2 };
    const juce::Colour red        { 0xffc8413b };
    const juce::Colour orange     { 0xffe0a33b };
    const juce::Colour yellow     { 0xffe6d06b };
    const juce::Colour blue       { 0xff4f9ed9 };
    const juce::Colour green      { 0xff9bba5e };
}

class OhaLookAndFeel : public juce::LookAndFeel_V4 {
public:
    OhaLookAndFeel();

    // hardware-style fader: recessed track, tick marks, groove, capped thumb
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
};

// The two hardware chorus buttons with LEDs: I, II, both = I+II.
// Bound to the 4-value chorus choice param (bit 0 = I, bit 1 = II).
class ChorusButtons : public juce::Component {
public:
    explicit ChorusButtons(juce::RangedAudioParameter& p);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> buttonRect(int i) const;

    juce::ParameterAttachment attachment;
    int value = 0;
};

// Vertical stack of latching buttons bound to an AudioParameterChoice
class SegSwitch : public juce::Component {
public:
    SegSwitch(juce::RangedAudioParameter& p, const juce::StringArray& options);
    void resized() override;

private:
    void showSelection(float denormValue);

    juce::ParameterAttachment attachment;
    juce::OwnedArray<juce::TextButton> buttons;
};

// A titled panel section holding labelled controls laid out horizontally
class Section : public juce::Component {
public:
    Section(juce::String title, juce::Colour accent);
    void add(std::unique_ptr<juce::Component> comp, const juce::String& label, int width);
    int idealWidth() const;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Item {
        std::unique_ptr<juce::Component> comp;
        juce::String label;
        int width;
    };
    juce::String title;
    juce::Colour accent;
    std::vector<Item> items;

    static constexpr int headerH = 24, labelH = 16, padX = 7, gap = 4;
};

} // namespace oha

class OhASynthEditor : public juce::AudioProcessorEditor {
public:
    explicit OhASynthEditor(OhASynthProcessor&);
    ~OhASynthEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::unique_ptr<juce::Slider> makeSlider(const juce::String& paramID);
    std::unique_ptr<oha::SegSwitch> makeSeg(const juce::String& paramID,
                                            const juce::StringArray& options);
    void drawWoodCheek(juce::Graphics&, juce::Rectangle<int> area, bool isLeft) const;

    OhASynthProcessor& proc;
    oha::OhaLookAndFeel lnf;

    std::vector<std::unique_ptr<oha::Section>> sections;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAtts;

    juce::ComboBox presetBox;
    juce::MidiKeyboardComponent keyboard;

    static constexpr int cheekW = 34;   // wooden end cheeks
    static constexpr int keybedH = 150; // taller, more playable keys

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OhASynthEditor)
};
