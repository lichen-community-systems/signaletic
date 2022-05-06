let star = new Module.Starlings();

star.TYPED_VIEWS = {
    "int8": "Int8Array",
    "uint8": "Uint8Array",
    "int16": "Int16Array",
    "uint16": "Uint16Array",
    "int32": "Int16Array",
    "uint32": "Uint16Array",
    "float32": "Float32Array"
};

star.dereferenceArray = function(ptr, length, type) {
    let arrayViewType = star.TYPED_VIEWS[type];
    if (arrayViewType === undefined) {
        throw Error("Can't dereference an array of type " + type);
    }

    return new globalThis[arrayViewType](Module.HEAP8.buffer,
        ptr, length);
};

class SignaleticOscillator extends AudioWorkletProcessor {
    constructor() {
        super();

        this.allocator = star.Allocator_new(1024 * 256);
        this.audioSettings = star.AudioSettings_new(this.allocator);
        this.audioSettings.sampleRate = sampleRate;
        this.audioSettings.blockSize = 128;
        this.audioSettings.numChannels = 2;

        /** Modulators **/
        this.freqMod = star.sig.Value_new(this.allocator,
            this.audioSettings);
        this.freqMod.parameters.value = 440.0;
        this.ampMod = star.sig.Value_new(this.allocator,
            this.audioSettings);
        this.ampMod.parameters.value = 1.0;


        /** Carrier **/
        this.carrierInputs = star.sig.Sine_Inputs_new(
            this.allocator,
            this.freqMod.signal.output,
            star.AudioBlock_newWithValue(this.allocator,
                this.audioSettings, 0.0),
            this.ampMod.signal.output,
            star.AudioBlock_newWithValue(this.allocator,
                this.audioSettings, 0.0)
        );

        this.carrier = star.sig.Sine_new(this.allocator,
            this.audioSettings, this.carrierInputs);

        /** Gain **/
        this.gainValue = star.sig.Value_new(this.allocator,
            this.audioSettings);
        this.gainValue.parameters.value = 0.85;

        this.gainInputs = star.sig.BinaryOp_Inputs_new(
            this.allocator,
            this.carrier.signal.output,
            this.gainValue.signal.output
        );

        this.gain = star.sig.Mul_new(this.allocator,
            this.audioSettings, this.gainInputs);

        this.gainOutput = star.dereferenceArray(
            this.gain.signal.output,
            this.audioSettings.blockSize,
            "float32");
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
                defaultValue: 0.0,
                minValue: 0.0,
                maxValue: 1.0,
                automation: 'k-rate'
            }
        ]
    }

    process (inputs, outputs, parameters) {
        // TODO: Signaletic needs value mapping functions
        // like libDaisy, or control values should always
        // mapped using Signals.
        this.gainValue.parameters.value =
            parameters.blueKnobParam[0];

        // Inputs may not be connected,
        // so we have to check before we read them.
        // TODO: Read inputs at audio rate.
        // TODO: Treat the two modulation sources (freq and mul)
        // as separate inputs, rather than as a single
        // multichannel input.
        let cvInputs = inputs[0];
        if (cvInputs.length > 0) {
            this.ampMod.parameters.value = cvInputs[0][0];
        }

        if (cvInputs.length > 1) {
            // Map to MIDI notes between 0..120
            let freqNote = cvInputs[1][0] * 60.0 + 60.0;
            this.freqMod.parameters.value = star.midiToFreq(
                freqNote);
        }

        for (let output of outputs) {
            // Evaluate the Signaletic graph.
            this.ampMod.signal.generate(this.ampMod);
            this.freqMod.signal.generate(this.freqMod);
            this.carrier.signal.generate(this.carrier);
            this.gainValue.signal.generate(this.gainValue);
            this.gain.signal.generate(this.gain);

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
