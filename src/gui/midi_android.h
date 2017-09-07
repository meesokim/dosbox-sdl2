/*
 *  Copyright (C) 2002-2017  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

extern bool android_MidiDriver_init(void);
extern int* android_MidiDriver_config(int *config);
extern bool android_MidiDriver_write(unsigned char *buf, int length);
extern bool android_MidiDriver_setVolume(int volume);
extern bool android_MidiDriver_shotdown(void);

#define SEQ_MIDIPUTC    5

class MidiHandler_android: public MidiHandler {
private:
	int  device;
	Bit8u device_num;
	bool isOpen;
public:
	MidiHandler_android() : MidiHandler(),isOpen(false) {};
	const char * GetName(void) { return "android";};
	bool Open(const char * conf) {
		bool ret = android_MidiDriver_init();
		return ret;
	};
	void Close(void) {
		if (!isOpen) return;
		android_MidiDriver_shotdown();
	};
	void PlayMsg(Bit8u * msg) {
		Bit8u buf[128];Bitu pos=0;
		Bitu len=MIDI_evt_len[*msg];
		android_MidiDriver_write(msg,len);
	};
	void PlaySysex(Bit8u * sysex,Bitu len) {
		android_MidiDriver_write(sysex,len);	
	}
	void Pause(void) {
		android_MidiDriver_setVolume(0);
	}
	void Resume(void) {
		android_MidiDriver_setVolume(90);
	}
};

MidiHandler_android Midi_android;




