/*

  Copyright � Grame 2001

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

#ifndef __RadioHDFeedback__
#define __RadioHDFeedback__

#include <Windows.h>

#include "MidiShareAppl.h"
#include "MidiShareTasks.h"
#include "FeedbackProvider.h"

//_________________________________________________________________________________
typedef struct {
	char 			title[512];
	char 			from[1024];
	unsigned long	clockOffset;
	Boolean 		complete;
	Boolean 		end;
} CurrentPiece;

//_________________________________________________________________________________
class TChanDTask : public MidiShareDTask {	
	void 	Run	(long date, short refNum, long a1,long a2);
};
class TActivityDTask : public MidiShareDTask {
		void 	Run	(long date, short refNum, long a1,long a2);
};
class TClockDTask : public MidiShareDTask {
		void 	Run	(long date, short refNum, long a1,long a2);
};

//_________________________________________________________________________________
class RadioHDFeedback : public FeedbackProvider, public MidiShareAppl
{
	public:
		//------------------------------------------ constructeur / destructeur
				 RadioHDFeedback (HWND hDlg);
		virtual ~RadioHDFeedback (void);		

		//------------------------------------------ methodes
		void 	Lookup 		(strPtr name);
		void 	Connecting 	(char * name);
		void 	CheckLatency (char * name);
		void 	CloseReport (TMidiRemote * remote, CnxReportPtr report) {}
		void 	Complete 	(TMidiRemote * remote);
		void 	Remove 		(TMidiRemote * remote);
		void 	CnxRefused	(strPtr host, short reason) {}
		void 	Failed 		(short reason);
		void 	Clear 		();

		void 	DoIdle 		();
		void 	DoRedraw 	();
		void 	RcvAlarm 	(short refnum)	{}
		void 	ApplAlarm 	(short refnum, short srcRefNum, short changeCode);	
		void 	ChanTask 	();
		void 	ActivityTask ();
		void 	ClockTask (unsigned long currtime);
	
	private:
		enum	{ kMaxText = 4, kChanCount = 16, kVelStep = 22, 
					kChanUpdateRate = 100, kActivityUpdateRate = 100 };
		enum	{ kLeft, kCenter, kRight };
		
		char *	MakeText (char * dst, char * t1, char * t2);
		char *	Time2Text (char * dst, short sec, short min, short hrs);

		void 	Ev2Text 	(MidiEvPtr e, char * text);
		void 	StartClock 	();
		void 	ScanInput 	();
		void 	Clear 		(short line);
		void 	TextLine 	(short line, char * text, COLORREF color, short justify);
		void 	ActivityFlash (Boolean on);
		void 	ChannelActivity (short chan, short level);
		void 	DrawChans 	();
		void 	DrawLines 	();
		void 	CurrentReset();

		HWND	 	 	fWinH;
		HDC				fWinDC;
		HBRUSH white, red, yellow, hdcolor, green, blue, black;
		HPEN			phdcolor;
		TMidiRemote *	fServer;
		
		char *			fText[kMaxText];
		COLORREF  		fCurTextColor[kMaxText];
		short 			fJustify[kMaxText];
		Boolean			fActivityState;
		char 			fChanValues [kChanCount], fCurChanValues[kChanCount];
		TChanDTask		fChanTask;
		TActivityDTask	fActivityTask;
		TClockDTask		fClockTask;
		CurrentPiece	fCurrent;
};

#endif