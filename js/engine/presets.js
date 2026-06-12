/*
 * Factory presets. Values follow the normalized schema in Juno.PARAMS.
 */
(function () {
  'use strict';
  window.Juno = window.Juno || {};

  Juno.PRESETS = [
    {
      name: 'STRINGS 1',
      lfoRate: 0.45, lfoDelay: 0.35,
      dcoRange: 1, dcoLfo: 0.14, dcoPw: 0.35, dcoPwmMode: 1,
      dcoSaw: 1, dcoPulse: 0, dcoSub: 0.25, dcoNoise: 0,
      hpf: 1,
      vcfFreq: 0.48, vcfRes: 0.10, vcfPol: 0, vcfEnv: 0.22, vcfLfo: 0.03, vcfKey: 0.45,
      vcaMode: 0, vcaLevel: 0.82,
      envA: 0.42, envD: 0.50, envS: 0.78, envR: 0.52,
      chorus: 1, bendDco: 0.1667, bendVcf: 0, velSens: 0.2, volume: 0.75
    },
    {
      name: 'PWM PAD',
      lfoRate: 0.36, lfoDelay: 0.2,
      dcoRange: 1, dcoLfo: 0.05, dcoPw: 0.55, dcoPwmMode: 1,
      dcoSaw: 0, dcoPulse: 1, dcoSub: 0.35, dcoNoise: 0,
      hpf: 1,
      vcfFreq: 0.44, vcfRes: 0.16, vcfPol: 0, vcfEnv: 0.28, vcfLfo: 0.04, vcfKey: 0.4,
      vcaMode: 0, vcaLevel: 0.8,
      envA: 0.58, envD: 0.55, envS: 0.8, envR: 0.62,
      chorus: 2, bendDco: 0.1667, bendVcf: 0, velSens: 0.25, volume: 0.75
    },
    {
      name: 'FAT BASS',
      lfoRate: 0.3, lfoDelay: 0,
      dcoRange: 0, dcoLfo: 0, dcoPw: 0.32, dcoPwmMode: 0,
      dcoSaw: 1, dcoPulse: 1, dcoSub: 0.7, dcoNoise: 0,
      hpf: 0,
      vcfFreq: 0.36, vcfRes: 0.22, vcfPol: 0, vcfEnv: 0.38, vcfLfo: 0, vcfKey: 0.3,
      vcaMode: 0, vcaLevel: 0.85,
      envA: 0.02, envD: 0.40, envS: 0.32, envR: 0.14,
      chorus: 0, bendDco: 0.1667, bendVcf: 0, velSens: 0.4, volume: 0.78
    },
    {
      name: 'BRASS 1',
      lfoRate: 0.5, lfoDelay: 0.45,
      dcoRange: 1, dcoLfo: 0.1, dcoPw: 0, dcoPwmMode: 0,
      dcoSaw: 1, dcoPulse: 0, dcoSub: 0.4, dcoNoise: 0,
      hpf: 1,
      vcfFreq: 0.34, vcfRes: 0.14, vcfPol: 0, vcfEnv: 0.52, vcfLfo: 0, vcfKey: 0.5,
      vcaMode: 0, vcaLevel: 0.85,
      envA: 0.12, envD: 0.46, envS: 0.6, envR: 0.22,
      chorus: 1, bendDco: 0.1667, bendVcf: 0, velSens: 0.35, volume: 0.75
    },
    {
      name: 'CHORUS PLUCK',
      lfoRate: 0.4, lfoDelay: 0,
      dcoRange: 1, dcoLfo: 0, dcoPw: 0.3, dcoPwmMode: 0,
      dcoSaw: 1, dcoPulse: 1, dcoSub: 0.2, dcoNoise: 0,
      hpf: 1,
      vcfFreq: 0.3, vcfRes: 0.34, vcfPol: 0, vcfEnv: 0.5, vcfLfo: 0, vcfKey: 0.7,
      vcaMode: 0, vcaLevel: 0.85,
      envA: 0.0, envD: 0.36, envS: 0.0, envR: 0.34,
      chorus: 1, bendDco: 0.1667, bendVcf: 0, velSens: 0.5, volume: 0.78
    },
    {
      name: 'LESLIE ORGAN',
      lfoRate: 0.4, lfoDelay: 0,
      dcoRange: 1, dcoLfo: 0, dcoPw: 0, dcoPwmMode: 0,
      dcoSaw: 0, dcoPulse: 1, dcoSub: 0.6, dcoNoise: 0,
      hpf: 1,
      vcfFreq: 0.78, vcfRes: 0.08, vcfPol: 0, vcfEnv: 0, vcfLfo: 0, vcfKey: 0.3,
      vcaMode: 1, vcaLevel: 0.72,
      envA: 0.0, envD: 0.3, envS: 1.0, envR: 0.1,
      chorus: 3, bendDco: 0.1667, bendVcf: 0, velSens: 0.15, volume: 0.72
    },
    {
      name: 'RES SWEEP PAD',
      lfoRate: 0.22, lfoDelay: 0.15,
      dcoRange: 1, dcoLfo: 0, dcoPw: 0.4, dcoPwmMode: 1,
      dcoSaw: 1, dcoPulse: 0, dcoSub: 0.3, dcoNoise: 0,
      hpf: 1,
      vcfFreq: 0.28, vcfRes: 0.68, vcfPol: 0, vcfEnv: 0.32, vcfLfo: 0.3, vcfKey: 0.4,
      vcaMode: 0, vcaLevel: 0.78,
      envA: 0.5, envD: 0.6, envS: 0.72, envR: 0.6,
      chorus: 2, bendDco: 0.1667, bendVcf: 0, velSens: 0.2, volume: 0.72
    },
    {
      name: 'SOFT KEYS',
      lfoRate: 0.42, lfoDelay: 0.5,
      dcoRange: 1, dcoLfo: 0.06, dcoPw: 0.12, dcoPwmMode: 0,
      dcoSaw: 0, dcoPulse: 1, dcoSub: 0.15, dcoNoise: 0,
      hpf: 1,
      vcfFreq: 0.5, vcfRes: 0.18, vcfPol: 0, vcfEnv: 0.34, vcfLfo: 0, vcfKey: 0.6,
      vcaMode: 0, vcaLevel: 0.8,
      envA: 0.03, envD: 0.55, envS: 0.42, envR: 0.38,
      chorus: 1, bendDco: 0.1667, bendVcf: 0, velSens: 0.55, volume: 0.75
    }
  ];
})();
