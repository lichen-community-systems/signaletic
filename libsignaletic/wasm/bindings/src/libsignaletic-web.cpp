#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <libsignaletic.h>

class Signals {
public:
    void Signal_SingleMonoOutput_newAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_Signal_SingleMonoOutput* outputs) {
        sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
            audioSettings, outputs);
    }

    void Signal_SingleMonoOutput_destroyAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_dsp_Signal_SingleMonoOutput* outputs) {
        sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
            outputs);
    }

    void evaluateSignals(struct sig_List* signalList) {
        return sig_dsp_evaluateSignals(signalList);
    }

    struct sig_dsp_SignalListEvaluator* SignalListEvaluator_new(
        struct sig_Allocator* allocator, struct sig_List* signalList) {
        return sig_dsp_SignalListEvaluator_new(allocator, signalList);
    }

    void SignalListEvaluator_init(
        struct sig_dsp_SignalListEvaluator* self,
        struct sig_List* signalList) {
        sig_dsp_SignalListEvaluator_init(self, signalList);
    }

    void SignalListEvaluator_evaluate(
        struct sig_dsp_SignalEvaluator* self) {
        sig_dsp_SignalListEvaluator_evaluate(self);
    }

    void SignalListEvaluator_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_SignalListEvaluator* self) {
        sig_dsp_SignalListEvaluator_destroy(allocator, self);
    }

    struct sig_dsp_Value* Value_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Value_new(allocator, context);
    }

    void Value_init(struct sig_dsp_Value* self,
        struct sig_SignalContext* context) {
        sig_dsp_Value_init(self, context);
    }

    void Value_generate(void* signal) {
        sig_dsp_Value_generate(signal);
    }

    void Value_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Value* self) {
        return sig_dsp_Value_destroy(allocator, self);
    }

    struct sig_dsp_ConstantValue* ConstantValue_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context,
        float value) {
        return sig_dsp_ConstantValue_new(allocator, context, value);
    }

    void ConstantValue_init(struct sig_dsp_ConstantValue* self,
        struct sig_SignalContext* context, float value) {
        sig_dsp_ConstantValue_init(self, context, value);
    }

    void ConstantValue_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_ConstantValue* self) {
        return sig_dsp_ConstantValue_destroy(allocator, self);
    }

    struct sig_dsp_Abs* Abs_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Abs_new(allocator, context);
    }

    void Abs_init(struct sig_dsp_Abs* self,
        struct sig_SignalContext* context) {
            sig_dsp_Abs_init(self, context);
    }

    void Abs_generate(void* signal) {
        sig_dsp_Abs_generate(signal);
    }

    void Abs_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Abs* self) {
        return sig_dsp_Abs_destroy(allocator, self);
    }

    struct sig_dsp_Clamp* Clamp_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Clamp_new(allocator, context);
    }

    void Clamp_init(struct sig_dsp_Clamp* self,
        struct sig_SignalContext* context) {
        sig_dsp_Clamp_init(self, context);
    }

    void Clamp_generate(void* signal) {
        sig_dsp_Clamp_generate(signal);
    }

    void Clamp_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Clamp* self) {
        return sig_dsp_Clamp_destroy(allocator, self);
    }

    struct sig_dsp_ScaleOffset* ScaleOffset_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_ScaleOffset_new(allocator, context);
    }

    void ScaleOffset_init(struct sig_dsp_ScaleOffset* self,
        struct sig_SignalContext* context) {
        sig_dsp_ScaleOffset_init(self, context);
    }

    void ScaleOffset_generate(void* signal) {
        sig_dsp_ScaleOffset_generate(signal);
    }

    void ScaleOffset_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_ScaleOffset* self) {
        return sig_dsp_ScaleOffset_destroy(allocator, self);
    }


    struct sig_dsp_Sine* Sine_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Sine_new(allocator, context);
    }

    void Sine_init(struct sig_dsp_Sine* self,
        struct sig_SignalContext* context) {
        sig_dsp_Sine_init(self, context);
    }

    void Sine_generate(void* signal) {
        sig_dsp_Sine_generate(signal);
    }

    void Sine_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Sine* self) {
        return sig_dsp_Sine_destroy(allocator, self);
    }


    struct sig_dsp_BinaryOp* Add_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Add_new(allocator, context);
    }

    void Add_init(struct sig_dsp_BinaryOp* self,
        struct sig_SignalContext* context) {
        sig_dsp_Add_init(self, context);
    }

    void Add_generate(void* signal) {
        sig_dsp_Add_generate(signal);
    }

    void Add_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Add_destroy(allocator, self);
    }

    struct sig_dsp_BinaryOp* Sub_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Sub_new(allocator, context);
    }

    void Sub_init(struct sig_dsp_BinaryOp* self,
        struct sig_SignalContext* context) {
        sig_dsp_Sub_init(self, context);
    }

    void Sub_generate(void* signal) {
        sig_dsp_Sub_generate(signal);
    }

    void Sub_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Sub_destroy(allocator, self);
    }

    struct sig_dsp_BinaryOp* Mul_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Mul_new(allocator, context);
    }

    void Mul_init(struct sig_dsp_BinaryOp* self,
        struct sig_SignalContext* context) {
        sig_dsp_Mul_init(self, context);
    }

    void Mul_generate(void* signal) {
        sig_dsp_Mul_generate(signal);
    }

    void Mul_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Mul_destroy(allocator, self);
    }

    struct sig_dsp_BinaryOp* Div_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Div_new(allocator, context);
    }

    void Div_init(struct sig_dsp_BinaryOp* self,
        struct sig_SignalContext* context) {
        sig_dsp_Div_init(self, context);
    }

    void Div_generate(void* signal) {
        sig_dsp_Div_generate(signal);
    }

    void Div_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_BinaryOp* self) {
        return sig_dsp_Div_destroy(allocator, self);
    }

    struct sig_dsp_Invert* Invert_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Invert_new(allocator, context);
    }

    void Invert_init(struct sig_dsp_Invert* self,
        struct sig_SignalContext* context) {
        sig_dsp_Invert_init(self, context);
    }

    void Invert_generate(void* signal) {
        sig_dsp_Invert_generate(signal);
    }

    void Invert_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Invert* self) {
        return sig_dsp_Invert_destroy(allocator, self);
    }

    struct sig_dsp_Accumulate* Accumulate_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Accumulate_new(allocator, context);
    }

    void Accumulate_init(struct sig_dsp_Accumulate* self,
        struct sig_SignalContext* context) {
        sig_dsp_Accumulate_init(self, context);
    }

    void Accumulate_generate(void* signal) {
        sig_dsp_Accumulate_generate(signal);
    }

    void Accumulate_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Accumulate* self) {
        return sig_dsp_Accumulate_destroy(allocator, self);
    }

    struct sig_dsp_GatedTimer* GatedTimer_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_GatedTimer_new(allocator, context);
    }

    void GatedTimer_init(struct sig_dsp_GatedTimer* self,
        struct sig_SignalContext* context) {
        sig_dsp_GatedTimer_init(self, context);
    }

    void GatedTimer_generate(void* signal) {
        sig_dsp_GatedTimer_generate(signal);
    }

    void GatedTimer_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_GatedTimer* self) {
        return sig_dsp_GatedTimer_destroy(allocator, self);
    }

    struct sig_dsp_TimedTriggerCounter* TimedTriggerCounter_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_TimedTriggerCounter_new(allocator, context);
    }

    void TimedTriggerCounter_init(
        struct sig_dsp_TimedTriggerCounter* self,
        struct sig_SignalContext* context) {
        sig_dsp_TimedTriggerCounter_init(self, context);
    }

    void TimedTriggerCounter_generate(void* signal) {
        sig_dsp_TimedTriggerCounter_generate(signal);
    }

    void TimedTriggerCounter_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_TimedTriggerCounter* self) {
        return sig_dsp_TimedTriggerCounter_destroy(allocator, self);
    }

    struct sig_dsp_ToggleGate* ToggleGate_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_ToggleGate_new(allocator, context);
    }

    void ToggleGate_init(struct sig_dsp_ToggleGate* self,
        struct sig_SignalContext* context) {
        sig_dsp_ToggleGate_init(self, context);
    }

    void ToggleGate_generate(void* signal) {
        sig_dsp_ToggleGate_generate(signal);
    }

    void ToggleGate_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_ToggleGate* self) {
        return sig_dsp_ToggleGate_destroy(allocator, self);
    }

    void Oscillator_Outputs_newAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_Oscillator_Outputs* outputs) {
        sig_dsp_Oscillator_Outputs_newAudioBlocks(allocator, audioSettings,
            outputs);
    }

    void Oscillator_Outputs_destroyAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_dsp_Oscillator_Outputs* outputs) {
        sig_dsp_Oscillator_Outputs_destroyAudioBlocks(allocator, outputs);
    }

    void Oscillator_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Oscillator* self) {
        sig_dsp_Oscillator_destroy(allocator, self);
    }

    struct sig_dsp_Oscillator* SineOscillator_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_SineOscillator_new(allocator, context);
    }

    void SineOscillator_init(struct sig_dsp_Oscillator* self,
        struct sig_SignalContext* context) {
        sig_dsp_SineOscillator_init(self, context);
    }

    void SineOscillator_generate(void* signal) {
        sig_dsp_SineOscillator_generate(signal);
    }

    void SineOscillator_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Oscillator* self) {
        return sig_dsp_SineOscillator_destroy(allocator, self);
    }

    struct sig_dsp_Oscillator* LFTriangle_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_LFTriangle_new(allocator, context);
    }

    void LFTriangle_init(struct sig_dsp_Oscillator* self,
        struct sig_SignalContext* context) {
        sig_dsp_LFTriangle_init(self, context);
    }

    void LFTriangle_generate(void* signal) {
        sig_dsp_LFTriangle_generate(signal);
    }

    void LFTriangle_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Oscillator* self) {
        return sig_dsp_LFTriangle_destroy(allocator, self);
    }

    void WaveOscillator_init(struct sig_dsp_WavetableOscillator* self,
        struct sig_SignalContext* context) {
        sig_dsp_WavetableOscillator_init(self, context);
    }

    struct sig_dsp_WavetableOscillator* WaveOscillator_new(
        struct sig_Allocator* allocator, struct sig_SignalContext* context) {
        return sig_dsp_WavetableOscillator_new(allocator, context);
    }

    void WaveOscillator_generate(void* signal) {
        sig_dsp_WavetableOscillator_generate(signal);
    }

    void WaveOscillator_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_WavetableOscillator* self) {
        sig_dsp_WavetableOscillator_destroy(allocator, self);
    }

    void WavetableBankOscillator_init(
        struct sig_dsp_WavetableBankOscillator* self,
        struct sig_SignalContext* context) {
        sig_dsp_WavetableBankOscillator_init(self, context);
    }

    struct sig_dsp_WavetableBankOscillator* WavetableBankOscillator_new(
        struct sig_Allocator* allocator, struct sig_SignalContext* context) {
        return sig_dsp_WavetableBankOscillator_new(allocator, context);
    }

    void WavetableBankOscillator_generate(void* signal) {
        sig_dsp_WavetableBankOscillator_generate(signal);
    }

    void WavetableBankOscillator_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_WavetableBankOscillator* self) {
        sig_dsp_WavetableBankOscillator_destroy(allocator, self);
    }

    struct sig_dsp_Smooth* Smooth_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Smooth_new(allocator, context);
    }

    void Smooth_init(struct sig_dsp_Smooth* self,
        struct sig_SignalContext* context) {
        sig_dsp_Smooth_init(self, context);
    }

    void Smooth_generate(void* signal) {
        sig_dsp_Smooth_generate(signal);
    }

    void Smooth_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Smooth* self) {
        return sig_dsp_Smooth_destroy(allocator, self);
    }

    struct sig_dsp_EMA* EMA_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_EMA_new(allocator, context);
    }

    void EMA_init(struct sig_dsp_EMA* self,
        struct sig_SignalContext* context)  {
        sig_dsp_EMA_init(self, context);
    }

    void EMA_generate(void* signal) {
        sig_dsp_EMA_generate(signal);
    }

    void EMA_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_EMA* self) {
        return sig_dsp_EMA_destroy(allocator, self);
    }

    struct sig_dsp_OnePole* OnePole_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_OnePole_new(allocator, context);
    }

    void OnePole_init(struct sig_dsp_OnePole* self,
        struct sig_SignalContext* context) {
        sig_dsp_OnePole_init(self, context);
    }

    void OnePole_recalculateCoefficients(struct sig_dsp_OnePole* self,
        float frequency) {
        sig_dsp_OnePole_recalculateCoefficients(self, frequency);
    }

    void OnePole_generate(void* signal) {
        sig_dsp_OnePole_generate(signal);
    }

    void OnePole_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_OnePole* self) {
        return sig_dsp_OnePole_destroy(allocator, self);
    }

    struct sig_dsp_Tanh* Tanh_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Tanh_new(allocator, context);
    }

    void Tanh_init(struct sig_dsp_Tanh* self,
        struct sig_SignalContext* context) {
        sig_dsp_Tanh_init(self, context);
    }

    void Tanh_generate(void* signal) {
        sig_dsp_Tanh_generate(signal);
    }

    void Tanh_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Tanh* self) {
        return sig_dsp_Tanh_destroy(allocator, self);
    }

    struct sig_dsp_Looper* Looper_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Looper_new(allocator, context);
    }

    void Looper_init(struct sig_dsp_Looper* self,
        struct sig_SignalContext* context) {
        sig_dsp_Looper_init(self, context);
    }

    void Looper_setBuffer(struct sig_dsp_Looper* self,
        struct sig_Buffer* buffer) {
        sig_dsp_Looper_setBuffer(self, buffer);
    }

    void Looper_generate(void* signal) {
        sig_dsp_Looper_generate(signal);
    }

    void Looper_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Looper* self) {
        return sig_dsp_Looper_destroy(allocator, self);
    }

    struct sig_dsp_Dust* Dust_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Dust_new(allocator, context);
    }

    void Dust_init(struct sig_dsp_Dust* self,
        struct sig_SignalContext* context) {
        sig_dsp_Dust_init(self, context);
    }

    void Dust_generate(void* signal) {
        sig_dsp_Dust_generate(signal);
    }

    void Dust_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Dust* self) {
        return sig_dsp_Dust_destroy(allocator, self);
    }

    struct sig_dsp_TimedGate* TimedGate_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_TimedGate_new(allocator, context);
    }

    void TimedGate_init(struct sig_dsp_TimedGate* self,
        struct sig_SignalContext* context) {
        sig_dsp_TimedGate_init(self, context);
    }

    void TimedGate_generate(void* signal) {
        sig_dsp_TimedGate_generate(signal);
    }

    void TimedGate_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_TimedGate* self) {
        return sig_dsp_TimedGate_destroy(allocator, self);
    }

    struct sig_dsp_DustGate* DustGate_new(struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_DustGate_new(allocator, context);
    }

    void DustGate_init(struct sig_dsp_DustGate* self,
        struct sig_SignalContext* context) {
        sig_dsp_DustGate_init(self, context);
    }

    void DustGate_generate(void* signal) {
        sig_dsp_DustGate_generate(signal);
    }

    void DustGate_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_DustGate* self) {
        return sig_dsp_DustGate_destroy(allocator, self);
    }

    void ClockSource_init(struct sig_dsp_ClockSource* self,
        struct sig_SignalContext* context) {
        sig_dsp_ClockSource_init(self, context);
    }

    struct sig_dsp_ClockSource* ClockSource_new(
        struct sig_Allocator* allocator, struct sig_SignalContext* context) {
        return sig_dsp_ClockSource_new(allocator, context);
    }

    void ClockSource_generate(void* signal) {
        sig_dsp_ClockSource_generate(signal);
    }

    void ClockSource_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_ClockSource* self) {
        return sig_dsp_ClockSource_destroy(allocator, self);
    }

    void ClockDetector_Outputs_newAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_ClockDetector_Outputs* outputs) {
        sig_dsp_ClockDetector_Outputs_newAudioBlocks(allocator, audioSettings,
            outputs);
    }

    void ClockDetector_Outputs_destroyAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_dsp_ClockDetector_Outputs* outputs) {
        sig_dsp_ClockDetector_Outputs_destroyAudioBlocks(allocator, outputs);
    }

    struct sig_dsp_ClockDetector* ClockDetector_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_ClockDetector_new(allocator, context);
    }

    void ClockDetector_init(struct sig_dsp_ClockDetector* self,
        struct sig_SignalContext* context) {
        sig_dsp_ClockDetector_init(self, context);
    }

    void ClockDetector_generate(void* signal) {
        sig_dsp_ClockDetector_generate(signal);
    }

    void ClockDetector_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_ClockDetector* self) {
        return sig_dsp_ClockDetector_destroy(allocator, self);
    }

    struct sig_dsp_LinearToFreq* LinearToFreq_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_LinearToFreq_new(allocator, context);
    }

    void LinearToFreq_init(struct sig_dsp_LinearToFreq* self,
        struct sig_SignalContext* context) {
        sig_dsp_LinearToFreq_init(self, context);
    }

    void LinearToFreq_generate(void* signal) {
        sig_dsp_LinearToFreq_generate(signal);
    }

    void LinearToFreq_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_LinearToFreq* self) {
        return sig_dsp_LinearToFreq_destroy(allocator, self);
    }

    struct sig_dsp_Branch* Branch_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Branch_new(allocator, context);
    }

    void Branch_init(struct sig_dsp_Branch* self,
        struct sig_SignalContext* context) {
        sig_dsp_Branch_init(self, context);
    }

    void Branch_generate(void* signal) {
        sig_dsp_Branch_generate(signal);
    }

    void Branch_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Branch* self) {
        return sig_dsp_Branch_destroy(allocator, self);
    }

    void List_Outputs_newAudioBlocks(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_List_Outputs* outputs) {
        sig_dsp_List_Outputs_newAudioBlocks(allocator, audioSettings, outputs);
    }

    void List_Outputs_destroyAudioBlocks(struct sig_Allocator* allocator,
        struct sig_dsp_List_Outputs* outputs) {
        sig_dsp_List_Outputs_destroyAudioBlocks(allocator, outputs);
    }

    struct sig_dsp_List* List_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_List_new(allocator, context);
    }

    void List_init(struct sig_dsp_List* self,
        struct sig_SignalContext* context) {
        sig_dsp_List_init(self, context);
    }

    float List_constrain(bool shouldWrap, float index,
        float lastIndex, float listLength) {
        return sig_dsp_List_constrain(shouldWrap, index, lastIndex, listLength);
    }

    void List_generate(void* signal) {
        sig_dsp_List_generate(signal);
    }

    void List_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_List* self) {
        return sig_dsp_List_destroy(allocator, self);
    }

    struct sig_dsp_LinearMap* LinearMap_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_LinearMap_new(allocator, context);
    }

    void LinearMap_init(struct sig_dsp_LinearMap* self,
        struct sig_SignalContext* context) {
        sig_dsp_LinearMap_init(self, context);
    }

    void LinearMap_generate(void* signal) {
        sig_dsp_LinearMap_generate(signal);
    }

    void LinearMap_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_LinearMap* self) {
        return sig_dsp_LinearMap_destroy(allocator, self);
    }

    struct sig_dsp_TwoOpFM* TwoOpFM_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_TwoOpFM_new(allocator, context);
    }

    void TwoOpFM_init(struct sig_dsp_TwoOpFM* self,
        struct sig_SignalContext* context) {
        sig_dsp_TwoOpFM_init(self, context);
    }

    void TwoOpFM_generate(void* signal) {
        sig_dsp_TwoOpFM_generate(signal);
    }

    void TwoOpFM_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_TwoOpFM* self) {
        return sig_dsp_TwoOpFM_destroy(allocator, self);
    }

    void FourPoleFilter_Outputs_newAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_FourPoleFilter_Outputs* outputs) {
        sig_dsp_FourPoleFilter_Outputs_newAudioBlocks(allocator,
            audioSettings, outputs);
    }

    void FourPoleFilter_Outputs_destroyAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_dsp_FourPoleFilter_Outputs* outputs) {
        sig_dsp_FourPoleFilter_Outputs_destroyAudioBlocks(allocator, outputs);
    }

    struct sig_dsp_Bob* Bob_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Bob_new(allocator, context);
    }

    void Bob_init(struct sig_dsp_Bob* self,
        struct sig_SignalContext* context) {
        sig_dsp_Bob_init(self, context);
    }

    float Bob_clip(float value, float saturation,
        float saturationInv) {
        return sig_dsp_Bob_clip(value, saturation, saturationInv);
    }

    void Bob_generate(void* signal) {
        sig_dsp_Bob_generate(signal);
    }

    void Bob_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Bob* self) {
        return sig_dsp_Bob_destroy(allocator, self);
    }

    struct sig_dsp_Ladder* Ladder_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Ladder_new(allocator, context);
    }

    void Ladder_init(
        struct sig_dsp_Ladder* self,
        struct sig_SignalContext* context) {
        sig_dsp_Ladder_init(self, context);
    }

    void Ladder_calcCoefficients(
        struct sig_dsp_Ladder* self, float freq) {
        sig_dsp_Ladder_calcCoefficients(self, freq);
    }

    float Ladder_calcStage(
        struct sig_dsp_Ladder* self, float s, uint8_t i) {
        return sig_dsp_Ladder_calcStage(self, s, i);
    }

    void Ladder_generate(void* signal) {
        sig_dsp_Ladder_generate(signal);
    }

    void Ladder_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Ladder* self) {
        return sig_dsp_Ladder_destroy(allocator, self);
    }

    struct sig_dsp_TiltEQ* TiltEQ_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_TiltEQ_new(allocator, context);
    }

    void TiltEQ_init(struct sig_dsp_TiltEQ* self,
        struct sig_SignalContext* context) {
        sig_dsp_TiltEQ_init(self, context);
    }

    void TiltEQ_generate(void* signal) {
        sig_dsp_TiltEQ_generate(signal);
    }

    void TiltEQ_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_TiltEQ* self) {
        return sig_dsp_TiltEQ_destroy(allocator, self);
    }

    struct sig_dsp_Delay* Delay_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Delay_new(allocator, context);
    }

    void Delay_init(struct sig_dsp_Delay* self,
        struct sig_SignalContext* context) {
        sig_dsp_Delay_init(self, context);
    }

    void Delay_read(struct sig_dsp_Delay* self, float source,
        size_t i) {
        sig_dsp_Delay_read(self, source, i);
    }

    void Delay_generate(void* signal) {
        sig_dsp_Delay_generate(signal);
    }

    void Delay_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Delay* self) {
        return sig_dsp_Delay_destroy(allocator, self);
    }

    struct sig_dsp_Delay* DelayTap_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_DelayTap_new(allocator, context);
    }

    void DelayTap_init(struct sig_dsp_Delay* self,
        struct sig_SignalContext* context) {
        sig_dsp_DelayTap_init(self, context);
    }

    void DelayTap_generate(void* signal) {
        sig_dsp_DelayTap_generate(signal);
    }

    void DelayTap_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Delay* self) {
        return sig_dsp_DelayTap_destroy(allocator, self);
    }

    struct sig_dsp_DelayWrite* DelayWrite_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_DelayWrite_new(allocator, context);
    }

    void DelayWrite_init(struct sig_dsp_DelayWrite* self,
        struct sig_SignalContext* context) {
        sig_dsp_DelayWrite_init(self, context);
    }

    void DelayWrite_generate(void* signal) {
        sig_dsp_DelayWrite_generate(signal);
    }

    void DelayWrite_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_DelayWrite* self) {
        return sig_dsp_DelayWrite_destroy(allocator, self);
    }

    struct sig_dsp_Comb* Comb_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Comb_new(allocator, context);
    }

    void Comb_init(struct sig_dsp_Comb* self,
        struct sig_SignalContext* context) {
        sig_dsp_Comb_init(self, context);
    }

    void Comb_generate(void* signal) {
        sig_dsp_Comb_generate(signal);
    }

    void Comb_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Comb* self) {
        return sig_dsp_Comb_destroy(allocator, self);
    }

    struct sig_dsp_Allpass* Allpass_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Allpass_new(allocator, context);
    }

    void Allpass_init(struct sig_dsp_Allpass* self,
        struct sig_SignalContext* context) {
        sig_dsp_Allpass_init(self, context);
    }

    void Allpass_generate(void* signal) {
        sig_dsp_Allpass_generate(signal);
    }

    void Allpass_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Allpass* self) {
        return sig_dsp_Allpass_destroy(allocator, self);
    }

    struct sig_dsp_Chorus* Chorus_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Chorus_new(allocator, context);
    }

    void Chorus_init(struct sig_dsp_Chorus* self,
        struct sig_SignalContext* context) {
        sig_dsp_Chorus_init(self, context);
    }

    void Chorus_generate(void* signal) {
        sig_dsp_Chorus_generate(signal);
    }

    void Chorus_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Chorus* self) {
        return sig_dsp_Chorus_destroy(allocator, self);
    }

    struct sig_dsp_LinearXFade* LinearXFade_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_LinearXFade_new(allocator, context);
    }

    void LinearXFade_init(struct sig_dsp_LinearXFade* self,
        struct sig_SignalContext* context) {
        sig_dsp_LinearXFade_init(self, context);
    }

    void LinearXFade_generate(void* signal) {
        sig_dsp_LinearXFade_generate(signal);
    }

    void LinearXFade_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_LinearXFade* self) {
        return sig_dsp_LinearXFade_destroy(allocator, self);
    }

    void Calibrator_Node_init(struct sig_dsp_Calibrator_Node* nodes,
        float_array_ptr targetValues, size_t numNodes) {
        sig_dsp_Calibrator_Node_init(nodes, targetValues, numNodes);
    }

    size_t Calibrator_locateIntervalForValue(float x,
        struct sig_dsp_Calibrator_Node* nodes, size_t numNodes) {
        return sig_dsp_Calibrator_locateIntervalForValue(x, nodes, numNodes);
    }

    float Calibrator_fitValueToCalibrationData(float x,
        struct sig_dsp_Calibrator_Node* nodes, size_t numNodes) {
        return sig_dsp_Calibrator_fitValueToCalibrationData(x, nodes, numNodes);
    }

    struct sig_dsp_Calibrator* Calibrator_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_Calibrator_new(allocator, context);
    }

    void Calibrator_init(struct sig_dsp_Calibrator* self,
        struct sig_SignalContext* context) {
        sig_dsp_Calibrator_init(self, context);
    }

    void Calibrator_generate(void* signal) {
        sig_dsp_Calibrator_generate(signal);
    }

    void Calibrator_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_Calibrator* self) {
        return sig_dsp_Calibrator_destroy(allocator, self);
    }

    struct sig_dsp_SineWavefolder* SineWavefolder_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_SineWavefolder_new(allocator, context);
    }

    void SineWavefolder_init(struct sig_dsp_SineWavefolder* self,
        struct sig_SignalContext* context) {
        sig_dsp_SineWavefolder_init(self, context);
    }

    void SineWavefolder_generate(void* signal) {
        sig_dsp_SineWavefolder_generate(signal);
    }

    void SineWavefolder_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_SineWavefolder* self) {
        return sig_dsp_SineWavefolder_destroy(allocator, self);
    }


    struct sig_dsp_NoiseGate* NoiseGate_new(
        struct sig_Allocator* allocator,
        struct sig_SignalContext* context) {
        return sig_dsp_NoiseGate_new(allocator, context);
    }

    void NoiseGate_init(struct sig_dsp_NoiseGate* self,
        struct sig_SignalContext* context) {
        sig_dsp_NoiseGate_init(self, context);
    }

    void NoiseGate_generate(void* signal) {
        sig_dsp_NoiseGate_generate(signal);
    }

    void NoiseGate_destroy(struct sig_Allocator* allocator,
        struct sig_dsp_NoiseGate* self) {
        return sig_dsp_NoiseGate_destroy(allocator, self);
    }
};

class Signaletic {
public:
    const float PI = sig_PI;
    const float TWOPI = sig_PI;
    const float RECIP_TWOPI = sig_RECIP_TWOPI;
    const float LOG0_001 = sig_LOG0_001;
    const float LOG2 = sig_LOG2;
    const float FREQ_C4 = sig_FREQ_C4;
    struct sig_AudioSettings DEFAULT_AUDIOSETTINGS =
        sig_DEFAULT_AUDIOSETTINGS;

    Signals dsp;

    Signaletic() {}

    struct sig_Status* Status_new(struct sig_Allocator* allocator) {
        struct sig_Status* self = (struct sig_Status*) allocator->impl->malloc(
            allocator, sizeof(struct sig_Status));
        sig_Status_init(self);

        return self;
    }

    void Status_init(struct sig_Status* status) {
        return sig_Status_init(status);
    }

    void Status_reset(struct sig_Status* status) {
        return sig_Status_reset(status);
    }

    void Status_reportResult(struct sig_Status* status,
        enum sig_Result result) {
        return sig_Status_reportResult(status, result);
    }

    float fminf(float a, float b) {
        return sig_fminf(a, b);
    }

    float fmaxf(float a, float b) {
        return sig_fmaxf(a, b);
    }

    float clamp(float value, float min, float max) {
        return sig_clamp(value, min, max);
    }

    float flooredfmodf(float num, float denom) {
        return sig_flooredfmodf(num, denom);
    }

    float randf() {
        return sig_randf();
    }

    float fastTanhf(float x) {
        return sig_fastTanhf(x);
    }

    float linearMap(float value,
        float fromMin, float fromMax, float toMin, float toMax) {
        return sig_linearMap(value, fromMin, fromMax, toMin, toMax);
    }

    uint16_t unipolarToUint12(float sample) {
        return sig_unipolarToUint12(sample);
    }

    uint16_t bipolarToUint12(float sample) {
        return sig_bipolarToUint12(sample);
    }

    uint16_t bipolarToInvUint12(float sample) {
        return sig_bipolarToInvUint12(sample);
    }

    float uint16ToBipolar(uint16_t sample) {
        return sig_uint16ToBipolar(sample);
    }

    float uint16ToUnipolar(uint16_t sample) {
        return sig_uint16ToUnipolar(sample);
    }

    float invUint16ToBipolar(uint16_t sample) {
        return sig_invUint16ToBipolar(sample);
    }

    float midiToFreq(float midiNum) {
        return sig_midiToFreq(midiNum);
    }

    float freqToMidi(float frequency) {
        return sig_freqToMidi(frequency);
    }

    float linearToFreq(float value, float middleFreq) {
        return sig_linearToFreq(value, middleFreq);
    }

    float freqToLinear(float freq, float middleFreq) {
        return sig_freqToLinear(freq, middleFreq);
    }

    float sum(float_array_ptr values, size_t length) {
        return sig_sum(values, length);
    }

    size_t indexOfMin(float_array_ptr values, size_t length) {
        return sig_indexOfMin(values, length);
    }

    size_t indexOfMax(float_array_ptr values, size_t length) {
        return sig_indexOfMax(values, length);
    }

    float randomFill(size_t i, float_array_ptr array) {
        return sig_randomFill(i, array);
    }

    void fillWithValue(float_array_ptr array, size_t length,
        float value) {
        return sig_fillWithValue(array, length, value);
    }

    void fillWithSilence(float_array_ptr array, size_t length) {
        return sig_fillWithSilence(array, length);
    }

    float interpolate_linear(float idx, float_array_ptr table,
        size_t length) {
        return sig_interpolate_linear(idx, table, length);
    }

    float interpolate_cubic(float idx, float_array_ptr table,
        size_t length) {
        return sig_interpolate_cubic(idx, table, length);
    }

    float filter_mean(float_array_ptr values, size_t length) {
        return sig_filter_mean(values, length);
    }

    float filter_meanExcludeMinMax(float_array_ptr values, size_t length) {
        return sig_filter_meanExcludeMinMax(values, length);
    }

    float filter_ema(float current, float previous, float a) {
        return sig_filter_ema(current, previous, a);
    }

    float filter_onepole(float current, float previous, float b0,
        float a1) {
        return sig_filter_onepole(current, previous, b0, a1);
    }

    float filter_onepole_HPF_calculateA1(float frequency, float sampleRate) {
        return sig_filter_onepole_HPF_calculateA1(frequency, sampleRate);
    }


    float filter_onepole_HPF_calculateB0(float a1) {
        return sig_filter_onepole_HPF_calculateB0(a1);
    }

    float filter_onepole_LPF_calculateA1(float frequency, float sampleRate) {
        return sig_filter_onepole_LPF_calculateA1(frequency, sampleRate);
    }

    float filter_onepole_LPF_calculateB0(float a1) {
        return sig_filter_onepole_LPF_calculateB0(a1);
    }

    float filter_smooth(float current, float previous, float coeff) {
        return sig_filter_smooth(current, previous, coeff);
    }

    float filter_smooth_calculateCoefficient(float timeSecs,
        float sampleRate) {
        return sig_filter_smooth_calculateCoefficient(timeSecs, sampleRate);
    }

    void filter_Smooth_init(struct sig_filter_Smooth* self, float coeff) {
        sig_filter_Smooth_init(self, coeff);
    }

    float filter_Smooth_generate(struct sig_filter_Smooth* self, float value) {
        return sig_filter_Smooth_generate(self, value);
    }

    float waveform_sine(float phase) {
        return sig_waveform_sine(phase);
    }

    float waveform_square(float phase) {
        return sig_waveform_square(phase);
    }

    float waveform_saw(float phase) {
        return sig_waveform_saw(phase);
    }

    float waveform_reverseSaw(float phase) {
        return sig_waveform_reverseSaw(phase);
    }

    float waveform_triangle(float phase) {
        return sig_waveform_triangle(phase);
    }


    void osc_Oscillator_init(struct sig_osc_Oscillator* self) {
        sig_osc_Oscillator_init(self);
    }

    float osc_Oscillator_eoc(float phase) {
        return sig_osc_Oscillator_eoc(phase);
    }

    float osc_Oscillator_wrapPhase(float phase) {
        return sig_osc_Oscillator_wrapPhase(phase);
    }

    void osc_Oscillator_accumulatePhase(void* phaseAccumulator,
        float frequency, float sampleRate) {
        sig_osc_Oscillator_accumulatePhase((float *) phaseAccumulator,
            frequency, sampleRate);
    }

    void osc_Wavetable_init(struct sig_osc_Wavetable* self,
        struct sig_Buffer* wavetable) {
        sig_osc_Wavetable_init(self, wavetable);
    }

    float osc_Wavetable_generate(struct sig_osc_Wavetable* self,
        float frequency, float phaseOffset, float sampleRate, void* eocOut) {
        return sig_osc_Wavetable_generate(self, frequency, phaseOffset,
            sampleRate, (float*) eocOut);
    }


    void osc_FastLFSine_init(struct sig_osc_FastLFSine* self,
        float sampleRate) {
        sig_osc_FastLFSine_init(self, sampleRate);
    }

    void osc_FastLFSine_setFrequency(struct sig_osc_FastLFSine* self,
        float frequency) {
        sig_osc_FastLFSine_setFrequency(self, frequency);
    }

    void osc_FastLFSine_setFrequencyFast(struct sig_osc_FastLFSine* self,
        float frequency) {
        sig_osc_FastLFSine_setFrequencyFast(self, frequency);
    }

    void osc_FastLFSine_generate(struct sig_osc_FastLFSine* self) {
        sig_osc_FastLFSine_generate(self);
    }

    size_t secondsToSamples(struct sig_AudioSettings* audioSettings,
        float duration) {
        return sig_secondsToSamples(audioSettings, duration);
    }

    /**
     * Creates a new TLSFAllocator and heap of the specified size,
     * both of which are allocated using the platform's malloc().
     *
     * @param size the size in bytes of the Allocator's heap
     * @return a pointer to the new Allocator
     */
    struct sig_Allocator* TLSFAllocator_new(size_t size) {
        void* memory = malloc(size);
        struct sig_AllocatorHeap* heap = (struct sig_AllocatorHeap*)
            malloc(sizeof(struct sig_AllocatorHeap));
        heap->length = size;
        heap->memory = memory;

        struct sig_Allocator* allocator = (struct sig_Allocator*)
            malloc(sizeof(struct sig_Allocator));
        allocator->impl = &sig_TLSFAllocatorImpl;
        allocator->heap = heap;

        allocator->impl->init(allocator);

        return allocator;
    }

    /**
     * Destroys an TLSFAllocator and its underlying heap
     * using the platform's free().
     *
     * @param allocator the Allocator instance to destroy
     */
    void TLSFAllocator_destroy(struct sig_Allocator* allocator) {
        free(allocator->heap->memory);
        free(allocator->heap);
        // Note that we don't delete the impl because
        // we didn't create it (it's a global singleton).
        free(allocator);
    }

    struct sig_List* List_new(struct sig_Allocator* allocator,
        size_t capacity) {
        return sig_List_new(allocator, capacity);
    }

    void List_insert(struct sig_List* self, size_t index, void* item,
        struct sig_Status* status) {
        return sig_List_insert(self, index, item, status);
    }

    void List_append(struct sig_List* self, void* item,
        struct sig_Status* status) {
        return sig_List_append(self, item, status);
    }

    void* List_pop(struct sig_List* self, struct sig_Status* status) {
        return sig_List_pop(self, status);
    }

    void* List_remove(struct sig_List* self, size_t index,
        struct sig_Status* status) {
        return sig_List_remove(self, index, status);
    }

    void List_destroy(struct sig_Allocator* allocator,
        struct sig_List* self) {
        return sig_List_destroy(allocator, self);
    }


    struct sig_AudioSettings* AudioSettings_new(
        struct sig_Allocator* allocator) {
        return sig_AudioSettings_new(allocator);
    }

    void AudioSettings_destroy(struct sig_Allocator* allocator,
        struct sig_AudioSettings* self) {
        return sig_AudioSettings_destroy(allocator, self);
    }


    struct sig_SignalContext* SignalContext_new(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings) {
        return sig_SignalContext_new(allocator, audioSettings);
    }

    void SignalContext_destroy(struct sig_Allocator* allocator,
        struct sig_SignalContext* self) {
        sig_SignalContext_destroy(allocator, self);
    }


    struct sig_Buffer* Buffer_new(struct sig_Allocator* allocator,
        size_t length) {
        return sig_Buffer_new(allocator, length);
    }

    void Buffer_fillWithValue(struct sig_Buffer* buffer, float value) {
        return sig_Buffer_fillWithValue(buffer, value);
    }

    void Buffer_fillWithSilence(struct sig_Buffer* buffer) {
        return sig_Buffer_fillWithSilence(buffer);
    }

    float Buffer_read(struct sig_Buffer* buffer, float idx) {
        return sig_Buffer_read(buffer, idx);
    }

    float Buffer_readLinear(struct sig_Buffer* buffer, float idx) {
        return sig_Buffer_readLinear(buffer, idx);
    }

    float Buffer_readCubic(struct sig_Buffer* buffer, float idx) {
        return sig_Buffer_readCubic(buffer, idx);
    }

    void Buffer_destroy(struct sig_Allocator* allocator,
        struct sig_Buffer* buffer) {
        return sig_Buffer_destroy(allocator, buffer);
    }

    struct sig_Buffer* BufferView_new(
        struct sig_Allocator* allocator, struct sig_Buffer* buffer,
        size_t startIdx, size_t length) {
        return sig_BufferView_new(allocator, buffer, startIdx, length);
    }

    void BufferView_destroy(struct sig_Allocator* allocator,
        struct sig_Buffer* self) {
        sig_BufferView_destroy(allocator, self);
    }

    float_array_ptr AudioBlock_new(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings) {
        return sig_AudioBlock_new(allocator, audioSettings);
    }

    sig_WavetableBank* WavetableBank_new(struct sig_Allocator* allocator,
        unsigned long numTables, unsigned long tableLength) {
        return sig_WavetableBank_new(allocator, numTables, tableLength);
    }

    float WavetableBank_readLinearAtPhase(struct sig_WavetableBank* wavetable,
        float tableIdx, float phase) {
        return sig_WavetableBank_readLinearAtPhase(wavetable, tableIdx, phase);
    }

    void WavetableBank_destroy(struct sig_Allocator* allocator,
        struct sig_WavetableBank* wavetable) {
        return sig_WavetableBank_destroy(allocator, wavetable);
    }

    float_array_ptr AudioBlock_newWithValue(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        float value) {
            return sig_AudioBlock_newWithValue(allocator,
                audioSettings, value);
    }

    float_array_ptr AudioBlock_newSilent(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings) {
        return sig_AudioBlock_newSilent(allocator, audioSettings);
    }

    void AudioBlock_destroy(struct sig_Allocator* allocator,
        float_array_ptr self) {
        return sig_AudioBlock_destroy(allocator, self);
    }

    struct sig_DelayLine* DelayLine_new(struct sig_Allocator* allocator,
        size_t maxDelayLength) {
        return sig_DelayLine_new(allocator, maxDelayLength);
    }

    struct sig_DelayLine* DelayLine_newSeconds(struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings, float maxDelaySecs) {
        return sig_DelayLine_newSeconds(allocator, audioSettings, maxDelaySecs);
    }

    struct sig_DelayLine* DelayLine_newWithTransferredBuffer(
        struct sig_Allocator* allocator, struct sig_Buffer* buffer) {
        return sig_DelayLine_newWithTransferredBuffer(allocator, buffer);
    }

    void DelayLine_init(struct sig_DelayLine* self) {
        sig_DelayLine_init(self);
    }

    float DelayLine_readAt(struct sig_DelayLine* self, size_t readPos) {
        return sig_DelayLine_readAt(self, readPos);
    }

    float DelayLine_linearReadAt(struct sig_DelayLine* self, float readPos) {
        return sig_DelayLine_linearReadAt(self, readPos);
    }

    float DelayLine_cubicReadAt(struct sig_DelayLine* self, float readPos) {
        return sig_DelayLine_cubicReadAt(self, readPos);
    }

    float DelayLine_allpassReadAt(struct sig_DelayLine* self,
        float readPos, float previousSample) {
        return sig_DelayLine_allpassReadAt(self, readPos, previousSample);
    }

    float DelayLine_readAtTime(struct sig_DelayLine* self, float source,
        float tapTime, float sampleRate) {
        return sig_DelayLine_readAtTime(self, source, tapTime, sampleRate);
    }

    float DelayLine_linearReadAtTime(struct sig_DelayLine* self, float source,
        float tapTime, float sampleRate) {
        return sig_DelayLine_linearReadAtTime(self, source, tapTime,
            sampleRate);
    }

    float DelayLine_cubicReadAtTime(struct sig_DelayLine* self, float source,
        float tapTime, float sampleRate) {
        return sig_DelayLine_cubicReadAtTime(self, source, tapTime, sampleRate);
    }

    float DelayLine_allpassReadAtTime(struct sig_DelayLine* self,
        float source, float tapTime, float sampleRate, float previousSample) {
        return sig_DelayLine_allpassReadAtTime(self, source, tapTime,
            sampleRate, previousSample);
    }

    float DelayLine_readAtTimes(struct sig_DelayLine* self, float source,
        float_array_ptr tapTimes, float_array_ptr tapGains, size_t numTaps,
        float sampleRate, float timeScale) {
        return sig_DelayLine_readAtTimes(self, source, tapTimes, tapGains,
            numTaps, sampleRate, timeScale);
    }

    float DelayLine_linearReadAtTimes(struct sig_DelayLine* self,
        float source, float_array_ptr tapTimes, float_array_ptr tapGains,
        size_t numTaps, float sampleRate, float timeScale) {
        return sig_DelayLine_linearReadAtTimes(self, source, tapTimes,
            tapGains, numTaps, sampleRate, timeScale);
    }

    float DelayLine_cubicReadAtTimes(struct sig_DelayLine* self,
        float source, float_array_ptr tapTimes, float_array_ptr tapGains,
        size_t numTaps, float sampleRate, float timeScale) {
        return sig_DelayLine_cubicReadAtTimes(self, source, tapTimes, tapGains,
            numTaps, sampleRate, timeScale);
    }

    void DelayLine_write(struct sig_DelayLine* self, float sample) {
        sig_DelayLine_write(self, sample);
    }

    float DelayLine_calcFeedbackGain(float delayTime, float decayTime) {
        return sig_DelayLine_calcFeedbackGain(delayTime, decayTime);
    }

    float DelayLine_feedback(float sample, float read, float g) {
        return sig_DelayLine_feedback(sample, read, g);
    }

    float DelayLine_comb(struct sig_DelayLine* self, float sample,
        size_t readPos, float g) {
        return sig_DelayLine_comb(self, sample, readPos, g);
    }

    float DelayLine_cubicComb(struct sig_DelayLine* self, float sample,
        float readPos, float g) {
        return sig_DelayLine_cubicComb(self, sample, readPos, g);
    }

    float DelayLine_allpass(struct sig_DelayLine* self, float sample,
        size_t readPos, float g) {
        return sig_DelayLine_allpass(self, sample, readPos, g);
    }

    float DelayLine_linearAllpass(struct sig_DelayLine* self,
        float sample, float readPos, float g) {
        return sig_DelayLine_linearAllpass(self, sample, readPos, g);
    }

    float DelayLine_cubicAllpass(struct sig_DelayLine* self, float sample,
        float readPos, float g) {
        return sig_DelayLine_cubicAllpass(self, sample, readPos, g);
    }

    void DelayLine_destroy(struct sig_Allocator* allocator,
        struct sig_DelayLine* self) {
        sig_DelayLine_destroy(allocator, self);
    }

    float linearXFade(float left, float right, float mix) {
        return sig_linearXFade(left, right, mix);
    }

    float sineWavefolder(float x, float gain, float factor) {
        return sig_sineWavefolder(x, gain, factor);
    }

    void SingleMonoOutput_newAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_AudioSettings* audioSettings,
        struct sig_dsp_Signal_SingleMonoOutput* outputs) {
        return sig_dsp_Signal_SingleMonoOutput_newAudioBlocks(allocator,
            audioSettings, outputs);
    }

    void SingleMonoOutput_destroyAudioBlocks(
        struct sig_Allocator* allocator,
        struct sig_dsp_Signal_SingleMonoOutput* outputs) {
        return sig_dsp_Signal_SingleMonoOutput_destroyAudioBlocks(allocator,
            outputs);
    }

    ~Signaletic() {}
};
