#include <libsignaletic.h>
#include "../../../../include/signaletic-host.h"

struct sig_host_SummedCVIn {
    struct sig_dsp_Signal signal;
    struct sig_dsp_Signal_SingleMonoOutput outputs;
    struct sig_host_HardwareInterface* hardware;
    struct sig_host_CVIn* leftCVIn;
    struct sig_host_CVIn* rightCVIn;
    struct sig_dsp_BinaryOp* summer;
};

void sig_host_SummedCVIn_generate(void* signal) {
    struct sig_host_SummedCVIn* self = (struct sig_host_SummedCVIn*) signal;
    // Update child bindings.
    self->leftCVIn->hardware = self->hardware;
    self->rightCVIn->hardware = self->hardware;
    self->summer->inputs.left = self->leftCVIn->outputs.main;
    self->summer->inputs.right = self->rightCVIn->outputs.main;

    // Generate child signal output.
    self->leftCVIn->signal.generate(self->leftCVIn);
    self->rightCVIn->signal.generate(self->rightCVIn);
    self->summer->signal.generate(self->summer);
}

void sig_host_SummedCVIn_init(struct sig_host_SummedCVIn* self,
    struct sig_SignalContext* context) {
    sig_dsp_Signal_init(self, context, *sig_host_SummedCVIn_generate);

    self->summer->inputs.left = self->leftCVIn->outputs.main;
    self->summer->inputs.right = self->rightCVIn->outputs.main;
    self->outputs.main = self->summer->outputs.main;
}

struct sig_host_SummedCVIn* sig_host_SummedCVIn_new(
    struct sig_Allocator* allocator, struct sig_SignalContext* context) {
    struct sig_host_SummedCVIn* self = sig_MALLOC(allocator,
        struct sig_host_SummedCVIn);

    self->leftCVIn = sig_host_CVIn_new(allocator, context);
    self->rightCVIn = sig_host_CVIn_new(allocator, context);
    self->summer = sig_dsp_Add_new(allocator, context);

    sig_host_SummedCVIn_init(self, context);

    return self;
}
