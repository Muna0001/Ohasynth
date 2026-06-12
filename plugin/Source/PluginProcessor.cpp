#include "PluginProcessor.h"

#include "PluginEditor.h"

OhASynthProcessor::OhASynthProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createParameterLayout()) {
    auto get = [this](const char* id) { return apvts.getRawParameterValue(id); };
    raw = {
        get("lfoRate"), get("lfoDelay"), get("dcoRange"), get("dcoLfo"), get("dcoPw"),
        get("dcoPwmMode"), get("dcoSaw"), get("dcoPulse"), get("dcoSub"), get("dcoNoise"),
        get("hpf"), get("vcfFreq"), get("vcfRes"), get("vcfPol"), get("vcfEnv"),
        get("vcfLfo"), get("vcfKey"), get("vcaMode"), get("vcaLevel"), get("envA"),
        get("envD"), get("envS"), get("envR"), get("chorus"), get("bendDco"),
        get("bendVcf"), get("velSens"), get("volume")
    };
    setCurrentProgram(0); // sound great immediately: STRINGS 1
}

juce::AudioProcessorValueTreeState::ParameterLayout OhASynthProcessor::createParameterLayout() {
    using juce::AudioParameterChoice;
    using juce::AudioParameterFloat;
    using juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto f = [&](const char* id, const char* nm, float def) {
        layout.add(std::make_unique<AudioParameterFloat>(
            ParameterID { id, 1 }, nm, juce::NormalisableRange<float>(0.0f, 1.0f, 0.0f), def));
    };
    auto c = [&](const char* id, const char* nm, const juce::StringArray& opts, int def) {
        layout.add(std::make_unique<AudioParameterChoice>(ParameterID { id, 1 }, nm, opts, def));
    };

    f("lfoRate", "LFO Rate", 0.40f);
    f("lfoDelay", "LFO Delay", 0.0f);
    c("dcoRange", "DCO Range", { "16'", "8'", "4'" }, 1);
    f("dcoLfo", "DCO LFO", 0.0f);
    f("dcoPw", "DCO PWM", 0.0f);
    c("dcoPwmMode", "PWM Source", { "Manual", "LFO" }, 0);
    c("dcoSaw", "Saw", { "Off", "On" }, 1);
    c("dcoPulse", "Pulse", { "Off", "On" }, 0);
    f("dcoSub", "Sub Level", 0.0f);
    f("dcoNoise", "Noise Level", 0.0f);
    c("hpf", "HPF", { "0 (Boost)", "1 (Flat)", "2", "3" }, 1);
    f("vcfFreq", "VCF Freq", 0.55f);
    f("vcfRes", "VCF Res", 0.0f);
    c("vcfPol", "VCF Env Polarity", { "Normal", "Inverted" }, 0);
    f("vcfEnv", "VCF Env", 0.25f);
    f("vcfLfo", "VCF LFO", 0.0f);
    f("vcfKey", "VCF Key Follow", 0.50f);
    c("vcaMode", "VCA Mode", { "Env", "Gate" }, 0);
    f("vcaLevel", "VCA Level", 0.80f);
    f("envA", "Env Attack", 0.05f);
    f("envD", "Env Decay", 0.40f);
    f("envS", "Env Sustain", 0.70f);
    f("envR", "Env Release", 0.30f);
    c("chorus", "Chorus", { "Off", "I", "II", "I+II" }, 0);
    f("bendDco", "Bend Range DCO", 0.1667f);
    f("bendVcf", "Bend To VCF", 0.0f);
    f("velSens", "Velocity Sens", 0.30f);
    f("volume", "Volume", 0.75f);
    return layout;
}

void OhASynthProcessor::prepareToPlay(double sampleRate, int) {
    engine.prepare(sampleRate);
    syncParams();
}

bool OhASynthProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void OhASynthProcessor::syncParams() {
    juno::Params p;
    p.lfoRate = *raw.lfoRate;       p.lfoDelay = *raw.lfoDelay;
    p.dcoRange = (int) *raw.dcoRange;
    p.dcoLfo = *raw.dcoLfo;         p.dcoPw = *raw.dcoPw;
    p.dcoPwmMode = (int) *raw.dcoPwmMode;
    p.dcoSaw = (int) *raw.dcoSaw;   p.dcoPulse = (int) *raw.dcoPulse;
    p.dcoSub = *raw.dcoSub;         p.dcoNoise = *raw.dcoNoise;
    p.hpf = (int) *raw.hpf;
    p.vcfFreq = *raw.vcfFreq;       p.vcfRes = *raw.vcfRes;
    p.vcfPol = (int) *raw.vcfPol;   p.vcfEnv = *raw.vcfEnv;
    p.vcfLfo = *raw.vcfLfo;         p.vcfKey = *raw.vcfKey;
    p.vcaMode = (int) *raw.vcaMode; p.vcaLevel = *raw.vcaLevel;
    p.envA = *raw.envA; p.envD = *raw.envD; p.envS = *raw.envS; p.envR = *raw.envR;
    p.chorus = (int) *raw.chorus;
    p.bendDco = *raw.bendDco;       p.bendVcf = *raw.bendVcf;
    p.velSens = *raw.velSens;       p.volume = *raw.volume;
    engine.setParams(p);
}

void OhASynthProcessor::handleMidi(const juce::MidiMessage& m) {
    if (m.isNoteOn())
        engine.noteOn(m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        engine.noteOff(m.getNoteNumber());
    else if (m.isPitchWheel())
        engine.pitchBend((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isController()) {
        const int cc = m.getControllerNumber();
        if (cc == 1) engine.modWheel((float) m.getControllerValue() / 127.0f);
        else if (cc == 64) engine.setSustain(m.getControllerValue() >= 64);
        else if (cc == 120 || cc == 123) engine.allNotesOff();
    } else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.allNotesOff();
}

void OhASynthProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    // merge notes played on the editor's on-screen keyboard
    keyboardState.processNextMidiBuffer(midi, 0, n, true);
    syncParams();

    float* L = buffer.getWritePointer(0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : L;

    // render in segments split at MIDI events for sample-accurate timing
    int pos = 0;
    for (const auto meta : midi) {
        const int at = juce::jlimit(0, n, meta.samplePosition);
        if (at > pos) { engine.render(L + pos, R + pos, at - pos); pos = at; }
        handleMidi(meta.getMessage());
    }
    if (pos < n) engine.render(L + pos, R + pos, n - pos);

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch)
        buffer.clear(ch, 0, n);
    midi.clear();
}

void OhASynthProcessor::setCurrentProgram(int index) {
    const auto& presets = juno::factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    currentProgram = index;
    for (const auto& [id, value] : presets[(size_t) index].values)
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(value));
}

const juce::String OhASynthProcessor::getProgramName(int index) {
    const auto& presets = juno::factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return {};
    return presets[(size_t) index].name;
}

void OhASynthProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    state.setProperty("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void OhASynthProcessor::setStateInformation(const void* data, int sizeInBytes) {
    if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
        auto state = juce::ValueTree::fromXml(*xml);
        if (state.isValid()) {
            currentProgram = (int) state.getProperty("program", 0);
            apvts.replaceState(state);
        }
    }
}

juce::AudioProcessorEditor* OhASynthProcessor::createEditor() {
    return new OhASynthEditor(*this);
}

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new OhASynthProcessor();
}
