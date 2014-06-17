/*****************************************************************************/
// File: kdu_sample_processing.h [scope = CORESYS/COMMON]
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
   Uniform interface to sample data processing services: DWT analysis; DWT
synthesis; subband sample encoding and decoding; colour and multi-component
transformation.  Here, we consider the encoder and decoder objects to be
sample data processing objects, since they accept or produce unquantized
subband samples.  They build on top of the block coding services defined in
"kdu_block_coding.h", adding quantization, ROI adjustments, appearance
transformations, and buffering to interface with the DWT.
******************************************************************************/

#ifndef KDU_SAMPLE_PROCESSING_H
#define KDU_SAMPLE_PROCESSING_H

#include <assert.h>
#include "kdu_messaging.h"
#include "kdu_compressed.h"
#include "kdu_arch.h"

// Defined here:
union kdu_sample32;
union kdu_sample16;
class kdu_sample_allocator;
class kdu_line_buf;

class kdu_push_ifc_base;
class kdu_push_ifc;
class kdu_pull_ifc_base;
class kdu_pull_ifc;
class kdu_analysis;
class kdu_synthesis;
class kdu_encoder;
class kdu_decoder;
class kd_multi_analysis_base;
class kd_multi_synthesis_base;
class kdu_multi_analysis;
class kdu_multi_synthesis;

// Defined elsewhere:
class kdu_roi_node;
class kdu_roi_image;


/* ========================================================================= */
/*                     Class and Structure Definitions                       */
/* ========================================================================= */

/* ========================================================================= */
/*                              Critical Macros                              */
/* ========================================================================= */

/* Notes:
   1. The following quantities are all expected to be powers of 2.
   2. The `KDU_PREALIGN_BYTES' value must be no smaller than
         max{ 2*`KDU_ALIGN_SAMPLES16', 4*`KDU_ALIGN_SAMPLES32' }
*/

#  define KDU_OVERREAD_BYTES   128
#  define KDU_PREALIGN_BYTES    64
#  define KDU_ALIGN_SAMPLES16    8
#  define KDU_ALIGN_SAMPLES32    8


/*****************************************************************************/
/*                              kdu_sample32                                 */
/*****************************************************************************/

union kdu_sample32 {
    float fval;
      /* [SYNOPSIS]
           Normalized floating point representation of an image or subband
           sample subject to irreversible compression.  See the description
           of `kdu_line_buf' for more on this.
      */
    kdu_int32 ival;
      /* [SYNOPSIS]
           Absolute 32-bit integer representation of an image or subband
           sample subject to reversible compression.  See the description
           of `kdu_line_buf' for more on this.
      */
  };

/*****************************************************************************/
/*                              kdu_sample16                                 */
/*****************************************************************************/

#define KDU_FIX_POINT ((int) 13) // Num frac bits in a 16-bit fixed-point value

union kdu_sample16 {
  kdu_int16 ival;
    /* [SYNOPSIS]
         Normalized (fixed point) or absolute (integer) 16-bit representation
         of an image or subband sample subject to irreversible (normalized)
         or reversible (integer) compression.  See the description of
         `kdu_line_buf' for more on this.
    */
  };

/*****************************************************************************/
/*                          kdu_sample_allocator                             */
/*****************************************************************************/

class kdu_sample_allocator {
  /* [BIND: reference]
     [SYNOPSIS]
     This object serves to prevent excessive memory fragmentation and
     provide useful memory alignment services.
     [//]
     Pre-allocation requests are made by all clients which wish to use
     its services, after which a single block of memory is allocated and
     the allocated memory is assigned in accordance with the original
     pre-allocation requests.  Memory chunks allocated by this
     object to a client are not be individually returned or recycled, since
     this incurs overhead and fragmentation.
     [//]
     The object may be re-used (after a call to its `restart' member) once
     all memory served out is no longer in use.  When re-used, the object
     makes every attempt to avoid destroying and re-allocating its memory
     block.  This avoids memory fragmentation and allocation overhead when
     processing images with multiple tiles or when used for video
     applications.
     [//]
     The object allocates memory in such a way that allocated buffers may
     be over-read by at least `KDU_OVERREAD_BYTES' bytes -- the value of
     this constant is typically 128, but a larger value might be required
     for SIMD architectures with extremely wide data paths.  Applications
     can legally read up to `KDU_OVERREAD_BYTES' bytes beyond the storage
     that has nominally been allocated and also up to `KDU_OVERREAD_BYTES'
     bytes before the storage that has nominally been allocated.  However,
     they may not generally write beyond the allocated storage without
     interfering with other buffers allocated by the same sample allocator.
     [//]
     Although the principle intent is to allocate storage for image
     samples (including transformed image samples), the object can also
     be used to allocate more general blocks of memory, taking advantage
     of the alignment support which it offers.
  */
  public: // Member functions
    kdu_sample_allocator()
      { 
        buffer_size = buffer_alignment = 0;
        buffer = buf_handle = NULL;
        restart();
      }
      /* [SYNOPSIS] Creates an empty object. */
    ~kdu_sample_allocator()
      { if (buf_handle != NULL) delete[] buf_handle; }
      /* [SYNOPSIS]
           Destroys all storage served from this object in one go.
      */
    void restart()
      { 
        bytes_reserved=0; alignment_reserved=KDU_MAX_L2_CACHE_LINE;
        pre_alloc_overflow = false;
        if (alignment_reserved < KDU_OVERREAD_BYTES)
          alignment_reserved = KDU_OVERREAD_BYTES;
        pre_creation_phase=true;
      }
      /* [SYNOPSIS]
           Invalidates all memory resources served up using `alloc16' or
           `alloc32' and starts the pre-allocation phase from scratch.  The
           internal resources are sized during subsequent calls to `pre_alloc'
           and memory is re-allocated, if necessary, during the next call to
           `finalize', based on these pre-allocation requests alone.
      */
    void pre_align(size_t alignment_multiple)
      { 
        assert(pre_creation_phase);
        while (alignment_reserved < alignment_multiple)
          alignment_reserved += alignment_reserved;
        bytes_reserved += alignment_multiple;
        if (bytes_reserved < alignment_multiple)
          pre_alloc_overflow = true;
        bytes_reserved &= ~(alignment_multiple-1);
      }
      /* [SYNOPSIS]
           This function allows you to enforce alignment constraints for
           the block of memory associated with the next call to
           `pre_alloc' or `pre_alloc_block'.  These functions have their
           own default alignment constraints which may be looser than you
           would like.  The `alignment_multiple' supplied here must be an
           exact power of 2 -- this is your responsibility and it might
           not be validated.  Values for the `alignment_multiple' that
           might be of interest are 32 (for AVX operands), 64 (typical
           L1/L2 cache line boundary).
      */
    size_t pre_alloc(bool use_shorts, int before, int after,
                     int num_requests=1)
      { 
        pre_align(KDU_PREALIGN_BYTES);
        size_t offset = bytes_reserved;
        int num_bytes;
        if (use_shorts)
          { 
            num_bytes =
              (((KDU_ALIGN_SAMPLES16-1+before) & ~(KDU_ALIGN_SAMPLES16-1)) +
               ((KDU_ALIGN_SAMPLES16-1+after) & ~(KDU_ALIGN_SAMPLES16-1)));
            num_bytes <<= 1;
          }
        else
          { 
            num_bytes =
              (((KDU_ALIGN_SAMPLES32-1+before) & ~(KDU_ALIGN_SAMPLES32-1)) +
               ((KDU_ALIGN_SAMPLES32-1+after) & ~(KDU_ALIGN_SAMPLES32-1)));
            num_bytes <<= 2;            
          }
        if (((before | after | num_requests | num_bytes) & 0x80000000) ||
            ((num_requests > 1) && (num_bytes > (INT_MAX / num_requests))))
          pre_alloc_overflow = true;
        num_bytes *= num_requests;
        bytes_reserved += num_bytes;
        if (bytes_reserved < offset)
          pre_alloc_overflow = true;
        return offset;        
      }
      /* [SYNOPSIS]
           Reserves enough storage for `num_requests' later calls to `alloc16'
           (if `use_shorts' is true) or `alloc32' (if `use_shorts' is false).
           Space is reserved such that each of these `num_requests' allocations
           can return an appropriately aligned pointer to an array which offers
           entries at locations n in the range -`before' <= n < `after', where
           each entry is of type `kdu_sample16' (if `use_shorts'=true) or
           `kdu_sample32' (if `use_shorts'=false).
           [//]
           Reservation of storage proceeds with the following alignment
           considerations in mind:
           [>>] Entry 0 must be aligned on a multiple of `KDU_ALIGN_SAMPLES16'
               16-bit samples (if `use_shorts'=true) or `KDU_ALIGN_SAMPLES32'
               32-bit samples (if `use_shorts'=false).  For example, octet
               alignment is achieved if both these alignment constants are
               equal to 8 (they will never be smaller than 8), and this is
               sufficient for aligned vector processing on SIMD architectures
               up to and including AVX.  For SIMD processing with AVX2
               instructions, the `KDU_ALIGN_SAMPLES16' macro must be at least
               16, while for vector processing using the Intel MIC instruction
               set, both `KDU_ALIGN_SAMPLES16' and `KDU_ALIGN_SAMPLES32' must
               be at least 16.
           [>>] The space reserved `before' and `after' entry 0 is rounded up
                to an integer multiple of `KDU_ALIGN_SAMPLES16' 16-bit
                samples or `KDU_ALIGN_SAMPLES32' 32-bit samples, as
                appropriate.
           [>>] The entire allocated block of memory also satisfies any
                alignment constraints associated with an immediately
                preceding call to `pre_align', and there is an implicit call
                to pre-align that guarantees alignment on at least a
                `KDU_PREALIGN_BYTES'-byte boundary.
           [//]
           Remember that you can legally read from (but not write to)
           locations that lie up to `KDU_OVERREAD_BYTES' bytes before or
           after the nominal allocated memory block.       
         [RETURNS]
           The returned value is an allocation offset that must be remembered
           by the caller and passed to the `alloc16' or `alloc32' function as
           appropriate.  This allows calls to `alloc16', `alloc32' and
           `alloc_block' to safely proceed in a multi-threaded environment
           without the need for thread synchronization.
      */
    size_t pre_alloc_block(size_t num_bytes)
      { 
        pre_align(KDU_PREALIGN_BYTES);
        size_t offset = bytes_reserved;
        bytes_reserved += num_bytes;
        if (bytes_reserved < offset)
          pre_alloc_overflow = true;
        return offset;
      }
      /* [SYNOPSIS]
           This function is provided to allow for allocation of general
           storage, a little bit like malloc.  The pre-allocated block of
           memory is aligned at least on a `KDU_PREALIGN_BYTES'-byte boundary,
           which should be sufficient at least to offer `KDU_ALIGN_SAMPLES16'
           sample alignment for 16-bit samples and `KDU_ALIGN_SAMPLES32'
           sample alignment for 32-bit samples.  You can also enforce
           stricter alignment by calling `pre_align' prior to this function.
           [//]
           Remember that you can legally read from (but not write to)
           locations that lie up to `KDU_OVERREAD_BYTES' bytes before or
           after the nominal allocated memory block.       
         [RETURNS]
           The returned value is an allocation offset that must be
           remembered by the caller and passed to the `alloc_block' function
           as appropriate.  This allows calls to `alloc16', `alloc32' and
           `alloc_block' to safely proceed in a multi-threaded environment
           without the need for thread synchronization.
      */
    void finalize(kdu_codestream codestream)
      { 
        if (pre_alloc_overflow)
          codestream.mem_failure("Core sample processing",
                                 "internal error (numerical overflow)");
        assert(pre_creation_phase); pre_creation_phase = false;
        if ((bytes_reserved > buffer_size) ||
            (alignment_reserved > buffer_alignment))
          { // Otherwise, use the previously allocated buffer.
            buffer_size = bytes_reserved;
            buffer_alignment = alignment_reserved;
            if (buf_handle != NULL) delete[] buf_handle;
            buffer=buf_handle = new kdu_byte[buffer_size+3*buffer_alignment];
            buffer += (-_addr_to_kdu_int32(buffer)) & (buffer_alignment-1);
            buffer += buffer_alignment;
          }
        assert((bytes_reserved == 0) || (buffer != NULL));
      }
      /* [SYNOPSIS]
           Call this function after all pre-allocation (calls to
           `pre_alloc' and/or `pre_alloc_block') has been completed.  The
           function performs the actual allocation of heap memory, if
           necessary.
           [//]
           From KDU-7.3.1 onwards, this function requires you to pass in
           a non-empty `codestream' interface which is used to report any
           internal errors through `codestream.mem_failure'.
      */
    kdu_sample16 *alloc16(int before, int after, size_t alloc_off,
                          int inst=0) const
      { 
        assert(!pre_creation_phase);
        assert(!(alloc_off & (KDU_PREALIGN_BYTES-1)));
        before = ((before+KDU_ALIGN_SAMPLES16-1) & ~(KDU_ALIGN_SAMPLES16-1));
        size_t num_samples =
          before + ((after+KDU_ALIGN_SAMPLES16-1) & (KDU_ALIGN_SAMPLES16-1));
        assert((alloc_off + ((num_samples*(inst+1))<<1)) <= bytes_reserved);
        kdu_sample16 *result = (kdu_sample16 *)(buffer+alloc_off);
        return result + before + num_samples*inst;        
      }
      /* [SYNOPSIS]
           This function completes the allocation of an array that was
           pre-allocated using the `pre_alloc' function.  If the
           `num_requests' value passed to `pre_alloc' was greater than 1,
           the `inst' argument should be used to identify which instance
           (in the range 0 to `num_requests'-1) is being allocated here.
           The `alloc_off' argument must be identical to the value returned
           by the call to `pre_alloc' and the `before' and `after' arguments
           must be identical to those passed to `pre_alloc'.
           [//]
           Before calling this function, you must have invoked `finalize'
           (after performing all the relevant `pre_alloc' calls).
           [//]
           Assuming you have correctly passed in the `alloc_off' value
           received from `pre_alloc', the returned pointer is guaranteed to
           have the required alignment properties.
           [//]
           This function can safely be invoked from multiple threads without
           taking out any mutual exclusion locks because it does not actually
           modify the state of the internal object.  This is possible because
           the allocation offset was generated during the call to `pre_alloc'
           and is passed in here.  This feature/requirement is new to
           Kakadu version 7.0
      */
    kdu_sample32 *alloc32(int before, int after, size_t alloc_off,
                          int inst=0) const
      { 
        assert(!pre_creation_phase);
        assert(!(alloc_off & (KDU_PREALIGN_BYTES-1)));
        before = ((before+KDU_ALIGN_SAMPLES32-1) & ~(KDU_ALIGN_SAMPLES32-1));
        size_t num_samples =
          before + ((after+KDU_ALIGN_SAMPLES32-1) & (KDU_ALIGN_SAMPLES32-1));
        assert((alloc_off + ((num_samples*(inst+1))<<2)) <= bytes_reserved);
        kdu_sample32 *result = (kdu_sample32 *)(buffer+alloc_off);
        return result + before + num_samples*inst;
      }
      /* [SYNOPSIS]
           Same as `alloc16', except that it allocates storage for arrays of
           32-bit quantities.  The `alloc_off' value supplied here must have
           been obtained from a call to `pre_alloc' in which the `use_shorts'
           argument was false.
      */
    void *alloc_block(size_t alloc_off, size_t num_bytes) const
      {
        assert((!pre_creation_phase) &&
               ((alloc_off+num_bytes) <= bytes_reserved));
        return buffer+alloc_off;
      }
      /* [SYNOPSIS]
           Similar to `alloc16' and `alloc32', except that it allocates
           storage for memory blocks that were pre-allocated using
           `pre_alloc_block'.  The `alloc_off' value must match that
           returned by the `pre_alloc_block' function.
      */
    size_t get_size() const
      {
        return buffer_size;
      }
      /* [SYNOPSIS]
           For memory consumption statistics.  Returns the total amount of
           heap memory allocated by this object.  The heap memory is allocated
           within calls to `finalize'; it either grows or stays the same in
           each successive call to `finalize', such calls being interspersed
           by invocations of the `restart' member.
      */
  private: // Data
    bool pre_creation_phase; // True if in the pre-creation phase.
    bool pre_alloc_overflow; // True if had numeric overflow in pre-alloc phase
    size_t bytes_reserved;
    size_t buffer_size; // Must be >= `bytes_reserved' except in precreation
    size_t alignment_reserved; // Max alignment multiple
    size_t buffer_alignment; // >= `alignment_reserved' except in precreation
    kdu_byte *buffer; // Aligned according to `buffer_alignment'
    kdu_byte *buf_handle; // Handle for deallocating buffer
  };

/*****************************************************************************/
/*                              kdu_line_buf                                 */
/*****************************************************************************/

#define KD_LINE_BUF_ABSOLUTE     ((kdu_byte) 1)
#define KD_LINE_BUF_SHORTS       ((kdu_byte) 2)
#define KD_LINE_BUF_EXCHANGEABLE ((kdu_byte) 4)

class kdu_line_buf {
  /* [BIND: copy]
     [SYNOPSIS]
     Instances of this structure manage the buffering of a single line of
     sample values, whether image samples or subband samples.  For the
     reversible path, samples must be absolute 32- or 16-bit integers.  For
     irreversible processing, samples have a normalized representation, where
     the nominal range of the data is typically -0.5 to +0.5 (actual nominal
     ranges for the data supplied to a `kdu_encoder' or `kdu_analysis' object
     or retrieved from a `kdu_decoder' or `kdu_synthesis' object may be
     explicitly set in the relevant constructor).  These normalized quantities
     may have either a true 32-bit floating point representation or a 16-bit
     fixed-point representation.  In the latter case, the least significant
     `KDU_FIX_POINT' bits of the integer are interpreted as binary fraction
     bits, so that 2^{KDU_FIX_POINT} represents a normalized value of 1.0.
     [//]
     The object maintains sufficient space to allow access to a given number
     of samples before the first nominal sample (the one at index 0) and a
     given number of samples beyond the last nominal sample (the one at
     index `width'-1).  These extension lengths are explained in the
     comments appearing with `pre_create'.
     [//]
     From Kakadu version 7, this object supports "exchangeable" line buffers,
     which provide a means for passing data across interfaces without copying
     the sample values themselves.  When passed across a `kdu_push_ifc::push'
     or `kdu_pull_ifc::pull' interface, an exchangeable line buffer might
     be copied to/from a resource managed by the target object or else the
     underlying memory buffer might be exchanged with a compatible buffer
     managed by the target object.  Compatible buffers must the same `width',
     the same data type, and the same nominal `extend_width' and
     `extend_right' parameters.  To set things up for more consistent
     exchange of buffers, Kakadu version 7's sample data processing objects
     are moving towards the use of 0 for `extend_left' and `extend_right'.
     This is accomplished by doing boundary extension operations on-the-fly
     wherever possible, rather than through memory transactions.
     [//]
     Also, from Kakadu version 7, the final allocation of line buffers
     (after pre-allocation) can be performed from any thread of execution
     without needing to take out any mutual exclusion locks.  This is part
     of the effort to reduce thread dependencies in Kakadu, so that multiple
     CPU cores can be used more effectively.
  */
  // --------------------------------------------------------------------------
  public: // Life cycle functions
    kdu_line_buf() { destroy(); }
    void destroy()
      {
      /* [SYNOPSIS]
           Restores the object to its uninitialized state ready for a new
           `pre_create' call.  Does not actually destroy any storage, since
           the `kdu_sample_allocator' object from which storage is served
           does not permit individual deallocation of storage blocks.
      */
        width=0; flags=0; pre_created=0; alloc_off = 0;
        allocator=NULL; buf16=NULL; buf32=NULL;
      }
    void pre_create(kdu_sample_allocator *allocator,
                    int width, bool absolute, bool use_shorts,
                    int extend_left, int extend_right)
      { 
      /* [SYNOPSIS]
           Declares the characteristics of the internal storage which will
           later be created by `create'.  If `use_shorts' is true, the sample
           values will have 16 bits each and normalized values will use a
           fixed point representation with KDU_FIX_POINT fraction bits.
           Otherwise, the sample values have 32 bits each and normalized
           values use a true floating point representation.
           [//]
           This function essentially calls `allocator->pre_alloc', requesting
           enough storage for a line with `width' samples, providing for legal
           accesses up to `extend_left' samples before the beginning of the
           line and `extend_right' samples beyond the end of the line.
           [//]
           Note: from Kakadu version 7, the `extend_left' and `extend_right'
           arguments no longer have defaults, because we wish to encourage
           the use of 0 extensions whenever this is not too difficult.
           [//]
           The returned line buffer is guaranteed to be aligned on an L-byte
           boundary, the `extend_left' and `extend_right' values are rounded
           up to the nearest multiple of L bytes, and the length of the
           right-extended buffer is also rounded up to a multiple of L bytes,
           where L is 2*`KDU_ALIGN_SAMPLES16' for 16-bit samples
           (`use_shorts'=true) and 4*`KDU_ALIGN_SAMPLES32' for 32-bit samples
           (`use_shorts'=false).  Finally, it is possible to read at least
           `KDU_OVERREAD_BYTES' bytes beyond the end or before the start of
           the extended region, although writes to these extra bytes will
           generally overwrite data belonging to other buffers allocated
           by the `allocator' object.       
         [ARG: allocator]
           Pointer to the object which will later be used to complete the
           allocation of storage for the line.  The pointer is saved
           internally until such time as the `create' function is called, so
           you must be careful not to delete this object.  You must also be
           careful to call its `kdu_sample_allocator::finalize' function
           before calling `create'.
         [ARG: width]
           Nominal width of (number of samples in) the line.  Note that space
           reserved for access to `extend_left' samples to the left and
           `extend_right' samples to the right.  Moreover, additional
           samples may often be accessed to the left and right of the nominal
           line boundaries due to the alignment policy discussed above.
         [ARG: absolute]
           If true, the sample values in the line buffer are to be used in
           JPEG2000's reversible processing path, which works with absolute
           integers.  otherwise, the line is prepared for use with the
           irreversible processing path, which works with normalized
           (floating or fixed point) quantities.
         [ARG: use_shorts]
           If true, space is allocated for 16-bit sample values
           (array entries will be of type `kdu_sample16').  Otherwise, the
           line buffer will hold samples of type `kdu_sample32'.
         [ARG: extend_left]
           This quantity should be small, since it will be represented
           internally using 8-bit numbers, after rounding up to an
           appropriately aligned value.  It would be unusual to select
           values larger than 16 or perhaps 32.  If you intend to use this
           buffer in an exchange operation (see `set_exchangeable'), it is
           most likely best to use 0 for the `extend_left' and `extend_right'
           to encourage compatibility with the buffer with which you want
           to perform an exchange.
         [ARG: extend_right]
           This quantity should be small, since it will be represented
           internally using 8-bit numbers, after rounding up to an
           appropriately aligned value.  It would be unusual to select
           values larger than 16 or perhaps 32.
      */
        assert((!pre_created) && (this->allocator == NULL));
        if (use_shorts)
          extend_right =
            (extend_right+KDU_ALIGN_SAMPLES16-1) & ~(KDU_ALIGN_SAMPLES16-1);
        else
          extend_right =
            (extend_right+KDU_ALIGN_SAMPLES32-1) & ~(KDU_ALIGN_SAMPLES32-1);
        assert((extend_left <= 255) && (extend_right <= 255));
        this->width=width;
        flags = (use_shorts)?KD_LINE_BUF_SHORTS:0; 
        flags |= (absolute)?KD_LINE_BUF_ABSOLUTE:0;
        this->allocator = allocator;
        buf_before = (kdu_byte) extend_left; // Rounded up automatically
        buf_after = (kdu_byte) extend_right;
        alloc_off=allocator->pre_alloc(use_shorts,buf_before,width+buf_after);
        pre_created = 1;
      }
    void create()
      {
      /* [SYNOPSIS]
           Finalizes creation of storage which was initiated by `pre_create'.
           Does nothing at all if the `pre_create' function was not called,
           or the object was previously created and has not been destroyed.
           Otherwise, you may not call this function until the
           `kdu_sample_allocator' object supplied to `pre_create' has had
           its `finalize' member function called.
           [//]
           Note: from Kakadu version 7, calls to this function need not
           be guarded by mutual exclusion objects, even when invoked from
           multiple threads concurrently.
      */
        if (!pre_created)
          return;
        pre_created = 0;
        size_t off = alloc_off;
        orig_allocator = allocator;
        if (flags & KD_LINE_BUF_SHORTS)
          buf16 = allocator->alloc16(buf_before,width+buf_after,off);
        else
          buf32 = allocator->alloc32(buf_before,width+buf_after,off);
      }
    int check_status()
      {
      /* [SYNOPSIS]
           Returns 1 if `create' has been called, -1 if only `pre_create' has
           been called, 0 if neither has been called since the object was
           constructed or since the last call to `destroy'.
      */
        if (pre_created) return -1;
        else if (buf != NULL) return 1;
        else return 0;
      }
    void set_exchangeable() { flags |= KD_LINE_BUF_EXCHANGEABLE; }
      /* [SYNOPSIS]
           This function simply marks the line buffer as "exchangeable".
           Otherwise, attempts to pass this object to another line buffer's
           `exchange' function will return false, doing nothing.  Typically,
           the caller uses this function to mark buffers that are passed to
           a `kdu_push_ifc::push' or `kdu_pull_ifc::pull' call as resources
           that it is happy to have exchanged with a compatible resource
           that is managed by the target object.  Of course, if an exchange
           does happen, it will invalidate the buffer pointers you may have
           obtained in previous calls to `get_buf16' or `get_buf32', which
           is why you must explicitly invoke this function to express your
           preparedness for an exchange of the underlying memory resources.
           [//]
           You may call this function any time after `pre_create'.
      */
    bool exchange(kdu_line_buf &src)
      { 
        if ((!(src.flags & KD_LINE_BUF_EXCHANGEABLE)) ||
            ((src.flags ^ flags) & ~KD_LINE_BUF_EXCHANGEABLE) ||
            (src.width != width) || (src.buf_before != buf_before) ||
            (src.buf_after != buf_after) || src.pre_created ||
            (src.buf == NULL) || (buf == NULL) ||
            (src.orig_allocator != orig_allocator))
          return false;
        void *tmp = buf; buf = src.buf; src.buf = tmp;
        return true;
      }
      /* [SYNOPSIS]
           This function does nothing, returning false, unless
           `set_exchangeable' has been involed on `src' and the `src'
           buffer is compatible with the current one.  There is no need to
           invoke `set_exchangeable' on the current object; and its own
           exchangeability is not changed by this function.  If an exchange
           does happen, the underlying memory buffers associated with
           the current and `src' objects are exchanged, and the function
           returns true -- that is all.
           [//]
           The current and `src' objects are considered compatible if they
           were pre-created with identical attributes -- i.e., identical
           sample data attributes, identical `width' and identical
           `extend_left' and `extend_right' attributes.
           [//]
           The current and `src' objects should also have been allocated using
           the same `kdu_sample_allocator' object, but this condition might
           or might not be checked, depending upon the internal structure of
           the object -- i.e., be careful not to invoke this function in
           contexts where different allocators might have been used.
      */
    bool raw_exchange(kdu_sample16 *&raw_buf, int raw_width)
      { 
        if ((!(flags & KD_LINE_BUF_EXCHANGEABLE)) || (this->buf_before!=0) ||
            (raw_width != (this->width+this->buf_after)) ||
            pre_created || (buf16==NULL) || !(flags & KD_LINE_BUF_SHORTS))
          return false;
        kdu_sample16 *tmp = buf16; buf16 = raw_buf; raw_buf = tmp;
        return true;
      }
      /* [SYNOPSIS]
           Similar to `exchange', but this function requires the current
           object to be marked as "exchangeable" (see `set_exchangeable') and
           performs the exchange (if compatible) with a raw buffer identified
           by `raw_buf'.  This function should be used with great care,
           because there is insufficient auxiliary information for the
           function to make absolutely certain that the exchange is not
           dangerous.  The supplied `raw_buf' array is expected to be
           aligned on a multiple of 2*`KDU_ALIGN_SAMPLES16' bytes and span
           `raw_width' samples, rounded up to a whole multiple of
           `KDU_ALIGN_SAMPLES16' 16-bit samples.  The exchange succeeds if
           the current has `left_extend'=0, `width'+`right_extend'=`raw_width',
           a short integer/fixed-point representation, and is marked as
           exchangeable.  The `raw_buf' resource should have been allocated
           using the same `kdu_sample_allocator' as the current object.
      */
    bool raw_exchange(kdu_sample32 *&raw_buf, int raw_width)
      { 
        if ((!(flags & KD_LINE_BUF_EXCHANGEABLE)) || (this->buf_before!=0) ||
            (raw_width != (this->width+this->buf_after)) ||
            pre_created || (buf32==NULL) || (flags & KD_LINE_BUF_SHORTS))
          return false;
        kdu_sample32 *tmp = buf32; buf32 = raw_buf; raw_buf = tmp;
        return true;
      }
      /* [SYNOPSIS]
           See 16-bit version for an explanation.  Note that the `raw_buf'
           array is expected to be aligned on a multiple of
           4*`KDU_ALIGN_SAMPLES32' bytes and have storage that spans
           `raw_width' samples, rounded up to a whole multiple of
           `KDU_ALIGN_SAMPLES32' 32-bit samples.  That buffer should have
           been allocated using the same `kdu_sample_allocator' as the
           current object.
      */
  // --------------------------------------------------------------------------
  public: // Access functions
    kdu_sample32 *get_buf32()
      {
      /* [SYNOPSIS]
           Returns NULL if the sample values are of type `kdu_sample16'
           instead of `kdu_sample32', or the buffer has not yet been
           created.  Otherwise, returns an array which supports accesses
           at least with indices in the range 0 to `width'-1, but typically
           beyond these bounds -- see `pre_create' for an explanation of
           extended access bounds.
           [//]
           Note that the returned pointer is always aligned on a
           4*`KDU_ALIGN_SAMPLES32'-byte boundary.
      */
        return (flags & KD_LINE_BUF_SHORTS)?NULL:buf32;
      }
    kdu_sample16 *get_buf16()
      {
      /* [SYNOPSIS]
           Returns NULL if the sample values are of type `kdu_sample32'
           instead of `kdu_sample16', or the buffer has not yet been
           created.  Otherwise, returns an array which supports accesses
           at least with indices in the range 0 to `width'-1, but typically
           beyond these bounds -- see `pre_create' for an explanation of
           extended access bounds.
           [//]
           Note that the returned pointer is always aligned on a
           2*`KDU_ALIGN_SAMPLES16'-byte boundary.
      */
        return (flags & KD_LINE_BUF_SHORTS)?buf16:NULL;
      }
    bool get_floats(float *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 32-bit normalized
           (floating point) sample representation.  Otherwise, returns true
           and copies the floating point samples into the supplied `buffer'.
           The first copied sample is `first_idx' positions from the start
           of the line.  There may be little or no checking that the
           sample range represented by `first_idx' and `num_samples' is
           legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf32'.
      */
        if (flags & (KD_LINE_BUF_ABSOLUTE | KD_LINE_BUF_ABSOLUTE))
          return false;
        for (int i=0; i < num_samples; i++)
          buffer[i] = buf32[first_idx+i].fval;
        return true;
      }
    bool set_floats(float *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 32-bit normalized
           (floating point) sample representation.  Otherwise, returns true
           and copies the floating point samples from the supplied `buffer'.
           The first sample in `buffer' is stored `first_idx' positions from
           the start of the line.  There may be little or no checking that
           the sample range represented by `first_idx' and `num_samples' is
           legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf32'.
      */
        if (flags & (KD_LINE_BUF_ABSOLUTE | KD_LINE_BUF_ABSOLUTE))
          return false;
        for (int i=0; i < num_samples; i++)
          buf32[i+first_idx].fval = buffer[i];
        return true;
      }
    bool get_ints(kdu_int32 *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 32-bit absolute
           (integer) sample representation.  Otherwise, returns true
           and copies the integer samples into the supplied `buffer'.
           The first copied sample is `first_idx' positions from the start
           of the line.  There may be little or no checking that the
           sample range represented by `first_idx' and `num_samples' is
           legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf32'.
      */
        if ((flags & KD_LINE_BUF_SHORTS) || !(flags & KD_LINE_BUF_ABSOLUTE))
          return false;
        for (int i=0; i < num_samples; i++)
          buffer[i] = buf32[first_idx+i].ival;
        return true;
      }
    bool set_ints(kdu_int32 *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 32-bit absolute
           (integer) sample representation.  Otherwise, returns true
           and copies the integer samples from the supplied `buffer'.
           The first sample in `buffer' is stored `first_idx' positions from
           the start of the line.  There may be little or no checking that
           the sample range represented by `first_idx' and `num_samples' is
           legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf32'.
      */
        if ((flags & KD_LINE_BUF_SHORTS) || !(flags & KD_LINE_BUF_ABSOLUTE))
          return false;
        for (int i=0; i < num_samples; i++)
          buf32[i+first_idx].ival = buffer[i];
        return true;
      }
    bool get_ints(kdu_int16 *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 16-bit sample
           representation (either fixed point or absolute integers).
           Otherwise, returns true and copies the 16-bit samples into the
           supplied `buffer'.  The first copied sample is `first_idx'
           positions from the start of the line.  There may be little or no
           checking that the sample range represented by `first_idx' and
           `num_samples' is legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf16'.
      */
        if (!(flags & KD_LINE_BUF_SHORTS))
          return false;
        for (int i=0; i < num_samples; i++)
          buffer[i] = buf16[first_idx+i].ival;
        return true;
      }
    bool set_ints(kdu_int16 *buffer, int first_idx, int num_samples)
      {
      /* [SYNOPSIS]
           Returns false if the object does not hold a 16-bit sample
           representation (either fixed point or absolute integers).
           Otherwise, returns true and copies the integer samples from the
           supplied `buffer'.  The first sample in `buffer' is stored
           `first_idx' positions from the start of the line.  There may be
           little or no checking that the sample range represented by
           `first_idx' and `num_samples' is legal, so be careful.
           [//]
           For native C/C++ interfacing, it is more efficient to explicitly
           access the internal buffer using `get_buf16'.
      */
        if (!(flags & KD_LINE_BUF_SHORTS))
          return false;
        for (int i=0; i < num_samples; i++)
          buf16[i+first_idx].ival = buffer[i];
        return true;
      }
    int get_width()
      {
      /* [SYNOPSIS]
           Returns 0 if the object has not been created.  Otherwise, returns
           the nominal width of the line.  Remember that the arrays returned
           by `get_buf16' and `get_buf32' are typically larger than the actual
           line width, as explained in the comments appearing with the
           `pre_create' function.
      */
        return width;
      }
    bool is_absolute() { return ((flags & KD_LINE_BUF_ABSOLUTE) != 0); }
      /* [SYNOPSIS]
           Returns true if the sample values managed by this object represent
           absolute integers, for use with JPEG2000's reversible processing
           path.
      */
  // --------------------------------------------------------------------------
  private: // Data -- should occupy 12 bytes on a 32-bit machine.
    int width; // Number of samples in buffer.
    kdu_byte buf_before, buf_after;
    kdu_byte flags; // Bit 0 set if absolute_ints;  Bit 1 set if use_shorts
    kdu_byte pre_created; // True after `pre_create' if `create' still pending
    union {
      size_t alloc_off; // Saves `allocator->pre_alloc' return until `create'
      kdu_sample_allocator *orig_allocator; // Valid after `create' is called
    };
    union {
      kdu_sample32 *buf32; // Valid if !KDU_LINE_BUF_SHORTS and !`pre_created'
      kdu_sample16 *buf16; // Valid if KDU_LINE_BUF_SHORTS && !`pre_created'
      void *buf; // Facilitates untyped buffer exchange
      kdu_sample_allocator *allocator; // Valid if `pre_created'
    };
  };

/*****************************************************************************/
/*                            kdu_push_ifc_base                              */
/*****************************************************************************/

class kdu_push_ifc_base {
  protected:
    friend class kdu_push_ifc;
    virtual ~kdu_push_ifc_base() { return; }
    virtual void start(kdu_thread_env *env) = 0;
    virtual void push(kdu_line_buf &line, kdu_thread_env *env) = 0;
  };

/*****************************************************************************/
/*                               kdu_push_ifc                                */
/*****************************************************************************/

#define KDU_LINE_WILL_BE_EXTENDED ((int) 1)

class kdu_push_ifc {
  /* [BIND: reference]
     [SYNOPSIS]
     All classes which support `push' calls derive from this class so
     that the caller may remain ignorant of the specific type of object to
     which samples are being delivered.  The purpose of derivation is usually
     just to introduce the correct constructor.  The objects are actually just
     interfaces to an appropriate object created by the relevant derived
     class's constructor.  The interface directs `push' calls to the internal
     object in a manner which should incur no cost.
     [//]
     The interface objects may be copied at will; the internal object will
     not be destroyed when an interface goes out of scope.  Consequently,
     the interface objects do not have meaningful destructors.  Instead,
     to destroy the internal object, the `destroy' member function must be
     called explicitly.
  */
  public: // Member functions
    kdu_push_ifc() { state = NULL; }
      /* [SYNOPSIS]
           Creates an empty interface.  You must assign the interface to
           one of the derived objects such as `kdu_encoder' or `kdu_analysis'
           before you may use the `push' function.
      */
    void destroy()
      {
        if (state != NULL) delete state;
        state = NULL;
      }
      /* [SYNOPSIS]
           Automatically destroys all objects which were created by the
           relevant derived object's constructor, including lower level
           DWT analysis stages and encoding objects.  Upon return, the
           interface will be empty, meaning that `exists' returns
           false.
      */
    void start(kdu_thread_env *env)
      { /* [SYNOPSIS]
             This function may be called at any point after construction
             of a `kdu_analysis' or `kdu_encoder' object, once you have
             invoked the `kdu_sample_allocator::finalize' function on the
             `kdu_sample_allocator' object used during construction.  In
             particular, this means that you will not be creating any
             further objects to share the storage offered by the sample
             allocator.
             [//]
             For multi-threaded applications (i.e., when `env' is non-NULL),
             calling this function explicitly allows the codestream machinery's
             background processing job to start allocating suitable containers
             to receive code-block bit-streams once they are produced.  Doing
             this ahead of time can avoid contention later on if you have
             a lot of processing threads simultaneously trying to do this.
             [//]
             However, it is not actually necessary to call this function
             explicitly.
             [//]
             The `kdu_multi_analysis' object automatically invokes `start'
             on all encoder/analysis objects that it creates.
         */
        state->start(env);
      }
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
           Returns false until the object is assigned to one constructed
           by one of the derived classes, `kdu_analysis' or `kdu_encoder'.
           Also returns false after `destroy' returns.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
           Opposite of `exists', returning true if the object has not been
           assigned to one of the more derived objects, `kdu_analysis' or
           `kdu_encoder', or after a call to `destroy'.
      */
    kdu_push_ifc &operator=(kdu_analysis rhs);
      /* [SYNOPSIS]
           It is safe to directly assign a `kdu_analysis' object to a
           `kdu_push_ifc' object, since both are essentially just references
           to an anonymous internal object.
      */
    kdu_push_ifc &operator=(kdu_encoder rhs);
      /* [SYNOPSIS]
           It is safe to directly assign a `kdu_encoder' object to a
           `kdu_push_ifc' object, since both are essentially just references
           to an anonymous internal object.
      */
    void push(kdu_line_buf &line, kdu_thread_env *env=NULL)
      { /* [SYNOPSIS]
             Delivers all samples from the `line' buffer across the interface.
             [//]
             From KDU 7, the `line' buffer can be marked as "exchangeable'
             using `kdu_line_buf::set_exchangeable', in which case the
             internal implementation of this function might exchange the
             supplied `line' object's internal memory buffer with one of
             its own, rather than copying sample values one by one.  Of
             course, this is generally more efficient, but you should be
             careful not to pass a `line' object marked as exchangeable if
             you actually need access to the data, or if you are keeping a
             copy of the internal memory pointer obtained via
             `line.get_buf16' or `line.get_buf32', or if `line' was allocated
             using a different `kdu_sample_allocator' object to the one
             used to construct the target object.
           [ARG: env]
             If the object was constructed for multi-threaded processing
             (see the constructors for `kdu_analysis' and `kdu_encoder'),
             you MUST pass a non-NULL `env' argument in here, identifying
             the thread which is performing the `push' call.  Otherwise,
             the `env' argument should be ignored.
        */
        state->push(line,env);
      }
  protected: // Data
    kdu_push_ifc_base *state;
  };

/*****************************************************************************/
/*                            kdu_pull_ifc_base                              */
/*****************************************************************************/

class kdu_pull_ifc_base {
  protected:
    friend class kdu_pull_ifc;
    virtual ~kdu_pull_ifc_base() { return; }
    virtual bool start(kdu_thread_env *env) = 0;
    virtual void pull(kdu_line_buf &line, kdu_thread_env *env) = 0;
  };

/*****************************************************************************/
/*                               kdu_pull_ifc                                */
/*****************************************************************************/

class kdu_pull_ifc {
  /* [BIND: reference]
     [SYNOPSIS]
     All classes which support `pull' calls derive from this class so
     that the caller may remain ignorant of the specific type of object from
     which samples are to be recovered.  The purpose of derivation is usually
     just to introduce the correct constructor.  The objects are actually just
     interfaces to an appropriate object created by the relevant derived
     class's constructor.  The interface directs `pull' calls to the internal
     object in a manner which should incur no cost.
     [//]
     The interface objects may be copied at will; the internal object will
     not be destroyed when an interface goes out of scope.  Consequently,
     the interface objects do not have meaningful destructors.  Instead,
     to destroy the internal object, the `destroy' member function must be
     called explicitly.
  */
  public: // Member functions
    kdu_pull_ifc() { state = NULL; }
      /* [SYNOPSIS]
           Creates an empty interface.  You must assign the interface to
           one of the derived objects such as `kdu_decoder' or `kdu_synthesis'
           before you may use the `pull' function.
      */
    void destroy()
      {
        if (state != NULL) delete state;
        state = NULL;
      }
      /* [SYNOPSIS]
           Automatically destroys all objects which were created by this
           relevant derived object's constructor, including lower level
           DWT synthesis stages and decoding objects.  Upon return, the
           interface will be empty, meaning that `exists' returns false.
      */
    bool start(kdu_thread_env *env)
      { /* [SYNOPSIS]
             This function may be called at any point after construction
             of a `kdu_synthesis' or `kdu_decoder' object, once you have
             invoked the `kdu_sample_allocator::finalize' function on the
             `kdu_sample_allocator' object used during construction.  In
             particular, this means that you will not be creating any
             further objects to share the storage offered by the sample
             allocator.
             [//]
             For multi-threaded applications (i.e., when `env' is non-NULL),
             this function allows code-block processing to be started
             immediately, which can help maximize throughput.
             [//]
             NOTE CAREFULLY: You should call this function REPEATEDLY
             (possibly inerleaved with other calls) UNTIL IT RETURNS TRUE,
             PRIOR TO the first call to `pull'.  If you choose to not call
             the function at all, that is OK, but in that case the function
             will automatically be called from within the first call to
             `pull'.  This function is not itself thread-safe, so you
             must be sure that another thread does not invoke `pull' while
             you are invoking this function.
             [//]
             It is not completely necessary to call this function, unless
             another data processing object is set up to defer the scheduling
             of jobs until dependencies associated with `kdu_decoder' objects
             are satisfied; in that case, if you fail to call this function,
             there will never be a call to `pull', assuming that this happens
             only when dependencies are satisifed.  Otherwise, in the event
             that you do not call this function, it will be invoked
             automatically when `pull' is first called.
             [//]
             The `kdu_multi_synthesis' object automatically invokes `start'
             on all decoder/synthesis objects that it creates, arranging
             for all calls to `start' to occur before any synchronous or
             asynchronous call to `pull'.
             [//]
             For applications which are not multi-threaded (i.e., when `env'
             is NULL) there is no particular benefit to calling this
             function, but you can if you like.
         */
        return state->start(env);
      }
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
           Returns false until the object is assigned to one constructed
           by one of the derived classes, `kdu_synthesis' or `kdu_decoder'.
           Also returns false after `destroy' returns.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
           Opposite of `exists', returning true if the object has not been
           assigned to one of the more derived objects, `kdu_synthesis' or
           `kdu_decoder', or after a call to `destroy'.
      */
    kdu_pull_ifc &operator=(kdu_synthesis rhs);
      /* [SYNOPSIS]
           It is safe to directly assign a `kdu_synthesis' object to a
           `kdu_pull_ifc' object, since both are essentially just references
           to an anonymous internal object.
      */
    kdu_pull_ifc &operator=(kdu_decoder rhs);
      /* [SYNOPSIS]
           It is safe to directly assign a `kdu_decoder' object to a
           `kdu_pull_ifc' object, since both are essentially just references
           to an anonymous internal object.
      */
    void pull(kdu_line_buf &line, kdu_thread_env *env=NULL)
      { /* [SYNOPSIS]
             Fills out the supplied `line' buffer's sample values before
             returning.
             [//]
             From KDU 7, the `line' buffer can be marked as "exchangeable'
             using `kdu_line_buf::set_exchangeable', in which case the
             internal implementation of this function might exchange the
             supplied `line' object's internal memory buffer with one of
             its own, rather than copying sample values one by one.  Of
             course, this is generally more efficient, but you should be
             careful not to pass a `line' object marked as exchangeable if
             you are keeping a separate copy of the buffer returned by
             `line.get_buf16' or `line.get_buf32', or if `line' was allocated
             using a different `kdu_sample_allocator' object to the one
             used to construct the target object.
           [ARG: env]
             If the object was constructed for multi-threaded processing
             (see the constructors for `kdu_synthesis' and `kdu_decoder'),
             you MUST pass a non-NULL `env' argument in here, identifying
             the thread which is performing the `pull' call.  Otherwise,
             the `env' argument should be ignored.
        */
        state->pull(line,env);
      }
  protected: // Data
    kdu_pull_ifc_base *state;
  };

/*****************************************************************************/
/*                              kdu_analysis                                 */
/*****************************************************************************/

class kdu_analysis : public kdu_push_ifc {
  /* [BIND: reference]
     [SYNOPSIS]
     Implements the subband analysis processes associated with a single
     DWT node (see `kdu_node').  A complete DWT decomposition tree is built
     from a collection of these objects, each containing a reference to
     the next stage.   The complete DWT tree and all required `kdu_encoder'
     objects may be created by a single call to the constructor,
     `kdu_analysis::kdu_analysis'.
  */
  public: // Member functions
    KDU_EXPORT
      kdu_analysis(kdu_node node, kdu_sample_allocator *allocator,
                   bool use_shorts, float normalization=1.0F,
                   kdu_roi_node *roi=NULL, kdu_thread_env *env=NULL,
                   kdu_thread_queue *env_queue=NULL,
                   int flags=0);
      /* [SYNOPSIS]
           Constructing an instance of this class for the primary node of
           a tile-component's highest visible resolution, will cause the
           constructor to recursively create instances of the class for
           each successive DWT stage and also for the block encoding process.
           [//]
           The recursive construction process supports all wavelet
           decomposition structures allowed by the JPEG2000 standard,
           including packet wavelet transforms, and transforms with
           different horizontal and vertical downsampling factors.  The
           `node' object used to construct the top level `kdu_analysis'
           object will typically be the primary node of a `kdu_resolution'
           object, obtained by calling `kdu_resolution::access_node'.  In
           fact, for backward compatibility with Kakadu versions 4.5 and
           earlier, a second constructor is provided, which does just this.
           [//]
           The optional `env' and `env_queue' arguments support a variety
           of multi-threaded processing paradigms, to leverage the
           capabilities of multi-processor platforms.  To see how this works,
           consult the description of these arguments below.
         [ARG: node]
           Interface to the DWT decomposition node for which the object is
           being created.  The analysis stage decomposes the image entering
           that node into one subband for each of the node's children.
           If the child node is a leaf (a final subband), a `kdu_encoder'
           object is created to receive the data produced in that subband.
           Otherwise, another `kdu_analysis' object is recursively
           constructed to process the subband data produced by the present
           node.
         [ARG: allocator]
           A `kdu_sample_allocator' object whose `finalize' member function
           has not yet been called must be supplied for pre-allocation of the
           various sample buffering arrays.  This same allocator will be
           shared by the entire DWT tree and by the `kdu_encoder' objects at
           its leaves.
         [ARG: use_shorts]
           Indicates whether 16-bit or 32-bit data representations are to be
           used.  The same type of representation must be used throughput the
           DWT processing chain and line buffers pushed into the DWT engine
           must use this representation.
         [ARG: normalization]
           Ignored for reversibly transformed data.  In the irreversible case,
           it indicates that the nominal range of data pushed into the
           `kdu_push_ifc::push' function will be from -0.5*R to 0.5*R, where
           R is the value of the `normalization' argument.  This
           capability is provided primarily to allow normalization steps to
           be skipped or approximated with simple powers of 2 during lifting
           implementations of the DWT; the factors can be folded into
           quantization step sizes.  The best way to use the normalization
           argument will generally depend upon the implementation of the DWT.
         [ARG: roi]
           If non-NULL, this argument points to an appropriately
           derived ROI node object, which may be used to recover region of
           interest mask information for the present tile-component.  In this
           case, the present function will automatically construct an ROI
           processing tree to provide access to derived ROI information in
           each individual subband.  The `roi::release' function will
           be called when the present object is destroyed -- possibly
           sooner (if it can be determined that ROI information is no
           longer required).
         [ARG: env]
           Supply a non-NULL argument here if you want to enable
           multi-threaded processing.  After creating a cooperating
           thread group by following the procedure outlined in the
           description of `kdu_thread_env', any one of the threads may
           be used to construct this processing engine.  However, you
           MUST TAKE CARE to create all objects which share the same
           `allocator' object from the same thread.
           [//]
           Separate processing queues will automatically be created for each
           subband, allowing multiple threads to be scheduled
           simultaneously to process code-block data for the corresponding
           tile-component.  Also, multiple tile-components may be processed
           concurrently and the available thread resources will be allocated
           amongst the total collection of job queues as required.
           [//]
           By and large, you can use this object in exactly the same way
           when `env' is non-NULL as you would with a NULL `env' argument.
           That is, the use of multi-threaded processing can be largely
           transparent.  However, you must remember the following three
           points:
           [>>] All objects which share the same `allocator' must be created
                using the same thread.  Thereafter, the actual processing
                may proceed on different threads.
           [>>] You must supply a non-NULL `env' argument to the
                `kdu_push_ifc::push' function -- it need not refer to the
                same thread as the one used to create the object here, but
                it must belong to the same thread group.
           [>>] You cannot rely upon all processing being complete until you
                invoke the `kdu_thread_entity::join' or
                `kdu_thread_entity::terminate' function.
         [ARG: env_queue]
           This argument is ignored unless `env' is non-NULL.  When `env'
           is non-NULL, all thread queues which are created inside this object
           are added as sub-queues of `env_queue'.  If `env_queue' is NULL,
           all thread queues which are created inside this object are added
           as top-level queues in the multi-threaded queue hierarchy.  The
           `kdu_analysis' object does not directly create any
           `kdu_thread_queue' objects, but it passes `env_queue' along to
           the `kdu_encoder' objects that it constructs to process each
           transformed subband and each of those objects does create a
           thread queue which is made a descendant of `env_queue'.
           [//]
           One advantage of supplying a non-NULL `env_queue' is that it
           provides you with a single hook for joining with the completion
           of all the thread queues which are created by this object
           and its descendants -- see `kdu_thread_entity::join' for more on
           this.
           [//]
           A second advantage of supplying a non-NULL `env_queue' is that
           it allows you to manipulate the sequencing indices that are
           assigned by the thread queues created internally -- see
           `kdu_thread_entity::attach_queue' for more on the role played by
           sequencing indices in controlling the order in which work is
           actually done.  In particular, if `env_queue' is initially
           added to the thread group with a sequencing index of N >= 0, each
           `kdu_encoder' object created as a result of the present call will
           also have a sequencing index of N.
           [//]
           Finally, and perhaps most importantly, the `env_queue' object
           supplied here provides a mechanism to determine whether or not
           calls to `push' might potentially block.  This is achieved by
           means of the `env_queue->update_dependencies' function that is
           invoked from within the `kdu_encoder' object, following the
           conventions outlined with the definition of the `kdu_encoder'
           object's constructor.  What this means is that if
           `env_queue->check_dependencies' returns false, the next call to
           this object's `kdu_push_ifc::push' function should not block the
           caller.  The `kdu_multi_analysis' object uses a derived
           `kdu_thread_queue' object to automatically schedule DWT analysis
           jobs only once it knows that they will not be blocked by missing
           dependencies.
         [ARG: flags]
           Used to access extended functionality.  Currently, the only
           defined flags are:
           [>>] `KDU_LINE_WILL_BE_EXTENDED' -- if this flag is defined,
                the caller is intending to push in lines that have been
                created with an `extend_right' value of 1.  That is,
                `kdu_line_buf' objects supplied to the `push' function will
                have an extra sample beyond the nominal end of the
                line in question.  Knowing this, the current object may
                allocate internal storage to have the same extended length
                so as to maximize the chance that internal calls to
                `kdu_line_buf::exchange' will succeed in performing an
                efficient data exchange without copying of sample values.
                This flag is provided primarily to allow efficient DWT
                implementations to work with buffers that have an equal
                amount of storage for low- and high-pass horizontal subbands.
                In practice, the flag will only be set for horizontal low
                (resp. high) subbands that are shorter (by 1) than the
                corresponding horizontal high (resp. low) subband, where
                the longer subband cannot be spanned by the same number
                of octets as the shorter subband; this is a relatively unusual
                condition, but still worth catering for. The application
                itself would not normally set this flag.
      */
    KDU_EXPORT
      kdu_analysis(kdu_resolution resolution, kdu_sample_allocator *allocator,
                   bool use_shorts, float normalization=1.0,
                   kdu_roi_node *roi=NULL, kdu_thread_env *env=NULL,
                   kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Same as the first form of the constructor, but the required
           `kdu_node' interface is recovered from `resolution.access_node'.
         [ARG: resolution]
           Interface to the top visible resolution level from which
           DWT analysis is to be performed.  For analysis, this should almost
           invariably be the actual top level resolution of a tile-component.
           For synthesis, the corresponding constructor might be supplied
           a lower resolution object in order to obtain partial synthesis
           to that resolution.
      */
  };

/*****************************************************************************/
/*                              kdu_synthesis                                */
/*****************************************************************************/

class kdu_synthesis : public kdu_pull_ifc {
  /* [BIND: reference]
     [SYNOPSIS]
     Implements the subband synthesis processes associated with a single
     DWT node (see `kdu_node').  A complete DWT synthesis tree is built
     from a collection of these objects, each containing a reference to
     the next stage.   The complete DWT tree and all required `kdu_decoder'
     objects may be created by a single call to the constructor,
     `kdu_synthesis::kdu_synthesis'.
  */
  public: // Member functions
    KDU_EXPORT
      kdu_synthesis(kdu_node node, kdu_sample_allocator *allocator,
                    bool use_shorts, float normalization=1.0F,
                    int pull_offset=0, kdu_thread_env *env=NULL,
                    kdu_thread_queue *env_queue=NULL, int flags=0);
      /* [SYNOPSIS]
           Constructing an instance of this class for the primary node of
           a tile-component's highest visible resolution, will cause the
           constructor to recursively create instances of the class for
           each successive DWT stage and also for the block decoding process.
           [//]
           The recursive construction process supports all wavelet
           decomposition structures allowed by the JPEG2000 standard,
           including packet wavelet transforms, and transforms with
           different horizontal and vertical downsampling factors.  The
           `node' object used to construct the top level `kdu_synthesis'
           object will typically be the primary node of a `kdu_resolution'
           object, obtained by calling `kdu_resolution::access_node'.  In
           fact, for backward compatibility with Kakadu versions 4.5 and
           earlier, a second constructor is provided, which does just this.
           [//]
           This function takes optional `env' and `env_queue' arguments to
           support a variety of multi-threaded processing paradigms, to
           leverage the capabilities of multi-processor platforms.  To see
           how this works, consult the description of these arguments below.
           To initiate processing as soon as possible, you might like to
           call `kdu_pull_ifc::start' once you have finished creating all
           objects which share the supplied `allocator' object and invoked
           its `kdu_sample_allocator::finalize' function.  Otherwise,
           background processing (on other threads) will not commence until
           the first call to `kdu_pull_ifc::pull'.
         [ARG: node]
           Interface to the DWT decomposition node for which the object is
           being created.  The synthesis stage reconstructs the image
           associated with that node by combining the subband images
           produced by each of the node's children.  If the child node is
           a leaf (a final subband), a `kdu_decoder' object is created to
           recover the data for that subband.  Otherwise, another
           `kdu_synthesis' object is recursively constructed to retrieve
           the child's subband data.
         [ARG: allocator]
           A `kdu_sample_allocator' object whose `finalize' member function
           has not yet been called must be supplied for pre-allocation of the
           various sample buffering arrays.  This same allocator will be
           shared by the entire DWT tree and by the `kdu_decoder' objects at
           its leaves.
         [ARG: use_shorts]
           Indicates whether 16-bit or 32-bit data representations are to be
           used.  The same type of representation must be used throughput the
           DWT processing chain and line buffers pulled from the DWT synthesis
           engine must use this representation.
         [ARG: normalization]
           Ignored for reversibly transformed data.  In the irreversible case,
           it indicates that the nominal range of data recovered from the
           `kdu_pull_ifc::pull' function will be from -0.5*R to 0.5*R, where
           R is the value of the `normalization' argument.  This
           capability is provided primarily to allow normalization steps to
           be skipped or approximated with simple powers of 2 during lifting
           implementations of the DWT; the factors can be folded into
           quantization step sizes.  The best way to use the normalization
           argument will generally depend upon the implementation of the DWT.
         [ARG: pull_offset]
           Applications should leave this argument set to 0.  The internal
           implementation uses this to maintain horizontal alignment
           properties for efficient memory access, when synthesizing a
           region of interest within the image.  The first `pull_offset'
           entries in each `kdu_line_buf' object supplied to the `pull'
           function are not used; the function should write the requested
           sample values into the remainder of the line buffer, whose
           width (`kdu_line_buf::get_width') is guaranteed to be `pull_offset'
           samples larger than the width of the region in that subband.
           In any event, offsets should be small, since the internal
           representation stores them and various derived quantities
           using 8-bit fields to keep the memory footprint as small
           as possible.
         [ARG: env]
           Supply a non-NULL argument here if you want to enable
           multi-threaded processing.  After creating a cooperating
           thread group by following the procedure outlined in the
           description of `kdu_thread_env', any one of the threads may
           be used to construct this processing engine.  However, you
           MUST TAKE CARE to create all objects which share the same
           `allocator' object from the same thread.
           [//]
           Separate processing queues will automatically be created for each
           subband, allowing multiple threads to be scheduled simultaneously
           to process code-block samples for the corresponding
           tile-component.  Also, multiple tile-components may be processed
           concurrently and the available thread resources will be allocated
           amongst the total collection of job queues as required.
           [//]
           By and large, you can use this object in exactly the same way
           when `env' is non-NULL as you would with a NULL `env' argument.
           That is, the use of multi-threaded processing can be largely
           transparent.  However, you must remember the following two
           points:
           [>>] All objects which share the same `allocator' must be created
                using the same thread.  Thereafter, the actual processing
                may proceed on different threads.
           [>>] You must supply a non-NULL `env' argument to the
                `kdu_push_ifc::push' function -- it need not refer to the
                same thread as the one used to create the object here, but
                it must belong to the same thread group.
         [ARG: env_queue]
           This argument is ignored unless `env' is non-NULL.  When `env'
           is non-NULL, all job queues which are created inside this object
           are added as sub-queues of `env_queue'.  If `env_queue' is NULL,
           all thread queues which are created inside this object are added
           as top-level queues in the multi-threaded queue hierarchy.  The
           `kdu_synthesis' object does not directly create any
           `kdu_thread_queue' objects, but it passes `env_queue' along to
           the `kdu_decoder' objects that it constructs to decode each
           subband and each of those objects does create a
           thread queue which is made a descendant of `env_queue'.
           [//]
           One advantage of supplying a non-NULL `env_queue' is that it
           provides you with a single hook for joining with the completion
           of all the thread queues which are created by this object
           and its descendants -- see `kdu_thread_entity::join' for more on
           this.
           [//]
           A second advantage of supplying a non-NULL `env_queue' is that
           it allows you to manipulate the sequencing indices that are
           assigned by the thread queues created internally -- see
           `kdu_thread_entity::attach_queue' for more on the role played by
           sequencing indices in controlling the order in which work is
           actually done.  In particular, if `env_queue' is initially
           added to the thread group with a sequencing index of N >= 0, each
           `kdu_decoder' object created as a result of the present call will
           also have a sequencing index of N.
           [//]
           Finally, and perhaps most importantly, the `env_queue' object
           supplied here provides a mechanism to determine whether or not
           calls to `pull' might potentially block.  This is achieved by
           means of the `env_queue->update_dependencies' function that is
           invoked from within the `kdu_decoder' object, following the
           conventions outlined with the definition of the `kdu_decoder'
           object's constructor.  What this means is that if
           `env_queue->check_dependencies' returns false, the next call to
           this object's `kdu_pull_ifc::pull' function should not block the
           caller.  The `kdu_multi_synthesis' object uses a derived
           `kdu_thread_queue' object to automatically schedule DWT synthesis
           jobs only once it knows that they will not be blocked by missing
           dependencies.
         [ARG: flags]
           Used to access extended functionality.  Currently, the only
           defined flags are:
           [>>] `KDU_LINE_WILL_BE_EXTENDED' -- if this flag is defined,
                the caller is intending to pull data into line buffers that
                have been created with an `extend_right' value of 1.  That is,
                `kdu_line_buf' objects supplied to the `pull' function will
                have an extra sample beyond the nominal end of the
                line in question.  Knowing this, the current object may
                allocate internal storage to have the same extended length
                so as to maximize the chance that internal calls to
                `kdu_line_buf::exchange' will succeed in performing an
                efficient data exchange without copying of sample values.
                This flag is provided primarily to allow efficient DWT
                implementations to work with buffers that have an equal
                amount of storage for low- and high-pass horizontal subbands.
                In practice, the flag will only be set for horizontal low
                (resp. high) subbands that are shorter (by 1) than the
                corresponding horizontal high (resp. low) subband, where
                the longer subband cannot be spanned by the same number
                of octets as the shorter subband; this is a relatively unusual
                condition, but still worth catering for. The application
                itself would not normally set this flag.
      */
    KDU_EXPORT
      kdu_synthesis(kdu_resolution resolution, kdu_sample_allocator *allocator,
                    bool use_shorts, float normalization=1.0F,
                    kdu_thread_env *env=NULL,
                    kdu_thread_queue *env_queue=NULL);
      /* [SYNOPSIS]
           Same as the first form of the constructor, but the required
           `kdu_node' interface is recovered from `resolution.access_node'.
         [ARG: resolution]
           Interface to the top visible resolution level to which DWT synthesis
           is to be performed.  This need not necessarily be the
           highest available resolution, so that partial synthesis to some
           lower resolution is supported -- in fact, common in Kakadu.
      */
  };

/*****************************************************************************/
/*                              kdu_encoder                                  */
/*****************************************************************************/

class kdu_encoder: public kdu_push_ifc {
    /* [BIND: reference]
       [SYNTHESIS]
       Implements quantization and block encoding for a single subband,
       inside a single tile-component.
    */
  public: // Member functions
    KDU_EXPORT
      kdu_encoder(kdu_subband subband, kdu_sample_allocator *allocator,
                  bool use_shorts, float normalization=1.0F,
                  kdu_roi_node *roi=NULL, kdu_thread_env *env=NULL,
                  kdu_thread_queue *env_queue=NULL, int flags=0);
      /* [SYNOPSIS]
           Informs the encoder that data supplied via its `kdu_push_ifc::push'
           function will have a nominal range from -0.5*R to +0.5*R where R
           is the value of `normalization'.  The `roi' argument, if non-NULL,
           provides an appropriately derived `kdu_roi_node' object whose
           `kdu_roi_node::pull' function may be used to recover ROI mask
           information for this subband.  Its `kdu_roi::release' function will
           be called when the encoder is destroyed -- possibly sooner, if it
           can be determined that ROI information is no longer required.
           [//]
           The optional `env' and `env_queue' arguments support a variety
           of multi-threaded processing paradigms, to leverage the
           capabilities of multi-processor platforms.  To see how this works,
           consult the description of these arguments below.
         [ARG: env]
           If non-NULL, the behaviour of the underlying
           `kdu_push_ifc::push' function is changed radically.  In
           particular, a job queue is created by this constructor, to enable
           asynchronous multi-threaded processing of the code-block samples.
           Once sufficient lines have been pushed to the subband to enable
           the encoding of a row of code-blocks, the processing of these
           code-blocks is not done immediately, as it is if `env' is NULL.
           Instead, one or more jobs are added to the mentioned queue,
           to be serviced by any available thread in the group to which
           `env' belongs.  You should remember the following three
           points:
           [>>] All objects which share the same `allocator' must be created
                using the same thread.  Thereafter, the actual processing
                may proceed on different threads.
           [>>] You must supply a non-NULL `env' argument to the
                `kdu_push_ifc::push' function -- it need not refer to the
                same thread as the one used to create the object here, but
                it must belong to the same thread group.
           [>>] You cannot rely upon all processing being complete until you
                invoke the `kdu_thread_entity::join' or
                `kdu_thread_entity::terminate' function.
         [ARG: env_queue]
           If `env' is NULL, this argument is ignored; otherwise, the job
           queue which is created by this constructor will be made a
           sub-queue of any supplied `env_queue'.  If `env_queue' is NULL,
           the queue created to process code-blocks within this
           tile-component-subband will be a top-level queue in the
           multi-threaded queue hierarchy.
           [//]
           The `env_queue->update_dependencies' function is invoked with
           a `new_dependencies' value of 1 whenever a call to `push'
           causes this object's internal subband sample buffer to become
           full, so that a subsequent call to `push' might require the caller
           to block until the buffer has been cleared by block encoding
           operations.  The `env_queue->update_dependencies' function is
           invoked with a `new_dependencies' value of -1 whenever block
           encoding operations cause a previously full subband sample buffer
           to become available to receive new data, so that a subsequent
           call to `push' will not block the caller.
           [//]
           For more on the role and benefits of the `env_queue' argument,
           see the discussion of this argument's namesake within the
           `kdu_analysis' constructor, as well as the discussion that appears
           with the definition of `kdu_thread_queue::update_dependencies'.
         [ARG: flags]
           Used to access extended functionality.  Currently, the only
           defined flags are:
           [>>] `KDU_LINE_WILL_BE_EXTENDED' -- if this flag is defined,
                the caller is intending to push in line buffers that
                have been created with an `extend_right' value of 1.  That is,
                `kdu_line_buf' objects supplied to the `push' function will
                have an extra sample beyond the nominal end of the
                line in question.  Knowing this, the current object may
                allocate internal storage to have the same extended length
                so as to maximize the chance that internal calls to
                `kdu_line_buf::exchange' will succeed in performing an
                efficient data exchange without copying of sample values.
                This flag is provided primarily to allow efficient DWT
                implementations to work with buffers that have an equal
                amount of storage for low- and high-pass horizontal subbands.
                In practice, the flag will only be set for horizontal low
                (resp. high) subbands that are shorter (by 1) than the
                corresponding horizontal high (resp. low) subband, where
                the longer subband cannot be spanned by the same number
                of octets as the shorter subband; this is a relatively unusual
                condition, but still worth catering for. The application
                itself would not normally set this flag.
      */
  };

/*****************************************************************************/
/*                              kdu_decoder                                  */
/*****************************************************************************/

class kdu_decoder: public kdu_pull_ifc {
    /* [BIND: reference]
       [SYNOPSIS]
       Implements the block decoding for a single subband, inside a single
       tile-component.
    */
  public: // Member functions
    KDU_EXPORT
      kdu_decoder(kdu_subband subband, kdu_sample_allocator *allocator,
                  bool use_shorts, float normalization=1.0F,
                  int pull_offset=0, kdu_thread_env *env=NULL,
                  kdu_thread_queue *env_queue=NULL, int flags=0);
      /* [SYNOPSIS]
           Informs the decoder that data retrieved via its `kdu_pull_ifc::pull'
           function should have a nominal range from -0.5*R to +0.5*R, where
           R is the value of `normalization'.
           [//]
           The `pull_offset' member should be left as zero when invoking
           this constructor directly from an application.  Internally,
           however, when a `kdu_decoder' object must be constructed within
           a `kdu_synthesis' object, the `pull_offset' value may be set to
           a non-zero value to ensure alignment properties required for
           efficient memory access during horizontal DWT synthesis.  When
           this happens, the width of the line buffer supplied to `pull',
           as returned via `kdu_line_buf::get_width' will be `pull_offset'
           samples larger than the actual width of the subband data being
           requested, and the data will be written starting from location
           `pull_offset', rather than location 0.
           [//]
           The optional `env' and `env_queue' arguments support a variety
           of multi-threaded processing paradigms, to leverage the
           capabilities of multi-processor platforms.  To see how this works,
           consult the description of these arguments below.
         [ARG: env]
           If non-NULL, the behaviour of the underlying
           `kdu_pull_ifc::pull' function is changed radically.  In
           particular, a job queue is created by this constructor, to enable
           asynchronous multi-threaded processing of the code-block samples.
           Processing of code-blocks commences once the first call to
           `kdu_pull_ifc::pull' or `kdu_pull_ifc::start' arrives.  The latter
           approach is preferred, since it allows parallel processing of
           the various subbands in a tile-component to commence immediately
           without waiting for DWT dependencies to be satisfied.  You should
           remember the following two points:
           [>>] All objects which share the same `allocator' must be created
                using the same thread.  Thereafter, the actual processing
                may proceed on different threads.
           [>>] You must supply a non-NULL `env' argument to the
                `kdu_push_ifc::pull' function -- it need not refer to the
                same thread as the one used to create the object here, but
                it must belong to the same thread group.
         [ARG: env_queue]
           If `env' is NULL, this argument is ignored; otherwise, the job
           queue which is created by this constructor will be made a
           sub-queue of any supplied `env_queue'.  If `env_queue' is NULL,
           the queue created to process code-blocks within this
           tile-component-subband will be a top-level queue.
           [//]
           The `env_queue->update_dependencies' function is invoked with
           a `new_dependencies' value of 1 both from within this constructor
           and then whenever a call to `pull' causes this object's internal
           subband sample buffer to become empty, so that a subsequent call
           to `pull' might require the caller to block until the buffer has
           been re-filled by block decoding operations.
           The `env_queue->update_dependencies' function is
           invoked with a `new_dependencies' value of -1 whenever block
           decoding operations cause a previously empty subband sample buffer
           to hold one or more complete lines of decoded subband samples,
           so that a subsequent call to `pull' will not block the caller.
           [//]
           For more on the role and benefits of the `env_queue' argument,
           see the discussion of this argument's namesake within the
           `kdu_synthesis' constructor, as well as the discussion that appears
           with the definition of `kdu_thread_queue::update_dependencies'.
         [ARG: flags]
           Used to access extended functionality.  Currently, the only
           defined flags are:
           [>>] `KDU_LINE_WILL_BE_EXTENDED' -- if this flag is defined,
                the caller is intending to pull data into line buffers that
                have been created with an `extend_right' value of 1.  That is,
                `kdu_line_buf' objects supplied to the `pull' function will
                have an extra sample beyond the nominal end of the
                line in question.  Knowing this, the current object may
                allocate internal storage to have the same extended length
                so as to maximize the chance that internal calls to
                `kdu_line_buf::exchange' will succeed in performing an
                efficient data exchange without copying of sample values.
                This flag is provided primarily to allow efficient DWT
                implementations to work with buffers that have an equal
                amount of storage for low- and high-pass horizontal subbands.
                In practice, the flag will only be set for horizontal low
                (resp. high) subbands that are shorter (by 1) than the
                corresponding horizontal high (resp. low) subband, where
                the longer subband cannot be spanned by the same number
                of octets as the shorter subband; this is a relatively unusual
                condition, but still worth catering for. The application
                itself would not normally set this flag.
      */
  };

#define KDU_MULTI_XFORM_PRECISE                      ((int) 0x00000001)
#define KDU_MULTI_XFORM_FAST                         ((int) 0x00000002)
#define KDU_MULTI_XFORM_SKIPYCC                      ((int) 0x00000004)

#define KDU_MULTI_XFORM_DBUF                         ((int) 0x00000100)
#define KDU_MULTI_XFORM_MT_DWT                       ((int) 0x00000100)
#define KDU_MULTI_XFORM_DELAYED_START                ((int) 0x00000400)

#define KDU_MULTI_XFORM_HIRES_CODER_STRIPES_1        ((int) 0x00001000)
#define KDU_MULTI_XFORM_HIRES_CODER_STRIPES_2        ((int) 0x00002000)
#define KDU_MULTI_XFORM_HIRES_CODER_STRIPES_3        ((int) 0x00003000)
#define KDU_MULTI_XFORM_HIRES_CODER_STRIPES_4        ((int) 0x00004000)
#define KDU_MULTI_XFORM_HIRES_CODER_STRIPES_MASK     ((int) 0x00007000)
#define KDU_MULTI_XFORM_LORES_EXTRA_CODER_STRIPE     ((int) 0x00008000)

#define KDU_MULTI_XFORM_MIN_JOB_SAMPLES_1K           ((int) 0x00010000)
#define KDU_MULTI_XFORM_MIN_JOB_SAMPLES_2K           ((int) 0x00020000)
#define KDU_MULTI_XFORM_MIN_JOB_SAMPLES_4K           ((int) 0x00030000)
#define KDU_MULTI_XFORM_MIN_JOB_SAMPLES_8K           ((int) 0x00040000)
#define KDU_MULTI_XFORM_MIN_JOB_SAMPLES_16K          ((int) 0x00050000)
#define KDU_MULTI_XFORM_MIN_JOB_SAMPLES_MASK         ((int) 0x00070000)

#define KDU_MULTI_XFORM_IDEAL_JOB_SAMPLES_1X_MIN     ((int) 0x00100000)
#define KDU_MULTI_XFORM_IDEAL_JOB_SAMPLES_2X_MIN     ((int) 0x00200000)
#define KDU_MULTI_XFORM_IDEAL_JOB_SAMPLES_4X_MIN     ((int) 0x00300000)
#define KDU_MULTI_XFORM_IDEAL_JOB_SAMPLES_8X_MIN     ((int) 0x00400000)
#define KDU_MULTI_XFORM_IDEAL_JOB_SAMPLES_16X_MIN    ((int) 0x00500000)
#define KDU_MULTI_XFORM_IDEAL_JOB_SAMPLES_MASK       ((int) 0x00700000)

#define KDU_MULTI_XFORM_DEFAULT_FLAGS 0

/*****************************************************************************/
/*                          kd_multi_analysis_base                           */
/*****************************************************************************/

class kd_multi_analysis_base {
  public: // Member functions
    virtual ~kd_multi_analysis_base() { return; }
    virtual void terminate_queues(kdu_thread_env *env) = 0;
      /* Called prior to the destructor if `env' is non-NULL, this function
         terminates tile-component queues created for multi-threading. */
    virtual kdu_coords get_size(int comp_idx) = 0;
    virtual kdu_line_buf *
      exchange_line(int comp_idx, kdu_line_buf *written,
                    kdu_thread_env *env) = 0;
    virtual bool is_line_precise(int comp_idx) = 0;
    virtual bool is_line_absolute(int comp_idx) = 0;
  };

/*****************************************************************************/
/*                           kdu_multi_analysis                              */
/*****************************************************************************/

class kdu_multi_analysis {
  /* [BIND: interface]
     [SYNOPSIS]
       This powerful object generalizes the functionality of `kdu_analysis'
       to the processing of multiple image components, allowing all the
       data for a tile to be managed by a single object.  The object
       creates the `kdu_analysis' objects required to process each
       codestream image component, but it also implements Part-1
       colour decorrelation transforms and Part-2 generalized
       multi-component transforms, as required.
       [//]
       Objects of this class serve as interfaces.  The constructor
       simply creates an empty interface, and there is no meaningful
       destructor.  This means that you may copy and transfer objects
       of this class at will, without any impact on internal resources.
       To create a meaningful insance of the internal machine, you must
       use the `create' member.  To destroy the internal machine you
       must use the `destroy' member.
    */
  public: // Member functions
    kdu_multi_analysis() { state = NULL; }
      /* [SYNOPSIS]
           Leaves the interface empty, meaning that the `exists' member
           returns false.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns true after `create' has been called; you may also
           copy an interface whose `create' function has been called. */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if the object represents
           an interface to instantiated internal processing machinery.
      */
    KDU_EXPORT kdu_long
      create(kdu_codestream codestream, kdu_tile tile,
             kdu_thread_env *env, kdu_thread_queue *env_queue,
             int flags, kdu_roi_image *roi=NULL, int buffer_rows=1);
      /* [SYNOPSIS]
           Use this function to create an instance of the internal
           processing machinery, for compressing data for the supplied
           open `tile' interface.  Until you call this function (or copy
           nother object which has been created), the `exists' function
           will return false.
           [//]
           Multi-component transformations performed by this function are
           affected by previous `kdu_tile::set_components_of_interest'
           calls.  In particular, you need only supply those components
           which have been marked of interest via the `exchange_line'
           function.  Those components marked as uninteresting are
           ignored -- you can pass them in via `exchange_line' if you like,
           but they will have no impact on the way in which codestream
           components are generated and subjected to spatial wavelet
           transformation and coding.
           [//]
           If insufficient components are currently marked as being
           of interest (i.e., too many components were excluded in a
           previous call to `kdu_tile::set_components_of_interest'), the
           present object might not be able to find a way of inverting
           the multi-component transformation network, so as to work back
           to codestream image components.  In this case, an informative
           error message will be generated through `kdu_error'.
           [//]
           This function takes optional `env' and `env_queue' arguments to
           support a variety of multi-threaded processing paradigms, to
           leverage the capabilities of multi-processor platforms.  To see
           how this works, consult the description of these arguments below.
           Also, pay close attention to the use of `env' arguments with the
           `exchange_line' and `destroy' functions.
           [//]
           The `flags' argument works together with subsequent arguments
           to control internal memory allocation, flow control and
           optimization policies.  See below for a description of the
           available flags.
         [RETURNS]
           Returns the number of bytes which have been allocated internally
           for the processing of multi-component transformations,
           spatial wavelet transforms and intermediate buffering between
           the wavelet and block coder engines.  Essentially, this
           includes all memory resources, except for those managed by the
           `kdu_codestream' machinery (for structural information and
           code-block bit-streams).  The latter information can be recovered
           by querying the `kdu_codestream::get_compressed_data_memory' and
           `kdu_codestream::get_compressed_state_memory' functions.
         [ARG: env]
           Supply a non-NULL argument here if you want to enable
           multi-threaded processing.  After creating a cooperating
           thread group by following the procedure outlined in the
           description of `kdu_thread_env', any one of the threads may
           be used to construct this processing engine.  Separate
           processing queues will automatically be created for each
           image component.
           [//]
           If the `KDU_MULTI_XFORM_MT_DWT' flag (or the `KDU_MULTI_XFORM_DBUF'
           flag) is supplied, these queues will also be used to schedule the
           spatial wavelet transform operations associated with each image
           component as jobs to be processed asynchronously by different
           threads.  Regardless multi-threaded DWT processing is requested,
           within each tile-component, separate queues are created to allow
           simultaneous processing of code-blocks from different subbands.
           [//]
           By and large, you can use this object in exactly the same way
           when `env' is non-NULL as you would with a NULL `env' argument.
           That is, the use of multi-threaded processing can be largely
           transparent.  However, you must remember the following two
           points:
           [>>] You must supply a non-NULL `env' argument to the
                `exchange_line' function -- it need not refer to the same
                thread as the one used to create the object here, but it
                must belong to the same thread group.
           [>>] You cannot rely upon all processing being complete until you
                invoke the `kdu_thread_entity::join' or
                `kdu_thread_entity::terminate' function.
         [ARG: env_queue]
           This argument is ignored unless `env' is non-NULL.  When `env'
           is non-NULL, all thread queues that are created inside this object
           are added as sub-queues of `env_queue'.  If `env_queue' is NULL,
           they are added as top-level queues in the multi-threaded queue
           hierarchy.  The present object creates one internal queue for each
           tile-component, to which each subband adds a sub-queue managed by
           `kdu_encoder'.
           [//]
           One advantage of supplying a non-NULL `env_queue' is that it
           provides you with a single hook for joining with the completion
           of all the thread queues which are created by this object
           and its descendants -- see `kdu_thread_entity::join' for more on
           this.
           [//]
           A second advantage of supplying a non-NULL `env_queue' is that
           it allows you to manipulate the sequencing indices that are
           assigned by the thread queues created internally -- see
           `kdu_thread_entity::attach_queue' for more on the role played by
           sequencing indices in controlling the order in which work is
           actually done.  In particular, if `env_queue' is initially
           added to the thread group with a sequencing index of N >= 0,
           all processing within all tile-components of the tile associated
           with this object will proceed with jobs having a sequence index
           of N.
           [//]
           Finally, the `env_queue' object supplied here provides a
           mechanism to determine whether or not calls to `excahnge_line'
           might potentially block.  This may be achieved by supplying
           an `env_queue' object whose `kdu_thread_queue::update_dependencies'
           function has been overridden, or by registering a dependency
           monitor (see `kdu_thread_queue::set_dependency_monitor') with
           the supplied `env_queue'.  If the number of potentially blocking
           dependencies identified by either of these mechanisms is 0,
           calls to `exchange_line' can be invoked at least once for each
           component index, without blocking the caller.  Otherwise, the
           caller might be temporarily blocked while waiting for
           dependencies to be satisfied by DWT analysis and/or subband
           encoding operations that are still in progress.  This temporary
           blocking is not a huge concern, since threads actually enter
           what we call a "working wait", using
           `kdu_thread_entity::wait_for_condition', during which they will
           often perform other tasks.  However, working waits can adversely
           affect cache utilization and often cause work to be done in a
           less than ideal sequence, so that other threads might go idle
           while waiting for jobs to be scheduled by a thread that is
           unduly delayed in a working wait.  For this reason, advanced
           implementations are offered the option of using the dependency
           analysis methods associated with an `env_queue' to schedule jobs
           only when it is known that they are fully ready to proceed.
         [ARG: flags]
           Controls the internal memory allocation, buffer management
           and various processing options.  This argument may be the
           logical OR of any appropriate combination of the flags listed
           below.  For convenience, the `KDU_MULTI_XFORM_DEFAULT_FLAGS'
           value may be used as a starting point, which will supply the
           flags that should always be present unless you have good reason
           not to include them.
           [>>] `KDU_MULTI_XFORM_PRECISE':  This option requests
                the internal machinery to work with 32-bit representations
                for all image component samples.  Otherwise, the internal
                machinery will determine a suitable representation precision,
                making every attempt to use lower precision processing
                paths, which are faster and consume less memory, so long
                as this does not unduly compromise quality.
           [>>] `KDU_MULTI_XFORM_FAST':  This option represents
                the opposite extreme in precision selection to that
                associated with `KDU_MULTI_XFORM_PRECISE'.  In fact,
                if both flags are supplied, the present one will be ignored.
                Otherwise, if the present flag is supplied, the function
                selects the lowest possible internal processing precision,
                even if this does sacrifice some image quality.
           [>>] `KDU_MULTI_XFORM_DBUF': Same as `KDU_MULTI_XFORM_MT_DWT' --
                see below.
           [>>] `KDU_MULTI_XFORM_MT_DWT': This flag is ignored
                unless `env' is non-NULL.  It specifies that the
                spatial DWT operations associated with each codestream
                image component should be executed as asynchronous jobs, as
                opposed to synchronously when new imagery is pushed in
                via the `exchange_line' function.  When the flag is present,
                the object allocates an internal buffer which can hold
                (approximately) 2*`buffer_rows' lines for each
                codestream tile-component.  In the simplest case (double
                buffering), this buffer is partitioned into two parts.  More
                generally, though, the implementation may partition the
                memory into a larger number of parts to improve
                performance.  At any given time, the MCT machinery may be
                writing data to one part while the DWT and block coding
                machinery may be supplied with data from another part.
                Multi-threaded DWT processing provides a useful
                mechanism for minimizing CPU idle time, since multiple threads
                can be scheduled not only to block encoding operations, but
                also to DWT analysis operations.  If DWT analysis is
                the bottleneck, it fundamentally limits the rate at which
                block encoding jobs can be made available to the
                multi-threaded machinery; the only way to overcome this
                bottleneck, if it exists, is to allocate multiple threads to
                DWT analysis via this flag.
         [ARG: roi]
           A non-NULL object may be passed in via this argument to
           allow for region-of-interest driven encoding.  Note carefully,
           however, that the component indices supplied to the
           `kdu_roi_image::acquire_node' function correspond to
           codestream image components.  It is up to you to ensure that
           the correct geometry is returned for each codestream image
           component, in the event that the source image components do not
           map directly (component for component) to codestream image
           components.
         [ARG: buffer_rows]
           This argument must be strictly positive.  It is used to size the
           internal buffers used to hold data that finds its way to the
           individual codestream tile-components, while it is waiting to be
           passed to the DWT analysis and block coding machinery.
           [//]
           From KDU-7.2.1 on, a value of 0 is automatically translated to 1,
           while a -ve value may be supplied for this argument, in which case
           the buffers will be dimensioned automatically, taking the tile
           width and number of components into account.
           [//]
           There are two important cases to understand:
           [>>] If the DWT processing is performed in-line, in the same thread
                as that which calls `exchange_line', then there is only one
                buffer, with this number of rows, and the buffer must be
                filled (or the tile-component exhausted) before DWT processing
                is performed.  In this case, it is usually best to select
                the default value of `buffer_rows'=1 so that processing can
                start as early as possible and there is less chance that the
                image component data will be evicted from lower levels of the
                CPU cache before it is subjected to DWT analysis.  However,
                in the special case where multi-threaded processing is
                employed, with both `env' AND `env_queue' arguments to this
                function being non-NULL, and where the `env_queue' or
                one of the super-queues in its ancestry advertises its
                interest in propagated dependency information (through an
                overridden `kdu_thread_queue::update_dependencies' function
                that returns true, for example, or an installed dependency
                monitor), then there are some multi-threaded synchronization
                overheads associated with the onset and completion of the
                internal DWT analysis machinery (perhaps amounting to several
                hundred or even a thousand clock cycles), so in this case it
                may be better to select a larger value for `buffer_rows',
                especially where the line width is small.
           [>>] If the `KDU_MULTI_XFORM_MT_DWT' or `KDU_MULTI_XFORM_DBUF' flag
                is present, the DWT operations are performed by jobs that may
                be executed on any thread within the multi-threaded environment
                associated with the `env' argument.  In this case, for reasons
                of backward compatibility, the total amount of memory
                allocated for buffering tile-component lines is given (at
                least approximately) by 2*`buffer_rows', since this is what
                would be consumed if a double buffering strategy were
                employed.  In reality, the internal implementation may
                partition this total amount of memory into a larger number
                of smaller buffers so that processing can start earlier and
                the memory can be used more effectively to manage interruption
                of threads, or scheduling of thread resources temporarily to
                other jobs.
           [//]
           You may need to play with this parameter to optimize
           processing efficiency for particular applications.
           [//]
           You should be aware that the actual number of buffer lines
           allocated internally may be smaller than that requested.  For
           example, the implementation may limit the total amount of
           buffer memory to 256 lines.
      */
    kdu_long
      create(kdu_codestream codestream, kdu_tile tile,
             bool force_precise=false, kdu_roi_image *roi=NULL,
             bool want_fastest=false, int buffer_rows=1,
             kdu_thread_env *env=NULL, kdu_thread_queue *env_queue=NULL,
             bool multi_threaded_dwt=false)
        {
          int flags = KDU_MULTI_XFORM_DEFAULT_FLAGS;
          if (force_precise) flags |= KDU_MULTI_XFORM_PRECISE;
          if (want_fastest) flags |= KDU_MULTI_XFORM_FAST;
          if (multi_threaded_dwt) flags |= KDU_MULTI_XFORM_MT_DWT;
          return create(codestream,tile,env,env_queue,flags,roi,buffer_rows);
        }
        /* [SYNOPSIS]
             This form of the `create' function is provided for compatibility
             with applications created for Kakadu v6.x and earlier.  It
             simply invokes the first form of the function with a
             set of flags and parameter values that is usually appropriate.
             [//]
             Note that the `buffer_rows' argument may be known as
             `processing_stripe_height' in other versions of Kakadu, but
             it has the same interpretation -- see the first form of
             `create' for an explanation.
             [//]
             Similarly, the `multi_threaded_dwt' argument may be known as
             `double_buffering' in other versions of Kakadu, but the memory
             consumption and multi-threading implications are the same.
        */
    void destroy(kdu_thread_env *env=NULL)
      {
        if (state != NULL)
          { state->terminate_queues(env); delete state; }
        state = NULL;
      }
      /* [SYNOPSIS]
           Use this function to destroy the internal processing machine
           created using `create'.  The function may be invoked on any
           copy of the original object whose `create' function was called,
           so be careful.
         [ARG: env]
           If this argument is non-NULL, this function will terminate all
           work within the object before cleaning up its resources; this
           is done using `kdu_thread_entity::terminate'.
           [//]
           Otherwise, so long as you have explicitly used
           `kdu_thread_entity::join' or `kdu_thread_entity::terminate'
           already, it is OK to invoke this function with a NULL `env'
           argument.  There is nothing stopping you from joining multiple
           times on a queue, so you can use both approaches without any
           problems.
      */
    kdu_coords get_size(int comp_idx) { return state->get_size(comp_idx); }
      /* [SYNOPSIS]
           This is a convenience function to return the size of the image
           component identified by `comp_idx', as seen within the present
           tile.  The same information may be obtained by invoking
           `kdu_codestream::get_tile_dims' with its `want_output_comps'
           argument set to true.
      */
    kdu_line_buf *exchange_line(int comp_idx, kdu_line_buf *written,
                                kdu_thread_env *env=NULL)
      { return state->exchange_line(comp_idx,written,env); }
      /* [SYNOPSIS]
           Use this function to exchange image data with the processing
           engine.  If `written' is NULL, you are only asking for access
           to a line buffer, into which to write a new line of image
           data for the component in question.  Once you have written
           to the supplied line buffer, you pass it back as the `written'
           argument in a subsequent call to this function.  Regardless
           of whether `written' is NULL, the function returns a pointer
           to the single internal line buffer which it maintains for each
           original image component, if and only if that line buffer is
           currently available for writing.  It will not be available if
           the internal machinery is waiting for a line of another component
           before it can process the data which has already been supplied.
           Thus, if a newly written line can be processed immediately, the
           function will return a non-NULL pointer even in the call with
           `written' non-NULL.  If it must wait for other component lines
           to arrive, however, it will return NULL.  Once returning non-NULL,
           the function will continue to return the same line buffer at
           least until the next call which supplies a non-NULL `written'
           argument.  This is because the current line number is incremented
           only by calls which supply a non-NULL `written' argument.
           [//]
           Note that all lines processed by this function should have a
           signed representation, regardless of whether or not
           `kdu_codestream::get_signed' reports that the components
           are signed.
         [RETURNS]
           Non-NULL if a line is available for writing.  That same line
           should be passed back to the function as its `written' argument
           in a subsequent call to the function (not necessarily the next
           one) in order to advance to a new line.  If the function returns
           NULL, you may have reached the end of the tile (you
           should know this), or else the object may be waiting for you
           to supply new lines for other image components which must
           be processed together with this one.
         [ARG: comp_idx]
           Index of the component for which a line is being written or
           requested.  This index must lie in the range 0 to Cs-1, where
           Cs is the `num_source_components' value supplied to
           `create'.
         [ARG: written]
           If non-NULL, this argument must be identical to the line buffer
           which was previously returned by the function, using the same
           `comp_idx' value.  In this case, the line is deemed to contain
           valid image data and the internal line counter for this component
           will be incremented before the function returns.  Otherwise,
           you are just asking the function to give you access to the
           internal line buffer so that you can write to it.
         [ARG: env]
           Must be non-NULL if and only if a non-NULL `env' argument was
           passed into `create'.  Any non-NULL `env' argument must identify
           the calling thread, which need not necessarily be the one used
           to create the object in the first place.
      */
    bool is_line_precise(int comp_idx)
      { return state->is_line_precise(comp_idx); }
      /* [SYNOPSIS]
           Returns true if the indicated line has been assigned a
           precise (32-bit) representation by the `create' function.
           Otherwise, calls to `exchange_line' will return lines which
           have a 16-bit representation.  This function is provided as
           a courtesy so that applications which need to allocate
           auxiliary lines with compatible precisions will be able to
           do so.
      */
    bool is_line_absolute(int comp_idx)
      { return state->is_line_absolute(comp_idx); }
      /* [SYNOPSIS]
           Returns true if the indicated line has been assigned a
           reversible (i.e., absolute integer) representation by the
           `create' function.  Otherwise, calls to `exchange_line' will
           return lines whose `kdu_line_buf::is_absolute' function
           returns false.  This function is provided as a courtesy, so
           that applications can know ahead of time what the type of the
           data associated with a line will be.  In the presence of
           multi-component transforms, this can be non-trivial to figure
           out based solely on the output component index.
      */
  private: // Data
    kd_multi_analysis_base *state;
  };

/*****************************************************************************/
/*                         kd_multi_synthesis_base                           */
/*****************************************************************************/

class kd_multi_synthesis_base {
  public: // Member functions
    virtual ~kd_multi_synthesis_base() { return; }
    virtual void terminate_queues(kdu_thread_env *env) = 0;
      /* Called prior to the destructor if `env' is non-NULL, this function
         terminates the tile-component queues created for multi-threading. */
    virtual bool start(kdu_thread_env *env) = 0;
    virtual kdu_coords get_size(int comp_idx) = 0;
    virtual kdu_line_buf *get_line(int comp_idx, kdu_thread_env *env) = 0;
    virtual bool is_line_precise(int comp_idx) = 0;
    virtual bool is_line_absolute(int comp_idx) = 0;
  };

/*****************************************************************************/
/*                           kdu_multi_synthesis                             */
/*****************************************************************************/

class kdu_multi_synthesis {
  /* [BIND: interface]
     [SYNOPSIS]
       This powerful object generalizes the functionality of `kdu_synthesis'
       to the processing of multiple image components, allowing all the
       data for a tile (or any subset thereof) to be reconstructed by a
       single object.  The object creates the `kdu_synthesis' objects
       required to process each required codestream image component, but it
       also inverts Part-1 colour decorrelation transforms (RCT and ICT)
       and Part-2 generalized multi-component transforms, as required.
       [//]
       Objects of this class serve as interfaces.  The constructor
       simply creates an empty interface, and there is no meaningful
       destructor.  This means that you may copy and transfer objects
       of this class at will, without any impact on internal resources.
       To create a meaningful insance of the internal machine, you must
       use the `create' member.  To destroy the internal machine you
       must use the `destroy' member.
       [//]
       To use this object, one typically invokes `create', followed by
       calls to `get_line' and then ultimately `destroy'.  In multi-threaded
       processing environments, one typically passes a `kdu_thread_queue'
       super-queue to the `create' function and invokes
       `kdu_thread_entity::terminate' on this super-queue prior to the
       call to `destroy'.  In multi-threaded multi-tiled environments,
       where multiple tiles are to be processed concurrently (as opposed
       to sequentially), it is generally preferable to include the
       `KDU_MULTI_XFORM_DELAYED_START' flag in the call to `create' and
       use the `start' function in the recommended manner prior to the
       first call to `get_line', although this is not required and might
       not make much difference in many cases, due to the ability of the
       internal machinery to compensate.   
  */
  public: // Member functions
    kdu_multi_synthesis() { state = NULL; }
      /* [SYNOPSIS]
           Leaves the interface empty, meaning that the `exists' member
           returns false.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns true after `create' has been called; you may also
           copy an interface whose `create' function has been called. */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if the object represents
           an interface to instantiated internal processing machinery.
      */
  
    KDU_EXPORT kdu_long
      create(kdu_codestream codestream, kdu_tile tile,
             kdu_thread_env *env, kdu_thread_queue *env_queue,
             int flags, int buffer_rows=1);
      /* [SYNOPSIS]
           Use this function to create an instance of the internal
           processing machinery, for decompressing data associated with
           the supplied open `tile' interface.  Until you call this
           function (or copy another object which has been created), the
           `exists' function will return false.
           [//]
           Note carefully that the output components which will be
           decompressed are affected by any previous calls to
           `kdu_codestream::apply_input_restrictions'.  You may use either
           of the two forms of that function to modify the set of output
           components which appear to be present.  If called with an
           `access_mode' argument of `KDU_WANT_CODESTREAM_COMPONENTS', the
           present object will present codestream image components as though
           they were the final output image components.  If, however,
           `kdu_codestream::apply_input_restrictions' was called with a
           component `access_mode' argument of `KDU_WANT_OUTPUT_COMPONENTS',
           or if it has never been called, the present object will present
           output components in their fullest form, after processing by any
           required inverse multi-component decomposition, if necessary.  In
           either case, the set of components which is presented is
           identical to that which appears via the various `kdu_codestream'
           interface functions, such as `kdu_codestream::get_num_components',
           `kdu_codestream::get_bit_depth', and so forth, in each case
           with the optional `want_output_comps' argument set to true.
           [//]
           The behaviour of this function is affected by calls to
           `kdu_tile::set_components_of_interest'.  In particular, any of the
           apparent output components which have been identified as
           uninteresting, will not be generated by the multi-component
           transformation network -- they will, instead, appear to contain
           constant sample values.   Of course, you will probably not want
           to access these constant components, or else you would not have
           marked them as uninteresting; however, you can access them if you
           wish without incurring any processing overhead.
           [//]
           The `env' and `env_queue' arguments may be used to implement a
           a variety of multi-threaded processing paradigms, to leverage
           the capabilities of multi-processor platforms.  To see how this
           works, consult the description of these arguments below.  Also,
           play close attention to the use of `env' arguments with the
           `get_line' and `destroy' functions.
           [//]
           The `flags' argument works together with subsequent arguments
           to control internal memory allocation, flow control and
           optimization policies.  See below for a description of the
           available flags.
           [//]
           If you are working with tiled images and you are intending to
           create multiple `kdu_multi_synthesis' engines from which image
           component lines will be retrieved in round-robbin fashion (typical
           for scan-line oriented processing of tiled imagery), it may be
           worth paying attention to the `KDU_MULTI_XFORM_DELAYED_START'
           flag and the `start' function, both of which are new to Kakadu
           v7, where they play an important role in maximizing the efficiency
           for multi-threaded processing environments involving a large number
           of threads.
         [RETURNS]
           Returns the number of bytes which have been allocated internally
           for the processing of multi-component transformations,
           spatial wavelet transforms and intermediate buffering between
           the wavelet and block decoder engines.  Essentially, this
           includes all memory resources, except for those managed by the
           `kdu_codestream' machinery (for structural information and
           code-block bit-streams).  The latter information can be recovered
           by querying the `kdu_codestream::get_compressed_data_memory' and
           `kdu_codestream::get_compressed_state_memory' functions.
         [ARG: env]
           Supply a non-NULL argument here if you want to enable
           multi-threaded processing.  After creating a cooperating
           thread group by following the procedure outlined in the
           description of `kdu_thread_env', any one of the threads may
           be used to construct this processing engine.  Separate
           processing queues will automatically be created for each
           image component.
           [//]
           If the `KDU_MULTI_XFORM_MT_DWT' flag (or the `KDU_MULTI_XFORM_DBUF'
           flag) is supplied, these queues will also be used to schedule the
           spatial wavelet transform operations associated with each image
           component as jobs to be processed asynchronously by different
           threads.  Regardless multi-threaded DWT processing is requested,
           within each tile-component, separate queues are created to allow
           simultaneous processing of code-blocks from different subbands.
           [//]
           By and large, you can use this object in exactly the same way
           when `env' is non-NULL as you would with a NULL `env' argument.
           That is, the use of multi-threaded processing can be largely
           transparent.  However, you must remember the following three
           points:
             [>>] You must supply a non-NULL `env' argument to the
                  `get_line' function -- it need not belong to the same
                  thread as the one used to create the object here, but it
                  must belong to the same thread group.
             [>>] Whereas single-threaded processing commences only with the
                  first call to `get_line', multi-threaded processing
                  may have already commenced before this function returns.
                  That is, other threads may be working in the background
                  to decode code-blocks, perform DWT synthesis and so forth,
                  and this may start happening even before the present
                  function returns.  Of course, this is exactly what you
                  want, to fully exploit the availability of multiple
                  processing resources.
             [>>] If you are creating multiple `kdu_multi_synthesis' objects
                  that you intend to use concurrently (retrieving lines from
                  their `get_line' functions in round-robbin fashion), you
                  are recommended to follow the startup protocol
                  associated with the `start' function and the
                  `KDU_MULTI_XFORM_DELAYED_START' creation flag.
         [ARG: env_queue]
           This argument is ignored unless `env' is non-NULL.  When `env'
           is non-NULL, all thread queues that are created inside this object
           are added as sub-queues of `env_queue'.  If `env_queue' is NULL,
           they are added as top-level queues in the multi-threaded queue
           hierarchy.  The present object creates one internal queue for each
           tile-component, to which each subband adds a sub-queue managed by
           `kdu_decoder'.
           [//]
           One advantage of supplying a non-NULL `env_queue' is that it
           provides you with a single hook for joining with the completion
           of all the thread queues which are created by this object
           and its descendants -- see `kdu_thread_entity::join' for more on
           this.
           [//]
           A second advantage of supplying a non-NULL `env_queue' is that
           it allows you to manipulate the sequencing indices that are
           assigned by the thread queues created internally -- see
           `kdu_thread_entity::attach_queue' for more on the role played by
           sequencing indices in controlling the order in which work is
           actually done.  In particular, if `env_queue' is initially
           added to the thread group with a sequencing index of N >= 0,
           all processing within all tile-components of the tile associated
           with this object will proceed with jobs having a sequence index
           of N.
           [//]
           Finally, the `env_queue' object supplied here provides a
           mechanism to determine whether or not calls to `get_line'
           might potentially block.  This may be achieved by supplying
           an `env_queue' object whose `kdu_thread_queue::update_dependencies'
           function has been overridden, or by registering a dependency
           monitor (see `kdu_thread_queue::set_dependency_monitor') with
           the supplied `env_queue'.  If the number of potentially blocking
           dependencies identified by either of these mechanisms is 0,
           calls to `get_line' can be invoked at least once for each
           component index, without blocking the caller.  Otherwise, the
           caller might be temporarily blocked while waiting for
           dependencies to be satisfied by DWT synthesis and/or subband
           decoding operations that are still in progress.  This temporary
           blocking is not a huge concern, since threads actually enter
           what we call a "working wait", using
           `kdu_thread_entity::wait_for_condition', during which they will
           often perform other tasks.  However, working waits can adversely
           affect cache utilization and often cause work to be done in a
           less than ideal sequence, so that other threads might go idle
           while waiting for jobs to be scheduled by a thread that is
           unduly delayed in a working wait.  For this reason, advanced
           implementations are offered the option of using the dependency
           analysis methods associated with an `env_queue' to schedule jobs
           only when it is known that they are fully ready to proceed.
         [ARG: flags]
           Controls the internal memory allocation, buffer management
           and various processing and optimization options.  This argument
           may be the logical OR of any appropriate combination of the flags
           listed below.  For convenience, the `KDU_MULTI_XFORM_DEFAULT_FLAGS'
           value may be used as a starting point, which will supply the
           flags that should always be present unless you have good reason
           not to include them.
           [>>] `KDU_MULTI_XFORM_DELAYED_START': This option is important
                in heavily multi-threading environments, if you are creating
                multiple `kdu_multi_synthesis' objects and you wish to use
                them concurrently -- i.e., you wish to retrieve lines from
                them in round-robbin fasion, as opposed to pulling all the
                data from one `kdu_multi_synthesis' engine via its `get_line'
                function and then moving onto the next one.  By default (i.e.,
                without this flag), the `create' function causes as many
                processing jobs as possible to be scheduled as soon as
                possible to service this `kdu_multi_synthesis' object.
                However, if you want to work concurrently with multiple
                `kdu_multi_synthesis' engines (typically to retrieve data from
                a collection of adjacent tiles), it is better if the
                processing jobs that service all these engines are scheduled
                in an interleaved fashion.  To achieve this, you may supply
                this flag during the `create' call and then you should invoke
                the `start' function before attempting to retrieve image
                component lines with `get_line'.  The `start' function should
                be invoked repeatedly on all concurrent `kdu_multi_synthesis'
                objects, in round-robbin fashion, until all calls to `start'
                return true.
           [>>] `KDU_MULTI_XFORM_PRECISE':  This option requests
                the internal machinery to work with 32-bit representations
                for all image component samples.  Otherwise, the internal
                machinery will determine a suitable representation precision,
                making every attempt to use lower precision processing
                paths, which are faster and consume less memory, so long
                as this does not unduly compromise quality.
           [>>] `KDU_MULTI_XFORM_FAST':  This option represents
                the opposite extreme in precision selection to that
                associated with `KDU_MULTI_XFORM_PRECISE'.  In fact,
                if both flags are supplied, the present one will be ignored.
                Otherwise, if the present flag is supplied, the function
                selects the lowest possible internal processing precision,
                even if this does sacrifice some image quality.  This mode
                is normally acceptable for rendering to low precision
                displays (e.g., 8 to 12 bits/sample).
           [>>] `KDU_MULTI_XFORM_SKIPYCC': Omit this flag unless you
                are quite sure that you want to retrieve raw codestream
                components without inverting any Part-1 decorrelating
                transform (inverse RCT or inverse ICT) which might otherwise
                be involved.  For this to make sense, you should be sure
                that the `kdu_codestream::apply_input_restrictions' function
                has been used to set the component access mode to
                `KDU_WANT_CODESTREAM_COMPONENTS' rather than
                `KDU_WANT_OUTPUT_COMPONENTS'.
           [>>] `KDU_MULTI_XFORM_DBUF': Same as `KDU_MULTI_XFORM_MT_DWT' --
                see below.
           [>>] `KDU_MULTI_XFORM_MT_DWT': This flag is ignored
                unless `env' is non-NULL.  It specifies that the
                spatial DWT operations associated with each codestream
                image component should be executed as asynchronous jobs, as
                opposed to synchronously when new imagery is pushed in
                via the `exchange_line' function.  When the flag is present,
                the object allocates an internal buffer which can hold
                (approximately) 2*`buffer_rows' lines for each
                codestream tile-component.  In the simplest case (double
                buffering), this buffer is partitioned into two parts.  More
                generally, though, the implementation may partition the
                memory into a larger number of parts to improve
                performance.  At any given time, the MCT machinery may be
                reading data from one part while the DWT and block decoding
                machinery may be working to write data to another part.
                Multi-threaded DWT processing provides a useful
                mechanism for minimizing CPU idle time, since multiple threads
                can be scheduled not only to block decoding operations, but
                also to DWT synthesis operations.  If DWT synthesis is
                the bottleneck, it fundamentally limits the rate at which
                block decoding jobs can be made available to the
                multi-threaded machinery; the only way to overcome this
                bottleneck, if it exists, is to allocate multiple threads to
                DWT synthesis via this flag.
         [ARG: buffer_rows]
           This argument must be strictly positive.  It is used to size the
           internal buffers used to hold data that is obtained via DWT
           synthesis and/or block decoding within individual codestream
           tile-components, while it is waiting to be processed by the MCT
           machinery.
           [//]
           From KDU-7.2.1 on, a value of 0 is automatically translated to 1,
           while a -ve value may be supplied for this argument, in which case
           the buffers will be dimensioned automatically, taking the tile
           width and number of components into account.
           [//]
           There are two important cases to understand:
           [>>] If the DWT processing is performed in-line, in the same thread
                as that which calls `pull_line', then there is only one
                buffer, with this number of rows, and the buffer must be
                empty before DWT processing is performed.  In this case, it
                is usually best to select the default value of `buffer_rows'=1
                so that DWT processing occurs as close as possible to the
                point at which the synthesized sample values are consumed.
                This minimizes the chance that the DWT synthesis results
                will be evicted from lower levels of the CPU cache before
                they are consumed via the `pull_line' function.  However,
                in the special case where multi-threaded processing is
                employed, with both `env' AND `env_queue' arguments to this
                function being non-NULL, and where the `env_queue' or one
                of the super-queues in its ancestry advertises its interest
                in propagated dependency information (through an overridden
                `kdu_thread_queue::update_dependencies' function that
                returns true, for example, or an installed dependency
                monitor), then there are some multi-threaded
                synchronization overheads associated with the onset and
                completion of the internal DWT synthesis machinery (perhaps
                amounting to several hundred or even a thousand clock cycles),
                so in this case it may be better to select a larger value for
                `buffer_rows', especially where the line width is small.
           [>>] If the `KDU_MULTI_XFORM_MT_DWT' or `KDU_MULTI_XFORM_DBUF' flag
                is present, the DWT operations are performed by jobs that may
                be executed on any thread within the multi-threaded environment
                associated with the `env' argument.  In this case, for reasons
                of backward compatibility, the total amount of memory
                allocated for buffering tile-component lines is given (at
                least approximately) by 2*`buffer_rows', since this is what
                would be consumed if a double buffering strategy were
                employed.  In reality, the internal implementation may
                partition this total amount of memory into a larger number
                of smaller buffers so that processing can start earlier and
                the memory can be used more effectively to manage interruption
                of threads, or scheduling of thread resources temporarily to
                other jobs.
           [//]
           You may need to play with this parameter to optimize
           processing efficiency for particular applications.
           [//]
           You should be aware that the actual number of buffer lines
           allocated internally may be smaller than that requested.  For
           example, the implementation may limit the total amount of
           buffer memory to 256 lines.
        */
    kdu_long
      create(kdu_codestream codestream, kdu_tile tile,
             bool force_precise=false, bool skip_ycc=false,
             bool want_fastest=false, int buffer_rows=1,
             kdu_thread_env *env=NULL, kdu_thread_queue *env_queue=NULL,
             bool multi_threaded_dwt=false)
        {
          int flags = KDU_MULTI_XFORM_DEFAULT_FLAGS;
          if (force_precise) flags |= KDU_MULTI_XFORM_PRECISE;
          if (skip_ycc) flags |= KDU_MULTI_XFORM_SKIPYCC;
          if (want_fastest) flags |= KDU_MULTI_XFORM_FAST;
          if (multi_threaded_dwt) flags |= KDU_MULTI_XFORM_MT_DWT;
          return create(codestream,tile,env,env_queue,flags,buffer_rows);
        }
        /* [SYNOPSIS]
             This form of the `create' function is provided for compatibility
             with applications created with other versions of Kakadu.  It
             simply invokes the first form of the function with an appropriate
             set of flags and parameter values.
             [//]
             However, this function is NOT IDEAL if you wish to
             process MULTIPLE TILES CONCURRENTLY -- i.e., pulling image
             component lines from multiple `kdu_multi_synthesis' objects'
             `get_line' functions in round-robbin fashion.  In that case,
             you are recommended to invoke the first form of the `create'
             function with the `KDU_MULTI_XFORM_DELAYED_START' flag and you
             should be following the efficient startup protocol described with
             the `start' function.
             [//]
             Note that the `buffer_rows' argument may be known as
             `processing_stripe_height' in other versions of Kakadu, but
             it has the same interpretation -- see the first form of
             `create' for an explanation.
             [//]
             Similarly, the `multi_threaded_dwt' argument may be known as
             `double_buffering' in other versions of Kakadu, but the memory
             consumption and multi-threading implications are the same.
        */
    bool start(kdu_thread_env *env=NULL)
      { return (state == NULL)?true:state->start(env); }
      /* [SYNOPSIS]
           It is always safe to call this function, but unless the
           `KDU_MULTI_XFORM_DELAYED_START' flag was passed to `create',
           the function will simply return true, doing nothing.
           [//]
           Delayed starts are useful only when using a multi-threaded
           processing environment (of which `env' must be a member) and
           then only when you wish to work with multiple `kdu_multi_synthesis'
           engines concurrently, retrieving image lines via their `get_line'
           interfaces in round-robbin fashion.  In this case, multi-threaded
           processing can be more efficient if you follow the startup
           protocol described below:
           [>>] Let E_1, E_2, ..., E_N denote N `kdu_multi_synthesis'
                processing engines that you wish to use concurrently.  Almost
                invariably, these correspond to N horizontally adjacent tiles
                from the source image that you wish to decompress
                "concurrently", so that the complete image (or a region of
                interest) can be recovered progressively, in raster scan
                fashion.
           [>>] First, you should invoke `E_1->create', `E_2->create', ...,
                `E_N->create', passing the `KDU_MULTI_XFORM_DELAYED_START'
                flag to each `create' function.  This actually does initiate
                the scheduling of compressed data parsing and block decoding
                jobs to some extent, but it does not generally cause all
                possible jobs to be scheduled and it does not activate any
                internal asynchronous DWT synthesis processing jobs.
           [>>] Next, you should invoke `E_1->start', `E_2->start', ...,
                `E_N->start' in sequence, paying attention to the return
                values from these functions.  If any of the functions returned
                false, you should repeat the process, until all of the calls
                to `start' return false.
           [//]
           If you fail to complete the above protocol before calling `get_line'
           for the first time, the protocol will be completed for you, but
           processing jobs will not generally be ideally interleaved between
           the `kdu_multi_synthesis' processing engines that you wish to use
           concurrently.  As a result, working threads may not be utilized
           as fully as possible.
           [//]
           If you fail to complete the above protocol and you have your
           own processing queue (passed to `create') that is waiting for
           an overridden `kdu_thread_queue::update_dependencies' function to
           signal the availability of data before scheduling its own
           processing jobs to retrieve image component lines via `get_line',
           that event will never occur and your application will become
           deadlocked -- of course, this is an exotic way to use the
           `kdu_multi_synthesis' machinery, but it is one that can be
           particularly efficient in heavily multi-threaded applications.
           [//]
           In any event, the message is that you should follow the protocol
           described above or else you should not pass the
           `KDU_MULTI_XFORM_DELAYED_START' flag to `create'.
      */
    void destroy(kdu_thread_env *env=NULL)
      {
        if (state != NULL)
          { state->terminate_queues(env); delete state; }
        state = NULL;
      }
      /* [SYNOPSIS]
           Use this function to destroy the internal processing machine
           created using `create'.  The function may be invoked on any
           copy of the original object whose `create' function was called,
           so be careful.
         [ARG: env]
           If this argument is non-NULL, this function will automatically
           termintae all work within the object before cleaning up its
           resources.  This is done using `kdu_thread_entity::terminate'.
           [//]
           Otherwise, so long as you have explicitly used
           `kdu_thread_entity::join' or `kdu_thread_entity::terminate'
           already, it is OK to invoke this function with a NULL `env'
           argument.  There is nothing stopping you from joining multiple
           times on a queue, so you can use both approaches without any
           problems.
      */
    kdu_coords get_size(int comp_idx) { return state->get_size(comp_idx); }
      /* [SYNOPSIS]
           This is a convenience function to return the size of the image
           component identified by `comp_idx', as seen within the present
           tile.  The same information may be obtained by invoking
           `kdu_codestream::get_tile_dims' with its `want_output_comps'
           argument set to true.
      */
    kdu_line_buf *get_line(int comp_idx, kdu_thread_env *env=NULL)
      { return state->get_line(comp_idx,env); }
      /* [SYNOPSIS]
           Use this function to get the next line of image data from
           the indicated component.  The function will return NULL if
           you have already reached the end of the tile, or if the next
           component cannot be decompressed without first retrieving
           new lines from one or more other components.  This latter
           condition may arise if the components are coupled through a
           multi-component transform, in which case the components must
           be accessed in an interleaved fashion -- otherwise, the object
           would need to allocate additional internal buffering resources.
           If there is a component that you are not interested in, you
           should declare that using either
           `kdu_codestream::apply_input_restrictions' and/or
           `kdu_tile::set_components_of_interest', before creating
           the present object.
           [//]
           Note that all lines returned by this function have a signed
           representation, regardless of whether or not
           `kdu_codestream::get_signed' reports that the components are
           signed.  In most cases, this minimizes the number of memory
           accesses which are required, deferring any required offsets
           until rendering (or saving to a file).
         [RETURNS]
           Non-NULL if a new decompressed line is available for the
           indicated component.  Each call to this function which returns
           a non-NULL pointer causes an internal line counter to be
           incremented for the component in question.
         [ARG: comp_idx]
           Index of the component for which a new line is being requested.
           This index musts lie in the range 0 to Co-1, where Co is
           the value returned by `kdu_codestream::get_num_components',
           with its `want_output_comps' argument set to true.  The number
           of these components may be affected by calls to
           `kdu_codestream::apply_input_restrictions' -- such calls must
           have been made prior to the point at which this object's
           `create' function was called.
         [ARG: env]
           Must be non-NULL if and only if a non-NULL `env' argument was
           passed into `create'.  Any non-NULL `env' argument must identify
           the calling thread, which need not necessarily be the one used
           to create the object in the first place.
      */
    bool is_line_precise(int comp_idx)
      { return state->is_line_precise(comp_idx); }
      /* [SYNOPSIS]
           Returns true if the indicated line has been assigned a
           precise (32-bit) representation by the `create' function.
           Otherwise, calls to `get_line' will return lines which
           have a 16-bit representation.  This function is provided as
           a courtesy so that applications which need to allocate
           auxiliary lines with compatible precisions will be able to
           do so.
      */
    bool is_line_absolute(int comp_idx)
      { return state->is_line_absolute(comp_idx); }
      /* [SYNOPSIS]
           Returns true if the indicated line has been assigned a
           reversible (i.e., absolute integer) representation by the
           `create' function.  Otherwise, calls to `exchange_line' will
           return lines whose `kdu_line_buf::is_absolute' function
           returns false.  This function is provided as a courtesy, so
           that applications can know ahead of time what the type of the
           data associated with a line will be.  In the presence of
           multi-component transforms, this can be non-trivial to figure
           out based solely on the output component index.
      */
  private: // Data
    kd_multi_synthesis_base *state;
  };

/*****************************************************************************/
/*                    Base Casting Assignment Operators                      */
/*****************************************************************************/

inline kdu_push_ifc &kdu_push_ifc::operator=(kdu_analysis rhs)
  { state = rhs.state; return *this; }
inline kdu_push_ifc &kdu_push_ifc::operator=(kdu_encoder rhs)
  { state = rhs.state; return *this; }

inline kdu_pull_ifc &kdu_pull_ifc::operator=(kdu_synthesis rhs)
  { state = rhs.state; return *this; }
inline kdu_pull_ifc &kdu_pull_ifc::operator=(kdu_decoder rhs)
  { state = rhs.state; return *this; }


/* ========================================================================= */
/*                     External Function Declarations                        */
/* ========================================================================= */

KDU_EXPORT extern void
  (*kdu_convert_rgb_to_ycc_rev16)(kdu_int16*, kdu_int16*, kdu_int16*, int);
KDU_EXPORT extern void
  (*kdu_convert_rgb_to_ycc_irrev16)(kdu_int16*, kdu_int16*, kdu_int16*, int);
KDU_EXPORT extern void
  (*kdu_convert_rgb_to_ycc_rev32)(kdu_int32*, kdu_int32*, kdu_int32*, int);
KDU_EXPORT extern void
  (*kdu_convert_rgb_to_ycc_irrev32)(float*, float*, float*, int);

KDU_EXPORT extern void
  (*kdu_convert_ycc_to_rgb_rev16)(kdu_int16*, kdu_int16*, kdu_int16*, int);
KDU_EXPORT extern void
  (*kdu_convert_ycc_to_rgb_irrev16)(kdu_int16*, kdu_int16*, kdu_int16*, int);
KDU_EXPORT extern void
  (*kdu_convert_ycc_to_rgb_rev32)(kdu_int32*, kdu_int32*, kdu_int32*, int);
KDU_EXPORT extern void
  (*kdu_convert_ycc_to_rgb_irrev32)(float*, float*, float*, int);

static inline void
  kdu_convert_rgb_to_ycc(kdu_line_buf &c1, kdu_line_buf &c2, kdu_line_buf &c3)
  { 
    int n = c1.get_width();
    assert((c2.get_width() == n) && (c3.get_width() == n));
    assert((c1.is_absolute() == c2.is_absolute()) &&
           (c1.is_absolute() == c3.is_absolute()));
    if ((c1.get_buf16() != NULL) && c1.is_absolute())
      kdu_convert_rgb_to_ycc_rev16(&(c1.get_buf16()->ival),
                                   &(c2.get_buf16()->ival),
                                   &(c3.get_buf16()->ival),n);
    else if (c1.get_buf16() != NULL)
      kdu_convert_rgb_to_ycc_irrev16(&(c1.get_buf16()->ival),
                                     &(c2.get_buf16()->ival),
                                     &(c3.get_buf16()->ival),n);
    else if (c1.is_absolute())
      kdu_convert_rgb_to_ycc_rev32(&(c1.get_buf32()->ival),
                                   &(c2.get_buf32()->ival),
                                   &(c3.get_buf32()->ival),n);
    else
      kdu_convert_rgb_to_ycc_irrev32(&(c1.get_buf32()->fval),
                                     &(c2.get_buf32()->fval),
                                     &(c3.get_buf32()->fval),n);
  }
  /* [SYNOPSIS]
       The line buffers must be compatible with respect to dimensions and data
       type.  The forward ICT (RGB to YCbCr transform) is performed if the data
       is normalized (i.e. `kdu_line_buf::is_absolute' returns false).
       Otherwise, the RCT is performed.
  */
static inline void
  kdu_convert_ycc_to_rgb(kdu_line_buf &c1, kdu_line_buf &c2, kdu_line_buf &c3,
                         int width=-1)
  {
    int n = (width < 0)?c1.get_width():width;
    assert((c1.get_width() >= n) && (c2.get_width() >= n) &&
           (c3.get_width() >= n));
    assert((c1.is_absolute() == c2.is_absolute()) &&
           (c1.is_absolute() == c3.is_absolute()));
    if ((c1.get_buf16() != NULL) && c1.is_absolute())
      kdu_convert_ycc_to_rgb_rev16(&(c1.get_buf16()->ival),
                                   &(c2.get_buf16()->ival),
                                   &(c3.get_buf16()->ival),n);
    else if (c1.get_buf16() != NULL)
      kdu_convert_ycc_to_rgb_irrev16(&(c1.get_buf16()->ival),
                                     &(c2.get_buf16()->ival),
                                     &(c3.get_buf16()->ival),n);
    else if (c1.is_absolute())
      kdu_convert_ycc_to_rgb_rev32(&(c1.get_buf32()->ival),
                                   &(c2.get_buf32()->ival),
                                   &(c3.get_buf32()->ival),n);
    else
      kdu_convert_ycc_to_rgb_irrev32(&(c1.get_buf32()->fval),
                                     &(c2.get_buf32()->fval),
                                     &(c3.get_buf32()->fval),n);
  }
  /* [SYNOPSIS]
       Inverts the effects of the forward transform performed by
       `kdu_convert_rgb_to_ycc'.  If `width' is negative, the number of
       samples in each line is determined from the line buffers themselves.
       Otherwise, only the first `width' samples in each line are actually
       processed.
  */

#endif // KDU_SAMPLE_PROCESSING
