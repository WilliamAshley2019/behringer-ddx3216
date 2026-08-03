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

Confirms the Am386SC300 datasheet you already had in the repo is the right reference, and a standard x86 real-mode disassembler (Ghidra with the x86-16 processor module, or IDA) will chew through this cleanly. This chip is your anchor: find its interrupt vector table and jump targets and you'll know exactly where in the four flash chips execution continues.

The four *_FLASH.BIN files (512KB each — matches the AT29C040A/4Mbit chips) are data-rich, not just code. Pulling every embedded string out of them, I found the firmware's own POST/self-test diagnostic banners, using an old embedded-systems space-saving trick: dropping vowels. E.g. GNRLERR!! decodes cleanly as GENERAL ERR!!, and the strings clearly reference things like DSP load tests (DP1234), RAM test, flash test, CRC test, "COMMUNICATIONS ERROR", RS-232/serial port test, and MIDI/SysEx-adjacent test banners — i.e. this really is the console's operating firmware, and its self-test code alone tells you a lot about the internal architecture (multiple DSPs, a comms/serial self-test stage, flash-integrity checks) before you've disassembled a single instruction.

CPUIC16/CPUIC18 look different — shorter, fixed-width ~7-character padded fields (TRODLY , TROCOU , NACR   , etc.) rather than sentences. My best guess without the schematic in front of me: these are front-panel display label tables (parameter names truncated to fit whatever character display sits above each channel strip), same vowel-dropping style. Worth checking against CPU01_Rev_H.pdf to see if that display width lines up.

I turned the extraction + entropy + de-vowel-heuristic logic into a reusable script so you (and I, next session) can point it at v109.bex/v112.bex
THIS IS A WORKING FOLDER NOT INTENDED FOR PUBLIC USE - USE AT YOUR OWN RISK. THIS IS UNTESTED.
(Ghidra, x86-16 real mode)
BELOW IS ANY LINKED FILES AND PROJECTS THAT ARE POTENTIALLY LINKED TO GETTING THE SCRIPT WORKING

See ddx3216_fw_analyze.py

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
