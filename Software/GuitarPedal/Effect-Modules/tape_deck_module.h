#pragma once
#ifndef TAPE_DECK_MODULE_H
#define TAPE_DECK_MODULE_H

#include "base_effect_module.h"
#include "daisysp.h"
#include "../Util/tape_modulator.h"
#include <stdint.h>
#include <cmath>

using namespace daisysp;

namespace bkshepherd {

class TapeDeckModule : public BaseEffectModule {
  public:
    enum Param {
        DRIVE = 0,
        TONE,
        VOLUME,
        VOICE, // 0 = Classic, 1 = Cassette
        // Doubletracker Params
        LAG_TIME,
        BLEND,
        WOBBLE,
        DT_TOGGLE, // 0 = Sum, 1 = Invert
        PARAM_COUNT
    };

    TapeDeckModule();
    ~TapeDeckModule();

    void Init(float sample_rate) override;
    void ParameterChanged(int parameter_id) override;
    void ProcessMono(float in) override;
    void ProcessStereo(float inL, float inR) override;
    float GetBrightnessForLED(int led_id) const override;
    void SetEnabled(bool isEnabled) override;
    void AlternateFootswitchPressed() override;
    bool IsEitherModuleEnabled() const {
    return m_satEnabled || m_dtEnabled || m_satMix > 0.0f || m_dtMix > 0.0f;
}

private:
    bool m_dtEnabled = false;
    bool m_satEnabled = false;
    float m_satMix = 0.0f;
    float m_dtMix = 0.0f;
    float m_fadeStep = 0.0014f;

    // Klon-style Active High-Shelf Tone Filter
    float m_toneLpStateL = 0.0f;
    float m_toneLpStateR = 0.0f;
    float m_toneAlpha = 0.2f;
    float m_toneHighGain = 1.0f;

    // Cassette ALC Compressors
    daisysp::Compressor m_alcCompressorL;
    daisysp::Compressor m_alcCompressorR;

    float m_currentLagSamples = 0.0f;

    // Hysteresis State Variables
    float m_M_n1_L = 0.0f;
    float m_M_n1_R = 0.0f;
    TapeModulator m_tapeMod;

    // Langevin function approximation
    float Langevin(float x);
    // Core Hysteresis Processing
    float ProcessHysteresis(float in, float& M_state);
};

} // namespace bkshepherd
#endif