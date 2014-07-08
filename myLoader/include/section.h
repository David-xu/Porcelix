#ifndef _MYLOADER_SECTION_H_
#define _MYLOADER_SECTION_H_

#define _SECTION_(secname)  __attribute__ ((__section__(#secname)))

#define __init  _SECTION_(.init.text)

/* make the compiler happy */
#define __used  __attribute__((__used__))

/*  */
#define asmlinkage      __attribute__((regparm(3)))

/*
 * .init
 * .init.data
 */

/*
 * .array
 * .array.fs
 */


/******************************************************************************
 *   we just want to define one symbol, storage space dose NOT needed.          *
 ******************************************************************************/
#define DEFINE_SYMBOL(sym)          extern u32 sym[0]

/* get symbol value */
#define GET_SYMBOLVALUE(sym)        ((u32)(sym))

#endif

