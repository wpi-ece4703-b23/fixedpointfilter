#include "xlaudio.h"
#include "xlaudio_armdsp.h"

#define FIRLEN 20

const float32_t coef_float[FIRLEN] = {
    0.001588789281,  0.004897921812,  0.00443964405,  -0.009905842133, -0.03936388716,
   -0.06086875126,  -0.03351534903,   0.0654496327,    0.204924494,     0.3078737259,
    0.3078737259,    0.204924494,     0.0654496327,   -0.03351534903,  -0.06086875126,
   -0.03936388716,  -0.009905842133,  0.00443964405,   0.004897921812,  0.001588789281
};

float32_t taps_float[FIRLEN];

float32_t fir_float(float32_t x) {
    taps_float[0] = x;

    float32_t q = 0.0;
    uint16_t i;
    for (i = 0; i<FIRLEN; i++)
        q += taps_float[i] * coef_float[i];

    for (i = FIRLEN-1; i>0; i--)
        taps_float[i] = taps_float[i-1];

    return q;
}

uint16_t processsample(uint16_t x) {
        return xlaudio_f32_to_dac14(fir_float(xlaudio_adc14_to_f32(x)));
}

#include <stdio.h>

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    uint32_t c = xlaudio_measurePerfSample(processsample);
    printf("cycle count %d\n", c);

    xlaudio_init_intr(FS_8000_HZ, XLAUDIO_J1_2_IN, processsample);
    xlaudio_run();

    return 1;
}
