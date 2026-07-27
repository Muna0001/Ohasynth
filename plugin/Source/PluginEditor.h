#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

namespace oha {

// Panel palette
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
    const juce::Colour led        { 0xffff5a3c };
}

// Layout metrics, taken from the web app so the two panels match.
namespace metrics {
    constexpr int sliderW  = 32;   // slider column width
    constexpr int segW     = 44;   // segmented-switch column width
    constexpr int sectionH = 178;  // yields a 130px slider track, as on the web
    constexpr int headerH  = 48;
    constexpr int keybedH  = 150;
    constexpr int bottomH  = 179;
    constexpr int cheekW   = 34;   // wooden end cheeks
    constexpr int secHeadH = 24;
    constexpr int labelH   = 16;
    constexpr int padX     = 7;
    constexpr int gap      = 4;
}

// Draw text with letter spacing (CSS letter-spacing has no direct JUCE
// equivalent, and the tracking is a big part of the panel's look).
void drawTracked(juce::Graphics&, const juce::String&, juce::Point<float> origin,
                 float tracking);

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

// Spring-back horizontal pitch-bend lever, as on the hardware.
class BenderLever : public juce::Component {
public:
    explicit BenderLever(OhASynthProcessor&);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    void setFromMouse(const juce::MouseEvent&);
    void push(float v);

    OhASynthProcessor& proc;
    float value = 0.0f;
};

// Momentary button: while held, the LFO ignores its delay fade-in.
class LfoTrigButton : public juce::Component {
public:
    explicit LfoTrigButton(OhASynthProcessor&);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

private:
    OhASynthProcessor& proc;
    bool held = false;
};

// Latching square button with an LED (arp ON / SYNC / HOLD).
class LedButton : public juce::Component {
public:
    explicit LedButton(juce::RangedAudioParameter& p);
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    juce::ParameterAttachment attachment;
    bool on = false;
};

// Arp RATE: continuous Hz normally, detented on note divisions when SYNC is
// on, with a readout underneath showing which.
class ArpRate : public juce::Component {
public:
    explicit ArpRate(OhASynthProcessor&);
    void resized() override;

private:
    void refresh();

    juce::Slider slider;
    juce::Label readout;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    // Watch the parameters directly: the values arrive with these callbacks,
    // so we never read a stale copy from the APVTS cache or the slider.
    std::unique_ptr<juce::ParameterAttachment> rateWatch, syncWatch;
    float rateValue = 0.545f;
    bool synced = false;
};

// Tempo used when nothing external is running; shows the external tempo
// (host or MIDI clock) when something is.
class BpmField : public juce::Component,
                 private juce::Timer {
public:
    explicit BpmField(OhASynthProcessor&);
    void resized() override;

private:
    void timerCallback() override;
    void commit();

    OhASynthProcessor& proc;
    juce::RangedAudioParameter* param = nullptr;
    juce::Label value, ext;
    std::unique_ptr<juce::ParameterAttachment> att;
    bool editing = false;
};

// Two controls sharing one panel column, bottom-aligned, each with its own
// label underneath. Keeps the arp section from sprawling sideways.
class StackColumn : public juce::Component {
public:
    void addItem(std::unique_ptr<juce::Component>, const juce::String& label, int height);
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Item {
        std::unique_ptr<juce::Component> comp;
        juce::String label;
        int height;
    };
    std::vector<Item> items;
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
};

// Bend / LFO-trig box that sits to the left of the keyboard, as on the web
// panel. Holds the bender lever, LFO TRIG, and the two bend-depth minis.
class BenderBox : public juce::Component {
public:
    explicit BenderBox(OhASynthProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;

    static constexpr int prefW = 206, prefH = 103;

private:
    BenderLever lever;
    LfoTrigButton trig;
    juce::Slider dcoMini, vcfMini;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dcoAtt, vcfAtt;
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
    oha::BenderBox benderBox;
    juce::MidiKeyboardComponent keyboard;

    // stacked bars (header / panel / bottom), stored for paint()
    juce::Rectangle<int> headerArea, panelArea, bottomArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OhASynthEditor)
};
