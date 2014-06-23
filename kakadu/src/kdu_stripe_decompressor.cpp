#ifndef NO_KAKADU
/*****************************************************************************/
// File: kdu_stripe_decompressor.cpp [scope = APPS/SUPPORT]
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
   Implements the `kdu_stripe_decompressor' object.
******************************************************************************/

#include <assert.h>
#include "kdu_messaging.h"
#include "stripe_decompressor_local.h"

/* Note Carefully:
      If you want to be able to use the "kdu_text_extractor" tool to
   extract text from calls to `kdu_error' and `kdu_warning' so that it
   can be separately registered (possibly in a variety of different
   languages), you should carefully preserve the form of the definitions
   below, starting from #ifdef KDU_CUSTOM_TEXT and extending to the
   definitions of KDU_WARNING_DEV and KDU_ERROR_DEV.  All of these
   definitions are expected by the current, reasonably inflexible
   implementation of "kdu_text_extractor".
      The only things you should change when these definitions are ported to
   different source files are the strings found inside the `kdu_error'
   and `kdu_warning' constructors.  These strings may be arbitrarily
   defined, as far as "kdu_text_extractor" is concerned, except that they
   must not occupy more than one line of text.
*/
#ifdef KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("E(kdu_stripe_decompressor.cpp)",_id);
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("W(kdu_stripe_decompressor.cpp)",_id);
#  define KDU_TXT(_string) "<#>" // Special replacement pattern
#else // !KDU_CUSTOM_TEXT
#  define KDU_ERROR(_name,_id) \
     kdu_error _name("Error in Kakadu Stripe Decompressor:\n");
#  define KDU_WARNING(_name,_id) \
     kdu_warning _name("Warning in Kakadu Stripe Decompressor:\n");
#  define KDU_TXT(_string) _string
#endif // !KDU_CUSTOM_TEXT

#define KDU_ERROR_DEV(_name,_id) KDU_ERROR(_name,_id)
 // Use the above version for errors which are of interest only to developers
#define KDU_WARNING_DEV(_name,_id) KDU_WARNING(_name,_id)
 // Use the above version for warnings which are of interest only to developers


/* ========================================================================= */
/*                            Internal Functions                             */
/* ========================================================================= */

/*****************************************************************************/
/* INLINE                       transfer_bytes                               */
/*****************************************************************************/

static inline void
  transfer_bytes(kdu_byte *dst, kdu_line_buf &src, int num_samples,
                 int sample_gap, int dst_bits, int original_bits)
{
  kdu_int32 shift, val, off, mask;
  if (src.get_buf16() != NULL)
    {
      kdu_sample16 *sp = src.get_buf16();
      if (!src.is_absolute())
        {
          shift = KDU_FIX_POINT - dst_bits; assert(shift >= 0);
          off = (1<<KDU_FIX_POINT)>>1; off += (1<<shift)>>1; // For rounding
          mask = (kdu_int32)((-1)<<KDU_FIX_POINT);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_byte)(val >> shift);
            }
        }
      else if (dst_bits < original_bits)
        { // Reversible processing, need to throw away some LSB's
          shift = original_bits - dst_bits;
          off = (1<<original_bits)>>1; off += (1<<shift)>>1; // For rounding
          mask = (kdu_int32)((-1)<<original_bits);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_byte)(val >> shift);
            }
        }
      else if (dst_bits > original_bits)
        { // Reversible processing, need to synthesize extra bits
          shift = dst_bits - original_bits;
          off = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_byte)(val << shift);
            }
        }
      else
        { // Reversible processing, `src_bits'=`original_bits'
          off = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_byte) val;
            }
        }
    }
  else
    {
      kdu_sample32 *sp = src.get_buf32();
      if (!src.is_absolute())
        {
          float scale = (float)(1<<24); // High precision intermediate values
          shift = 24 - dst_bits;
          off = (kdu_int32)(1<<23); off += (1<<shift)>>1; // For rounding
          mask = (kdu_int32)((-1)<<24);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = (kdu_int32)(sp->fval*scale); val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_byte)(val >> shift);
            }
        }
      else if (dst_bits < original_bits)
        { // Reversible processing, need to throw away some LSB's
          shift = original_bits - dst_bits;
          off = (1<<original_bits)>>1; off += (1<<shift)>>1; // For rounding
          mask = (kdu_int32)((-1)<<original_bits);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_byte)(val >> shift);
            }
        }
      else if (dst_bits > original_bits)
        { // Reversible processing, need to synthesize extra bits
          shift = dst_bits - original_bits;
          off = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_byte)(val << shift);
            }
        }
      else
        { // Reversible processing, `src_bits'=`original_bits'
          off = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_byte) val;
            }
        }
    }
}

/*****************************************************************************/
/* INLINE                         pad_bytes                                  */
/*****************************************************************************/

static inline void
  pad_bytes(kdu_byte *dst, int pad_flags, int num_samples,
            int gap, int dst_bits)
{
  if (gap < 2)
    return;
  if (pad_flags & KDU_STRIPE_PAD_BEFORE)
    dst--;
  else if (pad_flags & KDU_STRIPE_PAD_AFTER)
    dst++;
  else
    return;
  kdu_byte val = (kdu_byte)(1<<(dst_bits-1));
  if (pad_flags & KDU_STRIPE_PAD_HIGH)
    val = (kdu_byte)((1<<dst_bits)-1);
  else if (pad_flags & KDU_STRIPE_PAD_LOW)
    val = 0;
  for (; num_samples > 4; num_samples-=4)
    { 
      *dst = val; dst += gap;  *dst = val; dst += gap;
      *dst = val; dst += gap;  *dst = val; dst += gap;
    }
  for (; num_samples > 0; num_samples--)
    { *dst = val; dst += gap; }
}

/*****************************************************************************/
/* INLINE                       transfer_words                               */
/*****************************************************************************/

static inline void
  transfer_words(kdu_int16 *dst, kdu_line_buf &src, int num_samples,
                 int sample_gap, int dst_bits, int original_bits,
                 bool is_signed)
{
  kdu_int32 shift, val, off, off2, mask;
  if (src.get_buf16() != NULL)
    {
      kdu_sample16 *sp = src.get_buf16();
      if (!src.is_absolute())
        {
          shift = KDU_FIX_POINT - dst_bits;
          off = off2 = (1<<KDU_FIX_POINT)>>1;
          mask = (kdu_int32)((-1)<<KDU_FIX_POINT);
          if (!is_signed)
            off2 = 0;
          if (shift >= 0)
            {
              off += (1<<shift)>>1; // For rounding
              for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
                {
                  val = sp->ival; val += off; // Make unsigned
                  if (val & mask)
                    val = (val<0)?0:~mask; // Clip to range
                  *dst = (kdu_int16)((val-off2) >> shift);
                }
            }
          else
            {
              shift = -shift;
              for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
                {
                  val = sp->ival; val += off; // Make unsigned
                  if (val & mask)
                    val = (val<0)?0:~mask; // Clip to range
                  *dst = (kdu_int16)((val-off2) << shift);
                }
            }
        }
      else if (dst_bits < original_bits)
        { // Reversible processing, need to throw away some LSB's
          shift = original_bits - dst_bits;
          off = off2 = (1<<original_bits)>>1;
          off += (1<<shift)>>1; // For rounding
          if (!is_signed)
            off2 = 0;
          mask = (kdu_int32)((-1)<<original_bits);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_int16)((val-off2) >> shift);
            }
        }
      else if (dst_bits > original_bits)
        { // Reversible processing, need to synthesize extra bits
          shift = dst_bits - original_bits;
          off = off2 = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_int16)((val-off2) << shift);
            }
        }
      else
        { // Reversible processing, `src_bits'=`original_bits'
          off = off2 = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_int16)(val-off2);
            }
        }
    }
  else
    {
      kdu_sample32 *sp = src.get_buf32();
      if (!src.is_absolute())
        {
          float scale = (float)(1<<24); // High precision intermediate values
          shift = 24 - dst_bits;
          off = off2 = (kdu_int32)(1<<23);
          off += (1<<shift)>>1; // For rounding
          mask = (kdu_int32)((-1)<<24);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = (kdu_int32)(sp->fval*scale); val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_int16)((val-off2) >> shift);
            }
        }
      else if (dst_bits < original_bits)
        { // Reversible processing, need to throw away some LSB's
          shift = original_bits - dst_bits;
          off = off2 = (1<<original_bits)>>1;
          off += (1<<shift)>>1; // For rounding
          mask = (kdu_int32)((-1)<<original_bits);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_int16)((val-off2) >> shift);
            }
        }
      else if (dst_bits > original_bits)
        { // Reversible processing, need to synthesize extra bits
          shift = dst_bits - original_bits;
          off = off2 = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_int16)((val-off2) << shift);
            }
        }
      else
        { // Reversible processing, `src_bits'=`original_bits'
          off = off2 = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (kdu_int16)(val-off2);
            }
        }
    }
}

/*****************************************************************************/
/* INLINE                       transfer_dwords                              */
/*****************************************************************************/

static inline void
  transfer_dwords(kdu_int32 *dst, kdu_line_buf &src, int num_samples,
                  int sample_gap, int dst_bits, int original_bits,
                  bool is_signed)
{
  kdu_int32 shift, val, off, off2, mask;
  if (src.get_buf16() != NULL)
    {
      kdu_sample16 *sp = src.get_buf16();
      if (!src.is_absolute())
        {
          shift = KDU_FIX_POINT - dst_bits;
          off = off2 = (1<<KDU_FIX_POINT)>>1;
          mask = (kdu_int32)((-1)<<KDU_FIX_POINT);
          if (!is_signed)
            off2 = 0;
          if (shift >= 0)
            {
              off += (1<<shift)>>1; // For rounding
              for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
                {
                  val = sp->ival; val += off; // Make unsigned
                  if (val & mask)
                    val = (val<0)?0:~mask; // Clip to range
                  *dst = (val-off2) >> shift;
                }
            }
          else
            {
              shift = -shift;
              for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
                {
                  val = sp->ival; val += off; // Make unsigned
                  if (val & mask)
                    val = (val<0)?0:~mask; // Clip to range
                  *dst = (val-off2) << shift;
                }
            }
        }
      else if (dst_bits < original_bits)
        { // Reversible processing, need to throw away some LSB's
          shift = original_bits - dst_bits;
          off = off2 = (1<<original_bits)>>1;
          off += (1<<shift)>>1; // For rounding
          if (!is_signed)
            off2 = 0;
          mask = (kdu_int32)((-1)<<original_bits);
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (val-off2) >> shift;
            }
        }
      else
        { // Reversible processing; either no shift, or shift up
          shift = dst_bits - original_bits;
          off = off2 = (1<<original_bits)>>1;
          mask = (kdu_int32)((-1)<<original_bits);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (val-off2) << shift;
            }
        }
    }
  else
    {
      kdu_sample32 *sp = src.get_buf32();
      if (!src.is_absolute())
        {
          float fval, scale, max_fval;
          if (dst_bits > 16)
            scale = (float)(1<<16) * (float)(1<<(dst_bits-16));
          else
            scale = (float)(1<<dst_bits);
          off = (is_signed)?(1<<(dst_bits-1)):0;
          if (original_bits > 16)
            max_fval = 1.0F - ((1.0F / (1<<16)) / (1<<(original_bits-16)));
          else
            max_fval = 1.0F - (1.0F / (1<<original_bits));
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              fval = sp->fval + 0.5F; // Make it unsigned
              if (fval < 0.0F)
                fval = -0.0F;
              else if (fval > max_fval)
                fval = max_fval;
              *dst = ((kdu_int32)(fval*scale+0.5F)) - off;
            }
        }
      else if (dst_bits < original_bits)
        { // Reversible processing, need to throw away some LSB's
          shift = original_bits - dst_bits;
          off = off2 = (1<<(original_bits-1));
          off += (1<<shift)>>1; // For rounding
          mask = (kdu_int32)((-1)<<original_bits);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (val-off2) >> shift;
            }
        }
      else
        { // Reversible processing; either no shift, or shift up
          shift = dst_bits - original_bits;
          off = off2 = (1<<(original_bits-1));
          mask = (kdu_int32)((-1)<<original_bits);
          if (!is_signed)
            off2 = 0;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            {
              val = sp->ival; val += off; // Make unsigned
              if (val & mask)
                val = (val<0)?0:~mask; // Clip to range
              *dst = (val-off2) << shift;
            }
        }
    }
}

/*****************************************************************************/
/* INLINE                       transfer_floats                              */
/*****************************************************************************/

static inline void
  transfer_floats(float *dst, kdu_line_buf &src, int num_samples,
                  int sample_gap, int dst_bits, int original_bits,
                  bool is_signed)
{
  int src_bits = (src.is_absolute())?original_bits:KDU_FIX_POINT;
  float src_scale = 1.0F; // Amount required to scale src to unit dynamic range
  while (src_bits > 0)
    { src_bits -= 16; src_scale *= 1.0F / (float)(1<<16); }
  src_scale *= (float)(1<<(-src_bits));

  float dst_scale = 1.0F; // Amount to scale from unit range to dst
  while (dst_bits < 0)
    { dst_bits += 16; dst_scale *= 1.0F/(float)(1<<16); }
  while (dst_bits > 16)
    { dst_bits -= 16; dst_scale *= (float)(1<<16); }
  dst_scale *= (float)(1<<dst_bits);

  float offset = (!is_signed)?(0.5F*dst_scale):0.0F;
  if (src.get_buf16() != NULL)
    {
      float scale = dst_scale * src_scale;
      kdu_sample16 *sp = src.get_buf16();
      for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
        *dst = sp->ival * scale + offset;
    }
  else
    {
      kdu_sample32 *sp = src.get_buf32();
      if (src.is_absolute())
        {
          float scale = dst_scale * src_scale;
          for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
            *dst = sp->ival * scale + offset;
        }
      else
        for (; num_samples > 0; num_samples--, dst+=sample_gap, sp++)
          *dst = sp->fval * dst_scale + offset;
    }
}


/* ========================================================================= */
/*                           kdsd_component_state                            */
/* ========================================================================= */

/*****************************************************************************/
/*                       kdsd_component_state::update                        */
/*****************************************************************************/

void
  kdsd_component_state::update(kdu_coords next_tile_idx,
                               kdu_codestream codestream, bool all_done)
{
  int increment = stripe_height;
  if (increment > remaining_tile_height)
    increment = remaining_tile_height;
  stripe_height -= increment;
  remaining_tile_height -= increment;
  int adj = increment*row_gap;
  int log2_bps = buf_type & 3; // 2 LSB's hold log_2(bytes/sample)
  assert(log2_bps <= 2);
  buf_ptr += adj << log2_bps;
  if ((remaining_tile_height > 0) || all_done)
    return;
  kdu_dims dims; codestream.get_tile_dims(next_tile_idx,comp_idx,dims,true);
  remaining_tile_height = dims.size.y;
}


/* ========================================================================= */
/*                                kdsd_tile                                  */
/* ========================================================================= */

/*****************************************************************************/
/*                             kdsd_tile::init                               */
/*****************************************************************************/

void
  kdsd_tile::init(kdu_coords idx, kdu_codestream codestream,
                  kdsd_component_state *comp_states, bool force_precise,
                  bool want_fastest, kdu_thread_env *env,
                  kdu_thread_queue *env_queue, int env_dbuf_height)
{
  int c;
  if (!tile.exists())
    {
      tile = codestream.open_tile(idx,env);
      if (env != NULL)
        env->attach_queue(&tile_queue,env_queue,NULL);
      bool double_buffering = (env != NULL) && (env_dbuf_height != 0);
      engine.create(codestream,tile,force_precise,false,want_fastest,
                    (double_buffering)?env_dbuf_height:1,env,&tile_queue,
                    double_buffering);
      assert(components != NULL);
      for (c=0; c < num_components; c++)
        { 
          kdsd_component *comp = components + c;
          kdsd_component_state *cs = comp_states + c;
          comp->size = engine.get_size(c);
          comp->using_shorts = !engine.is_line_precise(c);
          comp->is_absolute = engine.is_line_absolute(c);
          kdu_dims dims;  codestream.get_tile_dims(idx,c,dims,true);
          comp->horizontal_offset = dims.pos.x - cs->pos_x;
          assert((comp->size == dims.size) && (comp->horizontal_offset >= 0));
          comp->ratio_counter = 0;
          comp->stripe_rows_left = 0;
        }
    }

  // Now go through the components, assigning buffers and counters
  for (c=0; c < num_components; c++)
    { 
      kdsd_component *comp = components + c;
      kdsd_component_state *cs = comp_states + c;
      assert(comp->stripe_rows_left == 0);
      assert(cs->remaining_tile_height == comp->size.y);
      comp->stripe_rows_left = cs->stripe_height;
      if (comp->stripe_rows_left > comp->size.y)
        comp->stripe_rows_left = comp->size.y;
      comp->sample_gap = cs->sample_gap;
      comp->row_gap = cs->row_gap;
      comp->precision = cs->precision;
      comp->is_signed = cs->is_signed;
      comp->buf_type = cs->buf_type;
      comp->buf_ptr = cs->buf_ptr;
      comp->pad_flags = cs->pad_flags;
      if ((comp->pad_flags & (KDU_STRIPE_PAD_BEFORE|KDU_STRIPE_PAD_AFTER)) ==
          (KDU_STRIPE_PAD_BEFORE|KDU_STRIPE_PAD_AFTER))
        comp->pad_flags &= ~KDU_STRIPE_PAD_AFTER; // Don't allow both
      int adj = comp->horizontal_offset * comp->sample_gap;
      int log2_bps = comp->buf_type & 3; // 2 LSB's hold log_2(bytes/sample)
      assert(log2_bps <= 2);
      comp->buf_ptr += adj << log2_bps;
    }

#ifdef KDU_SIMD_OPTIMIZATIONS
  // Finally, let's see if there are fast transfer functions that can be
  // used for the current configuration.  This may depend upon interleaving
  // patterns.
  int ilv_count=0; // Num elts in potential interleave group, so far
  kdu_byte *ilv_ptrs[4]; // Buf_ptr for each elt in potential ilv group so far
  kdsd_component *ilv_comps[5]; // Components contributing each elt in group
  for (c=0; c < num_components; c++)
    { 
      kdsd_component *comp = components + c;
      int log2_bps = comp->buf_type & 3; // 2 LSB's hold log_2(bytes/sample)
      comp->simd_transfer = NULL;
      comp->simd_grp = NULL;
      comp->simd_ilv = comp->simd_padded_ilv = -1;
      KDSD_FIND_SIMD_TRANSFER_FUNC(comp->simd_transfer,comp->buf_type,
                                   comp->using_shorts,comp->sample_gap,
                                   comp->precision,comp->original_precision,
                                   comp->is_absolute);
      if (comp->simd_transfer == NULL)
        { // No point in looking for an interleave group
          ilv_count = 0;
          continue;
        }
      int new_comps = 1 +
        ((comp->pad_flags & (KDU_STRIPE_PAD_BEFORE|KDU_STRIPE_PAD_AFTER))?1:0);
      if (comp->sample_gap <= new_comps)
        { // Component is not interleaved
          ilv_count = 0;
          comp->simd_grp = comp;
          comp->simd_ilv = 0;
          continue;
        }

      // Try to start or finish building an interleave group
      if ((ilv_count > 0) && (comp->sample_gap <= 4) &&
          (comp->sample_gap >= (ilv_count+new_comps)) &&
          (comp->size == comp[-1].size) &&
          (comp->using_shorts == comp[-1].using_shorts) &&
          (comp->is_absolute == comp[-1].is_absolute) &&
          (comp->sample_gap == comp[-1].sample_gap) &&
          (comp->row_gap == comp[-1].row_gap)  &&
          (comp->buf_type == comp[-1].buf_type))
        { // Augment existing potential interleave group
          ilv_ptrs[ilv_count] = comp->buf_ptr;
          ilv_comps[ilv_count] = ilv_comps[ilv_count+1] = comp;
          if (comp->pad_flags & KDU_STRIPE_PAD_BEFORE)
            ilv_ptrs[ilv_count+1] = comp->buf_ptr - (int)(1<<log2_bps);
          else if (comp->pad_flags & KDU_STRIPE_PAD_AFTER)
            ilv_ptrs[ilv_count+1] = comp->buf_ptr + (int)(1<<log2_bps);
          ilv_count += new_comps;
          if (ilv_count < comp->sample_gap)
            continue; // Need to keep building the group

          // See if `ilv_ptrs' is compatible with a true interleave group
          int j, k;
          kdu_byte *base_ptr = ilv_ptrs[0];
          for (j=0; j < ilv_count; j++)
            { // Find base interleaving address and prepare to generate
              // `simd_ilv' and `simd_padded_ilv' indices in the next step.
              if (base_ptr > ilv_ptrs[j])
                base_ptr = ilv_ptrs[j];
              ilv_comps[j]->simd_ilv = -1;
              ilv_comps[j]->simd_padded_ilv = -1;
            }
          for (j=0; j < ilv_count; j++)
            { 
              int ilv_off = (int)((ilv_ptrs[j] - base_ptr) >> log2_bps);
              for (k=0; k < j; k++)
                if (ilv_ptrs[k] == ilv_ptrs[j])
                  { // Not fully interleaved
                    ilv_off = comp->sample_gap; // Forces test below to fail
                    break;
                  }
              if (ilv_off >= comp->sample_gap)
                { // Not actually interleaved
                  ilv_count = 0; break;
                }
              if (ilv_comps[j]->simd_ilv < 0)
                ilv_comps[j]->simd_ilv = ilv_off;
              else if (ilv_comps[j]->simd_padded_ilv < 0)
                ilv_comps[j]->simd_padded_ilv = ilv_off;
              else
                assert(0);
            }
          if (ilv_count > 0)
            { // Finish configuring state information for SIMD interleaving
              comp->simd_src[0] = comp->simd_src[1] =
                comp->simd_src[2] = comp->simd_src[3] = NULL;
              for (j=0; j < ilv_count; j++)
                { 
                  kdsd_component *ic = ilv_comps[j];
                  if (ic->simd_grp == NULL)
                    ic->simd_grp = comp;
                  else
                    { // This one must be a padding channel
                      kdu_int32 pad_val = 0;
                      if (ic->using_shorts)
                        { // Create duplicated 16-bit padding value
                          if (ic->is_absolute)
                            pad_val = 1 << (ic->original_precision-1);
                          else
                            pad_val = 1 << (KDU_FIX_POINT-1);
                          if (ic->pad_flags & KDU_STRIPE_PAD_HIGH)
                            pad_val = pad_val-1;
                          else if (ic->pad_flags & KDU_STRIPE_PAD_LOW)
                            pad_val = -pad_val;
                          else
                            pad_val = 0;
                          pad_val = (pad_val<<16) | (pad_val & 0x0FFFF);
                        }
                      else if (ic->is_absolute)
                        { // Create 32-bit absolute integer padding value
                          pad_val = 1 << (ic->original_precision-1);
                          if (ic->pad_flags & KDU_STRIPE_PAD_HIGH)
                            pad_val = pad_val-1;
                          else if (ic->pad_flags & KDU_STRIPE_PAD_LOW)
                            pad_val = -pad_val;
                          else
                            pad_val = 0;
                        }
                      else
                        { // Create floating point padding value
                          union {
                            float aliased_float;
                            kdu_int32 aliased_int;
                          };
                          if (ic->pad_flags & KDU_STRIPE_PAD_HIGH)
                            aliased_float = 0.5F;
                          else if (ic->pad_flags & KDU_STRIPE_PAD_LOW)
                            aliased_float = -0.5F;
                          else
                            aliased_float = 0.0F;
                          pad_val = aliased_int;
                        }
                      int pad_elts = ic->size.x;
                      if ((ic->simd_pad_buf == NULL) ||
                          (pad_elts != ic->simd_pad_buf_elts))
                        { // Allocate and align32 buffer, with space to read
                          // up to 128 bytes beyond the nominal width.
                          if (ic->simd_pad_handle != NULL)
                            { delete[] ic->simd_pad_handle;
                              ic->simd_pad_handle = NULL; }
                          ic->simd_pad_handle = new kdu_int32[pad_elts+7+32];
                          ic->simd_pad_buf = ic->simd_pad_handle;
                          int align_off = _addr_to_kdu_int32(ic->simd_pad_buf);
                          ic->simd_pad_buf += ((-align_off) & 0x1F) >> 2;
                          ic->simd_pad_buf[0] = pad_val-1; // Force initialize    
                          ic->simd_pad_buf_elts = pad_elts;
                        }
                      if (ic->simd_pad_buf[0] != pad_val)
                        for (int i=0; i < pad_elts; i++)
                          ic->simd_pad_buf[i] = pad_val;
                    }
                }
              continue;
            }
        }
      
      // If we get here, we are not part of an existing interleave group
      // Start building a new potential interleave group.
      assert(new_comps < comp->sample_gap); // See test up above
      ilv_count = new_comps;
      ilv_comps[0] = ilv_comps[1] = comp;
      ilv_ptrs[0] = comp->buf_ptr;
      if (comp->pad_flags & KDU_STRIPE_PAD_BEFORE)
        ilv_ptrs[1] = comp->buf_ptr - (int)(1<<log2_bps);
      else if (comp->pad_flags & KDU_STRIPE_PAD_AFTER)
        ilv_ptrs[1] = comp->buf_ptr + (int)(1<<log2_bps);
    }
#endif // KDU_SIMD_OPTIMIZATIONS
}

/*****************************************************************************/
/*                            kdsd_tile::process                             */
/*****************************************************************************/

bool
  kdsd_tile::process(kdu_thread_env *env)
{
  int c;
  bool tile_complete = false;
  bool done = false;
  while (!done)
    {
      done = true;
      tile_complete = true;
      for (c=0; c < num_components; c++)
        { 
          kdsd_component *comp = components + c;
          if (comp->size.y > 0)
            tile_complete = false;
          if (comp->stripe_rows_left == 0)
            continue;
          done = false;
          comp->ratio_counter -= comp->count_delta;
          if (comp->ratio_counter >= 0)
            continue;

          comp->size.y--;
          comp->stripe_rows_left--;
          comp->ratio_counter += comp->vert_subsampling;

          int log2_bps = comp->buf_type & 3;
          kdu_line_buf *line = engine.get_line(c,env);
          assert(line != NULL);
          bool need_to_write_pad_vals =
            (comp->pad_flags&(KDU_STRIPE_PAD_BEFORE|KDU_STRIPE_PAD_AFTER))!=0;
#ifdef KDU_SIMD_OPTIMIZATIONS
          kdsd_component *grp = comp->simd_grp;
          if (grp != NULL)
            { 
              assert(comp->simd_transfer != NULL);
              int ilv = comp->simd_ilv;
              assert((ilv >= 0) && (ilv < 4));
              if (comp->using_shorts)
                grp->simd_src[ilv] = line->get_buf16();
              else 
                grp->simd_src[ilv] = line->get_buf32();
              if ((ilv=comp->simd_padded_ilv) >= 0)
                grp->simd_src[ilv] = comp->simd_pad_buf;
              else
                need_to_write_pad_vals = false;
              if (grp == comp)
                comp->simd_transfer(comp->buf_ptr-(comp->simd_ilv<<log2_bps),
                                    comp->simd_src,comp->size.x,
                                    comp->precision,comp->original_precision,
                                    comp->is_absolute,comp->is_signed);
            }
          else
#endif // KDU_SIMD_OPTIMIZATIONS
          if (comp->buf_type == KDSD_BUF8)
            transfer_bytes(comp->buf8,*line,comp->size.x,
                           comp->sample_gap,comp->precision,
                           comp->original_precision);
          else if (comp->buf_type == KDSD_BUF16)
            transfer_words(comp->buf16,*line,comp->size.x,
                           comp->sample_gap,comp->precision,
                           comp->original_precision,comp->is_signed);
          else if (comp->buf_type == KDSD_BUF32)
            transfer_dwords(comp->buf32,*line,comp->size.x,
                            comp->sample_gap,comp->precision,
                            comp->original_precision,comp->is_signed);
          else if (comp->buf_type == KDSD_BUF_FLOAT)
            transfer_floats(comp->buf_float,*line,comp->size.x,
                            comp->sample_gap,comp->precision,
                            comp->original_precision,comp->is_signed);
          else
            assert(0);
          if (need_to_write_pad_vals)
            { 
              assert(comp->buf_type == KDSD_BUF8);
              pad_bytes(comp->buf8,comp->pad_flags,comp->size.x,
                        comp->sample_gap,comp->precision);
            }
          comp->buf_ptr += comp->row_gap << log2_bps;
        }
    }
  if (!tile_complete)
    return false;

  // Clean up
  if (env != NULL)
    env->terminate(&tile_queue,false);
  engine.destroy(env);
  tile.close(env);
  tile = kdu_tile(NULL);
  return true;
}


/* ========================================================================= */
/*                          kdu_stripe_decompressor                          */
/* ========================================================================= */

/*****************************************************************************/
/*                  kdu_stripe_decompressor::get_new_tile                    */
/*****************************************************************************/

kdsd_tile *
  kdu_stripe_decompressor::get_new_tile()
{
  kdsd_tile *tp = free_tiles;
  if (tp == NULL)
    {
      int c, min_subsampling;
      tp = new kdsd_tile;
      tp->num_components = num_components;
      tp->components = new kdsd_component[num_components];
      for (c=0; c < num_components; c++)
        {
          tp->components[c].original_precision =
            comp_states[c].original_precision;
          kdu_coords subs; codestream.get_subsampling(c,subs,true);
          tp->components[c].vert_subsampling = subs.y;
          if ((c == 0) || (subs.y < min_subsampling))
            min_subsampling = subs.y;
        }
      for (c=0; c < num_components; c++)
        tp->components[c].count_delta = min_subsampling;
    }
  else
    free_tiles = tp->next;
  tp->next = NULL;
  return tp;
}

/*****************************************************************************/
/*                  kdu_stripe_decompressor::release_tile                    */
/*****************************************************************************/

void
  kdu_stripe_decompressor::release_tile(kdsd_tile *tp)
{
  tp->next = free_tiles;
  free_tiles = tp;
}

/*****************************************************************************/
/*              kdu_stripe_decompressor::kdu_stripe_decompressor             */
/*****************************************************************************/

kdu_stripe_decompressor::kdu_stripe_decompressor()
{
  force_precise = false;
  want_fastest = false;
  all_done = true;
  num_components = 0;
  left_tile_idx = num_tiles = kdu_coords(0,0);
  partial_tiles = free_tiles = NULL;
  comp_states = NULL;
  env = NULL; env_dbuf_height = 0;
}

/*****************************************************************************/
/*                       kdu_stripe_decompressor::start                      */
/*****************************************************************************/

void
  kdu_stripe_decompressor::start(kdu_codestream codestream, bool force_precise,
                               bool want_fastest, kdu_thread_env *env,
                               kdu_thread_queue *env_queue, int env_dbuf_height)
{
  assert((partial_tiles == NULL) && (free_tiles == NULL) &&
         (this->comp_states == NULL) && !this->codestream.exists() &&
         (this->env == NULL));

  // Start by getting some preliminary parameters
  this->codestream = codestream;
  this->force_precise = force_precise;
  this->want_fastest = want_fastest;
  this->num_components = codestream.get_num_components(true);
  kdu_dims tile_indices; codestream.get_valid_tiles(tile_indices);
  this->num_tiles = tile_indices.size;
  this->left_tile_idx = tile_indices.pos;

  // Finalize preparation for decompression
  all_done = false;
  comp_states = new kdsd_component_state[num_components];
  for (int n=0; n < num_components; n++)
    { 
      kdsd_component_state *cs = comp_states + n;
      cs->comp_idx = n;
      kdu_dims dims; codestream.get_dims(n,dims,true);
      cs->pos_x = dims.pos.x; // Values used by `kdsd_tile::init'
      cs->width = dims.size.x;
      cs->original_precision = codestream.get_bit_depth(n,true);
      if (cs->original_precision < 0)
        cs->original_precision = -cs->original_precision;
      cs->row_gap = cs->sample_gap = cs->precision = 0;
      cs->buf_ptr = NULL;  cs->buf_type = -1;
      cs->pad_flags = 0;
      cs->stripe_height = 0;
      kdu_coords idx = tile_indices.pos;
      codestream.get_tile_dims(idx,n,dims,true);
      cs->remaining_tile_height = dims.size.y;
      cs->max_tile_height = dims.size.y;
      if (tile_indices.size.y > 1)
        {
          idx.y++;
          codestream.get_tile_dims(idx,n,dims,true);
          if (dims.size.y > cs->max_tile_height)
            cs->max_tile_height = dims.size.y;
        }
      cs->max_recommended_stripe_height = 0; // Until we assign one
    }

  // Install multi-threaded environment
  if ((this->env = env) != NULL)
    env->attach_queue(&local_env_queue,env_queue,NULL);
  this->env_dbuf_height = env_dbuf_height;
}

/*****************************************************************************/
/*                      kdu_stripe_decompressor::finish                      */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::finish()
{
  if (env != NULL)
    { // Terminate the multi-threaded processing queues
      env->terminate(&local_env_queue,false);
      env = NULL;  env_dbuf_height = 0;
    }

  if (!codestream.exists())
    {
      assert((comp_states == NULL) && (partial_tiles == NULL) &&
             (free_tiles == NULL));
      return false;
    }

  if (comp_states != NULL)
    delete[] comp_states;
  comp_states = NULL;
  kdsd_tile *tp;
  while ((tp=partial_tiles) != NULL)
    { 
      partial_tiles = tp->next;
      if (tp->tile.exists())
        tp->tile.close();
      delete tp;
    }
  while ((tp=free_tiles) != NULL)
    { 
      free_tiles = tp->next;
      delete tp;
    }
  codestream = kdu_codestream(); // Make the interface empty
  return all_done;
}

/*****************************************************************************/
/*          kdu_stripe_decompressor::get_recommended_stripe_heights          */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::get_recommended_stripe_heights(int preferred_min,
                                                          int absolute_max,
                                                          int rec_heights[],
                                                          int *max_heights)
{
  if (preferred_min < 1)
    preferred_min = 1;
  if (absolute_max < preferred_min)
    absolute_max = preferred_min;
  if (!codestream.exists())
    { KDU_ERROR_DEV(e,1); e <<
        KDU_TXT("You may not call `kdu_stripe_decompressor's "
        "`get_recommended_stripe_heights' function without first calling the "
        "`start' function.");
    }
  int c, max_val;

  if (comp_states[0].max_recommended_stripe_height==0)
    { // Need to assign max recommended stripe heights, based on max tile size
      for (max_val=0, c=0; c < num_components; c++)
        {
          comp_states[c].max_recommended_stripe_height =
            comp_states[c].max_tile_height;
          if (comp_states[c].max_tile_height > max_val)
            max_val = comp_states[c].max_tile_height;
        }
      int limit = (num_tiles.x==1)?preferred_min:absolute_max;
      if (limit < max_val)
        {
          int scale = 1 + ((max_val-1) / limit);
          for (c=0; c < num_components; c++)
            {
              comp_states[c].max_recommended_stripe_height =
                1 + (comp_states[c].max_tile_height / scale);
              if (comp_states[c].max_recommended_stripe_height > limit)
                comp_states[c].max_recommended_stripe_height = limit;
            }
        }
    }

  for (max_val=0, c=0; c < num_components; c++)
    {
      kdsd_component_state *cs = comp_states + c;
      rec_heights[c] = cs->remaining_tile_height;
      if (rec_heights[c] > max_val)
        max_val = rec_heights[c];
      if (max_heights != NULL)
        max_heights[c] = cs->max_recommended_stripe_height;
    }
  int limit = (num_tiles.x==1)?preferred_min:absolute_max;
  if (limit < max_val)
    {
      int scale = 1 + ((max_val-1) / limit);
      for (c=0; c < num_components; c++)
        rec_heights[c] = 1 + (rec_heights[c] / scale);
    }
  for (c=0; c < num_components; c++)
    {
      if (rec_heights[c] > comp_states[c].max_recommended_stripe_height)
        rec_heights[c] = comp_states[c].max_recommended_stripe_height;
      if (rec_heights[c] > comp_states[c].remaining_tile_height)
        rec_heights[c] = comp_states[c].remaining_tile_height;
    }
  return (num_tiles.x > 1);
}

/*****************************************************************************/
/*                kdu_stripe_decompressor::pull_stripe (8-bit)               */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_stripe(kdu_byte *stripe_bufs[],
                                       int heights[], int *sample_gaps,
                                       int *row_gaps, int *precisions,
                                       int *pad_flags)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsd_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_type = KDSD_BUF8;
      cs->buf8 = stripe_bufs[c];
      cs->pad_flags = (pad_flags==NULL)?0:(pad_flags[c]);
      cs->stripe_height = heights[c];
      cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?8:(precisions[c]);
      cs->is_signed = false;
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 8) cs->precision = 8;
    }
  return pull_common();
}

/*****************************************************************************/
/*            kdu_stripe_decompressor::pull_stripe (8-bit, single buf)       */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_stripe(kdu_byte *buffer, int heights[],
                                       int *sample_offsets, int *sample_gaps,
                                       int *row_gaps, int *precisions,
                                       int *pad_flags)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsd_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_type = KDSD_BUF8;
      cs->buf8 = buffer + ((sample_offsets==NULL)?c:(sample_offsets[c]));
      cs->pad_flags = (pad_flags==NULL)?0:(pad_flags[c]);
      cs->stripe_height = heights[c];
      if ((sample_offsets == NULL) && (sample_gaps == NULL))
        cs->sample_gap = num_components;
      else
        cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?8:(precisions[c]);
      cs->is_signed = false;
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 8) cs->precision = 8;
    }
  return pull_common();
}

/*****************************************************************************/
/*                 kdu_stripe_decompressor::pull_stripe (words)              */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_stripe(kdu_int16 *stripe_bufs[],
                                       int heights[], int *sample_gaps,
                                       int *row_gaps, int *precisions,
                                       bool *is_signed)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsd_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_type = KDSD_BUF16;
      cs->buf16 = stripe_bufs[c];
      cs->pad_flags = 0;
      cs->stripe_height = heights[c];
      cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?16:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 16) cs->precision = 16;
    }
  return pull_common();
}

/*****************************************************************************/
/*           kdu_stripe_decompressor::pull_stripe (words, single buf)        */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_stripe(kdu_int16 *buffer, int heights[],
                                       int *sample_offsets, int *sample_gaps,
                                       int *row_gaps, int *precisions,
                                       bool *is_signed)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsd_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_type = KDSD_BUF16;
      cs->buf16 = buffer + ((sample_offsets==NULL)?c:(sample_offsets[c]));
      cs->pad_flags = 0;
      cs->stripe_height = heights[c];
      if ((sample_offsets == NULL) && (sample_gaps == NULL))
        cs->sample_gap = num_components;
      else
        cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?16:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 16) cs->precision = 16;
    }
  return pull_common();
}

/*****************************************************************************/
/*                 kdu_stripe_decompressor::pull_stripe (dwords)             */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_stripe(kdu_int32 *stripe_bufs[],
                                       int heights[], int *sample_gaps,
                                       int *row_gaps, int *precisions,
                                       bool *is_signed)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsd_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_type = KDSD_BUF32;
      cs->buf32 = stripe_bufs[c];
      cs->pad_flags = 0;
      cs->stripe_height = heights[c];
      cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?32:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 32) cs->precision = 32;
    }
  return pull_common();
}

/*****************************************************************************/
/*          kdu_stripe_decompressor::pull_stripe (dwords, single buf)        */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_stripe(kdu_int32 *buffer, int heights[],
                                       int *sample_offsets, int *sample_gaps,
                                       int *row_gaps, int *precisions,
                                       bool *is_signed)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsd_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_type = KDSD_BUF32;
      cs->buf32 = buffer + ((sample_offsets==NULL)?c:(sample_offsets[c]));
      cs->pad_flags = 0;
      cs->stripe_height = heights[c];
      if ((sample_offsets == NULL) && (sample_gaps == NULL))
        cs->sample_gap = num_components;
      else
        cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?32:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < 1) cs->precision = 1;
      if (cs->precision > 32) cs->precision = 32;
    }
  return pull_common();
}

/*****************************************************************************/
/*                 kdu_stripe_decompressor::pull_stripe (floats)             */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_stripe(float *stripe_bufs[],
                                       int heights[], int *sample_gaps,
                                       int *row_gaps, int *precisions,
                                       bool *is_signed)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsd_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_type = KDSD_BUF_FLOAT;
      cs->buf_float = stripe_bufs[c];
      cs->stripe_height = heights[c];
      cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?0:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < -64) cs->precision = -64;
      if (cs->precision > 64) cs->precision = 64;
    }
  return pull_common();
}

/*****************************************************************************/
/*          kdu_stripe_decompressor::pull_stripe (floats, single buf)        */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_stripe(float *buffer, int heights[],
                                       int *sample_offsets, int *sample_gaps,
                                       int *row_gaps, int *precisions,
                                       bool *is_signed)
{
  int c;
  assert(codestream.exists());
  for (c=0; c < num_components; c++)
    { // Copy pointers so they can be internally updated during processing
      kdsd_component_state *cs=comp_states+c;
      assert(cs->stripe_height == 0);
      cs->buf_type = KDSD_BUF_FLOAT;
      cs->buf_float = buffer + ((sample_offsets==NULL)?c:(sample_offsets[c]));
      cs->stripe_height = heights[c];
      if ((sample_offsets == NULL) && (sample_gaps == NULL))
        cs->sample_gap = num_components;
      else
        cs->sample_gap = (sample_gaps==NULL)?1:(sample_gaps[c]);
      cs->row_gap = (row_gaps==NULL)?(cs->width*cs->sample_gap):(row_gaps[c]);
      cs->precision = (precisions==NULL)?0:(precisions[c]);
      cs->is_signed = (is_signed==NULL)?true:(is_signed[c]);
      if (cs->precision < -64) cs->precision = -64;
      if (cs->precision > 64) cs->precision = 64;
    }
  return pull_common();
}

/*****************************************************************************/
/*                   kdu_stripe_decompressor::pull_common                    */
/*****************************************************************************/

bool
  kdu_stripe_decompressor::pull_common()
{
  if (num_tiles.y <= 0)
    return false; // The application probably ignored a previous false return
  int c;
  bool pull_complete = false;
  while (!pull_complete)
    {
      int t;
      kdu_coords tile_idx=left_tile_idx;
      kdsd_tile *next_tp=NULL, *tp=partial_tiles;
      for (t=num_tiles.x; t > 0; t--, tile_idx.x++, tp=next_tp)
        { 
          if (tp == NULL)
            tp = partial_tiles = get_new_tile();
          tp->init(tile_idx,codestream,comp_states,force_precise,
                   want_fastest,env,&local_env_queue,env_dbuf_height);
          if (tp->process(env))
            { // Tile is completed
              assert(tp == partial_tiles);
              next_tp = partial_tiles = tp->next;
              release_tile(tp);
            }
          else
            { // Not enough data to complete tile
              if ((t > 1) && ((next_tp = tp->next) == NULL))
                next_tp = tp->next = get_new_tile(); // Build on the list
            }
        }

      // See if the entire row of tiles is complete or not
      if (partial_tiles == NULL)
        {
          left_tile_idx.y++; num_tiles.y--;
          all_done = (num_tiles.y == 0);
        }
      pull_complete = true;
      for (c=0; c < num_components; c++)
        {
          comp_states[c].update(left_tile_idx,codestream,all_done);
          if (comp_states[c].stripe_height > 0)
            pull_complete = false;
        }
      if ((partial_tiles != NULL) && !pull_complete)
        { KDU_ERROR_DEV(e,2); e <<
            KDU_TXT("Inappropriate use of `kdu_stripe_decompressor' "
            "object.  Image component samples must not be processed by this "
            "object in such disproportionate fashion as to require the object "
            "to maintain multiple rows of open tile pointers!  See description "
            "of the `kdu_stripe_decompressor::pull_line' interface function "
            "for more details on how to use it correctly.");
        }
    }
  return !all_done;
}
#endif
