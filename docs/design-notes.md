# Design Notes

This document captures the reasoning behind major design choices in the PCB Capacitive Touch Synth project. The goal is to explain not only what was built, but why certain tradeoffs were made.

## Project Goals

- Standalone capacitive-touch musical keyboard/synth
- USB MIDI support for host-connected use
- Local audio output through speaker and/or headphones
- Compact PCB-first physical design
- Reasonably hand-assemblable prototype
- Open hardware and firmware structure

## Major Design Decisions

### PCB as the Primary Interface

The touch pads, labels, speaker grille, and user-facing controls are built directly into the PCB.

**Reasoning:**

- Reduces mechanical complexity
- Makes the PCB part of the finished product
- Supports capacitive touch pads directly on the board
- Keeps the enclosure simpler

**Tradeoffs:**

- PCB layout becomes more constrained
- Front-side aesthetics matter
- Routing must avoid touch-sensitive areas

---

### RP2040 Microcontroller

The project uses the RP2040 as the main controller.

**Reasoning:**

- Good GPIO count
- Strong USB support
- Low cost
- Good community/toolchain support
- PIO flexibility for future audio or protocol needs

**Tradeoffs:**

- No built-in DAC
- External flash required
- Some audio features require careful firmware design

---

### Capacitive Touch Input

Capacitive touch controllers are used instead of mechanical keys.

**Reasoning:**

- Keeps the top surface flat
- Reduces mechanical wear
- Supports a PCB-as-instrument design
- Allows flexible pad shapes and labels

**Tradeoffs:**

- Requires careful grounding and routing
- More sensitive to layout and environmental noise
- Firmware must handle calibration and false touches

---

### Local Audio Output

The board supports local audio through a small speaker and headphone output.

**Reasoning:**

- Allows standalone operation without a host computer
- Makes the instrument immediately usable
- Supports testing without external equipment

**Tradeoffs:**

- Audio layout requires more care
- Speaker volume and bass are limited by enclosure size
- Headphone/speaker switching adds schematic and layout complexity

---

### Separate Audio/Analog Ground Area

The audio section uses a more deliberate ground layout to reduce noise coupling.

**Reasoning:**

- Keeps noisy digital return currents away from sensitive audio paths
- Helps headphone and amplifier performance
- Makes the audio section easier to reason about during layout review

**Tradeoffs:**

- Ground regions must reconnect intentionally
- Poor stitching could make noise worse instead of better
- Requires review before fabrication

---

### GitHub Project Structure

The repository separates hardware and firmware.

```text
hardware/
firmware/
docs/
```

**Reasoning:**

- Keeps KiCad files separate from source code
- Allows hardware and firmware to evolve together
- Makes the project easier for others to navigate

**Tradeoffs:**

- Requires discipline around commits and documentation
- Generated fabrication outputs should be managed carefully

### Open Questions
- Final enclosure design
- Speaker chamber geometry
- Final firmware architecture
- USB MIDI/sample-transfer behavior
- Production versus prototype assembly choices

Revision Log
Date	Note
2026-05-19	Initial design notes document created

