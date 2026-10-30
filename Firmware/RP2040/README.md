# RP2040 firmware for the DAC8831 Howland test board

This prototype connects an ESP32 I2C controller to an RP2040 waveform generator and the DAC8831 on the assembled Howland PCB. The analog path is DAC8831 U1, bipolar amplifier U5, low pass filter U6, then the U2/U3 Howland current source. The build retains the original Seeed XIAO RP2040 board selection.

H2 enable remains manual for the bench demo. These files are for resistor load testing. They have not been validated on the assembled hardware.

The current project targets are 5 mA peak current, a 30 V peak port voltage ceiling and frequencies from 500 Hz through 30 kHz. This first integration version supports calculated current commands through 5 mA but limits DDS frequency to 10 kHz at 100 ksample/s. A faster sampling implementation and hardware measurements are still needed for the 30 kHz target. This PCB/firmware does not implement a 30 V output clamp or validated port isolation.

## Important behavior

1. The DAC command for approximately zero current is **0x8000**, not 0x0000. U5 converts the DAC output to a bipolar command. Code 0x0000 requests approximately minus 2.55 V at FILTER_OUT.
2. Keep physical **H2 EN at ground during power on, reset, firmware upload and initial DAC checks**. The DAC powers up at zero code before firmware can write midscale. Software cannot prevent this interval or turn off H2.
3. Firmware starts with the waveform disabled and writes midscale. There is no automatic startup sweep. The default requested amplitude is 200 microamps peak at 1 kHz.
4. I2C SET_ENABLE changes the waveform amplitude target. It does not switch H2. Normal enable and disable ramp the amplitude at approximately 100 microamps per second; allow about two seconds to ramp a 200 microamp waveform to zero, or about 50 seconds at 5 mA. Ground H2 for immediate physical disable.
5. A missed sample period latches fault 0x02 and commands midscale. Investigate the timing before sending RESET and enabling again. A software fault response cannot guarantee the state of disconnected or failed hardware.

## H10 wiring

Wire by signal and header pin number, not by a Raspberry Pi computer's 40 pin connector numbers. GPIO numbers below are RP2040 GPIO numbers.

| H10 pin | Board signal | Connection |
| :--- | :--- | :--- |
| 1 | ISO1_GND | RP2040 GND |
| 2 | DAC_SCLK | GPIO2, XIAO D8 |
| 3 | DAC_SDI | GPIO3, XIAO D10 |
| 4 | DAC_CS_N | GPIO4, XIAO D9; ordinary GPIO output |
| 5 | DAC_LDAC_N | Board GND for this firmware |
| 6 | ISO1_3V3_GPIO | RP2040 3.3 V output for the board pullups |

H10 does not supply the DAC's analog power. P3 still needs +5 V, ground and minus 5 V. The integrated Howland test also needs P1's bipolar supply, initially the previously tested +12 V and minus 12 V, and manual H2 enable. The board has no isolation barrier between these signals. H11 and H12 both select pins 2 and 3 for the DAC path.

SPI0 uses mode 3, MSB first, exactly two bytes per conversion and one CS pulse per word. Data is captured on rising SCLK edges. LDAC must be grounded; leaving it pulled high prevents output updates. The DDS clock request is 10 MHz. The manual test requests 1 MHz for easier probing. The SDK may round clock speed down to a supported divider. AD5662 powerdown commands and the unused DMA stream have been removed.

## First test without an I2C master

Build and flash **dac_bench_test.uf2** with H2 EN grounded. This program does not generate an automatic waveform or sweep.

1. Power P3 correctly, with H11 and H12 selector caps removed and H2 EN grounded. Verify the reference test point is approximately 2.5 V. Connect H10 as above, then start the RP2040. All measurement grounds connect to board ground.
2. Open its USB serial port in a terminal. Type `?` for the command reminder. Each character acts immediately; a following Enter is ignored.
3. Probe FILTER_OUT relative to board ground. Send the characters below and check the voltages. It holds the selected code until another command or reset, so send `0` when finished.

| Character | DAC code | Expected FILTER_OUT | Expected P2 OUT with enabled Howland and 1 kohm load |
| :--- | :--- | :--- | :--- |
| `0` | 0x8000 | approximately 0 V | approximately 0 V |
| `+` | 0x8500 | approximately +99.6 mV | approximately +200 mV |
| `-` | 0x7B00 | approximately minus 99.6 mV | approximately minus 200 mV |

4. After the DAC checks pass, power down before changing wiring. Install both selector caps on pins 2 and 3, connect a 1 kohm resistor across P2 OUT and GND, and restore P3 and P1 supplies. Keep H2 EN low while the RP2040 starts and verify zero command again. Then apply the previously tested 3.3 V enable to H2 and repeat the small commands.
5. To try I2C waveforms, ground H2 and flash **rp2040_dds.uf2**. Verify zero command before enabling H2. Begin with 1 kHz, 200 microamps peak, sine. Expect approximately 0.400 V peak to peak across a 1 kohm resistor after the ramp settles.

## Current scaling and sample timing

The ideal low frequency transfer for this PCB is:

```
V_bipolar = 2.5 * (DAC_code / 32768 - 1)
V_command = V_bipolar * (1 + 499 / 24900)
I_load_A = V_command / 499
peak_code = floor(I_peak_uA * 499 * 32768 * 24900 / (2500000 * 25399))
DAC_code = 32768 + centered_waveform * peak_code
```

The requested amplitude is peak current, not peak to peak current. A 200 microamp peak request corresponds to a DAC deviation of 1282 codes. The 5000 microamp command limit corresponds to 32059 codes after rounding down. Actual amplitude depends on component tolerances, frequency response and available output compliance. This conversion is calculated scaling, not measured calibration. The firmware limits command amplitude; it does not measure current or enforce the 30 V peak output ceiling.

The waveform lookup tables are centered around midscale before amplitude scaling. Core 0 schedules samples every 10 microseconds, giving a nominal 100 ksample/s rate. Core 1 installs the I2C interrupts. USB serial is disabled in the DDS program and available in the separate manual test. This replaces the old measured free running loop constant, which would become invalid when changing the SPI message length. Verify frequency, jitter and fault status on hardware; 10 kHz has only ten samples per cycle, and square/sawtooth edges are limited by sampling and filtering.

## I2C control

I2C1 uses GPIO6 SDA and GPIO7 SCL with a 100 kHz master. External pullups to the appropriate 3.3 V domain are required. The existing isolated controller wiring must retain its own correctly referenced grounds and pullups.

Address selection uses GPIO26 (bit 0), GPIO27 (bit 1) and GPIO28 (bit 2), read once at boot. Internal pulldowns make unconnected pins zero. Tie the desired bits to 3.3 V, optionally through 1 kohm resistors. The address is 0x50 plus the three bit value, giving 0x50 through 0x57. These are different pins from the previous README.

| Command | Payload after command byte | Meaning |
| :--- | :--- | :--- |
| 0x01 | uint32, little endian | Frequency in mHz; clamped to 500 Hz through 10 kHz |
| 0x02 | byte | 0 sine, 1 square, 2 triangle, 3 sawtooth |
| 0x03 | uint16, little endian | Peak amplitude in microamps, clamped to 5000 |
| 0x04 | byte | 0 ramps to zero, 1 ramps to requested amplitude |
| 0x05 | frequency 4 bytes, amplitude 2, waveform 1, enable 1 | Set all parameters |
| 0x06 | none, then read 9 bytes | Read state and fault flags |
| 0xFF | none | Reset to 1 kHz, 200 microamps, sine, disabled; clear timing fault |

Each command must be a single complete I2C write transaction. Malformed lengths, invalid waveform numbers and enable values other than 0 or 1 are ignored. Readback reports clamped settings. The status layout remains frequency (4), amplitude (2), waveform (1), enable (1), fault (1). A fault makes reported enable zero. Requested state is not a measurement of actual current, and enable zero during a normal ramp down does not mean the ramp has already finished.

Example SET_ALL bytes for a 1 kHz, 200 microamp sine, enabled:

```
05 40 42 0F 00 C8 00 00 01
```

The optional earlier GPIO29 lead detector is disabled by default because this test board has no validated ADC sensing connection. Enable it only for a separately wired compatible sensing circuit with `-DDDS_ENABLE_LEAD_MONITOR=ON`. Its existing threshold of 75 counts is uncalibrated. Fault bit 0x01 commands midscale while asserted; bit 0x02 is a latched timing fault. This is not a validated lead protection system. Never connect the Howland output directly to an RP2040 ADC input.

## Build

Requires the Raspberry Pi Pico C/C++ SDK, an ARM GCC toolchain, CMake and Ninja. The local validation uses SDK 2.2.0 and ARM GCC 14.2.Rel1. Set `PICO_SDK_PATH` to the SDK directory and initialize its TinyUSB submodule for USB serial. The CMake file can import directly from the SDK; a separately uploaded pico_sdk_import.cmake is no longer required.

```
cmake -S . -B build -G Ninja -DPICO_BOARD=seeed_xiao_rp2040 -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The SDK needs picotool for UF2 output. The Pico VS Code extension normally supplies it. Otherwise install the matching picotool or let the SDK build it. `-DPICO_NO_PICOTOOL=1` permits compile checks but omits UF2 output. Use the board selection for the actual RP2040 module if it differs from XIAO.

Build outputs are `build/rp2040_dds.uf2` and `build/dac_bench_test.uf2`. With H2 EN grounded, use the RP2040 board's BOOTSEL procedure to copy the selected UF2 to its USB storage device.

## Tests and file organization

The unchanged `dds_tables.c` and `.h` contain waveform tables. `dds_state.h` defines the board transfer and timing. `dds_control.c` parses the existing command protocol, and `i2c_slave.c` handles the hardware bus. `spi_dac.c` drives the DAC8831. `main.c` runs DDS; `dac_bench_test.c` provides the independent manual voltage test.

Host tests exercise actual C code for zero output, amplitude limits, waveform centering, frequency conversion, invalid I2C packets, status data, SPI byte ordering and CS framing. They do not validate physical SPI timing or analog behavior.

```
cmake -S tests -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

References: [TI DAC8830/DAC8831 datasheet](https://www.ti.com/document-viewer/dac8830/datasheet), [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk/tree/2.2.0).
