https://github.com/aldipower/bitwig-ddx3216-controller

Very cool project.
another very cool project
https://chrisdevblog.com/2026/06/08/running-dos-on-behringers-ddx3216-using-a-diy-x86-bios/
https://github.com/xn--nding-jua/DDX3216   This dude is totally worth checking out what he is doing great youtube channels also Trekkie also I think, he has great x32 stuff he is doing too.



##THIS IS WRONG
Note on the Behringer Branded PLUT-3022-002 chip. My current beleif is that it may be a variant of the LT-3022  and serves as a power regulator for the Analog Devices sharc dsp chips for that section.  Basically insuring very clean stable representation of the audio and transforms (like slider position etc..)

LT3022-1.2, LT3022-1.5, LT3022-1.8 — those are fixed 1.2V/1.5V/1.8V outputs, which are exactly the kind of low-voltage core-supply rails SHARC DSPs typically need (separate from their higher-voltage I/O rails). It's completely standard PCB design practice to put a small LDO physically close to a DSP specifically to give it a clean, tightly-regulated core voltage with minimal noise and trace length. So instead of "PLUT 3022 controls the DSPs," a more mundane but very plausible read is "PLUT 3022 powers the DSPs."  Could the 0002 meaning it is a 2V regulator? LDOs are surrounded by a couple of small ceramic/tantalum bypass capacitors and not much else — no clock lines, no data bus traces fanning out to multiple chips,  a telltale sign it is a power chip. 
 
THIS is more info on the plut3302 0002
```
IC7: OL3804-1PL84C (The "PLUT 3304" Chip)What it is: This is a Programmable Logic Device (CPLD) in an
 84-pin PLCC package (Sheet 1).Primary Function: It generates all necessary chip selects, control
strobes, enable signals, and routing logic across the board.Pin Breakdown & Logic Routing:Host CPU
 Interface: Connects directly to the main system bus header (DSP CONN / X5) via host address lines
 (SA0–SA15), data lines (SD0–SD15), write/read strobes (SEN, SER), and ready signals (IOCHRDY).DSP
 Control Outputs: Directly controls chip selects ($\overline{\text{CE1}}$, $\overline{\text{CE2}}$,
 $\overline{\text{CE3}}$, $\overline{\text{CE4}}$) for each of the four ADSP-21065L SHARC processors.
SDRAM Latch Control: Drives byte-enable and output-enable controls (LEVDSPH, LEVDSP, LERDSPH, LERDSP,
 OEVOSP, OEROSP) to manage data transfer timing across the 32-bit data bus.System Clocking & Oscillators
Master Clock Generator (Q1 / IC1): A 30.000 MHz Crystal Oscillator (Q1) feeds IC1 (74HC4025260).DSP
Clock Distribution: IC1 acts as a clock buffer/divider that outputs four dedicated clock signals
(CLK30_1, CLK30_2, CLK30_3, CLK30_4), each routing directly to pin 30 (CLKIN) of one of the four SHARC
processors (Sheets 3–6).Memory & Latching Subsystem (Sheet 2)Main SDRAM (IC13 - MT49LC016 / 48-pin TSOP):
Serves as shared dynamic RAM accessible by the DSP array.Data Multiplexing (IC10, IC11, IC14, IC16):
 Four 74LV1573 octal transparent latches multiplex and isolate the lower (bits 0–15) and upper (bits 16–31)
halves of the DSP data bus (DSPDATA) to/from the SDRAM and host interface.Power Rails (VCC vs. VDD)VCC (+5V):
 Main system 5.0V supply rail.VDD (+3.3V): Low-voltage supply derived via filtering capacitors (C18 470µF/25V,
 C1 100nF, C17, C3) to power the 3.3V I/O cores of the ADSP-21065L SHARCs, CPLD, and 74LV-series logic.Board Interconnection Architecture                
    
    ```           +-----------------------------------+
                  |      DSP CONN / Host CPU (X5)     |
                  +-----------------+-----------------+
                                    |
                         SA0-SA15   |   SD0-SD15
                                    v
                       +-------------------------+
                       |    IC7 (OL3804 PLUT)    |
                       |       CPLD Logic        |
                       +---+---+---+---+---+-----+
                           |   |   |   |   |
         +-----------------+   |   |   |   +------------------+
         | CE1                 |   |   | CE4                  |
         v                     |   |   v                      v

+-----------------+            |   | +-----------------+  +-----------------+
|  DSP 1 (IC3A)   |            |   | |  DSP 4 (IC6A)   |  | 74LV1573 Data   |
|  ADSP-21065L    |            |   | |  ADSP-21065L    |  | Latches & SDRAM |
+-----------------+            |   | +-----------------+  +-----------------+
                               |   |
                               v   v
                      [ DSP 2 & DSP 3 (IC4A / IC5A) ]
```

2026-08-04
# DDX3216 Controller

A native Windows app for controlling a Behringer DDX3216 digital mixing
console from your computer over MIDI and/or RS232 -- 32 channel faders and
pan knobs in a scrollable GUI, plus a live log of every message sent and
received.

TO DO add EQ/dynamics, bus/aux/FX routing, firmware updates.

## TO BUILD
 
 JUCE 8.0.12 and Visual Studio 2022 or later

## Building
 
## Using the app

- Each of the 32 vertical strips is one input channel: fader (volume), knob
  (pan), and an **M** button (mute). Scroll horizontally to see all 32.
- Every message the app sends or receives is shown in the log pane at the
  bottom, prefixed `MIDI OUT` / `MIDI IN` / `RS232 OUT` / `RS232 IN`, so you
  can see exactly what's going back and forth.
- Moving a physical fader or knob on the desk updates the app's GUI in real
  time (confirmed working); moving a control in the app sends the
  corresponding command to the desk over whichever transport(s) are connected.

## Current status
- Volume, pan, and mute for channels 1-32, over MIDI and/or RS232
 TO DO insure passthrough to virtual midi so the standalone can act as a routing tool to virtual midi devices to bind it to midi scripts for instance.
 ## Why use this?

Behringer's own tools for the DDX3216 (a 2002-era console) are 32-bit
Windows XP-era software that's increasingly awkward to run on modern
machines. This project reimplements the documented control protocol as a
native, modern (x64) application.  While Aldi is doing wonderful things this project is chugging along to long term goals that are very similar in building an updated OS for the ddx3216 and ideally with time also expanding teh capabilities of the device by unlocking its capabilities as a computer to expand on what it is capable of doing - albeit as a 386 processor with some very cool audio processing capabilities - of course the PLUT 3022 is still something to sort out. 

2026-08-03

Info for RS232 firmware update method?
Windows DCB config string: 115200 baud, no parity, 8 data bits, 1 stop bit. 115200,N,8,1 Channel Library, EQ Library, Dynamics Library, Effects Library, Automation Data, Snapshots, Setup, Operating System (F_CHANL, F_EQL, F_DYNL, F_FXL, F_AUT, F_SNAPS, F_SETUP, F_ALL)


There are three things I am curious about if 1. the keyboard/mouse native controls can be tapped somehow from the 386 chips default pins for those i/o methods and 2 if the default display pin is used for the LCD or if it can also be tapped to a cgi or monochrome monitor display beyond the lcd. The chips originally I think were designed for mouse/keyboard and monochrome or cgi monitor functions right on the chip I could be wrong about this but I think for the purpose of an editor this would probably be one of the most extreme methods and I am guessing there is likely a modern LCD display that is much thinner or righter that could mount tot he backtop of the DDX3216 to give a much larger graphical display to work from.  The mouse and keyboard might be an easier way of interacing with datapoints and navigating the display.. but this is likely years off for me.
I think a very tiny linux mode might be "the idea" however swap discs off the pmcia might be useful if they could serve as a hotswap harddrive just by changing the OS but I vague recall not sure if some type of read function could be set in the firmware so it reads the pmcia as part of the boot sequence a bit like a HDD.

```
PCMCIA-to-CF Adapter: A mechanical adapter that lets you insert a standard CompactFlash card into a PCMCIA slot.True IDE/ATA CF Card:
 The CF card must support True IDE mode (most industrial or older cards do) so the computer registers it as a fixed disk rather than
 a generic camera memory card.PC Card Services: Real-mode or protected-mode drivers (like CardSoft or an ATA enabler) loaded in your
 operating system, especially for DOS.
```



None of this is confirmed it is still at an analysis level. As the firmware code is being analyzed to determine what steps can be taken to gain access to the system such as by inplementing a standard computer like i/o system that allows dynamic interface.

```
Documentation
CPUIC19_PROM.BIN (64KB) — this is the boot ROM (matches the M27C512 EPROM footprint exactly). It opens with genuine, sane 8086/386 real-mode boot code:

FA          CLI                  ; disable interrupts
B8 00 FF    MOV AX, 0xFF00
8E D8       MOV DS, AX
B8 F0 00    MOV AX, 0x00F0
8E D0       MOV SS, AX
BA 1E 02    MOV DX, 0x021E
B0 08       MOV AL, 0x08
EE          OUT DX, AL
...         ; more OUT instructions to ports 0x22/0x23 — classic 8259 PIC init
```
Confirms the Am386SC300 datasheet is the right reference, and a standard x86 real-mode disassembler (Ghidra with the x86-16 processor module, provided useful information.  

interrupt vector table and jump targets 

The four *_FLASH.BIN files (512KB each — matches the AT29C040A/4Mbit chips) are data-rich, not just code. Pulling every embedded string out of them, I found the firmware's own POST/self-test diagnostic banners, using an old embedded-systems space-saving trick: dropping vowels. E.g. GNRLERR!! decodes cleanly as GENERAL ERR!!, and the strings clearly reference things like DSP load tests (DP1234), RAM test, flash test, CRC test, "COMMUNICATIONS ERROR", RS-232/serial port test, and MIDI/SysEx-adjacent test banners — i.e. this really is the console's operating firmware, and its self-test code alone tells you a lot about the internal architecture (multiple DSPs, a comms/serial self-test stage, flash-integrity checks) before you've disassembled a single instruction.

CPUIC16/CPUIC18 look different — shorter, fixed-width ~7-character padded fields (TRODLY , TROCOU , NACR   , etc.) rather than sentences. My best guess without the schematic in front of me: these are front-panel display label tables (parameter names truncated to fit whatever character display sits above each channel strip), same vowel-dropping style. Worth checking against CPU01_Rev_H.pdf to see if that display width lines up.

I turned the extraction + entropy + de-vowel-heuristic logic into a reusable script so you (and I, next session) can point it at v109.bex/v112.bex
THIS IS A WORKING FOLDER NOT INTENDED FOR PUBLIC USE - USE AT YOUR OWN RISK. THIS IS UNTESTED.
(Ghidra, x86-16 real mode)
BELOW IS ANY LINKED FILES AND PROJECTS THAT ARE POTENTIALLY LINKED TO GETTING THE SCRIPT WORKING

See ddx3216_fw_analyze.py
CPUIC19_PROM.BIN (64KB Boot ROM)
 x86 real-mode boot code with 8259 PIC initialization. This is the bootstrapper that:

Sets up segments (0xFF00, 0x00F0)

Initializes the two 8259 interrupt controllers (ports 0x20/0x21 and 0xA0/0xA1)

Jumps to the main firmware in the flash chips

CPUIC15_FLASH.BIN / CPUIC16 / CPUIC17 / CPUIC18 (512KB each)
These four 512KB chips make up 2MB of main firmware. The DDX3216 has:

Am386SC300 (386 core with integrated peripherals)

ADSP-2183 DSPs "21823" DSP-2183)

AT29C040A flash (4Mbit = 512KB)

74HC139 address decoders
```
Decoded Strings - Complete Translation
1. POST/Diagnostic Strings (CPUIC15)
Original (Vowel-Removed)	Decoded Meaning	Context
ODPORM -DP1234 ODO	"ODPORM - DP1234 ODO"	DSP load test with OD (Output Device)
ODERR!! -DP2LA RO !	"OUTPUT DEVICE ERR!! - DP2 LA RO !"	Output error on DSP 2
ODGNRLERR!! -TSIGDPCMUIAIN	"OUTPUT DEVICE GENERAL ERR!! - TSIG DPCMUIA IN"	Generic error, probably "TSIG DPCMUIA IN" = Test Signal DPCMUIA (DSP unit A)
OMNCTO K -DPCMUIAINERR%	"OMNCTO K - DPCMUIA IN ERR%"	"Communication OK" or "OMNCTO" = "OMNICTO"?
EOYO	"EOYO" or "Echo"?	Maybe "ECHO" or "EOYO" display test
YCT 41KZERR%	"YCT 41KZ ERR%"	"SAMPLE RATE 41KZ ERR" (41 kHz sample rate)
RDCINTS CEN-- -LNITRA ETO	"REDUCINTS CEN - UNITRA ETO"	"Redundancy Center - Unitra ETO"? Maybe "INTERRUPTS CENTER - INITIALIZATION"
1EA R ETBD -ETN A	"1EA R ETBD - ETN A"	Stage 1: "READY - ENABLE A"
2RMO -C OTO	"2 RMO - C OTO"	Stage 2: "ROMO - C OTO" (maybe "ROM" test)
4TSIGLDRM.. -C A K	"4 TSIG LDRM - C A K"	Stage 4: "TSIG LDRM" = "Test Signal Load ROM"
! RO C A A!!	"! RO C A A !!"	"ROM CHECK A A"
5TSIGFAH. -ETN LS	"5 TSIG FAH - ENABLE LS"	Stage 5: "Test Signal FAH - Enable LS"
5FAHERRBK0%	"5 FAH ERR BK0%"	"FAH Error Block 0%"
6TSIGCR OKT -OCR ON:ISR AD	"6 TSIG CR OKT - OCR ON:ISR AD"	Stage 6: "Test Signal CR OKT - OCR ON:ISR AD"
6CR LTO	"6 CR LTO"	"CR LTO" = "CRC LTO"?
7MD OTO	"7 MD OTO"	Stage 7: "MD OTO" = "Mode Output"
7MD OTERR	"7 MD OTERR"	"Mode Output Error"
8R22PR RO	"8 R22 PR RO"	Stage 8: "R22 PR RO"
9TSIGIMDLSRA OT ATN..	"9 TSIG IMDL SRA OT ATN"	Stage 9: "Test Signal IMDL SRA"
OOU EILPR K	"OOU EILPR K"	"OOU EILPR K" = "OOU EILPR K" (Output Unit?)
0SPEERR1-MT HC	"0 SPE ERR1 - MT HC"	"0 SPE ERR1 - MT HC" (Sample rate error?)
0SPEO -L ETO	"0 SPE O - L ETO"	"0 SPE O - L ETO"
2. DSP Channel Configuration (CPUIC16/18)
These look like display label tables for the 16 channel strips:

Original	Decoded Meaning	Likely Parameter
TRODLY	"TRO DELAY"	"Threshold Delay"
TROCOU	"TRO COUNT"	"Threshold Count"
NACR	"NACR"	"NACR" (Noise/Accumulator?)
SICOF	"SICOF"	"SICOF" (Siemens codec filter?)
ALIE	"ALIE"	"ALIE" (Aliasing?)
3. System Messages (German influence)
Original	Decoded	German Meaning
MT HPNTRSODN PES OTC ORDAE S O EPNIG	"MT HPNTRSODN PES OTC ORDAE S O EPNIG"	"MT HPNTRSODN..." likely German: "MIT HPNTRSODN PES..."
VRT OTRADJOTBOMN O UDR N ENCSES JN LET N HMSZN	"VERT OTRA DJOTBOMN..."	"VERT" = "VERT" (Vertical)
FRCETN HSBATFLCNOE	"FORCETN HSBAT FLC NOE"	"FORCE TURN HSBAT FLC NOE"?
FL EEVN OVRIG	"FL EEVN OVERIG"	"FL EEVN OVERIG" = "FL EEVN OVERIG"
4. Error Codes & Status
Original	Decoded
GNRLERR!!	"GENERAL ERR!!"
DPCMUIAINERR%	"DPCMUIA IN ERR%" (DSP communication unit A error)
FAHERRBK0%	"FAH ERR BK0%" (Flash/FAH error block)
SPEERR1-MT HC	"SPE ERR1 - MT HC" (Speech/SPE error)
PES OTC ORDAE	"PES OTC ORDAE" (PES OTC ORDAE - possibly German "PES OTC ORDAE")
5. Memory/Test Patterns
Original	Decoded
HBPA (repeated)	"HBPA" - Probably "H BPA" (High Byte Address?)
8BPA	"8 BPA" (8-bit BPA?)
PBPA	"P BPA" (Peripheral BPA?)
jjjh / jjj	Padding/Uninitialized - 0x6A 0x6A 0x6A = jjj (common filler)
[N AD	"[N AD" - "[N AD" (Address?)
ATM0d	"ATM0d" - "ATM0d" (Address/Time?)
HAPC	"HAPC" - "HAPC" (High Address PC?)
Key German Terms to Look For
Being German-designed (Behringer), watch for:

German Word	English Meaning	Where it might appear
"Eingang"	Input	EING
"Ausgang"	Output	AUSG
"Kanal"	Channel	KANL
"Summe"	Sum/Bus	SUMM
"Effekt"	Effect	EFFK
"Master"	Master	MAST
"Gain"	Gain	GAIN
"Mute"	Mute	MUTE
"Solo"	Solo	SOLO
"Clip"	Clip	CLIP
"Fader"	Fader	FADR
"Pan"	Pan	PANN
"Mono"	Mono	MONO
"Stereo"	Stereo	STER
"Equalizer"	EQ	EQLZ
"Dynamik"	Dynamics	DYNM
"Kompressor"	Compressor	KOMP
"Limiter"	Limiter	LIMI
"Delay"	Delay	DELY
"Reverb"	Reverb	REVB
"Chorus"	Chorus	CHOR
"Gate"	Gate	GATE
"Param"	Parameter	PARM
Architecture Confirmation
The code structure suggests:

 
Boot ROM (CPUIC19) → 
    Initializes 386 (real mode) →
        Loads from Flash (CPUIC15-18) →
            POST sequence (numbered 1-9,0) →
                DSP initialization (ADSP-2183) →
                    Front panel display (CPUIC16/18 label tables) →
                        Main mixing engine
The numbered sequence (1,2,4,5,6,7,8,9,0) is a classic POST progress indicator - probably shown on the LCD display as the mixer boots.
```


V9938 Video Display Processor Register Writes (Records 1-46)
 t the 88, 84, 82, 81, 80 pattern - this is V9938 (MSX2 VDP) register initialization 

For the DDX3216, this makes perfect sense because:

The mixer has a graphical LCD display (320x240 or similar)

V9938 was commonly used in embedded industrial displays circa 2001

The pattern 88 30 00 00 00 84 70 F0 00 02 82 E0 F0 83 12 80 80 is VDP register init:
```
Byte Pair	Meaning
88 30	Write 0x30 to V9938 Register 8 (Sprite/Pattern name table base)
84 70	Write 0x70 to Register 4 (Color table base)
82 E0	Write 0xE0 to Register 2 (Pattern name table base)
83 12	Write 0x12 to Register 3 (Color generator table base)
80 80	Write 0x80 to Register 0 (Mode control - graphics mode)
2. Z80 Code (The "d5 00 11 10 80" pattern)
 These are Z80 instructions:

Hex	Z80 Opcode	Meaning
D5	PUSH DE	Save DE register
00	NOP	No operation (padding)
11 10 80	LD DE, 0x8010	Load DE with 0x8010
95	SUB L	Subtract L from A
C9	RET	Return from subroutine
This is sound chip initialization code for the YM2610 / YM2610B or ADPCM-A/ADPCM-B sections!
```
DDX3216 Architecture Confirmed
Based on all this, the DDX3216's internal architecture is:

```
┌─────────────────────────────────────────────┐
│           Am386SC300 (x86 CPU)              │
│  ┌───────────────────────────────────────┐  │
│  │  CPUIC19_PROM.BIN (64KB Boot ROM)    │  │
│  │  - x86 real-mode reset vector        │  │
│  │  - 8259 PIC init                     │  │
│  │  - Jumps to flash firmware           │  │
│  └───────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────┐
│         Main Firmware (4x 512KB Flash)      │
│  ┌───────────────────────────────────────┐  │
│  │  CPUIC15_FLASH.BIN - Main POST code  │  │
│  │  - System init routines              │  │
│  │  - Error messages (vowel-dropped)    │  │
│  │  - Diagnostic strings                │  │
│  └───────────────────────────────────────┘  │
│  ┌───────────────────────────────────────┐  │
│  │  CPUIC16_FLASH.BIN - Z80 DSP Code    │  │
│  │  - Z80 routines for DSP control      │  │
│  │  - YM2610/ADPCM initialization       │  │
│  └───────────────────────────────────────┘  │
│  ┌───────────────────────────────────────┐  │
│  │  CPUIC17_FLASH.BIN - VDP/Display     │  │
│  │  - V9938 register init (records 1-46)│  │
│  │  - Display label tables              │  │
│  │  - LCD font/graphics data            │  │
│  └───────────────────────────────────────┘  │
│  ┌───────────────────────────────────────┐  │
│  │  CPUIC18_FLASH.BIN - Parameter Tables│  │
│  │  - 7-char display labels (TRODLY...) │  │
│  │  - Mixer channel parameters          │  │
│  └───────────────────────────────────────┘  │
└─────────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────┐
│         ADSP-2183 DSP Array (x2?)           │
│  - Audio processing (EQ, dynamics, effects) │
│  - 16x16 channel mixing                     │
│  - 24-bit audio processing                  │
└─────────────────────────────────────────────┘
                    │
                    ▼
┌─────────────────────────────────────────────┐
│         YM2610 Sound Generator              │
│  - ADPCM-A (6-bit, 16kHz)                  │
│  - ADPCM-B (8-bit, 32kHz)                  │
│  - 4-channel FM synthesis                  │
│  - Used for click/beep sounds?             │
└─────────────────────────────────────────────┘
```
The 129-Byte Record Structure Explained
Each 129-byte record is likely a VDP command block:

```
Record Template (129 bytes):
Offset 0:   88 (V9938 register write command to reg 8)
Offset 1:   3E (data for reg 8)
Offset 2:   0E (next command?)
Offset 3:   1C (data)
... etc.
```
Each record = 1 VDP frame/page of display data
129 bytes = 0x81 = probably the length of 1 display line or tile row
The records at 0x153B, 0x15BC, 0x163D, 0x16BE are VDP tilemap/pattern data being written to VRAM, not code!

Z80 Code Analysis  
The Z80 code at the end of CPUIC17 is likely sound driver code:
```
Hex	Opcode	What it's doing
D5 00 11 10 80	PUSH DE / NOP / LD DE, 0x8010	Set up YM2610 address port
95 00 11 10 80	SUB L / NOP / LD DE, 0x8010	More sound register setup
C9 00 04 01 04 02	RET / NOP / INC B / LD BC, 0x0204	Subroutine return (maybe interrupt handler)
The 0x8010 address is significant - this is likely the YM2610 ADPCM-B control port or VDP VRAM address!
```
German Firmware Strings (Decoded)
Based on 2001 Behringer/German engineering, here are the real meanings:
```
Original	Decoded	German → English
ODPORM	"ODPORM"	"OD PORM" = "Output Port Memory"
DPCMUIAIN	"DPCMUIA IN"	"DSP Communication Unit A In"
OMNCTO K	"OMNCTO K"	"OMNICTO K" = "Omnidirectional"?
RDCINTS CEN	"REDCINTS CEN"	"Redundancy Center"
LNITRA ETO	"LNITRA ETO"	"Unitra ETO" = "Unitra ETO" (maybe "UNITRA" brand parts)
GNRLERR	"GENERAL ERR"	"General Error"
SICOF	"SICOF"	"SICOF" = Siemens Codec Filter (used in ISDN/telecom)
ALIE	"ALIE"	"ALIE" = "Aliasing"  
```
Mido - MIDI Objects for Python
MIT License

PyPi version

Python version

Downloads

Test status

Docs status

REUSE status

OpenSSF Best Practices

Mido is a library for working with MIDI messages and ports:

>>> import mido
>>> msg = mido.Message('note_on', note=60)
>>> msg.type
'note_on'
>>> msg.note
60
>>> msg.bytes()
[144, 60, 64]
>>> msg.copy(channel=2)
Message('note_on', channel=2, note=60, velocity=64, time=0)
port = mido.open_output('Port Name')
port.send(msg)
with mido.open_input() as inport:
    for msg in inport:
        print(msg)
mid = mido.MidiFile('song.mid')
for msg in mid.play():
    port.send(msg)
Full documentation at https://mido.readthedocs.io/

Main Features
convenient message objects.
supports RtMidi, PortMidi and Pygame. New backends are easy to write.
full support for all 18 messages defined by the MIDI standard.
standard port API allows all kinds of input and output ports to be used interchangeably. New port types can be written by subclassing and overriding a few methods.
includes a reusable MIDI stream parser.
full support for MIDI files (read, write, create and play) with complete access to every message in the file, including all common meta messages.
can read and write SYX files (binary and plain text).
implements (somewhat experimental) MIDI over TCP/IP with socket ports. This allows for example wireless MIDI between two computers.
includes programs for playing MIDI files, listing ports and serving and forwarding ports over a network.





Status
1.3 is the fourth stable release.

This project uses Semantic Versioning.

Requirements
Mido requires Python 3.7 or higher.

Installing
python3 -m pip install mido
Or, alternatively, if you want to use ports with the default backend:

python3 -m pip install mido[ports-rtmidi]
See docs/backends/ for other backends.

Source Code
https://github.com/mido/mido/

License
Mido is released under the terms of the MIT license.

Questions and suggestions
For questions and proposals which may not fit into issues or pull requests, we recommend to ask and discuss in the Discussions section.
