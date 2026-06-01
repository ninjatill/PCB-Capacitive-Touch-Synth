# PCB-Capacitive-Touch-Synth
A Raspberry Pi-based synthesizer with capacitive touch keys directly on a PCB.

## Licensing

### Hardware
All files in `/hardware` are licensed under:

CC BY-NC-SA 4.0

### Firmware
All files in `/firmware` are licensed under:

GPLv3

## Concept
The PCB-Capacitive-Touch-Synth started as a sub-component of a larger science fair project. For my daughter's next science fair project (5th grade), she chose to create Plasma Speakers as we saw on the Franzoli Electronics YouTube page. The plasma speakers are flyback transformers that create a high-voltage arc (which ionizes the air into plasma). The frequeny of the arc creates audio tones. On Franzoli's demonstration videos, he plays MIDI-style music with each speaker acting as a left or right channel/voice. That is the concept we want to re-create.

If we re-create the plasma speakers, then how do we make then into an interactive display for the fair? Well, what if we had 2 midi-controller keyboards hooked up to a host PC. The host PC would do the heavy lifting of audio generation and output to the plasma speakers. Each keyboard would play its notes through either the left or right channel respectively. That's the main idea: 2 keyboards, a host PC, and plasma speakers.

To source the midi keyboards, you could go to Amazon and pick up a 30$-70$ keyboard (higher-end small midi keyboards can run upwards of 200-500$). But, commercial MIDI controllers usually do not output sound directly, it requires a host PC; so its usefullness outside the fair display is limited. The size and scale of the keyboards should be small (to match the entire setup which is portable and compact for a 36"-48" display.) The speakers on Franzoli's demos are very industrial/scientific looking. I thought commercial keyboards would look out of place or be too expensive. So I had a concept of creating keyboards printed on PCBs using capacitive touch buttons. The keyboards would be build to our specific needs and we can build in many features because of the DIY nature.

I decided the best course of action was to have my daughter re-create Fanzoli's Easy-Flyback circuits. It will teach her basic electrical components, schematic creation and PCB layout and fabrication. I think that's a wealth of knowledge for a 10 year old (maybe 11 by the time this comes to fruition). I will handle creating the PCB Capacitive Touch Synthesizer. It is a large complex project and is difficult for my own skill set.

## Documentation

- [Design Notes](docs/design-notes.md)

## Concept to Schematic
As an electrical engineer, I have created PCBs for simple devices. A capacitive touch synthesizer is the largest project/PCB I will have created. So, to jumpstart the project, I consulted my new "friend", ChatGPT. ChatGPT was able to answer questions and make recommendations faster than I could search DigiKey/Mouser, review datasheets, and find the right combination of ICs. But to be clear, I was driving the design. I wanted specific features and ChatGPT helped me realize those features faster than I could alone.

I started with the following design critera
- The front of the keyboard should be as flat as possible both for aestectic and functional reasons.
  - Aesthetically, I don't want a lot of clutter on the front of the board.
  - Functionally, Children will be interacting with this. I don't want exposed chip leads or solder pads. I also don't want through hole components sticking up with sharp edges. If kids can touch it, they will.
- The keyboard should be as small as possible but still allowing for decent usability (key widths wide enough for easy touch, etc.)
  
From the design criteria, we landed with some of the following choices:
- We will use capacitive touch sensors for the piano keys. Capacitive touch reduces mechanical parts that need fabricated (via 3D printing). It's more touch-friendly than many momentary push buttons.
- We will keep all IC's on the back of the board. Only LEDs and a volume knob will protrude from the front.
- We will use only surface mount components (to eliminate protrusions through the board and maintain the flat asthetic.)
- We will 3D print a back cover that serves 2 purposes: 
  - It will cover the back circuitry to prevent tampering.
  - It will raise the PCB off the display surface by a comfortable distance (and tilt it forward a few degrees) to increase usability.


## Basic Functionality
- The synthesizer will funtion in two main modes: standalone and MIDI.
  - In standalone mode, the keyboard will generate its own sounds via speaker or headphones.
  - In MIDI mode, the keyboard will send MIDI commands to a host PC (this is the intended mode for the science fair.
  
