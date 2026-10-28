# Validation record

Date: October 28, 2026. Based on repository commit 207d50e881744f1c81abbda3bb49a934f154aabd.

## Completed on the development computer

1. Built rp2040_dds and dac_bench_test for seeed_xiao_rp2040 using Pico SDK 2.2.0, ARM GCC 14.2.Rel1 and Release optimization. Both ELF targets linked successfully.
2. Generated both UF2 files with the official Raspberry Pi picotool 2.2.0 converter.
3. Ran the actual C math and protocol tests using Zig 0.14.1 as the host C compiler. Tests cover all requested integer amplitudes through 5000 microamps, all 65536 LUT input values at zero and maximum amplitude, actual waveform table means, frequency conversion, malformed commands, clamping, reset and status serialization. Passed.
4. Ran the SPI driver against mocks that check two data bytes, byte order, CS bracketing, mode 3, midscale initialization and the three manual test values. Passed.
5. Ran git diff --check. Passed.

## Still required on the bench

Verify actual RP2040 board selection, H10 wiring and LDAC ground connection. Keep H2 EN grounded until the reference and zero command have been measured. Run the manual DAC test before the I2C waveform firmware. Measure current, DC offset, frequency, timing fault status and enable behavior with resistor loads. Test the ESP32 command sequence and I2C readback on the connected boards.

The new project requirements are 5 mA peak and up to 30 kHz with a 30 V peak port voltage ceiling. The command conversion supports 5 mA; it is not a current measurement. This first DDS build remains limited to 10 kHz. A faster sampling implementation is needed before claiming 30 kHz support. Port voltage limiting, isolation and validated lead detection are outside this firmware change.

At 5 mA peak, a 1 kohm load requires 5 V peak and a 5 kohm load requires 25 V peak. Producing 30 V peak at 5 mA requires 6 kohm, plus about 2.495 V across the board's 499 ohm sense resistor and additional amplifier headroom. Therefore the existing plus/minus 30 V rails cannot deliver that full output target. Record the measured unclipped compliance limit instead of assuming it equals the supply voltage.
