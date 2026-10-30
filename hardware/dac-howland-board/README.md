# DAC, Filter, and Howland Current Source Board

This board is the single channel analog prototype for ECE455 Brain Stimulation 4. It supports testing the Howland current source with an external function generator or driving it through the onboard DAC and filter with a microcontroller.

## Circuit blocks

* **U1, DAC8831:** converts serial digital commands into an analog signal.
* **U4, REF5025:** supplies the 2.5 V DAC reference.
* **U5, OPA192:** produces the bipolar command voltage.
* **U6, OPA192:** implements the command low pass filter.
* **U2 and U3, OPA455:** implement the Howland current source and buffer. The 499 ohm sense resistor sets the nominal command to current scaling.

## Files

| Path | Contents |
| --- | --- |
| [schematics/dac-howland-schematics.json](schematics/dac-howland-schematics.json) | Editable EasyEDA schematic project containing HOWLAND CURRENT SOURCE and DAC COMMAND INTERFACE sheets |
| [pcb/dac-howland-pcb.json](pcb/dac-howland-pcb.json) | Combined DAC, filter, and Howland PCB with 58 component footprints |
| [archive/howland-only-pcb-copy.json](archive/howland-only-pcb-copy.json) | Separate PCB copy from the backup with 26 footprints and no DAC or OPA192 stages; retained for reference |

The JSON design files are preserved byte for byte from the supplied EasyEDA backup, with descriptive filenames. The original backup was named `BackupProjects_mjnugent_personal_1_20261030.zip`. The design files identify EasyEDA Standard editor version 6.5.51.

## Opening the design

Download the JSON files, then use **File > Open > EasyEDA...** in EasyEDA Standard. Open the schematic project and the combined PCB file, then save them into an EasyEDA project. The schematic JSON contains both sheets in one file.

## Interfaces and test modes

| Connector | Purpose |
| --- | --- |
| P1 | Howland positive supply, ground, and negative supply; board labels indicate +30 V and negative 30 V rails |
| P3 | Analog +5 V, ground, and negative 5 V supplies |
| H1 | External command input and ground |
| H2 | Manual Howland enable input and ground |
| H10 | Microcontroller interface for DAC clock, data, chip select, latch control, ground, and 3.3 V pullup supply |
| H11 and H12 | Command source selection; both jumpers on pins 1 and 2 select external command, both on pins 2 and 3 select DAC command |
| P2 | Load output and ground |
| H3 through H9, H13, H14 | Test points for ground, analog signals, supply rails, enable, reference, and filtered command |

For the assembled bench prototype, header pins were used in place of the specified terminal blocks. Some purchased components were substituted with equivalent parts, so supplier part numbers in the design are not a complete record of the assembled BOM.

## Prototype status

The board has been assembled and tested on resistor loads. The Howland stage was exercised independently with a function generator, followed by an integrated ESP32 to RP2040 to DAC to Howland test. These files document the bench prototype, not a validated final stimulation system.

PDF schematics and the manufacturing Gerbers are not included in this backup. Add the exact fabrication export used for the ordered PCB when available.

Circuit schematic design, PCB layout, and PCB assembly: **Mike Nugent**.
