/*
 * Factory presets — mirrors js/engine/presets.js. Values are raw parameter
 * values (sliders 0..1, switches as indices), keyed by parameter ID.
 */
#pragma once

#include <utility>
#include <vector>

namespace juno {

struct Preset {
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<Preset>& factoryPresets() {
    static const std::vector<Preset> presets = {
        { "STRINGS 1", {
            { "lfoRate", 0.45f }, { "lfoDelay", 0.35f },
            { "dcoRange", 1 }, { "dcoLfo", 0.14f }, { "dcoPw", 0.35f }, { "dcoPwmMode", 1 },
            { "dcoSaw", 1 }, { "dcoPulse", 0 }, { "dcoSub", 0.25f }, { "dcoNoise", 0 },
            { "hpf", 1 },
            { "vcfFreq", 0.48f }, { "vcfRes", 0.10f }, { "vcfPol", 0 }, { "vcfEnv", 0.22f },
            { "vcfLfo", 0.03f }, { "vcfKey", 0.45f },
            { "vcaMode", 0 }, { "vcaLevel", 0.82f },
            { "envA", 0.42f }, { "envD", 0.50f }, { "envS", 0.78f }, { "envR", 0.52f },
            { "chorus", 1 }, { "bendDco", 0.1667f }, { "bendVcf", 0 }, { "velSens", 0.2f }, { "volume", 0.75f } } },
        { "PWM PAD", {
            { "lfoRate", 0.36f }, { "lfoDelay", 0.2f },
            { "dcoRange", 1 }, { "dcoLfo", 0.05f }, { "dcoPw", 0.55f }, { "dcoPwmMode", 1 },
            { "dcoSaw", 0 }, { "dcoPulse", 1 }, { "dcoSub", 0.35f }, { "dcoNoise", 0 },
            { "hpf", 1 },
            { "vcfFreq", 0.44f }, { "vcfRes", 0.16f }, { "vcfPol", 0 }, { "vcfEnv", 0.28f },
            { "vcfLfo", 0.04f }, { "vcfKey", 0.4f },
            { "vcaMode", 0 }, { "vcaLevel", 0.8f },
            { "envA", 0.58f }, { "envD", 0.55f }, { "envS", 0.8f }, { "envR", 0.62f },
            { "chorus", 2 }, { "bendDco", 0.1667f }, { "bendVcf", 0 }, { "velSens", 0.25f }, { "volume", 0.75f } } },
        { "FAT BASS", {
            { "lfoRate", 0.3f }, { "lfoDelay", 0 },
            { "dcoRange", 0 }, { "dcoLfo", 0 }, { "dcoPw", 0.32f }, { "dcoPwmMode", 0 },
            { "dcoSaw", 1 }, { "dcoPulse", 1 }, { "dcoSub", 0.7f }, { "dcoNoise", 0 },
            { "hpf", 0 },
            { "vcfFreq", 0.36f }, { "vcfRes", 0.22f }, { "vcfPol", 0 }, { "vcfEnv", 0.38f },
            { "vcfLfo", 0 }, { "vcfKey", 0.3f },
            { "vcaMode", 0 }, { "vcaLevel", 0.85f },
            { "envA", 0.02f }, { "envD", 0.40f }, { "envS", 0.32f }, { "envR", 0.14f },
            { "chorus", 0 }, { "bendDco", 0.1667f }, { "bendVcf", 0 }, { "velSens", 0.4f }, { "volume", 0.78f } } },
        { "BRASS 1", {
            { "lfoRate", 0.5f }, { "lfoDelay", 0.45f },
            { "dcoRange", 1 }, { "dcoLfo", 0.1f }, { "dcoPw", 0 }, { "dcoPwmMode", 0 },
            { "dcoSaw", 1 }, { "dcoPulse", 0 }, { "dcoSub", 0.4f }, { "dcoNoise", 0 },
            { "hpf", 1 },
            { "vcfFreq", 0.34f }, { "vcfRes", 0.14f }, { "vcfPol", 0 }, { "vcfEnv", 0.52f },
            { "vcfLfo", 0 }, { "vcfKey", 0.5f },
            { "vcaMode", 0 }, { "vcaLevel", 0.85f },
            { "envA", 0.12f }, { "envD", 0.46f }, { "envS", 0.6f }, { "envR", 0.22f },
            { "chorus", 1 }, { "bendDco", 0.1667f }, { "bendVcf", 0 }, { "velSens", 0.35f }, { "volume", 0.75f } } },
        { "CHORUS PLUCK", {
            { "lfoRate", 0.4f }, { "lfoDelay", 0 },
            { "dcoRange", 1 }, { "dcoLfo", 0 }, { "dcoPw", 0.3f }, { "dcoPwmMode", 0 },
            { "dcoSaw", 1 }, { "dcoPulse", 1 }, { "dcoSub", 0.2f }, { "dcoNoise", 0 },
            { "hpf", 1 },
            { "vcfFreq", 0.3f }, { "vcfRes", 0.34f }, { "vcfPol", 0 }, { "vcfEnv", 0.5f },
            { "vcfLfo", 0 }, { "vcfKey", 0.7f },
            { "vcaMode", 0 }, { "vcaLevel", 0.85f },
            { "envA", 0 }, { "envD", 0.36f }, { "envS", 0 }, { "envR", 0.34f },
            { "chorus", 1 }, { "bendDco", 0.1667f }, { "bendVcf", 0 }, { "velSens", 0.5f }, { "volume", 0.78f } } },
        { "LESLIE ORGAN", {
            { "lfoRate", 0.4f }, { "lfoDelay", 0 },
            { "dcoRange", 1 }, { "dcoLfo", 0 }, { "dcoPw", 0 }, { "dcoPwmMode", 0 },
            { "dcoSaw", 0 }, { "dcoPulse", 1 }, { "dcoSub", 0.6f }, { "dcoNoise", 0 },
            { "hpf", 1 },
            { "vcfFreq", 0.78f }, { "vcfRes", 0.08f }, { "vcfPol", 0 }, { "vcfEnv", 0 },
            { "vcfLfo", 0 }, { "vcfKey", 0.3f },
            { "vcaMode", 1 }, { "vcaLevel", 0.72f },
            { "envA", 0 }, { "envD", 0.3f }, { "envS", 1.0f }, { "envR", 0.1f },
            { "chorus", 3 }, { "bendDco", 0.1667f }, { "bendVcf", 0 }, { "velSens", 0.15f }, { "volume", 0.72f } } },
        { "RES SWEEP PAD", {
            { "lfoRate", 0.22f }, { "lfoDelay", 0.15f },
            { "dcoRange", 1 }, { "dcoLfo", 0 }, { "dcoPw", 0.4f }, { "dcoPwmMode", 1 },
            { "dcoSaw", 1 }, { "dcoPulse", 0 }, { "dcoSub", 0.3f }, { "dcoNoise", 0 },
            { "hpf", 1 },
            { "vcfFreq", 0.28f }, { "vcfRes", 0.68f }, { "vcfPol", 0 }, { "vcfEnv", 0.32f },
            { "vcfLfo", 0.3f }, { "vcfKey", 0.4f },
            { "vcaMode", 0 }, { "vcaLevel", 0.78f },
            { "envA", 0.5f }, { "envD", 0.6f }, { "envS", 0.72f }, { "envR", 0.6f },
            { "chorus", 2 }, { "bendDco", 0.1667f }, { "bendVcf", 0 }, { "velSens", 0.2f }, { "volume", 0.72f } } },
        { "SOFT KEYS", {
            { "lfoRate", 0.42f }, { "lfoDelay", 0.5f },
            { "dcoRange", 1 }, { "dcoLfo", 0.06f }, { "dcoPw", 0.12f }, { "dcoPwmMode", 0 },
            { "dcoSaw", 0 }, { "dcoPulse", 1 }, { "dcoSub", 0.15f }, { "dcoNoise", 0 },
            { "hpf", 1 },
            { "vcfFreq", 0.5f }, { "vcfRes", 0.18f }, { "vcfPol", 0 }, { "vcfEnv", 0.34f },
            { "vcfLfo", 0 }, { "vcfKey", 0.6f },
            { "vcaMode", 0 }, { "vcaLevel", 0.8f },
            { "envA", 0.03f }, { "envD", 0.55f }, { "envS", 0.42f }, { "envR", 0.38f },
            { "chorus", 1 }, { "bendDco", 0.1667f }, { "bendVcf", 0 }, { "velSens", 0.55f }, { "volume", 0.75f } } }
    };
    return presets;
}

} // namespace juno
