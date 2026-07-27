#include "PluginEditor.h"

namespace oha {

using namespace metrics;

// ---------------------------------------------------------------------
void drawTracked(juce::Graphics& g, const juce::String& text,
                 juce::Point<float> origin, float tracking) {
    const auto f = g.getCurrentFont();
    float x = origin.x;
    for (int i = 0; i < text.length(); ++i) {
        const auto ch = text.substring(i, i + 1);
        g.drawSingleLineText(ch, juce::roundToInt(x), juce::roundToInt(origin.y));
        x += juce::GlyphArrangement::getStringWidth(f, ch) + tracking;
    }
}

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

    // tick marks, ~13px apart as on the web track
    g.setColour(juce::Colour(0xff3c3c42));
    const int nTicks = juce::jmax(4, juce::roundToInt((track.getHeight() - 12.0f) / 13.0f) + 1);
    for (int i = 0; i < nTicks; ++i) {
        const float ty = track.getY() + 6.0f
            + (track.getHeight() - 12.0f) * (float) i / (float) (nTicks - 1);
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
    const float capW = juce::jmin(24.0f, tw);
    juce::Rectangle<float> cap(cx - capW * 0.5f, sliderPos - 7.0f, capW, 14.0f);
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
BenderLever::BenderLever(OhASynthProcessor& p) : proc(p) {
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void BenderLever::push(float v) {
    value = juce::jlimit(-1.0f, 1.0f, v);
    proc.uiBend.store(value);
    repaint();
}

void BenderLever::setFromMouse(const juce::MouseEvent& e) {
    push((e.position.x / (float) juce::jmax(1, getWidth())) * 2.0f - 1.0f);
}

void BenderLever::mouseDown(const juce::MouseEvent& e) { setFromMouse(e); }
void BenderLever::mouseDrag(const juce::MouseEvent& e) { setFromMouse(e); }
void BenderLever::mouseUp(const juce::MouseEvent&)     { push(0.0f); } // springs back

void BenderLever::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff1a1a1d), r.getX(), r.getY(),
                                           juce::Colour(0xff232327), r.getX(), r.getBottom(),
                                           false));
    g.fillRoundedRectangle(r, 4.0f);
    g.setColour(juce::Colour(0xff0f0f11));
    g.drawRoundedRectangle(r.reduced(0.5f), 4.0f, 1.0f);

    // centre detent
    g.setColour(juce::Colour(0xff46464c));
    g.fillRect(r.getCentreX() - 1.0f, r.getY() + 4.0f, 2.0f, r.getHeight() - 8.0f);

    // lever
    const float cx = r.getCentreX() + value * (r.getWidth() * 0.42f);
    juce::Rectangle<float> lev(cx - 7.0f, r.getY() + 3.0f, 14.0f, r.getHeight() - 6.0f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff4a4a52), lev.getX(), lev.getY(),
                                           juce::Colour(0xff2a2a30), lev.getX(), lev.getBottom(),
                                           false));
    g.fillRoundedRectangle(lev, 3.0f);
    g.setColour(juce::Colour(0xff101012));
    g.drawRoundedRectangle(lev.reduced(0.5f), 3.0f, 1.0f);
}

// ---------------------------------------------------------------------
LfoTrigButton::LfoTrigButton(OhASynthProcessor& p) : proc(p) {}

void LfoTrigButton::mouseDown(const juce::MouseEvent&) {
    held = true;
    proc.uiLfoTrig.store(true);
    repaint();
}

void LfoTrigButton::mouseUp(const juce::MouseEvent&) {
    held = false;
    proc.uiLfoTrig.store(false);
    repaint();
}

void LfoTrigButton::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat().reduced(0.5f);
    g.setColour(held ? juce::Colour(0xff3d3d45) : juce::Colour(0xff1d1d20));
    g.fillRoundedRectangle(r, 3.0f);
    g.setColour(juce::Colour(0xff0f0f11));
    g.drawRoundedRectangle(r, 3.0f, 1.0f);

    g.setColour(held ? col::cream : col::creamDim);
    g.setFont(juce::FontOptions(9.0f));
    drawTracked(g, "LFO TRIG", { 9.0f, r.getCentreY() + 3.0f }, 1.0f);
}

// ---------------------------------------------------------------------
ChorusButtons::ChorusButtons(juce::RangedAudioParameter& p)
    : attachment(p, [this](float v) { value = (int) std::lround(v); repaint(); }, nullptr) {
    attachment.sendInitialUpdate();
}

juce::Rectangle<float> ChorusButtons::buttonRect(int i) const {
    const float size = 46.0f;
    return { (float) i * (size + 8.0f), (float) getHeight() - size - 14.0f, size, size };
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
            g.setColour(col::led.withAlpha(0.30f));
            g.fillEllipse(led.expanded(4.0f));
            g.setColour(col::led);
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
    const int h = juce::jmin(21, (r.getHeight() - 2 * (buttons.size() - 1)) / buttons.size());
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
    g.fillRect(2, secHeadH - 2, getWidth() - 4, 2);

    g.setColour(col::cream);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    const float tw = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), title)
        + 2.0f * (float) title.length();
    drawTracked(g, title, { ((float) getWidth() - tw) * 0.5f, 15.0f }, 2.0f);

    g.setColour(col::creamDim);
    g.setFont(juce::FontOptions(9.0f));
    for (auto& it : items) {
        auto b = it.comp->getBounds();
        g.drawText(it.label, b.getX() - 8, getHeight() - labelH, b.getWidth() + 16, labelH - 2,
                   juce::Justification::centred);
    }
}

void Section::resized() {
    int x = padX;
    const int top = secHeadH + 6;
    const int h = getHeight() - top - labelH - 2;
    for (auto& it : items) {
        it.comp->setBounds(x, top, it.width, h);
        x += it.width + gap;
    }
}

// ---------------------------------------------------------------------
BenderBox::BenderBox(OhASynthProcessor& p) : lever(p), trig(p) {
    addAndMakeVisible(lever);
    addAndMakeVisible(trig);
    for (auto* s : { &dcoMini, &vcfMini }) {
        s->setSliderStyle(juce::Slider::LinearVertical);
        s->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(*s);
    }
    dcoAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "bendDco", dcoMini);
    vcfAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        p.apvts, "bendVcf", vcfMini);
}

void BenderBox::paint(juce::Graphics& g) {
    auto r = getLocalBounds().toFloat();
    g.setColour(col::panel2);
    g.fillRoundedRectangle(r, 6.0f);
    g.setColour(juce::Colour(0xff38383e));
    g.drawRoundedRectangle(r.reduced(0.5f), 6.0f, 1.0f);

    g.setColour(col::creamDim);
    g.setFont(juce::FontOptions(9.0f));
    auto lb = lever.getBounds();
    g.drawText("BENDER", lb.getX(), lb.getBottom() + 1, lb.getWidth(), 12,
               juce::Justification::centred);
    g.drawText("DCO", dcoMini.getX() - 6, getHeight() - labelH, dcoMini.getWidth() + 12, 12,
               juce::Justification::centred);
    g.drawText("VCF", vcfMini.getX() - 6, getHeight() - labelH, vcfMini.getWidth() + 12, 12,
               juce::Justification::centred);
}

void BenderBox::resized() {
    auto r = getLocalBounds().reduced(10, 9);
    auto levers = r.removeFromLeft(112);
    lever.setBounds(levers.removeFromTop(34));
    levers.removeFromTop(14);                       // BENDER label
    trig.setBounds(levers.removeFromTop(24).reduced(6, 0));

    r.removeFromLeft(10);
    auto minis = r.removeFromTop(64 + labelH - 4);
    dcoMini.setBounds(minis.removeFromLeft(28));
    minis.removeFromLeft(4);
    vcfMini.setBounds(minis.removeFromLeft(28));
}

} // namespace oha

// ---------------------------------------------------------------------
OhASynthEditor::OhASynthEditor(OhASynthProcessor& p)
    : AudioProcessorEditor(&p), proc(p), benderBox(p),
      keyboard(p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard) {
    setLookAndFeel(&lnf);
    using oha::Section;
    namespace col = oha::col;
    using namespace oha::metrics;

    const int SL = sliderW, SEG = segW;

    auto* main = sections.emplace_back(std::make_unique<Section>("MAIN", col::red)).get();
    main->add(makeSlider("volume"), "VOLUME", SL);
    main->add(makeSlider("velSens"), "VEL", SL);

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
    const auto& presets = oha::factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem(presets[(size_t) i].name, i + 1);
    presetBox.setSelectedId(proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setTextWhenNothingSelected("PRESETS");
    presetBox.onChange = [this] {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0) proc.setCurrentProgram(idx);
    };
    addAndMakeVisible(presetBox);

    addAndMakeVisible(benderBox);

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

    int contentW = 14;                       // panel side padding
    for (auto& s : sections) contentW += s->idealWidth() + 8;
    setSize(juce::jmax(contentW + 6, 900) + cheekW * 2,
            headerH + sectionH + 22 + bottomH + 4);
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
    using namespace oha::metrics;

    g.fillAll(juce::Colour(0xff141416));

    auto full = getLocalBounds();
    drawWoodCheek(g, full.removeFromLeft(cheekW), true);
    drawWoodCheek(g, full.removeFromRight(cheekW), false);

    // Three stacked bars — header / panel / bottom — as on the web panel.
    auto bar = [&g](juce::Rectangle<int> area, juce::Colour top, juce::Colour bottom,
                    float radTop, float radBottom) {
        auto r = area.toFloat();
        juce::Path p;
        p.addRoundedRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight(),
                              juce::jmax(radTop, radBottom), juce::jmax(radTop, radBottom),
                              radTop > 0, radTop > 0, radBottom > 0, radBottom > 0);
        g.setGradientFill(juce::ColourGradient(top, r.getX(), r.getY(),
                                               bottom, r.getX(), r.getBottom(), false));
        g.fillPath(p);
        g.setColour(col::edge);
        g.strokePath(p, juce::PathStrokeType(1.0f));
    };

    bar(headerArea, juce::Colour(0xff323237), juce::Colour(0xff2a2a2e), 8.0f, 0.0f);
    bar(panelArea,  juce::Colour(0xff2e2e33), col::panel,               0.0f, 0.0f);
    bar(bottomArea, col::panel,               juce::Colour(0xff242428), 0.0f, 8.0f);

    // ----- logo -----
    const float lx = (float) headerArea.getX() + 16.0f;
    const float baseY = (float) headerArea.getCentreY() + 7.0f;

    g.setColour(col::cream);
    g.setFont(juce::FontOptions(21.0f, juce::Font::bold | juce::Font::italic));
    g.drawSingleLineText("Oh-a-synth", juce::roundToInt(lx), juce::roundToInt(baseY));
    const float logoW = juce::GlyphArrangement::getStringWidth(g.getCurrentFont(), "Oh-a-synth");

    // accent stripes, vertically centred like the web header
    const juce::Colour stripes[] = { col::red, col::orange, col::yellow, col::blue };
    const float sx = lx + logoW + 14.0f;
    for (int i = 0; i < 4; ++i) {
        g.setColour(stripes[i]);
        g.fillRect(sx + (float) i * 7.0f, (float) headerArea.getCentreY() - 8.0f, 5.0f, 16.0f);
    }

    g.setColour(col::creamDim);
    g.setFont(juce::FontOptions(10.0f));
    oha::drawTracked(g, "POLYPHONIC SYNTHESIZER",
                     { sx + 4 * 7.0f + 14.0f, (float) headerArea.getCentreY() + 4.0f }, 2.4f);

    // recessed keybed surround
    auto kb = keyboard.getBounds().expanded(4);
    g.setColour(juce::Colour(0xff121214));
    g.fillRoundedRectangle(kb.toFloat(), 4.0f);
    g.setColour(juce::Colours::black);
    g.drawRoundedRectangle(kb.toFloat().reduced(0.5f), 4.0f, 1.0f);
}

void OhASynthEditor::resized() {
    using namespace oha::metrics;

    auto r = getLocalBounds();
    r.removeFromLeft(cheekW);
    r.removeFromRight(cheekW);

    headerArea = r.removeFromTop(headerH);
    panelArea  = r.removeFromTop(sectionH + 22);
    bottomArea = r;

    // header: preset menu on the right
    presetBox.setBounds(headerArea.reduced(16, 11).removeFromRight(180));

    // panel: sections left to right
    auto panel = panelArea.reduced(7, 0);
    int x = panel.getX();
    const int secY = panel.getY() + 11;
    for (auto& s : sections) {
        s->setBounds(x, secY, s->idealWidth(), sectionH);
        x += s->idealWidth() + 8;
    }

    // bottom: bender box, then keyboard
    auto bottom = bottomArea.reduced(14, 14);
    benderBox.setBounds(bottom.getX(),
                        bottom.getCentreY() - oha::BenderBox::prefH / 2,
                        oha::BenderBox::prefW, oha::BenderBox::prefH);
    bottom.removeFromLeft(oha::BenderBox::prefW + 14);
    keyboard.setBounds(bottom.withHeight(keybedH).withY(bottom.getCentreY() - keybedH / 2));
    keyboard.setKeyWidth((float) keyboard.getWidth() / 29.0f);
}
