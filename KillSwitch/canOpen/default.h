/* *** $Header: P:/source/c/RCS/default.h 4.1 2009/01/01 12:00:00 vogt Sav $ ***
 *
 *      Projekt   :  All over the world!
 *
 *      Zweck     :  Spracherweiterungen für C/C++.
 *
 *      Copyright :  (c) 1993-96 by Uwe Vogt, Berlin.
 *                   (c) 2002-2009, UV Software, Friedrichshafen.
 *
 *      Compiler  :  Beliebiger ANSI 'C' Compiler.
 *
 *      Export    :  (siehe unten)
 *
 *      Include   :  (keine)
 *
 *      Autor     :  Uwe Vogt, Berlin.
 *
 */

#ifndef _DEFAULT_H_
#define _DEFAULT_H_

/* ****************  Schnittstellendokumentation  ****************************
 *
 *      In dieser Include-Datei sind Definitionen zur Spracherweiterung der
 *      Programmiersprache 'C' und 'C++' getroffen.
 *
 *      Die Datei ist mit einer #include-Anweisung jedem Modul hinzuzufügen!
 */


/* ****************  Änderungsbeschreibung  **********************************
 *
 *	$Log: default.h $
 *	Revision 4.1  2009/01/01 12:00:00  vogt
 *	Changes for other targets than Windows or DOS, e.g. Linux.
 *	Copyright changed to UV Software, Friedirchshafen
 *
 *	Revision 3.1  1996/08/09 21:19:00  vogt
 *	'C/C++' Erweiterungen fr das Microsoft Windows SDK.
 *
 *	Revision 1.1  1993/08/18 21:09:00  vogt
 *	Initial revision
 *
 *
 *      Version 1 vom 18.08.93:
 *
 *              'C' Syntaxerweiterung für Microsoft QuickC Compiler.
 *
 *      Version 2 vom 18.01.94:
 *
 *              'C' Syntaxerweiterung für beliebigen ANSI 'C' Compiler.
 *
 *	Version 3 vom 27.06.96:
 *
 *		'C/C++' Erweiterungen nach den Konventionen für das Microsoft
 *		Windows SDK; (siehe auch '...\include\windows.h').
 *
 *      Version 3.1 vom 09.08.96:
 *
 *              Makros zur Konvertierung von 8-, 16- und 32-bit Werten.
 */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _INC_WINDOWS

/* ****************  Common definitions and typedefs ***********************************/

#define VOID		    void

#ifndef _DOS
 #define FAR
 #define NEAR
 #define PASCAL
 #define CDECL

 #define WINAPI
 #define CALLBACK
#else
 #define FAR                 _far
 #define NEAR		    _near
 #define PASCAL		    _pascal
 #define CDECL		    _cdecl

 #define WINAPI              _far _pascal
 #define CALLBACK            _far _pascal
#endif

/* ****************  Simple types & common helper macros *********************************/

typedef int		    BOOL;
#define FALSE		    0
#define TRUE		    1

typedef unsigned char	    BYTE;
typedef unsigned short      WORD;
typedef unsigned long       DWORD;

typedef unsigned char	    UCHAR;
typedef unsigned int	    UINT;
typedef unsigned long	    ULONG;

#ifdef STRICT
typedef signed char	    CHAR;
typedef signed short    SHORT;
typedef signed long	    LONG;
#else
#define CHAR char
#define SHORT short
#define LONG long
#endif

/*#define LOBYTE(w)	    ((BYTE)(w))
 *#define HIBYTE(w)           ((BYTE)(((UINT)(w) >> 8) & 0xFF))
 *
 *#define LOWORD(l)           ((WORD)(DWORD)(l))
 *#define HIWORD(l)           ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))
 */
#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))

#ifndef NOMINMAX
#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif
#endif  /* NOMINMAX */

/* Types use for passing & returning polymorphic values */
typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;

#define MAKELPARAM(low, high)	((LPARAM)MAKELONG(low, high))
#define MAKELRESULT(low, high)	((LRESULT)MAKELONG(low, high))

/* ****************  Common pointer types ************************************************/

#ifndef NULL
#define NULL		    0
#endif

typedef char NEAR*          PSTR;
typedef char NEAR*          NPSTR;


typedef char FAR*           LPSTR;
typedef const char FAR*     LPCSTR;

typedef BYTE NEAR*	    PBYTE;
typedef BYTE FAR*	    LPBYTE;

typedef int NEAR*	    PINT;
typedef int FAR*	    LPINT;

typedef WORD NEAR*          PWORD;
typedef WORD FAR*           LPWORD;

typedef long NEAR*	    PLONG;
typedef long FAR*	    LPLONG;

typedef DWORD NEAR*         PDWORD;
typedef DWORD FAR*          LPDWORD;

typedef void FAR*           LPVOID;

#define MAKELP(sel, off)    ((void FAR*)MAKELONG((off), (sel)))
#define SELECTOROF(lp)      HIWORD(lp)
#define OFFSETOF(lp)        LOWORD(lp)

#define FIELDOFFSET(type, field)    ((int)(&((type NEAR*)1)->field)-1)

#endif

/* ****************  'C' Syntaxerweiterung  **********************************
 */

/* ---  loop condition: 'until'  ---
 */

#define until( condition )      while( !(condition) )


/* ---  constants: 'TRUE' and 'FALSE'  ---
 */

/*#define FALSE   0
 *#define TRUE    1
 */

/* ---  type:  'BOOL : { FALSE, TRUE }'  ---
 */

/*typedef enum
 *{
 *  FALSE = 0,
 *  TRUE  = 1
 *} BOOL;
 */

/* ---  types: 'BYTE', 'WORD', and 'DWORD'  ---
 */

/**#define BYTE    unsigned char
 **#define WORD    unsigned int
 **#define DWORD   unsigned long
 **#define LOBYTE(word)		(unsigned char)(((unsigned int)(word) & 0x00FF) >> 0)
 **#define HIBYTE(word)		(unsigned char)(((unsigned int)(word) & 0xFF00) >> 8)
 *
 *#define LOWORD(dword)		(unsigned int)(((unsigned long)(dword) & 0x0000FFFF) >> 0)
 *#define HIWORD(dword)		(unsigned int)(((unsigned long)(dword) & 0xFFFF0000) >> 16)
 *
 *#define LOLOBYTE(dword)		(unsigned char)(((unsigned long)(dword) & 0x000000FF) >> 0)
 *#define LOHIBYTE(dword)		(unsigned char)(((unsigned long)(dword) & 0x0000FF00) >> 8)
 *#define HILOBYTE(dword)		(unsigned char)(((unsigned long)(dword) & 0x00FF0000) >> 16)
 *#define HIHIBYTE(dword)		(unsigned char)(((unsigned long)(dword) & 0xFF000000) >> 24)
 */
#ifdef _BIG_ENDIAN
 #define HIBYTE(value)					*( (unsigned char*) &value)
 #define LOBYTE(value)					*(((unsigned char*) &value) + 1)
 #define HIWORD(value)					*( (unsigned short*) &value)
 #define LOWORD(value)					*(((unsigned short*) &value) + 1)

 #define HIHIBYTE(value)				*( (unsigned char*) &value)
 #define HILOBYTE(value)				*(((unsigned char*) &value) + 1)
 #define LOHIBYTE(value)				*(((unsigned char*) &value) + 2)
 #define LOLOBYTE(value)				*(((unsigned char*) &value) + 3)
#else
 #define LOBYTE(value)					*( (unsigned char*) &value)
 #define HIBYTE(value)					*(((unsigned char*) &value) + 1)
 #define LOWORD(value)					*( (unsigned short*) &value)
 #define HIWORD(value)					*(((unsigned short*) &value) + 1)

 #define LOLOBYTE(value)				*( (unsigned char*) &value)
 #define LOHIBYTE(value)				*(((unsigned char*) &value) + 1)
 #define HILOBYTE(value)				*(((unsigned char*) &value) + 2)
 #define HIHIBYTE(value)				*(((unsigned char*) &value) + 3)
#endif

#define BYTES2WORD(low,high)	(((unsigned int)((unsigned char)(low)))  << 0) | \
				(((unsigned int)((unsigned char)(high))) << 8)
#define WORDS2DWORD(low,high)	(((unsigned long)((unsigned int)(low)))  << 0) | \
				(((unsigned long)((unsigned int)(high))) << 16)
#define BYTES2DWORD(ll,lh,hl,hh)(((unsigned long)((unsigned char)(ll)))  << 0) | \
				(((unsigned long)((unsigned char)(lh))) << 8) | \
				(((unsigned long)((unsigned char)(hl))) << 16) | \
				(((unsigned long)((unsigned char)(hh))) << 24)

#define BYTEV2WORD(array)	(((unsigned int)((unsigned char)(array[0]))) << 0) | \
				(((unsigned int)((unsigned char)(array[1]))) << 8)
#define BYTEV2DWORD(array)	(((unsigned long)((unsigned char)(array[0]))) << 0) | \
				(((unsigned long)((unsigned char)(array[1]))) << 8) | \
				(((unsigned long)((unsigned char)(array[2]))) << 16) | \
				(((unsigned long)((unsigned char)(array[3]))) << 24)


/* ---  operators  ---
 */

#define EQ      ==
#define NEQ     !=

#define NOT     !
#define AND     &&
#define OR      ||

#define BITAND  &
#define BITOR   |
#define BITXOR  ^
#define BITCPL  ~

#define CONT(x) (*(x))
#define ADR(x)  (&(x))


#ifdef __cplusplus
}
#endif
#endif

/* ****************  Ende der Datei  *****************************************
 */


