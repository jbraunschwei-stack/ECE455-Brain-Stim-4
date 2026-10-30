#include "dds_control.h"

static uint16_t read_u16(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}
static uint32_t read_u32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
void dds_control_reset(dds_control_t *state) {
    *state = (dds_control_t){1000000U, DEFAULT_AMPLITUDE_UA, WAVE_SINE, false};
}
bool dds_control_write(dds_control_t *state, const uint8_t *data, size_t size) {
    if (!data || !size) return false;
    dds_control_t next = *state;
    switch (data[0]) {
        case 0x01:
            if (size != 5) return false;
            next.frequency_mHz = read_u32(data + 1);
            break;
        case 0x02:
            if (size != 2) return false;
            next.waveform = data[1];
            break;
        case 0x03:
            if (size != 3) return false;
            next.amplitude_uA = read_u16(data + 1);
            break;
        case 0x04:
            if (size != 2 || data[1] > 1) return false;
            next.enabled = data[1] != 0;
            break;
        case 0x05:
            if (size != 9 || data[8] > 1) return false;
            next.frequency_mHz = read_u32(data + 1);
            next.amplitude_uA = read_u16(data + 5);
            next.waveform = data[7];
            next.enabled = data[8] != 0;
            break;
        case 0xFF:
            if (size != 1) return false;
            dds_control_reset(&next);
            break;
        default: return false;
    }
    if (next.waveform >= WAVE_TYPE_COUNT) return false;
    if (next.frequency_mHz < FREQ_MIN_MHZ) next.frequency_mHz = FREQ_MIN_MHZ;
    if (next.frequency_mHz > FREQ_MAX_MHZ) next.frequency_mHz = FREQ_MAX_MHZ;
    if (next.amplitude_uA > DAC_IOUT_MAX_UA) next.amplitude_uA = DAC_IOUT_MAX_UA;
    *state = next;
    return true;
}
void dds_control_status(const dds_control_t *state, uint8_t fault, uint8_t data[9]) {
    for (unsigned i = 0; i < 4; ++i) data[i] = (uint8_t)(state->frequency_mHz >> (8 * i));
    data[4] = (uint8_t)state->amplitude_uA;
    data[5] = (uint8_t)(state->amplitude_uA >> 8);
    data[6] = state->waveform;
    data[7] = state->enabled && !fault;
    data[8] = fault;
}
