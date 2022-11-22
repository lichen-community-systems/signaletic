let sig = new Module.Signaletic();

sig.TYPED_VIEWS = {
    "int8": "Int8Array",
    "uint8": "Uint8Array",
    "int16": "Int16Array",
    "uint16": "Uint16Array",
    "int32": "Int16Array",
    "uint32": "Uint16Array",
    "float32": "Float32Array"
};

sig.dereferenceArray = function(ptr, length, type) {
    let arrayViewType = sig.TYPED_VIEWS[type];
    if (arrayViewType === undefined) {
        throw Error("Can't dereference an array of type " + type);
    }

    return new globalThis[arrayViewType](Module.HEAP8.buffer,
        ptr, length);
};

class SignaleticOscillator extends AudioWorkletProcessor {
    constructor() {
        super();

        this.allocator = sig.TLSFAllocator_new(1024 * 256);
        this.signalList = sig.List_new(this.allocator, 128);
        this.audioSettings = sig.AudioSettings_new(this.allocator);
        this.audioSettings.sampleRate = sampleRate;
        this.audioSettings.blockSize = 128;
        this.audioSettings.numChannels = 2;
        this.signalContext = sig.SignalContext_new(this.allocator,
            this.audioSettings);

        /** Modulators **/
        this.freqMod = sig.dsp.Value_new(this.allocator, this.signalContext);
        this.freqMod.parameters.value = 440.0;
        this.ampMod = sig.dsp.Value_new(this.allocator, this.signalContext);
        this.ampMod.parameters.value = 1.0;

        /** Carrier **/
        this.carrier = sig.dsp.Sine_new(this.allocator, this.signalContext);
        this.carrier.inputs.freq = this.freqMod.outputs.main;
        this.carrier.inputs.mul = this.ampMod.outputs.main;

        /** Gain **/
        this.gainValue = sig.dsp.Value_new(this.allocator,
            this.signalContext);
        this.gainValue.parameters.value = 0.85;

        this.gain = sig.dsp.Mul_new(this.allocator, this.signalContext);
        this.gain.inputs.left = this.carrier.outputs.main;
        this.gain.inputs.right = this.gainValue.outputs.main;

        this.gainOutput = sig.dereferenceArray(
            this.gain.outputs.main,
            this.audioSettings.blockSize,
            "float32");

        // TODO: Error handling.
        let status = sig.Status_new(this.allocator);
        sig.List_append(this.signalList, this.ampMod, status);
        sig.List_append(this.signalList, this.freqMod, status);
        sig.List_append(this.signalList, this.carrier, status);
        sig.List_append(this.signalList, this.gainValue, status);
        sig.List_append(this.signalList, this.gain, status);
    }

    static get parameterDescriptors() {
        return [
            {
                name: 'blueKnobParam',
                defaultValue: 0.0,
                minValue: 0.0,
                maxValue: 1.0,
                automation: 'k-rate'
            },
            {
                name: 'redKnobParam',
                minValue: 0,
                maxValue: 1.0,
                defaultValue: 0,
                automation: 'k-rate'
            }
        ]
    }

    process (inputs, outputs, parameters) {
        // TODO: Signaletic needs value mapping functions
        // like libDaisy, or control values should always
        // mapped using Signals.
        this.gainValue.parameters.value = parameters.blueKnobParam[0];
        this.freqMod.parameters.value =
            parameters.redKnobParam[0] * 1700 + 60;

        for (let output of outputs) {
            // Evaluate the Signaletic graph.
            sig.dsp.generateSignals(this.signalList);

            for (let channel of output) {
                for (let i = 0; i < channel.length; i++) {
                    // channel[i] = inputs[0][0][i];
                    channel[i] = this.gainOutput[i];
                }
            }
        }

        return true;
    }
}

registerProcessor('SignaleticOscillator', SignaleticOscillator);
