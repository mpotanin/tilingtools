/*****************************************************************************/
// File: kdu_arch.h [scope = CORESYS/COMMON]
// Version: Kakadu, V7.3.3
// Author: David Taubman
// Last Revised: 17 January, 2014
/*****************************************************************************/
// Copyright 2001, David Taubman, The University of New South Wales (UNSW)
// The copyright owner is Unisearch Ltd, Australia (commercial arm of UNSW)
// Neither this copyright statement, nor the licensing details below
// may be removed from this file or dissociated from its contents.
/*****************************************************************************/
// Licensee: LLC RD Centre Scanex
// License number: 01328
// The licensee has been granted a NON-COMMERCIAL license to the contents of
// this source file.  A brief summary of this license appears below.  This
// summary is not to be relied upon in preference to the full text of the
// license agreement, accepted at purchase of the license.
// 1. The Licensee has the right to install and use the Kakadu software and
//    to develop Applications for the Licensee's own use.
// 2. The Licensee has the right to Deploy Applications built using the
//    Kakadu software to Third Parties, so long as such Deployment does not
//    result in any direct or indirect financial return to the Licensee or
//    any other Third Party, which further supplies or otherwise uses such
//    Applications.
// 3. The Licensee has the right to distribute Reusable Code (including
//    source code and dynamically or statically linked libraries) to a Third
//    Party, provided the Third Party possesses a license to use the Kakadu
//    software, and provided such distribution does not result in any direct
//    or indirect financial return to the Licensee.
/******************************************************************************
Description:
   Definitions and functions which provide information about the machine
architecture, including information about special instruction sets for
vector arithmetic (MMX, SSE, Altivec, Sparc-VIS, etc.).
   All external variables and functions defined here are implemented in
"kdu_arch.cpp".
******************************************************************************/

#ifndef KDU_ARCH_H
#define KDU_ARCH_H

#include "kdu_ubiquitous.h"

/* ========================================================================= */
/*                      SIMD Support Testing Variables                       */
/* ========================================================================= */

KDU_EXPORT extern
  int kdu_mmx_level;
  /* [SYNOPSIS]
     Indicates the level of MMX support offered by the architecture:
     [>>] 0 if the architecture does not support MMX instructions (e.g.,
          non-Intel processor);
     [>>] 1 if the architecture supports MMX instructions only;
     [>>] 2 if the architecture supports MMX, SSE and SSE2 instructions;
     [>>] 3 if the architecture supports MMX, SSE, SSE2 and SSE3 instructions.
     [>>] 4 if the architecture supports MMX, SSE, SSE2, SSE3 and SSSE3.
     [>>] 5 if the architecture supports MMX, SSE, SSE2, SSE3, SSSE3 & SSE4.1.
     [>>] 6 if the architecture supports MMX through to AVX.
  */

KDU_EXPORT extern
  int kdu_get_mmx_level();
  /* [SYNOPSIS]
       This function should return exactly the same value that is stored in
       the global read-only variable `kdu_mmx_level'.  The only reason for
       providing the function is that it can be used in the construction of
       static/global objects and the initialization of other static/global
       variables -- the initialization code for such objects and variables
       is invoked in an uncertain order, possibly before the value of
       `kdu_mmx_level' is known, so you should use this function to
       explicitly evaluate the level of support if needed in such
       initialization functions.
  */

KDU_EXPORT extern
  bool kdu_pentium_cmov_exists;
  /* [SYNOPSIS]
     Indicates whether the X86 CMOV (conditional move) instruction
     is known to exist.
  */

KDU_EXPORT extern
  bool kdu_sparcvis_exists;
  /* [SYNOPSIS]
     True if the architecture is known to support the SPARC visual
     instruction set.
  */

KDU_EXPORT extern
  bool kdu_get_sparcvis_exists();
  /* [SYNOPSIS]
       This function should return exactly the same value that is stored in
       the global read-only variable `kdu_sparcvis_exists'.  The only reason
       ro providing the function is that it can be used in the construction of
       static/global objects and the initialization of other static/global
       variables -- the initialization code for such objects and variables
       is invoked in an uncertain order, possibly before the value of
       `kdu_sparcvis_exists' is known, so you should use this function to
       explicitly evaluate the level of support if needed in such
       initialization functions.
  */

KDU_EXPORT extern
  bool kdu_altivec_exists;
  /* [SYNOPSIS]
     True if the architecture is known to support the Altivec instruction --
     i.e., the fast vector processor available on many G4/G5 PowerPC
     CPU's.
  */

KDU_EXPORT extern
  bool kdu_get_altivec_exists();
  /* [SYNOPSIS]
       This function should return exactly the same value that is stored in
       the global read-only variable `kdu_altivec_exists'.  The only reason
       ro providing the function is that it can be used in the construction of
       static/global objects and the initialization of other static/global
       variables -- the initialization code for such objects and variables
       is invoked in an uncertain order, possibly before the value of
       `kdu_altivec_exists' is known, so you should use this function to
       explicitly evaluate the level of support if needed in such
       initialization functions.
  */

#ifdef KDU_MAC_SPEEDUPS
#  if defined(__ppc__) || defined(__ppc64__)
#    ifndef KDU_ALTIVEC_GCC
#      define KDU_ALTIVEC_GCC
#    endif
#  endif
#  if defined(__i386__) || defined(__x86_64__)
#    ifndef KDU_X86_INTRINSICS
#      define KDU_X86_INTRINSICS
#    endif
#  endif
#endif // KDU_MAC_SPEEDUPS

#ifndef KDU_MIN_MMX_LEVEL
#  if ((defined KDU_MAC_SPEEDUPS) && !(defined KDU_NO_SSSE3))
#    define KDU_MIN_MMX_LEVEL 4
#  else
#    define KDU_MIN_MMX_LEVEL 2
#  endif
#endif // Default KDU_MIN_MMX_LEVEL


/* ========================================================================= */
/*                        Cache-Related Properties                           */
/* ========================================================================= */

#define KDU_MAX_L2_CACHE_LINE 128 // Max bytes in an L2 cache line
                                  // Must be a power of 2!
#define KDU_CODE_BUFFER_ALIGN KDU_MAX_L2_CACHE_LINE
     /* Note: code buffers are best allocated to occupy (and be aligned on)
        whole cache-line boundaries.  A common L1 cache line size for
        modern processors is 64 bytes.  The situation is less clear for
        the L2 cache, but Intel processors typically read/write to/from
        the L2 cache in multiples of 128 bytes and it is best to avoid
        sharing lines of this size between threads. */

/* ========================================================================= */
/*                          Number of Processors                             */
/* ========================================================================= */

KDU_EXPORT extern int
  kdu_get_num_processors();
  /* [SYNOPSIS]
       This function returns the total number of logical processors which
       are available to the current process, or 0 if the value cannot be
       determined.  For a variety of good reasons, POSIX refuses to
       standardize a consistent mechanism for discovering the number of
       system processors, although their are a variety of platform-specific
       methods around.  If the number of processors cannot be discovered, this
       function returns 0.
  */

#endif // KDU_ARCH_H
