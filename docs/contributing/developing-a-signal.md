# Developing a Signaletic Signal

## Example Boilerplate for a Signal

Signals currently require quite a bit of boilerplate code to implement. This will change with time--first with a set of macros and eventually via a C code generator from a custom language. In the meantime, here's a guide to what's needed to implement your own signal.

### Signal Definition

This section, located in a header file, defines the structure of your signal. It must always contain
(or "inherit from" if you're OO-minded) a ```sig_dsp_Signal``` as its first member. It can optionally have a set of inputs, parameters, and any kind of free-form instance state needed.

It's likely in the future that instance state will be moved into more formal container struct, and other containers will be defined for things like buffers, in order to provide greater lifecycle support and a normalized constructor/initializer signature for all signals.

```c
struct sig_dsp_${SignalName} {
    struct sig_dsp_Signal signal;

    struct sig_dsp_${SignalName}_Inputs inputs;

    struct sig_dsp_${SignalName}_Parameters parameters;

    struct sig_dsp_Signal_{OutputType} outputs;

    float ${stateName};
};
```

### Signal Outputs

In Signaletic, signals can have multiple outputs. For example, oscillators have both a "main" output that contains the oscillator's waveform, as well as an "eoc" output that fires a trigger whenever the oscillator reaches the end of its cycle.

By default, Signaletic defines a series of core output types, which encompass common output topologies. Currently these include:
 * ```sig_dsp_Signal_SingleMonoOutput```, which provides one mono output called ```main```
 * ```sig_dsp_Oscillator_Outputs```, containing a mono ```main``` and a mono ```eoc`` output

Additional output types for other cases, including multichannel outputs, will be added. Custom outputs are defined as follows:

```c
struct sig_dsp_${SignalName}_Output {
    float_array_ptr ${outputName};
    float_array_ptr ${otherOutputName};
};
```

### Signal Inputs Definition

If your signal will read inputs from other signals, you must define them in an Inputs object. For the moment, all inputs are defined as pointers to the output arrays of other signals (using the ```float_array_ptr``` type).

In the future, inputs will become more complex, since it will be possible to define groups of signals that run at different block sizes yet interconnect them, which will require some additional metadata about how to read an input. Connection objects will also be created to provide a first class representation for a connection between two or more Signals.

```c
struct sig_dsp_${SignalName}_Inputs {
    float_array_ptr ${inputName};
};
```

### Signal Parameters Definition

Parameters are user-mutatable values that are not expected to change at audio rates. They may in the future be implemented via an Observer or event system.

```c
struct sig_dsp_${SignalName}_Parameters {
    float ${parameterName};
};
```

### Constructor

A signal's constructor is responsible for allocating memory for the signal's struct as well as any owned objects such as its output. The constructor should only allocate memory; it should defer to an initializer function to set up any state or assign default values. This separation between construction (i.e. dynamic memory allocation) and initialization allows Signaletic to be used in an entirely static memory environment, as is common on embedded devices. Signaletic, however, provides a custom memory allocator that is suited to real-time systems.

```c
struct sig_dsp_${SignalName}* sig_dsp_${SignalName}_new(
    struct sig_Allocator* allocator,
    struct sig_SignalContext* context) {
    // Allocate memory for the Signal.
    struct sig_dsp_${SignalName}* self = sig_MALLOC(allocator,
        struct sig_dsp_${SignalName});

    // Initialize the Signal.
    sig_dsp_${SignalName}_init(self, context);

    // Allocate output blocks.
    self->outputs.main = sig_AudioBlock_new(allocator,
        context->audioSettings);

    return self;
}
```

### Initializer

The initializer is responsible for assigning values to a signal's struct, setting any defaults for parameters and instance state as needed. It should never allocate any dynamic memory, as this should be done in the signal's ```_new()``` function. The initializer is also responsible for creating and setting default values for a signal's parameters.

```c
void sig_dsp_${SignalName}_init(struct sig_dsp_${SignalName}* self,
    struct sig_SignalContext* context) {
    // Call the base Signal initializer.
    sig_dsp_Signal_init(self, context, *sig_dsp_${SignalName}_generate);

    // Initialize default parameter values.
    struct sig_dsp_${SignalName}_Parameters params = {
        .${parameterName} = ${parameterDefaultValue}
    };
    self->parameters = params;

    // Initialize the default values for local state.
    self->${stateName} = ${stateDefaultValue}

    // Connect all inputs to silence.
    sig_CONNECT_TO_SILENCE(self, ${inputName}, context);
    sig_CONNECT_TO_SILENCE(self, ${otherInputName}, context);

}
```

### Generator

The generator function, which has a fixed type signature conforming to the ```sig_dsp_generateFn``` interface, is where your main sample generation loop is located. Signaletic supports configurable block sizes, so it is important to always read the block size from the signal's ```AudioSettings``` struct when generating samples.

Any time you access inputs or outputs (or any other ```float_array_ptr``` array), you'll need to use the ```FLOAT_ARRAY()``` macro to correctly deference it.

````c
void sig_dsp_${SignalName}_generate(void* signal) {
    // Cast a generic Signal pointer to the appropriate type.
    struct sig_dsp_${SignalName}* self = (struct sig_dsp_${SignalName}*) signal;

    // Generate each sample for a block of output.
    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        // Access inputs.
        float ${inputName} = FLOAT_ARRAY(self->inputs->${inputName})[i];

        // Calculate output.
        float val = ...;

        // Assign the output to the appropriate sample in the block.
        FLOAT_ARRAY(self->outputs.main)[i] = val;
    }
}
````

### Destructor

The destructor is responsible for freeing all memory allocated by the signal's constructor. It should not deallocate any pointers that were passed in as arguments to the signal's constructor--this is the responsibility of the caller.

````c
void sig_dsp_${SignalName}_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_${SignalName}* self) {
    // Destroy any memory that was allocated in the Signal's constructor.

    // Call the base Signal destructor.
    sig_dsp_Signal_destroy(allocator, self);
}
````
