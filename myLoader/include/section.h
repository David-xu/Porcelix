#ifndef _MYLOADER_SECTION_H_
#define _MYLOADER_SECTION_H_

#define _SECTION_(secname)  __attribute__ ((__section__(#secname)))

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
#define GET_SYMBOLVALUE(sym)        ((unsigned)(sym))


#endif

