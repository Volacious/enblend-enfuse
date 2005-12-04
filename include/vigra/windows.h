/************************************************************************/
/*                                                                      */
/*               Copyright 2002-2003 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    ( Version 1.2.0, Aug 07 2003 )                                    */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/
 
#ifndef VIGRA_WINDOWS_H
#define VIGRA_WINDOWS_H

// prevent the global namespace to become polluted with 
// badly named Windows macros

#if defined(_WIN32)
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef DIFFERENCE
#endif

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

#endif /* VIGRA_WINDOWS_H */
