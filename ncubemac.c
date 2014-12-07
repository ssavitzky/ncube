/*********************************************************************\
**
** ncubemac.c -- Macintosh-specific routines for ncube
**
**		ncube.c -- rotating N-D cube  (3 <= N <= 7)
**      Copyright (c) 1988,1989 Stephen Savitzky.  All rights reserved.
**		A product of HyperSpace Express
**		343 Leigh Av, San Jose, CA 95128
**
** NOTE
**		Good Macish programming practice would be to put all strings
**		into resources, for convenient customization.  We don't, for
**		the following reasons:
**
**		1.	Resource files are specific to the Mac, and hard to port.
**			We try to keep non-portable constructs to a minimum, especially
**			for things like the help strings, which are needed on ALL
**			versions.
**
**		2.	This program can be customized by editing the source, making
**			the use of ResEdit and friends unnecessary.
**
**		3.	Uncontrolled customization by people who don't know what
**			they're doing is dangerous, especially for shareware, 
**			because it means that users have no way of knowing whether
**			what they're getting is the original version.
**
\*********************************************************************/

#ifdef THINK_C
#define MAC 1
#if (sizeof(double) > 10)
#define _MC68881_
#else
/* #undef USE_FLOATS */
#endif
#include <storage.h>
#include <color.h>

/* Lightspeed C braindamage */
#define memcpy(d,s,n)   movmem((s),(d),(n))
#endif

/*
** Includes
*/
 
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <math.h>

#ifdef PC
#include <graph.h>
#include <malloc.h>
#include <string.h>
#else
#include <strings.h>
#endif

#ifdef unix
extern char *calloc();
#endif

/*********************************************************************\
**
** Imports
**
\*********************************************************************/

extern	WindowPtr myWindow;
extern	int	dim, buf, obuf, tail, maxtail;
extern	double vangle, viewx[1][1];
extern	void showAbout(), normalPersp(), stereoView();
extern  short rvFlag, xorFlag, logoFlag, stereoFlag, perspFlag, colorFlag,
			  haveColorFlag, kFlag, pauseFlag, pEflag;
extern 	short maxColors, avoidColors, theColor, red_Color, blue_Color;
extern  int doCmd();

/*
** Window parameters
*/ 
extern int winX, winY, winW, winH;			/* Window parameters */
extern int xoff, yoff;						/* offset of origin (center of window) */

extern short drawn;
extern int cycles;

typedef struct SPoint {					/* screen point */
	short x, y;
} SPoint;

typedef struct SLine  {					/* screen line */
	short x1, y1, x2, y2;
} SLine;

void clear();							/* forward */

extern char ncube_bits[];
#define ncube_width 96
#define ncube_height 48

extern char hsexp_bits[];
#define hsexp_width 96
#define hsexp_height 48


/*********************************************************************\
**
** Menu Handling
**
**		The basic organization of this code came from the "Bullseye"
**		demo that came with Lightspeed C.  Little remains but the outline.
**
\*********************************************************************/


/*********************************************************************\
**
** Menu Description Structures 
**
\*********************************************************************/

typedef struct MenuDscr {	/* Description of a menu */
	MenuHandle 	handle;		/* the menu handle */
	char		*name;		/* its name in the menu line (Pascal string) */
	char		**cmds;		/* the commands to execute (1-origin indexing) */
	char		*items;		/* the items that comprise it (Pascal string) */
} MenuDscr;


/*********************************************************************\
**
** Menu Descriptions
**
**		? starting a command string means it's disabled.
**
\*********************************************************************/

static char *appleCmds[] = {
	"-a"
};

static char *fileCmds[] = {
	"?new", "?open", "?close", "?save", "?dummy", "-q"
};

static char *editCmds[] = {
	"?undo", "?dummy", "?cut", "?copy", "?paste", "?clear"
};

static char *dimCmds[] = {
	"-d3", "-d4", "-d5", "-d6", "-d7"
};
static char *tailCmds[] = {
	"-n-1", "-n0", "-n1", "-n2", "-n3", "-n4", "-n5", "-n6", "-n7", "-n8", "-n9", "-n10",
};
static char *optCmds[] = {
	"-rv", "-x", "-l", "-p", "-s", "-sc", "-c",
};	

static MenuDscr menus[] = {
#define appleID 1
	{0,	"\p\024",		appleCmds,	"\pAbout Ncube...;(-"	},
#define fileID	2
	{0, "\pFile",		fileCmds,	"\pNew/N;Open/O;Close/W;Save/S;(-;Quit/Q" },
#define openItem 	2
#define closeItem 	3
#define quitItem	6
#define editID	3
	{0, "\pEdit",		editCmds,	"\pUndo/Z;(-;Cut/X;Copy/C;Paste/V;Clear" },
#define dimsID	4
    {0, "\pDimensions",	dimCmds,	"\p3/3;4/4;5/5;6/6;7/7" },
#define tailID	5
	{0, "\pTail",		tailCmds,	"\pAll;0;1;2;3;4;5;6;7;8;9;10" },
#define optsID	6
	{0, "\pOptions",	optCmds,	"\pInvert;Xor;Logos;Perspective;Stereo;Red-Blue;Color" },
#define rvItem 		1
#define xorItem 	2
#define logoItem 	3
#define perspItem 	4
#define stereoItem 	5
#define r_gItem 6
#define colorItem	7
};	

#define N_MENUS 6
#define appleMenu	(menus[0].handle)
#define fileMenu	(menus[1].handle)
#define editMenu	(menus[2].handle)
#define dimsMenu	(menus[3].handle)
#define tailMenu	(menus[4].handle)
#define optsMenu	(menus[5].handle)

/****
 * SetUpMenus()
 *
 *	Set up the menus. Normally, we’d use a resource file, but
 *	for this example we’ll supply “hardwired” strings.
 *
 ****/

SetUpMenus()
{
	int i;
	
	for (i = 0; i < N_MENUS; ++i) {
		InsertMenu(menus[i].handle = NewMenu(i + 1, menus[i].name), 0);
	}
	DrawMenuBar();
	for (i = 0; i < N_MENUS; ++i) {
		AppendMenu(menus[i].handle, menus[i].items);
	}
	AddResMenu(appleMenu, 'DRVR');
}


/****
 *  AdjustMenus()
 *
 *	Enable or disable the items in the Edit menu if a DA window
 *	comes up or goes away. Our application doesn't do anything with
 *	the Edit menu.
 *
 ****/

static enable(menu, item, ok)
	Handle menu;
	int item, ok;
{
	if (ok)		EnableItem(menu, item);
	else		DisableItem(menu, item);
}


AdjustMenus()
{
	register WindowPeek wp = (WindowPeek) FrontWindow();
	short kind = wp ? wp->windowKind : 0;
	Boolean DA = kind < 0;
	
	enable(editMenu, 1, DA);
	enable(editMenu, 3, DA);
	enable(editMenu, 4, DA);
	enable(editMenu, 5, DA);
	enable(editMenu, 6, DA);
	
	enable(fileMenu, openItem, !((WindowPeek) myWindow)->visible);
	enable(fileMenu, closeItem, DA || ((WindowPeek) myWindow)->visible);

	enable(optsMenu, colorItem, (Boolean) (haveColorFlag && maxColors > avoidColors));

	CheckItem(dimsMenu, dim - 2, true);
	CheckItem(tailMenu, tail + 2, true);
	
	CheckItem(optsMenu, rvItem, (Boolean) rvFlag);
	CheckItem(optsMenu, xorItem, (Boolean) xorFlag);
	CheckItem(optsMenu, logoItem, (Boolean) logoFlag);
	CheckItem(optsMenu, perspItem, (Boolean) perspFlag);
	CheckItem(optsMenu, stereoItem, (Boolean) stereoFlag);
	CheckItem(optsMenu, r_gItem, stereoFlag == 2);
	CheckItem(optsMenu, colorItem, (Boolean) colorFlag);
}


/*****
 * HandleMenu(mSelect)
 *
 *	Handle the menu selection. mSelect is what MenuSelect() and
 *	MenuKey() return: the high word is the menu ID, the low word
 *	is the menu item
 *
 *****/

int HandleMenu (mSelect)
	long	mSelect;
{
	int			menuID = HiWord(mSelect);
	int			menuItem = LoWord(mSelect);
	Str255		name;
	GrafPtr		savePort;
	WindowPeek	frontWindow;
	char		*argv[2];
	int 		res;

	res = 0;	
	switch (menuID) {
	  case	appleID:
	  	switch (menuItem) {
	  	 case 1:
	  	 	showAbout();
	  	 	break;
	  	 	
	  	 default:
			GetPort(&savePort);
			GetItem(appleMenu, menuItem, name);
			OpenDeskAcc(name);
			SetPort(savePort);
		}
		break;
	
	  case	fileID:
		switch (menuItem) {
		  case	openItem:
			ShowWindow(myWindow);
			SelectWindow(myWindow);
			break;
  				  			
		  case	closeItem:
			if ((frontWindow = (WindowPeek) FrontWindow()) == 0L) break;			  
			if (frontWindow->windowKind < 0)
				CloseDeskAcc(frontWindow->windowKind);
			else if (frontWindow = (WindowPeek) myWindow)
			  	HideWindow(myWindow);
  			break;
  				  	
		  case	quitItem:
			ExitToShell();
			break;
		  }
		break;
  				
	  case	editID:
		if (!SystemEdit(menuItem-1))
		  	SysBeep(5);
		break;
		
	  case	dimsID:
		CheckItem(dimsMenu, dim - 2, false);
		goto doit;
	 
	  case  tailID:
	  	CheckItem(tailMenu, tail + 2, false);
		/* fall through */
			  	
	  case	optsID:
      doit:
	  	argv[0] = 0;
	  	argv[1] = menus[menuID - 1].cmds[menuItem - 1];
	  	res = doCmd(2, argv, 0);
		InvalRect(&myWindow->portRect);
	  }
	  return(res);
}
/* end HandleMenu */

/*********************************************************************\
**
** Macintosh-specific Initialization, Graphics, and Event Handling
**
**		The initialization and event-handling code originally came from
**		various examples included with Lightspeed C 3.0, but have been
**		hacked over considerably since.
**
\*********************************************************************/

SysEnvRec	theEnv;
WindowPtr	myWindow;
Rect		dragRect;
Rect		windowBounds;
int 		quitFlg = 0;
int			aboutFlg = 0;				/* 1 when About box is visible */

WindowPtr	aboutWindow;
Rect		aboutBounds;
PicHandle	aboutHdl;

Rect		hsxRect, hsxAboutRect;		/* Where the bitmaps go in the windows */
Rect		logoRect;					/* Logo is always top left */

BitMap		logoBits = {ncube_bits, ncube_width / 8, 
						{0, 0, ncube_height, ncube_width}};
BitMap		hsxBits  = {hsexp_bits, hsexp_width / 8, 
						{0, 0, hsexp_height, hsexp_width}};

GDHandle		theDevice;
CTabHandle		colors;
PaletteHandle	thePalette;

/*
** checkColor()
**		return true if we REALLY have color
**		Also, set up maxColors
*/
int checkColor()
{
	SysEnvirons(1, &theEnv);
	if (theEnv.hasColorQD) {		
		theDevice = GetMainDevice();
		maxColors = (* (* (*theDevice) -> gdPMap) -> pmTable)  -> ctSize + 1;
		return (maxColors > 2);
	} else {
		return(0);
	}
}


/*
** SetUpWindows()
**
**		Create the application's main window, and open it.
**		Also locate resources and set up the About box in case we need it.
*/
SetUpWindows()
{
	static RGBColor xred = {-1, 0, 0};
	static RGBColor xblue = {0, 0, -1};
	
	/*
	** Start by finding out if we have color or not
	*/
	dragRect = screenBits.bounds;
	windowBounds = screenBits.bounds;
	windowBounds.top += 40;
	
	SysEnvirons(1, &theEnv);
	if (theEnv.hasColorQD) {		
		myWindow = (WindowPtr) NewCWindow(0L, &windowBounds, "\pNcube", true,
							 documentProc, -1L, true, 0);
		haveColorFlag = 1; avoidColors = 2;
		theDevice = GetMainDevice();
		maxColors = (* (* (*theDevice) -> gdPMap) -> pmTable)  -> ctSize + 1;
		thePalette = NewPalette(maxColors, NULL, pmExplicit, 0);
		SetPalette(myWindow, thePalette, TRUE);
		CTab2Palette((* (* theDevice) -> gdPMap) -> pmTable, 
					  thePalette, pmExplicit, 0);
		ActivatePalette(myWindow);
		red_Color = Color2Index(&xred);
		blue_Color = Color2Index(&xblue);
	} else {
		myWindow = NewWindow(0L, &windowBounds, "\pNcube", true,
							 documentProc, -1L, true, 0);
	}
	SetPort(myWindow);
	winW = myWindow -> portRect.right - myWindow -> portRect.left;
	winH = myWindow -> portRect.bottom - myWindow -> portRect.top;

	logoRect = logoBits.bounds;
	hsxRect.right = winW; hsxRect.bottom = winH;
	hsxRect.left = hsxRect.right - logoRect.right;
	hsxRect.top = hsxRect.bottom - logoRect.bottom;

	flip(ncube_bits, ncube_width * ncube_height/8);
	flip(hsexp_bits, ncube_width * ncube_height/8);

	aboutHdl= (PicHandle) GetNamedResource('PICT', "\pAbout");
	aboutBounds = (*aboutHdl) -> picFrame;
	OffsetRect(&aboutBounds, 75, 75);
	aboutWindow = NewWindow(0L, &aboutBounds, "\pAbout Ncube", false,
							dBoxProc, -1L, false, 0);

	hsxAboutRect.right = aboutBounds.right - aboutBounds.left;
	hsxAboutRect.bottom = aboutBounds.bottom - aboutBounds.top;
	hsxAboutRect.left = hsxAboutRect.right - hsxBits.bounds.right;
	hsxAboutRect.top = hsxAboutRect.bottom - hsxBits.bounds.bottom;
	
	PenMode(xorFlag? patXor : patCopy);
}

void initGraphics()
{
	MaxApplZone();
	
	InitGraf(&thePort);
	InitFonts();
	FlushEvents(everyEvent, 0);
	InitWindows();
	InitMenus();
	TEInit();
	InitDialogs(0L);
	InitCursor();
	
	SetUpWindows();
	SetUpMenus();
}

void showAbout()
{
	aboutFlg = 1;
	ShowWindow(aboutWindow);
	SelectWindow(aboutWindow);
	InvalRect(&aboutWindow->portRect);
}

void drawAbout()
{
	GrafPtr	savePort;

	GetPort(&savePort);
	SetPort(aboutWindow);
	EraseRect(&aboutWindow -> portRect);
	DrawPicture(aboutHdl, &aboutWindow -> portRect);
	CopyBits(&logoBits, &aboutWindow -> portBits, &logoRect, &logoRect,
			 srcCopy, 0L);
	CopyBits(&hsxBits, &aboutWindow -> portBits, &logoRect, &hsxAboutRect,
			 srcCopy, 0L);
	SetPort(savePort);
}

void MyGrowWindow(w, p)			/* from miniedit example */
	WindowPtr w;
	Point p;
{
	GrafPtr	savePort;
	long	theResult;
	Rect 	r;
	
	GetPort(&savePort);
	SetPort(w);

	SetRect(&r, 96, 48, screenBits.bounds.right, screenBits.bounds.bottom);
	theResult = GrowWindow( w, p, &r );
	if (theResult == 0) return;
	SizeWindow( w, LoWord(theResult), HiWord(theResult), 1);
	
	winW = w -> portRect.right - w -> portRect.left;
	winH = w -> portRect.bottom - w -> portRect.top;
	initScale();
	hsxRect.right = winW; hsxRect.bottom = winH;
	hsxRect.left = hsxRect.right - logoRect.right;
	hsxRect.top = hsxRect.bottom - logoRect.bottom;

	InvalRect(&w->portRect);
	SetPort(savePort);
}


void HandleMouseDown(theEvent)
	EventRecord	*theEvent;
{
	WindowPtr	theWindow;
	int			windowCode = FindWindow (theEvent->where, &theWindow);

	if (aboutFlg) {
		HideWindow(aboutWindow);
		aboutFlg = 0;
		return;
	}	
    switch (windowCode) {
	  case inSysWindow: 
	    SystemClick (theEvent, theWindow);
	    break;
	    
	  case inMenuBar:
	  	AdjustMenus();
	    switch (HandleMenu(MenuSelect(theEvent->where))) {
		 case 2:
			reInit();
		 case 1:
			clear();
		}
	    break;
	    
	  case inDrag:
	  	if (theWindow == myWindow)
	  		DragWindow(myWindow, theEvent->where, &dragRect);
	  	break;
	  	  
	  case inGrow:
		if (theWindow == myWindow)
			MyGrowWindow(theWindow, theEvent->where);
		break;
		
	  case inContent:
	  	if (theWindow != FrontWindow())
	  	    SelectWindow(myWindow);
	  	else
	  	    InvalRect(&myWindow->portRect);
	  	break;
	  	
	  case inGoAway:
	  	if (theWindow == myWindow && TrackGoAway(myWindow, theEvent->where))
		  	HideWindow(myWindow);
		  	exit(0);
	  	break;
    }
}


/*
** handleEvent()
**
**		The main event dispatcher.  Returns true if event handled.
*/

int handleEvent()
{
	int			ok, res;
	EventRecord	theEvent;
	WindowPtr	win;

	HiliteMenu(0);
	SystemTask();		/* Handle desk accessories */
	
	if (ok = GetNextEvent (everyEvent, &theEvent))
		switch (theEvent.what) {
		 case mouseDown:
			HandleMouseDown(&theEvent);
			break;
			
		 case keyDown: 
		 case autoKey:
		    if ((theEvent.modifiers & cmdKey) != 0) {
		      	AdjustMenus();
			 	switch (HandleMenu(
			 		MenuKey((char) (theEvent.message & charCodeMask)))) {
				 case 2:
					reInit();
				 case 1:
					clear();
				}
			} else {
				if (doKey(theEvent.message & 0xFF)  && !kFlag) quitFlg = 1;
			}
			break;
			
		 case updateEvt:
		 	win = (WindowPtr) theEvent.message;
		 	checkColor();
			BeginUpdate(win);
			if (win == myWindow) {
				clear();
				drawn = 0;
				cycle(0, 1);
			} else if (win == aboutWindow) {
				drawAbout();
			}
			EndUpdate(win);
		    break;
		    
		 case activateEvt:
			InvalRect(&myWindow->portRect);
			break;
	    }
	return(ok);
}

showKeys()
{
	/* === */
}

void clear()
{
	if (rvFlag) {
		ForeColor(whiteColor);
		BackColor(blackColor);
	} else {
		ForeColor(blackColor);
		BackColor(whiteColor);
	}
	EraseRect(&myWindow -> portRect);
	if (logoFlag) {
		CopyBits(&logoBits, &myWindow -> portBits, &logoRect, &logoRect,
				 srcXor, 0L);
		CopyBits(&hsxBits, &myWindow -> portBits, &logoRect, &hsxRect,
				 srcXor, 0L);
	}
	PenMode(xorFlag? patXor : patCopy);
}

void setColor(c)
	short c;
{
	PmForeColor(c);
}

void redraw(drawn, old, new, edges)
	short drawn;
	SLine *old, *new;
	int edges;
{
	register int i;
	register int rg = 0;
	
	if (colorFlag) {
		if (stereoFlag != 2) {
			/* rainbow */
			if (++theColor >= maxColors) theColor = avoidColors;
			setColor(theColor);
		} else {
			/* red-blue stereo */
			rg = 1;
			setColor(blue_Color);
		}
	}
	if (drawn) {
		if (!xorFlag) PenMode(notPatCopy);
		for (i = 0; i < edges; ++i) {
			MoveTo(old[i].x1, old[i].y1);
			LineTo(old[i].x2, old[i].y2);
		}
		if (!xorFlag) PenMode(patCopy);
	}
	for (i = 0; i < edges; ++i) {
		if (rg && i == edges / 2) setColor(red_Color);
		MoveTo(new[i].x1, new[i].y1);
		LineTo(new[i].x2, new[i].y2);
	}
}

void loop()
{
    for ( ; ; ) {
        while (handleEvent()) if (quitFlg) return;
        if (!pauseFlag) {cycle(pEflag, 0); ++cycles;}
		else cycle(0, 1);
    }
}

