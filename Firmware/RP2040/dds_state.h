#ifndef DDS_STATE_H
#define DDS_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "dds_tables.h"

/* Rev 1.1: REF5025, bipolar U5, U6 gain 1 + 499/24900, R5=499 ohms.
 * Ideal low-frequency transfer, not measured calibration. */
#define DAC_ZERO_CODE 32768U
#define DAC_VREF_UV 2500000ULL
#define HOWLAND_SENSE_OHMS 499ULL
#define FILTER_GAIN_NUM 25399ULL
#define FILTER_GAIN_DEN 24900ULL
#define DAC_IOUT_MAX_UA 5000U
#define DEFAULT_AMPLITUDE_UA 200U
#define SAMPLE_RATE_HZ 100000UL
#define SAMPLE_PERIOD_US (1000000UL / SAMPLE_RATE_HZ)
#define FREQ_MIN_MHZ 500000UL
#define FREQ_MAX_MHZ 10000000UL
#define AMP_RAMP_UA_PER_SEC 100ULL
#define RAMP_NUM (SAMPLE_RATE_HZ * DAC_VREF_UV * FILTER_GAIN_NUM)
#define RAMP_DEN (AMP_RAMP_UA_PER_SEC * HOWLAND_SENSE_OHMS * 32768ULL * FILTER_GAIN_DEN)
#define AMP_RAMP_INTERVAL ((uint32_t)((RAMP_NUM + RAMP_DEN - 1) / RAMP_DEN))
#define DDS_FAULT_LEAD_DISCONNECTED 0x01U
#define DDS_FAULT_TIMING 0x02U

extern volatile uint32_t g_phase_inc;
extern volatile wave_type_t g_wave_type;
/* Peak deviation from midscale in DAC codes. */
extern volatile uint32_t g_amplitude_target;
extern volatile bool g_lead_fault;
extern volatile bool g_timing_fault;

static inline uint32_t calc_phase_inc(uint32_t frequency_mHz) {
    return (uint32_t)(((uint64_t)frequency_mHz << 32) /
                      ((uint64_t)SAMPLE_RATE_HZ * 1000U));
}

static inline uint32_t calc_amp_scale(uint16_t peak_uA) {
    if (peak_uA > DAC_IOUT_MAX_UA) peak_uA = DAC_IOUT_MAX_UA;
    return (uint32_t)((peak_uA * HOWLAND_SENSE_OHMS * 32768ULL * FILTER_GAIN_DEN) /
                      (DAC_VREF_UV * FILTER_GAIN_NUM));
}

static inline uint16_t dds_sample_code(uint16_t lut_sample, uint32_t peak_code) {
    const uint32_t limit = calc_amp_scale(DAC_IOUT_MAX_UA);
    if (peak_code > limit) peak_code = limit;
    const int32_t centered = (int32_t)lut_sample - (int32_t)DAC_ZERO_CODE;
    /* Bounded by 32768 * 32059, within signed 32 bits. */
    const int32_t offset = (centered * (int32_t)peak_code) / 32768;
    return (uint16_t)((int32_t)DAC_ZERO_CODE + offset);
}
#endif
