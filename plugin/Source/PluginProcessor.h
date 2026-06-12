#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

#include "JunoDSP.h"
#include "Presets.h"

class OhASynthProcessor : public juce::AudioProcessor {
public:
    OhASynthProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Oh-a-synth"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return (int) juno::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void syncParams();
    void handleMidi(const juce::MidiMessage& m);

    // cached raw parameter values (atomics owned by the APVTS)
    struct Raw {
        std::atomic<float>* lfoRate, * lfoDelay, * dcoRange, * dcoLfo, * dcoPw,
            * dcoPwmMode, * dcoSaw, * dcoPulse, * dcoSub, * dcoNoise, * hpf,
            * vcfFreq, * vcfRes, * vcfPol, * vcfEnv, * vcfLfo, * vcfKey,
            * vcaMode, * vcaLevel, * envA, * envD, * envS, * envR, * chorus,
            * bendDco, * bendVcf, * velSens, * volume;
    } raw {};

    juno::Engine engine;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OhASynthProcessor)
};
