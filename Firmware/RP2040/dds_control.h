#ifndef DDS_CONTROL_H
#define DDS_CONTROL_H
#include <stddef.h>
#include "dds_state.h"
typedef struct {
    uint32_t frequency_mHz;
    uint16_t amplitude_uA;
    uint8_t waveform;
    bool enabled;
} dds_control_t;
void dds_control_reset(dds_control_t *state);
/* Complete I2C write including command. Invalid writes change nothing. */
bool dds_control_write(dds_control_t *state, const uint8_t *data, size_t size);
void dds_control_status(const dds_control_t *state, uint8_t fault, uint8_t data[9]);
#endif
