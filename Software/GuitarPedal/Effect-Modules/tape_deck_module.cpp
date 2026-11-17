#include "tape_deck_module.h"
#include <algorithm>
#include <array>

using namespace bkshepherd;

daisysp::DelayLine<float, 48000> DSY_SDRAM_BSS dt_lagDeckL;
daisysp::DelayLine<float, 48000> DSY_SDRAM_BSS dt_lagDeckR;
daisysp::DelayLine<float, 48000> DSY_SDRAM_BSS dt_refDeckL;
daisysp::DelayLine<float, 48000> DSY_SDRAM_BSS dt_refDeckR;

static const char *s_voiceNames[2] = {"Classic", "Cassette"};
static const char *s_dtToggleNames[2] = {"Sum", "Invert"};

static const auto s_metaData = [] {
    std::array<ParameterMetaData, TapeDeckModule::PARAM_COUNT> params{};
    params[TapeDeckModule::DRIVE] = {
        name : "Drive",
        valueType : ParameterValueType::Float,
        valueBinCount : 0,
        defaultValue : {.float_value = 0.5f},
        knobMapping : 0,
        midiCCMapping : 14
    };
    params[TapeDeckModule::TONE] = {
        name : "Tone",
        valueType : ParameterValueType::Float,
        valueBinCount : 0,
        defaultValue : {.float_value = 0.5f},
        knobMapping : 1,
        midiCCMapping : 15
    };
    params[TapeDeckModule::VOLUME] = {
        name : "Volume",
        valueType : ParameterValueType::Float,
        valueBinCount : 0,
        defaultValue : {.float_value = 0.5f},
        knobMapping : 2,
        midiCCMapping : 16
    };
    params[TapeDeckModule::VOICE] = {
        name : "Voice",
        valueType : ParameterValueType::Binned,
        valueBinCount : 2,
        valueBinNames : s_voiceNames,
        defaultValue : {.uint_value = 0}, // 0 = Classic, 1 = Cassette
        knobMapping : -1, // Assuming Knob 3 or mapped to a physical toggle via hardware update
        midiCCMapping : 17,
        toggleMapping : 3
    };
    params[TapeDeckModule::LAG_TIME] = {
        name : "Lag Time",
        valueType : ParameterValueType::Float,
        valueBinCount : 0,
        defaultValue : {.float_value = 0.5f},
        knobMapping : 3,
        midiCCMapping : 18
    };
    params[TapeDeckModule::BLEND] = {
        name : "Blend",
        valueType : ParameterValueType::Float,
        valueBinCount : 0,
        defaultValue : {.float_value = 0.5f},
        knobMapping : 4,
        midiCCMapping : 19
    };
    params[TapeDeckModule::WOBBLE] = {
        name : "Wobble",
        valueType : ParameterValueType::Float,
        valueBinCount : 0,
        defaultValue : {.float_value = 0.5f},
        knobMapping : 5,
        midiCCMapping : 20
    };
    params[TapeDeckModule::DT_TOGGLE] = {
        name : "Doubletracker Toggle",
        valueType : ParameterValueType::Binned,
        valueBinCount : 2,
        valueBinNames : s_dtToggleNames,
        defaultValue : {.uint_value = 0}, // 0 = Sum, 1 = Invert
        knobMapping : -1,
        midiCCMapping : 21,
        toggleMapping : 2
    };
    return params;
}();

TapeDeckModule::TapeDeckModule() : BaseEffectModule() {
    m_name = "Tape Deck";
    m_paramMetaData = s_metaData.data();
    this->InitParams(static_cast<int>(s_metaData.size()));
}

TapeDeckModule::~TapeDeckModule() {}

void TapeDeckModule::Init(float sample_rate) {
    BaseEffectModule::Init(sample_rate);

    m_toneLpStateL = 0.0f;
    m_toneLpStateR = 0.0f;

    // Initialize Cassette ALC (Auto Level Control) using a fast attack/slow release compressor
    m_alcCompressorL.Init(sample_rate);
    m_alcCompressorL.SetThreshold(-20.0f);
    m_alcCompressorL.SetRatio(4.0f);
    m_alcCompressorL.SetAttack(0.01f);
    m_alcCompressorL.SetRelease(0.5f);

    m_alcCompressorR.Init(sample_rate);
    m_alcCompressorR.SetThreshold(-20.0f);
    m_alcCompressorR.SetRatio(4.0f);
    m_alcCompressorR.SetAttack(0.01f);
    m_alcCompressorR.SetRelease(0.5f);

    m_tapeMod.Init(sample_rate);
    dt_lagDeckL.Init();
    dt_lagDeckR.Init();
    dt_refDeckL.Init();
    dt_refDeckR.Init();
    m_currentLagSamples = 2.0f * (sample_rate / 1000.0f);
    m_fadeStep = 1.0f / (0.015f * sample_rate);

    // Compute initial tone filter coefficients for the default knob position
    ParameterChanged(TONE);
}

void TapeDeckModule::ParameterChanged(int parameter_id) {
    if (parameter_id == TONE) {
        float tone = GetParameterAsFloat(TONE); // 0.0 to 1.0
        float sampleRate = GetSampleRate();
        if (sampleRate <= 0.0f) sampleRate = 48000.0f;

        // Klon-style shelf corner frequency (~1.2 kHz)
        float cutoffFreq = 1200.0f;
        m_toneAlpha = 1.0f - std::exp(-2.0f * 3.14159265f * cutoffFreq / sampleRate);

        // Map 0.0..0.5 -> -12dB..0dB cut, and 0.5..1.0 -> 0dB..+14dB active boost
        float gainDb = (tone < 0.5f)
            ? ((tone - 0.5f) * 24.0f)   // -12 dB at 0.0, 0 dB at 0.5
            : ((tone - 0.5f) * 28.0f);  // 0 dB at 0.5, +14 dB at 1.0

        m_toneHighGain = std::pow(10.0f, gainDb / 20.0f);
    }
}
// Footswitch 1 (Bypass) controls the Doubletracker
void TapeDeckModule::SetEnabled(bool isEnabled) {
    BaseEffectModule::SetEnabled(isEnabled);
    m_dtEnabled = isEnabled;
}

// Footswitch 2 (Alternate) controls the Saturation
void TapeDeckModule::AlternateFootswitchPressed() {
    m_satEnabled = !m_satEnabled;
}
float TapeDeckModule::Langevin(float x) {
    // Prevent division by zero[cite: 3]
    if (std::abs(x) > 1e-4f) {
        return (1.0f / std::tanh(x)) - (1.0f / x);
    } else {
        return x / 3.0f;
    }
}

float TapeDeckModule::ProcessHysteresis(float in, float& M_state) {
    // Simplified discrete-time approximation of the Jiles-Atherton model[cite: 3]
    const float a = 0.5f;     // Shaping parameter
    const float alpha = 0.1f; // Coupling
    const float M_s = 1.0f;   // Saturation magnetization

    // Q(t) calculation[cite: 3]
    float Q = (in + alpha * M_state) / a;
    
    // Anisotropic magnetization: M_an = M_s * L(Q)[cite: 3]
    float M_an = M_s * Langevin(Q);
    
    // Simple iterative update for state (combining derivatives into a saturating lowpass behavior)
    M_state = M_state + 0.5f * (M_an - M_state);
    
    return M_state;
}

void TapeDeckModule::ProcessMono(float in) {
    BaseEffectModule::ProcessMono(in);
    float signal = in;

    // --- 1. SMOOTH SATURATION RAMP & DSP ---
    float satTarget = m_satEnabled ? 1.0f : 0.0f;
    if (m_satMix < satTarget) {
        m_satMix = std::min(satTarget, m_satMix + m_fadeStep);
    } else if (m_satMix > satTarget) {
        m_satMix = std::max(satTarget, m_satMix - m_fadeStep);
    }

    if (m_satMix > 0.0f) {
        float drySat = signal;
        float drive = 1.0f + (GetParameterAsFloat(DRIVE) * 10.0f);
        float volume = GetParameterAsFloat(VOLUME) * 2.0f;
        int voice = GetParameterAsBinnedValue(VOICE) - 1;

        float wetSat = drySat * drive;

        if (voice == 0) {
            wetSat = ProcessHysteresis(wetSat, m_M_n1_L);
        } else {
            wetSat = m_alcCompressorL.Process(wetSat);
            wetSat = std::tanh(wetSat);
        }
       // Active High-Shelf EQ: split signal into lows + highs, then scale highs
        m_toneLpStateL += m_toneAlpha * (wetSat - m_toneLpStateL);
        float lowBand = m_toneLpStateL;
        float highBand = wetSat - lowBand;
        wetSat = lowBand + (highBand * m_toneHighGain);

        wetSat = wetSat * volume;

        // Smooth 15ms crossfade prevents any click when toggling FS1
        signal = drySat + (m_satMix * (wetSat - drySat));
    }

    // --- 2. CONTINUOUS DELAY BUFFERING & DOUBLETRACKER ---
    // Always write live audio into the delay lines so they never contain stale/zeroed audio on engage
    dt_refDeckL.Write(signal);
    dt_lagDeckL.Write(signal);

    float dtTarget = m_dtEnabled ? 1.0f : 0.0f;
    if (m_dtMix < dtTarget) {
        m_dtMix = std::min(dtTarget, m_dtMix + m_fadeStep);
    } else if (m_dtMix > dtTarget) {
        m_dtMix = std::max(dtTarget, m_dtMix - m_fadeStep);
    }

    if (m_dtMix > 0.0f) {
        float preDtSignal = signal;

        // 1. Piecewise Lag Time mapping (Linear -0.3 to 3.0, Logarithmic 3.0 to 500)
        float lagKnob = GetParameterAsFloat(LAG_TIME);
        float lagTimeMs = 0.0f;
        
        if (lagKnob <= 0.5f) {
            float t = lagKnob * 2.0f; 
            lagTimeMs = -0.3f + (t * 3.3f); 
        } else {
            float t = (lagKnob - 0.5f) * 2.0f; 
            lagTimeMs = 3.0f * powf(500.0f / 3.0f, t); 
        }

        // 2. Absolute Wobble Math
        float wobbleKnob = GetParameterAsFloat(WOBBLE);
        float sweepDepthMs = 1.5f + (powf(wobbleKnob, 2.0f) * 10.0f); 
        
        float wowRate = 0.5f + (wobbleKnob * 2.0f);
        float flutterRate = 2.0f + (wobbleKnob * 5.0f);
        float speedVariation = m_tapeMod.GetTapeSpeed(wowRate, flutterRate, 1.0f, 0.5f);

        // 3. Convert to samples and apply
        float sampleRate = GetSampleRate();
        float refSamples = 2.0f * (sampleRate / 1000.0f);
        float baseLagSamples = refSamples + (lagTimeMs * (sampleRate / 1000.0f));
        float sweepSamples = sweepDepthMs * (sampleRate / 1000.0f);

        float modulatedLagSamples = baseLagSamples + (speedVariation * sweepSamples);
        if (modulatedLagSamples < 1.0f) modulatedLagSamples = 1.0f;
        if (modulatedLagSamples > 47999.0f) modulatedLagSamples = 47999.0f;

        daisysp::fonepole(m_currentLagSamples, modulatedLagSamples, 0.0002f);

        // 4. Read from the decks
        dt_refDeckL.SetDelay(refSamples);
        dt_lagDeckL.SetDelay(m_currentLagSamples);
        
        float dryRef = dt_refDeckL.Read();
        float wetLag = dt_lagDeckL.Read();

        // Sum/Invert Toggle
        if (GetParameterAsBinnedValue(DT_TOGGLE) - 1 == 1) { 
            wetLag = -wetLag;
        }

        // Equal-power crossfade for unity gain
        float blend = GetParameterAsFloat(BLEND);
        float dryGain = std::cos(blend * 1.5707963f);
        float wetGain = std::sin(blend * 1.5707963f);
        float wetDt = (dryRef * dryGain) + (wetLag * wetGain);

        // Smooth 15ms crossfade prevents any click when toggling FS2
        signal = preDtSignal + (m_dtMix * (wetDt - preDtSignal));
    }

    m_audioLeft = signal;
    m_audioRight = signal;
}

void TapeDeckModule::ProcessStereo(float inL, float inR) {
    ProcessMono(inL); 
    // If you ever want to run this in true stereo, duplicate the mono block here, 
    // substituting inR, m_M_n1_R, and the right-channel delay lines.
}

float TapeDeckModule::GetBrightnessForLED(int led_id) const {
    if (led_id == 0) return m_dtEnabled ? 1.0f : 0.0f;  // Right LED -> FS2 (Doubletracker)
    if (led_id == 1) return m_satEnabled ? 1.0f : 0.0f; // Left LED -> FS1 (Saturation)
    return BaseEffectModule::GetBrightnessForLED(led_id);
}