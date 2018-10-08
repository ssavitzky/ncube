/*********************************************************************\
**
** ncube.c -- display a rotating N-Dimensional Cube
**            https://github.com/ssavitzky/ncube
**
\*********************************************************************/

char		  /* O P E N   S O U R C E */

 version   [] = "HyperSpace Express Ncube version 1.8 (1991 Sep 20)",
 copyright [] = "Copyright (c) 1991 Stephen Savitzky.  MIT License.",
 company   [] = "A product of HyperSpace Express",
 address   [] = "somewhere in hyperspace";

/*********************************************************************\
**
** Working Notes:
**
**	Color usage:  None / R-B stereo / Rainbow
**
** Bugs / Severe Deficiencies (To be corrected in the next version?)
**
**	Need to display typein during interactive mode. (showKeys())
**	The perspective transforms still aren't really right.
**	  Should use tangent to bounding sphere: sqrt(dim)/sin(theta).
**
**	X version should use the Toolkit.
**	X version cycles even if iconic or obscured
**	X version needs color
**
**	PC version needs color
**	PC Kidproof mode is buggy.
**
** Missing Features:  (- = low priority)
**
**	It's just about time to add the animated logo.
**	There should be accompanying documentation on 4-d geometry.
**
**	Need to save/restore figures, transforms, and ALL options
**
**	Integer arithmetic should be a runtime option (and the default).
**	Draw/erase by segment (optionally?) (smoother).
**	Stereo mode should interleave LRLR instead of LLLLRRRR (smoother).
**	The "cycles/second" clock keeps ticking when paused or invisible.
**
**	All versions should display an "about" box on request.
**	About box should be constructed from scratch when needed.
**	About box should show cycles, c/s, options, etc.
**	-Need multiple help screens in the about box. -h<topic>
**	Want a status line with cycles, c/s.
**
** 	All versions should have menus
**
**	Mac version needs to produce PICT files on demand (copy cmd).
**
**	PC version should scale X and Y for non-square pixels.
**	PC version needs handleEvent() like the others.
**	  (This would permit unifying the main loop again.)
**	-PC version should coexist with window systems (someday).
**
** Development Environment:
**
**	PC		Microsoft C 5.0 graphics library
**	Unix		X Window System Version 11 Release 2 or higher.
**	Macintosh	Symantec Think (Lightspeed) C 3.0
**
** Execution Environment:
**
**	PC		XT and AT clones with Hercules, EGA, and CGA. 
**			Hercules requires Microsoft's MSHERC driver.
**
**	Unix		Sun, Apollo, Mac (A/UX)
**
**	Macintosh	II, Plus, SE
**
** Theory of Operation:
**
**      The program maintains two transform matrices:
**          F   the initial Figure transformation
**          V   the current Viewing transformation
**      plus a LIST of intermediate transforms Xi, any number of which
**		may be associated with an increment Yi.
**
**      Initially, each point in the cube is transformed by F,
**      which is ignored thereafter.
**
**      At each "cycle", we do Xi = Yi * Xi for each transform Xi
**		in the transform list having an incremental transform Yi.
**
**      Then for each point p, the corresponding screen point s
**      is given by s = p * X1 * X2 * ... * Xn * V.  For speed,
**	the product transform is converted from floats to scaled
**	integers (type SForm) before transforming the points.
**
**      If transforms are given in the argument list, they are
**      concatenated by multiplying so that the leftmost is applied
**		first, as expected.
**
** Version History:		(Dates approximate before v1.4)
**
**	1.0	early 1988	PC (Hercules) and Unix only
**	1.1	later 1988	Microsoft C 5.0 library
**	1.2	1988 Aug	First Mac version
**	1.3	1988 Sep	Mac "About" box.  Limited shareware release.
**	1.4	1988 Dec 10	Stereo, -p option, transform list
**	1.4i	1988 Dec 13 integer arithmetic, -p, -s w\ no arg=toggle
**	1.4j	1988 Dec 15	Xor in X; fixes to -p, -s.
**	1.5	1989 Apr 15	integer Xforms for rendering; color;
**				  mac menus convert to commands.
**	1.6	1989 Dec	R-G stereo; Unix & PC color
**		
\*********************************************************************/

/*
** System-dependent conditionals
**
**	Undefine USE_FLOATS to do all transforms with scaled integers.
**	This is faster but results in serious roundoff error.
**
**	The standard compromise is to use doubles for transforms, but
**	convert to integers when we do the view transform.
**
**	On machines with floating-point hardware the cycle time is
**	mainly determined by line-drawing speed, anyway.
*/ 

#define USE_FLOATS

#ifdef unix
#define XWS 1
#endif

#ifdef MSDOS
#define PC 1
#endif

#ifdef THINK_C
#define MAC 1
#define signal(x,y)			/* MAC doesn't grok signals */
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

/*
** Imports
*/
#ifndef MAC
extern long time();
extern long random();
#endif 


/*********************************************************************\
**
** Types, Constants, and Macros
**
**	Transforms are indexed X[row][column]
**      We use homogeneous coordinates in  N + 1 dimensions.
**      Points are of the form:     [1, x, y, z, t, u, v, w]
**
**	Putting h first simplifies loops that only involve h, x, and y.
**	It also makes the h-axis a constant, rather than the dimension.
**
\*********************************************************************/

#define H 0
#define X 1
#define Y 2
#define Z 3

#ifdef MAC
#define MAXDIM 7			/* Braindead MAC 32K data segment! */
#else 
#define MAXDIM 9
#endif
#define MAXVEC (MAXDIM + 1)

/* Scaled Integer Coordinates and Transforms */ 

typedef long    ICoord;			/* integer coordinate */
#define IOne	16384L			/* scale factor for ICoord */
typedef ICoord  IForm [MAXVEC][MAXVEC];	/* integer transform */
typedef ICoord  SForm [MAXVEC][3];	/* world-to-screen xform */
#define ICScale(x) ((ICoord)((x)*IOne))	/* scale double to ICoord */
#define IDScale(x) ((x) / IOne)		/* descale after a multiply */

/* (usually) Floating Coordinates and Transforms */

#ifdef USE_FLOATS
typedef double Coord;			/* coordinate type: long or double */
#define One 1.0				/* scaled 1.0 */
#define CScale(x) (x)			/* scale double to Coord */
#define DScale(x) (x)			/* descale after a multiply */
#define IScale(x) ICScale(x)		/* scale Coord to ICoord */
#else
typedef ICoord Coord;			/* coordinate type: long or double */
#define One IOne			/* scaled 1.0 */
#define CScale(x) ((Coord)((x) * One))	/* scale double to Coord */
#define DScale(x) ((x) / One)		/* descale after a multiply */
#define IScale(x) (x)			/* scale Coord to ICoord */
#endif /* USE_FLOATS */

typedef Coord Vec[MAXVEC];		/* homogeneous vector */
typedef Coord XForm[MAXVEC][MAXVEC];	/* homogeneous transform */
					/* indexed xf[row][column] */

/* Figure and Screen components */

typedef struct FEdge {			/* figure edge (vertex pair) */
    short v1, v2;
} FEdge;
 
typedef struct SPoint {			/* screen point */
    short x, y;
} SPoint;

typedef struct SLine  {			/* screen line */
    short x1, y1, x2, y2;
} SLine;

/* Assorted useful constants */

#define DEGREES (3.14159265 / 180)
 
#define MAXPTS  (1<<MAXDIM)		/* max # of points displayed */
#define MAXEDGE (MAXDIM * MAXPTS / 2)


#ifndef MAC
#define MAXTRANS 10			/* max # of transforms */
#define MAXTAIL 100			/* max # of ghost images */
#else
#define MAXTRANS  5
#define MAXTAIL 50			/* MAC is smaller */
#endif
 
 
 
/*********************************************************************\
**
** Variables
**
\*********************************************************************/
 
/*
** Window parameters
*/ 
int winX, winY, winW, winH;		/* Window parameters */
int xoff, yoff;				/* offset of origin (window center) */
 
/* 
** Transforms
*/
XForm	xfl[MAXTRANS];			/* transform list */
XForm	xil[MAXTRANS];			/* incremental transform list */
short	xif[MAXTRANS];			/* 1 if incremental */
short   xfs = 0;			/* number of transforms in list */
short	curxf;				/* current transform for commands */

XForm   rotx;				/* incremental rotation transform */
XForm   curx;				/* current cumulative transform */
XForm   figx;				/* initial figure transform */

XForm   tx, tx1, xtl[2];		/* temporary transforms */

XForm	viewx;				/* view (perspective) transform */
XForm	viewl, viewr;			/* left & right views for stereo */

double	radius;				/* maximum radius of figure */
double	sangle;				/* stereo half-angle */
double	vangle;				/* view half-angle */
Coord   Sscale;				/* screen scale factor */
 

/*
** The figure to be drawn
*/ 
char *figure;				/* figure name */
static char figbuf[256];		/* buffer for figure name */
int dim = 4;				/* # dimensions: default 4 */

short   vertices;			/* # of vertices */
Vec    *vertex;				/* Vertex list */
short   edges;				/* # of edges */
FEdge  *edge;				/* Edge list */


/*
** Buffer for on-screen edges
*/ 
int buf = 1;				/* double-buffer index */
int obuf = 0;				/* other-buffer index */
int tail = 1;				/* number of ghost images */
int maxtail = MAXTAIL;			/* maximum tail length */
SLine *sEdge[MAXTAIL];			/* screen images of the edges */

/*
** Stuff used in the main loop
*/
#define pEscreen 1			/* print screen edges */
#define pEfigure 2			/* print figure edges */

short 
    kFlag		= 0,		/* Kidproof flag */
    pauseFlag		= 0,		/* paused: don't transform */
    pTflag 		= 0,		/* print Transforms */
    pEflag 		= 0,		/* print Edges */
    logoFlag		= 1,		/* display logos */
    stereoFlag		= 0,		/* stereo images 2 = red/blue */
    interFlag		= 1,		/* interactive mode */
    perspFlag		= 1,		/* perspective */
    rvFlag 		= 1,		/* invert the screen */
    xorFlag		= 0,		/* draw with Xor */
    colorFlag		= 0;		/* draw in color */

short drawn = 0;			/* TRUE if fig. drawn & needs erase */
short haveColorFlag = 0;		/* TRUE if color is available */
short maxColors = 2;			/* max number of colors available */
short avoidColors = 2;			/* # colors used by other stuff */
short theColor = 0;			/* current color */
short red_Color, blue_Color;		/* red and blue for stereo */


char keys[256];				/* Keyboard input buffer */
int  curKey;				/* current key index */
int  cycles = 0;			/* Cycle counter */
long theTime;				/* the time, for cycles/second */
int nargs;


/*********************************************************************\
**
** Machine-Dependent Initialization, Graphics, and Event Handling
**		There is a separate module for each platform.
**
**	initGraphics()		Initialize the graphical universe
**
**	clear()			Clear the window.
**				Display logos if necessary.
**
**	setColor(c)		Set current foreground color
**	showKeys()		Display the keyboard buffer
**
**	redraw(drawn, old, new, edges)
**				Erase the old figure if drawn is true
**				then draw the new one.
**				Called from cycle().
**
**	loop()			The main cycle/draw/check-event loop.
**				Calls cycle(), which contains the machine-
**				independent transform operations.
**
\*********************************************************************/

extern void initGraphics();
extern void clear();
extern void setColor();		/* (c) */
extern void showKeys();
extern void redraw();		/* (drawn, old, new, edges) */
extern void loop();


/*********************************************************************\
**
** Banner (About Box) Text
**
\*********************************************************************/
 
char *banner[] = {
"",
"         ___        ____   ___  ",
"  /| /  /     /  /  /__/  /__   ",
" / |/  /___  /__/ _/__/  /___   ",
"",
"",
version,
company,
address,
"",
copyright,
"Not to be included in any copyrighted anthology or collection",
"without the express written permission of the author.",
"",
"If you like this program, I'd be grateful for a $5 contribution.",
"$20 will get you a disk with sources and more documentation.",
"(Specify MS-DOS or Macintosh disk format.)",
"",
"Type ESC to exit the program.",
"For documentation use 'ncube -h' (piped to a printer or file to save it)",
0
};
 

/*********************************************************************\
**
** Help Text
**
\*********************************************************************/
 
char *ohelp[] = {
"",
"         ___        ____   ___  ",
"  /| /  /     /  /  /__/  /__   ",
" / |/  /___  /__/ _/__/  /___   ",
"",
"",
version,
company,
address,
"",
copyright,
"Not to be included in any copyrighted anthology or collection",
"without the express written permission of the author.",
"",
"If you like this program, I'd be grateful for a $5 contribution.",
"$20 will get you a disk with sources and more documentation.",
"(Specify MS-DOS or Macintosh disk format.)",
"",
"command line:                                                        ",
"    Options:  (N = any integer) (leading '-' optional)               ",
"      -dN        N dimensions; 3 <= N <= 7; default 4                ",
"      -c         toggle color mode                                   ",
"      -h         help -- print this information, then exit           ",
"      -i         toggle interactive mode (default OFF on PC, else ON)",
"      -k         kidproof -- require Ctrl-C to exit                  ",
"      -l         suppress logos                                      ",
"      -nN        number of 'ghost' images (max 50, def 1, -1 = all)  ",
"      -pe        print edge (n-dimensional) coordinates each cycle   ",
"      -ps        print screen (2-dimensional) edge coords each cycle ",
"      -pt        print transform matrices at beginning (to stdout)   ",
"      -rv        reverse video                                       ",
"      -x         exclusive or for drawing (not available on PC)      ",
"    Transform selection: (0 <= N <= 9 is xform #, default 0)         ",
"       ,N        operate on next viewing transform                   ",
"       +N        operate on next incremental transform (the default) ",
"       .         clear current transform                             ",
"       =         operate on the figure to be drawn                   ",
"    Transform options:  (A, B in {x,y,z,t,u,v,w}; F = any float)     ",
"       ABF       rotate by F degrees/cycle in the AB plane           ",
"       AF        translate by F along the A axis                     ",
"       AsF       scale A axis by F                                   ",
"       ApF       perspective from distance F along A axis            ",
"       pF        perspective angle of F degrees                      ",
"       rr        random rotations about all axes (default)           ",
"       sF        stereo angle F (F optional; s0 means monocular)     ",
"       sc        stereo using red-blue glasses                       ",
"",
" Interactive mode: type any of the above commands followed by RET.   ",
"                   Ctl-S to stop (pause), Ctl-Q to continue.         ",
#ifdef XWS
"",
"    X Window System options:                                         ",
"      -g geom    specify geometry (default 800x800)                  ",
"      -d display specify display                                     ",
"      -r         root window                                         ",
#endif
"",
" To Exit:                                                            ",
"       Interactive mode: type ESC or 'Q' + RETURN                    ",
#ifdef PC
"       Kidproof mode:    type Ctrl-C or Ctrl-Break                   ",
"       Otherwise:        type any key (which stays in input buffer)  ",
#else
"       Kidproof mode:    type Ctrl-C                                 ",
"       Otherwise:        type any key                                ",
#endif
"",
0
};
 
/*********************************************************************\
**
** prbox(lines) -- Print text centered in a box the size of the screen
**
**  (currently only useful on PC and Unix)
**
\*********************************************************************/
 
#define BOXWID 77
#define BOXHIG 21

#ifdef PC
#define UL 201
#define HF 205
#define UR 187
#define VF 186
#define LL 200
#define LR 188
#else
#define UL '/'
#define HF '='
#define UR '\\'
#define VF '|'
#define LL '\\'
#define LR '/'
#endif
 
prline(s, left, fill, right)
    char *s, left, right, fill;
{
    register int i, j, len;

    putc(left, stdout);
    len = strlen(s);
    j = (BOXWID - len)/2;
    for (i = 0; i < j; ++i) putc(fill, stdout);
    fputs(s, stdout);
    j = BOXWID - j - len;
    for (i = 0; i < j; ++i) putc(fill, stdout);
    putc(right, stdout);
    putc('\n', stdout);
}
 
prbox(p)
    char **p;
{
    int i;

    prline(version, UL, HF, UR);
    for (i = 0; *p; ++p, ++i)
        prline(*p, VF, ' ', VF);
    for ( ; i < BOXHIG; ++i)
        prline("", VF, ' ', VF);
    prline(version, LL, HF, LR);
}
 
 
/*********************************************************************\
**
** Matrix Arithmetic
**
**      This uses homogeneous coordinates in  N + 1 dimensions.
**      Points are of the form:     [1, x, y, z, t, u, v, w]
**
\*********************************************************************/
 
 
/*
 * Cpy(xfs, xfd)
 *
 *      Copy a transform matrix
 *      For speed, this is normally defined as a macro using memcpy.
 */
#define Cpy(xfs,xfd) memcpy((xfd), (xfs), sizeof(XForm))
#ifndef Cpy
Cpy(xfs, xfd)
    XForm xfs, xfd;
{
    register int i, j;
    
    for (i = 0; i <= dim; ++i)
        for (j = 0; j <= dim; ++j)
            xfd[i][j] = xfs[i][j];
}
#endif

/*
 * Cat(xfl, xfr, xfd)
 *      Concatenate two transform matrices
 */
void Cat(l, r, d)
    XForm l, r, d;
{
    register int i, j, k;
    register Coord f;
    
    for (i = 0; i <= dim; ++i)
        for (j = 0; j <= dim; ++j) {
            for (f = 0.0, k = 0; k <= dim; ++k) 
		f += DScale(l[i][k] * r[k][j]);
            d[i][j] = f;
        }
}

/*
 * CCat(xfl, xfr, xfd)
 *      Concatenate two transform matrices,
 *	Convert result to an IForm
 */
void CCat(l, r, d)
    XForm l, r;
    IForm d;
{
    register int i, j, k;
    register Coord f;
    
    for (i = 0; i <= dim; ++i)
        for (j = 0; j <= dim; ++j) {
            for (f = 0.0, k = 0; k <= dim; ++k) 
		f += DScale(l[i][k] * r[k][j]);
            d[i][j] = IScale(f);
        }
}

/*
 * SCat(xfl, xfr, xfd)
 *      Concatenate two transform matrices
 *	Convert result to an SForm
 */
void SCat(l, r, d)
    XForm l, r;
    SForm d;
{
    register int i, j, k;
    register Coord f;
    
    for (i = 0; i <= dim; ++i)
	for (j = 0; j <= 2; ++j) {
            for (f = 0.0, k = 0; k <= dim; ++k) 
		f += DScale(l[i][k] * r[k][j]);
            d[i][j] = IScale(f);
        }
}

/*
 * Ident(xform) 
 *      make an identity transform
 */
void Ident(xf)
    XForm xf;
{
    register int i, j;
    
    for (i = 0; i <= dim; ++i)
        for (j = 0; j <= dim; ++j)
            xf[i][j] = i == j ? One : 0;
}

/*
 * Rot(axis1, axis2, angle, xf)
 *      
 *      Concatenate xf (on the right) with a rotation (on the left)
 *      in plane axis1, axis2 of angle radians
 */
void Rot(a1, a2, a, xf)
    int a1, a2;
    double a;
    XForm xf;
{
    Coord s, c;
    
    Ident(tx);
    s = CScale(sin(a));
    c = CScale(cos(a));
    
    tx[a1][a1] = tx[a2][a2] = c;
    tx[a1][a2] = s;
    tx[a2][a1] = -s;
    
    Cpy(xf, tx1);
    Cat(tx, tx1, xf);
}

/*
 * Trans(axis, t, xf)
 *
 *      Concatenate xf with a translation of t along axis.
 *	The computation is xf = T * xf
 */
Trans(axis, t, xf)
    int axis;
    double t;
    XForm xf;
{
    Ident(tx);
    tx[H][axis] = CScale(t);
    
    Cpy(xf, tx1);
    Cat(tx, tx1, xf);
}

/*
 * Scale(axis, s, xf)
 *
 *      Concatenate xf with a scale of s along axis.
 */
Scale(axis, s, xf)
    int axis;
    double s;
    XForm xf;
{
    int i;
    
    Ident(tx);
    tx[axis][axis] = CScale(s);
    
    Cpy(xf, tx1);
    Cat(tx, tx1, xf);
}

/*
 * Persp(axis, a, d, xf)
 *
 *      POST-multiply xf with a perspective transform along axis
 *      where a = angle of view, d = distance.
 */
Persp(axis, a, d, xf)
    int axis;
    double a, d;
    XForm xf;
{
    int i;
    Coord s, c, q;
    
    s = CScale(sin(a/2.0));
    c = CScale(cos(a/2.0));
    q = CScale(s/2.0);			/* assume far = -near */
    
    Ident(tx);				/* I'm not sure how this generalizes */
    tx[X][X] = tx[Y][Y] = c;
    tx[axis][axis] = q;
    tx[axis][H]    = s;
    tx[dim][axis]  = -q * d;
    tx[H][H]       = CScale(-d);
    
    Cpy(xf, tx1);
    Cat(tx1, tx, xf);
}

 
/*********************************************************************\
**
** Viewing Transforms
**
\*********************************************************************/
 
/*
 * View(v, xf, s)
 *      Transform an n-point v into a screen point s using transform xf.
 *      We rely on the fact that v[H] = 1 for an untransformed n-vector.
 *      Moreover, we only compute transforms for v[X] and v[Y]
 */
void View(v, xf, s)
    Vec v;
    XForm xf;
    SPoint *s;
{
    register int i, j;
    register Coord f, sf;
    
    /* 
     * compute the scale factor (v[H]) 
     */
    for (f = xf[H][H], j = 1; j <= dim; ++j) 
	f += DScale(v[j] * xf[j][H]);
    sf = Sscale * One / f;
    
    /*
     * Transform the X and Y coordinates
     */
    for (f = xf[H][X], j = 1; j <= dim; ++j) 
	f += DScale(v[j] * xf[j][X]);
    s -> x = DScale(f * sf) + xoff;
    for (f = xf[H][Y], j = 1; j <= dim; ++j)
	f += DScale(v[j] * xf[j][Y]);
    s -> y = DScale(f * sf) + yoff;
}
 
/*
 * SView(v, xf, s)
 *      Transform an n-point v into a screen point s using SForm xf.
 *      We rely on the fact that v[H] = 1 for an untransformed n-vector.
 *      Moreover, we only compute transforms for v[x] and v[y]
 */
void SView(v, xf, s)
    Vec v;
    SForm xf;
    SPoint *s;
{
    register int i, j;
    register ICoord f, sf;
 
    /* 
     * compute the scale factor (v[H]) 
     */
    for (f = xf[H][H], j = 1; j <= dim; ++j) 
	f += IDScale(IScale(v[j]) * xf[j][H]);
    sf = Sscale * IOne / f;
 
    /*
     * Transform the X and Y coordinates
     */
    for (f = xf[H][X], j = 1; j <= dim; ++j) 
	f += IDScale(IScale(v[j]) * xf[j][X]);
    s -> x = IDScale(f * sf) + xoff;
    for (f = xf[H][Y], j = 1; j <= dim; ++j)
	f += IDScale(IScale(v[j]) * xf[j][Y]);
    s -> y = IDScale(f * sf) + yoff;
}
 
/*
 * Transform(v, xf, p)
 *      Transform an n-point v into an n-point p using transform matrix xf.
 *      It is permissible to have v == p.
 *      We assume that v[0,0] == 1, and scale the result so that
 *      the same condition holds for p.
 */
void Transform(v, xf, p)
    Vec v;
    XForm xf;
    Vec p;
{
    register int i, j;
    register Coord f;
    Vec xv;

    for (i = 0; i <= dim; ++i) {
        for (f = xf[H][i], j = 1; j <= dim; ++j) 
	    f += DScale(v[j] * xf[j][i]);
        xv[i] = f;
    }
 
    f = xv[H];
    for (i = 0; i <= dim; ++i) p[i] = CScale(xv[i]) / f;
}
 

/*********************************************************************\
**
** Rotating the hypercube:  cycle(print, drawOnly)
**
**      if print is true, print edges
**      if drawOnly is true, don't update the transform, just draw.
**      if drawn (global) is true, erase the previous image.
**
\*********************************************************************/

static SPoint *sVert;
static SPoint *rVert;
static SForm  stx;

cycle(print, drawOnly)
    int print, drawOnly;
{
    register int i, v;
    short e1, e2;
    XForm *lastx, *nextx;
    
    if (drawOnly) goto doDraw;
    
    /*
     * Concatenate all transforms in the list, 
     *		Use the list of temporaries to avoid copying.
     * 		End up with lastx pointing at the result.
     *		This is a no-op if xfs == 1.
     */
    
    for (lastx = xfl, nextx = xtl, i = 1; i < xfs; ++i) {
	Cat(lastx, xfl[i], nextx);
	lastx = nextx;
	nextx = xtl + (i & 1);
    }
    
    /*
     * Now tack on the view transform and print it if requested,
     * then transform the vertex list.
     */
    if (stereoFlag) SCat(lastx, viewl, stx); else SCat(lastx, viewx, stx);
    if (pTflag) xprint(tx);
    
    for (v = 0; v < vertices; ++v) {
        SView(vertex[v], stx, &sVert[v]);
    }
    
    /*
     * Now do it again if stereo
     */
    if (stereoFlag) {
	short sxoff;
	sxoff = xoff/2;
	SCat (lastx, viewr, stx);
	for (v = 0; v < vertices; ++v) {
	    SView(vertex[v], stx, &rVert[v]);
	    if (stereoFlag != 2) {
		rVert[v].x += sxoff;
		sVert[v].x -= sxoff;
	    }
	}
    }
    
    /*
     * Perform all incremental transforms (for next time)
     */
    for (i = 0; i < xfs; ++i) {
	if (xif[i]) {
	    Cpy(xfl[i], tx1);
	    Cat(xil[i], tx1, xfl[i]);
	}
    }
    
    /*
     * Loop to compute and buffer new edges.
     */
    buf = (buf + 1) % MAXTAIL;
    obuf = (obuf + 1) % MAXTAIL;
    
    for (i = 0; i < edges; ++i) {
	e1 = edge[i].v1; e2 = edge[i].v2;
	sEdge[buf][i].x1 = sVert[e1].x;
        sEdge[buf][i].y1 = sVert[e1].y;
        sEdge[buf][i].x2 = sVert[e2].x;
        sEdge[buf][i].y2 = sVert[e2].y;
    }
    if (stereoFlag)
	for (i = 0; i < edges; ++i) {
	    e1 = edge[i].v1; e2 = edge[i].v2;
	    sEdge[buf][i + edges].x1 = rVert[e1].x;
	    sEdge[buf][i + edges].y1 = rVert[e1].y;
	    sEdge[buf][i + edges].x2 = rVert[e2].x;
	    sEdge[buf][i + edges].y2 = rVert[e2].y;
	}
    if (print & (pEscreen | pEfigure)) {
	for (i = 0; i < edges; ++i) {
	    e1 = edge[i + edges].v1; e2 = edge[i + edges].v2;
	    if (print & pEscreen) {
		printf("    line(%03d, %03d,  %03d, %03d)\n",
		       sVert[e1].x, sVert[e1].y, sVert[e2].x, sVert[e2].y);
	    }
	    if (print & pEfigure) {
		Vec v;
		printf("    edge(");
		Transform(vertex[e1], tx, v); vprint(v);
		printf(",\n         ");
		Transform(vertex[e2], tx, v); vprint(v);
		printf(")\n");
	    }
	}
    }
    
    /*
     * erase and draw buffered edges, using polysegment ops if available.
     * If newly exposed, also redraw the old tail
     */
 doDraw:
    if (!drawn) {
	for (i = obuf; i != buf; ) {
	    i = (i + 1) % MAXTAIL;	/* like using i++ in the test */
	    redraw(0, NULL, sEdge[i], stereoFlag? edges * 2 : edges);
	}
	drawn = 1;
    } else {
	redraw(tail >= 0, sEdge[obuf], sEdge[buf], 
	       stereoFlag? edges * 2 : edges);
    }
}

 
/*********************************************************************\
**
** Initializing and printing transforms.
**
\*********************************************************************/
 
/*
** xinit(s, xf)
**
**      Concatenate xf with a transform given by argument string s.
**          ABF     rotate F degrees in the AB plane
**          AF      translate by F along the A axis
**          AsF     scale by F along the A axis
**          ApF     perspective by F along the A axis
**          F       scale by F
**
**      Returns 1 if it sees something it can handle,
**      0 otherwise.
*/
int xinit(opt, xf)
    char *opt;
    XForm xf;
{
    register int a1;
    register char *s = opt;
    double f;

    switch (s[0]) {
	 case '.': Ident(xf); break;

     case 'x': case 'X':    a1 = 1; goto doRot;
     case 'y': case 'Y':    a1 = 2; goto doRot;
     case 'z': case 'Z':    a1 = 3; goto doRot;
     case 't': case 'T':    a1 = 4; goto doRot;
     case 'u': case 'U':    a1 = 5; goto doRot;
     case 'v': case 'V':    a1 = 6; goto doRot;
     case 'w': case 'W':    a1 = 7;
doRot:
        switch (s[1]) {
         case 'p': case 'P':    Persp(a1, 45*DEGREES, atof(s+2), xf); break;
         case 's': case 'S':    Scale(a1, atof(s+2), xf); break;
 
         case 'x': case 'X':    Rot(a1, 1, atof(s+2)*DEGREES, xf); break;
         case 'y': case 'Y':    Rot(a1, 2, atof(s+2)*DEGREES, xf); break;
         case 'z': case 'Z':    Rot(a1, 3, atof(s+2)*DEGREES, xf); break;
         case 't': case 'T':    Rot(a1, 4, atof(s+2)*DEGREES, xf); break;
         case 'u': case 'U':    Rot(a1, 5, atof(s+2)*DEGREES, xf); break;
         case 'v': case 'V':    Rot(a1, 6, atof(s+2)*DEGREES, xf); break;
         case 'w': case 'W':    Rot(a1, 7, atof(s+2)*DEGREES, xf); break;
         default:
            Trans(a1, atof(s+1), xf);
        }
        return(1);
      default:
         return (0);
    }
}
 
 
/*
 * xprint(xf)
 *
 *      Print transform matrix
 */
xprint(xf)
    XForm xf;
{
    register int i, j;
    for (i = 0; i <= dim; ++i) {
        printf("    [");
        for (j = 0; j <= dim; ++j) printf("%8.4f ", (double)xf[i][j]);
        printf("]\n");
    }
}


/*
 * vprint(v)
 *
 *      Print vector (in non-homogeneous form)
 */
vprint(v)
    Vec v;
{
    register int i;
    
    for (i = 0; i < dim; ++i)
        printf("%7.4f%s",
               (double) v[i] / v[dim],
               i < dim-1? ", " : "");
}

/*********************************************************************\
**
** Initializing Transforms
**
\*********************************************************************/

void initScale()
{
    xoff = winW/2;
    yoff = winH/2;
    if (stereoFlag) 
	Sscale = (xoff/2 > yoff)? yoff : xoff/2;
    else
	Sscale = (xoff > yoff)? yoff : xoff;
}

void initXforms()
{
    xfs = curxf = 0;
    Ident(figx);			/* init figure transform */
    initScale();
}

void normalPersp(theta, tx)
    double theta;
    XForm  tx;
{
    int i;
    double r, z, q;
    
    r = 1.05 * radius;			/* max diagonal radius */
    q = sin(theta);			/* === sin(theta/2) ?? === */
    Ident(tx);	
    tx[X][X] = tx[Y][Y] = CScale(cos(theta));
    tx[H][H] = CScale(-r);
    for (i = dim; i > 2; --i) {
	tx[i][i] = CScale(q);
	tx[i][H] = CScale(sin(theta));
	if (q > 0) tx[H][i] = CScale(-r / q);
    }
    
    /*
     * === Still doesn't scale properly:
     * ===   theta = 30 is highly distorted, and clips.
     * === Also doesn't generalize to different angles in different directions,
     * === though the only real use is perspective vs. ortho.
     */
}


/*
 * stereoView
 *
 *	construct the left and right view transforms
 */
void stereoView()
{
    Cpy(viewx, viewl);	Rot(1, 3, -sangle, viewl);
    Cpy(viewx, viewr);	Rot(1, 3, sangle, viewr);
    initScale();
}


/*
 * randomXform
 *
 *	Add a random incremental rotation to the end of
 *	the transform list.
 */
#define rnum(m)     ((rand()) % (m))
#define rrnum(m)    (((rnum(16384) / 8192.0) - 1.0) * (m))

void randomXform()
{
    int i, j;
    double f;
    
    useXform(curxf);
    xif[curxf] = 1;
    for (i = 1; i < dim; ++i)
        for (j = i + 1; j <= dim; ++j) {
            f = rrnum(0.1);
            Rot(i, j, f*DEGREES, xil[curxf]);
        }
}



/*********************************************************************\
**
** Transform List Operations
**
\*********************************************************************/

/*
 * useXform(n)
 *
 *	Operate on the nth transform in the list.
 *	extend the list if necessary.
 *	return the transform
 */
useXform(n)
    int n;
{
    curxf = n;
    while (curxf >= xfs) {
	++ xfs;
	xif[curxf] = 0;
	Ident(xfl[curxf]);
	Ident(xil[curxf]);
    }
}


/*
 * nextXform
 *
 *	Start operating on the next transform in the list.
 *	Extend the list if necessary.
 */
void nextXform()
{
    if (curxf <= xfs) ++curxf;
}


/*********************************************************************\
**
** Initializing the Figure and the associated buffers
**
\*********************************************************************/

/*
 * initMemory(v, e, t)
 *
 *	Initialize buffers for v vertices, e edges and t screen images.
 */
initMemory(v, e, t)
	int v, e, t;
{
    int i;
    
    /*
     * Allocate the vertex and edge list buffers
     */
    if (v) {
	if (vertex) free(vertex);
	vertex = (Vec *) calloc(v, sizeof(Vec));
	vertices = v;
    }
    if (e) {
	if (edge) free(edge);
	edge = (FEdge *) calloc(e, sizeof(FEdge));
	edges = e;
    }

    /*
     * Allocate the screen edge buffers, doubling the size
     * to leave room for a stereo image if requested.
     */
    if (t) {
	for (i = 0; i < t; ++i) {
	    if (sEdge[i]) free(sEdge[i]);
	    sEdge[i] = (SLine*) calloc(edges * 2, sizeof(*sEdge[0]));
	}
	if (sVert) free(sVert);
	sVert = (SPoint*) calloc(vertices, sizeof(*sVert));
	if (rVert) free(rVert);
	rVert = (SPoint*) calloc(vertices, sizeof(*rVert));
    }
}

/*
 * initCube()		build a cube (measure polytope)
 *
 *      Build the n-cube's vertices and edges
 */
initCube()
{
    register int i, j, k;
    
    vertices = 1 << dim;
    edges = vertices * dim / 2;
    initMemory(vertices, edges, MAXTAIL);
    
    for (i = 0; i < vertices; ++i) {
        for (j = 0; j < dim; ++j) 
            vertex[i][j+1] = (i & (1 << j))? One : -One;
        vertex[i][H] = 1;
    }
    
    for (i = 0, k = 0; i < (1 << dim); ++i)
        for (j = 0; j < dim; ++j)
            if ((i & (1 << j)) == 0) {
                edge[k].v1 = i;
                edge[k++].v2 = i | (1 << j);
            }
    
    if (k != edges) {
	fprintf(stderr, "Computed edges = %d, actual = %d\n", edges, k);
	exit(1);
    }
}

/*
 * initSimplex()	build a simplex
 */
void initSimplex()
{
    register int i, j, k;
    double a, b, c, d, dd=dim;

    vertices = dim + 1;
    edges = vertices * (vertices - 1) / 2;
    initMemory(vertices, edges, MAXTAIL);

    /*
     * compute a = the altitude of the simplex (given an edge of sqrt(2)).
     */
    a = sqrt(2.0) * sqrt(dd + 1) / 2;
    /*
     * Construct the simplex based on the fact that the points whose
     *	coordinates are zero except for a single 1.0 form a dim-1 simplex.
     *
     * 	b = the portion of the altitude	on the 
     *		(-1, -1, ...) side of the origin.
     * 	c = the coordinate of the (-1, ...) point. 
     */
    b = a - sqrt(dd)/2;
    c = b / sqrt(dd);
    /* 
     * Correct for the fact that the simplex just constructed is "base-heavy".
     *	Put the origin at a/2.
     */
    d = a / (4 * sqrt(dd));

    /*
     * Finally, generate the vertices.
     */
    for (i = 0; i < vertices - 1; ++i) {
        for (j = 0; j < dim; ++j) 
            vertex[i][j+1] = (i == j)? 1 - d : -d;
        vertex[i][H] = One;
    }
    for (j = 0; j < dim; ++j) 
	vertex[i][j+1] = - c - d;
    vertex[i][H] = One;
printf ("a = %f, b = %f, c = %f, d = %f\n", a, b, c, d);
printf ("a2= %f, b2= %f, c2= %f, d2= %f\n", a*a, b*b, c*c, d*d);

    /*
     * Edges: each vertex connects to each of the others 
     */
    for (i = 0, k = 0; i < vertices - 1; ++i) {
        for (j = i + 1; j < vertices; ++j) {
	    edge[k].v1 = i;
	    edge[k++].v2 = j;
	}
    }
}

/*
 * initKite()		build a kite (cross polytope)
 *
 *	The vertices are at +-1 in each dimension;
 *	Each vertex is connected to all vertices off its own axis.
 */
void initKite()
{
    register int i, j, k;

    vertices = dim * 2;
    edges = vertices * (vertices - 2) / 2;
    initMemory(vertices, edges, MAXTAIL);

    for (i = 0; i < dim; ++i) {
	for (j = 0; j < dim; ++j) 
            vertex[i * 2][j+1] = (i == j)? One : 0;
        vertex[i * 2][H] = 1;
	for (j = 0; j < dim; ++j) 
            vertex[i * 2 + 1][j+1] = (i == j)? -One : 0;
        vertex[i * 2 + 1][H] = 1;
    }
    for (i = 0, k = 0; i < vertices - 2; i += 2) {
	/*
	 * k is the index into the edge buffer.
	 * Connect each vertex with all but the other one on its own axis.
	 */
	for (j = i + 2; j < vertices; ++j) {
	    edge[k].v1 = i;
	    edge[k++].v2 = j;
	    edge[k].v1 = i + 1;
	    edge[k++].v2 = j;
	}
    }
}

/*
 * initFile()
 *	Initialize by reading a file.
 *	If the file starts with a number, assume it's a .shp file:
 *	  4 dimensions, #verts, verts, #edges, edges
 */
void initFile()
{
    int i, j, k, v1, v2;
    double d;
    FILE *f;

    /*
     * Try to open the file.
     */
    f = fopen(figure, "r");
    if (!f) {
	fprintf(stderr, "Cannot open input file %s\n", figure);
	initCube();
	return;
    }
    /*
     * Try to read a number.
     * If successful, it's a .shp file's vertex count.
     */
    if (1 == fscanf(f, "%d ", &i)) {
	/*
	 * Read .shp file.
	 * === skip the error checking for now ===
	 */
printf("Vertices: %d\n", i);
	vertices = i;
	dim = 4;
	initMemory(vertices, 0, 0);
	for (i = 0; i < vertices; ++i) {
	    for (j = 0; j < dim; ++j) {
		fscanf(f, "%lg ", &d);
		vertex[i][j+1] = d;
	    }
	    vertex[i][H] = 1;
  	}
	fscanf(f, "%d ", &i);
printf("Edges: %d\n", i);
	edges = i;
	initMemory(0, edges, MAXTAIL);
	for (i = 0; i < edges; ++i) {
	    j = fscanf(f, "%d %d ", &v1, &v2);
            edge[i].v1 = v1; edge[i].v2 = v2; 
	    if (j != 2)
		fprintf(stderr, "Only %d fields reading edge %d\n", j, i);
	}

    } else {
	/*
	 * Must be something else. 
	 */
	/* ===  Punt for now. === */
	initCube();
    }
    fclose(f);
}


/*
 * initFigure()
 *
 *	Construct figure based on name.
 */
void initFigure()
{
    register int i, j;
    double r;

    if (!figure  ||
	!strcmp(figure, "cube") || !strcmp(figure, "c")) {
	initCube();
    } else if (!strcmp(figure, "simplex") || !strcmp(figure, "s")) {
	initSimplex();
    } else if (!strcmp(figure, "kite") || !strcmp(figure, "k")
	       || !strcmp(figure, "cross")) {
	initKite();
    } else {
	initFile();
    }

    /*
     * compute maximum radius of figure
     */

    radius = 0.0;
    for (i = 1; i < vertices; ++i) {
	r = 0.0;
	for (j = 1; j <= dim; ++j) r += vertex[i][j] * vertex[i][j];
	r /= vertex[i][H];
	r = sqrt(r);
	if (r > radius) radius = r;
    }
    normalPersp(perspFlag? vangle : 0.0, viewx);
    
    
}


/*
 * Re-initialize after drastic change or something
 */
void reInit()
{
    int i;
    
    initFigure();
    initXforms();
    normalPersp(perspFlag? vangle : 0.0, viewx);
    if (xfs == 0) randomXform();
    if (stereoFlag)
	stereoView();
    else
	initScale();
    
    for (i = 0; i < vertices; ++i) Transform(vertex[i], figx, vertex[i]);
    
    drawn = 0;
}

 
 
/*********************************************************************\
**
** Signal handling
**
\*********************************************************************/

cleanup()
{
#ifdef PC
    _setvideomode(_DEFAULTMODE);
#endif
}
 
quit()
{
	double f;

    cleanup();
    f = time(0L) - theTime;

#ifdef PC
    if (nargs == 1) prbox(banner);       /* Greet the user */
#endif /* PC */
	
    printf("Total: %d rotation cycles; %.2f cycles/sec.\n",
		   cycles,
		   cycles/f);
 
    if (pTflag) {
        printf("\nCurrent Transform:\n");
    }
    exit(0);
}
 
fpeHdlr()
{
    cleanup();
    fprintf(stderr, "floating point exception\n");
    exit(1);
}


/*********************************************************************\
**
** Bitmaps for "HyperCube" and "HyperSpace Express" logos
**
\*********************************************************************/


#define ncube_width 96
#define ncube_height 48
char ncube_bits[] = {
   0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
   0xa0, 0x0c, 0x00, 0x29, 0x79, 0x9e, 0xe7, 0x79, 0x9e, 0xe7, 0x01, 0xac,
   0x90, 0x30, 0x80, 0xa4, 0x24, 0x41, 0x12, 0x24, 0x49, 0x10, 0x80, 0x93,
   0x88, 0xc0, 0xc0, 0x63, 0x9e, 0xe7, 0x79, 0x9e, 0x27, 0x38, 0x60, 0x88,
   0x84, 0x00, 0x23, 0x11, 0x41, 0x50, 0x20, 0x41, 0x12, 0x04, 0x18, 0x84,
   0x02, 0x00, 0x90, 0x88, 0xe0, 0x89, 0x9e, 0x20, 0x79, 0x1e, 0x00, 0x82,
   0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x83,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83,
   0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xe0, 0x83,
   0x07, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x78, 0x83,
   0x07, 0x0b, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0x1e, 0x83,
   0x07, 0x1b, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x0e, 0x83,
   0x07, 0x1b, 0x02, 0x80, 0x01, 0x00, 0x00, 0x00, 0x06, 0x38, 0x06, 0x83,
   0x07, 0x1b, 0x3b, 0x00, 0x06, 0x00, 0x00, 0xc0, 0x01, 0x7c, 0x06, 0x83,
   0x07, 0x1b, 0xfb, 0x00, 0x18, 0x00, 0x00, 0x30, 0x00, 0x66, 0x06, 0x83,
   0x07, 0x33, 0x1b, 0x01, 0x60, 0x00, 0x00, 0x0c, 0x30, 0x63, 0x06, 0x83,
   0x07, 0x33, 0x19, 0x1d, 0x80, 0x01, 0x00, 0x83, 0x30, 0x63, 0x06, 0x83,
   0x07, 0xa3, 0x19, 0xa5, 0x00, 0xfe, 0xff, 0x98, 0x30, 0x63, 0x06, 0x83,
   0x07, 0xa3, 0x19, 0x85, 0x07, 0x01, 0xc0, 0x86, 0x30, 0x63, 0x06, 0x83,
   0x07, 0xe3, 0x18, 0x85, 0x88, 0x00, 0xe0, 0x82, 0x30, 0x63, 0x06, 0x83,
   0x07, 0xe3, 0x18, 0x85, 0xe8, 0xff, 0xbf, 0x82, 0x30, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x85, 0x48, 0x00, 0x90, 0x82, 0x30, 0x23, 0x06, 0x83,
   0x07, 0x63, 0xf8, 0x85, 0x48, 0x00, 0x90, 0x82, 0x30, 0x33, 0x06, 0x83,
   0xff, 0x63, 0x78, 0x9c, 0x4f, 0x00, 0x90, 0x82, 0x30, 0x1f, 0x7e, 0x83,
   0xff, 0x63, 0x18, 0x84, 0x42, 0x00, 0x50, 0x82, 0x30, 0x1f, 0x7e, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x42, 0x00, 0x30, 0x82, 0x30, 0x33, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0xc4, 0xff, 0x1f, 0x82, 0x30, 0x23, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x24, 0x00, 0x20, 0x82, 0x30, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x08, 0x00, 0x00, 0x82, 0x30, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x00, 0x00, 0x00, 0x86, 0x30, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x00, 0x00, 0x00, 0x98, 0x30, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x34, 0x00, 0x00, 0x00, 0x80, 0x30, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x80, 0x30, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x06, 0x83,
   0x07, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x06, 0x83,
   0x07, 0x63, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7c, 0x06, 0xfb,
   0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x06, 0x83,
   0x07, 0x03, 0x00, 0x5e, 0xc6, 0xf3, 0x3c, 0xcf, 0x03, 0x00, 0x06, 0x43,
   0x07, 0x01, 0x00, 0x41, 0x21, 0x49, 0x82, 0x20, 0x00, 0x00, 0x06, 0x23,
   0x07, 0x00, 0x80, 0x83, 0xf0, 0x3c, 0xc7, 0xf3, 0x00, 0x00, 0x0e, 0x13,
   0x07, 0x00, 0x40, 0x40, 0x09, 0x8a, 0x00, 0x41, 0x00, 0x00, 0x3c, 0x0b,
   0x07, 0x00, 0xe0, 0x31, 0x05, 0xd1, 0xf3, 0x3c, 0x00, 0x00, 0x70, 0x07,
   0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01};

#define hsexp_width 96
#define hsexp_height 48
char hsexp_bits[] = {
   0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xc0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
   0xa0, 0x0c, 0x80, 0xe1, 0x79, 0xce, 0x21, 0x79, 0x1f, 0x8e, 0x07, 0xac,
   0x90, 0x30, 0x40, 0x91, 0x24, 0x49, 0x92, 0x04, 0x04, 0x49, 0x80, 0x93,
   0x88, 0xc0, 0xe0, 0x79, 0x9e, 0x24, 0x49, 0x02, 0x82, 0xe4, 0x60, 0x88,
   0x84, 0x00, 0x93, 0x04, 0x49, 0x92, 0x24, 0x01, 0x41, 0x12, 0x18, 0x84,
   0x02, 0x00, 0x48, 0x82, 0xc8, 0x79, 0x9e, 0x87, 0xc0, 0x09, 0x00, 0x82,
   0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0, 0xff, 0x83,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x83,
   0x07, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xe0, 0x83,
   0x07, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x78, 0x83,
   0x07, 0x0b, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x60, 0x80, 0x1e, 0x83,
   0x07, 0x1b, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0xf0, 0x0e, 0x83,
   0x07, 0x1b, 0x02, 0x80, 0x01, 0x00, 0x00, 0x00, 0x06, 0x78, 0x06, 0x83,
   0x07, 0x1b, 0x3b, 0x00, 0x06, 0x00, 0x00, 0xc0, 0x81, 0x19, 0x06, 0x83,
   0x07, 0x1b, 0xfb, 0x00, 0x18, 0x00, 0x00, 0x30, 0xe0, 0x1b, 0x06, 0x83,
   0x07, 0x33, 0x1b, 0x05, 0x60, 0x00, 0x00, 0x0c, 0x16, 0x1b, 0x06, 0x83,
   0x07, 0x33, 0x19, 0x1d, 0x80, 0x01, 0x00, 0xc3, 0x15, 0x1b, 0x06, 0x83,
   0x07, 0xa3, 0x19, 0xa5, 0x00, 0xfe, 0xff, 0x58, 0x14, 0x1b, 0x06, 0x83,
   0x07, 0xa3, 0x19, 0x85, 0x07, 0x01, 0xc0, 0x46, 0x14, 0x1b, 0x06, 0x83,
   0x07, 0xe3, 0x18, 0x85, 0x88, 0x00, 0xe0, 0x42, 0x14, 0x1b, 0x06, 0x83,
   0x07, 0xe3, 0x18, 0x85, 0xe8, 0xff, 0xbf, 0x42, 0x14, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x85, 0x48, 0x00, 0x90, 0x42, 0x14, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0xf8, 0x85, 0x48, 0x00, 0x90, 0x42, 0x14, 0x1b, 0x06, 0x83,
   0xff, 0x63, 0x78, 0x9c, 0x4f, 0x00, 0x90, 0xde, 0xf7, 0x1b, 0x7e, 0x83,
   0xff, 0x63, 0x18, 0x84, 0x42, 0x00, 0x50, 0x50, 0xd0, 0x1b, 0x7e, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x42, 0x00, 0x30, 0x50, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0xc4, 0xff, 0x1f, 0x50, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x24, 0x00, 0x20, 0x50, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x08, 0x00, 0x00, 0x51, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x00, 0x00, 0x00, 0x56, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x84, 0x00, 0x00, 0x00, 0x58, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x34, 0x00, 0x00, 0x00, 0x40, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x40, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x04, 0x00, 0x00, 0x00, 0x00, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x06, 0x83,
   0x07, 0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x06, 0x83,
   0x07, 0x23, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x1a, 0x06, 0xfb,
   0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x06, 0x83,
   0x07, 0x03, 0x00, 0x5e, 0xc6, 0xf3, 0x3c, 0xcf, 0x03, 0xe0, 0x06, 0x43,
   0x07, 0x01, 0x00, 0x41, 0x21, 0x49, 0x82, 0x20, 0x00, 0x80, 0x06, 0x23,
   0x07, 0x00, 0x80, 0x83, 0xf0, 0x3c, 0xc7, 0xf3, 0x00, 0x00, 0x0e, 0x13,
   0x07, 0x00, 0x40, 0x40, 0x09, 0x92, 0x00, 0x41, 0x00, 0x00, 0x3c, 0x0b,
   0x07, 0x00, 0xe0, 0x31, 0x05, 0xd1, 0xf3, 0x3c, 0x00, 0x00, 0x70, 0x07,
   0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0x03,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01};


#ifndef unix

/* 
** The following stuff lets non-X systems flip the bits in the above
** bitmaps from their brain-damaged VAX form to the more usual ordering,
** thus letting us use fast, native-mode bitmap-drawing primitives.
*/

/* Table for reversing the bits in a byte */
unsigned char reverse[256] = {
  0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
  0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
  0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
  0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
  0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
  0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
  0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
  0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
  0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
  0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
  0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
  0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
  0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
  0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
  0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
  0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
  0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
  0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
  0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
  0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
  0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
  0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
  0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
  0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
  0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
  0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
  0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
  0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
  0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
  0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
  0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
  0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff, 
};

void flip(data, size)
	char *data;
	int  size;		/* in BYTES: i.e. sizeof(data) */
{
   	for ( ; size--; ++data) *data = reverse[*data & 255];
}

#endif /* unix */


/*********************************************************************\
**
** doCmd(argc, argv, flag) -- Command handler.
**
**      returns 0 normally
**				1 if clear needed
**				2 if re-init needed (dimension changed)
**		       -1 on error
**
**		flag =  0	do anything 		(called from doKey)
**				1	do only options		(first pass on cmd line)
**				2	do only transforms	(second pass on cmd line)
**
**		(Actually, anything safe is done on both pass 1 AND pass 2)
**
\*********************************************************************/

int doCmd(argc, argv, pass)
	int argc;
	char **argv;
	int pass;
{
    register int i, j, d;
    register char *ap;
    char c;
    double f;
    XForm xf;
    short fig = 0, incr = 1;			/* which xform to mung. */
    int retval = 0;						/* return value for the function */
    int ret = 0;						/* return value for this option  */
    
    for (i = 1, ap = argv[i]; i < argc; ) {
        switch (ap[0]) {
         case '-':
            if (*++ap) continue;		/* === bogus === */
            break;
#ifdef MAC
         case 'a': case 'A':
	    showAbout();
	    break;
#endif
	 case 'c': case 'C':
	    if (haveColorFlag && maxColors > avoidColors) colorFlag^= 1;
	    break;
	    
         case 'd': case 'D':
            d = atoi(ap + 1);
#ifdef XWS
            if (!d) {
		extern char *display;

                display = argv[++i];
                break;
            }
#endif
            if (d > MAXDIM || d < 2) {
                fprintf(stderr,
			"Dimension must be between 2 and %d (inclusive).\n",
			MAXDIM);
                if (pass != 0) exit(1);
            }
            if (pass != 2) dim = d;
	    ret = 2;
            break;
	 case 'f': case 'F':
	    if (ap[1]) {
		figure = figbuf;
		strcpy(figbuf, &ap[1]);
	    } 
	    if (pass == 0) {initFigure(); ret = 1;}
	    break;
         case 'h': case 'H': case '?':
	    if (pass == 1) {
		prbox(ohelp);
		exit(1);
	    }
            /* === else need about box === */
	    break;
	 case 'i': case 'I':
	    if (pass != 2) interFlag ^= 1;
	    break;
         case 'k': case 'K':
            if (pass != 2) kFlag ^= 1;
            break;
         case 'l': case 'L':
	    if (pass != 2) logoFlag ^= 1;
	    ret = 1;
	    break;
         case 'n': case 'N':
            tail = atoi(ap + 1);
	    if (pass != 0) break;
            if (tail >= MAXTAIL - 2) tail = MAXTAIL - 2;
            obuf = (tail >= 0)? buf - tail - 1 : buf - 1;
            if (obuf < 0) obuf += MAXTAIL;
	    ret = 1;			/* === may be some hassle here === */
            break;
         case 'p': case 'P':
            switch (ap[1]) {
             case 'e': case 'E': pEflag |= pEfigure; break;
             case 's': case 'S': pEflag |= pEscreen; break;
             case 't': case 'T': pTflag = 1; break;
	     case 0: if (pass != 2) perspFlag ^= 1; goto doView;
	     default: 
		vangle = atof(ap+1) * DEGREES;
		perspFlag = 1;
		goto doView;
            }
            break;
	 case 'q': case 'Q':
	    quit();
	    break;
         case 'r': case 'R':
	    if (ap[1] == 'v') {
		if (pass != 2) rvFlag ^= 1;
		ret = 1;
#ifdef XWS
            } else if (ap[1] == 0) {
		extern short useRoot;

		useRoot = 1;
#endif /* XWS */
	    } else if (ap[1] == 'r') {
		randomXform();
	    } else goto unrec;
            break;
	 case 's': case 'S':
	    if (!ap[1]) {
		if (pass != 2) stereoFlag = !stereoFlag;
	    } else if (ap[1] == 'c' || ap[1] == 'C') {
		stereoFlag = (stereoFlag == 2)? 1 : 2;
	    } else if (atof(ap + 1) == 0.0) {
		stereoFlag = 0;
	    } else {
		sangle = atof(ap + 1);
		if (!stereoFlag) stereoFlag = 1;
	    }
	 doView: 
	    if (pass == 0) {
		normalPersp(perspFlag? vangle : 0.0, viewx);
		if (stereoFlag)
		    stereoView();
		else
		    initScale();
	    }
	    ret = 1;
	    break;
#ifdef XWS
         case 'g': case 'G':
	    {
	    extern char *geom;
	    geom = argv[++i];
	    /* === ought to configure window if pass == 0 === */
	    }
            break;
#endif /* XWS */
         case ',':
	    incr = 0;
	    if (xfs > 0) ++curxf;
	    break;
	 case '+':
	    incr = 1;
	    if (xfs > 0) ++curxf;
	    break;
	 case '=':
	    fig = 1;
	    break;
	 case 'x': case 'X':
	    if (ap[1] == 0) {
		if (pass != 2) xorFlag ^= 1;
		ret = 1;
		break;
	    }
	    /* else fall through */
         default:
            if (*ap >= 's' && *ap <= 'z'
		|| *ap >= 'S' && *ap <= 'Z'
		|| *ap == '.') { 
                if (pass == 1) break;
		if (fig) {
		    xinit(ap, figx);
		} else {
		    useXform(curxf);
		    if (incr) xif[curxf] = 1;
		    if (incr) xinit(ap, xil[curxf]);
		    else      xinit(ap, xfl[curxf]);
		}
            } else {
             unrec:
                if (pass == 1) {
		    fprintf(stderr, "ncube: unknown option %s\n", ap);
		    prbox(ohelp);
		    exit(1);
		} else {
		    return(-1);
		}
            }
	}
	ap = argv[++i];
	if (ret > retval) retval = ret;
    }
    
    return(retval);
}


/*********************************************************************\
**
** Key-press event handler
**
**      returns result of doCmd on return; zero otherwise.
**
\*********************************************************************/

char *keyptrs[3] = {0, keys, 0};	/* === needs to tokenize buffer === */

int doKey(k)
    char k;
{
    int ret = 0;
    
    if (kFlag) return(0);
    
    switch (k) {
	
     case 0x1b:				/* quit immediately on escape */
        quit();
	break;
	
     case 0x13:				/* ^S stops (pauses) */
	pauseFlag = 1;
	break;
	
     case 0x11:				/* ^Q resumes (unpauses) */
	pauseFlag = 0;
	break;
	
     case '\n': case '\r':		/* newline: interpret command */
	ret = doCmd(2, keyptrs, 0);
	keys[0] = 0;
	curKey = 0;
	break;
	
     default:
	keys[curKey] = k;
	keys[++curKey] = 0;
    }
    showKeys();
    return(ret);
}

 
/*********************************************************************\
**
** Main Program
**
\*********************************************************************/
 
main(argc, argv, envp)
  int argc;
  char *argv[], *envp[];
{
    register int i;
    
    
    time(&theTime);                     /* Initialize random number seed */
    srand((unsigned)theTime);
    
#ifdef MAC
    argc = 0;			/* MAC has no command line */
#endif 
    
#ifdef PC
    interFlag = 0;			/* PC defaults to screensaver mode */
#endif
    
    nargs = argc;			/* PC cleanup needs this */
    sangle = 5.0 * DEGREES;		/* braindead MAC won't init floats */
    vangle = 15.0 * DEGREES;
    tail = -2;				/* so  we can tell if user sets it */
    
    /*
     ** Go through the command line the first time, looking for options.
     */
    doCmd(argc, argv, 1);
    
#ifndef MAC				/* MAC doesn't grok signals */
    signal(SIGINT, quit);
    signal(SIGFPE, fpeHdlr);
#endif
    
    initGraphics();
    
#ifdef XWS
    if (argc == 1) prbox(banner);       /* Greet the user on stdout */
    /* can't clear because the window's not exposed yet. */
#endif
    
    /*
     * Initialize Transforms and the Cube, plus any defaults that
     *	depend on things we discover while initializing graphics.
     * (We can't do this earlier because dim might have been changed).
     */
    initFigure();
    initXforms();
    if (haveColorFlag && maxColors > avoidColors) {
	colorFlag = 1;
	if (tail == -2) tail = 10;
    }
    if (tail == -2) tail = 1;
    if (tail >= MAXTAIL - 2) tail = MAXTAIL - 2;
    buf = obuf + tail + 1;
    
    /*
     ** Go through the command line again, this time looking for transforms
     */
    doCmd(argc, argv, 2);
    
    
    /*
     ** Apply the initial transform matrix
     ** If no incremental transform specified, make a random one.
     ** Build the view transform (in stereo, if requested).
     */
    for (i = 0; i < vertices; ++i) Transform(vertex[i], figx, vertex[i]);
    
    if (xfs == 0) randomXform();
    normalPersp(perspFlag? vangle : 0.0, viewx);
    if (stereoFlag) stereoView();
    
    
    if (pTflag) {
        printf("\nTranslation and view matrix:\n");
        xprint(viewx);
        printf("Incremental Rotation matrix:\n");
        xprint(xil[0]);
        printf("Figure transformation matrix:\n");
        xprint(figx);
    }
    
    /*
     ** Do the actual work
     */
    time(&theTime);                     /* for accurate cycles/second */
    
    loop();
    
 done:
    quit();
    
}
