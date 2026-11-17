# Changelog

All notable changes to this project will be documented in this file.

## [1.1.0] - 2026-11-16

### Added
- **Click-free internal crossfading:** Added 15 ms smooth mix ramps (`m_satMix`, `m_dtMix`) and continuous delay-buffer writing in `TapeDeckModule` to eliminate switching pops/clicks on soft-touch footswitches.
- **Active High-Shelf Tone EQ:** Replaced `daisysp::Tone` low-pass filter with a 1.2 kHz active shelving filter offering -12 dB cut to +14 dB active boost across the `Tone` knob sweep.
- **Sub-module state query:** Added `TapeDeckModule::IsEitherModuleEnabled()` so the host audio loop (`guitar_pedal.cpp`) continues processing audio when FS1 is active while FS2 is off.

### Changed
- **Footswitch & LED assignments:** Decoupled FS1 (`Alternate` / Left) and FS2 (`Bypass` / Right) so FS1 controls Saturation (Left LED) and FS2 controls the Doubletracker (Right LED) independently.
- **Doubletracker mixing law:** Switched `BLEND` from linear interpolation to an equal-power (`cos`/`sin`) crossfade to maintain unity gain when the Doubletracker is engaged without Saturation.

### Fixed
- Fixed ~3 dB perceived volume drop when engaging the Doubletracker alone.
- Fixed startup delay-time slew transient by initializing `m_currentLagSamples` to the 2 ms reference offset in `TapeDeckModule::Init()`.
- Fixed inverted LED mapping on Terrarium hardware (`LED 0` = Right/FS2, `LED 1` = Left/FS1).