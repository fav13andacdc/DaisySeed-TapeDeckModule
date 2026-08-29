## 🎛️ Custom Firmware: Tape Saturation & Through-Zero Doubletracker

**This fork transforms the Terrarium into a dual-sided tape emulator, featuring mastering-grade analog saturation on Side 1 and a mechanical through-zero flanger/chorus/echo on Side 2.**

### What This Is
This is a custom firmware build for the PedalPCB Terrarium (Daisy Seed), originally based on bkshepherd's Multi-Effect Guitar Pedal framework. It overrides the standard multi-effect list to boot directly into a bespoke `TapeDeckModule`.

### Under the Hood
*   **Jiles-Atherton Hysteresis:** Side 1 replaces standard clipping with a discrete-time approximation of the Jiles-Atherton magnetisation model, creating authentic, frequency-dependent analog tape warmth and compression.
*   **Auto Level Control (ALC):** A secondary "Cassette" voice routes the signal through a fast-attack/slow-release compressor before hitting the saturation block to simulate cheap, highly squashed cassette recorders.
*   **Through-Zero Doubletracker:** Side 2 features a fractional delay line (Lag Deck) that references a fixed permanent delay (Reference Deck). This allows the Lag Deck to cross the "zero" time barrier (-0.3ms to 3ms) for authentic studio tape flanging.
*   **Fractal Brownian Motion (FBM) Drift:** Instead of using a standard sine wave LFO for chorus/flanging, this module uses 1D Perlin noise layered via FBM to recreate the chaotic mechanical wow, flutter, and capstan friction of physical tape machines.

### The Controls

**Footswitches**
*   **Left Footswitch (FS1):** Engages Side 1 (Tape Saturation). Automatically comes on when the pedal is engaged. Can be turned off using the Left Footswitch.
*   **Right Footswitch (FS2):** Engages Side 2 (Through-Zero Doubletracker). Currently, this also acts as the True Bypass. So technically, Double-Tracking is always on. Use the Blend knob, described below, to take doubletracking out of the signal.

**Knobs**
*   **Saturation - Pot1:** Controls the drive into the tape machine. Lower settings provide subtle analog warmth; higher settings compress and distort as the virtual magnetic tape saturates.
*   **Tone - Pot2:** Low-Pass Filter, very warm to bright.
*   **Level - Pot3:** Master output volume. Only on when Tape Saturation is engaged.
*   **Lag Time - Pot4:** A custom-curved control for the Doubletracker. 
    *   *0% to 50% (Flange):* Pushes the tape across the zero-crossing for massive, hollow comb-filtering.
    *   *50% to 75% (Chorus):* Separates the tape decks enough to create a lush, widening chorus.
    *   *75% to 100% (Slap/Echo):* Expands the delay time logarithmically up to 500ms for classic rockabilly slapback and tape echo.
*   **Blend - Pot5:** Crossfades between the Reference Deck and the Lag Deck.
*   **Wobble - Pot6:** Controls the mechanical health of the tape machine. At 0, the tape drifts with microscopic analog imperfection. Turning it up increases the depth and speed of the randomized wow and flutter.


**Toggles**
*   **Invert Toggle - TS1:** Flips the phase of the Lag Deck. Essential for getting infinitely deep cancellation notches when in the "Flange" zone of the Lag Time knob.
*   **Voice Toggle - TS2:** Switches between "Classic" (mastering-grade reel-to-reel saturation) and "Cassette" (squashed ALC compression).

---
# Daisy Seed Projects
This is a demo of a pedal board I put together using 3 of my custom Daisy Seed based guitar pedals. I designed and built the hardware and programmed the software. Everything shown here is open source and all the code, pcb files, and basic instructions on how to build the pedals, can be found on these pages.

[![Demo Video](https://img.youtube.com/vi/ZkLnS43acQo/0.jpg)](https://www.youtube.com/watch?v=ZkLnS43acQo)

## Software Projects:
### 1. [Guitar Multi-Effect Pedal](Software/GuitarPedal/README.md)
The software that can run on any of the custom Guitar Pedal hardware projects listed below as well at the PedalPCB Terrarium hardware.  This software is a flexible platform that abstracts the underlying Guitar Pedal hardware and allows a collection of Effects Modules to run on any of those hardware platforms.

## Hardware Projects:

I provide everything you need to build any of pedals you see below along with pretty detailed documentation.  However, recently I started to get a lot of requests to buy kits or finished pedals based on my designs. This is not something I typically do, but sometimes I do have extra PCBs, parts, and enclosures lying around and I'm willing to pass them along to you, basically at cost. I've included some ordering details along with each design below.

### 1. [Guitar Effect Pedal in 125b Enclosure](Hardware/GuitarPedal125b/README.md) (SMD Parts)
A feature rich and flexible platform for making DSP based guitar FX pedals. Stereo Audio, Midi in/out, OLED Display, Rotary Encoder for Menu Navigation, 6 Knobs, 2 LEDs, 2 Foot Switches, Relay based "True Bypass" switching, all based on the Daisy Seed microcontroller.

[Ordering Information](Hardware/GuitarPedal125b/docs/OrderingHardware.md)

![FinalProduct125b](Hardware/GuitarPedal125b/docs/images/FinalProduct.png) ![CircuitBoard125b](Hardware/GuitarPedal125b/docs/images/CircuitBoard-Back.png)

### 2. [Guitar Effect Pedal in 1590b Enclosure](Hardware/GuitarPedal1590b-SMD/README.md) (SMD Parts)

A robust and tiny platform for making DSP based guitar FX pedals. Stereo Audio, Midi in/out, 4 Knobs, 2 LEDs, 1 or 2 Foot Switches, Relay based "True Bypass" switching, all based on the Daisy Seed microcontroller. Using SMD parts for easy manufacturing by a fab such as JLCPCB.

[Ordering Information](Hardware/GuitarPedal1590b-SMD/docs/OrderingHardware.md)

![FinalProduct1590b-SMD](Hardware/GuitarPedal1590b-SMD/docs/images/FinalProduct.png) ![CircuitBoard1590b-SMD](Hardware/GuitarPedal1590b-SMD/docs/images/CircuitBoard.png)

### 3. [Guitar Effect Pedal in 1590b Enclosure](Hardware/GuitarPedal1590b/README.md) (Through-hole parts)

Same form factor and features as the SMD version of the 1590b pedal, minus the Relay based "True Bypass" switching. Using all Through-Hole parts for easy assembly at home with limited soldering skills.

[Ordering Information](Hardware/GuitarPedal1590b/docs/OrderingHardware.md)

![FinalProduct1590b](Hardware/GuitarPedal1590b/docs/images/FinalProduct.png) ![CircuitBoard1590b](Hardware/GuitarPedal1590b/docs/images/CircuitBoard.png)

More Information about the Daisy Seed can be found at:

http://electro-smith.com
