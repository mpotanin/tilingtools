/*****************************************************************************/
// File: kdu_ubiquitous.h [scope = CORESYS/COMMON]
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
   Elementary data types and extensively used constants.  This file can be
understood as the part of "kdu_elementary.h" that can safely be included
from anywhere, without causing any code to be generated.  This is because it
contains no class definitions.  The problem with class definitions is that
inline functions generate object code, so if the header is included in
different build configurations (e.g., with/without support for AVX function
call transitions), different versions of the same function may be generated,
creating linking problems.
   You should avoid explicitly including this file, except in cases where
including "kdu_elementary.h" will create problems such as those identified
above.  This will make your code as impervious as possible to future changes
in the splitting of definitions between the two files.
******************************************************************************/

#ifndef KDU_UBIQUITOUS_H
#define KDU_UBIQUITOUS_H
#include <new>
#include <limits.h>
#include <string.h>

#define KDU_EXPORT      // This one is used for core system exports

#define KDU_AUX_EXPORT  // This one is used by managed/kdu_aux to build
                        // a DLL containing exported auxiliary classes for
                        // linking to managed code (e.g., C# or Visual Basic)

#if (defined WIN32) || (defined _WIN32) || (defined _WIN64)
#  define KDU_WINDOWS_OS
#  undef KDU_EXPORT
#  if defined CORESYS_EXPORTS
#    define KDU_EXPORT __declspec(dllexport)
#  elif defined CORESYS_IMPORTS
#    define KDU_EXPORT __declspec(dllimport)
#  else
#    define KDU_EXPORT
#  endif // CORESYS_EXPORTS

#  undef KDU_AUX_EXPORT
#  if defined KDU_AUX_EXPORTS
#    define KDU_AUX_EXPORT __declspec(dllexport)
#  elif defined KDU_AUX_IMPORTS
#    define KDU_AUX_EXPORT __declspec(dllimport)
#  else
#    define KDU_AUX_EXPORT
#  endif // KDU_AUX_EXPORTS
#endif // _WIN32 || _WIN64

#ifdef KDU_WINDOWS_OS
#  include <Windows.h>
#endif // KDU_WINDOWS_OS


/* ========================================================================= */
/*                         Simple Data Types and Macros                      */
/* ========================================================================= */

#ifdef HAVE_INTPTR_T
# include <stdint.h>
#endif // HAVE_INTPTR_T

/*****************************************************************************/
/*                                 8-bit Scalars                             */
/*****************************************************************************/

typedef unsigned char kdu_byte;

/*****************************************************************************/
/*                                 16-bit Scalars                            */
/*****************************************************************************/

typedef short int kdu_int16;
typedef unsigned short int kdu_uint16;

#define KDU_INT16_MAX ((kdu_int16) 0x7FFF)
#define KDU_INT16_MIN ((kdu_int16) 0x8000)

/*****************************************************************************/
/*                                 32-bit Scalars                            */
/*****************************************************************************/

#if (INT_MAX == 2147483647)
  typedef int kdu_int32;
  typedef unsigned int kdu_uint32;
#else
# error "Platform does not appear to support 32 bit integers!"
#endif

#define KDU_INT32_MAX ((kdu_int32) 0x7FFFFFFF)
#define KDU_INT32_MIN ((kdu_int32) 0x80000000)

/*****************************************************************************/
/*                                 64-bit Scalars                            */
/*****************************************************************************/

#ifdef KDU_WINDOWS_OS
  typedef __int64 kdu_int64;
# define KDU_INT64_MIN 0x8000000000000000
# define KDU_INT64_MAX 0x7FFFFFFFFFFFFFFF
#elif (defined __GNUC__) || (defined __sparc) || (defined __APPLE__)
  typedef long long kdu_int64;
# define KDU_INT64_MIN 0x8000000000000000
# define KDU_INT64_MAX 0x7FFFFFFFFFFFFFFF
#endif // Definitions for `kdu_int64'

/*****************************************************************************/
/*                           Best Effort Scalars                             */
/*****************************************************************************/

#if (defined _WIN64)
#        define KDU_POINTERS64
         typedef __int64 kdu_long;
#        define KDU_LONG_MAX 0x7FFFFFFFFFFFFFFF
#        define KDU_LONG_HUGE (((kdu_long) 1) << 52)
#        define KDU_LONG64 // Defined whenever kdu_long is 64 bits wide
#elif (defined WIN32) || (defined _WIN32)
#    define WIN32_64 // Comment this out if you do not want support for very
                     // large images and compressed file sizes.  Support for
                     // these may slow down execution a little bit and add to
                     // some memory costs, but removing this support may
                     // possibly cause undocumented problems.
#    ifdef WIN32_64
         typedef __int64 kdu_long;
#        define KDU_LONG_MAX 0x7FFFFFFFFFFFFFFF
#        define KDU_LONG_HUGE (((kdu_long) 1) << 52)
#        define KDU_LONG64 // Defined whenever kdu_long is 64 bits wide
#    endif
#elif (defined __GNUC__) || (defined __sparc) || (defined __APPLE__)
#    define UNX_64 // Comment this out if you do not want support for very
                   // large images and compressed file sizes.  Support for
                   // these may slow down execution a little bit and add to
                   // some memory costs, but removing this support may
                   // possibly cause undocumented problems.
#    if (defined _LP64)
#      define KDU_POINTERS64
#      ifndef UNX_64
#        define UNX_64 // Need 64-bit kdu_long with 64-bit pointers
#      endif
#    endif

#    ifdef UNX_64
         typedef long long int kdu_long;
#        define KDU_LONG_MAX 0x7FFFFFFFFFFFFFFFLL
#        define KDU_LONG_HUGE (1LL << 52)
#        define KDU_LONG64 // Defined whenever kdu_long is 64 bits wide
#    endif
#endif

#ifndef KDU_LONG64
    typedef long int kdu_long;
#   define KDU_LONG_MAX LONG_MAX
#   define KDU_LONG_HUGE LONG_MAX
#endif

/*****************************************************************************/
/*                        Other Architecture Constants                       */
/*****************************************************************************/

#ifdef KDU_POINTERS64
#  define KDU_POINTER_BYTES 8 // Length of an address, in bytes
#  define KDU_LOG2_POINTER_BYTES 3
#else
#  define KDU_POINTER_BYTES 4 // Length of an address, in bytes
#  define KDU_LOG2_POINTER_BYTES 2
#endif // !KDU_POINTERS64

#if (__BYTE_ORDER__ == __ORDER__LITTLE_ENDIAN__)
#  define KDU_IS_LITTLENDIAN() true
#elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#  define KDU_IS_LITTLENDIAN() false
#else
#  define KDU_IS_LITTLENDIAN() (((*((kdu_uint16 *)"\0\xff")) & 0xFF) == 0)
#endif

/*****************************************************************************/
/*                        Useful Debugging Symbols                           */
/*****************************************************************************/

#ifndef NDEBUG
#  ifndef _DEBUG
#    define _DEBUG
#  endif
#endif // Not in release mode

/*****************************************************************************/
/*                    Pointer to/from Integer Conversions                    */
/*****************************************************************************/
#if (defined _MSC_VER && (_MSC_VER >= 1300))
#  define _kdu_int64_to_addr(_val)  ((void *)((INT_PTR)(_val)))
#  define _addr_to_kdu_int64(_val)  ((kdu_int64)((INT_PTR)(_val)))
#  define _kdu_long_to_addr(_val)   ((void *)((INT_PTR)(_val)))
#  define _addr_to_kdu_long(_addr)  ((kdu_long)((INT_PTR)(_addr)))
#  define _kdu_int32_to_addr(_val)  ((void *)((INT_PTR)(_val)))
#  define _addr_to_kdu_int32(_addr) ((kdu_int32)(PtrToLong(_addr)))
#elif (defined HAVE_INTPTR_T)
#  define _kdu_int64_to_addr(_val)  ((void *)((intptr_t)(_val)))
#  define _addr_to_kdu_int64(_val)  ((kdu_int64)((intptr_t)(_val)))
#  define _kdu_long_to_addr(_val)   ((void *)((intptr_t)(_val)))
#  define _addr_to_kdu_long(_addr)  ((kdu_long)((intptr_t)(_addr)))
#  define _kdu_int32_to_addr(_val)  ((void *)((intptr_t)(_val)))
#  define _addr_to_kdu_int32(_addr) ((kdu_int32)((intptr_t)(_addr)))
#elif defined KDU_POINTERS64
#  define _kdu_int64_to_addr(_val)  ((void *)(_val))
#  define _addr_to_kdu_int64(_val)  ((kdu_int64)(_addr))
#  define _kdu_long_to_addr(_val)   ((void *)(_val))
#  define _addr_to_kdu_long(_addr)  ((kdu_long)(_addr))
#  define _kdu_int32_to_addr(_val)  ((void *)((kdu_long)(_val)))
#  define _addr_to_kdu_int32(_addr) ((kdu_int32)((kdu_long)(_addr)))
#else // !KDU_POINTERS64
#  define _kdu_int64_to_addr(_val)  ((void *)((kdu_uint32)(_val)))
#  define _addr_to_kdu_int64(_val)  ((kdu_int64)((kdu_uint32)(_addr)))
#  define _kdu_long_to_addr(_val)   ((void *)((kdu_uint32)(_val)))
#  define _addr_to_kdu_long(_addr)  ((kdu_long)((kdu_uint32)(_addr)))
#  define _kdu_int32_to_addr(_val)  ((void *)((kdu_uint32)(_val)))
#  define _addr_to_kdu_int32(_addr) ((kdu_int32)(_addr))
#endif // !KDU_POINTERS64

/*****************************************************************************/
/*                                EXCEPTIONS                                 */
/*****************************************************************************/

typedef int kdu_exception;
    /* [SYNOPSIS]
         Best practice is to include this type in catch clauses, in case we
         feel compelled to migrate to a non-integer exception type at some
         point in the future.
    */

#define KDU_NULL_EXCEPTION ((int) 0)
    /* [SYNOPSIS]
         You can use this value in your applications, for a default
         initializer for exception codes that you do not intend to throw.
         There is no inherent reason why a part of an application cannot
         throw this exception code, but it is probably best not to do so.
    */
#define KDU_ERROR_EXCEPTION ((int) 0x6b647545)
    /* [SYNOPSIS]
         Best practice is to throw this exception within a derived error
         handler, whenever the end-of-message flush call is received -- see
         `kdu_error' and `kdu_customize_errors' and `kdu_message'.
         [//]
         Conversely, in a catch statement, you may compare the value of a
         caught exception with this value to determine whether or not the
         exception was generated upon handling an error message dispatched
         via `kdu_error'.
    */
#define KDU_MEMORY_EXCEPTION ((int) 0x6b64754d)
    /* [SYNOPSIS]
         You should avoid using this exception code when throwing your own
         exceptions from anywhere.  This value is used primarily by the
         system to record the occurrence of a `std::bad_alloc' exception
         and pass it across programming interfaces which can only accept
         a single exception type -- e.g., when passing exceptions between
         threads in a multi-threaded processing environment.  When rethrowing
         exceptions across such interfaces, this value is checked and used
         to rethrow a `std::bad_alloc' exception.
         [//]
         To facilitate the rethrowing of exceptions with this value as
         `std::bad_alloc', Kakadu provides the `kdu_rethrow' function,
         which leaves open the possibility that other types of exceptions
         may be passed across programming interfaces using special
         values in the future.
    */
#define KDU_CONVERTED_EXCEPTION ((int) 0x6b647543)
    /* [SYNOPSIS]
         Best practice is to throw this exception code if you need to
         convert an exception of a different type (e.g., exceptions caught
         by a catch-all "catch (...)" statement) into an exception of type
         `kdu_exception' (e.g., so as to pass it to
         `kdu_thread_entity::handle_exception', or when passing an exception
         across language boundaries).
    */

static inline void kdu_rethrow(kdu_exception exc)
  { if (exc == KDU_MEMORY_EXCEPTION) throw std::bad_alloc(); else throw exc; }
    /* [SYNOPSIS]
         You should ideally use this function whenever you need to rethrow
         an exception of type `kdu_exception'; at a minimum, this will cause
         exceptions of type `KDU_MEMORY_EXCEPTION' to be rethrown as
         `std::bad_alloc' which will help maintain consistency within your
         application when exceptions are converted and passed across
         programming interfaces as `kdu_exception'.  It also provides a path
         to future catching and conversion of a wider range of exception
         types.
    */

/*****************************************************************************/
/*                              Subband Identifiers                          */
/*****************************************************************************/

#define LL_BAND ((int) 0) // DC subband
#define HL_BAND ((int) 1) // Horizontally high-pass subband
#define LH_BAND ((int) 2) // Vertically high-pass subband
#define HH_BAND ((int) 3) // High-pass subband

/*****************************************************************************/
/*                    Default 16-bit Fixed-Point Precision                   */
/*****************************************************************************/

#define KDU_FIX_POINT ((int) 13)
  /* Number of fraction bits in a 16-bit fixed-point value.  Note that this
     definition has been moved here from "kdu_sample_processing.h" so that
     it can be included within contexts that must be free from any class or
     function definitions. */

#endif // KDU_UBIQUITOUS_H
