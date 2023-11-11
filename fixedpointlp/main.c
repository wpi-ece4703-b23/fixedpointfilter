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

// coefficients are <8,8>, eg
//    the coefficient with value 1
//    corresponds to the fractional value 1 * (1.0f/256)

const int32_t coef_fix[FIRLEN] = {
             0,           1,           1,          -3,         -10,         -16,
            -9,          17,          52,          79,          79,          52,
            17,          -9,         -16,         -10,          -3,           1,
             1,           0
};

// q15_t is a data type with 15 fractional bits
// so, the value 1 in a q15_t corresponds to the fractional value 1 * (1.0f/32768)
// internally, a q15_t is mapped to a short, so it's actually a fix<16,15>

q15_t taps_fix[FIRLEN];

q15_t fir_fix(q15_t x) {
    taps_fix[0] = x;

    q15_t q = 0.0;
    uint16_t i;
    for (i = 0; i<FIRLEN; i++)
        //  <16,15> * <8,8> = <24,23>
        //  <24,23> >> 8 -> <16,15>
        q += (taps_fix[i] * coef_fix[i]) >> 8;

    for (i = FIRLEN-1; i>0; i--)
        taps_fix[i] = taps_fix[i-1];

    return q;
}

uint16_t processsample(uint16_t x) {

    if (xlaudio_pushButtonLeftDown()) {

        // LEFT BUTTON: fixed point version
        return xlaudio_q15_to_dac14(fir_fix(xlaudio_adc14_to_q15(x)));

    } else if (xlaudio_pushButtonRightDown()) {

        // RIGHT BUTTON: difference between fixed & floating point version
        float32_t qf = fir_float(xlaudio_adc14_to_f32(x));
        q15_t     qt = fir_fix(xlaudio_adc14_to_q15(x));
        float32_t qt2float;
        arm_q15_to_float(&qt, &qt2float, 1);
        return xlaudio_f32_to_dac14(qf - qt2float);

    } else {

        // NO BUTTON: floating point version
        return xlaudio_f32_to_dac14(fir_float(xlaudio_adc14_to_f32(x)));

    }
}

uint16_t processsample_flp(uint16_t x) {
    return xlaudio_f32_to_dac14(fir_float(xlaudio_adc14_to_f32(x)));
}

uint16_t processsample_fxp(uint16_t x) {
    return xlaudio_q15_to_dac14(fir_fix(xlaudio_adc14_to_q15(x)));
}

#include <stdio.h>

int main(void) {
    WDT_A_hold(WDT_A_BASE);

    uint32_t c;

    c = xlaudio_measurePerfSample(processsample_flp);
    printf("processsample_flp %d\n", c);

    c = xlaudio_measurePerfSample(processsample_fxp);
    printf("processsample_fxp %d\n", c);

    xlaudio_init_intr(FS_8000_HZ, XLAUDIO_J1_2_IN, processsample);
    xlaudio_run();

    return 1;
}
