# name=DDX3216 Control Universal
# url=
# supportedDevices=Behringer DDX3216

import patterns
import mixer
import device
import transport
import arrangement
import general
import launchMapPages
import playlist
import ui
import channels
import midi
import utils
import time
import plugins
import math

# Define MIDI message constants
SYSEX_START = 0xF0
SYSEX_END = 0xF7

# Constants from the user's code
DDX3216CU_KnobOffOnT = [(midi.MIDI_CONTROLCHANGE + (1 << 6)) << 16, midi.MIDI_CONTROLCHANGE + ((0xB + (2 << 4) + (1 << 6)) << 16)];
DDX3216CU_nFreeTracks = 64

DDX3216CUNote_Undo = 0x3C
DDX3216CUNote_Pat = 0x3E
DDX3216CUNote_Mix = 0x3F
DDX3216CUNote_Chan = 0x40
DDX3216CUNote_Tempo = 0x41
DDX3216CUNote_Free1 = 0x42
DDX3216CUNote_Free2 = 0x43
DDX3216CUNote_Free3 = 0x44
DDX3216CUNote_Free4 = 0x45
DDX3216CUNote_Marker = 0x48
DDX3216CUNote_Zoom = 0x64
DDX3216CUNote_Move = 0x46
DDX3216CUNote_Window = 0x4C

# DDX3216 CU pages
DDX3216CUPage_Pan = 0
DDX3216CUPage_Stereo = 1
DDX3216CUPage_Sends = 2
DDX3216CUPage_FX = 3
DDX3216CUPage_EQ = 4
DDX3216CUPage_Free = 5

ExtenderLeft = 0
ExtenderRight = 1

OffOnStr = ('off', 'on')

# Class to represent a single control column (fader, knob, buttons)
class TDDX3216Col:
	def __init__(self):
		self.TrackNum = 0
		self.BaseEventID = 0
		self.KnobEventID = 0 
		self.KnobPressEventID = 0
		self.KnobResetEventID = 0
		self.KnobResetValue = 0
		self.KnobMode = 0
		self.KnobCenter = 0
		self.SliderEventID = 0
		self.Peak = 0
		self.Tag = 0
		self.SliderName = ""
		self.KnobName = ""
		self.LastValueIndex = 0
		self.ZPeak = False
		self.Dirty = False
		self.KnobHeld = False

# Main class for the DDX3216 control surface
class TDDX3216CU():
	def __init__(self):
		self.LastMsgLen =  0x37
		self.TempMsgT = ["", ""]
		self.LastTimeMsg = bytearray(10)

		self.Shift = False
		self.TempMsgDirty = False
		self.JogSource = 0
		self.TempMsgCount = 0
		self.SliderHoldCount = 0
		self.FirstTrack = 0
		self.FirstTrackT = [0, 0]
		self.ColT = [0 for x in range(9)]
		for x in range(0, 9):
			self.ColT[x] = TDDX3216Col()

		self.FreeCtrlT = [0 for x in range(DDX3216CU_nFreeTracks - 1 + 2)]  # 64+1 sliders
		self.Clicking = False
		self.Scrub = False
		self.Flip = False
		self.MeterMode = 0
		self.CurMeterMode = 0
		self.Page = 0
		self.SmoothSpeed = 0
		self.MeterMax = 0
		self.ActivityMax = 0

		self.DDX3216CU_PageNameT = ('Panning                                (press to reset)', 'Stereo separation                      (press to reset)',  'Sends for selected track              (press to enable)', 'Effects for selected track            (press to enable)', 'EQ for selected track                  (press to reset)',  'Lotsa free controls')
		self.DDX3216CU_MeterModeNameT = ('Horizontal meters mode', 'Vertical meters mode', 'Disabled meters mode')
		self.DDX3216CU_ExtenderPosT = ('left', 'right')

		self.FreeEventID = 400
		self.ArrowsStr = chr(0x7F) + chr(0x7E) + chr(0x32)
		self.AlphaTrack_SliderMax = round(13072 * 16000 / 12800)
		self.ExtenderPos = ExtenderLeft
		
	def OnInit(self):
		self.FirstTrackT[0] = 1
		self.FirstTrack = 0
		self.SmoothSpeed = 469
		self.Clicking = True

		device.setHasMeters()
		self.LastTimeMsg = bytearray(10)

		for m in range (0, len(self.FreeCtrlT)):
			self.FreeCtrlT[m] = 8192 # default free faders to center
		if device.isAssigned():
			# This SysEx message is an example from the original code.
			# Its function is unknown but it's included for completeness.
			device.midiOutSysex(bytes([0xF0, 0x00, 0x00, 0x66, 0x14, 0x0C, 1, 0xF7]))

		self.SetBackLight(2) # backlight timeout to 2 minutes
		self.UpdateClicking()
		self.UpdateMeterMode()

		self.SetPage(self.Page)
		self.OnSendTempMsg('Linked to ' + ui.getProgTitle() + ' (' + ui.getVersion() + ')', 2000);
		print('OnInit ready')

	def OnDeInit(self):
		if device.isAssigned():
			for m in range(0, 8):
				# This SysEx message is an example from the original code.
				# Its function is unknown but it's included for completeness.
				device.midiOutSysex(bytes([0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, m, 0, 0xF7]))

			if ui.isClosing():
				self.SendMsg(ui.getProgTitle() + ' session closed at ' + time.ctime(time.time()), 0)
			else:
				self.SendMsg('')

			self.SendMsg('', 1)
			self.SendTimeMsg('')
			self.SendAssignmentMsg('  ')

		print('OnDeInit ready')

	def OnDirtyMixerTrack(self, SetTrackNum):
		for m in range(0, len(self.ColT)):
			if (self.ColT[m].TrackNum == SetTrackNum) | (SetTrackNum == -1):
				self.ColT[m].Dirty = True

	def OnRefresh(self, flags):
		if flags & midi.HW_Dirty_Mixer_Sel:
			self.UpdateMixer_Sel()

		if flags & midi.HW_Dirty_Mixer_Display:
			self.UpdateTextDisplay()
			self.UpdateColT()

		if flags & midi.HW_Dirty_Mixer_Controls:
			for n in range(0, len(self.ColT)):
				if self.ColT[n].Dirty:
					self.UpdateCol(n)
	
	# Function to handle MIDI messages
	def OnMidiMsg(self, event):
		# Handle SysEx messages
		if event.midiId == midi.MIDI_SYSEX:
			self.handle_sysex_message(event)
			event.handled = True
			return

		# Handle other MIDI events
		if (event.midiId == midi.MIDI_CONTROLCHANGE):
			if (event.midiChan == 0):
				event.inEv = event.data2
				if event.inEv >= 0x40:
					event.outEv = -(event.inEv - 0x40)
				else:
					event.outEv = event.inEv

				if event.data1 == 0x3C:
					self.Jog(event)
					event.handled = True
					
				# knobs (CC 0x10 to 0x17)
				elif event.data1 in range(0x10, 0x18):
					r = utils.KnobAccelToRes2(event.outEv)
					Res = r * (1 / (40 * 2.5))
					if self.Page == DDX3216CUPage_Free:
						i = event.data1 - 0x10
						self.ColT[i].Peak = self.ActivityMax
						event.data1 = self.ColT[i].BaseEventID + int(self.ColT[i].KnobHeld)
						event.isIncrement = 1
						s = chr(0x7E + int(event.outEv < 0))
						self.OnSendTempMsg('Free knob ' + str(event.data1) + ': ' + s, 500)
						device.processMIDICC(event)
						device.hardwareRefreshMixerTrack(self.ColT[i].TrackNum)
					else:
						self.SetKnobValue(event.data1 - 0x10, event.outEv, Res)
						event.handled = True
				else:
					event.handled = False
			else:
				event.handled = False

		elif event.midiId == midi.MIDI_PITCHBEND: # faders
			if event.midiChan <= 8:
				event.inEv = event.data1 + (event.data2 << 7)
				event.outEv = (event.inEv << 16) // 16383
				event.inEv -= 0x2000

				if self.Page == DDX3216CUPage_Free:
					self.ColT[event.midiChan].Peak = self.ActivityMax
					self.FreeCtrlT[self.ColT[event.midiChan].TrackNum] = event.data1 + (event.data2 << 7)
					device.hardwareRefreshMixerTrack(self.ColT[event.midiChan].TrackNum)
					event.data1 = self.ColT[event.midiChan].BaseEventID + 7
					event.midiChan = 0
					event.midiChanEx = event.midiChanEx & (not 0xF)
					self.OnSendTempMsg('Free slider ' + str(event.data1) + ': ' + ui.getHintValue(event.outEv, midi.FromMIDI_Max), 500)
					device.processMIDICC(event)
				elif self.ColT[event.midiChan].SliderEventID >= 0:
					event.handled = True
					mixer.automateEvent(self.ColT[event.midiChan].SliderEventID, self.AlphaTrack_SliderToLevel(event.inEv + 0x2000), midi.REC_MIDIController, self.SmoothSpeed)
					n = mixer.getAutoSmoothEventValue(self.ColT[event.midiChan].SliderEventID)
					s = mixer.getEventIDValueString(self.ColT[event.midiChan].SliderEventID, n)
					if s != '':
						s = ': ' + s
					self.OnSendTempMsg(self.ColT[event.midiChan].SliderName + s, 500)

		elif (event.midiId == midi.MIDI_NOTEON) | (event.midiId == midi.MIDI_NOTEOFF):
			# Handle buttons
			if event.pmeFlags & midi.PME_System:
				if event.data1 == 0x54: # Shift
					self.Shift = event.data2 > 0
					device.directFeedback(event)
					event.handled = True
					
				elif event.data1 == 0x5D: # Stop
					transport.globalTransport(midi.FPT_Stop, int(event.data2 > 0) * 2, event.pmeFlags)
					event.handled = True
				elif event.data1 == 0x5E: # Play
					transport.globalTransport(midi.FPT_Play, int(event.data2 > 0) * 2, event.pmeFlags)
					event.handled = True
				elif event.data1 == 0x5F: # Record
					transport.globalTransport(midi.FPT_Record, int(event.data2 > 0) * 2, event.pmeFlags)
					event.handled = True
				elif event.data1 == 0x5B or event.data1 == 0x5C: # Rewind/Forward
					transport.globalTransport(midi.FPT_Rewind + (event.data1 - 0x5B), int(event.data2 > 0) * 2, event.pmeFlags)
					event.handled = True
				else:
					event.handled = False
			else:
				event.handled = False
		else:
			event.handled = False

	# Function to handle SysEx messages
	def handle_sysex_message(self, event):
		data = event.sysex
		
		# Check if it is a channel fader message (SysEx from user's code)
		if data[0:6] == [0x00, 0x20, 0x32, 0x20, 0x01, 0x00]:
			fader_number = data[6]
			fader_value = data[7]
			self.set_channel_fader(fader_number, fader_value)
		
		# Check if it is a master fader message (SysEx from user's code)
		elif data[0:8] == [0x00, 0x20, 0x32, 0x00, 0x20, 0x02, 0x40, 0x01]:
			# Combine the fader value bytes
			fader_value = (data[10] << 7) | data[11]
			self.set_master_fader(fader_value)
		
		# Check if it is a pan control message (SysEx from user's code)
		elif data[0:6] == [0x00, 0x20, 0x32, 0x20, 0x01, 0x00] and len(data) == 8:
			pan_number = data[6]
			pan_value = data[7]
			self.set_channel_pan(pan_number, pan_value)

	def set_channel_fader(self, fader_number, value):
		# Assuming channels 1-32 are mapped to faders 0-31
		if 0 <= fader_number < 32:
			channel = fader_number
			# Scale the value from 0-127 to 0-1
			volume = value / 127.0
			channels.setChannelVolume(channel, volume)

	def set_master_fader(self, value):
		# Convert the SysEx value to a volume level (0-1)
		# The value range is unclear from the code, assuming 0-127 for now.
		# This may need adjustment if the DDX3216's master fader SysEx uses a different range.
		volume = value / 127.0
		mixer.setTrackVolume(mixer.trackNumber("Master"), volume)

	def set_channel_pan(self, pan_number, value):
		# Assuming pan knobs 0-31 correspond to channels 1-32
		if 0 <= pan_number < 32:
			track_number = pan_number
			# Convert 0-127 to -1.0 to 1.0 range
			pan = (value - 64) / 64.0
			mixer.setTrackPan(track_number, pan)

	def set_channel_fader(self, fader_number, value):
		if 0 <= fader_number < 32:
			track_number = fader_number
			volume = value / 127.0
			mixer.setTrackVolume(track_number, volume)

	def OnSendTempMsg(self, Msg, Duration = 1000):
		if self.CurMeterMode == 0:
			self.TempMsgCount = (Duration // 48) + 1
		self.TempMsgT[1] = Msg
		self.TempMsgDirty = True

	def OnUpdateBeatIndicator(self, Value):
		SyncLEDMsg = [ midi.MIDI_NOTEON + (0x5E << 8), midi.MIDI_NOTEON + (0x5E << 8) + (0x7F << 16), midi.MIDI_NOTEON + (0x5E << 8) + (0x7F << 16)]
		if device.isAssigned():
			device.midiOutNewMsg(SyncLEDMsg[Value], 128)

	def OnUpdateMeters(self):
		pass

	def OnIdle(self):
		pass

	def OnWaitingForInput(self):
		pass

	# Dummy functions for a complete script
	def SetBackLight(self, value):
		pass

	def UpdateClicking(self):
		pass

	def UpdateMeterMode(self):
		pass

	def SetPage(self, page):
		self.Page = page
		self.OnDirtyMixerTrack(-1)

	def UpdateMixer_Sel(self):
		pass

	def UpdateTextDisplay(self):
		pass
	
	def UpdateColT(self):
		pass

	def UpdateCol(self, n):
		pass

	def SendMsg(self, Msg, Row = 0):
		pass

	def SendTimeMsg(self, Msg):
		pass

	def SendAssignmentMsg(self, Msg):
		pass

	def Jog(self, event):
		pass

	def SetKnobValue(self, n, value, res):
		pass

	def AlphaTrack_SliderToLevel(self, value):
		return value / 16383.0 # FL Studio MIDI values are 0-16383 for pitch bend

	def UpdateLEDs(self):
		pass
		
	def SetJogSource(self, source):
		pass

	def SetFirstTrack(self, track_number):
		pass

# Create the instance of the class that will handle all logic
DDX3216CU = TDDX3216CU()

# These are the functions that FL Studio will call automatically
def OnInit():
	DDX3216CU.OnInit()

def OnDeInit():
	DDX3216CU.OnDeInit()

def OnMidiMsg(event):
	DDX3216CU.OnMidiMsg(event)

def OnRefresh(flags):
	DDX3216CU.OnRefresh(flags)

def OnDirtyMixerTrack(SetTrackNum):
	DDX3216CU.OnDirtyMixerTrack(SetTrackNum)

def OnSendTempMsg(Msg, Duration = 1000):
	DDX3216CU.OnSendTempMsg(Msg, Duration)

def OnUpdateBeatIndicator(Value):
	DDX3216CU.OnUpdateBeatIndicator(Value)

def OnUpdateMeters():
	DDX3216CU.OnUpdateMeters()

def OnIdle():
	DDX3216CU.OnIdle()

def OnWaitingForInput():
	DDX3216CU.OnWaitingForInput()
