# Behringer DDX3216 MIDI System Exclusive Protocol

## 1. System Exclusive Header Format

| Byte | Value | Description |
|------|-------|-------------|
| 1 | `F0` | SysEx Start |
| 2 | `00` | Manufacturer ID (2-byte format) |
| 3 | `20` | Behringer Manufacturer ID |
| 4 | `32` | Device ID + Active MIDI Channel Info |
| 5 | `dd` | Apparatus ID (`0B` for DDX3216) |
| 6 | `rf` | Function Code (`0rffffff`) |

### Byte 4: MIDI Channel Info (`ic`)
- Format: `0AB0` where:
  - **A** = 1: Ignore App ID
  - **B** = 1: Ignore MIDI Channel (Omni)
  - **c** = MIDI Channel `0..F` (1..16)

### Byte 6: Function Code (`rf`)
- **r** = Request bit: `1` = Request, `0` = Here's the data
- **ffffff** = Function number `0..3F`

### Function Numbers
| Code | Function |
|------|----------|
| `0` | Connection Test |
| `4` | Meter Data |
| `F` | Memory Dump |
| `20` | Parameter Change |
| `22` | Channel Attenuation |
| `50` / `10` | Request / Dump Current Settings |
| `51` / `11` | Request / Dump PC-Card File List |
| `52` / `12` | Request / Dump File from PC-Card |

### "What" Byte (`ww`) for Memory Dumps
| Value | Meaning |
|-------|---------|
| `F_ALL` | All data |
| `F_SETUP` | Prefs and Status |
| `F_CHANL` | Channel Library |
| `F_EQL` | EQ Library |
| `F_DYNL` | Dynamics Library |
| `F_FXL` | FX Library |
| `F_AUT` | Automation |
| `F_SNAPS` | Snapshots |

---

## 2. Parameter Change (Function 20)

All changed controllers are sent per frame within one SysEx header.

After the function code:

| Field | Size | Description |
|-------|------|-------------|
| `nn` | 1 byte | Number of changed parameters (min 1, max 23 per frame) |
| Parameters | 4 bytes each | See below |

### Parameter Frame (4 bytes each)
| Byte | Field | Description |
|------|-------|-------------|
| 1 | `le` | Module (channel) number |
| 2 | `2e` | Parameter number |
| 3 | `3e` | Parameter value: High 7 bits (1:1 mapping) |
| 4 | `4e` | Parameter value: Low 7 bits |

Terminator: `F7` (End of SysEx)

---

## 3. Channel Attenuation (Function 22)

All changed controllers are sent per frame within one SysEx header.

After the function code:

| Field | Size | Description |
|-------|------|-------------|
| `nn` | 1 byte | Number of changed parameters (min 1, max 23 per frame) |
| Parameters | 3 bytes each | See below |

### Attenuation Frame (3 bytes each)
| Byte | Field | Description |
|------|-------|-------------|
| 1 | `le` | Channel number (`0` = Ch 1; attenuates all channels in same mute group) |
| 2 | `2e` | Parameter value: High 7 bits (1:1 mapping) |
| 3 | `3e` | Parameter value: Low 7 bits |

Terminator: `F7`

---

## 4. Channel Parameters

### Input Channel Parameters
| # | Name | Range | Formula/Notes |
|---|------|-------|---------------|
| 1 | Volume | 0–1472 | dB = -80 + value/16 |
| 2 | Mute | 0/1 | On/Off |
| 3 | Pan | 0–60 | dB = -30 + value |
| 4 | Rout.toMain | 0/1 | On/Off |
| 5 | Rout.toBus | 0/1 | On/Off |
| 6 | Bus Volume | 0–1472 | dB = -80 + value/16 |
| 7 | Bus Volume Pre/Post | 0/1 | On/Off |
| 8 | Bus Pan | 0–60 | dB = -30 + value |
| 9 | Bus Pan Follow Channel | 0/1 | On/Off |
| 20 | EQ On | 0/1 | On/Off |
| 22 | EQ Band 1 Frequency | 0–159 | Hz = 20 × 1000^(value/159) |
| 23 | EQ Band 1 Gain | 0–72 | dB = -18 + value/2 |
| 24 | EQ Band 1 Q | 0–40 | Q = 0.1 × 100^(value/40) |
| 26 | EQ Band 2 Frequency | 0–159 | Hz = 20 × 1000^(value/159) |
| 27 | EQ Band 2 Gain | 0–72 | dB = -18 + value/2 |
| 28 | EQ Band 2 Q | 0–40 | Q = 0.1 × 100^(value/40) |
| 30 | EQ Band 3 Frequency | 0–159 | Hz = 20 × 1000^(value/159) |
| 31 | EQ Band 3 Gain | 0–72 | dB = -18 + value/2 |
| 32 | EQ Band 3 Q | 0–40 | Q = 0.1 × 100^(value/40) |
| 34 | EQ Band 4 Frequency | 0–159 | Hz = 20 × 1000^(value/159) |
| 35 | EQ Band 4 Gain | 0–72 | dB = -18 + value/2 |
| 36 | EQ Band 4 Q | 0–40 | Q = 0.1 × 100^(value/40) |
| 37 | High Pass On | 0/1 | On/Off |
| 38 | High Pass Frequency | 0–80 | Hz = 4 × 100^(value/80) |
| 40 | Compressor On | 0/1 | On/Off |
| 42 | Compressor Attack | 0–200 | msec = value |
| 43 | Compressor Release | 0–255 | msec = 20 × 250^(value/255) |
| 44 | Compressor Ratio | 0–15 | 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0, 6.0, 8.0, 10.0, 20.0, 100.0 |
| 45 | Compressor Knee | 0/1 | On/Off |
| 46 | Compressor Threshold | 0–60 | dB = -60 + value |
| 47 | Compressor Gain | 0–24 | dB = value |
| 50 | Gate On | 0/1 | On/Off |
| 51 | Gate Hold | 0–255 | msec = 10 × 100^(value/255) |
| 52 | Gate Attack | 0–200 | msec = value |
| 53 | Gate Release | 0–255 | msec = 20 × 250^(value/255) |
| 54 | Gate Range | 0–61 | dB = -value (61 = -∞) |
| 55 | Gate Threshold | 0–90 | dB = -90 + value |
| 60 | Channel Delay On | 0/1 | On/Off |
| 62 | Delay Time | 0–115 | samples = value² |
| 63 | Delay Feedback | 0–180 | % = -90 + value |
| 64 | Delay Mix | 0–100 | % = value |
| 70 | Aux 1 Send Volume | 0–1472 | dB = -80 + value/16 |
| 71 | Aux 1 Pre/Post | 0/1 | On/Off |
| 72 | Aux 2 Send Volume | 0–1472 | dB = -80 + value/16 |
| 73 | Aux 2 Pre/Post | 0/1 | On/Off |
| 74 | Aux 3 Send Volume | 0–1472 | dB = -80 + value/16 |
| 75 | Aux 3 Pre/Post | 0/1 | On/Off |
| 76 | Aux 4 Send Volume | 0–1472 | dB = -80 + value/16 |
| 77 | Aux 4 Pre/Post | 0/1 | On/Off |
| 80 | FX 1 Send Volume | 0–1472 | dB = -80 + value/16 |
| 81 | FX 1 Pre/Post | 0/1 | On/Off |
| 82 | FX 2 Send Volume | 0–1472 | dB = -80 + value/16 |
| 83 | FX 2 Pre/Post | 0/1 | On/Off |
| 84 | FX 3 Send Volume | 0–1472 | dB = -80 + value/16 |
| 85 | FX 3 Pre/Post | 0/1 | On/Off |
| 86 | FX 4 Send Volume | 0–1472 | dB = -80 + value/16 |
| 87 | FX 4 Pre/Post | 0/1 | On/Off |

### Bus Parameters
| # | Name | Range | Formula/Notes |
|---|------|-------|---------------|
| 1 | Volume | 0–1472 | dB = -80 + value/16 |
| 2 | Mute | 0/1 | On/Off |
| 3 | Pan | 0–60 | dB = -30 + value |
| 4 | Rout.toMain | 0/1 | On/Off |
| 5 | Rout.toBus | 0/1 | On/Off |
| 6 | Bus Volume | 0–1472 | dB = -80 + value/16 |
| 7 | Bus Volume Pre/Post | 0/1 | On/Off |
| 8 | Bus Pan | 0–60 | dB = -30 + value |
| 9 | Bus Pan Follow Channel | 0/1 | On/Off |
| 70 | Aux 1 Send Volume | 0–1472 | dB = -80 + value/16 |
| 71 | Aux 1 Pre/Post | 0/1 | On/Off |
| 72 | Aux 2 Send Volume | 0–1472 | dB = -80 + value/16 |
| 73 | Aux 2 Pre/Post | 0/1 | On/Off |
| 74 | Aux 3 Send Volume | 0–1472 | dB = -80 + value/16 |
| 75 | Aux 3 Pre/Post | 0/1 | On/Off |
| 76 | Aux 4 Send Volume | 0–1472 | dB = -80 + value/16 |
| 77 | Aux 4 Pre/Post | 0/1 | On/Off |

### Aux Master Parameters
| # | Name | Range | Formula/Notes |
|---|------|-------|---------------|
| 1 | Volume | 0–1472 | dB = -80 + value/16 |
| 2 | Mute | 0/1 | On/Off |
| 3 | Pan | 0–60 | dB = -30 + value |

### FX Unit Parameters
| # | Name | Range | Notes |
|---|------|-------|-------|
| 90 | FX Type | 0–26 | See FX Algorithms below |
| 91 | FX Parameter 1 | varies | See FX chapter |
| 92 | FX Parameter 2 | varies | See FX chapter |
| 93 | FX Parameter 3 | varies | See FX chapter |
| 94 | FX Parameter 4 | varies | See FX chapter |
| 95 | FX Parameter 5 | varies | See FX chapter |
| 96 | FX Parameter 6 | varies | See FX chapter |
| 97 | FX Parameter 7 | varies | See FX chapter |
| 98 | FX Parameter 8 | varies | See FX chapter |

---

## 5. FX Algorithms

### 0: Bypass
| Param | Name | Range | Notes |
|-------|------|-------|-------|
| 90 | FX Type | 0 | Bypass |

### 1: Cathedral
**Structure:** PreDelay → Hi-Shelv → Reverb (with MOD)

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 1 | | |
| 91 | Decay | 0–89 | 2–20 s | log |
| 92 | Damping | 0–100 | 0–100 % | lin |
| 93 | Bass Multiply | 0–100 | -50 to +50 | lin |
| 94 | Reverb Modulation | 0–49 | 1–50 | lin |
| 95 | PreDelay | 0–139 | 0–490 ms | log |
| 96 | Density | 0–50 | 0–50 | lin |
| 97 | Diffusion | 0–20 | 0–20 | lin |
| 98 | Hi-Shelv Damp | 0–30 | 0–30 dB | lin |

### 2: Plate
**Structure:** FILTER → ER → REVERB

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 2 | | |
| 91 | Decay | 0–90 | 1–10 s | log |
| 92 | HiDec Damp | 0–100 | 0–100 % | lin |
| 93 | HiDec Freq | 0–106 | 0.2–20 kHz | log |
| 94 | Stereo Width | 0–20 | 0–20 | lin |
| 95 | PreDelay | 0–139 | 0–490 ms | log |
| 96 | Metal Res. | 0–20 | 0–20 | lin |
| 97 | Diffusion | 0–20 | 0–20 | lin |
| 98 | Hi-Shelv Cut | 0–30 | 0–30 dB | lin |

### 3: Small Hall
**Structure:** PreDel → Hi-Shelv → Reverb (with MOD)

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 3 | | |
| 91 | Decay | 0–34 | 0.5–1.2 s | log |
| 92 | Damping | 0–100 | 0–100 % | lin |
| 93 | Bass Multiply | 0–100 | -50 to +50 | lin |
| 94 | Reverb Mod. | 0–49 | 1–50 | lin |
| 95 | PreDelay | 0–76 | 0–100 ms | log |
| 96 | Diffusion | 0–20 | 0–20 | lin |
| 97 | Hi-Shelv Freq | 0–53 | 1–10 kHz | log |
| 98 | Hi-Shelv Damp | 0–30 | 0–30 dB | lin |

### 4: Room
**Structure:** PreDel → Hi-Shelv → Reverb

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 4 | | |
| 91 | Decay | 0–43 | 0.5–1.5 s | log |
| 92 | Damping | 0–100 | 0–100 % | lin |
| 93 | Bass Multiply | 0–100 | -50 to +50 | lin |
| 94 | Diffusion | 0–20 | 0–20 | lin |
| 95 | PreDelay | 0–92 | 0–300 ms | log |
| 96 | Mic Distance | 0–100 | 0–100 | lin |
| 97 | Hi-Shelv Freq | 0–53 | 1–10 kHz | log |
| 98 | Hi-Shelv Damp | 0–30 | 0–30 dB | lin |

### 5: Concert
**Structure:** FILTER → ER → REVERB

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 5 | | |
| 91 | Decay | 0–90 | 0.8–8.5 s | log |
| 92 | HiDec Damp | 0–100 | 0–100 % | lin |
| 93 | ER/Rev Balance | 0–100 | 0–100 % | lin |
| 94 | Size | 0–49 | 1–50 | lin |
| 95 | PreDelay | 0–138 | 0–490 ms | log |
| 96 | ER Stereo Width | 0–20 | 0–20 | lin |
| 97 | Diffusion | 0–20 | 0–20 | lin |
| 98 | Hi-Shelv Damp | 0–30 | 0–30 dB | lin |

### 6: Stage
**Structure:** FILTER → ER → REVERB

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 6 | | |
| 91 | Decay | 0–90 | 0.8–8.5 s | log |
| 92 | HiDec Damp | 0–100 | 0–100 % | lin |
| 93 | ER/Rev Balance | 0–100 | 0–100 % | lin |
| 94 | Size | 0–49 | 1–50 | lin |
| 95 | PreDelay | 0–138 | 0–490 ms | log |
| 96 | Rev-Delay | 0–138 | 0–490 ms | log |
| 97 | Diffusion | 0–20 | 0–20 | lin |
| 98 | Stereo Width | 0–20 | 0–20 | lin |

### 7: Spring Reverb
**Structure:** FILTER → ER → REVERB

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 7 | | |
| 91 | Decay | 0–36 | 2–5.5 s | log |
| 92 | HiDec Damp | 0–100 | 0–100 % | lin |
| 93 | HiDec Freq | 0–106 | 0.2–20 kHz | log |
| 94 | Stereo Width | 0–20 | 0–20 | lin |
| 95 | PreDelay | 0–138 | 0–490 ms | log |
| 96 | Metal Res. | 0–20 | 0–20 | lin |
| 97 | Hi-Shelv Freq | 0–68 | 1–20 kHz | log |
| 98 | Hi-Shelv Damp | 0–30 | 0–30 dB | lin |

### 8: Gated Reverb
**Structure:** FILTER → ER → REVERB

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 8 | | |
| 91 | Decay | 0–89 | 2–20 s | log |
| 92 | HiDec Damp | 0–100 | 0–100 % | lin |
| 93 | Diffusion | 0–20 | 0–20 | lin |
| 94 | Stereo Width | 0–20 | 0–20 | lin |
| 95 | PreDelay | 0–138 | 0–490 ms | log |
| 96 | Gate Thresh | 0–60 | -60 to 0 dB | lin |
| 97 | Gate Hold | 0–156 | 10–1000 ms | log |
| 98 | Gate Resp | 0–101 | 2–200 ms | log |

### 9: Stereo Delay
**Structure:** Dual delay with feedback via LP/HP filters

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 9 | | |
| 91 | Delay L | 0–2700 | 0–2700 ms | lin |
| 92 | Delay R | 0–2700 | 0–2700 ms | lin |
| 93 | Feedback L | 0–99 | 0–99 % | lin |
| 94 | Feedback R | 0–99 | 0–99 % | lin |
| 95 | Feedback-HP | 0–144 | 20 Hz–10 kHz | log |
| 96 | Feedback-LP | 0–122 | 100 Hz–20 kHz | log |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 10: Echo
**Structure:** Dual delay with cross-feedback via LP/HP

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 10 | | |
| 91 | Delay L | 0–1800 | 0–1800 ms | lin |
| 92 | Delay R | 0–1800 | 0–1800 ms | lin |
| 93 | Feedback Del. L | 0–162 | 0–900 ms | log |
| 94 | Feedback Del. R | 0–162 | 0–900 ms | log |
| 95 | Feedback-HP | 0–144 | 20 Hz–10 kHz | log |
| 96 | Feedback-LP | 0–122 | 100 Hz–20 kHz | log |
| 97 | Feedback | 0–99 | 0–99 % | lin |
| 98 | Input Gain-R | 0–100 | 0–100 % | lin |

### 11: Stereo Chorus
**Structure:** Dual pitch shifter with LFO

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 11 | | |
| 91 | Wave | 0–1 | 0=Tri, 1=Sine | switch |
| 92 | LFO Speed | 0–94 | 0.05–20 Hz | log |
| 93 | Mod Depth | 0–100 | 0–100 % | lin |
| 94 | Mod Delay | 0–99 | 5–100 ms | log |
| 95 | Stereo Phase | 0–2 | 45°, 90°, 180° | switch |
| 96 | — | — | — | — |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 12: Stereo Flanger
**Structure:** Dual pitch with feedback via LP

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 12 | | |
| 91 | Wave | 0–1 | 0=Tri, 1=Sine | switch |
| 92 | LFO Speed | 0–94 | 0.05–20 Hz | log |
| 93 | Mod Depth | 0–100 | 0–100 % | lin |
| 94 | Mod Delay | 0–99 | 0.5–50 ms | log |
| 95 | Feedback | 0–198 | -99 to +99 % | lin |
| 96 | Feed-LP | 0–106 | 0.2–20 kHz | log |
| 97 | Stereo Phase | 0–2 | 45°, 90°, 180° | switch |
| 98 | — | — | — | — |

### 13: Stereo Phaser
**Structure:** Dual phase shift with feedback

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 13 | | |
| 91 | Stages | 2–29 | 2–9 stages | lin |
| 92 | Speed | 0–76 | 0.0–14 Hz | log |
| 93 | Depth | 0–100 | 0–100 % | lin |
| 94 | Feedback | 0–198 | -99 to +99 % | lin |
| 95 | Stereo Phase | 0–180 | 0–180° | lin |
| 96 | — | — | — | — |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 14: Pitch Shifter
**Structure:** Pitch shift → Delay with feedback

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 14 | | |
| 91 | Semitones | 0–24 | -12 to +12 | lin |
| 92 | Cents | 0–100 | -50 to +50 | lin |
| 93 | Delay | 0–158 | 0–800 ms | log |
| 94 | Feedback | 0–80 | 0–80 % | lin |
| 95 | — | — | — | — |
| 96 | — | — | — | — |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 15: Delay (Mono)
**Structure:** Delay with feedback via LP/HP

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 15 | | |
| 91 | Delay | 0–1800 | 0–1800 ms | lin |
| 92 | Feedback | 0–99 | 0–99 % | lin |
| 93 | Feedback-HP | 0–144 | 20 Hz–10 kHz | log |
| 94 | Feedback-LP | 0–122 | 100 Hz–20 kHz | log |
| 95 | — | — | — | — |
| 96 | — | — | — | — |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 16: Flanger (Mono)
**Structure:** Pitch with feedback via LP

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 16 | | |
| 91 | Wave | 0–1 | 0=Tri, 1=Sine | switch |
| 92 | LFO Speed | 0–94 | 0.05–20 Hz | log |
| 93 | Mod Depth | 0–100 | 0–100 % | lin |
| 94 | Mod Delay | 0–99 | 0.5–50 ms | log |
| 95 | Feedback | 0–198 | -99 to +99 % | lin |
| 96 | Feed-LP | 0–106 | 0.2–20 kHz | log |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 17: Chorus (Mono)
**Structure:** Pitch shifter with LFO

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 17 | | |
| 91 | Wave | 0–1 | 0=Tri, 1=Sine | switch |
| 92 | LFO Speed | 0–94 | 0.05–20 Hz | log |
| 93 | Mod Depth | 0–100 | 0–100 % | lin |
| 94 | Mod Delay | 0–99 | 5–100 ms | log |
| 95 | — | — | — | — |
| 96 | — | — | — | — |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 18: Phaser (Mono)
**Structure:** Phase shift with feedback

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 18 | | |
| 91 | Stages | 2–29 | 2–9 stages | lin |
| 92 | Speed | 0–76 | 0.0–14 Hz | log |
| 93 | Depth | 0–100 | 0–100 % | lin |
| 94 | Feedback | 0–198 | -99 to +99 % | lin |
| 95 | — | — | — | — |
| 96 | — | — | — | — |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 19: Tremolo
**Structure:** LFO amplitude modulation

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 19 | | |
| 91 | Wave | 0–2 | 0=Sine, 1=Tri, 2=Square | switch |
| 92 | Speed | 0–94 | 0.05–20 Hz | log |
| 93 | Depth | 0–100 | 0–100 % | lin |
| 94 | — | — | — | — |
| 95 | — | — | — | — |
| 96 | — | — | — | — |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 20: Autopan
**Structure:** LFO stereo amplitude modulation

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 20 | | |
| 91 | Wave | 0–2 | 0=Sine, 1=Tri, 2=Square | switch |
| 92 | Speed | 0–94 | 0.05–20 Hz | log |
| 93 | Depth | 0–100 | 0–100 % | lin |
| 94 | — | — | — | — |
| 95 | — | — | — | — |
| 96 | — | — | — | — |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 21: Enhancer
**Structure:** High processing + NR + Bass processing

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 21 | | |
| 91 | High Freq | 0–57 | 1–12 kHz | log |
| 92 | High Q | 0–30 | 1–4 | lin |
| 93 | Process | 0–100 | 0–100 % | lin |
| 94 | NR Response | 0–110 | 20–400 ms | log |
| 95 | Bass Freq | 0–53 | 50–500 Hz | log |
| 96 | Bass Q | 0–30 | 1–4 | lin |
| 97 | Bass Level | 0–100 | 0–100 % | lin |
| 98 | NR Threshold | 0–90 | -90 to 0 dB | lin |

### 22: Graphic EQ
**Structure:** 8-band graphic EQ

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 22 | | |
| 91 | 50 Hz | 0–60 | -15 to +15 dB | lin |
| 92 | 250 Hz | 0–60 | -15 to +15 dB | lin |
| 93 | 1.5 kHz | 0–60 | -15 to +15 dB | lin |
| 94 | 7 kHz | 0–60 | -15 to +15 dB | lin |
| 95 | 100 Hz | 0–60 | -15 to +15 dB | lin |
| 96 | 500 Hz | 0–60 | -15 to +15 dB | lin |
| 97 | 3.5 kHz | 0–60 | -15 to +15 dB | lin |
| 98 | 14 kHz | 0–60 | -15 to +15 dB | lin |

### 23: LFO Filter
**Structure:** LFO-modulated BP/LP/HP filter

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 23 | | |
| 91 | Speed | 0–105 | 0.05–40 Hz | log |
| 92 | Wave | 0–2 | 0=Sine, 1=Tri, 2=Square | switch |
| 93 | Base Freq | 0–100 | 100 Hz–10 kHz | log |
| 94 | Depth | 0–100 | 0–100 % | lin |
| 95 | — | — | — | — |
| 96 | Slewing | 0–48 | 1–50 ms | log |
| 97 | Filter Mode | 0–2 | 0=HP, 1=BP, 2=LP | switch |
| 98 | Filter Q | 0–49 | 1–20 | log |

### 24: Auto Filter
**Structure:** Envelope-follower BP/LP/HP filter

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 24 | | |
| 91 | Base Freq | 0–100 | 100 Hz–10 kHz | log |
| 92 | Sensitivity | 0–100 | 0–100 % | lin |
| 93 | Attack | 0–156 | 10–1000 ms | log |
| 94 | Release | 0–156 | 10–1000 ms | log |
| 95 | Filter Mode | 0–2 | 0=HP, 1=BP, 2=LP | switch |
| 96 | Filter Q | 0–49 | 1–20 | log |
| 97 | — | — | — | — |
| 98 | — | — | — | — |

### 25: LowFi
**Structure:** Bit crusher + noise + buzz

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 25 | | |
| 91 | Bits | 0–6 | 6–16 bits | log |
| 92 | Noise Gain | 0–100 | 0–100 % | lin |
| 93 | Noise HP | 0–154 | 20 Hz–16 kHz | log |
| 94 | Noise LP | 0–106 | 0.2–20 kHz | log |
| 95 | Signal HP | 0–154 | 20 Hz–16 kHz | log |
| 96 | Signal LP | 0–121 | 0.1–20 kHz | log |
| 97 | Buzz Gain | 0–100 | 0–100 % | lin |
| 98 | Buzz Freq | 0–1 | 0=50 Hz, 1=60 Hz | switch |

### 26: Ring Modulator
**Structure:** AM modulation with envelope and LFO

| Param | Name | Sysex Range | Parameter Range | Scale |
|-------|------|-------------|-----------------|-------|
| 90 | FX Type | 26 | | |
| 91 | Mod Mode | 0–3 | 0=Sine, 1=Tri, 2=Square, 3=Env | switch |
| 92 | LFO Speed | 0–107 | 0.1–100 Hz | log |
| 93 | AM Carrier Freq | 0–106 | 0.1–10 kHz | log |
| 94 | Band Limit | 0–106 | 0.2–20 kHz | log |
| 95 | Modulation Depth | 0–100 | 0–100 % | lin |
| 96 | Env Response | 0–156 | 10–1000 ms | log |
| 97 | AM Depth | 0–100 | 0–100 % | lin |
| 98 | — | — | — | — |

---

## 6. MIDI File Dump Protocol (RS232)

### Request / Dump Header
| Byte | Value | Description |
|------|-------|-------------|
| 1 | `F0` | SysEx Start |
| 2 | `00` | Manufacturer ID (2-byte) |
| 3 | `20` | Behringer ID |
| 4 | `32` | Device ID + MIDI channel |
| 5 | `dd` | Apparatus ID (`0B` for DDX3216) |
| 6 | `rf` | Function code (`50`/`10`/`51`/`11`/`52`/`12`) |
| 7 | `ww` | "What" byte (F_ALL, F_SETUP, etc.) |

### Data Transfer Format (Request bit = 0)
After the header:

| Field | Size | Description |
|-------|------|-------------|
| `VV` | 1 byte | Data file version (1–127); current = 1 |
| `hh` | 1 byte | Total number of data blocks (block size = 1000); decoded: hh×128+11 (max ~14 MB) |
| `hh` | 1 byte | Block number (0 = first); decoded: hh×128+11 |
| `dd` | 1 byte | Byte count (0–127); number of data bytes |
| Data | variable | Decoded as 7 bytes of 7-bit data + 1 byte of high bits for previous 7 |
| `cc` | 1 byte | Checksum = `!(sum) & 0x7F` |
| `F7` | 1 byte | End of SysEx |

**Note:** Byte count is always modulo 8.

**Header size without data:** 15 bytes

### Request Format (Request bit = 1)
After the header:

| Field | Size | Description |
|-------|------|-------------|
| `hh` | 1 byte | Block number (0 = first); decoded: hh×128+11 |
| `F7` | 1 byte | End of SysEx |

**Header size without data:** 11 bytes

### File List Transfer
- **Request bit = 0:** Series of filenames, each 9 bytes (8 chars + null)
- **Request bit = 1:** No additional data

### Single File Transfer
- **Request bit = 0:** C-string filename followed by file data
- **Request bit = 1:** C-string filename only

### Example: Request Dump
```
F0 00 20 32 00 0B 50 01 00 00 F7   ; Request block 0
F0 00 20 32 00 0B 50 01 00 01 F7   ; Request block 1
F0 00 20 32 00 0B 50 01 00 02 F7   ; Request block 2
F0 00 20 32 00 0B 50 01 00 03 F7   ; Request block 3
...
F0 00 20 32 00 0B 50 01 00 0F F7   ; Request block 15
```

---

## 7. Error Handling & Block Transfer Protocol

### Getting a file FROM the DDX3216
```
PC (initiator)          DDX3216
--------                --------
REQ Block 0     →
                ←       Send Block 0
Send ACK        →       (Checksum OK)
                ←       REQ Block 1
Send Block 1    →
                ←       REQ Block 1 (Checksum NOK — retry)
Send Block 1    →
                ←       REQ Block 2 (Checksum OK)
Send Block 2    →
                        ...done
```

### Sending a file TO the DDX3216
```
PC (initiator)          DDX3216
--------                --------
Send Block 0    →
                ←       REQ Block 1 (Checksum OK)
Send Block 1    →
                ←       REQ Block 2 (Checksum OK)
Send Block 2    →
                        ...done
```

**Rule:** On wrong checksum, send a new request for that block.

---

## 8. Data Encoding

Data is encoded as 7 bytes of 7-bit data plus 1 byte containing the high bits of the previous 7 bytes.

Therefore, byte count is always modulo 8.

---

*Document Source: BEHRINGER Spezielle Studiotechnik GmbH, © 2002*
*Device: DDX3216 Digital Mixer*
