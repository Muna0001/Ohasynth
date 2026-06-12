/*
 * Oh-a-synth DSP core — C++ port of js/engine/worklet.js.
 *
 * Header-only, no JUCE dependencies: the same per-sample code as the web
 * version so the two stay in sync. One Engine instance == one plugin
 * instance (6 voices, global LFO, shared ADSR, BBD chorus).
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace juno {

constexpr int kNumVoices = 6;

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float expMap(float v, float lo, float hi) { return lo * std::pow(hi / lo, v); }

// ---------------------------------------------------------------------
// Parameter smoother (kills zipper noise)
// ---------------------------------------------------------------------
struct Smoother {
    float v = 0, t = 0, c = 1;
    void setTau(double fs, double sec) {
        c = (float) (1.0 - std::exp(-1.0 / (std::max(sec, 5e-5) * fs)));
    }
    void snap(float x) { v = t = x; }
    float tick() { v += (t - v) * c; return v; }
};

// polyBLEP residual for band-limited edges
inline float polyblep(float t, float dt) {
    if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// Tiny fixed per-voice component tolerances (VCF cutoff in octaves, VCA
// gain). DCO pitch never drifts — that is the Juno character.
constexpr float kVoiceFcOff[kNumVoices]  = { 0.016f, -0.021f, 0.009f, -0.014f, 0.022f, -0.008f };
constexpr float kVoiceVcaOff[kNumVoices] = { 1.0f, 0.987f, 1.011f, 0.992f, 1.007f, 0.996f };

struct Voice {
    bool active = false, gate = false;
    int note = 60;
    float velG = 1.0f;
    uint32_t age = 0;
    double phase = 0, subPhase = 0;
    float env = 0;
    int stage = 0; // 0 idle, 1 attack, 2 decay, 3 release
    float gateLvl = 0;
    float s1 = 0, s2 = 0, s3 = 0, s4 = 0; // ladder states
    float hpY = 0, hpX = 0;               // HPF state
    float shelfLp = 0;                    // low-shelf state for HPF pos 0
};

// ---------------------------------------------------------------------
// BBD chorus: one delay line, two taps modulated in antiphase
// ---------------------------------------------------------------------
struct ChorusMode { float wet, rate, minD, maxD; };
constexpr ChorusMode kChorusModes[4] = {
    { 0.0f, 0.500f, 0.00300f, 0.00420f }, // off
    { 0.5f, 0.513f, 0.00154f, 0.00515f }, // I  : slow & lush
    { 0.5f, 0.863f, 0.00154f, 0.00515f }, // II : faster
    { 0.5f, 9.750f, 0.00322f, 0.00356f }  // I+II: fast shallow warble
};

struct Chorus {
    std::vector<float> buf;
    int size = 0, w = 0;
    double fs = 48000, phase = 0;
    float rate = 0.5f;
    Smoother wet, center, depth;
    float lpCL = 0, lpCR = 0, lpC = 0;
    float hpL = 0, hpLx = 0, hpR = 0, hpRx = 0, hpA = 0;
    float outL = 0, outR = 0;

    void prepare(double sampleRate) {
        fs = sampleRate;
        size = (int) std::ceil(0.025 * fs);
        buf.assign((size_t) size, 0.0f);
        w = 0; phase = 0;
        wet.setTau(fs, 0.05);    wet.snap(0);
        center.setTau(fs, 0.08); center.snap(0.0036f);
        depth.setTau(fs, 0.08);  depth.snap(0.0006f);
        lpC = (float) (1.0 - std::exp(-2.0 * M_PI * 9000.0 / fs));
        hpA = (float) std::exp(-2.0 * M_PI * 60.0 / fs);
        lpCL = lpCR = hpL = hpLx = hpR = hpRx = 0;
    }

    void setMode(int m) {
        const auto& md = kChorusModes[m & 3];
        rate = md.rate;
        wet.t = md.wet;
        center.t = (md.minD + md.maxD) * 0.5f;
        depth.t = (md.maxD - md.minD) * 0.5f;
    }

    float read(float delaySec) const {
        float d = delaySec * (float) fs;
        if (d < 2.0f) d = 2.0f;
        float rp = (float) w - d;
        int i0 = (int) std::floor(rp);
        float fr = rp - (float) i0;
        auto wrap = [this](int i) { return ((i % size) + size) % size; };
        float xm1 = buf[(size_t) wrap(i0 - 1)];
        float x0  = buf[(size_t) wrap(i0)];
        float x1  = buf[(size_t) wrap(i0 + 1)];
        float x2  = buf[(size_t) wrap(i0 + 2)];
        float c1 = 0.5f * (x1 - xm1);
        float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
        float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
        return ((c3 * fr + c2) * fr + c1) * fr + x0;
    }

    void process(float x) {
        buf[(size_t) w] = x;
        w = (w + 1) % size;
        phase += rate / fs;
        if (phase >= 1.0) phase -= 1.0;
        float tri = phase < 0.5 ? (float) (phase * 4 - 1) : (float) (3 - phase * 4);
        float wetG = wet.tick();
        if (wetG < 0.0005f) { outL = x; outR = x; return; }
        float c = center.tick();
        float dep = depth.tick();
        float wl = read(c + dep * tri);
        float wr = read(c - dep * tri);
        lpCL += (wl - lpCL) * lpC; // BBD bandwidth limit
        lpCR += (wr - lpCR) * lpC;
        hpL = hpA * (hpL + lpCL - hpLx); hpLx = lpCL; // AC-couple wet path
        hpR = hpA * (hpR + lpCR - hpRx); hpRx = lpCR;
        float dry = 1.0f - wetG * 0.35f;
        outL = x * dry + hpL * wetG * 1.25f;
        outR = x * dry + hpR * wetG * 1.25f;
    }
};

// ---------------------------------------------------------------------
// Raw patch values, mirrors the web app's Juno.PARAMS schema.
// Continuous params are normalized 0..1; switches are indices.
// ---------------------------------------------------------------------
struct Params {
    float lfoRate = 0.40f, lfoDelay = 0.0f;
    int   dcoRange = 1;                 // 0=16' 1=8' 2=4'
    float dcoLfo = 0.0f, dcoPw = 0.0f;
    int   dcoPwmMode = 0;               // 0=MAN 1=LFO
    int   dcoSaw = 1, dcoPulse = 0;
    float dcoSub = 0.0f, dcoNoise = 0.0f;
    int   hpf = 1;                      // 0..3 (0 = bass boost)
    float vcfFreq = 0.55f, vcfRes = 0.0f;
    int   vcfPol = 0;                   // 0=normal 1=inverted env
    float vcfEnv = 0.25f, vcfLfo = 0.0f, vcfKey = 0.50f;
    int   vcaMode = 0;                  // 0=ENV 1=GATE
    float vcaLevel = 0.80f;
    float envA = 0.05f, envD = 0.40f, envS = 0.70f, envR = 0.30f;
    int   chorus = 0;                   // 0=off 1=I 2=II 3=I+II
    float bendDco = 0.1667f, bendVcf = 0.0f;
    float velSens = 0.30f, volume = 0.75f;
};

// ---------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------
class Engine {
public:
    void prepare(double sampleRate) {
        fs = sampleRate;
        log2_20 = std::log2(20.0f);
        log2_22k = std::log2(22000.0f);
        maxFc = (float) std::min(20000.0, fs * 0.45);

        smPw.setTau(fs, 0.012);     smSaw.setTau(fs, 0.008);
        smPulse.setTau(fs, 0.008);  smSub.setTau(fs, 0.012);
        smNoise.setTau(fs, 0.012);  smCutoff.setTau(fs, 0.018);
        smRes.setTau(fs, 0.018);    smEnvAmt.setTau(fs, 0.015);
        smVcfLfo.setTau(fs, 0.015); smKeyFlw.setTau(fs, 0.02);
        smVcaLvl.setTau(fs, 0.012); smVolume.setTau(fs, 0.015);
        smDcoLfo.setTau(fs, 0.015); smBend.setTau(fs, 0.006);
        smMod.setTau(fs, 0.02);

        gateC = tauCoef(0.0025f);
        shelfC = fcCoef(150.0f);
        dcA = (float) std::exp(-2.0 * M_PI * 12.0 / fs);
        chorus.prepare(fs);
        setParams(params, true);

        for (auto& v : voices) v = Voice();
        heldCount = 0;
        for (auto& h : held) h = false;
        lfoPhase = 0; lfoTime = 100;
        dcL = dcLx = dcR = dcRx = 0;
        noiseState = 0x12345678u;
    }

    // Apply a full set of raw parameter values (called once per block).
    void setParams(const Params& p, bool snap = false) {
        params = p;
        smPw.t = p.dcoPw;        smSaw.t = (float) p.dcoSaw;
        smPulse.t = (float) p.dcoPulse;
        smSub.t = p.dcoSub;      smNoise.t = p.dcoNoise;
        smCutoff.t = p.vcfFreq;  smRes.t = p.vcfRes;
        smEnvAmt.t = p.vcfEnv;   smVcfLfo.t = p.vcfLfo;
        smKeyFlw.t = p.vcfKey;   smVcaLvl.t = p.vcaLevel;
        smVolume.t = p.volume;   smDcoLfo.t = p.dcoLfo;

        rangeMult = p.dcoRange == 0 ? 0.5f : (p.dcoRange == 2 ? 2.0f : 1.0f);
        aC = tauCoef(expMap(p.envA, 0.0015f, 3.2f) / 1.45f);
        dC = tauCoef(expMap(p.envD, 0.0025f, 10.0f) / 3.2f);
        rC = tauCoef(expMap(p.envR, 0.0025f, 10.0f) / 3.2f);
        sus = p.envS;
        lfoRateHz = expMap(p.lfoRate, 0.1f, 30.0f);
        lfoDelaySec = std::pow(p.lfoDelay, 1.5f) * 2.5f;
        lfoFadeSec = 0.05f + lfoDelaySec * 0.6f;
        hpA = 1.0f - fcCoef(p.hpf == 3 ? 520.0f : 245.0f);
        chorus.setMode(p.chorus);
        bendSemis = p.bendDco * 12.0f;

        if (snap) {
            for (auto* s : { &smPw, &smSaw, &smPulse, &smSub, &smNoise, &smCutoff,
                             &smRes, &smEnvAmt, &smVcfLfo, &smKeyFlw, &smVcaLvl,
                             &smVolume, &smDcoLfo })
                s->snap(s->t);
        }
    }

    void noteOn(int note, float vel) {
        note = std::clamp(note, 0, 127);
        if (heldCount == 0) lfoTime = 0; // restart LFO delay
        if (!held[note]) { held[note] = true; ++heldCount; }

        Voice& v = findVoice(note);
        v.note = note;
        float vn = clampf(vel <= 0 ? 0.8f : vel, 0.01f, 1.0f);
        v.velG = 1.0f - params.velSens * (1.0f - std::pow(vn, 1.4f));
        v.gate = true;
        v.stage = 1; // env retriggers from its current level (analog feel)
        v.age = ++ageCounter;
        v.active = true;
    }

    void noteOff(int note) {
        note = std::clamp(note, 0, 127);
        if (sustainOn) { sustained[note] = held[note]; if (held[note]) { held[note] = false; --heldCount; } return; }
        releaseNote(note);
    }

    void setSustain(bool on) {
        sustainOn = on;
        if (!on) {
            for (int n = 0; n < 128; ++n)
                if (sustained[n]) { sustained[n] = false; releaseNote(n); }
        }
    }

    void pitchBend(float v) { smBend.t = clampf(v, -1.0f, 1.0f); }
    void modWheel(float v)  { smMod.t = clampf(v, 0.0f, 1.0f); }

    void allNotesOff() {
        for (auto& h : held) h = false;
        for (auto& s : sustained) s = false;
        heldCount = 0;
        for (auto& v : voices) {
            v.gate = false;
            if (v.active) v.stage = 3;
        }
    }

    void reset() {
        for (auto& v : voices) { v = Voice(); }
        allNotesOff();
    }

    // Render n samples of stereo output (replaces buffer contents).
    void render(float* L, float* R, int n) {
        const bool pwmLfoMode = params.dcoPwmMode == 1;
        const bool gateMode = params.vcaMode == 1;
        const float envSign = params.vcfPol == 1 ? -1.0f : 1.0f;
        const int hpfPos = params.hpf;
        const float bendVcf = clampf(params.bendVcf, 0.0f, 1.0f);
        const float lfoInc = (float) (lfoRateHz / fs);
        const bool holding = heldCount > 0;
        const float invFs = (float) (1.0 / fs);

        for (int i = 0; i < n; ++i) {
            // --- global modulators -----------------------------------
            lfoPhase += lfoInc;
            if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            float lfoRaw = lfoPhase < 0.5f ? lfoPhase * 4 - 1 : 3 - lfoPhase * 4;
            if (holding) lfoTime += invFs;
            float lfoEnv = clampf((lfoTime - lfoDelaySec) / lfoFadeSec, 0.0f, 1.0f);
            float lfo = lfoRaw * lfoEnv;

            float bend = smBend.tick();
            float mod = smMod.tick();
            float pwAmt = smPw.tick();
            float sawG = smSaw.tick();
            float pulseG = smPulse.tick();
            float subLvl = smSub.tick();
            float noiseLvl = smNoise.tick();
            float cutoff = smCutoff.tick();
            float res = smRes.tick();
            float envAmt = smEnvAmt.tick();
            float vcfLfoAmt = smVcfLfo.tick();
            float keyFlw = smKeyFlw.tick();
            float vcaLvl = smVcaLvl.tick();
            float volume = smVolume.tick();
            float vibDepth = clampf(smDcoLfo.tick() + mod * 0.4f, 0.0f, 1.2f);

            float pitchSemisMod = lfo * vibDepth * vibDepth * 8.0f + bend * bendSemis;

            float pw = pwmLfoMode
                ? 0.5f - (0.5f + 0.5f * lfo) * pwAmt * 0.44f
                : 0.5f - pwAmt * 0.44f;
            if (pw < 0.05f) pw = 0.05f;

            float baseCutOct = log2_20 + cutoff * (log2_22k - log2_20)
                + lfo * vcfLfoAmt * 3.0f
                + bend * bendVcf * 3.0f;

            float k = res * 4.6f; // ~4.0 is the self-oscillation threshold
            float resComp = 1.0f + res * 0.65f;

            // single noise source shared by all voices, like the hardware
            float noiseSig = nextNoise() * noiseLvl * 0.55f;

            float acc = 0;

            // --- voices ----------------------------------------------
            for (int vi = 0; vi < kNumVoices; ++vi) {
                Voice& v = voices[vi];
                if (!v.active) continue;

                if (v.stage == 1) {
                    v.env += (1.28f - v.env) * aC;
                    if (v.env >= 1.0f) { v.env = 1.0f; v.stage = 2; }
                } else if (v.stage == 2) {
                    v.env += (sus - v.env) * dC;
                } else if (v.stage == 3) {
                    v.env += (0.0f - v.env) * rC;
                }
                v.gateLvl += ((v.gate ? 1.0f : 0.0f) - v.gateLvl) * gateC;

                float amp = gateMode ? v.gateLvl : v.env;
                if (!v.gate && amp < 0.0001f) {
                    v.active = false; v.stage = 0; v.env = 0;
                    v.s1 = v.s2 = v.s3 = v.s4 = 0;
                    continue;
                }

                // --- DCO ---
                float semis = (float) (v.note - 60) + pitchSemisMod;
                float f = 261.625565f * std::pow(2.0f, semis / 12.0f) * rangeMult;
                if (f > (float) (fs * 0.45)) f = (float) (fs * 0.45);
                float dt = (float) (f / fs);

                v.phase += dt;
                if (v.phase >= 1.0) v.phase -= 1.0;
                float t = (float) v.phase;

                float saw = (2 * t - 1 - polyblep(t, dt)) * sawG;

                float tp = t - pw + 1.0f; if (tp >= 1.0f) tp -= 1.0f;
                float pulse = ((t < pw ? 1.0f : -1.0f)
                    + polyblep(t, dt) - polyblep(tp, dt)) * pulseG;

                float dt2 = dt * 0.5f;
                v.subPhase += dt2;
                if (v.subPhase >= 1.0) v.subPhase -= 1.0;
                float st = (float) v.subPhase;
                float st2 = st + 0.5f; if (st2 >= 1.0f) st2 -= 1.0f;
                float sub = ((st < 0.5f ? 1.0f : -1.0f)
                    + polyblep(st, dt2) - polyblep(st2, dt2)) * subLvl;

                float sig = saw * 0.5f + pulse * 0.5f + sub * 0.6f + noiseSig;

                // --- HPF (pos 0 is the famous bass boost) ---
                if (hpfPos == 0) {
                    v.shelfLp += (sig - v.shelfLp) * shelfC;
                    sig = sig + v.shelfLp * 0.55f;
                } else if (hpfPos >= 2) {
                    v.hpY = hpA * (v.hpY + sig - v.hpX);
                    v.hpX = sig;
                    sig = v.hpY;
                }

                // --- VCF: zero-delay-feedback 4-pole ladder ---
                float cutOct = baseCutOct
                    + (float) (v.note - 60) / 12.0f * keyFlw * 1.05f
                    + envSign * envAmt * v.env * 9.0f
                    + kVoiceFcOff[vi];
                float fc = std::pow(2.0f, cutOct);
                fc = clampf(fc, 8.0f, maxFc);

                float g = std::tan((float) M_PI * fc / (float) fs);
                float G = g / (1 + g);
                float G2 = G * G;

                float S = (G2 * G * v.s1 + G2 * v.s2 + G * v.s3 + v.s4) / (1 + g);
                float u = (sig * 0.7f - k * S) / (1 + k * G2 * G2);
                u = std::tanh(u); // bounds the loop => stable self-oscillation

                float w1 = (u - v.s1) * G;  float y1 = w1 + v.s1; v.s1 = y1 + w1;
                float w2 = (y1 - v.s2) * G; float y2 = w2 + v.s2; v.s2 = y2 + w2;
                float w3 = (y2 - v.s3) * G; float y3 = w3 + v.s3; v.s3 = y3 + w3;
                float w4 = (y3 - v.s4) * G; float y4 = w4 + v.s4; v.s4 = y4 + w4;

                // --- VCA ---
                acc += y4 * resComp * amp * vcaLvl * v.velG * kVoiceVcaOff[vi];
            }

            acc *= 0.62f; // headroom for 6 voices

            // --- chorus + output stage --------------------------------
            chorus.process(acc);
            float lOut = std::tanh(chorus.outL * volume * 1.6f);
            float rOut = std::tanh(chorus.outR * volume * 1.6f);

            dcL = dcA * (dcL + lOut - dcLx); dcLx = lOut;
            dcR = dcA * (dcR + rOut - dcRx); dcRx = rOut;

            L[i] = dcL;
            R[i] = dcR;
        }
    }

private:
    float tauCoef(float t) { return (float) (1.0 - std::exp(-1.0 / (std::max((double) t, 5e-5) * fs))); }
    float fcCoef(float fc) { return (float) (1.0 - std::exp(-2.0 * M_PI * fc / fs)); }

    void releaseNote(int note) {
        if (held[note]) { held[note] = false; --heldCount; }
        for (auto& v : voices)
            if (v.active && v.note == note && v.gate) {
                v.gate = false;
                if (v.stage != 0) v.stage = 3;
            }
    }

    // Voice allocation: reuse same note, else a free voice, else the
    // oldest releasing voice, else steal the oldest held voice.
    Voice& findVoice(int note) {
        for (auto& v : voices)
            if (v.active && v.note == note) return v;
        for (auto& v : voices)
            if (!v.active) return v;
        Voice* best = nullptr;
        for (auto& v : voices)
            if (v.stage == 3 && (best == nullptr || v.age < best->age)) best = &v;
        if (best) return *best;
        for (auto& v : voices)
            if (best == nullptr || v.age < best->age) best = &v;
        return *best;
    }

    float nextNoise() {
        // xorshift32 — cheap, deterministic white noise
        noiseState ^= noiseState << 13;
        noiseState ^= noiseState >> 17;
        noiseState ^= noiseState << 5;
        return (float) (int32_t) noiseState * (1.0f / 2147483648.0f);
    }

    double fs = 48000;
    float log2_20 = 4.32f, log2_22k = 14.42f, maxFc = 20000.0f;

    Params params;
    Voice voices[kNumVoices];
    uint32_t ageCounter = 0;
    bool held[128] = {}, sustained[128] = {};
    int heldCount = 0;
    bool sustainOn = false;

    Smoother smPw, smSaw, smPulse, smSub, smNoise, smCutoff, smRes, smEnvAmt,
             smVcfLfo, smKeyFlw, smVcaLvl, smVolume, smDcoLfo, smBend, smMod;

    float aC = 0, dC = 0, rC = 0, sus = 0.8f;
    float lfoRateHz = 1, lfoDelaySec = 0, lfoFadeSec = 0.05f;
    float hpA = 0, shelfC = 0, gateC = 0;
    float rangeMult = 1, bendSemis = 2;
    float lfoPhase = 0, lfoTime = 100;

    Chorus chorus;
    float dcA = 0, dcL = 0, dcLx = 0, dcR = 0, dcRx = 0;
    uint32_t noiseState = 0x12345678u;
};

} // namespace juno
