#pragma once
#ifndef COMBO_MODULE_H
#define COMBO_MODULE_H

#include "base_effect_module.h"
#include "daisysp.h" // The Makefile will automatically find the LGPL files through this!

#ifdef __cplusplus

namespace bkshepherd {

class ComboModule : public BaseEffectModule {
  public:
    enum Param {
        DRIVE = 0,
        TONE,
        LEVEL,
        VOICE, 
        PARAM_COUNT
    };

    ComboModule();
    ~ComboModule();

    void Init(float sample_rate) override;
    void ProcessMono(float in) override;
    void ProcessStereo(float inL, float inR) override;
    
  private:
    // Notice the exact capitalization and the standard daisysp namespace!
    daisysp::Compressor m_alcCompressor;
    daisysp::Tone m_lowPassFilter;
    daisysp::ATone m_highPassFilter; 
    float m_prevState = 0.0f;
};
} 
#endif
#endif