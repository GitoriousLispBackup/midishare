/*

  Copyright � Grame, Sony CSL-Paris 2001

  This library is free software; you can redistribute it and modify it under 
  the terms of the GNU Library General Public License as published by the 
  Free Software Foundation version 2 of the License, or any later version.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public License 
  for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

  Grame Research Laboratory, 9, rue du Garet 69001 Lyon - France
  grame@grame.fr
  
*/

#include <windows.h>
#include <mmsystem.h>
#include "msMMDriver.h"
#include "msMMInOut.h"
#include "msMMError.h"

SlotPtr gInSlots = 0, gOutSlots = 0;
extern LinearizeMthTbl	gLinMethods;
extern ParseMethodTbl	gParseTbl;
extern Status2TypeTbl	gTypeTbl;

static void CloseSlot (SlotPtr slot, Boolean inputSlot);
//_________________________________________________________
static SlotPtr NewSlot ()
{
	HLOCAL h = LocalAlloc (LMEM_FIXED, sizeof(Slot) + sizeof(HLOCAL));
	if (h) {
		HLOCAL * ptr = (HLOCAL *)LocalLock (h);
		if (ptr) {
			*ptr++ = h;
			return (SlotPtr)ptr;
		}
	}
	return 0;
}

//_________________________________________________________
static void FreeSlot (SlotPtr slot)
{
	HLOCAL * ptr = (HLOCAL *)slot;
	if (ptr) {
		HLOCAL h = *--ptr;
		if (h) {
			LocalUnlock (h);
			LocalFree (h);
		}
	}
}

//_________________________________________________________
static SlotPtr CreateSlot(short refNum, char *name, SlotDirection dir)
{
	SlotPtr slot = NewSlot ();
	if (!slot) return 0;
	slot->refNum = MidiAddSlot (refNum, name, dir);
	if (Slot(slot->refNum) < 0) {
		FreeSlot (slot);
		return 0;
	}
	MidiStreamInit (&slot->out, gLinMethods);
	MidiParseInit (&slot->in, gParseTbl, gTypeTbl);
	slot->mmHandle = 0;
	slot->next = 0;
	return slot;
}

//_________________________________________________________
void AddSlots (short refNum)
{
	UINT i, n = midiInGetNumDevs();
	MMRESULT res;
	for (i=0; i<n; i++) {
		MIDIINCAPS caps;
		res = midiInGetDevCaps (i, &caps, sizeof(caps));
		if (res == MMSYSERR_NOERROR) {
			SlotPtr slot = CreateSlot (refNum, caps.szPname, MidiInputSlot);
			if (slot) {
				slot->mmIndex = i;
				slot->next = gInSlots;
				gInSlots = slot;
			}
		}
	}
	n = midiOutGetNumDevs ();
	for (i=0; i<n; i++) {
		MIDIOUTCAPS caps;
		res = midiOutGetDevCaps (i, &caps, sizeof(caps));
		if (res == MMSYSERR_NOERROR) {
			SlotPtr slot = CreateSlot (refNum, caps.szPname, MidiOutputSlot);
			if (slot) {
				slot->mmIndex = i;
				slot->next = gOutSlots;
				gOutSlots = slot;
			}
		}
	}
}

//_________________________________________________________
void RemoveSlotList (SlotPtr slot, Boolean input)
{
	SlotPtr next;
	while (slot) {
		next = slot->next;
		if (slot->mmHandle)
			CloseSlot (slot, input);
		FreeSlot (slot);
		slot = next;
	}
}

//_________________________________________________________
void RemoveSlots (short refNum)
{
	RemoveSlotList (gInSlots, true);
	gInSlots = 0;
	RemoveSlotList (gOutSlots, true);
	gOutSlots = 0;
}

//_________________________________________________________
static SlotPtr FindSlot (SlotPtr list, short port)
{
	while (list) {
		if (Slot(list->refNum) == port)
			return list;
		list = list->next;
	}
	return 0;
}

//_________________________________________________________
void MSALARMAPI KOffTask( long date, short ref, long a1,long a2,long a3)
{
	MidiEvPtr e = (MidiEvPtr)a1;
	MS2MM ((SlotPtr)a2, e);
}

//_________________________________________________________
void MSALARMAPI RcvAlarm  (short refNum )
{
	SlotPtr slot = 0;
	MidiEvPtr e = MidiGetEv (refNum);
	while (e) {
		if (!slot || (Slot(slot->refNum) != Port(e)))
			slot = FindSlot(gOutSlots, Port(e));
		if (slot && slot->mmHandle) {
			e = MS2MM (slot, e);
			if (e)
				MidiTask (KOffTask, Date(e), refNum, (long)e, (long)slot, 0);
		}
		e = MidiGetEv (refNum);
	}
}

//_________________________________________________________
static Boolean IsSlotConnected (SlotRefNum sref)
{
	short i;
	for (i=0; i<256; i++) {
		if (MidiIsSlotConnected (i, sref))
			return true;
	}
	return false;
}

//_________________________________________________________
static void InitHeaders (SlotPtr slot)
{
	slot->header.lpData = slot->buff;
	slot->header.dwBufferLength = kBuffSize;
	slot->header.dwFlags = 0;
	slot->header.dwUser = 0;
}

//_________________________________________________________
static void OpenInputSlot (SlotPtr slot)
{
	HMIDIIN h;
	MMRESULT ret = midiInOpen (&h, slot->mmIndex, 
		(DWORD)MidiInProc, (DWORD)slot, CALLBACK_FUNCTION);
	if (ret == MMSYSERR_NOERROR) {
		slot->mmHandle = h;
		InitHeaders (slot);
		ret= midiInPrepareHeader (h, &slot->header, sizeof(slot->header));
		if (ret == MMSYSERR_NOERROR) {
			ret= midiInAddBuffer (h, &slot->header, sizeof(slot->header));
			if (ret == MMSYSERR_NOERROR) {
				ret= midiInStart (h);
				if (ret != MMSYSERR_NOERROR) {
					MMError ("midiInStart", ret, true);
					CloseSlot (slot, true);
				}
			}
			else {
				MMError ("midiInAddBuffer", ret, true);
				CloseSlot (slot, true);
			}
		}
		else {
			MMError ("midiInPrepareHeader", ret, true);
			midiInClose (h);
		}
	}
	else MMError ("midiInOpen", ret, true);
}

//_________________________________________________________
static void OpenOutputSlot (SlotPtr slot)
{
	HMIDIOUT h; 
	UINT ret = midiOutOpen(&h, slot->mmIndex, 0L, 0L, CALLBACK_NULL);
	if (ret == MMSYSERR_NOERROR) {
		MMRESULT res;
		slot->mmHandle = h;
		InitHeaders (slot);
		res = midiOutPrepareHeader(h, &slot->header, sizeof(slot->header)); 
		if (res != MMSYSERR_NOERROR)
			MMError ("midiOutPrepareHeader", res, false);
		else slot->header.dwUser = 1;
	}
	else MMError ("midiOutOpen", ret, false);
}

//_________________________________________________________
void OpenSlot (SlotPtr slot, Boolean inputSlot)
{
	if (inputSlot) OpenInputSlot (slot);
	else OpenOutputSlot (slot);
}

//_________________________________________________________
static void CloseSlot (SlotPtr slot, Boolean inputSlot)
{
	MMRESULT res;
	if (!slot->mmHandle) return;
	if (inputSlot) {
		HMIDIIN h = (HMIDIIN)slot->mmHandle;
		midiInStop (h);
		midiInReset(h);
		midiInUnprepareHeader(h, &slot->header, sizeof(slot->header)); 
		res= midiInClose (h);
	}
	else {
		HMIDIOUT h = (HMIDIOUT)slot->mmHandle;
		midiOutUnprepareHeader (h, &slot->header, sizeof(slot->header)); 
		res= midiOutClose (h);
	}
	slot->mmHandle = 0;
}

//_________________________________________________________
static void ScanSlotChanges (SlotPtr slot, Boolean input)
{
	while (slot) {
		if (IsSlotConnected (slot->refNum)) {
			if (!slot->mmHandle)
				OpenSlot (slot, input);
		}
		else if (slot->mmHandle)
			CloseSlot (slot, input);
		slot = slot->next;
	}
}

//_________________________________________________________
void MSALARMAPI ApplAlarm (short refNum, long code )
{
	short alarmCode = (short)code;
	short ref = (short)(code >> 16);
	switch (alarmCode) {        
		case MIDIChgSlotConnect:
			if (ref == refNum) {
				ScanSlotChanges (gInSlots, true);
				ScanSlotChanges (gOutSlots, false);
			}
			break;
	}
}