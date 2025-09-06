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

# --- Classes to represent the control surface state ---
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

		self.DDX3216CU_PageNameT = ('Panning (press to reset)', 'Stereo separation (press to reset)',  'Sends for selected track (press to enable)', 'Effects for selected track (press to enable)', 'EQ for selected track (press to reset)',  'Lotsa free controls')
		self.DDX3216CU_MeterModeNameT = ('Horizontal meters mode', 'Vertical meters mode', 'Disabled meters mode')
		self.DDX3216CU_ExtenderPosT = ('left', 'right')

		self.FreeEventID = 400
		self.ArrowsStr = chr(0x7F) + chr(0x7E) + chr(0x32)
		self.AlphaTrack_SliderMax = round(13072 * 16000 / 12800)
		self.ExtenderPos = ExtenderLeft
		
	# --- Main API Functions ---
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
			# Example SysEx message from the original code
			device.midiOutSysex(bytes([0xF0, 0x00, 0x00, 0x66, 0x14, 0x0C, 1, 0xF7]))

		self.SetBackLight(2)
		self.UpdateClicking()
		self.UpdateMeterMode()

		self.SetPage(self.Page)
		self.OnSendTempMsg('Linked to ' + ui.getProgTitle() + ' (' + ui.getVersion() + ')', 2000);
		print('OnInit ready')

	def OnDeInit(self):
		if device.isAssigned():
			# Example SysEx message from the original code
			for m in range(0, 8):
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
	
	def OnMidiMsg(self, event):
		# Handle SysEx messages
		if event.midiId == midi.MIDI_SYSEX:
			self.handle_sysex_message(event)
			event.handled = True
			return

		# Handle other MIDI events
		ArrowStepT = [2, -2, -1, 1]
		CutCopyMsgT = ('Cut', 'Copy', 'Paste', 'Insert', 'Delete')

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
			if event.midiId == midi.MIDI_NOTEON:
				if event.data1 in [0x68, 0x69, 0x70]:
					self.SliderHoldCount += -1 + (int(event.data2 > 0) * 2)

				if (event.pmeFlags & midi.PME_System != 0):
					# F1..F8
					if self.Shift & (event.data1 in [0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D]):
						transport.globalTransport(midi.FPT_F1 - 0x36 + event.data1, int(event.data2 > 0) * 2, event.pmeFlags)
						event.data1 = 0xFF

					if event.data1 == 0x34: # display mode
						if event.data2 > 0:
							if self.Shift:
								self.ExtenderPos = abs(self.ExtenderPos - 1)
								self.FirstTrackT[self.FirstTrack] = 1
								self.SetPage(self.Page)
								self.OnSendTempMsg('Extender on ' + self.DDX3216CU_ExtenderPosT[self.ExtenderPos], 1500)
							else:
								self.MeterMode = (self.MeterMode + 1) % 3
								self.OnSendTempMsg(self.DDX3216CU_MeterModeNameT[self.MeterMode])
								self.UpdateMeterMode()
								device.dispatch(0, midi.MIDI_NOTEON + (event.data1 << 8) + (event.data2 << 16) )
					elif event.data1 == 0x35: # time format
						if event.data2 > 0:
							ui.setTimeDispMin()
					elif (event.data1 == 0x2E) | (event.data1 == 0x2F): # mixer bank
						if event.data2 > 0:
							self.SetFirstTrack(self.FirstTrackT[self.FirstTrack] - 8 + int(event.data1 == 0x2F) * 16)
							device.dispatch(0, midi.MIDI_NOTEON + (event.data1 << 8) + (event.data2 << 16))
					elif (event.data1 == 0x30) | (event.data1 == 0x31):
						if event.data2 > 0:
							self.SetFirstTrack(self.FirstTrackT[self.FirstTrack] - 1 + int(event.data1 == 0x31) * 2)
							device.dispatch(0, midi.MIDI_NOTEON + (event.data1 << 8) + (event.data2 << 16) )
					elif event.data1 == 0x32: # self.Flip
						if event.data2 > 0:
							self.Flip = not self.Flip
							device.dispatch(0, midi.MIDI_NOTEON + (event.data1 << 8) + (event.data2 << 16))
							self.UpdateColT()
							self.UpdateLEDs()
					elif event.data1 == 0x33: # smoothing
						if event.data2 > 0:
							self.SmoothSpeed = int(self.SmoothSpeed == 0) * 469
							self.UpdateLEDs()
							self.OnSendTempMsg('Control smoothing ' + OffOnStr[int(self.SmoothSpeed > 0)])
					elif event.data1 == 0x65: # self.Scrub
						if event.data2 > 0:
							self.Scrub = not self.Scrub
							self.UpdateLEDs()
							# jog sources
					elif event.data1 in [DDX3216CUNote_Undo, DDX3216CUNote_Pat, DDX3216CUNote_Mix, DDX3216CUNote_Chan, DDX3216CUNote_Tempo, DDX3216CUNote_Free1, DDX3216CUNote_Free2, DDX3216CUNote_Free3, DDX3216CUNote_Free4, DDX3216CUNote_Marker, DDX3216CUNote_Zoom, DDX3216CUNote_Move, DDX3216CUNote_Window]:
						self.SliderHoldCount +=  -1 + (int(event.data2 > 0) * 2)
						if event.data1 in [DDX3216CUNote_Zoom, DDX3216CUNote_Window]:
							device.directFeedback(event)
						if event.data2 == 0:
							if self.JogSource == event.data1:
								self.SetJogSource(0)
						else:
							self.SetJogSource(event.data1)
							event.outEv = 0
							self.Jog(event)
					elif event.data1 in [0x60, 0x61, 0x62, 0x63]: # arrows
						if self.JogSource == 0:
							transport.globalTransport(midi.FPT_Up - 0x60 + event.data1, int(event.data2 > 0) * 2, event.pmeFlags)
						else:
							if event.data2 > 0:
								event.inEv = ArrowStepT[event.data1 - 0x60]
								event.outEv = event.inEv
								self.Jog(event)

					elif event.data1 in [0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D]: # self.Page
						self.SliderHoldCount +=  -1 + (int(event.data2 > 0) * 2)
						if event.data2 > 0:
							n = event.data1 - 0x28
							self.OnSendTempMsg(self.DDX3216CU_PageNameT[n], 500)
							self.SetPage(n)
							device.dispatch(0, midi.MIDI_NOTEON + (event.data1 << 8) + (event.data2 << 16) )

					elif event.data1 == 0x54: # self.Shift
						self.Shift = event.data2 > 0
						device.directFeedback(event)
					elif event.data1 == 0x5D: # stop
						transport.globalTransport(midi.FPT_Stop, int(event.data2 > 0) * 2, event.pmeFlags)
					elif event.data1 == 0x5E: # play
						transport.globalTransport(midi.FPT_Play, int(event.data2 > 0) * 2, event.pmeFlags)
					elif event.data1 == 0x5F: # record
						transport.globalTransport(midi.FPT_Record, int(event.data2 > 0) * 2, event.pmeFlags)
					elif event.data1 == 0x5B or event.data1 == 0x5C: # Rewind/Forward
						transport.globalTransport(midi.FPT_Rewind + (event.data1 - 0x5B), int(event.data2 > 0) * 2, event.pmeFlags)
						event.handled = True
					else:
						event.handled = False
			else:
				event.handled = False
		else:
			event.handled = False

	# --- SysEx message handling for DDX3216-specific messages ---
	def handle_sysex_message(self, event):
		data = event.sysex
		
		# Check if it is a channel fader message
		# DDX3216 specific SysEx messages for faders, pan, etc.
		if len(data) >= 8 and data[0:6] == [0x00, 0x20, 0x32, 0x20, 0x01, 0x00]:
			fader_number = data[6]
			fader_value = data[7]
			self.set_channel_fader(fader_number, fader_value)
			
		# Check if it is a master fader message
		elif len(data) >= 12 and data[0:8] == [0x00, 0x20, 0x32, 0x00, 0x20, 0x02, 0x40, 0x01]:
			fader_value = (data[10] << 7) | data[11]
			self.set_master_fader(fader_value)
			
		# Check if it is a pan control message
		elif len(data) == 8 and data[0:6] == [0x00, 0x20, 0x32, 0x20, 0x01, 0x00]:
			pan_number = data[6]
			pan_value = data[7]
			self.set_channel_pan(pan_number, pan_value)

	def set_channel_fader(self, fader_number, value):
		if 0 <= fader_number < 32:
			track_number = fader_number
			volume = value / 127.0
			mixer.setTrackVolume(track_number, volume)

	def set_master_fader(self, value):
		volume = value / 127.0
		mixer.setTrackVolume(mixer.trackNumber("Master"), volume)

	def set_channel_pan(self, pan_number, value):
		if 0 <= pan_number < 32:
			track_number = pan_number
			pan = (value - 64) / 64.0
			mixer.setTrackPan(track_number, pan)

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

	# --- Helper Functions from the original script ---
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
		sysex = bytearray([0xF0, 0x00, 0x00, 0x66, 0x14, 0x12, (self.LastMsgLen + 1) * Row]) + bytearray(Msg.ljust(self.LastMsgLen + 1, ' '), 'utf-8')
		sysex.append(0xF7)
		device.midiOutSysex(bytes(sysex))

	def SendTimeMsg(self, Msg):
		TempMsg = bytearray(10)
		for n in range(0, len(Msg)):
			TempMsg[n] = ord(Msg[n])

		if device.isAssigned():
			for m in range(0, min(len(self.LastTimeMsg), len(TempMsg))):
				if self.LastTimeMsg[m] != TempMsg[m]:
					device.midiOutMsg(midi.MIDI_CONTROLCHANGE + ((0x49 - m) << 8) + ((TempMsg[m]) << 16))

		self.LastTimeMsg = TempMsg

	def SendAssignmentMsg(self, Msg):
		s_ansi = Msg + chr(0)
		if device.isAssigned():
			for m in range(1, 3):
				device.midiOutMsg(midi.MIDI_CONTROLCHANGE + ((0x4C - m) << 8) + (ord(s_ansi[m]) << 16))

	def TrackSel(self, Index, Step):
		Index = 2 - Index
		device.baseTrackSelect(Index, Step)
		if Index == 0:
			s = channels.getChannelName(channels.channelNumber())
			self.OnSendTempMsg(self.ArrowsStr + 'Channel: ' + s, 500);
		elif Index == 1:
			self.OnSendTempMsg(self.ArrowsStr + 'Mixer track: ' + mixer.getTrackName(mixer.trackNumber()), 500);
		elif Index == 2:
			s = patterns.getPatternName(patterns.patternNumber())
			self.OnSendTempMsg(self.ArrowsStr + 'Pattern: ' + s, 500);

	def Jog(self, event):
		if self.JogSource == 0:
			transport.globalTransport(midi.FPT_Jog + int(self.Shift ^ self.Scrub), event.outEv, event.pmeFlags)
		elif self.JogSource == DDX3216CUNote_Move:
			transport.globalTransport(midi.FPT_MoveJog, event.outEv, event.pmeFlags)
		elif self.JogSource == DDX3216CUNote_Marker:
			if self.Shift:
				s = 'Marker selection'
			else:
				s = 'Marker jump'
			if event.outEv != 0:
				if transport.globalTransport(midi.FPT_MarkerJumpJog + int(self.Shift), event.outEv, event.pmeFlags) == midi.GT_Global:
					s = ui.getHintMsg()
			self.OnSendTempMsg(self.ArrowsStr + s, 500)
		elif self.JogSource == DDX3216CUNote_Undo:
			if event.outEv == 0:
				s = 'Undo history'
			elif transport.globalTransport(midi.FPT_UndoJog, event.outEv, event.pmeFlags) == midi.GT_Global:
				s = ui.GetHintMsg()
			self.OnSendTempMsg(self.ArrowsStr + s + ' (level ' + general.getUndoLevelHint() + ')', 500)
		elif self.JogSource == DDX3216CUNote_Zoom:
			if event.outEv != 0:
				transport.globalTransport(midi.FPT_HZoomJog + int(self.Shift), event.outEv, event.pmeFlags)
		elif self.JogSource == DDX3216CUNote_Window:
			if event.outEv != 0:
				transport.globalTransport(midi.FPT_WindowJog, event.outEv, event.pmeFlags)
			s = ui.getFocusedFormCaption()
			if s != "":
				self.OnSendTempMsg(self.ArrowsStr + 'Current window: ' + s, 500)
		elif (self.JogSource == DDX3216CUNote_Pat) | (self.JogSource == DDX3216CUNote_Mix) | (self.JogSource == DDX3216CUNote_Chan):
			self.TrackSel(self.JogSource - DDX3216CUNote_Pat, event.outEv)
		elif self.JogSource == DDX3216CUNote_Tempo:
			if event.outEv != 0:
				channels.processRECEvent(midi.REC_Tempo, channels.incEventValue(midi.REC_Tempo, event.outEv, midi.EKRes), midi.PME_RECFlagsT[int(event.pmeFlags & midi.PME_LiveInput != 0)] - midi.REC_FromMIDI)
			self.OnSendTempMsg(self.ArrowsStr + 'Tempo: ' + mixer.getEventIDValueString(midi.REC_Tempo, mixer.getCurrentTempo()), 500)
		elif self.JogSource in [DDX3216CUNote_Free1, DDX3216CUNote_Free2, DDX3216CUNote_Free3, DDX3216CUNote_Free4]:
			event.data1 = 390 + self.JogSource - DDX3216CUNote_Free1
			if event.outEv != 0:
				event.isIncrement = 1
				s = chr(0x7E + int(event.outEv < 0))
				self.OnSendTempMsg(self.ArrowsStr + 'Free jog ' + str(event.data1) + ': ' + s, 500)
				device.processMIDICC(event)
				return
			else:
				self.OnSendTempMsg(self.ArrowsStr + 'Free jog ' + str(event.data1), 500)
	
	def SetKnobValue(self, n, value, res):
		pass

	def AlphaTrack_SliderToLevel(self, Value, Max = midi.FromMIDI_Max):
		return min(round(Value / self.AlphaTrack_SliderMax * Max), Max)

	def UpdateLEDs(self):
		pass
		
	def SetJogSource(self, source):
		pass

	def SetFirstTrack(self, track_number):
		pass

	def UpdateTextDisplay(self):
		s1 = ''
		for m in range(0, len(self.ColT) - 1):
			s = ''
			if self.Page == DDX3216CUPage_Free:
				s = '  ' + utils.Zeros(self.ColT[m].TrackNum + 1, 2, ' ')
			else:
				s = mixer.getTrackName(self.ColT[m].TrackNum, 6)
			for n in range(1, 7 - len(s) + 1):
				s = s + ' '
			s1 = s1 + s
		self.TempMsgT[0] = s1
		if self.CurMeterMode == 0:
			if self.TempMsgCount == 0:
				self.UpdateTempMsg()
		else:
			self.SendMsg(s1, 1)

	def GetSplitMarks(self):
		s2 = '';
		for m in range(0, len(self.ColT) - 1):
			s2 = s2 + '      .'
		return s2

	def UpdateMeterMode(self):
		if self.Page == DDX3216CUPage_Free:
			self.CurMeterMode = 1
		else:
			self.CurMeterMode = self.MeterMode
		if device.isAssigned():
			for m in range(0, len(self.ColT) - 1):
				device.midiOutMsg(midi.MIDI_CHANAFTERTOUCH + (0xF << 8) + (m << 12))
			for m in range (0, 8):
				device.midiOutSysex(bytes([0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, m, 0, 0xF7]))
		if self.CurMeterMode > 0:
			self.TempMsgCount = -1
		else:
			self.TempMsgCount = 500 // 48 + 1
		self.MeterMax = 0xD + int(self.CurMeterMode == 1)
		self.ActivityMax = 0xD - int(self.CurMeterMode == 1) * 6
		if self.CurMeterMode == 0:
			self.SendMsg(self.GetSplitMarks(), 1)
		else:
			self.UpdateTextDisplay()
		if device.isAssigned():
			device.midiOutSysex(bytes([0xF0, 0x00, 0x00, 0x66, 0x14, 0x21, int(self.CurMeterMode > 0), 0xF7]))
			if self.CurMeterMode == 2:
				n = 1
			else:
				n = 1 + 2;
			for m in range(0, 8):
				device.midiOutSysex(bytes([0xF0, 0x00, 0x00, 0x66, 0x14, 0x20, m, n, 0xF7]))
				
	def AlphaTrack_LevelToSlider(self, Value, Max = midi.FromMIDI_Max):
		return round(Value / Max * self.AlphaTrack_SliderMax)

# --- FL Studio Entry Points ---
DDX3216CU = TDDX3216CU()

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
