#include "combo_module.h"
#include <array>
#include <math.h>

using namespace bkshepherd;

static const char *s_voiceNames[2] = {"Classic", "Cassette"};

static const auto s_metaData = [] {
    std::array<ParameterMetaData, ComboModule::PARAM_COUNT> params{};

    params[ComboModule::DRIVE] = {
        name : "Drive", valueType : ParameterValueType::Float,
        valueBinCount : 0, valueBinNames : nullptr,
        defaultValue : {.float_value = 2.0f}, knobMapping : 0, 
        midiCCMapping : -1, minValue : 1, maxValue : 15    
    };
    params[ComboModule::TONE] = {
        name : "Tone", valueType : ParameterValueType::Float,
        valueBinCount : 0, valueBinNames : nullptr,
        defaultValue : {.float_value = 10000.0f}, knobMapping : 1, 
        midiCCMapping : -1, minValue : 500, maxValue : 15000 
    };
    params[ComboModule::LEVEL] = {
        name : "Level", valueType : ParameterValueType::Float,
        valueBinCount : 0, valueBinNames : nullptr,
        defaultValue : {.float_value = 0.8f}, knobMapping : 2, 
        midiCCMapping : -1, minValue : 0, maxValue : 1
    };
    params[ComboModule::VOICE] = {
        name : "Voice", valueType : ParameterValueType::Binned,
        valueBinCount : 2, valueBinNames : s_voiceNames,
        defaultValue : {.uint_value = 0}, 
        knobMapping : -1, midiCCMapping : -1,
        toggleMapping : 2
    };
    return params;
}();

ComboModule::ComboModule() : BaseEffectModule() {
    m_name = "TapeSaturator";
    m_paramMetaData = s_metaData.data();
    this->InitParams(static_cast<int>(s_metaData.size()));
}

ComboModule::~ComboModule() {}

void ComboModule::Init(float sample_rate) {
    BaseEffectModule::Init(sample_rate);
    
    m_alcCompressor.Init(sample_rate);
    m_alcCompressor.SetThreshold(-24.0f); 
    m_alcCompressor.SetRatio(10.0f);      
    m_alcCompressor.SetAttack(0.01f);     
    m_alcCompressor.SetRelease(0.1f);     
    
    m_lowPassFilter.Init(sample_rate);
    m_highPassFilter.Init(sample_rate);
}

void ComboModule::ProcessMono(float in) {
    BaseEffectModule::ProcessMono(in);

    float drive = GetParameterAsFloat(DRIVE);
    float toneFreq = GetParameterAsFloat(TONE);
    float level = GetParameterAsFloat(LEVEL);
    int voice = GetParameterAsBinnedValue(VOICE); 
    
    bool isCassette = (voice == 2); 
    float out = in;

if (isCassette) {
        // 1. ALC Squish
        out = m_alcCompressor.Process(out);
        
        float hpFreq = 60.0f; 
        m_highPassFilter.SetFreq(hpFreq); 
        
        float lpFreq = toneFreq * 0.6f;
        m_lowPassFilter.SetFreq(lpFreq); 
        
        // 2. STATEFUL HYSTERESIS SATURATION
        float hystAmount = 0.6f; // The "alpha" memory coefficient (0.0 to 0.99)
        float inputDriven = out * drive;
        
        // Add magnetic memory to the current input
        float statefulInput = inputDriven + (m_prevState * hystAmount);
        
        // Saturate the combined signal
        out = tanhf(statefulInput);
        
        // Save this output to become the "memory" for the next sample!
        m_prevState = out;
        
    } else {
        float hpFreq = 20.0f; 
        m_highPassFilter.SetFreq(hpFreq); 
        
        float lpFreq = toneFreq;
        m_lowPassFilter.SetFreq(lpFreq);
        
        out = tanhf(out * (drive * 0.5f)) * 1.5f; 
    }

    out = m_highPassFilter.Process(out);
    out = m_lowPassFilter.Process(out);
    out = out * level;

    m_audioLeft = out;
    m_audioRight = out;
}

void ComboModule::ProcessStereo(float inL, float inR) {
    ProcessMono(inL);
}