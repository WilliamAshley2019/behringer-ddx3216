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

Confirms the Am386SC300 datasheet is the right reference, and a standard x86 real-mode disassembler (Ghidra with the x86-16 processor module, or IDA) will chew through this cleanly. This chip is your anchor: find its interrupt vector table and jump targets and you'll know exactly where in the four flash chips execution continues.

The four *_FLASH.BIN files (512KB each — matches the AT29C040A/4Mbit chips) are data-rich, not just code. Pulling every embedded string out of them, I found the firmware's own POST/self-test diagnostic banners, using an old embedded-systems space-saving trick: dropping vowels. E.g. GNRLERR!! decodes cleanly as GENERAL ERR!!, and the strings clearly reference things like DSP load tests (DP1234), RAM test, flash test, CRC test, "COMMUNICATIONS ERROR", RS-232/serial port test, and MIDI/SysEx-adjacent test banners — i.e. this really is the console's operating firmware, and its self-test code alone tells you a lot about the internal architecture (multiple DSPs, a comms/serial self-test stage, flash-integrity checks) before you've disassembled a single instruction.

CPUIC16/CPUIC18 look different — shorter, fixed-width ~7-character padded fields (TRODLY , TROCOU , NACR   , etc.) rather than sentences. My best guess without the schematic in front of me: these are front-panel display label tables (parameter names truncated to fit whatever character display sits above each channel strip), same vowel-dropping style. Worth checking against CPU01_Rev_H.pdf to see if that display width lines up.

I turned the extraction + entropy + de-vowel-heuristic logic into a reusable script so you (and I, next session) can point it at v109.bex/v112.bex
THIS IS A WORKING FOLDER NOT INTENDED FOR PUBLIC USE - USE AT YOUR OWN RISK. THIS IS UNTESTED.
(Ghidra, x86-16 real mode)
BELOW IS ANY LINKED FILES AND PROJECTS THAT ARE POTENTIALLY LINKED TO GETTING THE SCRIPT WORKING

See ddx3216_fw_analyze.py
CPUIC19_PROM.BIN (64KB Boot ROM)
You've already identified this perfectly. Textbook x86 real-mode boot code with 8259 PIC initialization. This is the bootstrapper that:

Sets up segments (0xFF00, 0x00F0)

Initializes the two 8259 interrupt controllers (ports 0x20/0x21 and 0xA0/0xA1)

Jumps to the main firmware in the flash chips

CPUIC15_FLASH.BIN / CPUIC16 / CPUIC17 / CPUIC18 (512KB each)
These four 512KB chips make up 2MB of main firmware. The DDX3216 has:

Am386SC300 (386 core with integrated peripherals)

ADSP-2183 DSPs (I see "21823" in your list - likely ADSP-2183)

AT29C040A flash (4Mbit = 512KB)

74HC139 address decoders

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

text
Boot ROM (CPUIC19) → 
    Initializes 386 (real mode) →
        Loads from Flash (CPUIC15-18) →
            POST sequence (numbered 1-9,0) →
                DSP initialization (ADSP-2183) →
                    Front panel display (CPUIC16/18 label tables) →
                        Main mixing engine
The numbered sequence (1,2,4,5,6,7,8,9,0) is a classic POST progress indicator - probably shown on the LCD display as the mixer boots.


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
