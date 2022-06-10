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

    struct sig_dsp_${SignalName}_Inputs* inputs;

    struct sig_dsp_${SignalName}_Parameters parameters;

    float ${stateName};
};
```

### Signal Inputs Definition

If your signal will read inputs from other signals, you must define them in an Inputs object. For the moment, all inputs are simply defined as ```float_array_ptr``` types, and are typically pointers directly to the output arrays of other signals.

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
    struct sig_AudioSettings* settings,
    struct sig_dsp_${SignalName}_Inputs* inputs) {
    float_array_ptr output = sig_AudioBlock_new(allocator, settings);
    struct sig_dsp_${SignalName}* self = sig_Allocator_malloc(allocator,
        sizeof(struct sig_dsp_${SignalName}));
    sig_dsp_${SignalName}_init(self, settings, inputs, output);

    return self;
}
```

### Initializer

The initializer is responsible for assigning values to a signal's struct, setting any defaults for parameters and instance state as needed. It should never allocate any dynamic memory, as this should be done in the signal's ```_new()``` function. The initializer is also responsible for creating and setting default values for a signal's parameters.

```c
void sig_dsp_${SignalName}_init(struct sig_dsp_${SignalName}* self,
    struct sig_AudioSettings *settings,
    struct sig_dsp_${SignalName}_Inputs* inputs,
    float_array_ptr output) {

    sig_dsp_Signal_init(self, settings, output,
        *sig_dsp_Value_generate);

    struct sig_dsp_${SignalName}_Parameters params = {
        .${parameterName} = ${parameterDefaultValue}
    };

    self->parameters = params;

    self->${stateName} = ${stateDefaultValue}
}
```

### Generator

The generator function, which has a fixed type signature conforming to the ```sig_dsp_generateFn``` interface, is where your main sample generation loop is located. Signaletic supports configurable block sizes, so it is important to always read the block size from the signal's ```AudioSettings``` struct when generating samples.

Any time you access inputs or outputs (or any other ```float_array_ptr``` array), you'll need to use the ```FLOAT_ARRAY()``` macro to correctly deference it.

````c
void sig_dsp_${SignalName}_generate(void* signal) {
    struct sig_dsp_${SignalName}* self = (struct sig_dsp_${SignalName}*) signal;

    for (size_t i = 0; i < self->signal.audioSettings->blockSize; i++) {
        float ${inputName} = FLOAT_ARRAY(self->inputs->${inputName})[i];

        float val = ${inputName};

        FLOAT_ARRAY(self->signal.output)[i] = val;
    }
}
````

### Destructor

The destructor is responsible for freeing all memory allocated by the signal's constructor. It should not deallocate any pointers that were passed in as arguments to the signal's constructor--this is the responsibility of the caller.

````c
void sig_dsp_${SignalName}_destroy(struct sig_Allocator* allocator,
    struct sig_dsp_${SignalName}* self) {
    sig_dsp_Signal_destroy(allocator, self);
}
````
