/*****************************************************************************/
// File: stripe_compressor_local.h [scope = APPS/SUPPORT]
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
   Local definitions used in the implementation of the
`kdu_stripe_compressor' object.
******************************************************************************/

#ifndef STRIPE_COMPRESSOR_LOCAL_H
#define STRIPE_COMPRESSOR_LOCAL_H

#include "kdu_stripe_compressor.h"

// Objects declared here:
struct kdsc_component_state;
struct kdsc_component;
struct kdsc_tile;

#define KDSC_BUF8      0
#define KDSC_BUF16     1
#define KDSC_BUF32     2
#define KDSC_BUF_FLOAT 6

/* ========================================================================= */
/*                      ACCELERATION FUNCTION POINTERS                       */
/* ========================================================================= */

// Configure processor-specific compilation options
#if (defined KDU_PENTIUM_MSVC)
#  undef KDU_PENTIUM_MSVC
#  ifndef KDU_X86_INTRINSICS
#    define KDU_X86_INTRINSICS // Use portable intrinsics instead
#  endif
#endif // KDU_PENTIUM_MSVC

#if defined KDU_X86_INTRINSICS
#  include "x86_stripe_transfer_local.h"
#  define KDU_SIMD_OPTIMIZATIONS
#endif

typedef void
  (*kdsc_simd_transfer_func)(void **dst, void *src, int width,
                             int src_precision, int tgt_precision,
                             bool is_absolute, bool src_signed);

/*****************************************************************************/
/*                            kdsc_component_state                           */
/*****************************************************************************/

struct kdsc_component_state {
  public: // Member functions
    void update(kdu_coords next_tile_idx, kdu_codestream codestream,
                bool all_done);
      /* Called immediately after processing stripe data for a row of tiles.
         Adjusts the values of `remaining_tile_height' and `stripe_height'
         accordingly, while also updating the `buf8', `buf16', `buf32' or
         `buf_float' pointers to address the start of the next row of tiles.
         If the tile height is reduced to 0, and `all_done' is false, the
         function also uses the code-stream interface and `next_tile_idx'
         argument to figure out the vertical height of the next row of tiles,
         installing this value in the `remaining_tile_height' member. */
  public: // Data
    int comp_idx;
    int pos_x; // x-coord of left-most sample in the component
    int width; // Full width of the component
    int original_precision; // Precision recorded in SIZ marker segment
    int row_gap, sample_gap, precision; // Values supplied by `push_stripe'  
    bool is_signed; // Value supplied by `push_stripe'; always false for `buf8'  
    int buf_type; // One of `KDSC_BUFxxx'; note, 2 lsbs hold log2(bytes/sample)
    union {
      kdu_byte *buf_ptr; // Emphasizes the presence of an anonymous union
      kdu_byte *buf8;   // if `buf_type'=`KDSC_BUF8'=0
      kdu_int16 *buf16; // if `buf_type'=`KDSC_BUF16'=1
      kdu_int32 *buf32; // if `buf_type'=`KDSC_BUF32'=2
      float *buf_float; // if `buf_type'=`KDSC_BUF_FLOAT'=6
    };
    int stripe_height; // Remaining height in the current stripe
    int remaining_tile_height; // See below
    int max_tile_height;
    int max_recommended_stripe_height;
  };
  /* Notes:
       `stripe_height' holds the total number of rows in the current stripe
     which have not yet been fully processed.  This value is updated at the
     end of each row of tiles, by subtracting the smaller of `stripe_height'
     and `remaining_tile_height'.
       `remaining_tile_height' holds the number of rows in the current
     row of tiles, which have not yet been fully processed.  This value
     is updated at the end of each row of tiles, by subtracting the smaller
     of `stripe_height' and `remaining_tile_height'.
       `max_tile_height' is the maximum height of any tile in the image.
     In practice, this is the maximum of the heights of the first and
     second vertical tiles, if there are multiple tiles.
       `max_recommended_stripe_height' remembers the value returned by the
     first call to `kdu_stripe_compressor::get_recommended_stripe_heights'. */

/*****************************************************************************/
/*                              kdsc_component                               */
/*****************************************************************************/

struct kdsc_component {
    // Manages processing of a single tile-component.
  public: // Data configured by `kdsc_tile::init' for a new tile
    kdu_coords size; // Tile width, by tile rows left in tile
    bool using_shorts; // If `kdu_line_buf's hold 16-bit samples
    bool is_absolute; // If `kdu_line_buf's hold absolute integers
    int horizontal_offset; // From left edge of image component
    int ratio_counter; // See below
  public: // Data configured by `kdsc_tile::init' for a new or existing tile
    int stripe_rows_left; // Counts down from `stripe_rows' during processing
    int sample_gap; // For current stripe being processed by `push_stripe'
    int row_gap; // For current stripe being processed by `push_stripe'
    int precision; // For current stripe being processed by `push_stripe'
    bool is_signed; // For current stripe being processed by `push_stripe'
    int buf_type; // One of `KDSC_BUFxxx'; note, 2 lsbs hold log2(bytes/sample)
    union { 
      kdu_byte *buf_ptr; // Emphasizes the presence of an anonymous union
      kdu_byte *buf8;   // if `buf_type'=`KDSC_BUF8'=0
      kdu_int16 *buf16; // if `buf_type'=`KDSC_BUF16'=1
      kdu_int32 *buf32; // if `buf_type'=`KDSC_BUF32'=2
      float *buf_float; // if `buf_type'=`KDSC_BUF_FLOAT'=6
    };
    kdu_line_buf *line; // From last `kdu_multi_analysis::exchange_line' call
  public: // Data configured by `kdu_stripe_compressor::get_new_tile'
    int original_precision; // Original sample precision
    int vert_subsampling; // Vertical sub-sampling factor of this component
    int count_delta; // See below
  public: // Acceleration functions
#ifdef KDU_SIMD_OPTIMIZATIONS
  kdsc_simd_transfer_func  simd_transfer; // See below
  kdsc_component *simd_grp; // See below
  int simd_ilv; // See below
  void *simd_dst[4]; // See below
#endif
  };
  /* Notes:
        The `stripe_rows_left' member holds the number of rows in the current
     stripe, which are still to be processed by this tile-component.
        Only one of the `buf8', `buf16', `buf32' or `buf_float' members will
     be non-NULL, depending on whether the stripe data is available as bytes
     or words.  The relevant member points to the start of the first
     unprocessed row of the current tile-component.
        The `ratio_counter' is initialized to 0 at the start of each stripe
     and decremented by `count_delta' each time we consider processing this
     component within the tile.  Once the counter becomes negative, a new row
     from the stripe is processed and the ratio counter is subsequently
     incremented by `vert_subsampling'.  The value of the `count_delta'
     member is the minimum of the vertical sub-sampling factors for all
     components.  This policy ensures that we process the components in a
     proportional way.
           The `simd_transfer' function pointer, along with the other
     `simd_xxx' members, are configured by `kdsc_tile::init' based on
     functions that might be available to handle the particular conversion
     configuration for a tile-component (or collection of tile-components) in
     a vectorized manner.
        `simd_grp', `simd_ilv', `simd_dst' and `simd_lines' work together
     to support efficient transfer from interleaved component buffers, but
     the interpretation works also for non-interleaved buffers.  They
     have the following meanings:
     -- `simd_grp' points to the last component in an interleaved group; if
        NULL there is no SID implementation; the `simd_transfer' function
        is not called until we reach that component.
     -- `simd_ilv' holds the location that the current image component
        occupies within the `simd_grp' object's `simd_dst' array -- the
        address of the current `kdu_line_buf' object's internal array should
        be written to that entry prior to any call to the `simd_transfer'
        function.
     -- `simd_lines' keeps track of 
  */

/*****************************************************************************/
/*                                kdsc_tile                                  */
/*****************************************************************************/

struct kdsc_tile {
  public: // Member functions
    kdsc_tile() { components=NULL; next=NULL; }
    ~kdsc_tile()
      {
        if (components != NULL) delete[] components;
        if (engine.exists()) engine.destroy();
      }
    void init(kdu_coords idx, kdu_codestream codestream,
              kdsc_component_state *comp_states, bool force_precise,
              bool want_fastest, kdu_thread_env *env,
              kdu_thread_queue *env_queue, int env_dbuf_height);
      /* Initializes a new or partially completed tile, with a new set of
         stripe buffers, and associated parameters. If the `tile' interface
         is empty, a new code-stream tile is opened, and the various members
         of this structure and its constituent components are initialized.
         Otherwise, the tile already exists and we are supplying additional
         stripe samples.
            The pointers supplied by the `buf8' or `buf16' member of each
         element in the `comp_states' array, whichever is non-NULL,
         correspond to the first sample in what remains of the current
         stripe, within each component.  This first sample is aligned at the
         left edge of the image.  The function determines the amount by which
         the buffer should be advanced to find the first sample in the current
         tile, by comparing the horizontal location of each tile-component,
         as returned by `kdu_tile_comp::get_dims', with the values in the
         `pos_x' members of the `kdsc_component_state' entries in the
         `comp_states' array. */
    bool process(kdu_thread_env *env);
      /* Processes all the stripe rows available to tile-components in the
         current tile, returning true if the tile is completed and false
         otherwise.  Before returning true, this function destroys the various
         tile-component processing engines and line buffers and restarts the
         `allocator' object.  Always call this function right after `init'. */
  public: // Data
    kdu_tile tile;
    kdu_multi_analysis engine;
    kdsc_tile *next; // Next free tile, or next partially completed tile.
    kdu_thread_queue tile_queue;
  public: // Data configured by `kdu_stripe_compressor::get_new_tile'.
    int num_components;
    kdsc_component *components; // Array of tile-components
  };

#endif // STRIPE_COMPRESSOR_LOCAL_H
