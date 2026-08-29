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
    
    m_toneFilterL.Init(sample_rate);
    m_toneFilterR.Init(sample_rate);
    
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
    m_tapeMod.Init(sample_rate); //[cite: 9]
    dt_lagDeckL.Init();
    dt_lagDeckR.Init();
    dt_refDeckL.Init();
    dt_refDeckR.Init();
}

void TapeDeckModule::ParameterChanged(int parameter_id) {
    if (parameter_id == TONE) {
        // Map tone 0.0 - 1.0 to 500Hz - 15000Hz
        float cutoff = 500.0f + (14700.0f * GetParameterAsFloat(TONE));
        m_toneFilterL.SetFreq(cutoff);
        m_toneFilterR.SetFreq(cutoff);
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
    
    // --- 1. TAPE SATURATION ---
    if (m_satEnabled) {
        float drive = 1.0f + (GetParameterAsFloat(DRIVE) * 10.0f);
        float volume = GetParameterAsFloat(VOLUME) * 2.0f;
        int voice = GetParameterAsBinnedValue(VOICE) - 1; 

        signal = signal * drive;

        if (voice == 0) {
            signal = ProcessHysteresis(signal, m_M_n1_L);
        } else {
            signal = m_alcCompressorL.Process(signal);
            signal = std::tanh(signal);
        }
    signal = m_toneFilterL.Process(signal);
        signal = signal * volume;
    }

    // --- 2. DOUBLETRACKER ---
    if (m_dtEnabled) {
        dt_refDeckL.Write(signal);
        dt_lagDeckL.Write(signal);

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
        
        // BASELINE: Always sweep by at least 1.5ms to guarantee built-in flange/chorus.
        // WOBBLE KNOB: Adds up to 10ms of extra, pronounced depth.
        float sweepDepthMs = 1.5f + (powf(wobbleKnob, 2.0f) * 10.0f); 
        
        // Modulator rates increase slightly as you turn the knob
        float wowRate = 0.5f + (wobbleKnob * 2.0f);
        float flutterRate = 2.0f + (wobbleKnob * 5.0f);
        
        // Get the normalized [-1.0 to 1.0] tape speed variation from FBM
        float speedVariation = m_tapeMod.GetTapeSpeed(wowRate, flutterRate, 1.0f, 0.5f);

        // 3. Convert everything to samples and apply
        float sampleRate = GetSampleRate();
        float refSamples = 2.0f * (sampleRate / 1000.0f); // 2ms fixed reference
        float baseLagSamples = refSamples + (lagTimeMs * (sampleRate / 1000.0f));
        float sweepSamples = sweepDepthMs * (sampleRate / 1000.0f);

        // Apply the absolute sweep to the base lag
        float modulatedLagSamples = baseLagSamples + (speedVariation * sweepSamples);

        // Clamp to prevent out-of-bounds array reads
        if (modulatedLagSamples < 1.0f) modulatedLagSamples = 1.0f;
        if (modulatedLagSamples > 47999.0f) modulatedLagSamples = 47999.0f;

        // GLIDE the delay time to the new target to prevent DSP crashing and clicking
        daisysp::fonepole(m_currentLagSamples, modulatedLagSamples, 0.0002f);

        // 4. Read from the decks
        dt_refDeckL.SetDelay(refSamples);
        dt_lagDeckL.SetDelay(m_currentLagSamples);
        
        float dryRef = dt_refDeckL.Read();
        float wetLag = dt_lagDeckL.Read();

        // Sum/Invert Toggle
        if (GetParameterAsBinnedValue(DT_TOGGLE) - 1 == 1) { 
            wetLag = -wetLag; // Phase flip
        }

        // Linear Blend
        float blend = GetParameterAsFloat(BLEND);
        signal = (dryRef * (1.0f - blend)) + (wetLag * blend);
    }

    m_audioLeft = signal;
    m_audioRight = signal; // Dual-mono output
}

void TapeDeckModule::ProcessStereo(float inL, float inR) {
    ProcessMono(inL); 
    // If you ever want to run this in true stereo, duplicate the mono block here, 
    // substituting inR, m_M_n1_R, and the right-channel delay lines.
}

float TapeDeckModule::GetBrightnessForLED(int led_id) const {
    if (led_id == 0) return m_dtEnabled ? 1.0f : 0.0f;
    if (led_id == 1) return m_satEnabled ? 1.0f : 0.0f;
    return BaseEffectModule::GetBrightnessForLED(led_id);
}