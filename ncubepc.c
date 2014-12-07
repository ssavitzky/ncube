/*********************************************************************\
**
** ncubepc.c -- PC-specific routines for ncube
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
** PC-specific Initialization, Graphics, and Event Handling
**
\*********************************************************************/

#ifdef PC

static void initGraphics()
{
	struct videoconfig vc;
	short isgraph;
	int vmode;

	/*
	** Select best available graphics mode.
	** 		This code is based on an original by Abe Savitzky
	** 		of Silvermine Resources, Inc., as used in RD7
	*/
	_setvideomode (_DEFAULTMODE);	/* see what we have */
	_getvideoconfig(&vc);

	isgraph = 1;					/* only MDA is NO */
	switch (vc.adapter) {
		case _MDPA :
			isgraph = 0;
			vmode = _TEXTMONO;
			break;
		case _CGA :
			vmode =	_MRES4COLOR;
			break;
		case _HGC :
			vmode = _HERCMONO;
			break;
		case _EGA :
			if (vc.memory > 64) vmode = _ERESCOLOR;
			else {
				vmode = _HRES16COLOR;
				haveColorFlag = 1; maxColors = 16; avoidColors=0;
			}
			break;
		case _VGA :
		case _MCGA :
			vmode =	_MRES256COLOR;
			haveColorFlag = 1; maxColors = 256; avoidColors=0;
			break;
	}
	switch (vc.mode) {
		case _TEXTBW80 :
		case _TEXTBW40 :
		case _TEXTMONO :	/*??? as */
		case _HERCMONO :
			break;
		case _ERESNOCOLOR :
			vmode = _ERESNOCOLOR;
	}

	if (!isgraph) {
	    prbox(banner);       /* Greet the user, then complain */
		fprintf(stderr, 
"Sorry, but you don't seem to have a graphics board!\n\
If you have a Hercules board, you have to run MSHERC\n");
		exit(1);
	}
	_setvideomode(vmode);  /*with luck, this is it */
	_getvideoconfig(&vc);
	winW = vc.numxpixels;
	winH = vc.numypixels;
	clear();			   /* get the logos up now */
}

/*
** This is a kludge to squirt out an X bitmap on a PC.
** It assumes that the bitmap's width is a multiple of 8.
*/
static void putbmap(p, w, h, x, y)
	char *p;
	short x, y, w, h;
{
	register int r, c;			/* row, column in bitmap */
	register char b;			/* current byte */

	for (r = 0; r < h; ++r, ++y)
		for (c = 0; c < w; ) {
			b =	*p++;
			if (b & 1) _setpixel(x + c, y);	++c; b >>= 1;
			if (b & 1) _setpixel(x + c, y);	++c; b >>= 1;
			if (b & 1) _setpixel(x + c, y);	++c; b >>= 1;
			if (b & 1) _setpixel(x + c, y);	++c; b >>= 1;
			if (b & 1) _setpixel(x + c, y);	++c; b >>= 1;
			if (b & 1) _setpixel(x + c, y);	++c; b >>= 1;
			if (b & 1) _setpixel(x + c, y);	++c; b >>= 1;
			if (b & 1) _setpixel(x + c, y);	++c;
		}
}

static void clear()
{
	int x, y;
	char b, *p;

	_clearscreen(_GCLEARSCREEN);
	if (logoFlag) {
		putbmap(hsexp_bits, hsexp_width, hsexp_height,
				winW - hsexp_width, winH - hsexp_height);
		putbmap(ncube_bits, ncube_width, ncube_height, 0, 0);
	}
}

showKeys()
{
	/* === */
}

static void setColor(c)
	short c;
{
	/* === */
}

static void redraw(drawn, old, new, edges)
	short drawn;
	SLine *old, *new;
	int edges;
{
	register int i;

	if (drawn) {
		_setcolor(0);
		for (i = 0; i < edges; ++i) {
			_moveto(old[i].x1, old[i].y1);
			_lineto(old[i].x2, old[i].y2);
		}
		_setcolor(1);
	}
	for (i = 0; i < edges; ++i) {
		_moveto(new[i].x1, new[i].y1);
		_lineto(new[i].x2, new[i].y2);
	}
}

static void loop()
{
    for ( ; ; ) {
        while (kbhit()) {
			if (!interFlag) quit();
			switch(doKey(getch())) {
			 case 2:
				reInit();
			 case 1:
				clear();
			}
        }
        if (!pauseFlag) {cycle(pEflag, 0); ++cycles;}
    }
    _setvideomode(_DEFAULTMODE);
}
#endif /* PC */


/
