/*********************************************************************\
**
** ncubex.c -- PC-specific routines for ncube
**
**		ncube.c -- rotating N-D cube  (3 <= N <= 7)
**      Copyright (c) 1988,1989 Stephen Savitzky.  All rights reserved.
**		A product of HyperSpace Express
**		343 Leigh Av, San Jose, CA 95128
**
\*********************************************************************/

/*
** Includes
*/
 
#define XWS 1

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

#ifdef XWS
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
extern GC XCreateGC();
#endif


/*********************************************************************\
**
** Imports
**
\*********************************************************************/

extern	int	dim, buf, obuf, tail, maxtail;
extern	double vangle, viewx[1][1];
extern	void showAbout(), normalPersp(), stereoView();
extern  short rvFlag, xorFlag, logoFlag, stereoFlag, perspFlag, colorFlag,
    haveColorFlag, kFlag, pauseFlag, pEflag, interFlag, pTflag;
extern 	short maxColors, avoidColors, theColor;
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
** X-specific Initialization, Graphics, and Event Handling
**
**      The variables in this section are those involved in 
**      initialization and command-line parsing.
**
\*********************************************************************/

/* Parameters from command line */

char	   *display		= NULL;			/* cmd line: -display */
char	   *geom		= NULL;			/* cmd line: -geom */
int 		useRoot		= 0;			/* cmd line: -r */

/* 
** Global, comparatively static X state 
**		This stuff gets set up at the beginning and changes
**		rarely or never.
*/

Display	*dpy;
Window	 win;

GC	gcx, gc1, gc0;

int 	fg, bg;				/* foreground, background pixels */
unsigned int depth;			/* drawable depth */
Pixmap	logo1, logo2;


/* event handling stuff for main loop */

int	windowExposed = 0;		/* 0 if window invisible */
XEvent	xev;
char	keybf[256];			/* key translation buffer */


/*
** The X initialization code is based on that used in the "ico" demo,
** modified to correctly handle various command-line options.
**
** (For that matter, the transform routines came from there, too,
**  but have since been completely re-written several times.)
*/
void initGraphics()
{
    XSetWindowAttributes xswa;
    XGCValues	xgcv;
    int			gflags;
    XWMHints	wmhints;
    XSizeHints	szhints;
    
    if (!(dpy= XOpenDisplay(display))) {
        perror("Cannot open display\n");
        exit(-1);
    }
    
    if (rvFlag) {
        fg = BlackPixel(dpy, DefaultScreen(dpy));
        bg = WhitePixel(dpy, DefaultScreen(dpy));
    } else {
        fg = WhitePixel(dpy, DefaultScreen(dpy));
        bg = BlackPixel(dpy, DefaultScreen(dpy));
    }
    
    /* Set up window parameters, create and map window if necessary: */
    
    if (useRoot) {
        win = DefaultRootWindow(dpy);
        winX = 0;
        winY = 0;
        winW = DisplayWidth(dpy, DefaultScreen(dpy));
        winH = DisplayHeight(dpy, DefaultScreen(dpy));
        windowExposed = 1;
    } else {
        winW = 800;
        winH = 800;
        winX = (DisplayWidth(dpy, DefaultScreen(dpy)) - winW) >> 1;
        winY = (DisplayHeight(dpy, DefaultScreen(dpy)) - winH) >> 1;
        if (geom) {
            gflags = XParseGeometry(geom, &winX, &winY, &winW, &winH);
	    szhints.x = winX;
	    szhints.y = winY;
	    szhints.width = winW;
	    szhints.height= winH;
	    if (gflags & (XValue | YValue)) szhints.flags |= USPosition;
	    if (gflags & (WidthValue | HeightValue)) szhints.flags |= USSize;
	    if (gflags & XNegative)
		winX = szhints.x = DisplayWidth(dpy, DefaultScreen(dpy))
		    - winW + winX;
	    if (gflags & YNegative)
		winY = szhints.y = DisplayHeight(dpy, DefaultScreen(dpy))
		    - winH + winY;
	    
	}
	
        xswa.event_mask = 0;
        xswa.background_pixel = bg;
        xswa.border_pixel = fg;
        win = XCreateWindow(dpy, DefaultRootWindow(dpy), 
                            winX, winY, winW, winH, 0, 
                            DefaultDepth(dpy, DefaultScreen(dpy)), 
                            InputOutput,
			    DefaultVisual(dpy, DefaultScreen(dpy)),
                            CWEventMask | CWBackPixel | CWBorderPixel, &xswa);
        XChangeProperty(dpy, win, XA_WM_NAME, XA_STRING, 8, 
                        PropModeReplace, "ncube", 5);
	wmhints.input = 1;
	wmhints.icon_pixmap =
	    XCreatePixmapFromBitmapData(dpy, win, ncube_bits,
					ncube_width, ncube_height,
					fg, bg, 1);
	wmhints.flags = IconPixmapHint | InputHint;
	XSetWMHints(dpy, win, &wmhints);
	if (geom) XSetNormalHints(dpy, win, &szhints);
        XMapWindow(dpy, win);
    }
    
    XSelectInput(dpy, win,
                 ExposureMask | StructureNotifyMask
		 | KeyPressMask | VisibilityChangeMask
                 );
    
    /* Set up a graphics context: */
    
    gcx = XCreateGC(dpy, win, 0, NULL);
    XSetFunction(dpy, gcx, GXxor);
    XSetForeground(dpy, gcx, bg ^ fg);
    XSetBackground(dpy, gcx, 0);
    
    gc0 = XCreateGC(dpy, win, 0, NULL);
    XSetFunction(dpy, gc0, GXcopy);
    XSetForeground(dpy, gc0, bg);
    XSetBackground(dpy, gc0, fg);
    
    gc1 = XCreateGC(dpy, win, 0, NULL);
    XSetFunction(dpy, gc1, GXcopy);
    XSetForeground(dpy, gc1, fg);
    XSetBackground(dpy, gc1, bg);
    
    depth = DefaultDepth(dpy, DefaultScreen(dpy));
    if (depth > 2) {
	haveColorFlag = 1;
	maxColors = 128;
    }
    /* Display suitable logos */
    
    logo1 = XCreatePixmapFromBitmapData(dpy, win, ncube_bits,
					ncube_width, ncube_height,
					fg ^ bg, 0, depth);
    logo2 = XCreatePixmapFromBitmapData(dpy, win, hsexp_bits,
					hsexp_width, hsexp_height,
					fg ^ bg, 0, depth);
	
}

void clear()
{
    if (! rvFlag) {
	XSetForeground(dpy, gc0, bg);
	XSetBackground(dpy, gc0, fg);
	XSetForeground(dpy, gc1, fg);
	XSetBackground(dpy, gc1, bg);
	XSetWindowBackground(dpy, win, bg);
    } else {
	XSetForeground(dpy, gc0, fg);
	XSetBackground(dpy, gc0, bg);
	XSetForeground(dpy, gc1, bg);
	XSetBackground(dpy, gc1, fg);
	XSetWindowBackground(dpy, win, fg);
    }
    
    XClearWindow(dpy, win);
    if (logoFlag) {
	XCopyArea(dpy, logo2, win, gcx, 0, 0,
		  hsexp_width, hsexp_height,
		  winW - hsexp_width, winH - hsexp_height);
	XCopyArea(dpy, logo1, win, gcx, 0, 0,
		  ncube_width, ncube_height, 0, 0);
    }
}


showKeys()
{
    /* === */
}


void setColor(c)
    short c;
{
    XSetForeground(dpy, gc1, c);
    XSetBackground(dpy, gc1, bg);
}

void redraw(drawn, old, new, edges)
    short drawn;
    SLine *old, *new;
    int edges;
{
    if (colorFlag) {
	if (++theColor >= maxColors) theColor = avoidColors;
	setColor(theColor);
    }
    if (drawn) {
	XDrawSegments(dpy, win, xorFlag? gcx : gc0, old, edges);
    }
    XDrawSegments(dpy, win, xorFlag? gcx : gc1, new, edges);
}

void handleEvent()
{
    while (XPending(dpy) || pauseFlag || !windowExposed) {
        XNextEvent(dpy, &xev);
        switch (xev.type) {
	    
         case ConfigureNotify:
            winW = ((XConfigureEvent *)&xev) -> width;
            winH = ((XConfigureEvent *)&xev) -> height;
	    initScale();
            break;
	    
         case KeyPress:
	    if (XLookupString(&xev, keybf, sizeof(keybf), NULL, NULL)) {
		if (!interFlag) quit();
                switch(doKey(keybf[0])) {
		 case 2:
		    reInit();
		 case 1:
		    clear();
		    drawn = 0;
		    cycle(0, 1);
		}
            }
            break;
	    
	 case VisibilityNotify:
	    if (((XVisibilityEvent *)&xev) -> state == VisibilityFullyObscured)
		windowExposed = 0;
	    break;
	    
	 case UnmapNotify:
	    windowExposed = 0;
	    break;
	    
	 case Expose:
            if (((XExposeEvent *)&xev) -> count == 0) {
                windowExposed = 1;
		clear();
                drawn = 0;
		cycle(0, 1);
            }
        }
    }
}

void loop()
{
    for (;;) {
    	handleEvent();
        if (windowExposed) {
            if (pEflag || pTflag) {
                printf("\ncycle(%d)\n", cycles);
            }
	    if (!pauseFlag) {cycle(pEflag, 0); ++cycles;}
	    else cycle(0, 1);
	}
        XSync(dpy, 0);	/* Be polite: without this we monopolize the server */
    }
}

