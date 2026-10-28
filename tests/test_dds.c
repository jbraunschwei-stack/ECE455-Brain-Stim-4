#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "dds_control.h"

static void check_mapping(void) {
    /* Independent electrical equation, with less than one code of rounding. */
    const double ua_per_code = 1e6 * (2.5 / 32768.0) * (1 + 499.0 / 24900.0) / 499.0;
    for (unsigned ua = 0; ua <= 5000; ++ua) {
        const unsigned code = calc_amp_scale((uint16_t)ua);
        assert(code * ua_per_code <= ua + 1e-9);
        assert(ua - code * ua_per_code < ua_per_code + 1e-9);
    }
    assert(calc_amp_scale(65535) == calc_amp_scale(5000));
    assert(calc_amp_scale(200) == 1282);
    assert(calc_amp_scale(2000) == 12823);
    assert(calc_amp_scale(5000) == 32059);
    assert(ua_per_code * SAMPLE_RATE_HZ / AMP_RAMP_INTERVAL <= 100.0);
    for (unsigned sample = 0; sample < 65536; ++sample) {
        assert(dds_sample_code((uint16_t)sample, 0) == 0x8000);
        const unsigned code = dds_sample_code((uint16_t)sample, calc_amp_scale(5000));
        assert(code >= 32768 - 32059 && code <= 32768 + 32059);
    }
    assert(dds_sample_code(0x8000, 1282) == 0x8000);
    assert(dds_sample_code(0, 1282) == 32768 - 1282);
    assert(dds_sample_code(65535, 1282) == 32768 + 1281);
    assert(dds_sample_code(0, UINT32_MAX) == 32768 - 32059);
    for (unsigned wave = 0; wave < WAVE_TYPE_COUNT; ++wave) {
        int64_t sum = 0;
        for (unsigned i = 0; i < LUT_SIZE; ++i) {
            sum += (int)dds_sample_code(lut_table[wave][i], 32059) - 32768;
        }
        assert(fabs((double)sum / LUT_SIZE) < 2.0);
    }
    const double hz = calc_phase_inc(1000000) * (double)SAMPLE_RATE_HZ / 4294967296.0;
    assert(fabs(hz - 1000.0) < 0.001);
}

static void check_protocol(void) {
    dds_control_t s;
    dds_control_reset(&s);
    assert(!s.enabled && s.amplitude_uA == 200 && s.frequency_mHz == 1000000);
    const uint8_t all[] = {0x05, 0x40, 0x42, 0x0f, 0x00, 0xc8, 0x00, 0x00, 0x01};
    assert(dds_control_write(&s, all, sizeof(all)));
    assert(s.enabled && s.amplitude_uA == 200 && s.frequency_mHz == 1000000);
    uint8_t status[9];
    dds_control_status(&s, 0, status);
    const uint8_t expected[] = {0x40, 0x42, 0x0f, 0, 0xc8, 0, 0, 1, 0};
    assert(!memcmp(status, expected, sizeof(status)));
    dds_control_status(&s, DDS_FAULT_TIMING, status);
    assert(status[7] == 0 && status[8] == DDS_FAULT_TIMING);
    dds_control_t before = s;
    for (unsigned n = 0; n < sizeof(all); ++n) {
        assert(!dds_control_write(&s, all, n));
        assert(s.enabled == before.enabled && s.amplitude_uA == before.amplitude_uA);
    }
    const uint8_t bad_wave[] = {2, 4}, bad_enable[] = {4, 2};
    assert(!dds_control_write(&s, bad_wave, sizeof(bad_wave)));
    assert(!dds_control_write(&s, bad_enable, sizeof(bad_enable)));
    assert(s.waveform == WAVE_SINE && s.enabled);
    const uint8_t high_amp[] = {3, 0xff, 0xff};
    assert(dds_control_write(&s, high_amp, sizeof(high_amp)) && s.amplitude_uA == 5000);
    const uint8_t fractional[] = {1, 0x34, 0x44, 0x0f, 0}; /* 1000.500 Hz */
    assert(dds_control_write(&s, fractional, sizeof(fractional)));
    assert(s.frequency_mHz == 1000500);
    const uint8_t low_freq[] = {1, 0, 0, 0, 0};
    const uint8_t high_freq[] = {1, 0xff, 0xff, 0xff, 0xff};
    assert(dds_control_write(&s, low_freq, sizeof(low_freq)) && s.frequency_mHz == FREQ_MIN_MHZ);
    assert(dds_control_write(&s, high_freq, sizeof(high_freq)) && s.frequency_mHz == FREQ_MAX_MHZ);
    const uint8_t off[] = {4, 0};
    assert(dds_control_write(&s, off, sizeof(off)) && !s.enabled);
    const uint8_t reset[] = {0xff};
    assert(dds_control_write(&s, reset, sizeof(reset)));
    assert(!s.enabled && s.amplitude_uA == 200 && s.frequency_mHz == 1000000);
}
int main(void) {
    check_mapping();
    check_protocol();
    puts("PASS: bipolar output, current limits, waveform mean, frequency and I2C commands");
}
