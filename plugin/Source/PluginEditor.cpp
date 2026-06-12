#include "PluginEditor.h"

namespace oha {

// ---------------------------------------------------------------------
OhaLookAndFeel::OhaLookAndFeel() {
    setColour(juce::ResizableWindow::backgroundColourId, col::panel);
    setColour(juce::Slider::backgroundColourId, juce::Colour(0xff141416));
    setColour(juce::Slider::trackColourId, juce::Colour(0xff585860));
    setColour(juce::Slider::thumbColourId, col::cream);
    setColour(juce::Label::textColourId, col::creamDim);
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1d1d20));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff44444d));
    setColour(juce::TextButton::textColourOffId, col::creamDim);
    setColour(juce::TextButton::textColourOnId, col::cream);
    setColour(juce::ComboBox::backgroundColourId, col::panel2);
    setColour(juce::ComboBox::textColourId, col::cream);
    setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff46464c));
    setColour(juce::ComboBox::arrowColourId, col::creamDim);
    setColour(juce::PopupMenu::backgroundColourId, col::panel2);
    setColour(juce::PopupMenu::textColourId, col::cream);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff44444d));
}

void OhaLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
                                      float sliderPos, float minSliderPos, float maxSliderPos,
                                      juce::Slider::SliderStyle style, juce::Slider& s) {
    if (style != juce::Slider::LinearVertical) {
        LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, sliderPos, minSliderPos,
                                         maxSliderPos, style, s);
        return;
    }

    const float tw = juce::jmin(24.0f, (float) w);
    juce::Rectangle<float> track((float) x + (float) w * 0.5f - tw * 0.5f, (float) y, tw, (float) h);

    // recessed track body
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1d1d20), track.getX(), track.getY(),
                                           juce::Colour(0xff232327), track.getX(), track.getBottom(),
                                           false));
    g.fillRoundedRectangle(track, 3.0f);

    // tick marks (same density as the web app's track)
    g.setColour(juce::Colour(0xff3c3c42));
    const int nTicks = juce::jmax(8, h / 16);
    for (int i = 0; i < nTicks; ++i) {
        float ty = track.getY() + 6.0f + (track.getHeight() - 12.0f) * (float) i / (float) (nTicks - 1);
        g.fillRect(track.getX() + 2.0f, ty, tw - 4.0f, 1.0f);
    }

    g.setColour(juce::Colour(0xff0f0f11));
    g.drawRoundedRectangle(track.reduced(0.5f), 3.0f, 1.0f);

    // centre groove
    const float cx = track.getCentreX();
    g.setColour(juce::Colour(0xff0a0a0b));
    g.fillRoundedRectangle(cx - 2.0f, track.getY() + 4.0f, 4.0f, track.getHeight() - 8.0f, 2.0f);

    // value fill from the bottom up to the cap
    const float fillTop = juce::jlimit(track.getY() + 4.0f, track.getBottom() - 4.0f, sliderPos);
    g.setColour(juce::Colour(0xff585860));
    g.fillRoundedRectangle(cx - 2.0f, fillTop, 4.0f, track.getBottom() - 4.0f - fillTop, 2.0f);

    // fader cap with drop shadow and cream indicator line
    juce::Rectangle<float> cap(cx - 12.0f, sliderPos - 7.0f, 24.0f, 14.0f);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(cap.translated(0, 2.0f), 2.0f);

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4a4a52), cap.getX(), cap.getY(),
                                           juce::Colour(0xff3a3a41), cap.getX(), cap.getBottom(),
                                           false));
    g.fillRoundedRectangle(cap, 2.0f);
    g.setColour(juce::Colour(0xff222226));
    g.fillRect(cap.getX() + 1.0f, cap.getCentreY() + 1.5f, cap.getWidth() - 2.0f, 3.5f);
    g.setColour(juce::Colour(0xff101012));
    g.drawRoundedRectangle(cap.reduced(0.5f), 2.0f, 1.0f);

    g.setColour(col::cream);
    g.fillRect(cap.getX() + 2.0f, cap.getCentreY() - 1.0f, cap.getWidth() - 4.0f, 2.0f);
}

void OhaLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                          const juce::Colour&, bool over, bool down) {
    auto r = b.getLocalBounds().toFloat().reduced(0.5f);
    auto base = b.getToggleState() ? juce::Colour(0xff3d3d45) : juce::Colour(0xff1d1d20);
    if (down) base = base.brighter(0.10f);
    else if (over) base = base.brighter(0.05f);

    g.setGradientFill(juce::ColourGradient(base.brighter(0.06f), r.getX(), r.getY(),
                                           base.darker(0.08f), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(juce::Colour(0xff0f0f11));
    g.drawRoundedRectangle(r, 3.0f, 1.0f);
    if (b.getToggleState()) {
        g.setColour(juce::Colour(0xff55555e));
        g.drawRoundedRectangle(r.reduced(1.0f), 2.5f, 1.0f);
    }
}

// ---------------------------------------------------------------------
ChorusButtons::ChorusButtons(juce::RangedAudioParameter& p)
    : attachment(p, [this](float v) { value = (int) std::lround(v); repaint(); }, nullptr) {
    attachment.sendInitialUpdate();
}

juce::Rectangle<float> ChorusButtons::buttonRect(int i) const {
    const float size = 46.0f;
    return { (float) i * (size + 8.0f), (float) getHeight() - size - 6.0f, size, size };
}

void ChorusButtons::paint(juce::Graphics& g) {
    for (int i = 0; i < 2; ++i) {
        auto r = buttonRect(i);
        const bool on = (value & (i + 1)) != 0;

        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff3a3a41), r.getX(), r.getY(),
                                               juce::Colour(0xff26262b), r.getX(), r.getBottom(),
                                               false));
        g.fillRoundedRectangle(r, 4.0f);
        g.setColour(juce::Colour(0xff101012));
        g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

        // LED
        juce::Rectangle<float> led(r.getCentreX() - 4.0f, r.getY() + 9.0f, 8.0f, 8.0f);
        if (on) {
            const juce::Colour glow(0xffff5a3c);
            g.setColour(glow.withAlpha(0.30f));
            g.fillEllipse(led.expanded(4.0f));
            g.setColour(glow);
            g.fillEllipse(led);
        } else {
            g.setColour(juce::Colour(0xff4a1410));
            g.fillEllipse(led);
            g.setColour(juce::Colours::black.withAlpha(0.5f));
            g.drawEllipse(led, 1.0f);
        }

        g.setColour(col::cream);
        g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
        g.drawText(i == 0 ? "I" : "II",
                   r.withTrimmedTop(r.getHeight() * 0.45f).toNearestInt(),
                   juce::Justification::centred);
    }
}

void ChorusButtons::mouseDown(const juce::MouseEvent& e) {
    for (int i = 0; i < 2; ++i)
        if (buttonRect(i).contains(e.position)) {
            attachment.setValueAsCompleteGesture((float) (value ^ (i + 1)));
            return;
        }
}

// ---------------------------------------------------------------------
SegSwitch::SegSwitch(juce::RangedAudioParameter& p, const juce::StringArray& options)
    : attachment(p, [this](float v) { showSelection(v); }, nullptr) {
    for (int i = 0; i < options.size(); ++i) {
        auto* b = buttons.add(new juce::TextButton(options[i]));
        addAndMakeVisible(b);
        b->setClickingTogglesState(false);
        b->onClick = [this, i] { attachment.setValueAsCompleteGesture((float) i); };
    }
    attachment.sendInitialUpdate();
}

void SegSwitch::showSelection(float denormValue) {
    const int sel = (int) std::lround(denormValue);
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState(i == sel, juce::dontSendNotification);
}

void SegSwitch::resized() {
    auto r = getLocalBounds();
    const int h = juce::jmin(22, (r.getHeight() - 2 * (buttons.size() - 1)) / buttons.size());
    // option 0 at the bottom, like the hardware switches
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setBounds(r.getX(), r.getBottom() - (i + 1) * (h + 2) + 2, r.getWidth(), h);
}

// ---------------------------------------------------------------------
Section::Section(juce::String t, juce::Colour a) : title(std::move(t)), accent(a) {}

void Section::add(std::unique_ptr<juce::Component> comp, const juce::String& label, int width) {
    addAndMakeVisible(*comp);
    items.push_back({ std::move(comp), label, width });
}

int Section::idealWidth() const {
    int w = padX * 2 - gap;
    for (auto& it : items) w += it.width + gap;
    return juce::jmax(w, 60);
}

void Section::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(col::panel2);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(juce::Colour(0xff38383e));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);

    g.setColour(accent);
    g.fillRect(2, headerH - 2, getWidth() - 4, 2);
    g.setColour(col::cream);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText(title, 0, 0, getWidth(), headerH - 2, juce::Justification::centred);

    g.setColour(col::creamDim);
    g.setFont(juce::FontOptions(9.5f));
    for (auto& it : items) {
        auto b = it.comp->getBounds();
        g.drawText(it.label, b.getX() - 6, getHeight() - labelH, b.getWidth() + 12, labelH - 2,
                   juce::Justification::centred);
    }
}

void Section::resized() {
    int x = padX;
    const int top = headerH + 6;
    const int h = getHeight() - top - labelH - 2;
    for (auto& it : items) {
        it.comp->setBounds(x, top, it.width, h);
        x += it.width + gap;
    }
}

} // namespace oha

// ---------------------------------------------------------------------
OhASynthEditor::OhASynthEditor(OhASynthProcessor& p)
    : AudioProcessorEditor(&p), proc(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&lnf);
    using oha::Section;
    namespace col = oha::col;

    const int SL = 36, SEG = 52;

    auto* main = sections.emplace_back(std::make_unique<Section>("MAIN", col::red)).get();
    main->add(makeSlider("volume"), "VOLUME", SL);
    main->add(makeSlider("velSens"), "VEL", SL);
    main->add(makeSlider("bendDco"), "BEND", SL);

    auto* lfo = sections.emplace_back(std::make_unique<Section>("LFO", col::blue)).get();
    lfo->add(makeSlider("lfoRate"), "RATE", SL);
    lfo->add(makeSlider("lfoDelay"), "DELAY", SL);

    auto* dco = sections.emplace_back(std::make_unique<Section>("DCO", col::orange)).get();
    dco->add(makeSeg("dcoRange", { "16'", "8'", "4'" }), "RANGE", SEG);
    dco->add(makeSlider("dcoLfo"), "LFO", SL);
    dco->add(makeSlider("dcoPw"), "PWM", SL);
    dco->add(makeSeg("dcoPwmMode", { "MAN", "LFO" }), "PWM SRC", SEG);
    dco->add(makeSeg("dcoPulse", { "OFF", "ON" }), "PULSE", SEG);
    dco->add(makeSeg("dcoSaw", { "OFF", "ON" }), "SAW", SEG);
    dco->add(makeSlider("dcoSub"), "SUB", SL);
    dco->add(makeSlider("dcoNoise"), "NOISE", SL);

    auto* hpf = sections.emplace_back(std::make_unique<Section>("HPF", col::green)).get();
    hpf->add(makeSeg("hpf", { "0", "1", "2", "3" }), "FREQ", SEG);

    auto* vcf = sections.emplace_back(std::make_unique<Section>("VCF", col::red)).get();
    vcf->add(makeSlider("vcfFreq"), "FREQ", SL);
    vcf->add(makeSlider("vcfRes"), "RES", SL);
    vcf->add(makeSeg("vcfPol", { juce::String(juce::CharPointer_UTF8("\xE2\x86\x91")),
                                 juce::String(juce::CharPointer_UTF8("\xE2\x86\x93")) }),
             "ENV POL", SEG);
    vcf->add(makeSlider("vcfEnv"), "ENV", SL);
    vcf->add(makeSlider("vcfLfo"), "LFO", SL);
    vcf->add(makeSlider("vcfKey"), "KYBD", SL);

    auto* vca = sections.emplace_back(std::make_unique<Section>("VCA", col::blue)).get();
    vca->add(makeSeg("vcaMode", { "ENV", "GATE" }), "MODE", SEG);
    vca->add(makeSlider("vcaLevel"), "LEVEL", SL);

    auto* env = sections.emplace_back(std::make_unique<Section>("ENV", col::orange)).get();
    env->add(makeSlider("envA"), "A", SL);
    env->add(makeSlider("envD"), "D", SL);
    env->add(makeSlider("envS"), "S", SL);
    env->add(makeSlider("envR"), "R", SL);

    auto* ch = sections.emplace_back(std::make_unique<Section>("CHORUS", col::green)).get();
    {
        auto* chorusParam = proc.apvts.getParameter("chorus");
        jassert(chorusParam != nullptr);
        ch->add(std::make_unique<oha::ChorusButtons>(*chorusParam), "", 100);
    }

    for (auto& s : sections) addAndMakeVisible(*s);

    // preset selector
    const auto& presets = juno::factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem(presets[(size_t) i].name, i + 1);
    presetBox.setSelectedId(proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected("PRESETS");
    presetBox.onChange = [this] {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) proc.setCurrentProgram(idx);
    };
    addAndMakeVisible(presetBox);

    // cream/charcoal keybed
    keyboard.setAvailableRange(36, 84);
    keyboard.setOctaveForMiddleC(4);
    keyboard.setScrollButtonsVisible(false);
    keyboard.setBlackNoteWidthProportion(0.62f);
    keyboard.setBlackNoteLengthProportion(0.62f);
    keyboard.setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour(0xfff0ebdd));
    keyboard.setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour(0xff1b1b1e));
    keyboard.setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xffb6ae99));
    keyboard.setColour(juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha(0.6f));
    keyboard.setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
                       juce::Colours::black.withAlpha(0.06f));
    keyboard.setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId,
                       juce::Colours::black.withAlpha(0.22f));
    keyboard.setColour(juce::MidiKeyboardComponent::textLabelColourId, juce::Colour(0xff8a8270));
    addAndMakeVisible(keyboard);

    int w = 16;
    for (auto& s : sections) w += s->idealWidth() + 8;
    setSize(juce::jmax(w + 8, 900) + cheekW * 2, 560);
}

OhASynthEditor::~OhASynthEditor() {
    setLookAndFeel(nullptr);
}

std::unique_ptr<juce::Slider> OhASynthEditor::makeSlider(const juce::String& paramID) {
    auto s = std::make_unique<juce::Slider>(juce::Slider::LinearVertical,
                                            juce::Slider::NoTextBox);
    sliderAtts.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, paramID, *s));
    if (auto* p = proc.apvts.getParameter(paramID))
        s->setDoubleClickReturnValue(true, p->convertFrom0to1(p->getDefaultValue()));
    return s;
}

std::unique_ptr<oha::SegSwitch> OhASynthEditor::makeSeg(const juce::String& paramID,
                                                        const juce::StringArray& options) {
    auto* p = proc.apvts.getParameter(paramID);
    jassert(p != nullptr);
    return std::make_unique<oha::SegSwitch>(*p, options);
}

void OhASynthEditor::drawWoodCheek(juce::Graphics& g, juce::Rectangle<int> area, bool isLeft) const {
    auto r = area.toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5e3d27), r.getX(), r.getY(),
                                           juce::Colour(0xff372315), r.getRight(), r.getBottom(),
                                           false));
    g.fillRect(r);

    // grain: fixed seed so it doesn't shimmer on repaints
    juce::Random rng(isLeft ? 7331 : 1337);
    for (int i = 0; i < 26; ++i) {
        const float gx = r.getX() + rng.nextFloat() * r.getWidth();
        const float alpha = 0.05f + rng.nextFloat() * 0.10f;
        const float wob = (rng.nextFloat() - 0.5f) * 5.0f;
        g.setColour(juce::Colours::black.withAlpha(alpha));
        juce::Path grain;
        grain.startNewSubPath(gx, r.getY());
        grain.quadraticTo(gx + wob, r.getCentreY(), gx, r.getBottom());
        g.strokePath(grain, juce::PathStrokeType(0.8f + rng.nextFloat()));
    }

    // edge shading + inner bevel toward the panel
    g.setGradientFill(juce::ColourGradient(
        juce::Colours::black.withAlpha(isLeft ? 0.0f : 0.35f), r.getX(), r.getY(),
        juce::Colours::black.withAlpha(isLeft ? 0.35f : 0.0f), r.getRight(), r.getY(), false));
    g.fillRect(r);
    g.setColour(juce::Colour(0x33ffffff));
    g.fillRect(isLeft ? r.getX() : r.getRight() - 1.5f, r.getY(), 1.5f, r.getHeight());

    // screws
    auto screw = [&g](float cx, float cy) {
        juce::Rectangle<float> s(cx - 5, cy - 5, 10, 10);
        g.setGradientFill(juce::ColourGradient(juce::Colour(0xff8d8d94), s.getX(), s.getY(),
                                               juce::Colour(0xff3c3c41), s.getX(), s.getBottom(),
                                               false));
        g.fillEllipse(s);
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.drawEllipse(s, 1.0f);
        g.setColour(juce::Colour(0xff232327));
        g.drawLine(cx - 3.2f, cy, cx + 3.2f, cy, 1.4f);
        g.drawLine(cx, cy - 3.2f, cx, cy + 3.2f, 1.4f);
    };
    screw(r.getCentreX(), r.getY() + 16.0f);
    screw(r.getCentreX(), r.getBottom() - 16.0f);
}

void OhASynthEditor::paint(juce::Graphics& g) {
    namespace col = oha::col;
    auto full = getLocalBounds();

    // metal face with a slight vertical sheen
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff313137), 0, 0,
                                           juce::Colour(0xff26262a), 0, (float) getHeight(), false));
    g.fillAll();

    drawWoodCheek(g, full.removeFromLeft(cheekW), true);
    drawWoodCheek(g, full.removeFromRight(cheekW), false);

    // logo
    g.setColour(col::cream);
    g.setFont(juce::FontOptions(23.0f, juce::Font::bold | juce::Font::italic));
    g.drawText("Oh-a-synth", cheekW + 16, 8, 200, 30, juce::Justification::centredLeft);

    const juce::Colour stripes[] = { col::red, col::orange, col::yellow, col::blue };
    for (int i = 0; i < 4; ++i) {
        g.setColour(stripes[i]);
        g.fillRect(cheekW + 192 + i * 7, 14, 5, 17);
    }
    g.setColour(col::creamDim);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("POLYPHONIC SYNTHESIZER", cheekW + 232, 8, 320, 30,
               juce::Justification::centredLeft);

    // recessed keybed surround
    auto kb = keyboard.getBounds().expanded(4);
    g.setColour(juce::Colour(0xff121214));
    g.fillRoundedRectangle(kb.toFloat(), 4.0f);
    g.setColour(juce::Colours::black);
    g.drawRoundedRectangle(kb.toFloat().reduced(0.5f), 4.0f, 1.0f);
}

void OhASynthEditor::resized() {
    auto r = getLocalBounds();
    r.removeFromLeft(cheekW);
    r.removeFromRight(cheekW);
    r.reduce(8, 8);

    auto header = r.removeFromTop(38);
    presetBox.setBounds(header.removeFromRight(180).reduced(0, 6));

    auto kbArea = r.removeFromBottom(keybedH);
    keyboard.setBounds(kbArea.reduced(2, 6));
    keyboard.setKeyWidth((float) keyboard.getWidth() / 29.0f);

    r.removeFromBottom(6);
    r.removeFromTop(2);
    int x = r.getX();
    for (auto& s : sections) {
        s->setBounds(x, r.getY(), s->idealWidth(), r.getHeight() - 4);
        x += s->idealWidth() + 8;
    }
}
