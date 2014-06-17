/*****************************************************************************/
// File: kdu_compressed.h [scope = CORESYS/COMMON]
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
   This file provides the key interfaces between the Kakadu framework
and an application which uses its services.  In particular, it defines the
services required to create a codestream interface object and to access the
various sub-ordinate elements: tiles, tile-components, resolutions, subbands
and code-blocks.  It provides high level interfaces which isolate the
application from the actual machinery used to incrementally read, write and
transform JPEG2000 code-streams.  The key data processing objects attach to
one or more of these code-stream interfaces.
   You are strongly encouraged to use the services provided here as-is and
not to meddle with the internal machinery.  Most services which you should
require for compression, decompression, transcoding and interactive
applications are provided.
******************************************************************************/

#ifndef KDU_COMPRESSED_H
#define KDU_COMPRESSED_H

#include <time.h>
#include "kdu_threads.h"
#include "kdu_params.h"
#include "kdu_kernels.h"

// Defined here:
struct kdu_nc_coords;
struct kdu_coords;
struct kdu_dims;

class kdu_codestream_comment;

class kdu_compressed_source;
class kdu_compressed_source_nonnative;
class kdu_compressed_target;
class kdu_compressed_target_nonnative;

class kdu_codestream;
class kdu_memory_limit_handler;
class kdu_tile;
class kdu_tile_comp;
class kdu_resolution;
class kdu_node;
class kdu_subband;
class kdu_precinct;
struct kdu_block;
class kdu_thread_env;

// Referenced here, defined inside the private implementation
class kd_codestream_comment;
struct kd_codestream;
struct kd_tile;
struct kd_tile_comp;
struct kd_resolution;
struct kd_leaf_node;
struct kd_subband;
struct kd_precinct;
class kd_block;


/* ========================================================================= */
/*                         Core System Version                               */
/* ========================================================================= */

#define KDU_CORE_VERSION "v7.3.3"

KDU_EXPORT const char *
  kdu_get_core_version();
  /* [SYNOPSIS]
       This function returns a string such as "v3.2.1", indicating the version
       number of the Kakadu core system.  In many cases, the Kakadu core system
       may be compiled as a DLL or shared library, in which case this function
       will return the version which was current when the DLL or shared library
       was compiled.
       [//]
       The macro, KDU_CORE_VERSION, may also be included in compiled
       applications.  It will evaluate to the core version string at the
       time when the application was compiled.  You may use this to check
       for differences between the version of a core system DLL or shared
       library which is being linked in at run time, and the the version
       against which the application was originally compiled.
  */


/* ========================================================================= */
/*                             Marker Codes                                  */
/* ========================================================================= */

#define KDU_SOC ((kdu_uint16) 0xFF4F)
                // Delimiting marker         -- processed in "codestream.cpp"
#define KDU_SOT ((kdu_uint16) 0xFF90)
                // Delimiting marker segment -- processed in "compressed.cpp"
#define KDU_SOD ((kdu_uint16) 0xFF93)
                // Delimiting marker         -- processed in "compressed.cpp"
#define KDU_SOP ((kdu_uint16) 0xFF91)
                // In-pack-stream marker     -- processed in "compressed.cpp"
#define KDU_EPH ((kdu_uint16) 0xFF92)
                // In-pack-stream marker     -- processed in "compressed.cpp"
#define KDU_EOC ((kdu_uint16) 0xFFD9)
                // Delimiting marker         -- processed in "codestream.cpp"

#define KDU_SIZ ((kdu_uint16) 0xFF51)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_CAP ((kdu_uint16) 0xFF50)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_CBD ((kdu_uint16) 0xFF78)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_MCT ((kdu_uint16) 0xFF74)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_MCC ((kdu_uint16) 0xFF75)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_MCO ((kdu_uint16) 0xFF77)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_COD ((kdu_uint16) 0xFF52)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_COC ((kdu_uint16) 0xFF53)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_QCD ((kdu_uint16) 0xFF5C)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_QCC ((kdu_uint16) 0xFF5D)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_RGN ((kdu_uint16) 0xFF5E)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_POC ((kdu_uint16) 0xFF5F)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_CRG ((kdu_uint16) 0xFF63)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_DFS ((kdu_uint16) 0xFF72)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_ADS ((kdu_uint16) 0xFF73)
                // Parameter marker segment  -- processed in "params.cpp"
#define KDU_ATK ((kdu_uint16) 0xFF79)
                // Parameter marker segment  -- processed in "params.cpp"

#define KDU_COM ((kdu_uint16) 0xFF64)
                // Packet headers in advance -- processed in "compressed.cpp"
#define KDU_TLM ((kdu_uint16) 0xFF55)
                // Packet headers in advance -- processed in "compressed.cpp"

#define KDU_PLM ((kdu_uint16) 0xFF57)
                // Comment marker -- can safely ignore these
#define KDU_PLT ((kdu_uint16) 0xFF58)
                // Comment marker -- can safely ignore these
#define KDU_PPM ((kdu_uint16) 0xFF60)
                // Comment marker -- can safely ignore these
#define KDU_PPT ((kdu_uint16) 0xFF61)
                // Comment marker -- can safely ignore these


/* ========================================================================= */
/*                     Class and Structure Definitions                       */
/* ========================================================================= */

/*****************************************************************************/
/*                              kdu_nc_coords                                */
/*****************************************************************************/

struct kdu_nc_coords {
  /* [BIND: copy]
     [SYNOPSIS]
       Manages a single pair of integer-valued coordinates.  To manage
       a pair of coordinates, you should generally use `kdu_coords',
       rather than this object.  However, `kdu_coords' involves a
       constructor, so you may not be able to use it in all circumstances
       (e.g., when embedding a coordinate pair within a C++ union).  This
       object has no default constructor at all, so is easy to embed into
       other objects without requiring invocation of a constructor.  The
       `kdu_coords' object is also equipped with copy constructor and
       assignment operators, which can accept `kdu_nc_coords' as input, so
       you should be able to pass a `kdu_nc_coords' instance to any
       function which normally requires a `kdu_coords' object.
  */
  public: // Data
    int y; /* [SYNOPSIS] The vertical coordinate. */
    int x; /* [SYNOPSIS] The horizontal coordinate. */
  };

/*****************************************************************************/
/*                               kdu_coords                                  */
/*****************************************************************************/

struct kdu_coords {
  /* [BIND: copy]
     [SYNOPSIS]
       Manages a single pair of integer-valued coordinates.
  */
  public: // Data
    int y; /* [SYNOPSIS] The vertical coordinate. */
    int x; /* [SYNOPSIS] The horizontal coordinate. */
  public: // Convenience functions
    kdu_coords() { x = y = 0; }
    kdu_coords(int x, int y) {this->x=x; this->y=y; }
    kdu_coords(const kdu_nc_coords &src) { x=src.x; y=src.y; }
      /* [SYNOPSIS] This copy constructor allows you to pass `kdu_nc_coords'
         objects directly to functions which accept `kdu_coords' arguments. */
    void assign(const kdu_coords &src) { *this = src; }
      /* [SYNOPSIS] Copies the contents of `src' to the present object.
         This function is useful only when using a language binding
         which does not support data member access or direct copying
         of contents. */
    void assign(const kdu_nc_coords &src) { x=src.x; y=src.y; }
      /* [SYNOPSIS] This form of the `assign' operator allows you to initialize
         the object from the contents of a `kdu_nc_coords' object -- the latter
         may be required in certain contexts where you cannot embed an
         object which requires explicit construction. */
    kdu_coords operator=(const kdu_nc_coords &rhs)
      { x=rhs.x; y=rhs.y; return *this; }
      /* [SYNOPSIS] Allows you to directly assign a `kdu_nc_coords' object
         to a `kdu_coords' object -- the former may be required in certain
         contexts where you cannot embed an object which requires explicit
         construction. */
    int get_x() const { return x; }
    int get_y() const { return y; }
    void set_x(int x) { this->x = x; }
    void set_y(int y) { this->y = y; }
    void transpose() {int tmp=y; y=x; x=tmp; }
      /* [SYNOPSIS] Swaps the vertical and horizontal coordinates. */
    kdu_coords operator+(const kdu_coords &rhs) const
      { kdu_coords result; result.x=x+rhs.x; result.y=y+rhs.y; return result; }
    kdu_coords plus(const kdu_coords &rhs) const
      { /* [SYNOPSIS] Same as `operator+', but more suitable for
                      some language bindings. */
           return (*this)+rhs;
      }
    kdu_coords operator-(const kdu_coords &rhs) const
      { kdu_coords result; result.x=x-rhs.x; result.y=y-rhs.y; return result; }
    kdu_coords minus(const kdu_coords &rhs) const
      { /* [SYNOPSIS] Same as `operator-', but more suitable for
                      some language bindings. */
           return (*this)-rhs;
      }
    kdu_coords operator+=(const kdu_coords &rhs)
      { x+=rhs.x; y+=rhs.y; return *this; }
    kdu_coords add(const kdu_coords &rhs)
      { /* [SYNOPSIS] Same as `operator+=', but more suitable for some
                      language bindings. */
        x+=rhs.x; y+=rhs.y; return *this;
      }
    kdu_coords operator-=(const kdu_coords &rhs)
      { x-=rhs.x; y-=rhs.y; return *this; }
    kdu_coords subtract(const kdu_coords &rhs)
      { /* [SYNOPSIS] Same as `operator-=', but more suitable for some
                      language bindings. */
        x-=rhs.x; y-=rhs.y; return *this;
      }
    bool operator==(const kdu_coords &rhs) const
      { return (x==rhs.x) && (y==rhs.y); }
    bool equals(const kdu_coords &rhs) const
      { /* [SYNOPSIS] Same as `operator==', but more suitable for
                      some language bindings. */
           return (*this)==rhs;
      }
    bool operator!=(const kdu_coords &rhs) const
      { return (x!=rhs.x) || (y!=rhs.y); }
    void from_apparent(bool transp, bool vflip, bool hflip)
      { /* [SYNOPSIS]
             Converts a point from the apparent coordinate system established
             by `kdu_codestream::change_appearance' to the real coordinates.
             The `transp', `vflip' and `hflip' parameters are identical to
             those supplied to `kdu_codestream::change_appearance'.
        */
        x=(hflip)?(-x):x;
        y=(vflip)?(-y):y;
        if (transp) transpose();
      }
    void to_apparent(bool transp, bool vflip, bool hflip)
      { /* [SYNOPSIS]
             Does the reverse of `from_apparent', assuming the same values
             for `transp', `vflip' and `hflip' are supplied.
        */
        if (transp) transpose();
        x = (hflip)?(-x):x;
        y = (vflip)?(-y):y;
      }
  };

/*****************************************************************************/
/*                                kdu_dims                                   */
/*****************************************************************************/

struct kdu_dims {
  /* [BIND: copy]
     [SYNOPSIS]
       Generic structure for holding location and size information for various
       partitions on the canvas.  The `size' coordinates identify the
       dimensions of the specific tile, tile-component, precinct, code-block,
       etc., while the `pos' coordinates identify the absolute location of
       its upper left hand corner.
       [//]
       When used to describe partitions, the dimensions of the partition
       element are maintained by `size', while `pos' holds the anchor
       point for the partition.  The anchor point is the absolute coordinates
       of the upper left hand corner of a reference partition element. */
  public: // Data
    kdu_coords pos;
      /* [SYNOPSIS] Upper left hand corner. */
    kdu_coords size;
      /* [SYNOPSIS] Dimensions of rectangle or partition element. */
  public: // Convenience functions
    kdu_dims() {};
    void assign(const kdu_dims &src) { *this = src; }
      /* [SYNOPSIS] Copies the contents of `src' to the present object.
         This function is useful only when using a language binding
         which does not support data member access or direct copying
         of contents. */
    kdu_coords *access_pos() { return &pos; }
      /* [SYNOPSIS] Returns a pointer (reference) to the public
         `pos' member.  This is useful when working with a language
         binding which does not support data member access.  When
         used with the Java language binding, for example, interacting
         with the returned object, is equivalent to interacting with
         the `pos' member of the present object directly. */
    kdu_coords *access_size() { return &size; }
      /* [SYNOPSIS] Returns a pointer (reference) to the public
         `size' member.  This is useful when working with a language
         binding which does not support data member access.  When
         used with the Java language binding, for example, interacting
         with the returned object, is equivalent to interacting with
         the `size' member of the present object directly. */
    kdu_long area() const
      { return ((kdu_long) size.x) * ((kdu_long) size.y); }
      /* [SYNOPSIS]
         Returns the product of the horizontal and vertical dimensions. */
    void transpose()
      { size.transpose(); pos.transpose(); }
      /* [SYNOPSIS]
         Swap the roles played by horizontal and vertical coordinates. */
    kdu_dims operator&(kdu_dims &rhs) const // Intersects region with RHS.
      { kdu_dims result = *this; result &= rhs; return result; }
      /* [SYNOPSIS]
           Returns the intersection of the region represented by `rhs' with
           that represented by the current object.
      */
    kdu_dims intersection(kdu_dims &rhs) const
      { /* [SYNOPSIS] Same as `operator&', but more appropriate for
                      some language bindings. */
        return (*this) & rhs;
      }
    kdu_dims operator&=(kdu_dims &rhs) // Returns intersection of operands
      {
      /* [SYNOPSIS]
           Intersects the region represented by `rhs' with that  represented
           by the current object.
      */
        kdu_coords lim = pos+size;
        kdu_coords rhs_lim = rhs.pos + rhs.size;
        if (lim.x > rhs_lim.x) lim.x = rhs_lim.x;
        if (lim.y > rhs_lim.y) lim.y  = rhs_lim.y;
        if (pos.x < rhs.pos.x) pos.x = rhs.pos.x;
        if (pos.y < rhs.pos.y) pos.y = rhs.pos.y;
        size = lim-pos;
        if (size.x < 0) size.x = 0;
        if (size.y < 0) size.y = 0;
        return *this;
      }
    bool intersects(kdu_dims &rhs)
      {
      /* [SYNOPSIS]
           Checks whether or not the region represented by `rhs' has
           a non-empty intersection with that represented by the current
           object.
         [RETURNS]
           True if the intersection is non-empty.
      */
        if ((pos.x+size.x) <= rhs.pos.x) return false;
        if ((pos.y+size.y) <= rhs.pos.y) return false;
        if (pos.x >= (rhs.pos.x+rhs.size.x)) return false;
        if (pos.y >= (rhs.pos.y+rhs.size.y)) return false;
        if ((size.x <= 0) || (size.y <= 0) ||
            (rhs.size.x <= 0) || (rhs.size.y <= 0))
          return false;
        return true;
      }
    bool operator!()
      { return ((size.x>0)&&(size.y>0))?false:true; }
      /* [SYNOPSIS]
           Same a `is_empty'.
      */
    bool is_empty() const
      { return ((size.x>0)&&(size.y>0))?false:true; }
      /* [SYNOPSIS]
           Checks for an empty region.
         [RETURNS]
           True if either of the dimensions is less than or equal to 0.
      */
    bool operator==(const kdu_dims &rhs) const
      { return (pos==rhs.pos) && (size==rhs.size); }
      /* [SYNOPSIS]
           Returns true if the dimensions and coordinates are identical in
           the `rhs' and current objects.
      */
    bool equals(const kdu_dims &rhs) const
      { /* [SYNOPSIS] Same as `operator==', but more appropriate for some
                      language bindings. */
        return (*this)==rhs;
      }
    bool operator!=(const kdu_dims &rhs) const
      { return (pos!=rhs.pos) || (size!=rhs.size); }
      /* [SYNOPSIS]
           Returns false if the dimensions and coordinates are identical in
           the `rhs' and current objects.
      */
    bool contains(const kdu_coords &rhs)
      { kdu_coords diff = rhs - pos;
        return ((diff.x >= 0) && (diff.y >= 0) &&
                (diff.x < size.x) && (diff.y < size.y)); }
      /* [SYNOPSIS]
           Returns true if the current region contains the point `rhs'.
      */
    bool contains(const kdu_dims &rhs)
      { return ((rhs & *this) == rhs); }
      /* [SYNOPSIS]
           Returns true if the current region completely contains the `rhs'
           region -- if `rhs' is equal to the current region, it is still
           considered to be contained.
      */
    void augment(const kdu_coords &p)
      { /* [SYNOPSIS]
             Enlarges the region as required to ensure that it includes the
             location given by `p'.  If the region is initially empty, it
             will be set to have size 1x1 and location `p'.
        */
        if (is_empty()) { pos = p; size.x=size.y=1; return; }
        int delta;
        if ((delta=pos.x-p.x) > 0)
          { size.x+=delta; pos.x-=delta; }
        else if ((delta=p.x+1-pos.x-size.x) > 0)
          size.x += delta;
        if ((delta=pos.y-p.y) > 0)
          { size.y+=delta; pos.y-=delta; }
        else if ((delta=p.y+1-pos.y-size.y) > 0)
          size.y += delta;        
      }
    void augment(const kdu_dims &src)
      { /* [SYNOPSIS]
             Enlarges the region as required to encompass the `src' region.
        */
        if (!src.is_empty())
          { augment(src.pos); augment(src.pos+src.size-kdu_coords(1,1)); }
      }
    bool clip_point(kdu_coords &pt) const
      { /* [SYNOPSIS]
             Forces `pt' inside the boundary described by this object, if it
             is not already in there, returning true if `pt' was changed.
        */
        bool changed = false;
        if (pt.x < pos.x) { pt.x = pos.x; changed=true; }
        else if (pt.x >= (pos.x+size.x)) { pt.x=pos.x+size.x-1; changed=true; }
        if (pt.y < pos.y) { pt.y = pos.y; changed = true; }
        else if (pt.y >= (pos.y+size.y)) { pt.y=pos.y+size.y-1; changed=true; }
        return changed;
      }
  void from_apparent(bool transp, bool vflip, bool hflip)
      { /* [SYNOPSIS]
             Converts the region from the apparent coordinate system
             established by `kdu_codestream::change_appearance' to the real
             coordinates.  The `transp', `vflip' and `hflip' parameters
             are identical to those supplied to
             `kdu_codestream::change_appearance'.
        */
        if (hflip) pos.x = -(pos.x+size.x-1);
        if (vflip) pos.y = -(pos.y+size.y-1);
        if (transp) transpose();
      }
    void to_apparent(bool transp, bool vflip, bool hflip)
    { /* [SYNOPSIS]
           Does the reverse of `kdu_from_apparent', assuming the same
           values for `transp', `vflip' and `hflip' are supplied.
      */
      if (transp) transpose();
      if (hflip) pos.x = -(pos.x+size.x-1);
      if (vflip) pos.y = -(pos.y+size.y-1);
    }
  };

/*****************************************************************************/
/* ENUM                     kdu_component_access_mode                        */
/*****************************************************************************/

enum kdu_component_access_mode {
  // See the `kdu_codestream::apply_input_restrictions' function to understand
  // the interpretation of component access modes.
    KDU_WANT_OUTPUT_COMPONENTS=0,
    KDU_WANT_CODESTREAM_COMPONENTS=1
  };

/*****************************************************************************/
/*                            kdu_codestream_comment                         */
/*****************************************************************************/

class kdu_codestream_comment {
  /* [BIND: interface]
     [SYNOPSIS]
     Objects of this class are only interfaces, having the size of a single
     pointer (memory address).  Copying the object has no effect on the
     underlying state information, but simply serves to provide another
     interface (or reference) to it.
     [//]
     To create a code-stream comment, use `kdu_codestream::add_comment' and
     then modify the contents of the comment using the returned
     `kdu_codestream_comment' interface.  Note that comments should all be
     created and filled in prior to any call to `kdu_codestream::flush'.
     After that call, all comments will be marked as "read only", although
     the interfaces returned by `kdu_codestream::add_comment' or
     `kdu_codestream::get_comment' remain valid until the relevant
     `kdu_codestream' object has been destroyed (see
     `kdu_codestream::destroy').
  */
  public: // Member functions
    kdu_codestream_comment() { state = NULL; }
      /* [SYNOPSIS]
           Creates an empty interface.  To get a valid comment interface,
           use one of the functions `kdu_codestream::get_comment' or
           `kdu_codestream::add_comment'.
      */
    kdu_codestream_comment(kd_codestream_comment *state) { this->state=state; }
      /* [SYNOPSIS]
         You should never need to call this function yourself.  It is used
         by the internal codestream management machinery to provide the
         interfaces returned by `add_comment' or `get_comment'.
      */
    bool exists() { return (state != NULL); }
      /* [SYNOPSIS]
           Returns true if the interface is non-empty.
      */
    bool operator!() { return (state == NULL); }
      /* [SYNOPSIS]
           Returns true if the interface is empty.
      */
    KDU_EXPORT const char *get_text();
      /* [SYNOPSIS]
           Returns any text string stored in the object.  The returned pointer
           will be NULL only if the interface has not yet been initialized.
           Otherwise, a null-terminated string is always returned, even if it
           has no characters. In particular, if the object holds binary data,
           as opposed to text, this function will return an empty string, but
           `get_data' will return the binary data.
           [//]
           If the object is not read-only (see `check_readonly'), the returned
           string buffer might cease to be valid if more text is added.
      */
    KDU_EXPORT int get_data(kdu_byte buf[], int offset, int length);
      /* [SYNOPSIS]
           You can use this function to read a range of bytes from the object,
           regardless of whether it represents text or binary data.  If the
           object represents text, the range of bytes which is available for
           reading includes the null terminator of the string returned by
           `get_text'.  If the object represents binary data, `get_text' will
           retrn an empty string (not NULL).
         [RETURNS]
           The number of bytes actually written to `buf' (or which would be
           written to `buf' if it were non-NULL).  May be zero if
           `offset' is greater than or equal to the number of bytes available
           for writing.
         [ARG: buf]
           Must be large enough to hold `length' bytes, or else NULL.  If NULL,
           the function is being used only to determine the size of a buffer
           to allocate.
         [ARG: offset]
           Initial position, within the range of bytes represented by the
           comment, from which to start copying data to `buf'.
         [ARG: length]
           Number of bytes to copy from the internal buffer to `buf'.
      */
    KDU_EXPORT bool check_readonly();
      /* [SYNOPSIS]
           Returns true if any future call to `put_data' or `put_text' will
           fail.  This happens if the comment has been retrieved from or
           written as a code-stream comment marker segment already.
      */
    KDU_EXPORT bool put_data(const kdu_byte data[], int num_bytes);
      /* [SYNOPSIS]
           Returns false if the comment contents have been frozen, as a result
           of writing it to or converting it from a code-stream comment marker
           segment.  Also returns false if the `put_text' function has already
           been used to store text in the object, as opposed to binary data.
           Otherwise, appends the `num_bytes' of data supplied in the `data'
           array to any data previously supplied, and returns true.
      */
    KDU_EXPORT bool put_text(const char *string);
      /* [SYNOPSIS]
           Returns false if the comment contents have been frozen, as a result
           of writing it to or converting it from a code-stream comment marker
           segment.  Also returns false if the `put_data' function has already
           been used to store binary data in the object, as opposed to text.
           Otherwise, appends the text in `string' to any existing text and
           returns true.  The `string' should be NULL-terminated ASCII
           although nothing explicitly prevents you from embedding
           arbitrary UTF-8 strings in codestream comments.
      */
    kdu_codestream_comment &operator<<(const char *string)
      { put_text(string); return *this; }
      /* [SYNOPSIS]
           Uses `put_text' to write the string, returning a reference to the
           object itself.  If `put_text' fails, no action is taken, so you
           may wish to use `check_readonly' first to check for this condition
           if there is a possibility that `put_text' would fail.
      */
    kdu_codestream_comment &operator<<(char ch)
      { char text[2]; text[0]=ch; text[1]='\0'; put_text(text); return *this; }
      /* [SYNOPSIS]
           Same as the first form of this overloaded operator, but appends
           a single character to the comment text.
      */
    kdu_codestream_comment &operator<<(int val)
      { char text[80]; sprintf(text,"%d",val); put_text(text); return *this; }
      /* [SYNOPSIS]
           Same as the first form of this overloaded operator, but appends
           a decimal string representing the integer `val' to the comment
           text.
      */
    kdu_codestream_comment &operator<<(unsigned int val)
      { char text[80]; sprintf(text,"%u",val); put_text(text); return *this; }
      /* [SYNOPSIS]
           Same as the first form of this overloaded operator, but appends
           a decimal string representing the unsigned integer `val' to the
           comment text.
      */
    kdu_codestream_comment &operator<<(kdu_long val)
      {
#ifdef KDU_LONG64
        if (val >= 0)
          return (*this)<<((unsigned)(val>>32))<<((unsigned) val);
        else
          return (*this)<<'-'<<((unsigned)((-val)>>32))<<((unsigned) -val);
#else
        return (*this)<<(int) val;
#endif
      }
      /* [SYNOPSIS]
           Same as the first form of this overloaded operator, but appends
           a decimal string representing `val' to the comment text.
      */
    kdu_codestream_comment &operator<<(short int val)
      { return (*this)<<(int) val; }
      /* [SYNOPSIS]
           Same as the first form of this overloaded operator, but appends
           a decimal string representing `val' to the comment text.
      */
    kdu_codestream_comment &operator<<(unsigned short int val)
      { return (*this)<<(unsigned int) val; }
      /* [SYNOPSIS]
           Same as the first form of this overloaded operator, but appends
           a decimal string representing `val' to the comment text.
      */
    kdu_codestream_comment &operator<<(float val)
      { char text[80]; sprintf(text,"%f",val); put_text(text); return *this; }
      /* [SYNOPSIS]
           Same as the first form of this overloaded operator, but appends
           a decimal string representing `val' to the comment text.
      */
    kdu_codestream_comment &operator<<(double val)
      { char text[80]; sprintf(text,"%f",val); put_text(text); return *this; }
      /* [SYNOPSIS]
           Same as the first form of this overloaded operator, but appends
           a decimal string representing `val' to the comment text.
      */
  private: // Data
    friend class kdu_codestream;
    kd_codestream_comment *state;
  };

/*****************************************************************************/
/*                             kdu_compressed_source                         */
/*****************************************************************************/

#define KDU_SOURCE_CAP_SEQUENTIAL   ((int) 0x0001)
#define KDU_SOURCE_CAP_SEEKABLE     ((int) 0x0002)
#define KDU_SOURCE_CAP_CACHED       ((int) 0x0004)
#define KDU_SOURCE_CAP_IN_MEMORY    ((int) 0x0008)

class kdu_compressed_source {
  /* [BIND: reference]
     [SYNOPSIS]
     Abstract base class must be derived to create a real compressed data
     source with which to construct an input `kdu_codestream' object.
     [//]
     Supports everything from simple file reading sources to caching sources
     for use in sophisticated client-server applications.
     [//]
     For Java, C# or other foreign languages, the "kdu_hyperdoc" utility does
     not create language bindings which allow you to directly inherit
     this class.  To implement the `read' and other member functions in
     a foreign language, you should derive from
     `kdu_compressed_source_nonnative' instead.
  */
  public: // Member functions
    virtual ~kdu_compressed_source() { return; }
      /* [SYNOPSIS]
         Allows destruction of source objects from the abstract base.
      */
    virtual bool close() { return true; }
      /* [SYNOPSIS]
           This particular function does nothing, but its presence as a virtual
           function ensures that a more derived object's `close' function can
           be invoked from the abstract base object.  Closure might not have
           meaning for some types of sources, in which case there is no need
           to override this function in the relevant derived class.
         [RETURNS]
           The return value has no meaning to Kakadu's code-stream management
           machinery, but derived objects may choose to return false if not
           all of the available data was consumed.  See `jp2_input_box::close'
           for an example of this.
      */
    virtual int get_capabilities() { return KDU_SOURCE_CAP_SEQUENTIAL; }
      /* [SYNOPSIS]
           Returns the logical OR of one or more capability flags, whose
           values are identified by the macros, `KDU_SOURCE_CAP_SEQUENTIAL',
           `KDU_SOURCE_CAP_SEEKABLE' `KDU_SOURCE_CAP_IN_MEMORY' and
           `KDU_SOURCE_CAP_CACHED'.  These flags have the following
           interpretation:
           [>>] `KDU_SOURCE_CAP_SEQUENTIAL': If this flag is set, the source
                supports sequential reading of data in the order expected
                of a valid JPEG2000 code-stream.  If this flag is not set,
                the KDU_SOURCE_CAP_CACHED flag must be set.
           [>>] `KDU_SOURCE_CAP_SEEKABLE': If this flag is set, the source
                supports random access using the `seek' function.  Most file
                or memory based sources should support seeking, although
                disabling the seeking capability can have useful effects on
                the sequencing of code-stream parsing operations. In
                particular, if the source advertises seekability, Kakadu
                will attempt to use optional pointer marker segments to
                minimize code-stream buffering.  In some cases, however,
                this may result in undesirable disk access patterns.  Error
                resilience is also weakened or destroyed when seeking is
                employed.
           [>>] `KDU_SOURCE_CAP_IN_MEMORY': If this flag is set, the source
                is stored in a contiguous block of memory.  You can use the
                `seek' and `read' functions to read from arbitrary locations
                in this memory block, but you can also do this if the source
                exists in a seekable file.  The additional advantage of
                "in memory" sources is that the memory block can be accessed
                directly using the `access_memory' function.  This generally
                increases the efficiency of high performance applications.
                If this flag is set, `KDU_SOURCE_CAP_SEQUENTIAL' and
                `KDU_SOURCE_CAP_SEEKABLE' must also be set.
           [>>] `KDU_SOURCE_CAP_CACHED': If this flag is set, the source is a
                client cache, which does not guarantee to support regular
                sequential reading of the code-stream beyond the end of
                the main header.  To access individual tile headers, the
                `set_tileheader_scope' function must be invoked.  To
                access packet data for individual precincts, the
                `set_precinct_scope' function must be used.
           [//]
           Cached sources might conceivably support sequential reading as
           well, meaning that it is possible to read the code-stream
           linearly as if it were organized in a file.  However, in
           most cases only one of the KDU_SOURCE_CAP_SEQUENTIAL and
           KDU_SOURCE_CAP_CACHED flags will be set.
           [//]
           Both cached and sequential sources may support seekability.
           If a cached source is seekable, seeking is performed with
           respect to the current scope.  Specifically, this is the scope
           of the precinct associated with the most recent call to
           `set_precinct_scope' or the tile header associated with the
           most recent `set_tile_header_scope', whichever call was
           invoked most recently, or the scope of the main code-stream header,
           if neither `set_precinct_scope' nor `set_tile_header_scope' has
           yet been called.
      */
    virtual int read(kdu_byte *buf, int num_bytes) = 0;
      /* [SYNOPSIS]
           This function must be implemented in every derived class.  It
           performs a sequential read operation, within the current read scope,
           transferring the next `num_bytes' bytes from the scope into the
           supplied buffer.  The function must return the actual number of
           bytes recovered, which will be less than `num_bytes' only if the
           read scope is exhausted.
           [//]
           The default read scope is the entire code-stream, although the
           source may support tile header and precinct scopes (see
           `set_tileheader_scope' and `set_precinct_scope' functions below).
           [//]
           The system provides its own internal temporary buffering of
           compressed data to maximize internal access efficiency.  For this
           reason, the virtual `read' function defined here will usually be
           called to transfer quite a few bytes at a time.
         [RETURNS]
           Actual number of bytes written into the buffer.
         [ARG: buf]
           Pointer to a buffer large enough to accept the requested bytes.
         [ARG: num_bytes]
           Number of bytes to be read, unless the read scope is exhausted
           first.
      */
    virtual bool seek(kdu_long offset) { return false; }
      /* [SYNOPSIS]
           This function should be implemented by sources which support
           sequential reading of the code-stream from any position (i.e.,
           KDU_SOURCE_CAP_SEEKABLE).  Subsequent reads transfer data starting
           from a location `offset' bytes beyond the seek origin.
           [//]
           For non-caching sources, the seek origin is the start of the
           code-stream, regardless of whether or not the code-stream appears
           at the start of its containing file.
           [//]
           For sources supporting `KDU_SOURCE_CAP_CACHED', the seek origin
           is the start of the code-stream only until such point as the
           first call to `set_precinct_scope' or `set_tile_header_scope'
           is called.  After that, the seek origin is the first byte of
           the relevant precinct or tile-header (excluding the SOT
           marker segment).
         [RETURNS]
           If seeking is not supported, the function should return false.
           If seeking is supported, the function should return true, even if
           `offset' identifies a location outside the relevant scope.
      */
    virtual kdu_long get_pos() { return -1; }
      /* [SYNOPSIS]
           Returns the location of the next byte to be read, relative to
           the current seek origin.
           [//]
           For non-caching sources, the seek origin is the start of the
           code-stream, regardless of whether or not the code-stream appears
           at the start of its containing file.
           [//]
           For sources supporting `KDU_SOURCE_CAP_CACHED', the seek origin
           is the start of the code-stream only until such point as the
           first call to `set_precinct_scope' or `set_tile_header_scope'
           is called.  After that, the seek origin is the first byte of
           the relevant precinct or tile-header (excluding the SOT marker
           segment).
         [RETURNS]
           Non-seekable sources may return a negative value (actually any
           value should be OK); seekable sources, however, must
           implement this function correctly.
      */
    virtual kdu_byte *access_memory(kdu_long &pos, kdu_byte * &lim)
      { return NULL; }
      /* [SYNOPSIS]
           This function should return NULL except for in-memory sources
           (i.e., sources which advertise the `KDU_SOURCE_CAP_IN_MEMORY'
           capability).  For in-memory sources, the function should return
           a pointer to the next location in memory that would be read by
           a call to `read'.  The variable referenced by the `pos' argument
           should be set to the absolute location of the returned memory
           address, expressed relative to the start of the source memory
           block.  The variable referenced by the `lim' argument should
           be set to the address immediately beyond the source memory block.
           This means that you can legally access locations `ptr[idx]',
           where `ptr' is the address returned by the function and
           `idx' lies in the range -`pos' <= `idx' < `lim'-`ptr'.
      */
    virtual bool set_tileheader_scope(int tnum, int num_tiles)
      { return false; }
      /* [SYNOPSIS]
           This function should be implemented by sources which support
           non-sequential code-stream organizations, (i.e., sources which
           advertise the `KDU_SOURCE_CAP_CACHED' capability).  The principle
           example which we have in mind is a compressed data cache in an
           interactive client-server browsing application.  Subsequent
           reads will transfer the contents of the tile header, starting
           from immediately beyond the first SOT marker segment and continuing
           up to (but not including) the SOD marker segment.
           [//]
           Cached sources have no tile-parts.  They effectively concatenate
           all tile-parts for the tile and the information from all header
           marker segments from all tile-parts of the tile.  Tile-parts in
           JPEG2000 are designed to allow such treatment.  Cached sources
           also have no packet sequence.  For this reason, the sequence order
           in COD marker segments and any sequencing schedules advertised by
           POC marker segments are to be entirely disregarded.  Instead,
           precinct packets are recovered directly from the cache with the
           aid of the `set_precinct_scope' function.
           [//]
           If the requested tile header has not yet been loaded into the cache
           the present function should return false.  Many tile headers may
           well have zero length, meaning that no coding parameters in the
           main header are overridden by this tile.  However, an empty
           tile header is not the same as an as-yet unknown tile header.
           The false return value informs the code-stream machinery that
           it should not yet commit to any particular structure for the
           tile.  It will attempt to reload the tile header each time
           `kdu_codestream::open_tile' is called, until the present
           function returns true.
         [ARG: tnum]
           0 for the first tile in the codestream.
         [ARG: num_tiles]
           Should hold the actual total number of tiles in the code-stream.
           It provides some consistency checking information to the cached
           data source.
         [RETURNS]
           False if the `KDU_SOURCE_CAP_CACHED' capability is not supported or
           if the tile header is not yet available in the cache.
      */
    virtual bool set_precinct_scope(kdu_long unique_id) { return false; }
      /* [SYNOPSIS]
           This function must be implemented by sources which advertise
           support for the `KDU_SOURCE_CAP_CACHED' capability.  It behaves
           somewhat like `seek' in repositioning the read pointer to an
           arbitrarily selected location, except that the location is
           specified by a unique identifier associated with the
           precinct, rather than an offset from the code-stream's SOC marker.
           Subsequent reads recover data from the packets of the identified
           precinct, in sequence, until no further data for the precinct
           exists.
           [//]
           Valid precinct identifiers are composed from three
           quantities: the zero-based tile index, T, identifying the tile
           to which the precinct belongs; the zero-based component index, C,
           indicating the image component to which the precinct belongs;
           and a zero-based sequence number, S, uniquely identifying the
           precinct within its tile-component.  The identifier is then given by
             [>>] unique_id = (S*num_components+C)*num_tiles+T
           [//]
           where `num_tiles' is the number of tiles in the compressed image,
           as passed in any call to the `set_tileheader_scope' member
           function.
           [//]
           The sequence number, S, is based upon a fixed sequence, not
           affected by packet sequencing information in COD or POC marker
           segments.  All precincts in resolution level 0 (the lowest
           resolution) appear first in this fixed sequence, starting from 0,
           followed immediately by all precincts in resolution level 1,
           and so forth.  Within each resolution level, precincts are
           numbered in scan-line order, from top to bottom and left to right.
           [//]
           Although this numbering system may seem somewhat complicated,
           it has a number of desirable attributes: each precinct has a single
           unique address; lower resolution images or portions of images have
           relatively smaller identifiers; and there is no need for the
           compressed data source to know anything about the number of
           precincts in each tile-component-resolution.
         [ARG: unique_id]
           Unique identifier for the precinct which is to be accessed in
           future `read' calls.  See above for a definition of this
           identifier.  If the supplied value refers to a non-existent
           precinct, the source is at liberty to generate a terminal error,
           although it may simply treat such cases as precincts with no
           data, so that subsequent `read' calls would return 0.
         [RETURNS]
           Must return true if and only if the KDU_SOURCE_CAP_CACHED
           capability is supported, even if there is no data currently
           cached for the relevant precinct.
      */
  };

/*****************************************************************************/
/*                      kdu_compressed_source_nonnative                      */
/*****************************************************************************/

class kdu_compressed_source_nonnative : public kdu_compressed_source {
  /* [BIND: reference]
     [SYNOPSIS]
     This alternate base class is provided explicitly for binding to
     non-native languages.  The `kdu_compressed_source' class cannot be
     directly inherited by Java, C# or other non-native languages because
     it contains a pure virtual function `read', which cannot be safely
     implemented across language boundaries.  The reason for this is that
     the `read' function accepts an unsafe memory buffer (i.e., an array
     without explicit dimensions) into which the data is supposed to be
     written by the implementing class.
     [//]
     The present class gets around that problem by providing an implementation
     for `read' which invokes a `post_read' member function that is
     inheritable.  The `post_read' function's implementation in the derived
     class should invoke `push_data' to pass the requested data down into
     the native code via an explicitly dimensioned array.  Consult the
     relevant function descriptions for more on this.
     [//]
     Do not forget that your derived class must override the
     `get_capabilities' function, which returns an otherwise illegal value
     of 0 unless overridden.
  */
  public: // Member functions
    kdu_compressed_source_nonnative()
      { pending_read_buf=NULL; pending_read_bytes=0; }
    virtual int get_capabilities() { return 0; }
      /* [BIND: callback]
         [SYNOPSIS]
           Implements `kdu_compressed_source::get_capabilities' again so as
           to provide a callback binding for implementation in a foreign
           language derived class.  The implementing class must provide its
           own overriding definition for this function, which returns one of:
           `KDU_SOURCE_CAP_SEQUENTIAL', `KDU_SOURCE_CAP_SEEKABLE',
           `KDU_SOURCE_CAP_CACHED' or `KDU_SOURCE_CAP_IN_MEMORY'.  See the
           description of `kdu_compressed_source::get_capabilities' for
           an explanation of these constants.
      */
    virtual bool seek(kdu_long offset) { return false; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_source::seek' function so that it
           can be redeclared as a callback function for foreign language
           classes to provide a meaningful implementation.  Returns false
           if you do not implement it in your derived class.
      */
    virtual kdu_long get_pos() { return 0; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_source::get_pos' function so that it
           can be redeclared as a callback function for foreign language
           classes to provide a meaningful implementation.  Returns 0
           if you do not implement it in your derived class.
      */
    virtual bool set_tileheader_scope(int tnum, int num_tiles) { return false;}
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_source::set_tileheader_scope'
           function so that it can be redeclared as a callback function
           for foreign language classes to provide a meaningful
           implementation.  Returns false if you do not implement it in
           your derived class.
      */
    virtual bool set_precinct_scope(kdu_long unique_id) { return false; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_source::set_precinct_scope' function
           so that it can be redeclared as a callback function for foreign
           language classes to provide a meaningful implementation.  Returns
           false if you do not implement it in your derived class.
      */
    virtual int post_read(int num_bytes) { return 0; }
      /* [BIND: callback]
         [SYNOPSIS]
           This function is called for you, in response to an underlying call
           to the `kdu_compressed_source::read' function.  You must implement
           the present function in a derived class, using `push_data'
           to supply the data that was requested by the
           `kdu_compressed_source::read' function.
           [//]
           Note that the implementation of this object is not thread safe
           in and of itself, meaning that asynchronous calls to the `read'
           function from multiple threads could produce race conditions
           as they call through the `post_read' function.  However, this
           should not present a problem in practice, since you should
           associate a separate `kdu_compressed_source'-derived object with
           each distinct `kdu_codestream' that consumes compressed data and
           Kakadu's multi-threaded environment ensures that threads which
           share a single `kdu_codestream' via the `kdu_thread_env'
           mechanism will not simultaneously access its compressed data
           source.
         [RETURNS]
           Your derived class' implementation of this function should return
           the total number of bytes actually written to the read buffer via
           its call (or calls) to `push_data'.  If you do not implement the
           `post_read' funtion in your derived class, a value of 0 will
           always be returned by this funtion, meaning that the compressed
           source has no data.  You may return a value which is less than
           `num_bytes' only if the read scope is exhausted.
         [ARG: num_bytes]
           The number of bytes your derived class is being asked to supply
           via one or more calls to `push_data'.
      */
    void push_data(kdu_byte data[], int first_byte_pos, int num_bytes)
      {
        if (num_bytes > pending_read_bytes) num_bytes = pending_read_bytes;
        pending_read_bytes -= num_bytes;
        for (data+=first_byte_pos; num_bytes > 0; num_bytes--)
          *(pending_read_buf++) = *(data++);
      }
      /* [SYNOPSIS]
           You must call this function only from your derived class'
           implementation of the `post_read' function, in order to deliver
           the data requested of the internal `kdu_compressed_source::read'
           function.
           [//]
           In order to complete a request delivered via `post_read' your
           derived class may, if desired, supply the data via multiple calls
           to the present function.  Each call writes some number of bytes
           to the buffer originally passed into the internal call to
           `kdu_compressed_source::read'.  The `post_read' function should
           then return the total number of bytes supplied in all such
           `push_data' calls.
           [//]
           The data supplied by this function is taken from the location
           in the `data' array identified by `first_byte_pos', running
           through to the location `first_byte_pos'+`num_bytes'-1.
         [ARG: data]
           Array containing data to be returned ultimately in response to
           the underlying `kdu_compressed_source::read' call.
         [ARG: first_byte_pos]
           Location of the first byte of the `data' array which is to
           be appended to the internal buffer supplied in the underlying
           `kdu_compressed_source::read' call.  This allows you to supply
           much larger memory buffers in your calls to `push_data'.  However,
           depending on the language, passing large arrays across language
           interfaces might involve costly copying operations, especially
           where the foreign language array uses a fragmented internal
           representation.
         [ARG: num_bytes]
           Number of bytes from the `data' array which are to be appended to
           the internal buffer supplied in the underlying
           `kdu_compressed_source::read' call.
      */
  private:
    virtual int read(kdu_byte *buf, int num_bytes)
      {
        pending_read_buf = buf;
        pending_read_bytes = num_bytes;
        return post_read(num_bytes);
      }
  private: // Data
    kdu_byte *pending_read_buf;
    int pending_read_bytes;
  };

/*****************************************************************************/
/*                             kdu_compressed_target                         */
/*****************************************************************************/

#define KDU_TARGET_CAP_SEQUENTIAL   ((int) 0x0100)
#define KDU_TARGET_CAP_CACHED       ((int) 0x0400)
  
class kdu_compressed_target {
  /* [BIND: reference]
     [SYNOPSIS]
     Abstract base class, which must be derived to create a real compressed
     data target, to be passed to the `kdu_codestream::create' function.
     [//]
     From KDU7.2, derived classes may support either sequential writing
     of an entire compressed codestream (conventional compressed data target)
     or structured writing to a cache, or database of codestream elements.
     In the latter case, the codestream is decomposed into elements, which
     are synonymous with JPIP data-bins.  Specifically, the codestream main
     header forms one element, each tile header forms another element,
     and each JPEG2000 precinct has its own element.  These elements can
     be written in any order.
     [//]
     Compressed target objects that offer structured writing of codestream
     elements are known herein as "structured cache" targets.  They should
     provide implementations of the `start_mainheader', `end_mainheader',
     `start_tileheader', `end_tileheader', `start_precinct' and `end_precinct'
     functions, along with a `get_capabilities' function that returns
     `KDU_TARGET_CAP_CACHED'.
  */
  public: // Member functions
    virtual ~kdu_compressed_target() { return; }
      /* [SYNOPSIS]
         Allows destruction of compressed target objects from the
         abstract base.
      */
    virtual bool close() { return true; }
      /* [SYNOPSIS]
           This particular function does nothing, but its presence as a virtual
           function ensures that a more derived object's `close' function can
           be invoked from the abstract base object.  Closure might not have
           meaning for some types of targets, in which case there is no need
           to override this function in the relevant derived class.
         [RETURNS]
           The return value has no meaning to Kakadu's code-stream management
           machinery, but derived objects may use the return value to signal
           whether or not an output device was able to consume all of the
           written data.  It may happen that `write' returns true because
           the data had to be buffered, but that upon calling `close' it was
           found that not all of the buffered data could actually be
           transferred to the ultimate output device.
      */
    virtual int get_capabilities() { return KDU_TARGET_CAP_SEQUENTIAL; }
      /* [SYNOPSIS]
           Returns the logical OR of one of the following capability flags:
           [>>] `KDU_TARGET_CAP_SEQUENTIAL': If this flag is set, the
                target supports sequential writing of the complete codestream.
                Most compressed data targets would be of this form.
           [>>] `KDU_TARGET_CAP_CACHED': If this flag is set, the target
                provides for structured writing of the codestream contents,
                where elements of the structure may be written in any desired
                order.  The elements of the structure are the main codestream
                header, tile headers and precincts.  These are bracketed by
                calls to `start_mainheader' and `end_mainheader',
                `start_tileheader' and `end_tileheader', and
                `start_precinct' and `end_precinct'.  Any call to `write'
                that is not bracketed by those functions should be interpreted
                as a sequential write, which should generate an error if the
                target does not offer the `KDU_TARGET_CAP_SEQUENTIAL'
                capability.
           [//]
           The default implementation returns `KDU_TARGET_CAP_SEQUENTIAL'.
           [//]
           If the compressed data target advertises the `KDU_TARGET_CAP_CACHED'
           capability, the core Kakadu codestream generation machinery will
           employ the structured writing approach, so there is no value in
           also advertising `KDU_TARGET_CAP_SEQUENTIAL' and unbracketed calls
           to `write' should never occur.
           [//]
           The `KDU_TARGET_CAP_SEQUENTIAL' and `KDU_TARGET_CAP_CACHED' flags
           are deliberately chosen to be disjoint with the capability
           flags returned by `kdu_compressed_source::get_capabilities' so
           that a single derived class can actually serve as both a 
           compressed target and a compressed source simultaneously.
      */
    virtual void start_mainheader() { return; }
      /* [SYNOPSIS]
           This function must be implemented by compressed data targets that
           offer the `KDU_TARGET_CAP_CACHED' capability via `get_capabilities'.
           The elements associated with a structured cache target are
           the main codestream header, tile header elements (one per tile
           in the codestream) and precinct elements.  These elements may be
           written in any order -- i.e., it is not required to write the
           main codestream header first.  However, the present function may
           be called only once, followed by one or more calls to `write'
           and then `end_mainheader'.  The content written between calls to
           `start_mainheader' and `end_mainheader' must commence with the
           SOC marker and should not contain an EOC marker or any SOT
           marker segments.
      */
    virtual void end_mainheader() { return; }
      /* [SYNOPSIS]
           See `start_mainheader'.
      */
    virtual void start_tileheader(int tnum, int num_tiles) { return; }
      /* [SYNOPSIS]
           This function must be implemented by compressed data targets that
           offer the `KDU_TARGET_CAP_CACHED' capability via `get_capabilities'.
           The elements associated with a structured cache target are
           identical to those associated with a cached compressed data source
           (see `kdu_compressed_source').  Specifically, these elements are
           the main codestream header, tile header elements (one per tile
           in the codestream) and precinct elements.  Tile header elements
           may be written in any order, but only one call to `start_tileheader'
           is allowed for each tile in the codestream.  The same is true
           for precincts.
           [//]
           This function initiates the writing of a tile header element and
           must be followed by `end_tileheader'.  Whereas a sequential
           codestream's tiles may be organized into multiple "tile-parts",
           each with its own "tile-part header", there is exactly one tile
           header element per tile in a structured cached target.  This
           single tile header element contains all marker segments that
           would be found between SOT and SOD marker segments within any
           tile-part of the tile in a sequential codestream.  The tile header
           element does not contain the SOT marker segment or the SOD marker.
           [//]
           The packet progression sequence order advertised by COD marker
           segments and any sequencing schedules advertised by POC marker
           segments, play no role in the interpretation of JPEG2000 packet
           data that is written to a structured cached data target, because
           the packets belong to precinct elements that are explicitly
           delineated by calls to `start_precinct' and `end_precinct'.
           However, the progression order information may be used if the
           structured cache target later needs to export its contents as a
           sequential codestream.
         [ARG: tnum]
           0 for the first tile in the codestream.
         [ARG: num_tiles]
           Should hold the actual total number of tiles in the codestream.
           This argument provides some consistency checking information to
           the structured cache target.
      */
    virtual void end_tileheader(int tnum) { return; }
      /* [SYNOPSIS]
           Each call to `start_tileheader' must be bracketed by a call to
           `end_tileheader' that specifies the same `tnum' value, without
           any intervening calls to `start_tileheader' or `start_precinct'.
      */
    virtual void start_precinct(kdu_long unique_id) { return; }
      /* [SYNOPSIS]
           This function must be implemented by targets which advertise
           the `KDU_TARGET_CAP_CACHED' capability.  These targets offer
           structured writing of the codestream contents as a well-defined
           set of elements.  Precinct elements may be written in any order,
           but only one call to `start_precinct' is allowed for each precinct;
           the same is true for tile headers.
           [//]
           A precinct element holds the concatenation of all JPEG2000 packets
           that belong to the precinct (one per quality layer).  If SOP or
           EPH marker segments are used to delineate packet headers, they
           appear within the packet sequence that is written to the
           precinct element.
           [//]
           The unique precinct identifier, `unique_id', has exactly the
           same meaning as in `kdu_compressed_source::set_precinct_scope'.
           Specifically,
           [>>] unique_id = (S*num_components+C)*num_tiles+T
           [//]
           where T is the zero-based tile index (`tnum') of the precinct's
           tile, C is the zero-based index of the precinct's image component,
           and S is the sequence number defined in the comments appearing
           with `kdu_compressed_source::set_precinct_scope'.
           [//]
           This function might or might not check the validity of the
           `unique_id' argument.
      */
    virtual void end_precinct(kdu_long unique_id, int num_packets,
                              const kdu_long packet_lengths[]) { return; }
      /* [SYNOPSIS]
           Each call to `start_precinct' must be bracketed by a call to
           `end_precinct' that specifies the same `unique_id' value, without
           any intervening calls to `start_tileheader' or `start_precinct'.
         [ARG: num_packets]
           The `num_packets' argument should correspond to the number of
           JPEG2000 packets concatenated into the written precinct element.
           This value should be identical for all precincts that belong to
           the same tile, but this requirement might not be checked.
         [ARG: packet_lengths]
           This argument must supply an array with at least `num_packets'
           entries, corresponding to the number of bytes written for each
           of the JPEG2000 packets that have been written to the precinct
           element.  The function should usually check that the sum
           of these packet lengths is identical to the number of bytes
           written since the corresponding call to `start_precinct'.  Some
           structured cache target implementations might discard the packet
           length information, but it is particularly useful if a sequential
           codestream is later to be exported from the cache, or if the
           structured cache target is to be used to serve content via the
           JPIP protocol.
      */
    virtual bool start_rewrite(kdu_long backtrack) { return false; }
      /* [SYNOPSIS]
           This function is allowed only for sequential (non-cached)
           compressed data targets; if the `start_tileheader' or
           `start_precinct' functions have been called, this
           function should return false; however, the core codestream
           machinery should not be expected to invoke it in this case.
           [//]
           Together with `end_rewrite', this function provides access to
           repositioning capabilities which may be offered by a compressed
           target.  Repositioning is required if the code-stream generation
           machinery is to write TLM (tile-length main header) marker
           segments, for example, since these must be written in the main
           header, but their contents cannot generally be efficiently
           determined until after the tile data has been written; in fact,
           even the number of non-empty tile-parts cannot generally be known
           until the code-stream has been written, since we may need to
           create extra tile-parts on demand for incremental flushing.
           [//]
           Regardless of how this function may be used by the code-stream
           generation machinery, or by any other Kakadu objects (e.g.,
           `jp2_family_tgt') which may write to `kdu_compressed_target'
           objects, the purpose of this function is to allow the
           caller to overwrite some previously written data.  It is
           not a general repositioning function.  Also, rewriting is not
           recursive (no nested rewrites).  Having written N bytes of
           data, you may start rewriting from position B, where N-B is
           the `backtrack' value.  You may not then write any more than
           N-B bytes before calling `end_rewrite', which restores the
           write pointer to position N.  Attempting to write more data
           than this will result in a false return from `write'.
           [//]
           Compressed data targets do not have to offer the rewrite
           capability.  In this case, the present function must return false.
           An application can test whether or not the rewrite capability
           is offered by invoking `start_rewrite' with `backtrack'=0,
           followed by a call to `end_rewrite'.
         [RETURNS]
           False if any of the following apply:
           [>>] The target does not support rewriting;
           [>>] The `backtrack' value would position the write pointer at
                an illegal position, either beyond the current write pointer
                or before the start of the logical target device; or
           [>>] The target object is already in a rewrite section (i.e., 
                `start_rewrite' has previously been called without a matching
                call to `end_rewrite').
         [ARG: backtrack]
           Amount by which to backtrack from the current write position,
           to get to the portion of the target stream you wish to overwrite.
           Any attempt to write more than `backtrack' bytes before a
           subsequent call to `end_rewrite' should result in a false return
           from the `write' function.  If `backtrack' is less than 0,
           or greater than the current number of bytes which have been
           written, the function should return 0.
      */
    virtual bool end_rewrite() { return false; }
      /* [SYNOPSIS]
           This function should be called after completing any rewriting
           operation initiated by `start_rewrite'.  The function
           repositions the write pointer to the location it had prior
           to the `start_rewrite' call.
           [//]
           It is safe to call this function, even if `start_rewrite' was
           never called, in which case the function has no effect and
           should return false.
      */
    virtual bool write(const kdu_byte *buf, int num_bytes) = 0;
      /* [SYNOPSIS]
           This function must be implemented in every derived class.  It
           implements the functionality of a sequential write operation,
           transferring `num_bytes' bytes from the supplied buffer to the
           target.
           [//]
           The system provides its own internal temporary buffering for
           compressed data to maximize internal access efficiency.  For this
           reason, the virtual `write' function defined here will usually be
           called to transfer quite a few bytes at a time.
         [RETURNS]
           The function returns true unless the transfer could not
           be completed for some reason, in which case it returns false.
      */
    virtual void set_target_size(kdu_long num_bytes) { return; }
      /* [SYNOPSIS]
           This function may be called by the rate allocator at any point,
           except within a rewrite section (see `start_rewrite'), to indicate
           the total size of the code-stream currently being generated,
           including all marker codes.  The function may be called after some
           or all of the code-stream bytes have been delivered to the
           `write' member function; in fact, it may never be called at
           all.  The `kdu_codestream' object's rate allocation procedure
           (e.g., `kdu_codestream::flush') may attempt to identify the
           code-stream size early on, but this might not always be possible.
           If the target requires knowledge of the size before writing the
           data, it must be prepared to buffer the code-stream data until
           this function has been called or the object is destroyed.
           [//]
           This function is not meaningful for structured cache
           (non-sequential) compressed data targets and will not generally
           be invoked if `get_capabilities' returns `KDU_TARGET_CAP_CACHED'.
      */
  };



/*****************************************************************************/
/*                      kdu_compressed_target_nonnative                      */
/*****************************************************************************/

class kdu_compressed_target_nonnative : public kdu_compressed_target {
  /* [BIND: reference]
     [SYNOPSIS]
     This alternate base class is provided explicitly for binding to
     non-native languages.  The `kdu_compressed_target' class cannot be
     directly inherited by Java, C# or other non-native languages because
     it contains a pure virtual function `write', which cannot be safely
     implemented across language boundaries.  The reason for this is that
     the `write' function accepts an unsafe memory buffer (i.e., an array
     without explicit dimensions) from which the data is supposed to be
     extracted by the implementing class.
     [//]
     The present class gets around that problem by providing an implementation
     for `write' which invokes a `post_write' member function that is
     inheritable.  The `post_write' function's implementation in the derived
     class should invoke `pull_data' to bring the data into the foreign
     language implementation via an explicitly dimensioned array.  Consult
     the relevant function descriptions for more on this.
     [//]
     To fully support structured cache compressed data targets implemented
     in a non-native language, the present class also provides an
     implementation for `end_precinct', which invokes a `post_end_precinct'
     function that is inheritable, from which it is possible to query the
     lengths of each packet in the precinct by calling
     `retrieve_packet_lengths'.
  */
  public: // Member functions
    kdu_compressed_target_nonnative()
      { pending_write_buf=NULL; pending_write_bytes=0;
        pending_packet_lengths=NULL; pending_num_packets=0; }
    virtual int get_capabilities() { return KDU_TARGET_CAP_SEQUENTIAL; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_target::get_capabilities' function
           so that it can be redeclared as a callback function for foreign
           language classes to provide a meaningful implementation.  Returns
           `KDU_TARGET_CAP_SEQUENTIAL' if you do not implement it in your
           derived class.
      */
    virtual void start_tileheader(int tnum, int num_tiles) { return; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_target::start_tileheader' function
           so that it can be redeclared as a callback function for foreign
           language classes to provide a meaningful implementation.
      */
    virtual void end_tileheader(int tnum) { return; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_target::end_tileheader' function
           so that it can be redeclared as a callback function for foreign
           language classes to provide a meaningful implementation.
      */
    virtual void start_precinct(kdu_long unique_id) { return; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_target::start_precinct' function
           so that it can be redeclared as a callback function for foreign
           language classes to provide a meaningful implementation.
      */
    virtual void post_end_precinct(int num_packets) { return; }
      /* [BIND: callback]
           This function is called for you, in response to an underlying call
           to `kdu_compressed_target::end_precinct'.  You must implement
           the present function in a derived class, using
           `retrieve_packet_lengths' if you wish to recover the packet
           length information that was supplied with the call to
           `end_precinct'.
      */
    int retrieve_packet_lengths(int num_packets, kdu_long packet_lengths[])
      { 
        if (num_packets > pending_num_packets)
          num_packets = pending_num_packets;
        for (int i=0; i < num_packets; i++)
          packet_lengths[i] = pending_packet_lengths[i];
        return num_packets;
      }
      /* [SYNOPSIS]
           If you wish to learn the number of bytes written to each packet
           within a precinct, you should invoke this function from within
           the `post_end_precinct' call -- at any other point, the function
           will just return 0.  Within `post_end_precinct', the function
           returns the minimum of `num_packets' and the number of packets
           that was passed to the `post_end_precinct' function, and
           writes this number of length values to the `packet_lengths' array,
           starting with the length of the first packet in the precinct.
      */
    virtual bool start_rewrite(kdu_long backtrack) { return false; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_target::start_rewrite' function
           so that it can be redeclared as a callback function for foreign
           language classes to provide a meaningful implementation.  Returns
           false if you do not implement it in your derived class.
      */  
    virtual bool end_rewrite() { return false; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_target::end_rewrite' function
           so that it can be redeclared as a callback function for foreign
           language classes to provide a meaningful implementation.  Returns
           false if you do not implement it in your derived class.
      */
    virtual void set_target_size(kdu_long num_bytes) { return; }
      /* [BIND: callback]
         [SYNOPSIS]
           Overrides the `kdu_compressed_target::set_target_size' function
           so that it can be redeclared as a callback function for foreign
           language classes to provide a meaningful implementation.
      */
    virtual bool post_write(int num_bytes) { return false; }
      /* [BIND: callback]
         [SYNOPSIS]
           This function is called for you, in response to an underlying call
           to the `kdu_compressed_target::write' function.  You must implement
           the present function in a derived class, using `pull_data'
           to recover the data that was supplied by the
           `kdu_compressed_target::write' function.
           [//]
           Note that the implementation of this object is not thread safe
           in and of itself, meaning that asynchronous calls to the `write'
           function from multiple threads could produce race conditions
           as they call through the `post_write' function.  However, this
           should not present a problem in practice, since you should
           associate a separate `kdu_compressed_target'-derived object with
           each distinct `kdu_codestream' that generates compressed data and
           Kakadu's multi-threaded environment ensures that threads which
           share a single `kdu_codestream' via the `kdu_thread_env'
           mechanism will not simultaneously access its compressed data
           target.
         [RETURNS]
           Your derived class' implementation of this function should return
           false if and only if the underlying `kdu_compressed_target::write'
           function would return false.  That is, you should return false
           only if you cannot actually write all the data for some reason
           (e.g., a file might be full).  If you do not implement the
           `post_write' funtion in your derived class, a value of false will
           always be returned by this funtion.
         [ARG: num_bytes]
           The number of bytes your derived class is expected to retrieve
           via one or more calls to `pull_data'.
      */
    int pull_data(kdu_byte data[], int first_byte_pos, int num_bytes)
      {
        if (num_bytes > pending_write_bytes) num_bytes = pending_write_bytes;
        pending_write_bytes -= num_bytes;
        int result = num_bytes;
        for (data+=first_byte_pos; num_bytes > 0; num_bytes--)
          *(data++) = *(pending_write_buf++);
        return result;
      }
      /* [SYNOPSIS]
           You must call this function only from your derived class'
           implementation of the `post_write' function, in order to recover
           the data delivered by the internal `kdu_compressed_target::write'
           function.
           [//]
           In order to complete the recovery of data delivered via `post_write'
           your derived class may, if desired, retrieve the data in multiple
           calls to the present function.  Each call recovers some number of
           bytes from the buffer originally passed into the internal call to
           `kdu_compressed_target::write'.
           [//]
           The data retrieved by this function is written to the location
           in the `data' array identified by `first_byte_pos', running
           through to the location `first_byte_pos'+`num_bytes'-1.
         [RETURNS]
           The number of bytes actually written into the `data' array, which
           may be less than `num_bytes' if the cumulative number of bytes
           requested in calls to this function since the last call to
           `post_write' exceeds the number of bytes passed as an argument
           to that function.
         [ARG: data]
           Array into which the data provided by the underlying
           `kdu_compressed_target::write' call is to be written.
         [ARG: first_byte_pos]
           Location of the first byte of the `data' array into which data
           is to be written.
         [ARG: num_bytes]
           Maximum number of bytes which are to be written into the `data'
           array in this call.  If this value exceeds the number of as-yet
           uncollected bytes from the underlying `kdu_compressed_target::write'
           call, the function returns a value which is less than `num_bytes'.
      */
  private:
    virtual void end_precinct(kdu_long unique_id, int num_packets,
                              const kdu_long packet_lengths[])
      { 
        this->pending_packet_lengths = packet_lengths;
        this->pending_num_packets = num_packets;
        post_end_precinct(num_packets);
        pending_packet_lengths = NULL;
        pending_num_packets = 0;
      }
    virtual bool write(const kdu_byte *buf, int num_bytes)
      { 
        pending_write_buf = buf;
        pending_write_bytes = num_bytes;
        bool result = post_write(num_bytes);
        pending_write_buf = NULL;
        pending_write_bytes = 0;
        return result;
      }
  private: // Data
    const kdu_byte *pending_write_buf;
    int pending_write_bytes;
    const kdu_long *pending_packet_lengths;
    int pending_num_packets;
  };

/*****************************************************************************/
/*                                 kdu_codestream                            */
/*****************************************************************************/

#define KD_THREADLOCK_GENERAL 0  // For structure, parsing and flush
#define KD_THREADLOCK_ROI 1      // Guard calls to `kdu_roi_node::pull'
#define KD_THREADLOCK_PRECINCT 2 // For precinct resource allocation 
#define KD_THREADLOCK_COUNT 3

#define KDU_FLUSH_USES_THRESHOLDS_AND_SIZES ((int) 1)
#define KDU_FLUSH_THRESHOLDS_ARE_HINTS      ((int) 2)

class kdu_codestream {
  /* [BIND: interface]
     [SYNOPSIS]
     Objects of this class are only interfaces, having the size of a single
     pointer (memory address).  Copying the object has no effect on the
     underlying state information, but simply serves to provide another
     interface (or reference) to it.
     [//]
     There is no destructor, because destroying
     an interface has no impact on the underlying state.  Unlike the other
     interface classes provided for accessing Kakadu's code-stream management
     sub-system (`kdu_tile', `kdu_tile_comp', `kdu_resolution', `kdu_subband',
     and so on), this one does have a creation function.  The creation
     function is used to build the internal machinery to which the interface
     classes provide access.
     [//]
     The internal machinery manages the various entities associated with a
     JPEG2000 code-stream: tiles, tile-components, resolutions, subbands,
     precincts and code-blocks.  These internal entities are accessed
     indirectly through the public `kdu_tile', `kdu_tile_comp',
     `kdu_resolution', `kdu_subband', `kdu_precinct' and `kdu_block'
     interface classes.  The indirection serves to protect applications
     from changes in the implementation of the internal machinery and
     vice-versa.
     [//]
     Perhaps even more importantly, indirect access to the internal
     code-stream management machinery through these interface classes
     allows a rotated, flipped, zoomed or spatially restricted view of the
     actual compressed image to be presented to the application.  The
     application can manipulate the geometry and the spatial region of
     interest (i.e., a viewport) through various member functions provided
     by the `kdu_codestream' interface.  The information transferred to or
     from the application via the various interface classes documented
     in this header file is transparently massaged so as to give the
     impression that the geometry of the application's viewport and that
     of the compressed image are identical.
     [//]
     Be sure to read the descriptions of the `create', `destroy' and
     `access_siz' functions carefully.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle member functions
    kdu_codestream() { state = NULL; }
      /* [SYNOPSIS]
         Creates an empty interface.  You should not
         invoke any of the member functions, other than `create', `exists'
         or `operator!' on such an object.
      */
    bool exists() const { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
         Returns true after a successful call to `create' and before a
         call to `destroy'.  Be careful of copies.  If the interface is
         copied then only the copy on which `destroy' is called will
         actually indicate this fact via a false return from `exists'.
      */
    bool equals(const kdu_codestream rhs) const
      { return (state == rhs.state); }
      /* [SYNOPSIS]
           Returns true if `rhs' is an interface to the same codestream
           machinery as the present object.
      */
    bool operator!() const { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
         Opposite of `exists'.  Returns true if the interface is empty. */
    bool operator==(const kdu_codestream rhs) const
      { return (state == rhs.state); }
      /* [SYNOPSIS] Same as `equals'. */
    bool operator!=(const kdu_codestream rhs) const
      { return (state != rhs.state); }
      /* [SYNOPSIS] Opposite of `equals'. */
    KDU_EXPORT void
      create(siz_params *siz, kdu_compressed_target *target,
             kdu_dims *fragment_region=NULL, int fragment_tiles_generated=0,
             kdu_long fragment_tile_bytes_generated=0,
             kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Constructs the internal code-stream management machinery, to be
           accessed via the `kdu_codestream' interface and its close
           relatives, `kdu_tile', `kdu_tile_comp', `kdu_resolution',
           `kdu_subband', `kdu_precinct' and `kdu_block'.
           [//]
           We refer to the machinery created by this particular version of
           the overloaded `create' function as an `output codestream', for
           want of more appropriate terminology.  The function
           prepares the internal machinery for generation of a new JPEG2000
           code-stream.  This is required by applications which need to
           compress an image, or applications which need to transcode an
           existing compressed image into a new one.  In any event, the
           distinguishing features of this version of the `create' function
           are that it requires a compressed data `target' object and that
           it requires a `siz_params' object, identifying the dimensional
           properties (image component dimensions and related info) which
           establish the structural configuration of a JPEG2000 code-stream.
           [//]
           Once this function returns, you will generally want to invoke
           the `access_siz' function to gain access to the complete parameter
           sub-system created here.  With the returned pointer, you can
           perform any specific parameter configuration tasks of interest
           to your application.  You should then invoke the
           `kdu_params::finalize_all' function on the pointer returned by
           `access_siz', so as to finalize your parameter settings.
           Thereafter, you should make no further parameter changes.  It
           is best to do these things before calling any other member
           functions of this object, but definitely before using the
           `open_tile', `get_dims', `get_tile_dims', `get_subsampling' or
           `get_registration' function.  The behaviour could otherwise
           be unreliable.
           [//]
           The function is deliberately not implemented as a constructor,
           since robust exception handlers might not always be able to clean up
           partially constructed objects if an error condition is thrown from
           within a constructor (as a general rule, constructors are the
           least robust place to do significant work).  For safety, this
           function should never be invoked on a non-empty `kdu_codestream'
           interface (one whose `exists' function returns true).
           [//]
           Starting from Kakadu v4.3, this function includes three optional
           arguments which may be used to control the generation of
           codestream fragments.  This allows applications to compress
           absolutely enormous images (easily into the tens or hundreds of
           tera-pixels) in pieces, putting all the pieces together
           automatically.  To access this facility, the following notes
           should be heeded:
           [>>] Each fragment must consist of a whole number of tiles.
           [>>] The `siz' parameters identify the dimensions of the entire
                image, while `fragment_region' supplies the location and
                dimensions of the region represented by the collection of
                tiles which you are compressing in the current fragment.
           [>>] Once `create' returns, you need to configure the coding
                parameters exactly as you would normally, calling `access_siz'
                to access the parameter sub-system, and filling in whatever
                non-default coding parameters are of interest.  The parameter
                subsystem must be configured fully, as if you were compressing
                all tiles at once, and it must be configured in exactly the
                same way for each fragment, except that you need only provide
                tile-specific parameters when you are compressing the
                fragment which contains those tiles.  If you provide
                tile-specific parameters for other tiles, they will be
                ignored.
           [>>] You must indicate the number of tiles which have been
                already compressed in all previous fragments, via the
                `fragment_tiles_generated' argument.  A main header will
                be written only when generating the first fragment -- i.e.,
                the one for which `fragment_tiles_generated' is 0.  An EOC
                marker will be appended only to the last fragment -- the
                system can figure out that you are compressing the last
                fragment, by adding the number of tiles represented by
                the `fragment_region' to the number which have already
                been generated, and comparing this with the total number
                of tiles in the entire image.  See also the
                `is_last_fragment' function.
           [>>] You must indicate the number of tile bytes which have
                been previously generated, via `fragment_tile_bytes_generated'.
                This value corresponds to the cumulative length of all
                generated fragments so far, excluding only the main header.
                After generating a fragment, you may discover the number
                of tile bytes generated for that fragment by calling the
                `get_total_bytes' function, with the
                `exclude_main_header' argument set to false.  You will have
                to add the values generated for all previous fragments
                yourself.
           [>>] If you want TLM marker segments to be generated for you (this
                is generally a very good idea when compressing huge images),
                the `target' object will need to support the functionality
                represented by `kdu_compressed_target::start_rewrite' and
                `kdu_compressed_target::end_rewrite'.  Moreover, you will
                have to make sure that these functions can be used to
                backtrack into the main header to insert TLM information.
                This means that it must generally be possible to backtrack
                to a position prior to the start of the current fragment.
                The simplest way to implement this is by passing a single
                open `kdu_compressed_target' object into the `create'
                function associated with each fragment, closing it only
                once all fragments have been written.  However, by clever
                implementation of the `kdu_compressed_target' interface,
                your application can arrange to place the fragments in
                separate files if you like.
           [>>] It is possible to implement a `kdu_compressed_target'
                object in such a way that you can process multiple
                fragments concurrently.  This might appear to be
                impossible, since the `fragment_tile_bytes_generated'
                cannot be known until all previous fragments have been
                processed completely, suggesting that you will have to
                process them in sequence.  However, the
                `fragment_tile_bytes_generated' value is actually used only
                to compute the location of the TLM marker segments relative
                to the current location.  This means that you can actually
                supply fake values for `fragment_tile_bytes_generated' so
                long as your implementation of the `target' object can
                correctly interpret `kdu_compressed_target::start_rewrite'
                calls based on these fake `fragment_tile_bytes_generated'
                values, as references to specific locations in the first
                fragment.
           [>>] All public interface functions offered by the
                `kdu_codestream' object, or any of its descendants
                (`kdu_tile', `kdu_tile_comp', `kdu_resolution',
                `kdu_subband', `kdu_block', `kdu_precinct', etc.) will
                behave exactly as though you were compressing an image
                whose location and dimensions on the canvas were equal to
                those identified by `fragment_region'.  This means that
                almost every aspect of an application can be ignorant of
                whether it is compressing an entire image or just a
                fragment.  Amongst other things, this also means that
                the `get_valid_tiles' member function returns only the
                range of tiles which are associated with this fragment
                and `get_dims' returns only the dimensions of the
                fragment.  The only exceptions are the `kdu_tile::get_tnum'
                function, which returns the true index of a tile, as it
                will appear within the final code-stream, and the
                `kdu_codestream::is_last_fragment' function, which indicates
                whether or not the present fragment is the terminal one.
                This is required in order to access tile-specific elements
                of the coding parameter sub-system (see `kdu_params').  The
                coding parameter sub-system is ignorant of fragmentation.
         [ARG: target]
           Points to the object which is to receive the compressed JPEG2000
           code-stream when or as it becomes available.
         [ARG: siz]
           Pointer to a `siz_params' object, which the caller must supply to
           permit determination of the basic structure.  The contents of
           this object must have been filled out (and finalized) by the
           caller.  An internal copy of the object is made for use by
           the internal machinery, so the caller is free to destroy the
           supplied `siz' object at any point.
         [ARG: fragment_region]
           If non-NULL, this argument points to a `kdu_dims' object which
           identifies the location and dimensions of a region which is
           being compressed as a separate code-stream fragment.  These
           are absolute coordinates on the global JPEG2000 canvas (high
           resolution grid).
         [ARG: fragment_tiles_generated]
           Total number of tiles associated with all previous fragments.
           This value is used to determine whether or not to write the
           codestream main header and whether or not to write the EOC
           (end-of-codestream) marker.  It is also used to figure out
           where to position TLM (tile-part-length) information associated
           with tile-parts generated in the current fragment, relative to
           the start of the TLM data, all of which appears consecutively in
           the main header.  This argument should be 0 if
           `fragment_region' is NULL.
         [ARG: fragment_tile_bytes_generated]
           Total number of tile data bytes (everything except the main header)
           associated with all fragments which will precede this one.
           This value is used only to correctly position TLM marker segments,
           so you can provide a fake value if you like, so long as your
           implementation of `target' is able to figure out where TLM
           data being generated in one fragment must be placed within the
           main header, based on the backtrack values supplied to
           `kdu_compressed_target::start_rewrite'.  This value should be
           0 if `fragment_region' is NULL.
         [ARG: env]
           If this argument is non-NULL, the function also constructs the
           internal machinery required to manage safe multi-threaded access
           and multi-threaded processing.  This machinery can be created
           later, however.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT void
      create(kdu_compressed_source *source, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Constructs the internal code-stream management machinery, to be
           accessed via the `kdu_codestream' interface and its close
           relatives, `kdu_tile', `kdu_tile_comp', `kdu_resolution',
           `kdu_subband', `kdu_precinct' and `kdu_block'.
           [//]
           We refer to the machinery created by this particular version of
           the overloaded `create' function as an `input codestream', for
           want of more appropriate terminology.  The function
           prepares the internal machinery for use in unpacking or
           decompressing an existing JPEG2000 code-stream, which is
           retrieved via the supplied `source' object on an as-needed
           basis.
           [//]
           For the same reasons mentioned in connection with the first
           version of the overloaded `create' function, the function is
           deliberately not implemented as a constructor and cannot be used
           unless (or until) the `kdu_codestream' interface is empty,
           meaning that its `exists' function returns false.
           [//]
           The function reads the main header of the JPEG2000 code-stream,
           up to but not including the first tile-part header.  Further
           code-stream reading occurs on a strictly as-needed basis,
           as tile, precinct and code-block data is requested by the
           application, or the sample data processing machinery.
        [ARG: source]
           Points to the object which supplies the actual JPEG2000
           code-stream data.  The capabilities of this object can have a
           substantial impact on the behaviour of the internal machinery.
           For example, if the `source' object supports seeking, the
           internal machinery may be able to avoid buffering substantial
           amounts of compressed data, regardless of the order in which
           decompressed image data is required.
           [//]
           If the source provides its own compressed data cache,
           asynchonously loaded from a remote server, the internal
           code-stream management machinery will deliberately avoid caching
           parsed segments of compressed data, since this would prevent it
           from responding to changes in the state of the `source' object's
           cache.
           [//]
           The interested reader, should consult the descriptions provided
           for the `kdu_compressed_source' class and its member functions,
           as well as some of the more advanced derived classes such as
           `kdu_cache', `kdu_client' or `jp2_source'.
        [ARG: env]
           If this argument is non-NULL, the function also constructs the
           internal machinery required to manage safe multi-threaded access
           and multi-threaded processing.  This machinery can be created
           later, however.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT void
      create(siz_params *siz, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This third form of the `create' function is used to create what we
           refer to as an `interchange codestream', for want of a better name.
           There is no source and no target for the compressed data.
           Code-blocks are generally pushed into the internal code-stream
           management machinery with the aid of the `kdu_precinct::open_block'
           function and assembled code-stream packets are generally recovered
           using `kdu_precinct::get_packets'.  The `kdu_subband' and
           `kdu_precinct' interfaces may ultimately be recovered from the
           top level `kdu_codestream' interface by exercising appropriate
           functions to open tiles, and access their tile-components,
           resolutions, subbands and precincts.
           [//]
           It is perhaps easiest to think of an `interchange codestream'
           as an `output codestream' (see the first form of the `create'
           function) without any compressed data target.  By and large,
           code-blocks are pushed into the object in the same manner as
           they are for an output codestream and this may be accomplished
           either by the data processing machinery (see
           "kdu_sample_processing.h") or (more commonly) by handing on
           code-blocks recovered from a separate input codestream.
           [//]
           There are a number of key points at which the behaviour of
           an `interchange codestream' differs sigificantly from that of an
           `output codestream'.  Firstly, the `kdu_precinct' interface is
           accessible only when the internal code-stream management machinery
           is created for interchange.  Moreover, when a precinct is closed
           (by calling `kdu_precinct::close'), all of the associated
           code-block data is discarded immediately, regardless of whether
           it has been put to any good purpose or not.
           [//]
           Interchange codestreams are primarily intended for implementing
           dynamic code-stream servers.  In such applications, a separate
           input codestream (usually one with the persistent mode enabled --
           see `set_persistent') is used to derive compressed data on an
           as-needed basis, which is then transcoded on-the-fly into packets
           which may be accessed in any order and delivered to a client based
           upon its region of interest.  The precinct size adopted by the
           interchange codestream object may be optimized for the
           communication needs of the client-server application, regardless
           of the precinct size used when compressing the separate input
           code-stream.  Other transcoding operations may be employed in
           this interchange operation, including the insertion of error
           resilience information.
         [ARG: siz]
           Plays the same role as its namesake in the first form of the
           `create' function, used to create an `output codestream'.  As in
           that case, the `siz_params' object must have been filled out
           (and finalized) by the caller; an internal copy of the object is
           made for use by the internal machinery.
         [ARG: env]
           If this argument is non-NULL, the function also constructs the
           internal machinery required to manage safe multi-threaded access
           and multi-threaded processing.  This machinery can be created
           later, however.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT void
      restart(kdu_compressed_target *target, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This function has a similar effect to destroying the existing
           code-stream management machinery (see `destroy') and then creating
           another one with exactly the same set of coding parameter
           attributes.  Its principle intent is for efficient re-use of the
           structures created for frame-by-frame video compression
           applications.
           [//]
           Note carefully, that this function may not be called unless
           `enable_restart' was called after the code-stream
           management machinery was originally created with the `create'
           function.
           [//]
           While the principle intent is that the original coding parameter
           attributes should be re-used, it is possible to access the parameter
           sub-system (see `access_siz') and modify individual parameters.
           If any parameter values are changed, however, the value of
           recreation (from an efficiency point of view) will be lost.
           To detect changes in the coding parameters, this function relies
           on the `kdu_params::clear_marks' and `kdu_params::any_changes'
           functions.
           [//]
           If you wish to textualize parameter attributes for the new
           code-stream, you must call `set_textualization' after the
           present function returns.  The textualization state is not
           preserved across multiple restarts.
           [//]
           Any code-stream comments generated using `add_comment' are
           discarded by this function.
         [ARG: target]
           Points to the object which will actually receive all compressed
           data generated by the restarted machinery.
         [ARG: env]
           As a general rule, in multi-threaded applications it is best
           to provide the caller's `kdu_thread_env' reference here rather
           than NULL.  This is not strictly necessary if you can be sure
           that all multi-threaded processing within the codestream
           has completed -- see `kdu_thread_env::cs_terminate' for a
           discussion of how you can do this.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT void
      restart(kdu_compressed_source *source, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This function has a similar effect to destroying the existing
           code-stream management machinery (see `destroy') and then creating
           another one. Its principle intent is for efficient re-use of the
           structures created for frame-by-frame video decompression
           applications.
           [//]
           Note carefully, that this function may not be called unless
           `enable_restart' was called after the code-stream
           management machinery was originally created with the second form
           of the `create' function.
           [//]
           All code-stream marker segments are read from the `source' object,
           exactly as for the second form of the `create' function.  If the
           coding parameters are exactly the same as they were in the existing
           object, a great deal of the internal machinery can be left intact,
           saving significant computational effort, especially when
           working with small video frame sizes.  Otherwise, the function
           essentially destroys the existing code-stream machinery and
           rebuilds it from scratch.  To detect changes in the coding
           parameters, this function relies on the `kdu_params::clear_marks'
           and `kdu_params::any_changes' functions.
           [//]
           If you wish to textualize parameter attributes for the new
           code-stream, you must call `set_textualization' after the
           present function returns.  The textualization state is not
           preserved across multiple restarts.
        [ARG: env]
           As a general rule, in multi-threaded applications it is best
           to provide the caller's `kdu_thread_env' reference here rather
           than NULL.  This is not strictly necessary if you can be sure
           that all multi-threaded processing within the codestream
           has completed -- see `kdu_thread_env::cs_terminate' for a
           discussion of how you can do this.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT void
      share_buffering(kdu_codestream existing);
      /* [SYNOPSIS]
           When two or more codestream objects are to be used simultaneously,
           considerable memory can be saved by encouraging them to use the
           same underlying service for buffering compressed data.  This is
           because resources allocated for buffering compressed data are not
           returned to the system heap until the object is destroyed.  By
           sharing buffering resources, one code-stream may use excess
           buffers already freed up by the other code-stream.  This is
           particularly beneficial when implementing transcoders.  The
           internal buffering service will not be destroyed until all
           codestream objects which are sharing it have
           been destroyed, so there is no need to worry about the order in
           which the codestream objects are destroyed.
           [//]
           This function is not completely thread-safe.  In multi-threading
           environments (see `kdu_thread_env'), you should do all your
           buffering sharing before you open tiles and start processing
           (most notably, parallel processing in multiple threads) within
           the codestreams which are going to be sharing their storage.
           This is not likely to present an obstacle in practice.
           [//]
           In multi-threaded environments, you should also make sure that
           all access to codestreams which share buffering involves
           the same thread group (created using `kdu_thread_entity::create')
           and supplies the optional `kdu_thread_env' reference to all
           Kakadu functions which can accept it.
      */
    KDU_EXPORT void
      mem_failure(const char *mem_category, const char *message);
      /* [SYNOPSIS]
           This function may be called if an internal error has occurred in
           connection with memory allocation.  Currently, the function is
           called only if numerical overflow has occurred in a call to
           any of the pre-alloc functions offered by `kdu_sample_allocator'.
           In general, though, the function is called with a brief `message'
           that explains the failure (e.g., "internal error (numerical "
           "overflow)" and a `mem_category' string that identifies the
           sub-system that produced the error -- neither argument should be
           NULL, as they are used to synthesize a suitable error message
           for `kdu_error'.
      */
    KDU_EXPORT void
      destroy();
      /* [SYNOPSIS]
           Providing an explicit destructor for `kdu_codestream' is
           dangerous, since it is merely a container for a hidden reference
           to the internal code-stream mangement machinery.  The
           interpretation of copying should be (and is) that of generating
           another reference, so the underlying object should not be
           destroyed when the copy goes out of scope.  Reference counting
           would be an elegant, though tedious and less explicit way of
           avoiding this difficulty.
           [//]
           Destroys the internal code-stream management machinery, along with
           all tiles, tile-components and other subordinate state information
           for which interfaces are described in this header.
           [//]
           When you invoke this function within a multi-threaded environment,
           based around `kdu_thread_env', you need to be careful that no
           processing is taking place within the codestream on other threads.
           To this end, you would normally use `kdu_thread_env::cs_terminate'.
           An agressive way to do this is to completely close down (i.e.,
           destroy) a `kdu_thread_env' environment before destroying
           codestreams, but this is not necessary; it should be perfectly
           safe to have ongoing multi-threaded processing progressing in
           the background on other codestreams while you destroy one for
           which processing has terminated.
      */
    KDU_EXPORT void
      enable_restart();
      /* [SYNOPSIS]
           This function may not be called after the first call to `open_tile'.
           By calling this function, you notify the internal code-stream
           management machinery that the `restart' function may be called
           in the future.  This prevents certain types of early clean-up
           behaviour, preserving as much as possible of the structure which
           could potentially be re-used after a `restart' call.
      */
    KDU_EXPORT void
      set_persistent();
      /* [SYNOPSIS]
           The persistent mode is important for interactive applications.  By
           default, tiles, precincts and code-blocks will be discarded as soon
           as it can be determined that the user will no longer be accessing
           them.  Moreover, after each code-block has been opened and closed,
           it will automatically be discarded.  This behaviour minimizes memory
           consumption when the image (or some region of interest) is to be
           decompressed only once.  For interactive applications, however, it
           is often desirable to leave the code-stream intact and permit
           multiple accesses to the same information so that a new region or
           resolution of interest can be defined and decompressed at will.
           For these applications, you must invoke this member function
           before any attempt to access (i.e., open) an image tile.
           [//]
           Evidently, persistence is irrelevant for `output codestreams'.
           For `interchange codestreams', this function also has no effect.
           Interchange codestreams have some persistent properties in that
           any tile may be opened and closed as often as desired.  They
           also have some of the properties of the non-persistent mode, in
           that compressed data is discarded as soon as the relevant precinct
           is closed (but not when the containing tile is closed!).  For more
           on these issues, see the description of
           `kdu_resolution::open_precinct'.
      */
    KDU_EXPORT kdu_long
      augment_cache_threshold(int extra_bytes);
      /* [SYNOPSIS]
           This function plays an important role in managing the memory
           consumption and data access overhead associated with persistent
           input codestreams.  In particular, it controls the conditions
           under which certain reloadable elements may be unloaded from
           memory.  These elements are as follows:
           [>>] Whole precincts, including their code-block state structure
                and compressed data, may be unloaded from memory so long
                as the following conditions are satisfied: 1) the compressed
                data source's `kdu_compressed_data_source::get_capabilities'
                function must advertise one of `KDU_SOURCE_CAP_SEEKABLE' or
                `KDU_SOURCE_CAP_CACHED'; 2) sufficient information must be
                available (PLT marker segments or `KDU_SOURCE_CAP_CACHED')
                to support random access into the JPEG2000 packet data; and
                3) all packets of any given precinct must appear
                consecutively (in the code-stream, or in a data source
                which advertises `KDU_SOURCE_CAP_CACHED').  These conditions
                are enough to ensure that unloaded precincts can be
                reloaded on-demand.  Precincts become candidates for
                unloading if they do not contribute to the current region
                of interest within a currently open tile.
           [>>] Whole tiles, including all structural state information and
                compressed data, may be unloaded from memory, so long as
                the compressed data source's
                `kdu_compressed_data_source::get_capabilities' function
                advertises one of the `KDU_SOURCE_CAP_SEEKABLE' or
                `KDU_SOURCE_CAP_CACHED' capabilities.  Tiles become
                candidates for unloading if they are not currently open.
                However, once unloading is triggered, the system attempts
                to unload those which do not intersect with the current
                region of interest first.  Since the search for tiles
                which do not belong to the current region of interest
                has a complexity which grows with the number of potentially
                unloadable tiles, this number is limited.  You can change
                the limit using the `set_tile_unloading_threshold' function.
           [//]
           By default, tiles and precincts are unloaded from memory
           as soon as possible, based on the criteria set out above.  While
           this policy minimizes internal memory consumption, it can
           result in quite large access overheads, if elements of the
           codestream need to be frequently re-loaded from disk and reparsed.
           [//]
           To minimize this access burden, the internal machinery provides
           two caching thresholds which you can set.  The threshold set
           using the present function refers to the total amount of memory
           currently being consumed by loaded precincts and tiles, including
           both the compressed data and the associated structural and
           addressing information.  Once a precinct or a tile becomes
           a candidate for unloading, it is entered on an internal list,
           from which it is unloaded only if the cache threshold is
           exceeded.  Unloading of old precincts is contemplated immediately
           before new precincts (or their packets) are loaded, while
           unloading of tiles is contemplated immediately before new
           tile-parts are loaded from the code-stream.  Since unloading is
           considered only at these points, the cache threshold may be
           crossed substantially if a new precinct or tile-part is very
           large, or if the application's data access patterns are not
           amenable to resource recycling.
           [//]
           The cache threshold associated with this function starts out at 0
           (meaning, unload all unloadable elements before loading new ones)
           but may be augmented as required by calling this function any
           number of times.
           [//]
           If `share_buffering' has been used to share buffer resources with
           other codestream management machines, this function controls the
           caching threshold for all participating entities.
           [//]
           You should be aware that the internal implementation involves
           quite a few approximations, in the interest of efficiency.  This
           is especially important for multi-threaded applications where
           memory is allocated centrally and potentially distributed to
           separate thread-specific allocators.  When multiple codestreams
           share a common memory management engine through `share_buffering',
           the approximations may become even more conservative.  For these
           reasons, cache threshold of a few hundred kBytes or less might
           not behave much differently from a cache threshold of 0.
           [//]
           This function is inherently thread-safe.
         [RETURNS]
           The new size of the threshold, which is `extra_bytes' larger than
           the previous value.  It is legal to call this function with
           `extra_bytes'=0 to find out the current threshold.
      */
    KDU_EXPORT int
      set_tile_unloading_threshold(int max_tiles_on_list,
                                   kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This function sets the second of two thresholds associated with
           the internal memory management logic described in conjunction with
           the `augment_cache_threshold' function.  This second threshold
           serves to limit the number of unloadable tiles which the
           code-stream management machinery is prepared to manage
           on an internal list.  A tile becomes unloadable once it is closed,
           so long as the compressed data source is seekable (or a dynamic
           cache) and the codestream manage is in the persistent mode (see
           `set_persistent').  Actually, a closed tile may become unloadable
           slightly later, if it has a partially parsed tile-part -- this is
           a feature of the on-demand parsing machinery, which waits to
           see if the tile will subsequently be re-opened and accessed,
           before completing the parsing of a partially parsed tile-part.
           [//]
           When a new tile-part is about to be parsed into memory, the
           cache management machinery checks to see if the number of
           unloadable tiles exceeds `max_tiles_on_list'.  If so, unloadable
           tiles are deleted as necessary (by definition, they can be
           reloaded later).  This minimizes the number of tiles which must
           be scanned to determine which ones intersect with the current
           region of interest, since we want to unload those which do
           not intersect before unloading those which do.
           [//]
           Whenever this function is called, the tile unloading process is
           triggered automatically, so the function also provides you with
           a mechanism for explicitly deciding when to unload tiles from
           memory so as to satisfy both the `max_tiles_on_list' limit
           and the cache memory limit adjusted by `augment_cache_threshold'.
           In particular, if you call this function with `max_tiles_on_list'
           equal to 0, all unloadable tiles will immediately be deleted from
           memory and their precinct and bit-stream buffering resources
           will be recycled for later use.
           [//]
           If you do not call this function, a default unloading threshold
           of 64 will be used.  Unlike the cache memory threshold controlled
           by `augment_cache_threshold', the tile list threshold is
           specific to each `kdu_codestream' object, regardless of whether
           or not `share_buffering' has been used to share buffering
           resources amongst multiple codestreams.
         [RETURNS]
           The previous value of the tile unloading threshold.
         [ARG: env]
           In a multi-threaded environment, it is strongly recommended that
           you supply the caller's `kdu_thread_env' reference here.  If you
           do not, and the codestream has been created or used with
           a `kdu_thread_env'-based multi-threaded processing system already,
           the present function will update the tile unloading threshold but
           will not immediately perform any unloading that might otherwise
           be required.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT bool
      is_last_fragment();
      /* [SYNOPSIS]
           Returns true if the `flush' function can be expected to write an
           EOC marker.  This should be the case whenever the codestream was
           created for output (it will never be the case if it was created
           for input or interchange), except when compressing a non-terminal
           fragment.
      */
  // --------------------------------------------------------------------------
  public: // Member functions used to access information
    KDU_EXPORT siz_params *
      access_siz();
      /* [SYNOPSIS]
           Associated with the internal code-stream management machinery
           (and hence with every non-empty `kdu_codestream' interface) is a
           unique `siz_params' object, which is the head (or root) of the
           entire network of coding parameters associated with the relevant
           code-stream.  These concepts are discussed in the description
           of the `kdu_params' object.
           [//]
           Because the returned object provides access to the entire
           network of `kdu_params'-derived objects relevant to the
           code-stream, the caller may use the returned pointer to
           parse command-line arguments (see `kdu_params::parse_string'),
           copy parameters between code-streams (see
           `kdu_params::copy_with_xforms') and to explicitly set or
           retrieve individual parameter values (see `kdu_params::set'
           and `kdu_params::get'), as they appear in code-stream marker
           segments.
           [//]
           Be careful to note that the `kdu_codestream' interface
           and its immediate relatives, `kdu_tile', `kdu_tile_comp',
           `kdu_resolution', etc., support geometrically transformed views
           of the underlying compressed image representation.  These
           transformed views are not reflected in the explicit parameter
           values which might be retrieved or configured directly using
           the various member functions offered by `kdu_params'.  The
           same principle applies to codestream fragments, which you
           might be generating if you have supplied a non-NULL
           `fragment_region' argument to `create'.  For these
           reasons, you are generally advised to work only with the
           `kdu_codestream' functions and those of the related interfaces
           described here, except where new parameters must be explicitly
           configured for compression, or modified in a transcoding
           application.
           [//]
           If you are generating a new output codestream, you will
           generally want to invoke this function immediately after
           `create' so as to configure parameters of interest.  Thereafter,
           you should use `kdu_params::finalize_all' to finalize your
           coding parameters.  As explained in the comments accompanying
           the output for of the `create' function, these things are best
           done before invoking any other member functions of this
           object, but at least prior to calling the `open_tile',
           `get_dims', `get_tile_dims', `get_subsampling' or
           `get_registration' function.
      */
    KDU_EXPORT int
      get_num_components(bool want_output_comps=false);
      /* [SYNOPSIS]
           Returns the apparent number of image components.  Note that
           the return value is affected by calls to `apply_input_restrictions'
           as well as the `want_output_comps' argument.
           [//]
           If `want_output_comps' is false, the function returns
           the number of visible codestream image components -- a value which
           may have been restricted by previous calls to
           `apply_input_restrictions'.
           [//]
           If `want_output_comps' is true AND the last call
           to `apply_input_restrictions' specified
           `KDU_WANT_OUTPUT_COMPONENTS', or if there has been no call to
           `apply_input_restrictions', the function returns the number of
           output image components.  The distinction between codestream
           and output image components is particularly important where a
           Part-2 multi-component transform may be involved.  In this case,
           the number of output image components may differ from the number
           of codestream image components, even where no restrictions have
           been applied.  The distinction is explained more thoroughly in
           connection with the `apply_input_restrictions' function.
         [ARG: want_output_comps]
           Affects the interpretation of returned value, as explained above,
           but only if there have been no calls to `apply_input_restrictions'
           or the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
    KDU_EXPORT int
      get_bit_depth(int comp_idx, bool want_output_comps=false);
      /* [SYNOPSIS]
           Returns the bit-depth of one of the image components.
           The interpretation of the component index
           is affected by calls to `apply_input_restrictions' as well as
           the `want_output_comps' argument.
           [//]
           If `want_output_comps' is false, this is a codestream
           component index, but its interpretation is affected by any
           restrictions and/or permutations which may have been applied to
           the collection of visible components by previous calls to
           `apply_input_restrictions'.
           [//]
           If `want_output_comps' is true AND the last call to
           `apply_input_restrictions' specified `KDU_WANT_OUTPUT_COMPONENTS',
           or if there has been no call to
           `apply_input_restrictions', then the component index refers to an
           output image component.  The distinction between codestream and
           output image components is particularly important where a
           Part-2 multi-component transform may be involved.  In this case,
           the bit-depths associated with output components may be very
           different to those associated with codestream image components.
           The distinction is explained in connection with the
           `apply_input_restrictions' function.  Again, the
           interpretation of `comp_idx' may be influenced by restrictions
           and/or permutations which have been applied to the set of output
           components which are visible across the API (again, this is the
           role of `apply_input_restrictions').
         [ARG: want_output_comps]
           Affects the interpretation of `comp_idx', as explained above, but
           only if there have been no calls to `apply_input_restrictions' or
           the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
    KDU_EXPORT bool
      get_signed(int comp_idx, bool want_output_comps=false);
      /* [SYNOPSIS]
           Returns true if the image component originally had a signed
           representation; false if it was unsigned.
           The interpretation of the component index
           is affected by calls to `apply_input_restrictions' as well as
           the `want_output_comps' argument.  This is explained
           in connection with the `get_bit_depth' function.
         [ARG: want_output_comps]
           Affects the interpretation of `comp_idx', as explained in connection
           with `get_bit_depth'.  Note that this argument is relevant only if
           if there have been no calls to `apply_input_restrictions' or
           the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
    KDU_EXPORT void
      get_subsampling(int comp_idx, kdu_coords &subs,
                      bool want_output_comps=false);
      /* [SYNOPSIS]
           Retrieves the canvas sub-sampling factors for this image component.
           The interpretation of the component index is affected by
           calls to `apply_input_restrictions' as well as
           the `want_output_comps' argument.  This is explained
           in connection with the `get_bit_depth' function.
           [//]
           The sub-sampling factors are also affected by the
           `transpose' argument supplied in any call to the
           `change_appearance' member function.
           [//]
           The sub-sampling factors identify the separation between the
           notional centres of the component samples on the high resolution
           code-stream canvas.  Since component dimensions are affected
           by discarding resolution levels, but this has no impact on the
           high resolution canvas coordinate system, the sub-sampling factors
           returned by this function are corrected (shifted up) to
           accommodate the effects of discarded resolution levels, as
           specified in any call to `apply_input_restrictions'.
         [ARG: want_output_comps]
           Affects the interpretation of `comp_idx', as explained in connection
           with `get_bit_depth'.  Note that this argument is relevant only if
           if there have been no calls to `apply_input_restrictions' or
           the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
    KDU_EXPORT void
      get_registration(int comp_idx, kdu_coords scale, kdu_coords &crg,
                       bool want_output_comps=false);
      /* [SYNOPSIS]
           Retrieves component registration information for the indicated
           image component.  The horizontal and vertical registration offsets
           are returned in `crg' after scaling by `scale' and rounding to
           integers.  Samples from this component have horizontal locations
           (kx + crg.x/scale.x)*subs.x and vertical locations
           (ky + crg.y/scale.y)*subs.y, where kx and ky are integers ranging
           over the bounds returned by the `get_dims' member function and
           subs.x and subs.y are the sub-sampling factors for this component.
           The component offset information is recovered from any CRG marker
           segment in the code-stream; the default offsets of 0 will be used
           if no such marker segment exists.
           [//]
           The interpretation of the component index is affected by
           calls to `apply_input_restrictions' as well as
           the `want_output_comps' argument.  This is explained
           in connection with the `get_bit_depth' function.
           [//]
           The registration offsets and `scale' argument are also corrected
           to account for the effects of any prevailing geometric
           transformations which may have been applied through a call
           to the `change_appearance' member function.
           [//]
           If you are generating an output codestream, you should not call
           this function prior to completing the configuration of all
           codestream parameters and invoking `kdu_params::finalize_all'
           on the pointer returned via `access_siz'.
         [ARG: want_output_comps]
           Affects the interpretation of `comp_idx', as explained in connection
           with `get_bit_depth'.  Note that this argument is relevant only if
           if there have been no calls to `apply_input_restrictions' or
           the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
    KDU_EXPORT void
      get_relative_registration(int comp_idx, int ref_comp_idx,
                                kdu_coords scale, kdu_coords &crg,
                                bool want_output_comps=false);
      /* [SYNOPSIS]
           This function is almost identical to `get_registration', except
           that registration offsets are measured relative to any registration
           offsets associated with the reference component, as identified by
           `ref_comp_idx'.  If the reference image component has no
           registration offset, this function's behaviour is identical to
           `get_registration'.  If the two image components both have the
           same registration offset, the function sets `crg' to (0,0).
      */
    KDU_EXPORT void
      get_dims(int comp_idx, kdu_dims &dims,
               bool want_output_comps=false);
      /* [SYNOPSIS]
           Retrieves the apparent dimensions and position of an image component
           on the canvas.  These are calculated from the image region on the
           high resolution canvas and the individual sub-sampling factors for
           the image component.  Note that the apparent dimensions are affected
           by any calls which may have been made to `change_appearance' or
           `apply_input_restrictions'.  In particular, both
           geometric transformations and resolution reduction requests affect
           the apparent component dimensions.
           [//]
           If `comp_idx' is negative, the function returns the dimensions
           of the image on the high resolution canvas itself.  In this case,
           the return value is corrected for geometric transformations, but
           not resolution reduction requests.  Since the sub-sampling factors
           returned by `get_subsampling' are corrected for resolution
           reduction, the caller may recover the image component
           dimensions directly from the canvas dimensions returned here
           with `comp_idx'<0 by applying the sub-sampling factors.  For
           information on how to perform such mapping operations, see
           Chapter 11 in the book by Taubman and Marcellin.
           [//]
           The interpretation of non-negative component indices is affected by
           calls to `apply_input_restrictions' as well as
           the `want_output_comps' argument.  This is explained
           in connection with the `get_bit_depth' function.
           [//]
           If you are generating an output codestream, you should not call
           this function prior to completing the configuration of all
           codestream parameters and invoking `kdu_params::finalize_all'
           on the pointer returned via `access_siz'.
         [ARG: want_output_comps]
           Affects the interpretation of `comp_idx', as explained in connection
           with `get_bit_depth'.  Note that this argument is relevant only if
           if there have been no calls to `apply_input_restrictions' or
           the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
    KDU_EXPORT void
      get_tile_partition(kdu_dims &partition);
      /* [SYNOPSIS]
           Retrieves the location and dimensions of the tile partition.  In
           particular, `partition.pos' will be set to the location of the
           upper left hand corner of the first tile in the "uncut" partition,
           and `partition.size' will be set to the dimensions of each
           tile in the "uncut" partition.  By "uncut" partition, we mean the
           collection of tiles, all having the same size, which intersect
           with the image region.  The actual tiles are obtained by taking
           the intersection of each of element in the uncut partition with
           the image region itself.  The (m,n)'th tile in the uncut partition
           has upper left hand corner at location
           (`partition.pos.y'+m*`partition.size.y',
           `partition.pos.x'+n*`partition.size.x') on the high resolution
           canvas.
           [//]
           Note that the apparent tile partition is affected by any
           calls which may have been made to `change_appearance'.  That is,
           any geometric transformations will be reflected in the tile
           partition so that the returned partition is consistent with
           other geometrically transformed coordinates and dimensions.
           However, the tile partition is always expressed on the high
           resolution codestream canvas, without regard to any discarding
           of image resolution levels via `apply_input_restrictions'.
      */
    KDU_EXPORT void
      get_valid_tiles(kdu_dims &indices);
      /* [SYNOPSIS]
           Retrieves the range of tile indices which correspond to the current
           region of interest (or the whole image, if no region of interest
           has been defined).  The indices of the first tile within the
           region of interest are returned via `indices.pos'.  Note that
           these may be negative, if geometric transformations were
           specified via the `change_appearance' function.  The number of
           tiles in each direction within the region of interest is returned
           via `indices.size'.
           [//]
           Note that tile indices in the range `indices.pos' through
           `indices.pos'+`indices.size'-1 are apparent tile indices,
           rather than actual code-stream tile indices.  They are
           affected not only by the prevailing region of interest, but
           also by the geometric transformation flags supplied during
           any call to `change_appearance'.  The caller should not attempt
           to attach any interpretation to the absolute values of these
           indices.
      */
    KDU_EXPORT bool
      find_tile(int comp_idx, kdu_coords loc, kdu_coords &tile_idx,
                bool want_output_comps=false);
      /* [SYNOPSIS]
           Locates the apparent tile indices of the tile-component which
           contains the indicated location (it is also an apparent location,
           taking into account the prevailing geometric view and the number
           of discarded levels). Note that the search is conducted within the
           domain of the indicated image component.
         [RETURNS]
           False if the supplied coordinates lie outside the image or
           any prevailing region of interest.
         [ARG: comp_idx]
           Identifies the component, with respect to which the `loc' location
           is being specified.  Note that the interpretation of the component
           index is affected by calls to `apply_input_restrictions' as well
           as the `want_output_comps' argument.  This is explained
           in connection with the `get_bit_depth' function.
         [ARG: want_output_comps]
           Affects the interpretation of `comp_idx', as explained in connection
           with `get_bit_depth'.  Note that this argument is relevant only if
           if there have been no calls to `apply_input_restrictions' or
           the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
    KDU_EXPORT void
      get_tile_dims(kdu_coords tile_idx, int comp_idx, kdu_dims &dims,
                    bool want_output_comps=false);
      /* [SYNOPSIS]
           Same as `get_dims' except that it returns the location and
           dimensions of only a single tile (if `comp_idx' is negative) or
           tile-component (if `comp_idx' >= 0).  The tile indices must lie
           within the range identified by the `get_valid_tiles' function.
           As with `get_dims', all coordinates and dimensions are affected
           by the prevailing geometric appearance and constraints set up using
           the `change_appearance' and `apply_input_restrictions' functions.
           As a result, the function returns the same dimensions as those
           returned by the `kdu_tile_comp::get_dims' function provided by
           the relevant tile-component; however, the present function has
           the advantage that it can be used without first opening the
           tile -- an act which may consume substantial internal resources
           and may involve substantial parsing of the code-stream.
           [//]
           The interpretation of non-negative component indices is affected by
           calls to `apply_input_restrictions' as well as
           the `want_output_comps' argument.  This is explained
           in connection with the `get_bit_depth' function.
           [//]
           If you are generating an output codestream, you should not call
           this function prior to completing the configuration of all
           codestream parameters and invoking `kdu_params::finalize_all'
           on the pointer returned via `access_siz'.
         [ARG: want_output_comps]
           Affects the interpretation of `comp_idx', as explained in connection
           with `get_bit_depth'.  Note that this argument is relevant only if
           if there have been no calls to `apply_input_restrictions' or
           the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
    KDU_EXPORT int
      get_max_tile_layers();
      /* [SYNOPSIS]
           Returns the maximum number of quality layers seen in any tile
           opened or created so far.  The function returns 1 (smallest number
           of allowable layers) if no tiles have yet been opened.
           [//]
           If you need reliable information concerning the maximum
           number of quality layers across all tiles in the codestream, but
           do not necessarily wish to incur the overhead of opening and
           closing each tile, consider using the `create_tile' function
           instead.
      */
    KDU_EXPORT int
      get_min_dwt_levels();
      /* [SYNOPSIS]
           Returns the minimum number of DWT levels in any tile-component
           of any tile which has been opened or created so far.  This value is
           initially set to the number of DWT levels identified in the main
           header COD marker segment, but may decrease as tiles are opened.
           [//]
           If you need reliable information concerning the minimum
           number of DWT levels across all tiles in the codestream, but
           do not necessarily wish to incur the overhead of opening and
           closing each tile, consider using the `create_tile' function
           instead.
      */
    KDU_EXPORT bool
      can_flip(bool check_current_appearance_only);
      /* [SYNOPSIS]
           This function returns false if the underlying codestream contains
           some feature which is incompatible with view flipping.  Currently,
           the only feature which is incompatible with view flipping is
           the use of wavelet packet decomposition structures in which
           horizontally high-pass subband branches are further decomposed
           in the horizontal direction, or vertically high-pass subband
           branches are further decomposed in the vertical direction.  Part-2
           codestreams may contain such decomposition structures, specified
           via ADS (Arbitrary Decomposition Style) marker segments.  However,
           there are many useful packet decomposition structures which
           remain compatible with flipping.
           [//]
           Like the `get_min_dwt_levels' function, the value returned by
           this function may change as tiles are progressively opened.  This
           is because the flipping property might be lost only in one
           tile or even just one tile-component.  In any event, no errors
           will be generated by opening tiles within a codestream which
           cannot be flipped, even if the last call to `change_appearance'
           specified horizontal or vertical flipping, until
           `kdu_tile_comp::access_resolution' is called.  Thus, the most
           robust applications will check this present function prior to
           invoking `kdu_tile_comp::access_resolution'.
         [ARG: check_current_appearance_only]
           If this argument is true, the function will return false only if
           the flipping parameters requested via the most recent call to
           `change_appearance' cannot be satisfied.  Thus, if the current
           view does not involve flipping, the function will always return
           true.  If this argument is false, the function returns false if
           flipping is not supported, regardless of whether the current
           view involves flipping.
      */
    KDU_EXPORT void
      map_region(int comp_idx, kdu_dims comp_region, kdu_dims &hires_region,
                 bool want_output_comps=false);
      /* [SYNOPSIS]
           Maps a region of interest, specified in the domain of a single
           image component (or the full image) in the current geoemetry and
           at the current resolution onto the high-res canvas, yielding a
           region suitable for use with the `apply_input_restrictions' member
           function.  The supplied region is specified with respect to the
           same coordinate system as that associated with the region
           returned by `get_dims' -- i.e., taking into account component
           sub-sampling factors, discarded resolution levels and any
           geometric transformations (see `change_appearance') associated
           with the current appearance.   The component index is also
           interpreted relative to any restrictions on the range of
           available image components, unless negative, in which case the
           function is being asked to transform a region on the current
           image resolution to the full high-res canvas.
           [//]
           The region returned via `hires_region' lives on the high-res
           canvas of the underlying code-stream and is independent of
           appearance transformations, discarded resolution levels or
           component-specific attributes.  The hi-res region is guaranteed
           to be large enough to cover all samples belonging to the
           intersecton between the supplied component (or resolution) region
           and the region returned via `get_dims'.
         [ARG: comp_idx]
           If negative, the `comp_region' argument defines a region on the
           current image resolution, as it appears after any geometric
           transformations supplied by `change_appearance'.  Otherwise, the
           `comp_region' argument refers to a region on the image component
           indexed by `comp_idx'.  The interpretation of the component index
           is affected by calls to `apply_input_restrictions' as well as
           the `want_output_comps' argument.
           [//]
           If `want_output_comps' is false, this is a codestream
           component index, but its interpretation is affected by any
           restrictions and/or permutations which may have been applied to
           the collection of visible components by previous calls to
           `apply_input_restrictions'.
           [//]
           If `want_output_comps' is true AND the last call to
           `apply_input_restrictions' specified `KDU_WANT_OUTPUT_COMPONENTS',
           or if there has been no call to
           `apply_input_restrictions', then the component index refers to an
           output image component.  The distinction between codestream and
           output image components is particularly important where a
           Part-2 multi-component transform may have been involved.  In any
           case, the distinction is explained in connection with the second
           form of the `apply_input_restrictions' function.  Again, the
           interpretation of `comp_idx' may be influenced by restrictions
           and/or permutations which have been applied to the set of output
           components which are visible across the API (again, this is the
           role of `apply_input_restrictions').
         [ARG: want_output_comps]
           Affects the interpretation of `comp_idx', as explained above, but
           only if there have been no calls to `apply_input_restrictions' or
           the most recent call specified `KDU_WANT_OUTPUT_COMPONENTS'.
      */
  // --------------------------------------------------------------------------
  public: // Member functions used to modify behaviour
    KDU_EXPORT void
      set_textualization(kdu_message *output);
      /* [SYNOPSIS]
           Supplies an output messaging object to which code-stream parameters
           will be textualized as they become available.  Main header
           parameters are written immediately, while tile-specific parameters
           are written at the point when the tile is destroyed.  This function
           is legal only when invoked prior to the first tile access.
           [//]
           Note that the specification of regions of interest without
           persistence (see `apply_input_restrictions' and `set_persistent')
           may prevent some tiles from actually being read -- if these
           contain tile-specific parameters, they may not be textualized.
           [//]
           If `restart' is used to restart the code-stream management
           machinery, you must call the present function again to re-enable
           textualization after each `restart' call.
      */
    KDU_EXPORT void
      set_max_bytes(kdu_long max_bytes, bool simulate_parsing=false,
                    bool allow_periodic_trimming=true);
      /* [SYNOPSIS]
           If used with an `input codestream', this function sets the
           maximum number of bytes which will be read from the input
           code-stream. Additional bytes will be discarded.  In this
           case, the `simulate_parsing' argument may modify the behaviour
           as described in connection with that argument below.
           [//]
           If used with an `interchange codestream', the function does
           nothing at all.
           [//]
           If used with an `output codestream', this function enables
           internal machinery for incrementally estimating the parameters which
           will be used by the PCRD-opt rate allocation algorithm, so that the
           block coder can be given feedback to assist it in minimizing the
           number of coding passes which must be processed -- in many cases,
           most of the coding passes will be discarded.  Note that the actual
           rate allocation performed during a call to the `flush' member
           function is independent of the value supplied here, although it is
           expected that `max_bytes' will be equal to (certainly no smaller
           than) the maximum layer byte count supplied in the `flush' call.
           [//]
           The following cautionary notes should be observed concerning the
           incremental rate control machinery enabled when this function is
           invoked on an `output codestream':
           [>>] The rate control prediction strategy relies upon the
                assumption that the image samples will be processed
                incrementally, with all image components processed
                together at the same relative rate.  It is not at all
                appropriate to process one image component completely,
                followed by another component and so forth.  It such
                a processing order is intended, this function should not
                be called.
           [>>] The prediction strategy may inappropriately discard
                information, thereby harming compression, if the
                compressibility of the first part of the image which is
                processed is very different to that of the last part of
                the image.  Although this is rare in our experience, it
                may be a problem with certain types of imagery, such
                as medical images which sometimes contain empty regions
                near boundaries.
           [>>] If truly lossless compression is desired, this function
                should not be called, no matter how large the supplied
                `max_bytes' value.  This is because terminal coding
                passes which do not lie on the convex hull of the
                rate-distortion curve will be summarily discarded,
                violating the official mandate that a lossless code-stream
                contain all compressed bits.
           [//]
           For both input and output codestreams, the function should only be
           called prior to the point at which the first tile is opened and it
           should only be called once, if at all.  The behaviour is not
           guaranteed at other points, particularly in a multi-threading
           environment.
           [//]
           If `restart' is used to restart the code-stream managment
           machinery, the effect of the current function is lost.  It may
           be restored by calling the function again after each call to
           `restart'.
         [ARG: max_bytes]
           If the supplied limit exceeds `KDU_LONG_MAX'/2, the limit will
           be reduced to that value, to avoid the possibility of overflow
           in the internal implementation.
         [ARG: simulate_parsing]
           The default value of this argument is false.  If set to true, the
           behaviour with non-persistent input code-streams is modified as
           follows.  Rather than simply counting the number of bytes consumed
           from the compressed data source, the function explicitly excludes
           all bytes consumed while parsing packet data which does not belong
           to precincts which contribute to the current spatial region, at
           the current resolution, within the current image components,
           as configured in any call to `apply_input_restrictions'.  It also
           excludes bytes consumed while parsing packets which do not belong
           to the current layers of interest, as configured in any call to
           `apply_input_restrictions'.  The byte limit applies only to those
           bytes which are of interest.  Moreover, in this case, the number
           of bytes returned by the `get_total_bytes' function will also
           exclude the bytes corresponding to packets which are not of
           interest.  If SOP markers have been used, the cost of these may
           still be counted, even for packets which would have been parsed
           away.
         [ARG: allow_periodic_trimming]
           This argument is relevant only for output codestreams.  If true, the
           coded block data will periodically be examined to determine whether
           some premature truncation of the generated coding passes can safely
           take place, without waiting until the entire image has been
           processed.  The purpose of this is to minimize buffering
           requirements.  On the other hand, periodic trimming of existing
           compressed data can require a large number of accesses to
           memory which is often no longer loaded in the cache.
           [//]
           In multi-threaded environments, periodic trimming (if allowed by
           this argument) is performed in a background processing job,
           which can substantially avoid blocking the ongoing processing
           of code-block data in other threads.  Nevertheless, this background
           processing job must lock the `KD_THREADLOCK_GENERAL' mutex which
           will block certain API functions -- most notably,
           `kdu_codestream::open_tile' and `kdu_tile::close'.  Thus, periodic
           trimming may reduce overally CPU utilization when processing tiled
           imagery.  Moreover, in multi-threaded environments, periodic
           trimming will be disabled (regardless of this argument) if
           automatic incremental flushing has been activated via the
           `auto_flush' or `auto_trans_out' functions.
           [//]
           With these things in mind, in multi-threaded environments you
           should consider allowing periodic trimming only for untiled
           imagery where you do not intend to use incremental flushing,
           while for tiled imagery, you should generally use incremental
           flushing instead of periodic trimming, since incremental flushing
           is particularly effective when there are multiple vertical tiles.
           [//]
           Periodic trimming can be resource intensive in any event, so if
           you have plenty of memory, you might do well to avoid the option
           altogether.
      */
    KDU_EXPORT void
      set_min_slope_threshold(kdu_uint16 min_slope);
      /* [SYNOPSIS]
           This function has no impact on input or interchange codestreams.
           When applied to an `output codestream', the function has a
           similar effect to `set_max_bytes', except that it supplies a limit
           on the distortion-length slope threshold which will be used by the
           rate control algorithm.  This is most effective if the `flush'
           member function is to be called with a set of slope thresholds,
           instead of layer size specifications.
           [//]
           Although the intent is that `min_slope' should actually be the
           minimum distortion-length slope threshold which will be used to
           generate any quality layer, the block encoder must be somewhat
           conservative in determining a safe stopping point.  For this reason,
           it will usually generate additional data, having distortion-length
           slopes smaller than the supplied bound.  In particular, the
           current implementation of the block encoder produces 2 extra
           coding passes beyond the point where the supplied slope bound
           is passed.  This data is not discarded until the final rate
           allocation step, so in some applications it may be safe to
           specify a fairly aggressive value for `min_slope'.
           [//]
           If `set_max_bytes' is used, the slope threshold information will
           be incrementally estimated from statistics of the compressed data
           as it appears.
           [//]
           If `set_max_bytes' and `set_min_slope_threshold' are both used
           together, the larger (most constraining) of the two thresholds
           will be used by the block encoder to minimize its coding efforts.
           [//]
           Unlike `set_max_bytes', the effects of the present function are
           preserved across multiple calls to `restart'.
      */
    KDU_EXPORT void
      set_resilient(bool expect_ubiquitous_sops=false);
      /* [SYNOPSIS]
           This function may be called at any time to modify the way
           input errors are treated.  The function has no impact on output
           or interchange codestreams.  The modes established by
           `set_resilient', `set_fussy' and `set_fast' are mutually exclusive.
           [//]
           In the resilient mode, the code-stream management machinery makes
           a serious attempt to both detect and recover from errors in the
           code-stream.  We attempt to guarantee that decompression will not
           fail, so long as the main header is uncorrupted and there is only
           one tile with one tile-part.
         [ARG: expect_ubiquitous_sops]
           Indicating whether or not the decompressor should expect SOP
           marker segments to appear in front of every packet whenever
           the relevant flag in the Scod byte of the COD marker segment is
           set.  According to the JPEG2000 standard, SOP markers need not
           appear in front of every packet when this flag is set; however,
           this weakens error resilience, since we cannot predict when an SOP
           marker should appear.  If you know that the code-stream has been
           constructed to place SOP markers in front of every packet (or not
           use them at all), then set `expect_ubiquitous_sops' to true,
           thereby allowing the error resilient code-stream parsing algorithm
           to do a better job.
      */
    KDU_EXPORT void
      set_fussy();
      /* [SYNOPSIS]
           This function may be called at any time to modify the way
           input errors are treated.  The function has no impact on output
           or interchange codestreams.  The modes established by
           `set_resilient', `set_fussy' and `set_fast' are mutually exclusive.
           [//]
           In the fussy mode, the code-stream management machinery makes
           a serious attempt to identify compliance violations and generates
           an appropriate terminal error message if it finds one.
      */
    KDU_EXPORT void
      set_fast();
      /* [SYNOPSIS]
           This function may be called at any time to modify the way
           input errors are treated.  The functions has no impact on output
           or interchange codestreams.  The modes established by
           `set_resilient', `set_fussy' and `set_fast' are mutually exclusive.
           [//]
           The fast mode is used by default.  In this case, compliance
           is assumed and checking is minimized; there is no attempt to
           recover from any errors -- for that, use `set_resilient'.
      */
    KDU_EXPORT void
      apply_input_restrictions(int first_component, int max_components,
                               int discard_levels, int max_layers,
                               kdu_dims *region_of_interest,
                               kdu_component_access_mode access_mode =
                                   KDU_WANT_CODESTREAM_COMPONENTS);
      /* [SYNOPSIS]
           This function may be used only with `input codestreams'
           (i.e., `kdu_codestream' created for reading from a compressed data
           source) or `interchange codestreams' (`kdu_codestream' created
           without either a compressed source or a compressed target).
           It restricts the elements of the compressed image
           representation which will appear to be present.  Since the
           function has an impact on the dimensions returned by other member
           functions, these dimensions may need to be re-acquired afterwards.
           The role of this function is closely related to that of the
           `change_appearance' member function; however, the latter function
           may be applied to `output codestreams' as well.
           [//]
           The function may be invoked multiple times to alter the region
           of interest, resolution, image components or number of quality
           layers visible to the user.  However, after the first tile is
           opened, the function may be applied only to interchange
           codestreams or input codestreams which are set up in the
           persistent mode (see `set_persistent').  This is because
           non-persistent input codestreams discard compressed data and
           all associated structural elements (tiles, subbands, etc.) once
           we know that they will no longer be used -- this information is
           based on the input restrictions themselves.  Even in the
           persistent case, you may not call this function while any tile
           is open.
           [//]
           Zero valued arguments are always interpreted as meaning that any
           restrictions should be removed from the relevant parameter.
           [//]
           There are two forms to the overloaded `apply_input_restrictions'
           function.  The second form provides significantly more flexibility
           in specifying the collection of image components which are of
           interest.  For both forms of the function, however, it is possible
           to identify whether the image components which are of interest
           belong to the class known as "output image components" or the
           class known as "codestream image components".  This is determined
           by the `access_mode' argument which takes one of the following
           two values:
           [>>] `KDU_WANT_CODESTREAM_COMPONENTS' -- If this access
                mode is selected, the collection of visible codestream
                image components is determined by the `first_component' and
                `max_components' arguments (if 0, no restrictions apply).  In
                this case, however, the codestream image components will be
                treated as though they are also output image components, in
                calls to functions such as `get_dims', `get_subsampling' and
                so forth; information about any multi-component transforms
                will be entirely concealed.  This mode is consistent with the
                services offered by Kakadu in versions prior to v5.0, but not
                useful for more general applications which may need to
                correctly handle Part-2 multi-component transforms.  It is set
                as the default value for the `access_mode' argument to this
                function so as to ensure backward compatibility.
           [>>] `KDU_WANT_OUTPUT_COMPONENTS' -- If this access mode
                is selected, the `first_component' and `max_components'
                arguments serve (if non-zero) to restrict the
                collection of output components, as found after the
                application of any colour transform or inverse multi-component
                transform.  In this case, all original codestream components
                will be visible, in the sense that calls to functions such as
                `get_dims', `get_bit_depth', `get_signed' and so forth return
                information for all original codestream components.  Also,
                if these component information functions are accessed with
                the optional `want_output_comps' argument set to
                true, they will return information associated with the
                (possibly restricted and/or permuted) output image components.
                Even though all original codestream image components are
                visible, the `kdu_tile::access_component' function will
                return an empty interface if you attempt to access any
                tile-component which is not actually involved in the
                reconstruction of any visible output image component
                within that tile.  These components are discarded immediately
                during codestream parsing, if the non-persistent input mode is
                being used (see `set_persistent').  As mentioned above, it can
                happen that some tile-components are not used in the
                reconstruction of any output image component at all,
                regardless of whether restrictions are imposed.  These
                tile-components will never be accessible in the
                `KDU_WANT_OUTPUT_COMPONENTS' mode.  If there is no Part-2
                multi-component transform, there is a 1-1 correspondence
                between output image components and codestream image
                components, except possibly where the Part-1 reversible or
                irreversible colour transform (RCT or ICT) is used (this may
                vary from tile to tile), in which case the first three
                codestream image components will always be accessible, if
                any of the first three output components is made visible.
         [ARG: first_component]
           Identifies the index (starting from 0) of the first component to be
           presented to the user.  If `access_mode' is equal to
           `KDU_WANT_CODESTREAM_COMPONENTS', this is the first apparent
           codestream component and it will also appear to be the first
           apparent output component, since output and codestream components
           become equivalent in this mode.  If `access_mode' is equal to
           `KDU_WANT_OUTPUT_COMPONENTS', this is the first apparent output
           component, while all codestream components will be apparent.
         [ARG: max_components]
           Identifies the maximum number of codestream image components
           which will appear to be present, starting from the component
           identified by the `first_component' argument.  If `max_components'
           is 0, all remaining components will appear.  As with
           `first_component', the interpretation of the components which
           are being restricted depends upon the `access_mode' argument.
         [ARG: discard_levels]
           Indicates the number of highest resolution levels which will
           not appear to the user.  Image dimensions are essentially divided
           by 2 to the power of this number.  This argument affects the
           apparent dimensions and number of DWT levels in each tile-component.
           Note, however, that neither this nor any other argument has any
           effect on the parameter sub-system, as accessed through the
           `kdu_params' object (actually the head of a multi-dimensional
           list of parameter objects) returned by the `access_siz'
           function.
           [//]
           If `discard_levels' exceeds the number of DWT levels in some
           tile-component, it will still be possible to open the relevant
           tile successfully; however, any attempt to access the problem
           tile-component's resolutions (`kdu_tile_comp::access_resolution')
           will cause an error message to be generated through `kdu_error'.
           The minimum number of DWT levels across all tile-components is
           incrementally determined as tiles are opened; the current value is
           reported via the `get_min_dwt_levels' function.
         [ARG: max_layers]
           Identifies the maximum number of quality layers which will appear
           to be present when precincts or code-blocks are accessed.  A value
           of 0 has the interpretation that all layers should be retained.
         [ARG: region_of_interest]
           If NULL, any region restrictions are removed.  Otherwise, provides
           a region of interest on the high resolution grid.  Any
           attempt to access tiles or other subordinate partitions which do
           not intersect with this region will result in an error being
           generated through `kdu_error'.  Note that the region may
           be larger than the actual image region.  Also, the region must be
           described in terms of the original code-stream geometry.
           Specifically any appearance transformations supplied by the
           `change_appearance' member function have no impact on the
           interpretation of the region.  You may find the `map_region'
           member function useful in creating suitable regions.
         [ARG: access_mode]
           Takes one of the values `KDU_WANT_CODESTREAM_COMPONENTS'
           or `KDU_WANT_OUTPUT_COMPONENTS', as explained above.
      */
    KDU_EXPORT void
      apply_input_restrictions(int num_indices,
                               int *component_indices,
                               int discard_levels, int max_layers,
                               kdu_dims *region_of_interest,
                               kdu_component_access_mode access_mode);
      /* [SYNOPSIS]
           This second form of the overloaded `apply_input_restrictions'
           function is similar to the first, except that it provides
           more flexibility with the way image components are handled.
         [ARG: num_indices]
           Number of entries in the `component_indices' array.  These
           indices do not all need to be unique, although that will normally
           be the case.
         [ARG: component_indices]
           Array with `num_indices' entries, providing the indices of
           the components which are to be visible.  The set of image components
           indices identified by this argument corresponds either to
           codestream image components or to output image components, depending
           on the value of the `access_mode' argument.
           [//]
           The visible components (codestream or output components, as
           appropriate) will appear in the same order that the indices appear
           in this array.  Thus, if this array contains two component indices,
           3 and 0, the first apparent component will be the true 4'th
           original component, while the second apparent component will be the
           true 1'st original component.  This means that the
           `component_indices' argument may be used to effect permutations.
           [//]
           If the `component_indices' array contains repeated indices, the
           repeats are simply ignored, as if they had been removed from
           the array, compacting it down to a smaller size.  Thus, if the
           `component_indices' array contains entries (0,2,0,1), it will
           be treated as if `num_indices' were 3 and the `component_indices'
           array had contained entries (0,2,1).
         [ARG: access_mode]
           Takes one of the values `KDU_WANT_CODESTREAM_COMPONENTS'
           or `KDU_WANT_OUTPUT_COMPONENTS', as explained in connection with
           the first form of this function.
         [ARG: discard_levels]
           See the first form of the `apply_input_restrictions' function.
         [ARG: max_layers]
           See the first form of the `apply_input_restrictions' function.
         [ARG: region_of_interest]
           See the first form of the `apply_input_restrictions' function.
      */
    KDU_EXPORT void
      change_appearance(bool transpose, bool vflip, bool hflip);
      /* [SYNOPSIS]
           This function alters the apparent orientation of the image,
           affecting the apparent dimensions and regions indicated by all
           subordinate objects and interfaces.  Multiple calls are permitted
           and the functionality is supported for both input and output
           codestreams.  Except in the case of an input codestream marked for
           persistence (see `set_persistent'), the function may not be called
           after the first tile access.  Even in the case of a persistent
           input codestreams, the function may not be called while any tile is
           open.
           [//]
           Note that this function has no impact on `kdu_params' objects in
           the list returned by the `access_siz' function, since those
           describe the actual underlying code-stream.
           [//]
           Note carefully that some packet wavelet decomposition structures
           which can be used by JPEG2000 Part-2, are incompatible with
           flipping.  This does not mean that an error will be generated
           if you invoke the present function on such codestreams.  However,
           if your appearance changes involve flipping and the codestream
           turns out to be incompatible with flipping, an error will be
           generated during calls to `kdu_tile_comp::access_resolution'.
           To learn more about this behaviour, and how to check for
           flipping problems, consult the `can_flip' function.
         [ARG: transpose]
           Causes vertical coordinates to be transposed with horizontal.
         [ARG: vflip]
           Flips the image vertically, so that vertical coordinates start
           from the bottom of the image/canvas and work upward.  If `transpose'
           and `vflip' are both true, vertical coordinates start
           from the right and work toward the left of the true underlying
           representation.
         [ARG: hflip]
           If true, individual image component lines (and all related
           quantities) are flipped.  If `transpose' is also true, these
           lines correspond to columns of the true underlying representation.
      */
    KDU_EXPORT void
      set_block_truncation(kdu_int32 factor);
      /* [SYNOPSIS]
           This function may be called at any time to modify the amount of
           compressed data which is actually passed across the
           `kdu_subband::open_block' interface.  Probably the only reason
           you would want to do this is to decrease the amount of CPU time
           spent on the critical code-block decoding phase during image/video
           decompression, by stripping away coding passes in a controlled
           fashion.  Both increases and decreases in the block truncation
           factor can be respected by future calls to
           `kdu_subband::open_block', so as to maximize the responsiveness
           of a decompression system to computational load adjustments in
           critical applications.  The function is inherently thread safe.
           [//]
           Note that the internal record of the block truncation factor is
           reset to 0 by calls to `create' and `restart'.
         [ARG: factor]
           If less than or equal to 0, truncation is disabled,
           so that future calls to `kdu_subband::open_block' will return all
           available code-block data.
           [//]
           For values greater than zero, the internal algorithm used to
           selectively trim coding passes from the content returned by
           `kdu_subband::open_block' is subject to change.  As a rough guide,
           however, each increment of 256 in the value of `factor' corresponds
           to the removal of one coding pass from all code-blocks.
           Intermediate values may remove coding passes from only some
           code-blocks, in accordance with a reasonable policy.  In
           constrained environments, you should be able to find a roughly
           linear relationship between the value of `factor' and the amount
           of CPU time saved during decompression.  Indeed, one objective
           of any future modifications to the internal algorithm is that
           it should lend itself to such modeling.
      */
  // --------------------------------------------------------------------------
  public: // Data processing/access functions
    KDU_EXPORT kdu_tile
      open_tile(kdu_coords tile_idx, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Accesses a specific tile, returning an interface which can be
           used for accessing the tile's contents and further interfaces to
           specific tile-components.  The returned interface contains no
           state of its own, but only an embedded reference to the real state
           information, which is buried in the implementation.  When a tile is
           first opened, the internal state information may be automatically
           created from the code-stream parameters available at that time.
           These in turn may be translated automatically from marker segments
           if this is an input codestream.  Indeed, for this to happen, any
           amount of the code-stream may need to be read and buffered
           internally, depending upon whether or not the sufficient
           information is available to support random access into the
           code-stream.
           [//]
           For these reasons, you should be careful only to open tiles
           which you really want to read from or write to.  Instead of walking
           through tiles to find one you are interested in, use the
           `find_tile' member function.  You may also find the
           `get_tile_dims' function useful.
           [//]
           With input codestreams, the function attempts to read
           and buffer the minimum amount of the code-stream in order to
           satisfy the request.  Although not mandatory, it is a good idea to
           close all tiles (see `kdu_tile::close') once you are done with
           them.
           [//]
           If you are generating an output codestream, you should not call
           this function prior to completing the configuration of all
           codestream parameters and invoking `kdu_params::finalize_all'
           on the pointer returned via `access_siz'.
         [ARG: tile_idx]
           The tile indices must lie within the region returned
           by the `get_valid_tiles' function; otherwise, an error will be
           generated.
         [ARG: env]
           This argument is provided to enable safe asynchronous opening of
           tiles by multiple threads.  If this is the first time a non-NULL
           `kdu_thread_env' reference is passed to one of the codestream
           interface functions, the internal machinery required to handle
           multi-threaded processing will be constructed on the fly.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT void
      create_tile(kdu_coords tile_idx, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Similar to opening and then closing a tile immediately, but with
           less overhead.  This can be useful for input codestreams because
           it causes the tile's first tile-part header to be read and parsed
           for any marker segments which may be specific to the tile.  After
           this you can access the relevant information within the parameter
           sub-system (accessible via `access_siz').  It is safe to call this
           function on a tile which is already created (even one which is
           already open).  Even though the function does generally instantiate
           resources, those resources are usually internally unloadable,
           meaning that creating/opening another tile may cause this tile's
           resources to be unloaded from memory.
      */
    KDU_EXPORT kdu_codestream_comment
      get_comment(kdu_codestream_comment prev=kdu_codestream_comment());
      /* [SYNOPSIS]
           Returns an interface to the text of the next ASCII comment marker
           segment for the main header, following that referenced by the
           `prev' interface.  If `prev' is an empty interface, the function
           returns an interface to the very first AXCII comment in the
           main header.  If no comment exists, conforming to the above
           rules, the function returns an empty interface.
      */
    KDU_EXPORT kdu_codestream_comment add_comment();
      /* [SYNOPSIS]
           Returns an interface to a new comment, to which text may be written.
           Returns an empty interface unless the code-stream management
           machinery was created for output or for interchange, and `flush'
           has not yet been called.
           [//]
           This function is not currently thread-safe.
      */
    KDU_EXPORT void
      flush(kdu_long *layer_bytes, int num_layer_specs,
            kdu_uint16 *layer_thresholds=NULL, bool trim_to_rate=true,
            bool record_in_comseg=true, double tolerance=0.0,
            kdu_thread_env *env=NULL, int flags=0);
      /* [SYNOPSIS]
           In most cases, this is the function you use to actually write out
           code-stream data once the subband data has been generated and
           encoded.  The function may be invoked multiple times, in order
           to generate the code-stream incrementally.  This allows you to
           interleave image data transformation and encoding steps with
           code-stream flushing, avoiding the need to buffer all
           compressed data in memory.
           [//]
           The function provides two modes of operation, as follows:
           [>>] Firstly, it may be used to generate quality layers whose
                compressed sizes conform to the limits identified in a
                supplied `layer_bytes' array.  This mode of operation is
                employed if the `layer_thresholds' argument is NULL, if the
                first entry in a non-NULL `layer_thresholds' array is 0, or
                if the `KDU_FLUSH_THRESHOLDS_ARE_HINTS' flag is present in
                the `flags' argument.  In this mode of operation, the     
                function selects the smallest distortion-length slope
                threshold which is consistent with the non-zero layer sizes.
                Note that the k'th entry in the `layer_bytes' array holds the
                maximum cumulative size of the first k quality layers,
                including all code-stream headers, excepting only the size of
                optional pointer marker segments (e.g., PLT and TLM marker
                segments for recording packet and/or tile lengths).  As
                explained below, in this mode of operation, if the
                `layer_thresholds' array contains one or more zero-valued
                entries, the function effectively synthesizes appropriate
                values.
           [>>] The function's second mode of operation is triggered by the
                presence of a non-NULL `layer_thresholds' array, whose first
                entry is non-zero, unless the `KDU_FLUSH_THRESHOLDS_ARE_HINTS'
                flag is present.  In this mode, the function generates
                quality layers based on the distortion-length slope thresholds
                supplied in this array.  The actual slope thresholds recorded
                in the `layer_thresholds' array are 16-bit unsigned integers,
                whose values are interpreted as
                    256*(256-64) + 256*log_2(delta_D/delta_L),
                where delta_D is the total squared error distortion (possibly
                weighted according to the frequency weights supplied via the
                `Cband_weights', `Clev_weights' and `Cweight' attributes)
                associated with an increase of delta_L in the byte count.  It
                should be noted that distortion is assessed with respect to
                normalized sample values, having a nominal range from -128 to
                +128, no matter what the original image bit-depth might have
                been -- previous versions of Kakadu erroneously reported the
                nominal bit-depth used to assess delta_D here as -0.5 to +0.5,
                but the actual slope threshold values have not changed.  For
                the purpose of determining appropriate `layer_thresholds'
                values, we point out that a threshold of 0 yields the largest
                possible output size, i.e., all bytes are included by the end
                of that layer.  A slope threshold of 0xFFFF yields the
                smallest possible output size, i.e., no code-block
                contributions are included in the layer.
           [>>] The function's second mode of operation may be enhanced by
                inclusion of the `KDU_FLUSH_USES_THRESHOLDS_AND_SIZES'
                flag -- see the `flags' argument.  In this case, both the
                `layer_thresholds' and `layer_bytes' values are used.
                Specifically, `layer_thresholds' supplies distortion-length
                slope thresholds that govern the amount of data included
                from each code-block into each quality layer, except that
                these thresholds are automatically reduced (lower threshold
                means higher quality) to ensure that the cumulative size
                of the first k quality layers is at least as large as the
                largest of the first k entries in the `layer_bytes' array,
                to the extent that this is possible given the amount of
                encoded data.  In this modified second mode, the `layer_bytes'
                array provides lower bounds for the amount of data included
                in the quality layers, as opposed to upper bounds.  These
                lower bounds would normally be significantly smaller than
                the size that is expected to be produced based on the
                `layer_thresholds' alone, except if the content turns out
                to be unusually compressible.  If `set_min_slope_threshold'
                was used to limit the number of coding passes processed
                during compression, the effectiveness of the lower bounds
                expressed in the `layer_bytes' array will be limited by
                the value that was passed to that function, so you might
                want to consider passing a smaller minimum slope bound than
                the smallest distortion-length slope threshold you intend
                to supply via the `layer_thresholds' array here.
           [>>] The function's first mode of operation may be enhanced by
                inclusion of the `KDU_FLUSH_THRESHOLDS_ARE_HINTS' flag --
                see `flags'.  In this case, the entries of the
                `layer_thresholds' array are treated as initial suggestions
                for the rate control algorithm to try, while endeavouring
                to satisy the size targets found in the `layer_bytes' array.
                These hints can be used in a video compression application,
                supplying the distortion-length slope thresholds found for
                one frame as hints for a subsequent frame -- supplying good
                hints enhance the computational efficiency of the rate
                control process.
           [//]
           It should be noted that regardless of which mode of operation
           is selected, the quality layers generated by this function will
           respect the upper bounds imposed by any `Creslengths' parameter
           attributes that have been specified -- see `cod_params' for a
           discussion of the interpretation of the `Creslengths' attribute.
           However, `Creslengths' attributes do not directly constrain the
           lengths of the quality layers whose `layer_bytes' entries are
           zero, in the function's first mode of operation.
           [//]
           Normally, `num_layer_specs' should be identical to the actual
           number of quality layers to be generated.  In this case, every
           non-zero entry in a `layer_bytes' array identifies the target
           maximum number of bytes for the corresponding quality layer and
           every non-zero entry in a `layer_thresholds' array identifies a
           distortion-length slope threshold for the corresponding layer.
           [//]
           It can happen, however, that individual tiles have fewer quality
           layers.  In this case, these tiles participate only in the rate
           allocation associated with the initial quality layers and they
           make no contribution to the later (higher quality) layers.  If no
           tiles have `num_layer_specs' quality layers, the code-stream size
           will be determined entirely by the highest `layer_bytes' or
           `layer_thresholds' entry for which at least one tile has a quality
           layer.
           [//]
           It can also happen that individual tiles have more quality layers
           than the number of layer specs provided here.  Packets associated
           with all such layers will be written with the "empty header bit"
           set to 0 -- they will thus have the minimum 1 byte representation.
           The function makes an effort to account for these useless packet
           bytes, so as to guarantee that the complete code-stream does not
           exceed the size specified in a final layer spec.
           [//]
           In the function's first mode of operation (see above), the
           `layer_bytes' array is used to size the quality layers.  Zero
           valued entries in this array mean that the rate allocator
           should attempt to assign roughly logarithmically spaced
           cumulative sizes to those quality layers.  From KDU7.2, this is
           accomplished by assigning slope thresholds to these layers that
           are uniformly spaced.  As explained above, slope thresholds
           are expressed in a logarithmic domain; in practice, this typically
           yields quality layers whose sizes are also roughly logarithmically
           spaced.  Prior to KDU7.2, the unsized layers were explicitly
           assigned a size which was then used in the post-compressed
           rate-distortion optimization algorithm, but can involve an
           unnecessarily large amount of computation when many layers are
           involved.
           [//]
           Any or all of the entries in the `layer_bytes' array may be 0.  If
           the last entry is 0, all generated bits will be output by
           the time the last quality layer is encountered.  In this case, the
           second last layer, if it exists and has no specified size, is
           assigned an explicit target size based on the logarithmic
           cumulative size rule mentioned above.
           [//]
           If one or more initial entries in the `layer_bytes' array are 0,
           the function assigns logarithmic slope thresholds to these layers
           which are separated by 300.  In practice, this simple policy
           usually yields quality layers whose cumulative sizes are
           separated from one another by a factor of approximately sqrt(2).
           While this might not appear to be the case at very low qualities,
           you should find that after discarding resolution levels to the
           point where the quality is appropriate for that resolution, the
           relevant quality layers do roughly satisfy the sqrt(2) rule.  In
           effect, the constant separation of quality layer slope thresholds
           produces a set of quality layers that are well suited to
           multi-resolution interactive viewing applications.
           [//]
           If both the `layer_bytes' and the `layer_thresholds' arguments are
           NULL, the function behaves as though `layer_bytes' pointed to an
           array having all zero entries, so that the layer size allocation
           policy described above will be employed.
           [//]
           As noted at the beginning of this description, the function also
           supports incremental flushing of the code-stream.  What this means
           is that you can generate some of the compressed data, pushing the
           code-block bit-streams into the code-stream management machinery
           using the `kdu_subband::open_block' and `kdu_subband::close_block'
           interfaces, and then call `flush' to flush as much of the generated
           data to the attached `kdu_compressed_target' object as possible.
           You can then continue generating coded block bit-streams, calling
           the `flush' function every so often.  This behaviour allows you
           to minimize the amount of coded data which must be stored
           internally.  While the idea is quite straightforward, there are
           quite a number of important factors which you should take into
           account if incremental flushing is to be beneficial to your
           application:
           [>>] Most significantly, for the Post-Compression-Rate-Distortion
                (PCRD) optimization algorithm to work reliably, a large amount
                of compressed data must be available for sizing the various
                quality layers prior to code-stream generation.  This means
                that you should generally call the `flush' function as
                infrequently as possible.  In a typical application, you
                should process at least 500 lines of image data between
                calls to this function and maybe quite a bit more, depending
                upon the code-block dimensions you are using.  If you are
                supplying explicit distortion-length slope `layer_thresholds'
                to the function rather than asking for them to be generated
                by the PCRD optimization algorithm, you may call the `flush'
                function more often, to reduce the amount of compressed data
                which must be buffered internally.  Note, however, that the
                remaining considerations below still apply.  In particular,
                the tile-part restriction (no more than 255 tile-parts
                per tile) may still force you to call the function as
                infrequently as possible and the dimensions of the low
                resolution precincts may render frequent calls to this
                function useless, although both of these constraints go away
                if the compressed data target is a structured cache
                (see `kdu_compressed_target::get_capabilities').
           [>>] Prior to Kakadu version 7.0, it was particularly
                important to call `ready_for_flush' prior to calling this
                function.  Now, this is no longer critical, because the
                function effectively invokes `ready_for_flush' internally.
                In any event if `ready_for_flush' (or an internal equivalent)
                indicates that no new data can be written to the code-stream
                yet, the function does not generate anything (not even empty
                tile-parts, as it used to do prior to v7.0).  If the
                compressed data target is a sequential codestream (i.e.,
                not a structured cache), the ability
                to generate new code-stream data is affected by the packet
                progression sequence, as well as the precinct dimensions,
                particularly those of the lower resolution levels.  For
                effective incremental flushing, you should employ a packet
                progression order which sequences all of the packets for
                each precinct consecutively.  This means that you should use
                one of PCRL, CPRL or RPCL.  In practice, only the first two
                are likely to be of much use, since the packet progression
                should also reflect the order in which coded data becomes
                available.  In most applications, it is best to process the
                image components in an interleaved fashion so as to allow
                safe utilization of the `max_bytes' function, which limits
                the number of useless coding passes which are generated by
                the block encoder.  For this reason, the PCRL packet
                progression order will generally be the one you want.
                Moreover, it is important that low resolution precincts have
                as little height as possible, since they typically represent
                the key bottlenecks preventing smooth incremental flushing
                of the code-stream.  As an example, suppose you wish to
                flush a large image every 700 lines or so, and that 7 levels
                of DWT are employed.  In this case, the height of the lowest
                resolution's precincts (these represent the LL subband) should
                be around 4 samples.  The precinct height in each successive
                resolution level can double, although you should often select
                somewhat smaller precincts than this rule would suggest for
                the highest resolution levels.  For example, you might use
                the following parameter values for the `Cprecincts' attribute:
                "{128,256},{128,128},{64,64},{32,64},{16,64}, {8,64},{4,32}".
           [>>] If the image has been vertically tiled, you would do best to
                invoke this function after generating compressed data for a
                whole number of tiles.  In this case, careful construction of
                vertical precinct dimensions, as suggested above, is not
                necessary, since the tiles will automatically induce an
                hierarchically descending set of precinct dimensions.
           [>>] You should be aware of the fact that each call to
                this function (apart from one issued at a point when
                `ready_for_flush' would return false) adds a new tile-part
                to each tile whose compressed data has not yet been completely
                written. The standard imposes a limit of at most 255
                tile-parts for any tile, so if the image is not tiled, you
                must call this function no more than 255 times.
           [>>] If your compressed data target is a structured cache
                (see `kdu_compressed_target::get_capabilities'), you can
                ignore most of the above constraints/suggestions, because
                precincts can be written in any order at all.  In this case,
                the `Corder' attribute is irrelevant and tiles are not
                divided into multiple tile-parts.  You do still need to
                set up the `Cprecincts' attribute, but you do not need to
                force the lowest resolution precincts to be very small.  A
                reasonable configuration, for a flush period of 1024, would
                be "Cprecincts={256,256},{128,256},{64,256}", which can be
                used with an arbitrarily large value of `Clevels'.
           [>>] You can arrange for this function to be automatically
                invoked at regular intervals (determined by internally
                meaningful trigger-points) by using the `auto_flush'
                function.  This is highly recommended, especially for
                multi-threading environments.
           [//]
           We conclude this introductory discussion by noting the way in
           which values are taken in from and returned via any non-NULL
           `layer_bytes' and `layer_thresholds' arrays when the function is
           invoked one or more times.  When the function is first invoked,
           the mode of operation is determined to be either size driven
           (first mode of operation), or threshold driven (second mode of
           operation), depending upon whether or not a `layer_thresholds'
           array is supplied having a non-zero first entry and whether or
           not the `KDU_FLUSH_THRESHOLDS_ARE_HINS' flag is present.  If the
           size driven mode of operation is in force, the target layer sizes
           are copied to an internal array and the same overall layer size
           targets are used for this and all subsequent calls to the `flush'
           function until the code-stream has been entirely flushed; it makes
           no difference what values are supplied in the `layer_bytes' array
           for subsequent `flush' calls in an incremental flushing sequence.
           If the threshold-driven mode is in force, the supplied
           `layer_thresholds' values copies to an internal array and the
           same mode is employed for all future calls to this function until
           the code-stream has been completely flushed. However, in this case,
           if subsequent calls to this function provide a non-NULL
           `layer_thresholds' array, the supplied thresholds will replace
           those stored in the internal array.  This allows the application
           to implement its own rate control loop, adapting the slope
           thresholds between incremental `flush' calls so as to achieve some
           objective.  If the `KDU_FLUSH_USES_THRESHOLDS_AND_SIZES' flag is
           present in the first call to the `flush' function, and the
           threshold-driven mode of operation is in force, both the
           `layer_thresholds' and the `layer_bytes' arrays are copied
           internally, so as to preserve the layer size lower bounds, and
           only the thresholds may be changed by subsequent calls to the
           `flush' function.
           [//]
           Regardless of the mode of operation, whenever this function
           returns, it copies into any non-NULL `layer_bytes' array, the total
           number of bytes which have actually been written out to the
           code-stream so far for each specified layer.  Similarly, whenever
           the function returns, it copies into any non-NULL `layer_thresholds'
           array, the actual slope threshold used when generating code-stream
           data for each layer during this call.
           [//]
           Note that internal state information is transferred to a non-NULL
           `layer_bytes' array and/or a non-NULL `layer_thresholds' array,
           even if the function does not actually write any new codestream
           content (i.e., even if `ready_for_flush' would have returned
           false).
         [ARG: num_layer_specs]
           Number of elements in any `layer_bytes' or `layer_thresholds'
           arrays.  If the function is invoked multiple times to incrementally
           flush the code-stream, each call must provide exactly the same
           number of layer specifications.
         [ARG: layer_bytes]
           Array containing `num_layer_specs' elements to be used either
           for returning or for explicitly setting the cumulative number of
           bytes associated with the first k quality layers, for each k
           from 1 to `num_layer_specs'.  This argument may be NULL, in which
           case it will be treated as though a `layer_bytes' array were
           supplied with all zero entries.  See above for a detailed discussion
           of how the function synthesizes target specifications for layers
           whose `layer_bytes' entry is 0.  Also, see above for an explanation
           of how the special flag `KDU_FLUSH_USES_THRESHOLDS_AND_SIZES'
           modifies the interpretation of the layer sizes supplied via a
           `layer_bytes' array.
         [ARG: layer_thresholds]
           If non-NULL, must point to an array with `num_layer_specs'
           entries, to be used either for returning or for explicitly
           setting distortion-length slope thresholds for the quality layers.
           Determination as to whether or not the `layer_thresholds' array
           should be used to explicitly set thresholds for the quality layers
           is based upon whether or not the first element in the array holds
           0 upon entry, when the function is first called, as described
           more thoroughly above.
         [ARG: trim_to_rate]
           This argument is ignored if slope thresholds are being used to
           control layer formation, instead of target layer sizes, or if this
           is not the final call to the `flush' function (as already noted, the
           function may be called multiple times before the code-stream is
           completely flushed out).
           [//]
           By default, the rate allocation logic performs an additional
           trimming step when constructing the final (highest rate) quality
           layer.  In this trimming step, the distortion-length slope
           threshold is selected so as to just exceed the maximum target size
           and the allocator then works back through the code-blocks, starting
           from the highest resolution ones, trimming the extra code-bytes
           which would be included by this excessively small slope threshold,
           until the rate target is satisfied.
           [//]
           If `trim_to_rate' is set to false, the last layer will be treated in
           exactly the same way as all the other layers.  This can be useful
           for several reasons: 1) it can improve the execution speed
           slightly; 2) it ensures that the final distortion-length slopes
           which are returned via a non-NULL `layer_thresholds' array can be
           used in a subsequent compression step (e.g., compressing the same
           image or a similar image again) without any unexpected surprises.
           [//]
           If the final quality layer's compressed size is limited by a
           `Creslengths' constraint, the `trim_to_rate' functionality is
           automatically disabled so as to avoid accidentally violating
           the constraint.
        [ARG: record_in_comseg]
           If true, the rate-distortion slope and the target number of bytes
           associated with each quality layer will be recorded in a COM
           (comment) marker segment in the main code-stream header.  This
           can be very useful for applications which wish to process the
           code-stream later in a manner which depends upon the interpretation
           of the quality layers.  For this reason, you should generally
           set this argument to true, unless you want to get the smallest
           possible file size when compressing small images.  If the function
           is called multiple times to effect incremental code-stream flushing,
           the parameters recorded in the COM marker segment will be
           extrapolated from the information available when the `flush'
           function is first called.  The information in this comment is thus
           generally to be taken more as indicative than absolutely accurate.
        [ARG: tolerance]
           This argument is ignored unless quality layer generation is being
           driven by layer sizes, supplied via the `layer_bytes' array.  In
           this case, it may be used to trade accuracy for speed when
           determining the distortion-length slopes which achieve the target
           layer sizes as closely as possible.  In particular, the algorithm
           will finish once it has found a distortion-length slope which
           yields a size in the range target*(1-tolerance) <= size <= target,
           where target is the target size for the relevant layer.  If no
           such slope can be found, the layer is assigned a slope such that
           the size is as close as possible to target, without exceeding it.
         [ARG: env]
           This argument is provided to allow safe incremental flushing of
           codestream data while processing continues on other threads.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
         [ARG: flags]
           Additional flags that may control the behaviour of this function.
           Currently the only flags that are defined are:
           [>>] `KDU_FLUSH_THRESHOLDS_ARE_HINTS' -- this flag causes any
                non-zero slope thresholds supplied via a non-NULL
                `layer_thresholds' array to be re-interpreted as "hints"
                to the rate-control machinery, as to good values to try
                first when hunting for distortion-length slope thresholds
                that will satisfy the cumulative target compressed sizes
                supplied via the `layer_bytes' array.  If `layer_bytes' is
                NULL or contains all zeros, the `layer_thresholds' hints
                are not relevant.  Video compression applications provide
                good opportunities for this hinting process, since the
                `layer_thresholds' discovered during the compression of one
                frame may be used as hints for a subsequent frame.
           [>>] `KDU_FLUSH_USES_THRESHOLDS_AND_SIZES' -- this flag is ignored
                unless this is the first call to `flush', with `layer_bytes'
                and `layer_thresholds' both non-NULL, the first entry of
                the `layer_thressholds' array is non-zero, and the
                `KDU_FLUSH_THRESHOLDS_ARE_HINTS' flag is not present.
                Under these conditions, the function's second mode of
                operation is in effect and the values passed via the
                `layer_bytes' array are interpreted as lower bounds on the
                sizes of the generated quality layers.  Specifically, in this
                case `layer_bytes'[k] is interpreted as a lower bound on the
                cumulative number of bytes associated with the first k quality
                layers, meaning that the distortion-length slope threshold
                found at `slope_thresholds'[k] should be decreased as required
                to ensure that at least this many bytes are generated.  In
                this case, each call to the `flush' function will return the
                possibly adjusted distortion-length slope values that were
                actually used.
       */
    KDU_EXPORT int
      trans_out(kdu_long max_bytes=KDU_LONG_MAX,
                kdu_long *layer_bytes=NULL, int layer_bytes_entries=0,
                bool record_in_comseg=false, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Use this output function instead of `flush' when the code-stream
           has been created by a transcoding operation which has no real
           distortion information.  In this case, the individual code-block
           `pass_slopes' values are expected to hold 0xFFFF-layer_idx, where
           layer_idx is the zero-based index of the quality layer to which
           the code-block contributes (the pass slope value is 0 if a later
           pass contributes to the same layer).  This policy is described in
           the comments appearing with the definition of
           `kdu_block::pass_slopes'.
           [//]
           A modified form of the rate allocation algorithm is used to write
           output quality layers with the same code-block contributions as
           the quality layers in the input codestream which is being
           transcoded.
           [//]
           If the existing layers exceed the `max_bytes' limit, empty
           packets are written for any complete quality layers which are to
           be discarded and partial layers are formed by discarding
           code-blocks starting from the highest frequency subbands and
           the bottom of the image.
           [//]
           Like the `flush' function, `trans_out' may be invoked multiple
           times to incrementally flush the code-stream contents, based on
           availability.  Each time the function is called, the function
           writes as much of the code-stream as it can, given the availability
           of compressed data, and the current packet progression sequence.
           The same restrictions and recommendations supplied with the
           description of the `flush' function in regard to incremental
           flushing apply here also.  In particular, you should remember that:
           [>>] Each call generates new tile-parts (unless the call is issued
                in at a point when `ready_for_flush' would return false);
           [>>] The maximum number of tile-parts for any tile is 255; and
           [>>] The rate control associated with a supplied `max_bytes' limit
                is most effective if the function is called as infrequently as
                possible.
           [//]
           The value of the `max_bytes' argument is ignored for all but the
           first call to this function, at which point it is recorded
           internally for use in incrementally sizing the incrementally
           generated code-stream data in a uniform and consistent manner.
           [//]
           If the `layer_bytes' argument is non-NULL, it points to an array
           with `layer_bytes_entries' entries. Upon return, the entries in this
           array are set to the cumulative number of bytes written to each
           successive quality layer so far.  If the array contains insufficient
           entries to cover all available quality layers, the total code-stream
           length will generally be larger than the last entry in the
           array, upon return.
           [//]
           Note that internal state information is transferred to a non-NULL
           `layer_bytes' array even if the function does not actually
           generate any new codestream content (i.e., even if
           `ready_for_flush' would have returned false).
         [RETURNS]
           The number of non-empty quality layers which were generated.  This
           may be less than the number of layers in the input codestream
           being transcoded if `max_bytes' presents an effective restriction.
         [ARG: max_bytes]
           Maximum number of compressed data bytes to write out.  If necessary,
           one or more quality layers will be discarded, and then, within
           the last quality layer, code-blocks will be discarded in order,
           starting from the highest frequency subbands and working toward
           the lowest frequency subbands, in order to satisfy this limit.
           [//]
           If you do not want the compressed data to be truncated, it is
           best to specify a value of `KDU_LONG_MAX' for this argument, as
           opposed to any other large value, because the `KDU_LONG_MAX'
           value is recognized internally as the condition under which
           redundant sizing simulation phases can be avoided.
         [ARG: layer_bytes]
           If non-NULL, the supplied array has `layer_bytes_entries' entries
           and is used to return the number of bytes written so far into each
           quality layer.
         [ARG: layer_bytes_entries]
           Number of entries in the `layer_bytes' array.  Ignored if
           `layer_bytes' is NULL.
         [ARG: record_in_comseg]
           If true, the size and "rate-distortion" slope information will
           be recorded in a main header COM (comment) marker segment, for
           each quality layer.  While this may seem appropriate, it often is
           not, since fake R-D slope information is synthesized from the
           quality layer indices.  As a general rule, you should copy
           any COM marker segments from the original code-stream into the
           new one when transcoding.
         [ARG: env]
           This argument is provided to allow safe incremental flushing of
           codestream data while processing continues on other threads.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT bool
      ready_for_flush(kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Returns true if a call to `flush' or `trans_out' would be able
           to write at least one packet to the code-stream, given the
           constraints imposed by image tiling, the packet progression
           sequence in each relevant tile, and the amount of compressed data
           which is currently available.  Both `flush' and `trans_out'
           effectively invoke this function (or an internal equivalent),
           doing nothing (apart from returning internal state information)
           if this function would return false; this avoids the
           wasteful generation of empty tile-parts.
           [//]
           From Kakadu version 7, a more efficient mechanism for incremental
           flushing of the codestream is made available through the
           `auto_flush' and `auto_trans_out' functions.  These are especially
           beneficial in multi-threading environments, since the point at
           which an incremental flush should ideally be scheduled is hard
           to predict at the application level, due to asynchronous
           processing of sample data transforms and block encoding operations
           within a potentially large number of threads.
      */
    KDU_EXPORT void
      auto_flush(int first_tile_comp_trigger_point,
                 int tile_comp_trigger_interval,
                 int first_incr_trigger_point, int incr_trigger_interval,
                 const kdu_long *layer_bytes, int num_layer_specs,
                 const kdu_uint16 *layer_thresholds=NULL,
                 bool trim_to_rate=true, bool record_in_comseg=true,
                 double tolerance=0.0, kdu_thread_env *env=NULL,
                 int flags=0);
      /* [SYNOPSIS]
           From Kakadu version 7, it is recommended that you use this
           function for incremental flushing of the codestream, rather than
           explicitly calling `flush' -- this is particularly valuable
           when a multi-threaded processing environment is used to generate
           compressed data.  The function arranges for calls to `flush'
           to be generated automatically, based on a schedule which is
           determined by the first four arguments.  These arguments identify
           up to two types of trigger-points that can provoke the automatic
           `flush' call.
           [//]
           The auto-flush trigger-points are ultimately driven by calls to
           `kdu_subband::block_row_generated'.  If the auto-flush mechanism
           is to work, it is expected that `kdu_subband::block_row_generated'
           is called from within the `kdu_encoder' object, once each full
           row of code-blocks has been generated and transferred to the
           subband via calls to `kdu_subband::open_block' and
           `kdu_subband::close_block'.  The two types of trigger points are
           as follows:
           [>>] Tile-component trigger points are designed primarily for
                processing images with multiple tiles.  Once a tile (or a
                collection of tiles) has been generated, codestream content
                can be generated.  The internal machinery maintains a
                tile-component counter that is initialized to the value of
                `first_tile_comp_trigger_point' and decremented each time
                a tile-component has been fully generated (i.e., available
                for flushing).  When the counter reaches 0, an incremental
                "flush action" is taken and the counter is reloaded with the
                value of `tile_comp_trigger_interval'.  In single-threaded
                mode the "flush action" is equivalent to calling `flush'.
                In multi-threaded mode, however, the "flush action" involves
                setting an internal flag that will cause a background flush
                job to be scheduled by the next call to
                `kdu_subband::block_row_generated'; the reason for waiting
                until `kdu_subband::block_row_generated' is next
                called before scheduling the background flush job, is
                that this gives the application an opportunity to close
                old `kdu_tile' interfaces and open new ones before internal
                state information is locked for the flush operation; in most
                cases, this should allow tile processing to continue while
                automatic flushing of generated content proceeds in the
                background.
           [>>] Incremental (incr) trigger points are designed for incremental
                The internal machinery maintains an internal counter that is
                initialized to the value of `first_incr_trigger_point'.
                Similar to tile-component trigger points, this counter is
                decreased by the generation of code-block sample data, a
                "flush action" is taken when it becomes <= 0, after which it
                is incremented by `incr_trigger_interval'.  The two main
                differences here are: 1) the "flush action" becomes available
                to run (or be scheduled) as soon as the counter reaches 0;
                and 2) the amount by which the counter is decreased depends
                on the block row that has been generated and also on whether
                the compressed data target is a structured cache or a linear
                codestream.  If the compressed data target is a linear
                codestream, the counter is decremented only on
                `kdu_subband::block_row_generated' calls that are associated
                with LL subbands, in which case it is decremented by 2^D*S*H,
                whre D is the number of DWT levels in the relevant
                tile-component, S is the vertical sub-sampling factor of the
                relevant image component, and H is the height of the relevant
                LL subband's code-blocks.  If the compressed data target is a
                structured cache, precincts can be flushed in any order, so
                we don't need to wait for LL code-blocks to be generated; in
                this case, the counter is decremented by every call to
                `kdu_subband::block_row_generated' that corresponds to a
                horizontally low-pass subband, by the amount S*H, where S and
                H are as before.  In the above description, the terms
                "vertical" (or height) and "horizontal" are interpreted
                with respect to any geometric adjustments that may have been
                introduced by calls to `change_appearance'.
           [//]
           In a typical application, image data is supplied line by line and
           pushed into a collection of one or more tile processing engines
           (one for each horizontally adjacent tile).  In this case, the
           natural choice for `tile_comp_trigger_interval' would be C*NT,
           where C is the number of image components and NT is the number of
           tiles spanned by a single image row.  For incremental flushing
           within tiles, the natural choice for incr_trigger_interval' would
           be C*NT*F, where F is understood as the number of lines between
           flushes, expressed on the high resolution canvas.
           [//]
           Typically, you would set `first_tile_comp_trigger_point' equal to
           K * `tile_comp_trigger_interval' and `first_incr_trigger_point'
           equal to K * `incr_trigger_interval', where K is an integer
           multiplier.  K = 1 generally minimizes internal memory consumption,
           but larger values of K may produce better rate control if the
           generated output is being constrained by non-zero entries in the
           `layer_bytes' array.
           [//]
           You would typically call this function once, at the point when
           you are ready to start generating compressed content; calling the
           function multiple times may produce unpredictable results with
           regard to the interpretation of flush trigger points.  In any
           event, the first call to either `flush' or `auto_flush' (whichever
           comes first) is special in that it sets up the rate-control
           parameters, based on the contents of the `layer_bytes' and
           `layer_thresholds' arrays -- this is explained in connection with
           `flush'.
           [//]
           Unlike `flush', the present function does not return any
           information via the `layer_bytes' and/or `layer_thresholds' arrays
           (they are considered read-only arguments).  However, you should
           generally issue a final call to `flush' once all processing is
           complete, and that call can be used to return the number of bytes
           actually allocated to each layer and, optionally, the associated
           slope thresholds; this will work even if all content has been
           flushed.
         [ARG: first_tile_comp_trigger_point]
           Identifies the first tile-component based flush trigger point.
           This value must be strictly greater than 0.  If you have only one
           tile, you will probably only want incremental flush triggering so
           you can set this argument to a value that is larger than the
           number of image components, effectively disabling tile-component
           based flush triggering.
         [ARG: tile_comp_trigger_interval]
           Identifies subsequent tile-component based flush trigger points.
           This value also must be strictly greater than 0.
         [ARG: first_incr_trigger_point]
           Identifies the first incremental flush trigger point.  A value of 0
           may be used to disable incremental flush triggering.
         [ARG: incr_trigger_interval]
           If 0, there will be no incremental flush triggering after the
           `first_incr_trigger_point' (if it exists) has occurred.
         [ARG: layer_bytes]
           Same interpretation as in `flush'.
         [ARG: num_layer_specs]
           Same interpretation as in `flush'.
         [ARG: layer_thresholds]
           Same interpretation as in `flush'.
         [ARG: trim_to_rate]
           Same interpretation as in `flush'.
         [ARG: record_in_comseg]
           Same interpretation as in `flush'.
         [ARG: tolerance]
           Same interpretation as in `flush'.
         [ARG: flags]
           Same interpretation as in `flush'.
      */
    KDU_EXPORT void
      auto_trans_out(int first_tile_comp_trigger_point,
                     int tile_comp_trigger_interval,
                     int first_incr_trigger_point, int incr_trigger_interval,
                     kdu_long max_bytes=KDU_LONG_MAX,
                     bool record_in_comseg=false, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This function plays exactly the same role as `auto_flush',
           except that it automatically generates calls to `trans_out'
           (or an internal equivalent), whereas `auto_flush'
           automatically generates calls to `trans_out' (or an internal
           equivalent).  The first four arguments have the same
           interpretation as in `auto_flush', while the last three
           arguments have the same interpretation as their namesakes in
           `trans_out'.  As with `auto_flush', a final explicit call to
           `trans_out' may be required once all content has been generated.
      */
  // ------------------------------------------------------------------------
  public: // Summary information reporting functions
    KDU_EXPORT kdu_long
      get_total_bytes(bool exclude_main_header=false);
      /* [SYNOPSIS]
           Returns the total number of bytes written to or read from
           the code-stream so far.  Returns 0 if used with an interchange
           codestream (i.e., if `kdu_codestream' was created with neither a
           compressed data source nor a compressed data target).
           [//]
           If `set_max_bytes' was called with its `simulate_parsing'
           argument set to true, and the code-stream was created for input,
           the value returned here excludes the bytes associated with
           code-stream packets which do not belong to the resolutions,
           components or quality layers of interets, as configured via
           any call to `apply_input_restrictions'.
         [ARG: exclude_main_header]
           If true, the size of the code-stream main header is excluded
           from the returned byte count.
      */
    KDU_EXPORT kdu_long
      get_packet_bytes();
      /* [SYNOPSIS]
           Returns the total number of bytes found in JPEG2000 packets
           (packet headers + packet bodies).  This is the majority of the
           codestream bytes identified by `get_total_bytes', but excludes
           all markers and marker segments other than SOP (Start-Of-Packet)
           marker segments, if used, and EPH (End-Packet-Header) markers,
           if used.
           [//]
           The function is intended for use with output codestreams
           (those for which a `kdu_compressed_target' reference was passed
           to `create').  The reason for being interested in this function
           is that the `Creslengths' parameter attribute (see `cod_params')
           provides absolute limits on the amount of compressed data that
           is generated, in exactly the same sense that is used by this
           present function.  That is the `Creslengths' attribute does not
           take into account the cost of any markers or marker segments (i.e.,
           header information) other than SOP/EPH markers where they are used.
           The `Creslengths' attribute provides an important vehicle for
           creating codestreams that are guaranteed to conform to certain
           profiles -- notably, the digital cinema and broadcast profiles
           (see `Sprofile' in `siz_params').  However, the actual sizes that
           need to be regulated in accordance with some profiles incorporate
           the additional marker segments found in codestream headers -- these
           are usually fixed in size for any given compression application.
           To ensure profile conformance, one can determine the size of these
           headers by comparing the value returned by this function with
           that returned by `get_total_bytes' and subtract the difference from
           the overall length regulated by `Creslength'.
           [//]
           The value returned by this function for input or interchange
           codestreams is currently undefined.
      */
    KDU_EXPORT int
      get_num_tparts();
      /* [SYNOPSIS]
           Returns the total number of tile-parts written to or read from
           the code-stream so far.  When reading a persistent code-stream
           with multiple tiles, this value can grow indefinitely if tiles
           are unloaded from memory and subsequently re-parsed.
      */
    KDU_EXPORT void
      collect_timing_stats(int num_coder_iterations);
      /* [SYNOPSIS]
           Modifies the behaviour of `kdu_block_encoder' or
           `kdu_block_decoder', if used, to collect timing statistics
           which may later be retrieved using the `get_timing_stats' function.
           [//]
           If `num_coder_iterations' is non-zero, the block encoder or decoder
           will be asked to time itself (it need not necessarily comply) by
           processing each block `num_coder_iterations' times.  If it does so,
           block coder throughput statistics will also be reported.  If
           `num_coder_iterations' is 0, end-to-end times are generally very
           reliable.  Otherwise, the numerous calls to the internal `clock'
           function required to time block coding operations may lead to
           some inaccuracies.  The larger the value of `num_coder_iterations',
           the more reliable block coding times are likely to be, since the
           coder is executed multiple times between calls to `clock'.  On the
           other hand, the end-to-end execution time needs to be corrected to
           account for multiple invocations of the block coder, and this
           correction can introduce substantial inaccuracies.
      */
    KDU_EXPORT double
      get_timing_stats(kdu_long *num_samples, bool coder_only=false);
      /* [SYNOPSIS]
           If `coder_only' is false, the function returns the number of
           seconds since the last call to `collect_timing_stats'.  If a block
           coder timing loop has been used to gather more accurate block coding
           statistics by running the block coder multiple times, the function
           estimates the number of seconds which would have been consumed if
           the block coder had been executed only once per code-block.
           [//]
           If `coder_only' is true, the function returns the number of
           seconds required to process all code-blocks.  If the coder was
           invoked multiple times, the returned number of seconds is
           normalized to the time which would have been required by a single
           invocation of the coder.  If the coding operation was not timed,
           the function returns 0.
           [//]
           If `num_samples' is non-NULL, it is used to return the number of
           samples associated with returned CPU time.  If `coder_only' is true,
           this is the number of code-block samples.  Otherwise, it is the
           number of samples in the current image region.  In any event,
           dividing the returned time by the number of samples yields the
           most appropriate estimate of per-sample processing time.
      */
    KDU_EXPORT kdu_long
      get_compressed_data_memory(bool get_peak_allocation=true);
      /* [SYNOPSIS]
           Returns the total amount of heap memory allocated for storing
           compressed data.  This includes compressed code-block bytes,
           coding pass length and R-D information and layering information.
           It does not include the storage used for managing the structures
           within which the compressed data is encapsulated -- the amount
           of memory required for this is reported by
           `get_compressed_state_memory'.
           [//]
           In the event that the `share_buffering' member function has been
           used to associate a single compressed data buffering service with
           multiple codestream objects, the value reported here represents the
           amount of memory consumed by all such codestream objects together.
         [ARG: get_peak_allocation]
           If true (default), the value returned represents the maximum amount
           of storage ever allocated for storing compressed data.  Otherwise,
           the value represents the current amount of storage actually being
           used to store compressed data.  This latter quantity is useful if
           incremental flushing (see `kdu_codestream::flush' and/or
           `kdu_codestream::trans_out') is to be used to minimize internal
           buffering.  In that case, the incremental flushing may be partially
           controlled by the amount of compressed data which is currently in
           use.
      */
    KDU_EXPORT kdu_long
      get_compressed_state_memory(bool get_peak_allocation=true);
      /* [SYNOPSIS]
           Returns the total amount of heap memory allocated to storing
           the structures which are used to manage compressed data.  This
           includes tile, tile-component, resolution, subband and precinct
           data structures, along with arrays used to store the addresses
           of precincts which might not actually be loaded into memory --
           for massive images, even a single pointer per precinct can
           represent a significant amount of memory.  The memory
           requirements may go up and down while the codestream management
           machinery is in use, since elements are created on-demand as
           late as possible, and destroyed as early as possible.  Also,
           both precincts and tiles can be dynamically unloaded and
           reloaded on-demand when reading suitably structured code-streams.
           For more on this, see the `augment_cache_threshold' function.
           [//]
           In the event that the `share_buffering' member function has been
           used to associate a single compressed data buffering service with
           multiple codestream objects, the value reported here represents the
           amount of structural memory consumed by all such codestream
           objects together.
         [ARG: get_peak_allocation]
           If true (default), the value returned represents the maximum amount
           of structural memory ever allocated.  Otherwise, the value
           represents the current amount of storage actually being used.
           This value could be used by an application which wants to
           manage its own memory resources more tightly than can be
           achieved by the automatic cache management mechanism manipulated
           by the `augment_cache_threshold' function.  In particular, an
           application could monitor the current memory allocation
           obtained by summing the return values from this function and
           the `get_compressed_data_memory' function, using this value
           to determine whether it should close some open tiles -- open
           tiles cannot be dynamically unloaded by the automatic cache
           management mechanism.
      */

    // ------------------------------------------------------------------------
    private: // Interface state
      friend class kdu_thread_env;
      kd_codestream *state;
  };

/*****************************************************************************/
/*                                   kdu_tile                                */
/*****************************************************************************/

class kdu_tile {
  /* [BIND: interface]
     [SYNOPSIS]
       Objects of this class are only interfaces, having the size of a single
       pointer (memory address).  Copying such objects has no effect on the
       underlying state information, but simply serves to provide another
       interface (or reference) to it.  The class provides no destructor,
       because destroying an interface has no impact on the
       state of the internal code-stream management machinery.
       [//]
       To obtain a valid `kdu_tile' interface into the internal code-stream
       management machinery, you should call `kdu_codestream::open_tile'.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle member functions
    kdu_tile() { state = NULL; }
      /* [SYNOPSIS]
         Creates an empty interface.  You should not
         invoke any of the member functions, other than `exists'
         or `operator!' on such an object.
      */
    kdu_tile(kd_tile *state) { this->state = state; }
      /* [SYNOPSIS]
         You should never need to call this function yourself.  It is used
         by the internal codestream management machinery to provide the
         interface with a valid tile reference.
      */
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
         Indicates whether or not the interface is empty.  Use
         `kdu_codestream::open_tile' to obtain a non-empty tile interface
         into the internal code-stream management machinery.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
         Opposite of `exists'.  Returns true if the interface is empty. */
    KDU_EXPORT void
      close(kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Although not strictly necessary, it is a good idea to call this
           function once you have finished reading from or writing to a tile.
           [//]
           Once closed, future attempts to access the same tile via the
           `kdu_codestream::open_tile' function will generate an error,
           except in the event that the codestream was created (see
           `kdu_codestream::create') for interchange (i.e., with neither a
           compressed data source nor a compressed data target) or it was
           created for input and configured as persistent (see the
           `kdu_codestream::set_persistent' function).
           [//]
           With non-persistent input codestreams, the system is free
           to discard all internal storage associated with a closed tile at
           its earliest convenience.
           [//]
           For interchange and persistent input codestreams, all open
           tiles must be closed before the appearance can be modified by
           `kdu_codestream::change_appearance' or
           `kdu_codestream::apply_input_restrictions'.
      */
  // --------------------------------------------------------------------------
  public: // Identification member functions
    KDU_EXPORT int
      get_tnum();
      /* [SYNOPSIS]
           Returns the index of the present tile, as defined by
           the JPEG2000 standard.  In JPEG2000 tiles, are numbered in
           raster scan fashion, starting from 0.  There may be no more
           than 2^{16} tiles.
      */
    KDU_EXPORT kdu_coords
      get_tile_idx();
      /* [SYNOPSIS]
           Returns the coordinates of this tile, using the same coordinate
           system as that used by `kdu_codestream::open_tile'.
      */
  // --------------------------------------------------------------------------
  public: // Data processing/access member functions
    KDU_EXPORT bool
      get_ycc();
      /* [SYNOPSIS]
           Returns true if a component colour transform is to be applied to
           the first 3 codestream image components of the current tile.  In
           the event that the one or more of the first 3 codestream image
           components is not visible (due to prior calls to
           `kdu_codestream::apply_input_restrictions'), of not of
           interest (as indicated by calls to
           `kdu_tile::set_components_of_interest' in the
           `KDU_WANT_CODESTREAM_COMPONENTS' access mode), the function will
           return false.
      */
    KDU_EXPORT void
      set_components_of_interest(int num_components_of_interest=0,
                                 const int *components_of_interest=NULL);
      /* [SYNOPSIS]
           This function may be used together with `get_mct_block_info'
           and the other `get_mct_...' functions to customize the
           way the multi-component transform network appears.
           [//]
           By default, the multi-component transform network described by
           `get_mct_block_info' and related functions is configured to
           produce all apparent output components, where the set of
           apparent output components is determined by calls to
           `kdu_codestream::apply_input_restrictions'.
           [//]
           Calls to this function, however, can identify any subset of these
           apparent output components as the ones which the multi-component
           transform network should be configured to generate.  All other
           apparent output components will appear to contain constant
           sample values.
           [//]
           We make the following important observations:
           [>>] Let N be the number of apparent output components.  This is
                the value returned by `kdu_codestream::get_num_components'
                with its optional `want_output_comps' argument set to true.
           [>>] The value of N is not affected by calls to this function.
                Moreover, the `num_stage_outputs' value returned by
                `get_mct_block_info' for the final transform stage, is also
                not affected by calls to this function -- this value will
                always be equal to N.
           [>>] Calls to this function do, however, affect the connectivity
                of the multi-component transform network.  In particular
                they may affect the number of transform blocks which appear
                to be involved, as well as the number of components which
                those transform blocks appear to produce and/or consume.
           [>>] You can call the present function as often as you like,
                without closing and re-opening the tile interface.  After
                each call, the multi-component transform network may appear
                to be different.  This allows you to create multiple
                partial renderings for different subsets of the N apparent
                output components, each with their own multi-component
                transform network.
           [>>] After each call to this function, you use the tile interface
                to construct a `kdu_multi_synthesis' or `kdu_multi_analysis'
                object; with multiple calls to this function, each
                so-constructed object may be different.  Each of them
                will advertize the same set of output image components, but
                they may have different sets of "constant" output
                components.  Of course, you are not likely to want to access
                the constant output components, but doing so will not incur
                any computational overhead.
           [>>] Calls to this function have no effect on the set of
                codestream components which are presented by the tile
                interface.  In particular, they have no impact at all on
                the `access_component' function's return value.
           [//]
           Note that the impact of this function is lost if the tile is
           closed and then re-opened.
         [ARG: num_components_of_interest]
           If 0, all apparent output components are considered to be of
           interest so that subsequent calls to `get_mct_block_info' and
           related functions will describe a multi-component transform
           network which generates all apparent output image components.
         [ARG: components_of_interest]
           Contains `num_components_of_interest' output component indices,
           each of which must be in the range 0 to N-1 where N is the
           value returned by `kdu_codestream::get_num_components' with its
           `want_output_comps' argument set to true.  If this argument is
           NULL and `num_components_of_interest' is greater than 0, the
           first `num_components_of_interest' apparent output components will
           be the ones which are considered to be of interest.
           [//]
           The `components_of_interest' array is allowed to contain multiple
           entries with the same component index if you like.
      */
    KDU_EXPORT bool
      get_mct_block_info(int stage_idx, int block_idx,
                         int &num_stage_inputs, int &num_stage_outputs,
                         int &num_block_inputs, int &num_block_outputs,
                         int *block_input_indices=NULL,
                         int *block_output_indices=NULL,
                         float *irrev_block_offsets=NULL,
                         int *rev_block_offsets=NULL,
                         int *stage_input_indices=NULL);
      /* [SYNOPSIS]
           This is the principle interface to multi-component transform
           information.  The auxiliary functions, `get_mct_matrix_info',
           `get_mct_rxform_info', `get_mct_dependency_info' and
           `get_mct_dwt_info', are used to recover specific parameters for
           each type of transform operation, but the structure of the
           transform is identified by the present function.
           [//]
           To discover the number of transform stages, simply invoke this
           function with progressively larger `stage_idx' arguments, keeping
           `block_idx' equal to 0.  The first value of `stage_idx', for which
           the function returns false, is the total number of stages.  Note
           that stage 0 is the first stage to be performed when reconstructing
           output image components from the codestream image components during
           decompression.
           [//]
           To discover the number of transform blocks in each stage (there
           must be at least one), invoke the function with progressively
           larger `block_idx' arguments, until it returns false.
           [//]
           If no Part-2 MCT is defined, or the current component access mode
           is `KDU_WANT_CODESTREAM_COMPONENTS' (see
           `kdu_codestream::apply_input_restrictions'), this function will
           always report the existence of exactly one (dummy) transform
           stage, with one transform block, representing a null transform.
           Moreover, the function ensures that whenever `get_ycc' returns
           true, this dummy transform stage involves sufficient permutations
           to ensure that the first 3 stage input components invariably
           correspond to the original first 3 codestream components, so that
           the YCC transform can be applied exclusively to those 3 components,
           without reordering.  These conventions simplify the implementation
           of general purpose data processing engines.
           [//]
           The number of nominal output components produced by the stage
           is returned via the `num_stage_outputs' argument.  This value
           covers all input components required by the next stage (if any)
           or all output image components (for the last stage).  It is
           possible that some of these stage output components are not
           generated by any of the stage's transform blocks; these
           default to being identically equal to 0.
           [//]
           The `num_stage_inputs' argument is used to return the number of
           input components used by the stage.  This is identical to the
           number of output components produced by the previous stage (as
           explained above), except for the first stage -- its input
           components are the enabled tile-components which are required
           by the multi-component transform network, in order to process
           all the output components which it offers.  If you supply
           a non-NULL `stage_input_indices' array, it will be filled with
           the absolute codestream component indices which are required by
           the first transform stage.  If this is not the first stage
           (`stage_idx' > 0), the `stage_input_indices' argument should be
           NULL.
           [//]
           The `block_input_indices' and `block_output_indices' arguments,
           if non-NULL, will be filled with the indices of the input
           components used by the transform block and the output components
           produced by the transform block.  These indices are expressed
           relative to the collections of stage input and output components,
           so all indices are strictly less than the values returned via
           `num_stage_inputs' and `num_stage_outputs', respectively.
           [//]
           To discover how the transform block is implemented, you will need
           to call `get_mct_matrix_info', `get_mct_rxform_info',
           `get_mct_dependency_info' and `get_mct_dwt_info', at most one of
           which will return non-zero (not NULL or false).  If all four
           methods return zero, this transform block is what we call a
           `null transform'.  A "null transform" passes its inputs through
           to its outputs (possibly with the addition of output offsets, as
           described below).  Where there are more outputs than inputs, the
           final `num_block_outputs'-`num_block_inputs' components produced
           by a null transform are simply set to 0, prior to the addition of
           any offsets (see below).
           [//]
           The `irrev_block_offsets' and `rev_block_offsets' arrays, if
           non-NULL, are used to return offsets which are to be added to the
           outputs produced by the transform block.  The offsets, and indeed
           all transform coefficients returned via `get_mct_matrix_info',
           `get_mct_rxform_info', `get_mct_dependency_info' or
           `get_mct_dwt_info', are represented using the normalization
           conventions of the JPEG2000 marker segments from which they
           derive.  This convention means that the supplied coefficients
           would produce the correct output image components, at their
           declared bit-depths and signed/unsigned characteristics,
           if applied to codestream image components which are also
           expressed at their declared bit-depths (as signed quantities).
           [//]
           The `irrev_block_offsets' argument should be used to retrieve
           offsets for irreversible transforms, while the `rev_block_offsets'
           argument should be used for reversible transforms.  Whether or not
           the transform is reversible can be learned only by calling the
           `get_mct_matrix_info', `get_mct_rxform_info',
           `get_mct_dependency_info' and `get_mct_dwt_info' functions.  If
           all of these return 0 (false or NULL), meaning that a "null
           transform" is being used, each component may be represented either
           reversibly or irreversibly.  In this case, offset information may
           be retrieved using either `rev_block_offsets' or
           `irrev_block_offsets', but the former method just returns
           integer-rounded versions of the values returned by the latter
           method.
         [RETURNS]
           True if `stage_idx' identifies a valid multi-component transform
           stage and `block_idx' identifies a valid transform block within
           that stage.  Note that this function will never return false when
           invoked with `stage_idx' and `block_idx' both equal to 0.
         [ARG: block_input_indices]
           Leave this NULL until you know how many input indices the block
           has, as returned via `num_block_inputs'.  Then call the function
           again, passing an array with `num_block_inputs' entries for this
           argument.  It will be filled with the indices of the input
           components used by this block (unless the function returns false,
           of course).
         [ARG: block_output_indices]
           Leave this NULL until you know how many output indices the block
           has, as returned via `num_block_outputs'.  Then call the function
           again, passing an array with `num_block_outputs' entries for this
           argument.  It will be filled with the indices of the output
           components used by the block (unless the function returns false,
           of course).
         [ARG: irrev_block_offsets]
           Leave this NULL until you know the number of output indices for
           the block and whether or not the transform is irreversible.  Then
           call the function again, passing an array with `num_block_outputs'
           entries.  It will be filled with the offsets to be applied to the
           transform's output components.  Offsets are added to the transform
           outputs during decompression (inverse transformation).
         [ARG: rev_block_offsets]
           Same as `irrev_block_offsets', but this argument is used to return
           integer-valued offsets for reversible transformations.  For
           null transforms, you may use either `irrev_block_offsets' or
           `rev_block_offsets' to return the offsets, but this latter method
           will return rounded versions of the values returned via
           `irrev_block_offsets' if they happen not to be integers already.
         [ARG: stage_input_indices]
           Leave this NULL if `stage_idx' > 0 or if you do not yet know the
           number of stage inputs.  Once the value of `num_stage_inputs'
           has been set in a previous call to this function, you may
           supply an array with this many entries for this argument.  It
           will be filled with the absolute codestream indices associated with
           each tile-component which is used by the multi-component inverse
           transform.  If the `kdu_codestream' machinery was created for
           output (i.e., with a `kdu_compressed_target' object), the first
           stage's `num_stage_inputs' argument will always be equal to the
           global number of codestream components, and the i'th entry of
           any non-NULL `stage_input_indices' array will be filled with the
           value of i.
      */
    KDU_EXPORT bool
      get_mct_matrix_info(int stage_idx, int block_idx,
                          float *coefficients=NULL);
      /* [SYNOPSIS]
           Use this function, after first discovering the structure of a
           multi-component transform block, using `get_mct_block_info', to
           determine whether the transform block is implemented using an
           irreversible decorrelation transform.  If the function returns
           false, you should try `get_mct_rxform_info',
           `get_mct_dependency_info' and `get_mct_dwt_info' to determine if
           the block is implemented using a reversible decorrelation
           transform, a dependency transform or a discrete wavelet transform.
           If all four functions return zero (false or NULL), the block is
           implemented using a "null transform" -- see `get_mct_block_info'
           for a discussion of null transforms.
           [//]
           Any non-NULL `coefficients' array should have M by N entries,
           where M and N are the `num_block_outputs' and `num_block_inputs'
           values returned by the corresponding call to `get_mct_block_info'.
           Note that the M by N matrix returned via `coefficients' is in
           row-major order and represents the inverse multi-component
           transform -- i.e., the transform to be applied during
           decompression.
           [//]
           If this function returns true then you should use the
           `irrev_block_offsets' argument of the `get_mct_block_info'
           function to determine any offsets to be applied to the
           transform outputs.
         [RETURNS]
           False if this block is not implemented by an irreversible
           decorrelation transform.
         [ARG: coefficients]
           You would typically call the function first with this argument set
           to NULL.  Then, once you know that an irreversible decorrelation
           transform is being used, you would allocate an array of the
           correct size and pass it to the function in a second call, so as
           to receive the transform coefficients.
      */
    KDU_EXPORT bool
      get_mct_rxform_info(int stage_idx, int block_idx,
                          int *coefficients=NULL,
                          int *active_outputs=NULL);
      /* [SYNOPSIS]
           Use this function, after first discovering the structure of a
           multi-component transform block, using `get_mct_block_info', to
           determine whether the transform block is implemented using a
           reversible decorrelation transform.  Reversible decorrelation
           transforms are implemented very differently from irreversible
           matrix decorrelation transforms (see `get_mct_matrix_info').  If
           this function returns false, you should try `get_mct_matrix_info',
           `get_mct_dependency_info' and `get_mct_dwt_info' to determine if
           the block is implemented using an irreversible matrix decorrelation
           transform, a dependency transform or a discrete wavelet transform.
           If all four functions return zero (false or NULL), the block is
           implemented using a "null transform" -- see `get_mct_block_info'
           for a discussion of null transforms.
           [//]
           Any non-NULL `coefficients' array must have M by (M+1) entries,
           where M is the value of `num_block_inputs' returned by
           `get_mct_block_info'.  A non-NULL `coefficients' array is used
           to return an M by (M+1) matrix, in row-major order.  The (M+1)
           columns of this matrix represent a succession of reversible
           "lifting" steps.  During the inverse multicomponent transform
           (the one used during decompression), the first column serves to
           adjust the last input component, based on a rounded linear
           combination of the others.  The second column serves to adjust
           the second-last input component, based on a rounded linear
           combination of the others, and so forth.  The final (extra)
           column again adjusts the last component, based on a
           rounded linear combination of the first M-1 components, but it
           also introduces a possible sign change.
           [//]
           A more precise description of the reversible decorrelation
           transform is as follows (remember that this description is for
           the inverse transform, used during decompression).  Note that
           all coefficients and input samples are integers.  For each
           column c=0 to c=M-1, update component c'=M-1-c as follows
           [>>] Form the weighted linear combination, Uc', of all input
                components j != c', using weights Tjc (row j, column c of the
                `rev_coefficients' matrix).
           [>>] Divide the result by Tc'c, rounding to the nearest integer.
                Specifically, form floor((Uc'+(Tc'c/2))/Tc'c).  Here, Tc'c is
                guaranteed to be a positive power of 2.
           [>>] Subtract the result from input component c' and use the
                modified value for subsequent steps.
           [//]
           The final step (column c=M) is similar to the first, modifying
           component c'=M-1, based on a weighted combination of components
           0 through M-1 using coefficients TjM, followed by rounded division.
           In this case, however, the rounded division step is performed using
           |Tc'M| and the adjusted final component samples are sign reversed if
           Tc'M is negative.  Here, |Tc'M| is guaranteed to be a positive
           power of 2.
           [//]
           In their original form, reversible transform blocks always have
           an identical number of input and output components, so that M is
           also the number of output components produced by the block.
           However, Kakadu's codestream management machinery conceals the
           presence of output components which are not used by subsequent
           stages.  This depends also on the set of output components which
           are of interest to the application, as configured by calls to
           the `kdu_codestream::apply_input_restrictions' function.  The
           application can typically remain oblivious to such changes, except
           in a few circumstances, where intermediate results must be computed
           over a larger set of original components.  In the present case, the
           reversible transform must first be applied to all the input
           components.  Once this has been done, the transformed input
           components are mapped to output components according to the
           information returned via the `active_outputs' array.  Specifically,
           the output component with index n should be taken from the
           transformed input component whose index is `active_outputs'[n], for
           each n in the range 0 to `num_block_outputs', where
           `num_block_outputs' is the value returned via `get_mct_block_info'.
           [//]
           If this function returns true then you should use the
           `rev_block_offsets' argument of the `get_mct_block_info'
           function to determine any offsets to be applied to the
           transform outputs.
         [RETURNS]
           False if this block is not implemented by a reversible
           decorrelation transform.
         [ARG: coefficients]
           You would typically call the function first with this argument set
           to NULL.  Then, once you know that a reversible decorrelation
           transform is being used, you would allocate an array of the
           correct size and pass it to the function in a second call, so as
           to receive the transform coefficients.  Note carefully that the
           array represents a matrix with M by (M+1) entries where M is the
           number of block inputs (not necessarily equal to the number of
           block outputs).
         [ARG: active_outputs]
           You would typically call the function first with this argument set
           to NULL.  Then, once you know that a reversible decorrelation
           transform is being used, you would allocate an array with
           `num_block_outputs' entries (the number of block outputs is
           returned by `get_mct_block_info') and pass it to the function in a
           second call, so as to determine the indices of the transformed
           input components which are to be assigned to each output component.
           In the special case where the `num_block_inputs' and
           `num_block_outputs' values returned via `get_mct_block_info' are
           identical, this is not necessary, since then the set (and order)
           of outputs is guaranteed to be identical to the set of inputs --
           this is the way reversible transform blocks are defined by the
           underlying JPEG2000 standard, in the absence of any restrictions
           on the set of components which are of interest to the application.
       */
    KDU_EXPORT bool
      get_mct_dependency_info(int stage_idx, int block_idx,
                              bool &is_reversible,
                              float *irrev_coefficients=NULL,
                              float *irrev_offsets=NULL,
                              int *rev_coefficients=NULL,
                              int *rev_offsets=NULL,
                              int *active_outputs=NULL);
      /* [SYNOPSIS]
           Use this function, after first discovering the structure of a
           multi-component transform block, using `get_mct_block_info', to
           determine whether the transform block is implemented using a
           dependency transform.  If the function returns false, you should
           try `get_mct_matrix_info', `get_mct_rxform_info' and
           `get_mct_dwt_info' to determine if the block is implemented using
           a decorrelation transform or a discrete wavelet transform.
           If all four functions return zero (false or NULL), the block is
           implemented using a "null transform" -- see `get_mct_block_info'
           for a discussion of null transforms.
           [//]
           Unlike decorrelation transforms, the offsets associated with
           dependency transforms are added to each output component before
           it is used as a predictor for subsequent components.  These
           offsets are returned via the `irrev_offsets' and `rev_offsets'
           arguments provided to this function; the corresponding arguments
           to `get_mct_block_info' will consistently return 0's for
           dependency transforms.
           [//]
           The inverse irreversible dependency transform (used during
           decompression) is implemented as follows:
           [>>] Form Out_0 = In_0 + Off_0
           [>>] Form Out_1 = T_10 * Out_0 + In_1 + Off_1
           [>>] Form Out_2 = T_20 * Out_0 + T_21 * Out_1 + In_2 + Off_2
           [>>] etc.
           [//]
           Here, the coefficients of the strictly lower triangular matrix, T,
           are returned via a non-NULL `irrev_coefficients' array with
           M*(M-1)/2 entries, where M is the value of `num_block_inputs'
           returned via `get_mct_block_info'.  These coefficients are
           returned in row-major order.
           [//]
           The inverse reversible dependency transform is implemented as
           follows:
           [>>] Form Out_0 = In_0 + Off_0
           [>>] Form Out_1 = floor((T_10*Out_0 +
                                   floor(T_11/2))/T_11) + In_1 + Off_1
           [>>] Form Out_2 = floor((T_20*Out_0 + T_21*Out_1 +
                                   floor(T_22/2))/T_22) + In_2 + Off_2
           [>>] etc.
           [//]
           Here, the coefficients of the lower triangular matrix, T, are
           returned via a non-NULL `rev_coefficients' array with M*(M+1)/2-1
           entries, where M is the value of `num_block_inputs' returned via
           `get_mct_block_info'.  Note that the reversible dependency
           transform has M-1 extra coefficients, corresponding to the
           diagonal entries, which are used to scale the weighted
           contribution of previously computed outputs -- there is no scale
           factor for the first component.  Note also that the diagonal
           entries, T_ii are guaranteed to be exact positive powers of 2.
           [//]
           Inverse dependency transforms can be implemented for any
           contiguous prefix of the original set of components, but the
           application may require only a subset of these components.  In
           this case, the `num_block_outputs' value returned via
           `get_mct_block_info' may be smaller than M.  In this case, you
           will have to supply an `active_outputs' array to discover the
           indices of the transformed input components which are actually
           mapped through to outputs.
         [RETURNS]
           False if this block is not implemented by a reversible or
           irreversible dependency transform.
         [ARG: is_reversible]
           Used to return whether or not the transform is to be implemented
           reversibly.  If true, the `rev_coefficients' argument must be
           used rather than `irrev_coefficients' to recover the transform
           coefficients.  Also, in this case the `rev_block_offsets' argument
           must be used instead of the `irrev_block_offsets' argument when
           retrieving offsets from the `get_mct_block_info' function.
         [ARG: irrev_coefficients]
           You would typically call the function first with this argument set
           to NULL.  Then, once you know that an irreversible dependency
           transform is being used, you would allocate an array of the
           correct size and pass it to the function in a second call, so as
           to receive the transform coefficients.  The array should have
           M*(M-1)/2 entries, where M is the number of block inputs, which
           is also equal to the number of block outputs.  This argument must
           be NULL if the transform block is reversible.
         [ARG: irrev_offsets]
           This argument must be NULL if the transform block is reversible.
           It is used to return exactly M offsets, which are to be applied
           in the manner described above.  Note that offsets belong to the
           input components, M, which may be larger than the set of output
           components (required by the application) reported by
           `get_mct_block_info'.
         [ARG: rev_coefficients]
           Similar to `irrev_coefficients' but this array is used to
           return the integer-valued coefficients used for reversible
           transforms.  It must be NULL if the transform block is irreversible.
           Note carefully that the array should have M*(M+1)/2-1 entries,
           where M is the number of block inputs, which is also equal to
           the number of block outputs -- see above for an explanation.
         [ARG: rev_offsets]
           Similar to `irrev_offsets', but this argument must be NULL except
           for reversible transforms, for which it may be used to return the
           M offset values.
         [ARG: active_outputs]
           You would typically call the function first with this argument set
           to NULL.  Then, once you know that a dependency transform is being
           used, you would allocate an array with `num_block_outputs' entries
           (the number of block outputs is returned by `get_mct_block_info')
           and pass it to the function in a second call, so as to determine
           the indices of the transformed input components which are to be
           assigned to each output component.  In the special case where the
           `num_block_inputs' and `num_block_outputs' values returned via
           `get_mct_block_info' are identical, this is not necessary, since
           then the set (and order) of outputs is guaranteed to be identical
           to the set of inputs -- this is the way dependency transform blocks
           are defined by the underlying JPEG2000 standard, in the absence of
           any restrictions on the set of components which are of interest
           to the application.
      */
    KDU_EXPORT const kdu_kernel_step_info *
      get_mct_dwt_info(int stage_idx, int block_idx,
                       bool &is_reversible, int &num_levels,
                       int &canvas_min, int &canvas_lim,
                       int &num_steps, bool &symmetric,
                       bool &symmetric_extension, const float * &coefficients,
                       int *active_inputs=NULL, int *active_outputs=NULL);
      /* [SYNOPSIS]
           Use this function, after first discovering the structure of a
           multi-component transform block, using `get_mct_block_info', to
           determine whether the transform block is implemented using a
           discrete wavelet transform (DWT).  If the function returns NULL,
           you should try `get_mct_matrix_info', `get_mct_rxforms_info' and
           `get_mct_dependency_info' to determine if the block is implemented
           using a decorrelation transform or a dependency transform.  If all
           four functions return zero (false or NULL), the block is
           implemented using a "null transform" -- see `get_mct_block_info'
           for a discussion of null transforms.
           [//]
           The returned `kdu_kernel_step_info' array, together with the
           information returned via the `is_reversible', `num_steps',
           `symmetric', `symmetric_extension' and `coefficients' arguments,
           completely defines an arbitrary wavelet kernel.  This information
           may be passed to `kdu_kernels::init', if desired.  The transform
           block's input components are considered as the subbands produced
           by application of a forward DWT to the output components -- DWT
           synthesis is thus required to reconstruct the output components
           from the input components.
           [//]
           More specifically, the original set of output components
           (not just those which are of interest to the application) is
           understood to form a 1D sequence, taking indices in the half-open
           interval [E,F), where E and F are the values returned via the
           `canvas_min' and `canvas_lim' arguments.  The input components
           are understood to have been formed by applying an N level DWT to
           this 1D sequence, where N is the value returned via the
           `num_levels' argument.  The low-pass subband produced by this
           DWT contains ceil(F/2^N) - ceil(E/2^N) samples.  These correspond
           to the initial ceil(F/2^N) - ceil(E/2^N) original block input
           components.  These are followed by the components from the lowest
           level's high-pass subband and so-on, finishing with the components
           which belong to the highest frequency subband.
           [//]
           It is important to remember that `canvas_min' and `canvas_lim'
           determine the length of the original 1D sequence to which the
           DWT was applied.  In practice, the application may be interested
           only in a subset of the output image components, and it may
           be possible to reconstruct these using only a subset of the
           original input components.  For this reason, the values of
           `num_block_inputs' and `num_block_outputs' returned via
           `get_mct_block_info' may both be smaller than F-E
           (i.e., `canvas_lim' - `canvas_min') and you will need to
           supply `active_inputs' and `active_outputs' arrays in order
           to discover which of the original component actually requires
           an input and which of the original components is being
           pushed through to the block output.
         [RETURNS]
           Array of `num_steps' lifting step descriptors, in analysis order,
           starting from the analysis lifting step which updates the
           odd (high-pass) sub-sequence based on the even (low-pass)
           sub-sequence.
         [ARG: is_reversible]
           Used to return whether or not the transform is to be implemented
           reversibly.  If true, the `rev_block_offsets' argument
           must be used instead of the `irrev_block_offsets' argument when
           retrieving offsets from the `get_mct_block_info' function.
         [ARG: num_levels]
           Used to return the number of levels of DWT which need to be
           synthesized in order to recover the output image components
           from the input components.
         [ARG: canvas_min]
           Used to return the inclusive lower bound E, of the interval
           [E,F), over which the DWT is defined.
         [ARG: canvas_lim]
           Used to return the exclusive upper bound F, of the interval
           [E,F), over which the DWT is defined.
         [ARG: num_steps]
           Used to return the number of lifting steps.
         [ARG: symmetric]
           Set to true if the transform kernel belongs to the whole-sample
           symmetric class defined by Part-2 of the JPEG2000 standard,
           with symmetric boundary extension.  For a definition of this
           class, see `kdu_kernels::is_symmetric'.
         [ARG: symmetric_extension]
           Returns true if each lifting step uses symmetric extension,
           rather than zero-order hold extension at the boundaries.  For
           a definition of this extension policy, see
           `kdu_kernels::get_symmetric_extension'.
         [ARG: coefficients]
           Used to return a pointer to an internal array of lifting
           step coefficients.  This array may be passed to `kdu_kernels::init'
           along with the other information returned by this function, to
           deduce other properties of the DWT kernel.
         [ARG: active_inputs]
           You would typically call the function first with this argument set
           to NULL.  Then, once you know that a DWT is being
           used, you would allocate an array with `num_block_inputs' entries
           (the number of block inputs is returned by `get_mct_block_info')
           and pass it to the function in a second call, so as to determine
           the locations of each input component which is actually involved
           in the synthesis of the output components of interest.
           Specifically, block input component n is used to initialize the
           original block input component with indices `active_inputs'[n].
           Original block input component indices are defined above, but
           essentially they are arranged in the following order: all
           original low-pass subband locations appear first, followed by
           all original high-pass subband locations at the lowest level
           in the DWT, and so on, finishing with all original high-pass
           subband locations at the top level in the DWT.
         [ARG: active_outputs]
           You would typically call the function first with this argument set
           to NULL.  Then, once you know that a DWT is being
           used, you would allocate an array with `num_block_outputs' entries
           (the number of block outputs is returned by `get_mct_block_info')
           and pass it to the function in a second call, so as to determine
           the locations to which each output component is to be associated.
           Specifically, block output component n is to be associated with
           location E+`active_outputs'[n] in the interval [E,F), over which
           DWT synthesis is notionally performed.
      */
    KDU_EXPORT int
      get_num_components();
      /* [SYNOPSIS]
           Returns the number of visible codestream image components, which
           is also the number of visible tile-components in the current tile.
           Note firstly that this value may be affected by previous calls to
           `kdu_codestream::apply_input_restrictions', but only if such calls
           modify the visibility of codestream image components.
           [//]
           Calls to the 2'nd form of `kdu_codestream::apply_input_restrictions'
           which specify `KDU_WANT_OUTPUT_COMPONENTS' do not restrict
           codestream component visibility in any way.  They may, however,
           affect the visibility of MCT output image components.
           [//]
           It may happen that some tile-components are not actually involved
           in the reconstruction of visible (or indeed any) output image
           components.  One reason for this is that a Part-2 multi-component
           transform does not use all codestream components in defining
           the full set of output components.  It can also happen that
           calls to the 2'nd form of `kdu_codestream::apply_input_restrictions'
           have restricted the set of visible output components, in which
           case some of the codestream components may not be required.
           [//]
           In any case, unused codestream image components are treated
           differently to invisible codestream image components.  Invisible
           components can arise only if the component `access_mode' supplied
           to the `kdu_codestream::apply_input_restrictions' function is
           `KDU_WANT_CODESTREAM_COMPONENTS'.  Of course, this cannot happen
           with `output codestreams' (i.e., if `kdu_codestream'
           was created for writing to a compressed data target).
           [//]
           Where a codestream component is visible but not used, it is still
           included in the count returned by the present function.
           However, attempting to access an unused component via
           the `access_component' function will result in an empty
           `kdu_tile_comp' interface being returned.  Note, however, that
           this does not happen with `output codestreams' (i.e., if
           `kdu_codestream' was created for writing to a compressed data
           target).  For output codestreams, all tile-components are both
           visible and accessible.
      */
    KDU_EXPORT int
      get_num_layers();
      /* [SYNOPSIS]
           Returns the apparent number of quality layers for the tile.  Note
           that this value is affected by the `max_layers' argument supplied
           in any call to `kdu_codestream::apply_input_restrictions'.
      */
    KDU_EXPORT bool
      parse_all_relevant_packets(bool start_from_scratch_if_possible,
                                 kdu_thread_env *env);
      /* [SYNOPSIS]
           This function may be used to parse all compressed data packets
           which belong to the tile.  The packets which are required to be
           parsed generally depend upon resolution, component, quality layer
           and region of interest restrictions which may have been applied via
           a previous call to `kdu_codestream::apply_input_restrictions'.
           [//]
           Normally, packets are parsed from the codestream on demand, in
           response to calls to the `kdu_subband::open_block' function.
           However, this function can be used to bring the relevant compressed
           data into memory ahead of time.  One reason you may wish to do this
           is to discover the amount of compressed data in each quality layer
           of the relevant tile-component-resolutions via the
           `get_parsed_packet_stats' function, so that you can make sensible
           choices concerning the number of quality layers you are prepared
           to discard in a time-constrained rendering application.
           [//]
           If your application has previously opened the tile and has since
           changed the region of interest, number of quality layers, and so
           forth, it may happen that some precincts have already been partially
           parsed and then unloaded from memory.  If this is the case, the
           information returned via a call to `get_parsed_packet_stats' will
           be incomplete, even after calling this function, since reparsed
           packets do not affect the internal parsed length counters.
           [//]
           If you want accurate information under these conditions, you can
           specify the `start_from_scratch_if_possible' option.  In this case,
           the collection of all precincts in the tile-component-resolutions
           which are relevant are first scanned to determine whether or not
           any have been unloaded from memory after parsing, or any precinct
           which is irrelevant to the current region of interest has
           already been parsed, in part or in full.  If none of these are
           true, nothing need be done.  Otherwise, all precincts in the
           tile-component resolutions of interest (not just those which are
           relevant to a current region of interest) are unloaded from memory
           and marked as never having been parsed.  After unloading all
           such precincts, the internal statistical counters for the relevant
           tile-components resolutions are reset and the function proceeds to
           parse just those packets which are relevant.
           [//]
           There are some conditions under which the functionality associated
           with `start_from_scratch_if_possible' cannot be realized, in
           which case the function returns false and no new information will
           be parsed.  These conditions are described below under the
           "RETURNS" section.
           [//]
           One consequence of the procedure described above is that for
           randomly accessible codestreams, the
           `start_from_scratch_if_possible' option can be used to force the
           statistics collected and later returned by
           `get_parsed_packet_stats' (with a non-negative
           `component_idx' argument) to correspond exactly to
           the currently selected region of interest, as specified in the most
           recent call to `kdu_codestream::apply_input_restrictions', if any.
         [RETURNS]
           False if the codestream was not created for input, or if
           `start_from_scratch_if_possible' was set to true and either:
           [>>] Precincts could not be randomly accessed, so as to be
                unloaded and subsequently reloaded from memory; or
           [>>] One or more precincts is currently in a state where it has
                been parsed (fully or partially) but its contents might be
                vulnerable to access by calls to `kdu_subband::open_block'
                arriving on another thread.  To avoid this possibility, you
                should generally call this function immediately after opening
                the tile -- i.e., before using the tile to instantiate any
                multi-threaded block decoding machinery.
         [ARG: start_from_scratch_if_possible]
           If true, the function tries to unload all existing precincts from
           the tile-component resolutions which are relevant, and reload
           only those which are relevant to the current region of interest,
           so that statistics returned via `get_parsed_packet_stats' will be
           consistent with those that would be returned if the tile had not
           previously been accessed.  This is not generally possible unless
           the codestream supports dynamic unloading and reloading of
           precincts from the compressed data source, via random access
           pointers, or the existence of a compressed data cache, so calls
           to the function which request this option may well return false.
           They may also return false if any of the precincts have been
           re-parsed or had their contents accessed since the tile was last
           opened, so you may need to close and re-open the tile to get the
           function to return true with this option.
         [ARG: env]
           In a multi-threading environment, the calling thread should supply
           its `kdu_thread_env' reference to protect against dangerous
           race conditions.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT kdu_long
      get_parsed_packet_stats(int component_idx, int discard_levels,
                              int num_layers, kdu_long *layer_bytes,
                              kdu_long *layer_packets=NULL);

      /* [SYNOPSIS]
           This function can be used to recover information about the number
           of compressed bytes (together with packet headers) encountered within
           each quality layer, while parsing the tile.  The information returned
           by the function can be specialized to a particular image component,
           or you can supply a -ve `component_idx' argument to obtain the
           aggregate statistics for all components.  Similarly, the information
           can be specialized to a particular resolution, by identifying a
           non-zero number of highest resolution levels to discard.  The
           information is returned via `layer_bytes' and, optionally,
           `layer_packets' (if you want to know how many packets were actually
           parsed in each quality layer), each of which are arrays with
           `num_layers' entries.  The function only augments the entries
           found in these arrays; rather than initializing them.  This is
           useful, since it allows you to conveniently accumulate results from
           multiple tiles, but it means that the caller must take
           responsibility for initializing their contents to zero at some
           point.
           [//]
           It is most important to realize the limitations of this function.
           Firstly, the function returns information only about those packets
           which have actually been parsed.  It cannot predict ahead of time
           what the statistics are.  So, for example, you might use the function
           after decompressing a tile, in order to determine how much compressed
           data was processed.  If you want to know how much data will be
           processed, before actually decompressing any content, you need to
           pre-parse the tile by calling the `parse_all_relevant_packets'
           member function.  There may be many benefits to doing this, but
           you should be aware that the parsed content will need to be stored
           internally, at least until it is actually decompressed.
           [//]
           In general, you may have already parsed some precincts from the
           tile (or perhaps only some packets of those precincts, if layer
           restrictions have been applied via the
           `kdu_codestream::apply_input_restrictions' function).  You can
           discover how much data has been parsed, by supplying a non-NULL
           `layer_packets' array.  These values may be compared with the
           function's return value, which identifies the total number of
           precincts (and hence the maximum number of packets per layer) in
           the requested component (or all components) and resolutions,
           allowing you to judge whether or not it is worthwhile to call
           the potentially expensive `parse_all_relevant_packets' function.
           [//]
           Another limitation of this function is that it does not directly
           respect any region of interest that you may have specified via
           `kdu_codestream::apply_input_restrictions'.  If you have a limited
           region of interest, that region of interest will generally have
           an effect on the packets which get parsed and so limit the values
           returned via the `layer_bytes' and `layer_packets' arrays.
           However, if the codestream did not support random access into its
           precincts, the system may have had to parse a great many more
           packets than those required for your region of interest.  Moreover,
           if you are working with a persistent codestream, you may have
           changed the region of interest multiple times.  In general, the
           information reported by this function represents the collection of
           all packets parsed so far, except that precincts which were
           unloaded from memory and then reloaded (can happen when working
           with randomly accessible persistent codestreams) are not counted
           twice.
           [//]
           A final thing to be aware of is that if your ultimate source of
           data is a dynamic cache, the number of packets actually available
           at the time of parsing may have been much smaller than the number
           which are available when you call this function.  You can, if you
           like, provoke the `parse_all_relevant_packets' function into
           unloading all precincts of the tile (at least for the resolutions
           and components in question) and parsing again from scratch,
           collecting the relevant statistics as it goes.  Again, though, this
           could be costly, and you can use the statistics reported via the
           `layer_packets' array to determine whether or not it is worthwhile
           for your intended application.
         [RETURNS]
           0 if the operation cannot be performed.  This may occur if the
           codestream was not created for input, or if some argument is illegal.
           Otherwise, the function returns the total number of precincts in
           the image component (or tile if `component_idx' is -ve) within the
           requested resolutions (considering the `discard_levels' argument).
           This is also the maximum number of packets which could potentially
           be parsed within any given quality layer; it may be compared with
           the values returned via a non-NULL `layer_bytes' array.  It is
           worth noting that the return value does not depend in any way upon
           any region of interest which may have been specified via a call to
           `kdu_codestream::apply_input_restrictions'.
         [ARG: component_idx]
           Supply -1 if you want aggregate information for all image components.
           Otherwise, this should be legal codestream component index, ranging
           from 0 to the value returned by `kdu_codestream::get_num_components'.
         [ARG: discard_levels]
           Supply 0 if you want information at full resolution.  A value of 1
           returns the aggregate information for all but the first DWT level
           in the image component (or all components if `component_idx' was
           -ve).  Similarly, a value of 2 returns aggregate information for
           all but the top two DWT levels, and so forth.  If this argument
           exceeds the number of DWT levels in the image component (or all
           components, if `component_idx' was -ve), the function returns 0.
           [//]
           If you supply a -ve value for this argument, it will be treated as
           zero.  This is consistent with the interpretation that the returned
           results should correspond to the packets which are relevant to
           an identified image resolution (hence, including packets found
           at all lower resolutions).
         [ARG: num_layers]
           The number of quality layers for which you want information
           returned.  If this value is larger than the actual number of
           quality layers, the additional entries in the supplied
           `layer_bytes' and `layer_packets' arrays will be untouched.  It
           can also be smaller than the number of actual quality layers, if
           you are only interested in a subset of the available information.
         [ARG: layer_bytes]
           An array with at least `num_layers' entries.  Upon successful return,
           the entries are augmented by the cumulative number of parsed bytes
           (packet header bytes, plus packet body bytes, including any
           SOP marker segments and EPH markers), for the relevant collection
           of image components and resolutions, in each successive quality
           layer.  Note that the number of bytes in any given quality layer
           should be added to the number of bytes in all previous quality
           layers, to determine the total number of bytes associated with
           truncating the representation to the quality layer in question.
           This array may be NULL if you are not interested in the
           number of parsed bytes.
         [ARG: layer_packets]
           Array with at least `num_layers' entries.  Upon successful
           return, the entries are augmented by the number of packets which
           have been parsed for the relevant collection of image components
           and resolutions, in each successive quality layer.  Typically, the
           same number of packets will have been parsed for all quality layers,
           but layer restrictions applied via
           `kdu_codestream::apply_input_restrictions' may alter this situation.
           Also, a tile may have been truncated.  If the ultimate source of
           data is a dynamic cache, the statistics may vary greatly from layer
           to layer here.  This array may be NULL if you are not interested in
           the number of parsed packets.
      */
    KDU_EXPORT kdu_tile_comp
      access_component(int component_idx);
      /* [SYNOPSIS]
           Provides interfaces to specific tile-components within the
           code-stream management machinery.  The interpretation of the
           `component_idx' argument is affected by calls to
           `kdu_codestream::apply_input_restrictions'.  In particular,
           the following rules apply to codestreams which were created for
           input or interchange (i.e., except in the case that `kdu_codestream'
           was created to write to a compressed data target):
           [>>] If the `kdu_codestream::apply_input_restrictions' function has
                been called, with a component `access_mode' equal to
                `KDU_WANT_CODESTREAM_COMPONENTS', the set of visible codestream
                image components may have been restricted and the codestream
                components may even have been reordered (possible with the
                2'nd form).  In this case, `component_idx' refers to the
                apparent codestream index, which addresses the potentially
                restricted and reordered set of visible components.  In this
                case, all calls to this function for which `component_idx'
                lies in the range 0 to N-1, will return a non-empty interface,
                where N is the value returned by `get_num_components'.
           [>>] If the `kdu_codestream::apply_input_restrictions' function
                has never been called, or it is called with a component
                `access_mode' argument of `KDU_WANT_OUTPUT_COMPONENTS', the
                set of visible codestream image components is identical to the
                actual set of components in the underlying codestream.
                However, one or more tile-components might not actually be
                involved in the reconstruction of any visible output image
                component.  Such components may still be addressed by the
                `component_idx' argument to the present function, but the
                return value will be an empty `kdu_tile_comp' interface.
           [//]
           For `output codestreams' (those for which `kdu_codestream' was
           created to write to a compressed data target), this function will
           always return a non-empty interface for all valid component indices.
      */
    KDU_EXPORT float
      find_component_gain_info(int comp_idx, bool restrict_to_interest);
      /* [SYNOPSIS]
           This function is particularly useful for interactive server
           applications, since it allows them to determine the relative
           significance of a codestream component, given the collection
           of output components which are of interest.  The
           `get_mct_block_info' function may be used only to determine the
           collection of components which are of interest, not the degree
           to which each of these contributes.
           [//]
           More specifically, the function efficiently explores the impact
           of a single impulsive distortion sample (of magnitude equal to
           the nominal range -- 2^precision) in the codestream component
           identified by `comp_idx', on the output components produced after
           applying any multi-component transform (Part-2 MCT) or Part-1
           decorrelating transform (RCT or ICT).  These output contributions
           are expressed relative to the nominal ranges of the respective
           output image components and then the sum of the squared
           normalized output distortion contributions is formed and returned
           by the function.  This is what we mean by the transform's energy
           gain.
           [//]
           If `restrict_to_interest' is false, all output contributions are
           considered when computing the gain, regardless of which output
           components are currently visible (see
           `kdu_codestream::apply_input_restrictions') and which of these
           output components are currently of interest (see
           `set_components_of_interest').  The energy gain returned in this
           case is the one which is used during compression to balance the
           bit allocation associated with different codestream components.
           [//]
           If `restrict_to_interest' is true, however, only those output
           component contributions which are currently visible and of interest
           are considered.  This generally results in a smaller energy gain
           term (it cannot result in a larger one) than that obtained when
           `restrict_to_interest' is false.
           [//]
           The ratio between the energy gains returned when
           `restrict_to_interest' is false and when it is true, is a good
           indicator of the relative significance of a codestream component
           during decompression, compared to the significance it was assigned
           during compression.  Kakadu's JPIP server uses this information
           to adjust the relative transmission rates allocated to relevant
           component precincts (JPIP data-bins).
           [//]
           It is worth noting that if the current component access mode is
           `KDU_WANT_CODESTREAM_COMPONENTS', this function invariably returns
           a value of 1.0, since the application is presumably not interested
           in multi-component or RCT/ICT transforms.
      */
  // --------------------------------------------------------------------------
  private: // Interface state
    kd_tile *state;
  };

/*****************************************************************************/
/*                                kdu_tile_comp                              */
/*****************************************************************************/

class kdu_tile_comp {
  /* [BIND: interface]
     [SYNOPSIS]
     Objects of this class are only interfaces, having the size of a single
     pointer.  Copying the object has no effect on the underlying state
     information, but simply serves to provide another interface (or
     reference) to it.  There is no destructor, because destroying
     an interface has no impact on the underlying state.
     [//]
     To obtain a valid `kdu_tile_comp' interface into the internal code-stream
     management machinery, you should call `kdu_tile::access_component'.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle member functions
    kdu_tile_comp() { state = NULL; }
      /* [SYNOPSIS]
         Creates an empty interface.  You should not
         invoke any of the member functions, other than `exists'
         or `operator!' on such an object.
      */
    kdu_tile_comp(kd_tile_comp *state) { this->state = state; }
      /* [SYNOPSIS]
         You should never need to call this function yourself.  It is used
         by the internal codestream management machinery to provide the
         interface with a valid tile-component reference.
      */
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
         Indicates whether or not the interface is empty.  Use
         `kdu_tile::access_component' to obtain a non-empty tile-component
         interface into the internal code-stream management machinery.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
         Opposite of `exists'.  Returns true if the interface is empty. */
  // --------------------------------------------------------------------------
  public: // Data processing/access functions
    KDU_EXPORT bool
      get_reversible();
      /* [SYNOPSIS]
           Returns true if tile-component is to be processed reversibly. */
    KDU_EXPORT void
      get_subsampling(kdu_coords &factors);
      /* [SYNOPSIS]
           Retrieves the horizontal and vertical sub-sampling factors for this
           image component. These are affected by the `transpose' argument
           supplied to `kdu_codestream::change_appearance'.  They are
           also affected by discarded resolution levels, as specified through
           any call to the `kdu_codestream::apply_input_restrictions' function.
           See the description of `kdu_codestream::get_subsampling' for more
           information.
      */
    KDU_EXPORT int
      get_bit_depth(bool internal = false);
      /* [SYNOPSIS]
           If `internal' is false, the function returns the same
           value as `kdu_codestream::get_bit_depth', i.e., the precision
           of the image component samples represented by this tile-component,
           prior to any forward colour transform.
           [//]
           If `internal' is true, the function reports the maximum number
           of bits required to represent the tile-component samples in the
           original image domain, the colour transformed domain and the
           subband domain.  This allows applications to judge the most
           appropriate numerical representation for the data. In the
           reversible path, it is sufficient to employ integers with the
           returned bit-depth.  Otherwise, the returned bit-depth is just
           a guideline -- the use of higher precision representations for
           irreversible path processing will generally improve accuracy.
      */
    KDU_EXPORT bool
      get_signed();
      /* [SYNOPSIS]
           Returns true if the original sample values for this component
           had a signed representation; otherwise, returns false.
      */
    KDU_EXPORT int
      get_num_resolutions();
      /* [SYNOPSIS]
           Returns the total number of available resolution levels,
           which is one more than the total number of accessible DWT
           levels.  Note, however, that the return value is influenced
           by the number of discarded levels supplied in any call to
           `kdu_codestream::apply_input_restrictions'.  The return value
           will be 0 if the current number of discarded levels exceeds the
           number of DWT levels used in compressing the tile-component.
      */
    KDU_EXPORT kdu_resolution
      access_resolution(int res_level);
      /* [SYNOPSIS]
           Returns an interface to a particular resolution level of the
           tile-component, within the internal code-stream management
           machinery.  Generates an error (through `kdu_error') if either of
           the following two conditions hold:
           [>>] The indicated resolution level does not exist.  Specifically,
                `res_level'+`discard_levels' exceeds the number of DWT levels
                for this component, an error will be generated, where
                `discard_levels' is the value supplied in any call to
                `kdu_codestream::apply_input_restrictions', or else 0.
           [>>] The `kdu_codestream::change_appearance' function was used
                to specify geometric flipping, but the decomposition structure
                involves a packet wavelet transform (only possible for a
                Part-2 codestream which involves Arbitrary Decomposition
                Styles -- ADS marker segments), with a specific structure
                which is incompatible with flipping.  Many packet wavelet
                transforms are compatible with flipping, but those which
                horizontally (resp. vertically) split horizontally (resp.
                vertically) high-pass subbands are fundamentally incompatible
                with flipping.
           [//]
           Perhaps the simplest way to avoid the above error conditions is
           to check the values returned by `kdu_codestream::get_min_dwt_levels'
           and `kdu_codestream::can_flip'.
         [ARG: res_level]
           A value of 0 refers to the lowest resolution level, corresponding
           to the LL subband of the tile-component's DWT.  Successively larger
           values for `res_level' return interfaces to successively higher
           resolutions, each essentially twice as wide and twice as high.  Use
           the second form of this overloaded function if you want to access
           the tile-component's highest resolution level.
      */
    KDU_EXPORT kdu_resolution
      access_resolution();
      /* [SYNOPSIS]
           Returns an interface to the highest resolution level of the
           tile-component, within the internal code-stream management
           machinery.  It is equivalent to calling the first form of the
           overloaded `access_resolution' function, supplying a `res_level'
           index equal to the value returned by `get_num_resolutions'.
           [//]
           You can then use `kdu_resolution::access_next' to access
           successively lower resolution levels within the same
           tile-component.
           [//]
           For a discussion of the two error conditions which could occur
           when invoking this function, see the first form of the
           `access_resolution' function.  You can avoid these error conditions
           by checking the values returned by
           `kdu_codestream::get_min_dwt_levels' and `kdu_codestream::can_flip'.
      */
  // --------------------------------------------------------------------------
  private: // Interface state
    kd_tile_comp *state;
  };

/*****************************************************************************/
/*                                kdu_resolution                             */
/*****************************************************************************/

class kdu_resolution {
  /* [BIND: interface]
     [SYNOPSIS]
     Objects of this class are only interfaces, having the size of a single
     pointer.  Copying the object has no effect on the underlying state
     information, but simply serves to provide another interface (or
     reference) to it.  There is no destructor, because destroying
     an interface has no impact on the underlying state.
     [//]
     To obtain a valid `kdu_resolution' interface into the internal
     code-stream management machinery, you should call
     `kdu_tile_comp::access_resolution' or `kdu_resolution::access_next'.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle/identification/navigation member functions
    kdu_resolution() { state = NULL; }
      /* [SYNOPSIS]
         Creates an empty interface.  You should not
         invoke any of the member functions, other than `exists'
         or `operator!' on such an object.
      */
    kdu_resolution(kd_resolution *state) { this->state = state; }
      /* [SYNOPSIS]
         You should never need to call this function yourself.  It is used
         by the internal codestream management machinery to provide the
         interface with a valid resolution reference.
      */
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
         Indicates whether or not the interface is empty.  Use
         `kdu_tile_comp::access_resolution' or `kdu_resolution::access_next'
         to obtain a non-empty resolution interface into the internal
         code-stream management machinery.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
         Opposite of `exists'.  Returns true if the interface is empty. */
    KDU_EXPORT kdu_resolution
      access_next();
      /* [SYNOPSIS]
           Returns an interface to the next lower resolution level.  If this
           is already the lowest resolution level (the one consisting only
           of the relevant DWT's LL subband), the returned interface will
           be empty, meaning that its `exists' member will return false.
      */
    KDU_EXPORT int
      which();
      /* [SYNOPSIS]
           Gets the resolution level index.  This is the same index which must
           be supplied to `kdu_tile_comp::access_resolution' in order to obtain
           the present interface.  0 is the index of the resolution
           level which contains only the LL subband.
      */
    KDU_EXPORT int
      get_dwt_level();
      /* [SYNOPSIS]
           Gets the DWT level index associated with the subbands used to build
           this resolution from the next lower resolution.  If no DWT is used,
           the function returns 0.  Otherwise, the highest resolution level
           returns 1, the next one returns 2 and so forth.  The two lowest
           resolution objects both return the total number of DWT levels,
           since they hold, respectively, the LL subband and the HL,LH,HH
           subbands at the bottom of the DWT decomposition tree.  The
           function is most useful for implementations of the DWT.
           [//]
           Note that the value returned by this function is NOT affected
           by calls to `kdu_codestream::apply_input_restrictions'.
      */
  // --------------------------------------------------------------------------
  public: // Data processing/access member functions
    KDU_EXPORT void
      get_dims(kdu_dims &dims);
      /* [SYNOPSIS]
           Retrieves the apparent dimensions and location of the current
           resolution on the canvas.  The apparent dimensions are affected by
           the geometric transformation flags supplied during any call to
           `kdu_codestream::change_appearance', as well as any region of
           interest which may have been specified through
           `kdu_codestream::apply_input_restrictions'.
           [//]
           Furthermore, the region of interest, as it appears within any given
           resolution level, depends upon the region of support of the DWT
           synthesis kernels.
      */
    KDU_EXPORT void
      get_valid_precincts(kdu_dims &indices);
      /* [SYNOPSIS]
           Returns the range of precinct indices which correspond to the
           current region of interest.  The indices of the first precinct whose
           code-blocks make any contribution to the region of interest are
           returned via `indices.pos'.  Note that these may be negative if
           geometric transformations were specified in any call to
           `kdu_codestream::change_appearance'.  The number of precincts in
           each direction whose code-blocks contribute to the region of
           interest is returned via `indices.size'.
           [//]
           It is worth stressing the fact that the range of indices returned
           by this function refers to precincts whose code-blocks contribute to
           the region of interest during subband synthesis.  This does not
           necessarily mean that the region occupied by the precinct on the
           canvas actually intersects with the region of interest, since
           subband synthesis is a spatially expansive operation.  This
           interpretation of the range of valid indices is particularly
           useful for building servers for interactive client-server
           applications.
           [//]
           The caller should not attempt to attach any interpretation to the
           absolute indices retrieved by this function.  Instead, it should use
           these indices to determine which precincts are to be opened using
           the `open_precinct' function.
      */
    KDU_EXPORT kdu_precinct
      open_precinct(kdu_coords precinct_idx, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This function may be used only with `interchange codestreams'
           (i.e., those created using the version of `kdu_codestream::create'
           which takes neither a compressed data source, nor a compressed data
           target).
           [//]
           Prior to KDU-7.3, this function did not take an `env' argument,
           so it could not safely be used in multi-threaded environments.
           You can now safely open precincts from any of the threads that
           are participating in the thread group to which `env' belongs;
           however, you should be sure also to pass the relevant non-NULL
           `env' pointer to the `kdu_precinct::close' function.
         [ARG: precinct_idx]
           Must lie within the range returned by `get_valid_precincts'.
         [ARG: env]
           You must supply a non-NULL `env' reference if there is any chance
           that this codestream is being simultaneously accessed from another
           thread.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
       */
    KDU_EXPORT kdu_long
      get_precinct_id(kdu_coords precinct_idx);
      /* [SYNOPSIS]
           Returns the same identifier which would be returned if
           `open_precinct' were used to first open a precinct and
           `kdu_precinct::get_unique_id' were invoked on the returned
           interface.  However, this function does not actually open the
           precinct.
         [ARG: precinct_idx]
           Must lie within the range returned by `get_valid_precinct'.
      */
    KDU_EXPORT double
      get_precinct_relevance(kdu_coords precinct_idx);
      /* [SYNOPSIS]
           Returns a floating point quantity in the range 0 to 1, suggesting
           the degree to which the indicated precinct is relevant to the
           region selected using `kdu_codestream::apply_input_restrictions'.
           In particular, the returned value may be interpreted (at least
           roughly) as the fraction of the precinct's subband samples which
           are involved in the reconstruction of the current spatial region
           and resolution of interest, as specified in a call to
           `kdu_codestream::apply_input_restrictions'.  The interpretation of
           the `precinct_idx' argument is identical to that in the
           `open_precinct' and `get_precinct_id' functions.
      */
    KDU_EXPORT int
      get_precinct_packets(kdu_coords precinct_idx,
                           kdu_thread_env *env=NULL,
                           bool parse_if_necessary=true);
      /* [SYNOPSIS]
           Returns the number of code-stream packets which are available
           for the indicated precinct.
           [//]
           The `precinct_idx' argument should lie within the range of indices
           returned via `get_valid_precincts'.
           [//]
           To find out the number of packets which could potentially be
           read for any precinct by this function, you may use the
           `kdu_tile::get_num_layers' function.
         [RETURNS]
           If the codestream was created for output (codestream generation),
           the function will always return the number of quality layers in the
           associated tile.  In practice, however, the function is only of
           real interest when invoked on codestreams created for input
           (processing compressed content).
           [//]
           With an input codestream, this function returns the number of
           packets that have been read for the precinct.  This value is
           explicitly limited to the current number of required quality
           layers, as set by `kdu_codestream::apply_input_restrictions' and
           returned via `kdu_tile::get_num_layers', even if more packets
           have actually been read for the precinct.
         [ARG: env]
           Used only if `parse_if_necessary' is true -- see below for an
           explanation.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
        [ARG: parse_if_necessary]
           It may be necessary for the function to actually parse one or
           more packets in order to determine the answer to this question,
           in which case the parsing will be performed so long as
           `parse_if_necessary' is true.  If `parse_if_necessary' is false,
           the function will only report the number of packets which it can
           be sure are available, which may be limited to the number which
           have already been parsed, depending upon the nature of the
           compressed data source.  In fact, even if a precinct's packets
           have already been parsed, its storage may have subsequently been
           recycled for use elsewhere, so re-parsing is almost invariably
           a good idea, if you really want to know how many packets are
           currently available for the precinct.
           [//]
           Note carefully that when `parse_if_necessary' is true, the
           function's behaviour is not inherently thread safe.  If you are
           using a multi-threaded environment, you should supply the
           appropriate `kdu_thread_env' pointer as the function's second
           argument, so that thread safe execution can be ensured.
      */
    KDU_EXPORT kdu_long
      get_precinct_samples(kdu_coords precinct_idx);
      /* [SYNOPSIS]
           Returns the total number of code-block samples which belong to
           the precinct identified by `precinct_idx'.  This can be 0 if
           the precinct contains no code-blocks -- some precincts in JPEG2000
           can contain no code-blocks yet exist nonetheless, depending upon
           the image dimensions.
           [//]
           The `precinct_idx' argument should lie within the range of indices
           returned via `get_valid_precincts'.
      */
    KDU_EXPORT kdu_node
      access_node();
      /* [SYNOPSIS]
           This function returns an interface to the primary node associated
           with this resolution.  For a discussion of primary nodes, see
           the introductory comments appearing with the definition of
           `kdu_node'.
      */
    KDU_EXPORT int
      get_valid_band_indices(int &min_idx);
      /* [SYNOPSIS]
           This function returns the total number of subbands which belong
           to this resolution, along with the index assocated with the
           first subband index.  This `min_idx' value will be set to 0
           (`LL_BAND') if this is the lowest resolution, in which case the
           function is also guaranteed to return exactly 1.  Otherwise,
           `min_idx' is set to 1 and valid subband indices passed to the
           `access_subband' function must lie in the range 1 to N where
           N is the return value.
           [//]
           In the case of a conventional Mallat subband decomposition
           structure, the valid subband indices for all but the lowest
           resolution will be identical to the values defined by the
           constants `HL_BAND' (horizontally high-pass), `LH_BAND'
           (vertically high-pass) and `HH_BAND' (horizontally and vertically
           high-pass), meaning that `min_idx' is 1 and the function's return
           value is 3.  For more general decomposition structures, however,
           the subband indices do not have any special meaning which you
           will find useful.  However, they are guaranteed to be identical
           to the indices associated with entries in the array of subband
           descriptors returned by `ads_params::get_dstyle_bands'.
      */
    KDU_EXPORT kdu_subband
      access_subband(int band_idx);
      /* [SYNOPSIS]
           Returns an interface to a particular subband within the internal
           code-stream management machinery.
         [ARG: band_idx]
           This argument must lie in the range `min_idx' to `min_idx'+N-1
           where `min_idx' and N are returned by the
           `get_valid_band_indices' function (N is the function's return
           value).  In practice, this means that `band_idx' must be equal to
           `LL_BAND' if `get_res_level' returns 0.  Otherwise, `band_idx'
           must lie in the range 1 to N where N is the total number of
           subbands, returned via `get_valid_band_indices'.
           [//]
           For the conventional Mallat decomposition structure, all but the
           lowest decomposition level have 3 subbands, with indices
           `HL_BAND' (horizontally high-pass), `LH_BAND' (vertically
           high-pass) and `HH_BAND' (horizontally and vertically high-pass).
           For more general decomposition structures, however, the number
           of bands may range anywhere from 1 to 48 and the interpretation of
           the indices is not so obvious.
           [//]
           It should be noted that the `transpose' flag supplied during any
           call to `kdu_codestream::change_appearance' affects the
           interpretation of the `band_idx' argument.  This is done so as
           to ensure that the roles of the HL and LH bands are reversed
           if the codestream is to be presented in transposed fashion.
      */
    KDU_EXPORT bool
      get_reversible();
      /* [SYNOPSIS]
           Returns true if the DWT stage built on this resolution level is
           to use reversible lifting.
      */
    KDU_EXPORT bool
      propagate_roi();
      /* [SYNOPSIS]
           This function returns false if there is no need to propagate ROI
           mask information from this resolution level into its derived
           subbands.  The function always returns false if invoked on an
           input or interchange codestream).  For output codestreams (those
           created using the first form of the overloaded
           `kdu_codestream::create' function), the function's return value
           depends on the `Rlevels' attribute of the relevant `roi_params'
           parameter object set up during content creation.
      */
  // --------------------------------------------------------------------------
  private: // Interface state
    kd_resolution *state;
  };

/*****************************************************************************/
/*                                  kdu_node                                 */
/*****************************************************************************/

#define KDU_NODE_DECOMP_HORZ ((int) 1)
#define KDU_NODE_DECOMP_VERT ((int) 2)
#define KDU_NODE_TRANSPOSED  ((int) 4)
#define KDU_NODE_DECOMP_BOTH (KDU_NODE_DECOMP_HORZ | KDU_NODE_DECOMP_VERT)

class kdu_node {
  /* [BIND: interface]
     [SYNOPSIS]
     Objects of this class are only interfaces, having the size of a single
     pointer.  Copying the object has no effect on the underlying state
     information, but simply serves to provide another interface (or
     reference) to it.  There is no destructor, because destroying
     an interface has no impact on the underlying state.
     [//]
     To obtain a valid `kdu_node' interface into the internal
     code-stream management machinery, you may call
     `kdu_resolution::access_node'.
     [//]
     Nodes are used to represent complex DWT decomposition structures.
     To that end, it is helpful to identify three types of nodes:
     [>>] Primary nodes are associated with image resolutions.  Each image
          resolution has its own node, which may be accessed via
          `kdu_resolution::node'.  The `LL_BAND' child of a primary
          node is the primary node of the next lower resolution.  The
          subbands which belong to a resolution are all descended from
          its primary node, via its other children.  The lowest resolution
          level is a bit different, since its primary node contains only
          an `LL_BAND' child and that child is a leaf node (a subband)
          rather than a primary node.
     [//] Leaf nodes have no children.  They are associated with coded
          subbands.  The subband associated with a leaf node may be
          accessed via the `access_subband' method.
     [//] Intermediate nodes are neither primary nodes nor leaf nodes.
          They appear only in wavelet packet decomposition structures,
          where detail subbands produced by the primary node are further
          decomposed into smaller subbands.  Part-1 of the JPEG2000
          standard does not support packet decomposition structures, while
          Part-2 allows for structures with up to 2 levels of intermediate
          nodes within each resolution level.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle/identification member functions
    kdu_node() { state = NULL; }
      /* [SYNOPSIS]
         Creates an empty interface.  You should not
         invoke any of the member functions, other than `exists'
         or `operator!' on such an object.
      */
    kdu_node(kd_leaf_node *state) { this->state = state; }
      /* [SYNOPSIS]
         You should never need to call this function yourself.  It is used
         by the internal codestream management machinery to provide the
         interface with a valid node reference.
      */
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
         Indicates whether or not the interface is empty.  Use
         `kdu_resolution::access_node' to obtain a non-empty subband
         interface into the internal code-stream management machinery.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
         Opposite of `exists'.  Returns true if the interface is empty. */
    bool compare(const kdu_node &src)
      { return (src.state == this->state); }
      /* [SYNOPSIS]
         Tests whether or not the internal object referenced by this interface
         is identical to that referenced by the `rhs' interface. */
    bool operator==(const kdu_node &rhs)
      { return (rhs.state == this->state); }
      /* [SYNOPSIS] Same as `compare'. */
    bool operator!=(const kdu_node &rhs)
      { return (rhs.state != this->state); }
      /* [SYNOPSIS] Opposite of `compare'. */
  // --------------------------------------------------------------------------
  public: // Structure navigation member functions
    KDU_EXPORT kdu_node
      access_child(int child_idx);
      /* [SYNOPSIS]
           Use this function to probe the descendants of a node.  If
           this is a leaf node, there will be no descendants so all
           calls to this function will return empty interfaces.  Otherwise,
           there will be anywhere from 1 to 4 children.
           [//]
           If the node is split only in the horizontal direction,
           non-empty interfaces will be returned when `child_idx' is one
           of `LL_BAND' or `HL_BAND'.
           [//]
           If the node is split only in the vertical direction,
           non-empty interfaces will be returned when `child_idx' is
           one of `LL_BAND' or `LH_BAND'.
           [//]
           If the node is split in both directions, non-empty interfaces
           will be returned when `child_idx' is one of `LL_BAND',
           `LH_BAND', `HL_BAND' or `HH_BAND'.
           [//]
           In the degenerate case of no splitting at all, only the `LL_BAND'
           child exists.  This can happen for a primary node in a degenerate
           resolution level, whose next resolution has exactly the same
           dimensions -- JPEG2000 Part-2 DFS marker segments can cause this
           to occur if a splitting code happens to be 0.
           [//]
           The `LL_BAND' child of a primary node is always the primary node
           of the next lower resolution object, unless that resolution
           would be the lowest resolution of all (the one with `res_level'=0),
           in which case the `LL_BAND' child is a subband -- i.e., its
           `access_subband' function will return a non-empty interface.  This
           means that you can use the `access_child' and `access_resolution'
           functions as an alternate mechanism for navigating to each of the
           available resolutions in a tile-component, except for the lowest
           resolution.
      */
    KDU_EXPORT int
      get_directions();
      /* [SYNOPSIS]
           You can use this function to discover the directions in which
           decomposition is performed at this node to obtain its children.
           The return value is the logical OR of 3 flags, as follows:
           [>>] `KDU_NODE_DECOMP_HORZ' -- if present, horizontal
                decomposition is employed to generate 2 or 4 children for
                this node (4 if vertical decomposition is employed as well).
           [>>] `KDU_NODE_DECOMP_VERT' -- if present, vertical
                decomposition is employed to generate 2 or 4 children for
                this node (4 if horizontal decomposition is employed as well).
           [>>] `KDU_NODE_TRANSPOSED' -- if present, the node is presented
                with transposition relative to its nominal presentation.  This
                can only happen if `kdu_codestream::change_appearance' has
                been called with `transpose' equal to true.  This information
                is useful when implementing the actual wavelet transform
                analysis or synthesis processing, in the case where
                decomposition is in both directions.  In that case, the
                analysis should ideally be performed in the order horizontal
                then vertical (as opposed to vertical then horizontal),
                while synthesis should be performed in the order vertical
                then horizontal (as opposed to horizontal then vertical).
                This reordering is important only for reversible transforms,
                without which the least significant bits of the result can
                become slightly corrupted.
      */
    KDU_EXPORT int
      get_num_descendants(int &num_leaf_descendants);
      /* [SYNOPSIS]
           Returns the total number of nodes which may be accessed from this
           node by recursively executing the `access_child' function.
           This information is useful in pre-allocating storage to hold
           structural information prior to creating compression/decompression
           machinery.  Note that the returned count does not include the
           current node; for leaf nodes (subbands), therefore, the returned
           value will always be 0.
         [ARG: num_leaf_descendants]
           This argument is used to return the total number of descendant
           nodes which are leaves (subbands).  The count does not include
           the current node, even if it is a leaf node.
      */
    KDU_EXPORT kdu_subband
      access_subband();
      /* [SYNOPSIS]
           Returns an empty interface unless this is a leaf node -- one
           which has no children whatsoever.  Leaf nodes are subbands,
           so for leaf nodes this function returns an interface to the
           relevant subband.
      */
    KDU_EXPORT kdu_resolution
      access_resolution();
      /* [SYNOPSIS]
           Accesses the resolution object to which this node belongs.  If
           this is a primary node, then the returned resolution object's
           `kdu_resolution::access_node' function returns an interface
           to the same underlying resource as the present interface.  This
           may be tested using the `compare' function.  However, it is
           possible that the initial detail subbands created by the
           primary node in a resolution are themselves split into further
           nodes.  If this is one of those nodes, this function returns an
           interface to the resolution which contains the originating
           primary node.  In any event, this function never returns an
           empty interface when presented with a valid non-empty
           node interface.
      */
    KDU_EXPORT void
      get_dims(kdu_dims &dims);
      /* [SYNOPSIS]
           Returns the apparent dimensions and location of the node on the
           canvas.  Note that the apparent dimensions are affected by the
           geometric transformation flags supplied during any call to
           `kdu_codestream::change_appearance', as well as any region of
           interest which may have been specified in the call to
           `kdu_codestream::apply_input_restrictions'. Moreover, the region of
           interest, as it appears within any given node, depends upon the
           region of support of the DWT synthesis kernels.  In particular,
           the subband samples of interest are those which influence the
           reconstruction of the image region of interest.
      */
    KDU_EXPORT int
      get_kernel_id();
      /* [SYNOPSIS]
           Returns one of Ckernels_W5X3, Ckernels_W9X7 or
           Ckernels_ATK (-1).  In the former two cases, the return value
           may be passed directly to `kdu_kernels::init'.  In the last
           case, however, kernel information is stored in an ATK marker
           segment.  In view of this, the only general purpose way to
           recover transform kernel information is to call `get_kernel_info'
           and `get_kernel_coefficients'.
      */
    KDU_EXPORT const kdu_kernel_step_info *
      get_kernel_info(int &num_steps, float &dc_scale, float &nyq_scale,
                      bool &symmetric, bool &symmetric_extension,
                      int &low_support_min, int &low_support_max,
                      int &high_support_min, int &high_support_max,
                      bool vertical);
      /* [SYNOPSIS]
           This function returns most summary information of interest
           concerning the transform kernel used in the vertical or
           horizontal direction within this node, depending on the `vertical'
           argument.  Both kernels may be queried even if horizontal and/or
           vertical decomposition is not actually involved in this node, since
           the kernels are actually the same.  The only potential difference
           between the vertical and horizontal kernel information returned by
           this function may arise due to flipping specifications supplied
           to `kdu_codestream::change_appearance'.  In particular, if
           the view is flipped horizontally, but not vertically (or
           vice-versa), the horizontal and vertical kernels will differ
           if the underlying single kernel is not symmetric.
           [//]
           To complete the information returned by this function, use the
           `get_kernel_coefficients' function to obtain the actual lifting
           step coefficients.  Together, these two functions provide
           sufficient information to initialize a `kdu_kernels' object, if
           desired.  However, this will not normally be necessary, since
           quite a bit of the information which can be produced by
           `kdu_kernels' is also provided here and by the `get_bibo_gains'
           function.
         [RETURNS]
           Array of `num_steps' lifting step descriptors, in analysis order,
           starting from the analysis lifting step which updates the
           odd (high-pass) sub-sequence based on even (low-pass) sub-sequence
           inputs.
         [ARG: num_steps]
           Used to return the number of lifting steps.
         [ARG: dc_scale]
           Used to return a scale factor by which the low-pass subband
           samples must be multiplied after all lifting steps are complete,
           during subband analysis.  For reversible transforms, this factor
           is guaranteed to be equal to 1.0.  For irreversible transforms, the
           scale factor is computed so as to ensure that the DC gain of the
           low-pass analysis kernel is equal to 1.0.
         [ARG: nyq_scale]
           Similar to `dc_scale', but applied to the high-pass subband.  For
           reversible transforms the factor is guaranteed to be 1.0, while for
           irreversible transforms, it is computed so as to ensure that the
           gain of the high-pass analysis kernel at the Nyquist frequency is
           equal to 1.0.
         [ARG: symmetric]
           Set to true if the transform kernel belongs to the whole-sample
           symmetric class defined by Part-2 of the JPEG2000 standard,
           with symmetric boundary extension.  For a definition of this
           class, see `kdu_kernels::is_symmetric'.
         [ARG: symmetric_extension]
           Returns true if each lifting step uses symmetric extension,
           rather than zero-order hold extension at the boundaries.  For
           a definition of this extension policy, see
           `kdu_kernels::get_symmetric_extension'.
         [ARG: low_support_min]
           Lower bound of the low-pass synthesis impulse response's
           region of support.
         [ARG: low_support_max]
           Upper bound of the low-pass synthesis impulse response's
           region of support.
         [ARG: high_support_min]
           Lower bound of the high-pass synthesis impulse response's
           region of support.
         [ARG: high_support_max]
           Upper bound of the high-pass synthesis impulse response's
           region of support.
         [ARG: vertical]
           If true, information returned by this function relates to the
           vertical decomposition process.  Otherwise, it relates to
           horizontal decomposition.  This argument makes a difference
           only if the horizontal and vertical flipping values supplied to
           `kdu_codestream::change_appearance' are different, and the
           kernel is not symmetric.
      */
    KDU_EXPORT const float *
      get_kernel_coefficients(bool vertical);
      /* [SYNOPSIS]
           This function returns the actual lifting step coefficients
           associated with the lifting steps described by
           `get_kernel_info'.  The coefficients associated with the
           first lifting step all preceed those for the second lifting
           step, and so forth.  The actual number of coefficients for
           each lifting step is determined from the
           `kdu_kernel_step_info::support_length' values.
           [//]
           As with `get_kernel_info', this function returns information
           for the vertical decomposition kernel if `vertical' is true,
           else for the horizontal kernel.  There will be a difference
           only if different horizontal and vertical flipping values
           were supplied in the most recent call to
           `kdu_codestream::change_appearance' and the kernel is not
           symmetric.
           [//]
           The coefficients returned here are in exactly the form required
           by `kdu_kernels::init'.
      */
    KDU_EXPORT const float *
      get_bibo_gains(int &num_steps, bool vertical);
      /* [SYNOPSIS]
           This function returns an array containing N+1 entries, where
           N is the value returned via `num_steps'.  The first entry in
           the array always holds the BIBO gain from the original input
           image through to the image which enters this node in the DWT
           decomposition tree, taking all horizontal (`vertical'=false)
           decomposition stages into account OR all vertical (`vertical'=true)
           decomposition stages into account.  The 2D BIBO gain is the
           product of the horizontal and vertical BIBO gains to this node.
           [//]
           If this node is horizontally (resp. vertically) decomposed into
           subbands, the value of N returned via `num_steps' with
           `vertical'=false (resp. true) will be the number of lifting steps
           actually used, and the last N entries in the returned array hold
           the horizontal (resp. vertical) BIBO gain from the input image to
           the output of each successive analysis lifting stage.  Otherwise,
           `num_steps' will be set to 0.
           [//]
           It is worth noting that BIBO gains are calculated based on the
           assumption that irreversible subband transforms are normalized
           to have unit DC gain along the low-pass analysis branch and
           unit Nyquist frequency gain along the high-pass analysis branch.
           Reversible transforms are assumed not to be normalized in any way.
         [RETURNS]
           Array with 1 + `num_steps' BIBO gains.
         [ARG: num_steps]
           Set to the number of lifting steps performed in the horizontal
           or vertical direction (depending on `vertical') to split this
           node into subbands.  The value will be 0 if the node is not
           split in the relevant direction.  The value will always be 0
           for leaf nodes (final subbands).
         [ARG: vertical]
           True if information about vertical decomposition processes is
           being queried.  Otherwise, information about horizontal
           decomposition processes is being queried.  For JPEG2000 Part-1
           codestreams, the results should always be the same in both
           directions.
      */
  // --------------------------------------------------------------------------
  private: // Interface state
    kd_leaf_node *state;
  };

/*****************************************************************************/
/*                                 kdu_subband                               */
/*****************************************************************************/

class kdu_subband {
  /* [BIND: interface]
     [SYNOPSIS]
     Objects of this class are only interfaces, having the size of a single
     pointer.  Copying the object has no effect on the underlying state
     information, but simply serves to provide another interface (or
     reference) to it.  There is no destructor, because destroying
     an interface has no impact on the underlying state.
     [//]
     To obtain a valid `kdu_subband' interface into the internal
     code-stream management machinery, you should call
     `kdu_resolution::access_subband'.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle/identification member functions
    kdu_subband() { state = NULL; }
      /* [SYNOPSIS]
         Creates an empty interface.  You should not
         invoke any of the member functions, other than `exists'
         or `operator!' on such an object.
      */
    kdu_subband(kd_subband *state) { this->state = state; }
      /* [SYNOPSIS]
         You should never need to call this function yourself.  It is used
         by the internal codestream management machinery to provide the
         interface with a valid subband reference.
      */
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
         Indicates whether or not the interface is empty.  Use
         `kdu_resolution::access_subband' to obtain a non-empty subband
         interface into the internal code-stream management machinery.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
         Opposite of `exists'.  Returns true if the interface is empty. */
  // --------------------------------------------------------------------------
  public: // Data processing/access member functions
    KDU_EXPORT int
      get_band_idx();
      /* [SYNOPSIS]
           Returns the index which uniquely identifies this subband within
           its resolution level.  Passing this index to
           `kdu_resolution::access_subband' will return the same subband.
           For more information, see the comments accompanying that
           function.
      */
    KDU_EXPORT kdu_resolution
      access_resolution();
      /* [SYNOPSIS]
           Use this to recover an interface to the resolution to which this
           subband belongs.
      */
    KDU_EXPORT bool
      is_top_level_band();
      /* [SYNOPSIS]
           Returns true if this subband belongs to the highest resolution
           level that is being used.  During decompression, this function is
           affected by calls to `kdu_codestream::apply_input_restrictions'
           since these may cause one or more top resolution levels from
           the original source not to be used.
      */
    KDU_EXPORT kdu_thread_context *
      get_thread_context(kdu_thread_env *env);
      /* [SYNOPSIS]
           Each codestream has an internal `kdu_thread_context'-derived object
           that plays an important role in multi-threaded processing.
           Normally, you should not access this internal thread context.
           This function is provided only to enable the "safe" locking of the
           `KD_THREADLOCK_ROI' mutex to simplify Region-Of-Interest encoding
           operations within the `kdu_encoder' object.  In the future, the
           need to lock an explicit mutex in order to safely access dynamically
           generated ROI information will go away; however, at present there
           is not a very high demand for highly efficient multi-threaded
           ROI encoding machinery.
         [RETURNS]
           The returned `kdu_thread_context' object offers locks with the
           indices `KD_THREADLOCK_GENERAL', `KD_THREADLOCK_PRECINCT' and
           `KD_THREADLOCK_ROI', of which only the last one is probably of
           interest outside the `kdu_codestream' machinery itself.  These
           indices may be passed to `kdu_thread_context::acquire_lock',
           `kdu_thread_context::try_lock' and
           `kdu_thread_context::release_lock'.
         [ARG: env]
           If NULL, this function will always return NULL.  Once a
           codestream is created, it may only be used with `kdu_thread_env'
           objects that belong to a single thread group.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT int
      get_K_max();
      /* [SYNOPSIS]
           Returns the maximum number of magnitude bit-planes for the subband,
           not including any ROI adjustments.  For ROI adjustments, use
           `get_K_max_prime'.
      */
    KDU_EXPORT int
      get_K_max_prime();
      /* [SYNOPSIS]
           Returns the maximum number of magnitude bit-planes for the subband,
           including any ROI adjustments.  To avoid ROI adjustments, use
           `get_K_max'.
           [//]
           K_max_prime - K_max is the ROI upshift value, U, which should
           either be 0 or a value sufficiently large to ensure that the
           foreground and background regions can be separated.  The upshift
           is determined by the `Rshift' attribute of the `rgn_params'
           parameter class.
      */
    KDU_EXPORT bool
      get_reversible();
      /* [SYNOPSIS]
           Returns true if the subband samples are coded reversibly. */
    KDU_EXPORT float
      get_delta();
      /* [SYNOPSIS]
           Returns the quantization step size for irreversibly coded subband
           samples; returns 0 for reversibly coded subbands.
      */
    KDU_EXPORT float
      get_msb_wmse();
      /* [SYNOPSIS]
           Returns the contribution of a single bit error in the most
           significant magnitude bit-plane of any sample in this subband
           to overall image weighted MSE.  The number of magnitude bit-planes
           is returned by `get_K_max_prime'.  The weighted MSE contributions
           are expressed relative to a normalization of the original image
           data in which the sample values all have a nominal range from -0.5
           to +0.5.  This normalization framework is used for both reversibly
           and irreversibly processed samples, so that rate allocation can be
           performed correctly across images containing both reversibly and
           irreversibly compressed tile-components.
           [//]
           NOTE carefully that for codestreams which were opened for input
           (i.e., whose `kdu_codestream::create' function was supplied a
           `kdu_compressed_source' object) this function will invariably
           return 1.0, rather than a meaningful weight.
      */
    KDU_EXPORT bool
      get_roi_weight(float &energy_weight);
      /* [SYNOPSIS]
           Returns true if a special energy weighting factor has been requested
           for code-blocks which contribute to the foreground region of an
           ROI mask.  In this case, the `energy_weight' value is set to the
           amount by which MSE distortions should be scaled.  This value is
           obtained by squaring the `Rweight' attribute from the relevant
           `rgn_params' coding parameter object.
      */
    KDU_EXPORT void
      get_dims(kdu_dims &dims);
      /* [SYNOPSIS]
           Returns the apparent dimensions and location of the subband on the
           canvas.  Note that the apparent dimensions are affected by the
           geometric transformation flags supplied during any call to
           `kdu_codestream::change_appearance', as well as any region of
           interest which may have been specified in the call to
           `kdu_codestream::apply_input_restrictions'. Moreover, the region of
           interest, as it appears within any given subband, depends upon the
           region of support of the DWT synthesis kernels.  In particular,
           the subband samples of interest are those which influence the
           reconstruction of the image region of interest.
      */
    KDU_EXPORT void
      get_valid_blocks(kdu_dims &indices);
      /* [SYNOPSIS]
           Returns the range of valid indices which may be used to access
           code-blocks for this subband.  Upon return, `indices.area' will
           return the total number of code-blocks in the subband.  As with
           all of the index manipulation functions, indices may well turn
           out to be negative as a result of geometric manipulations.
      */
    KDU_EXPORT void
      get_block_size(kdu_coords &nominal_size, kdu_coords &first_size);
      /* [SYNOPSIS]
           Retrieves apparent dimensions for the code-block partition of
           the current subband, whose primary purpose is to facilitate the
           determination of buffer sizes for quantized subband samples
           prior to encoding, or after decoding.  Although the `open_block'
           member function returns a `kdu_block' object which contains
           complete information regarding each block's dimensions and the
           region of interest within the block, this function has profound
           implications for resource consumption and should not be invoked
           until the caller is ready to decode or encode a block of sample
           values.
         [ARG: nominal_size]
           Used to return the dimensions of the code-block partition for
           the subband.  Note that these are apparent dimensions and so they
           are affected by the `transpose' flag supplied in any call to
           `kdu_codestream::change_appearance'.
         [ARG: first_size]
           Used to return the dimensions of the first apparent code-block.
           This is the code-block which has the smallest indices (given by
           the `pos' field of the `indices' structure) retrieved via
           `kdu_subband::get_valid_blocks'.  The actual code-block
           corresponding to these indices may be at the upper left, lower
           right, lower left or upper right corner of the subband, depending
           upon the prevailing geometric view.  Moreover, the dimensions
           returned via `first_size' are also sensitive to the geometric
           view and the prevailing region of interest.  When this first
           code-block is opened using the `kdu_subband::open' function,
           the returned `kdu_block' object should have its `kdu_block::size'
           member equal to the dimensions returned via `first_size',
           transposed if necessary.
      */
    KDU_EXPORT int
      get_block_geometry(bool &transpose, bool &vflip, bool &hflip);
      /* [SYNOPSIS]
           Returns information that identifies the type of operation
           that must be performed during block encoding/decoding, as
           appropriate. All of this information is separately recorded
           within the `kdu_block' object returned in response to a call to
           `open_block', but since the information is true for all blocks
           in the subband, there is an opportunity for objects that work
           with code-block data to optimize their implementation at
           the point of configuration based on the information returned
           here.
         [RETURNS]
           The value recorded in `kdu_block::orientation' for each of
           the subband's code-blocks.
         [ARG: transpose]
           Returns the value recorded in `kdu_block::transpose' for each of
           the subband's code-blocks.
         [ARG: vflip]
           Returns the value recorded in `kdu_block::vflip' for each of
           the subband's code-blocks.
         [ARG: hflip]
           Returns the value recorded in `kdu_block::hflip' for each of
           the subband's code-blocks.
      */
    KDU_EXPORT void
      block_row_generated(int block_height, bool subband_finished,
                          kdu_thread_env *env);
      /* [SYNOPSIS]
           This important function is new to Kakadu version 7.  The
           new `kdu_codestream::auto_flush' function relies upon the
           present function being called once an entire row of code-blocks
           has been generated during compression.  Similar considerations
           apply to transcoding and the `kdu_codestream::auto_trans_out'
           function.  Specifically, a row of code-blocks is considered
           to have been generated once each code-block has been passed back
           to the internal machinery via the `close_block' function.  In
           multi-threaded applications, code-blocks may be generated
           asynchronously by multiple threads, so the code-block generation
           machinery must keep its own internal synchronized count of the
           number of blocks that have been completed on a row, so that
           this function can be called.
           [//]
           If this function does not get called for some reason or other,
           nothing disastrous happens, but `kdu_codestream::auto_flush' and
           `kdu_codestream::auto_trans_out' will not actually generate any
           internal calls to `flush'. This means that all codestream
           content is flushed to the relevant compressed data target
           in the final call to `kdu_codestream::flush'.
         [ARG: block_height]
           This argument identifies the height of each of the code-blocks
           in the row that has just been generated.  This is the code-block
           height, as perceived from outside the `kdu_codestream' object,
           which naturally depends upon any geometric re-orientation
           identified in calls to `kdu_codestream::change_appearance'.
         [ARG: subband_finished]
           If true, all code-blocks in the subband have been generated.
           This saves the internal machinery from figuring out whether or
           not this is true.  Referring to the comments that appear with
           `auto_flush', the internal incremental flush counter is
           not decremented if `subband_finished' is true.
           [//]
           Note: in pathalogical situations where a subband does not have
           any code-blocks at all, you should not call this function at
           all -- i.e., do not think that you have to call the function
           just to indicate that the subband is finished.
         [ARG: env]
           Must be non-NULL if processing is multi-threaded.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT bool
      attach_block_notifier(kdu_thread_queue *client_queue,
                            kdu_thread_env *env);
      /* [SYNOPSIS]
           This important function is new to Kakadu version 7.  It works
           with `advance_block_rows_needed' and `detach_block_notifier'
           in a multi-threaded processing environment -- if `env' is NULL,
           this function will do nothing, returning false.  The intent
           behind these functions is to provide a mechanism whereby
           code-block encoding/decoding machinery can be notified of the
           point at which the relevant processing jobs can proceed without
           encountering conditions that may require them to block upon
           entry to a critical section.
           [//]
           Without this feature, block encoding jobs might need to block
           upon entry to a codestream critical section within which code-block
           containers are allocated to receive encoded bit-streams, while
           block decoding jobs might need to block upon entry to a codestream
           critical section within which content is read and parsed from the
           compressed data source.
           [//]
           When this feature is used, the relevant activities (allocation
           of precinct and code-block resources to receive code-block
           bit-streams, and parsing of codestream contents to recover
           precincts and code-block bit-streams) are performed by background
           processing jobs, which notify the block encoding/decoding thread
           queues that depend upon these activities once they have been
           performed.
           [//]
           Notifications arrive via the `kdu_thread_queue::update_dependencies'
           function whose purpose is to provide a mechanism for queues to
           schedule jobs for processing only when those jobs can be expected
           to run without blocking on unavailable dependencies.
           [//]
           The `client_queue' passed to this function is almost certainly
           an internal block decoding or encoding queue that is associated
           with a `kdu_decoder' or `kdu_encoder' object; however, it can be
           any object derived from `kdu_thread_queue' that is interested in
           scheduling jobs (via `kdu_thread_queue::schedule_jobs') that
           ultimately depend on the availability of resources to store
           code-block bit-streams (compression) or pre-parsed code-block
           bit-streams to decode (decompression) in order for calls to the
           `open_block' or `close_block' function to complete.
           [//]
           The function may return false if no background processing machinery
           is available in the internal implementation, or if the machinery
           associated with this particular subband has been used by
           another `client_queue' -- in this case, the machinery is not
           completely released until the tile is closed and re-opened.
           [//]
           Regardless of the function's return value, however, it is
           always safe to call `advance_block_rows_needed' and always
           safe to invoke `open_block' and `close_block' from within your
           client queue's implementation.  If the background processing
           machinery does not exist or cannot be used, nothing will go
           wrong; instead, `advance_block_rows_needed' will immediately
           invoke the `client_queue->update_dependencies' function to
           indicate that all requested dependencies have been satisfied,
           without actually checking if this is true or not, enticing the
           `client_queue' object to schedule jobs that invoke `open_block'
           or `close_block'.  If the resources or data do not happen to be
           available when `open_block' or `close_block' is called, it may be
           forced to allocate precincts and code-blocks for storing encoded
           bit-streams, or to parse precincts and code-block bit-streams for
           decoding, but it will eventually return with the required content.
           [//]
           The `client_queue->update_dependencies' function is not actually
           invoked from within this function; nor is it invoked from any
           context until at least the first call to
           `advance_block_rows_needed'.  For an explanation of how these
           calls are interpreted, you should consult that function.
         [RETURNS]
           False if background processing machinery will do nothing for the
           supplied `client_queue'.  This is not a fundamental problem,
           as explained above, so it is fine to ignore the return value
           and use `advance_block_rows_needed' anyway.
         [ARG: env]
           If NULL, the function will definitely return false.  The
           function will generate an error through `kdu_error' if `env'
           is non-NULL, but does not belong to the same thread group as
           that previously used with the same `kdu_codestream' interface
           and its descendants.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT void
      advance_block_rows_needed(kdu_thread_queue *client_queue,
                                kdu_uint32 delta_rows_needed,
                                kdu_uint32 quantum_bits,
                                kdu_uint32 num_quantum_blocks,
                                kdu_thread_env *env);
      /* [SYNOPSIS]
           This function is used together with `attach_block_notifier' and
           `detach_block_notifier'.  As noted in the description of
           `attach_block_notifier', you can call the present function all
           by itself, without successfully invoking `attach_notifier',
           in which case the `client_queue->update_dependencies' function is
           invoked immediately to indicate that all requested block
           rows are available, regardless of whether this is true or not.
           As noted with `attach_block_notifier', this is not an error and
           should not cause any problems for an implementation; this is
           because calls to `client_queue->update_dependencies' serve only
           to entice the `client_queue' object into believing that certain
           calls to `open_block' or `close_block' will not have to
           enter a critical section to allocate resources and/or parse
           source data. If this is not entirely true, the calls to `open_block'
           or `close_block' may delay or temporarily block the caller, but
           will not actually fail.
           [//]
           The first argument passed to `client_queue->update_dependencies'
           does not follow the normal conventions described in connection with
           `kdu_thread_queue::update_dependencies'.  Normally, this first
           argument is known as `new_dependencies', but for our purpose here
           we will use the term `p_delta' (read as "progress delta").  The
           `p_delta' value should always be strictly positive.  The
           second argument passed to `client_queue->update_dependencies'
           also has a special interprtation; we will call it the `closure'
           argument here.  The `closure' argument is always 0 except on the
           final call to `client_queue->update_dependencies' when it is
           non-zero (in fact, it is always 1 on the final call).  The
           `client_queue' implementation should generally watch out for this
           special `closure' message because it means that there is no need
           to explicitly call `detach_block_notifier', which could otherwise
           involve an expensive interchange with a background processing
           thread.
           [//]
           To understand the meaning of `p_delta', it helps to define a
           quantity `p_status'=(Rp<<Qbits)+(Cp/Qsize) that is never explicitly
           communicated.  Here, Qbits and Qsize are the values of the
           arguments `quantum_bits' and `num_quantum_blocks', Rp is the number
           of code-block rows, starting from the top of the range returned by
           `kdu_subband::get_valid_blocks', for which dependencies should
           be considered satisfied already.  Cp is the number of extra
           code-blocks (starting from the left edge of the next row
           after the first Rp code-block rows), for which dependencies
           should be considered satisfied already.
           [//]
           Evidently, the purpose of Qsize is to quantize the block status
           information so that (Cp / Qsize) is strictly less than 2^Qbits;
           this way, Rp and Cp occupy distinct bit positions within the
           `p_status' integer.
           [//]       
           Finally, we define another quantity max_Rp, that is the
           maximum value of Rp for which information should be passed to
           `client_queue->update_dependencies' at the present time.  The
           value of `max_Rp' is initialized to 0 by `attach_block_notifier'
           and incremented by `delta_rows_needed' each time this function is
           called.
           [//]
           We are now in a position to describe the `p_delta' values passed to
           `client_queue->update_dependencies'.  The `p_delta' values are
           nothing other than the sequence of increments in the quantity
                 [>>] `p_trunc_status' = min{ p_status, (max_Rp<<Qbits) }
           [//]
           These increments result from changes either to `max_Rp' or to
           the values of `Rp' or `Cp'.
           [//]
           You may wonder why we communicate everthing through differences
           (`delta_rows_needed' and `p_delta') rather than as absolute
           quantities.  Ultimately the reason for this choice is that it
           allows both the internal codestream management machinery and the
           encoding and decoding machinery to keep all related quantities
           within an individual 32-bit synchronized variable, using only one
           atomic update operation.  Absolute quantities such as
           `p_trunc_status', might appear to change non-monotonically in calls
           to `client_queue->update_dependencies', because they can arrive on
           different threads.  The `p_delta' increments, however, will
           always be non-negative, no matter how individual threads might
           get delayed.
           [//]
           In the above discussion, `p_status' is derived from Rp and Cp,
           which represent the number of whole code-block "rows", plus the
           number of extra code-blocks on the next row that are to be
           "considered" available.  The internal machinery can and should
           do various things to regulate the rate at which code-blocks
           are "considered" to be available.  In particular, the
           background processing machinery schedules progressive changes in
           the value of `p_status' so `notify_client->update_dependencies'
           should never receive `p_delta' values that exceed 2^Qbits (i.e.,
           one "row" of code-blocks).  If more blocks are available, the
           internal machinery schedules additional updates to occur after all
           other pending updates have occurred.  This helps `kdu_encoder' and
           `kdu_decoder' objects to achieve a near optimal scheduling
           sequence for block encoding/decoding jobs.
         [ARG: client_queue]
           If this argument does not correspond to the queue attached to
           the subband via a successful call to `attach_block_notifier',
           `client_queue->update_dependencies' will be called immediately
           (synchronously, from within this function) with a `p_delta'
           value of `delta_rows_needed'<<4.  This is the behaviour one would
           expect if all code-blocks had been parsed or allocated already,
           to that Rp was effectively equal to the number of code-block
           rows in the subband, right from the outset.  While this might
           not be true, it entices the block encoding/decoding machinery
           into believing that all required dependencies have been satisfied,
           so that subsequent calls to `open_block' or `close_block' may
           need to enter a critical section to perform the relevant parsing
           or resource allocation, but nothing will break.
           [//]
           If `client_queue' is the one attached to the background processing
           machinery, calls to `client_queue->update_dependencies' will be
           more meaningful and may arrive from within the current function
           and/or from other threads.
         [ARG: delta_rows_needed]
           As explained above, this argument effectively increments an
           internal `max_Rp' variable that starts out at 0 when
           `attach_block_notifier' is called, and is one of the two factors
           that determine the `p_delta' values passed to
           `client_queue->update_dependencies'.  For decoding applications,
           it is strongly recommended that calls to this function
           supply `delta_rows_needed'=1.  If the ultimate intent is to
           advance the background parsing status at a faster rate, it
           is best for all relevant subbands to supply their increments one
           at a time in round-robin fashion until all increments have been
           supplied, since this usually results in the best scheduling order
           for block decoding jobs.  For encoding applications, this is less
           important.
         [ARG: quantum_bits]
           This is the value of the `Qbits' variable that is used to determine
           the interpretation of the `delta_p' values passed to
           `client_queue->update_dependencies'.  Specifically, the least
           significant `quantum_bits' bits of the `delta_p' values are used
           to communicate changes in the number of quanta for which resources
           are available, where each row of code-blocks is divided into at
           most 2^`quantum_bits' quanta.  See the `advance_block_rows_needed'
           function for more information on quanta and the `delta_p' values
           passed to `client_queue->update_dependencies'.
           [//]
           The value passed for this argument should be the same in all
           calls to this function.
         [ARG: num_quantum_blocks]
           This is the value of the `Qsize' variable that is used to determine
           the interpretation of the `delta_p' values passed to
           `client_queue->update_dependencies'.  Specifically, each row of
           code-blocks for the subband is partitioned into quanta, all but
           the last of which should consist of `num_quantum_blocks'
           code-blocks.  The last quantum may consist of fewer code-blocks,
           so long as `num_quantum_blocks' << `quantum_bits' is at least as
           large as the number of code-blocks across the subband, as measured
           with respect to the current appearance geometry -- see
           `kdu_codestream::change_appearance'.  For more on who this value is
           used consult the `advance_block_rows_needed' function.
           [//]
           The value passed for this argument should be the same in all
           calls to this function.
         [ARG: env]
           If this argument is NULL (or if `client_queue' is NULL), the
           function will return immediately without doing anything at all.
           Otherwise, `env' must belong to the same thread group as that
           previously used with the same `kdu_codestream' interface and its
           descendants; if not, an error will be generated through
           `kdu_error'.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT bool
      detach_block_notifier(kdu_thread_queue *client_queue,
                            kdu_thread_env *env);
      /* [SYNOPSIS]
           Each call to `attach_block_notifier' should ultimately be matched
           with a call to `detach_block_notifier', EXCEPT in the event that
           a call to `client_queue->update_dependencies' has been received
           with a non-zero second argument (this is the `closure' argument,
           discussed in connection with `advance_block_rows_needed').
           [//]
           Normally, the `client_queue' object watches for a non-zero
           `closure' value in calls to its `update_dependencies' function.
           If it encounters such a call, there is no need to issue future
           calls to `advance_block_rows_needed' or `detach_block_notifier';
           however, it is not unsafe to issue such calls.
           [//]
           In view of the above, calls to `detach_block_notifier' will
           normally be made only to force premature detachment of the
           `client_queue' from the internal background processing machinery.
           To avoid complex race conditions that might ensue as a result
           of calls that an application might make to `kdu_tile::close'
           after the `client_queue' invokes its `all_done' method, the
           internal machinery does not usually detach the `client_queue'
           immediately; instead, the present function usually returns false
           meaning that a future call to `client_queue->update_dependencies'
           has been scheduled to mark the actual detachment, and that
           function will be made with `p_delta' and `closure' arguments of
           0 and 1, respectively.  This combination is not otherwise
           meaningful or possible; upon receipt of such a call the
           `client_queue->update_dependencies' implementation will normally
           invoke `client_queue->all_done'.
           [//]
           In any event, if this function returns false, it is not safe
           for the caller to invoke `client_queue->all_done' itself.
           [//]
           If `client_queue' does not match the object that the internal
           machinery considers to be attached to any background processing
           machinery for the subband, the function does nothing, returning
           true.
           [//]
           If you do fail, for some reason, to match a call to
           `attach_block_notifier' with one to `detach_block_notifier',
           even though a closure call to `client_queue->update_dependencies'
           has not been received, any background processing machinery
           will only remain "attached" up until the point at which the tile is
           closed (see `kdu_tile::close'), or the codestream is passed to
           `kdu_thread_env::cs_terminate'.  During exception handling,
           `kdu_thread_entity::handle_exception' should normally be invoked
           somewhere; this causes any background processing machinery to be
           permanently halted -- it will then be cleaned up naturally within
           `kdu_codestream::destroy'.  However, during exception processing,
           you should always make sure that the `kdu_thread_entity::destroy'
           function is invoked by the group owner before any of the
           codestreams that are actively using the thread group are
           destroyed.  A convenient way of doing this is to make sure that
           thread group owners always invoke
           `kdu_thread_entity::handle_exception' when they catch an
           exception, since this function destroys the thread group,
           terminating all of its worker threads, if invoked by the group
           owner.
         [RETURNS]
           True if the supplied `client_queue' was not actually attached
           to the background processing machinery (it might never have been
           attached or it might have been automatically detached after all
           possible notifications had been delivered) or if the detachment
           processing completed immediately.
           [//]
           Returns false if detachment processing is deferred, in which case
           the caller must not directly invoke `client_queue->all_done', but
           must wait instead for the `client_queue->udpate_dependencies'
           function to be called with arguments 0 and -1 before the `all_done'
           call.  This sort of thing is only likely to occur when a custom
           `kdu_thread_queue'-derived processing object is responding to
           a premature termination request initiated by
           `kdu_thread_queue::request_termination'.  In fact, an efficient
           `client_queue' implementation will only wind up calling this
           function under such circumstances, because it will otherwise
           normally detect the special closure call to
           `client_queue->update_dependencies'.
         [ARG: env]
           This argument MUST NOT be NULL; it MUST refer to a thread in the
           same thread group as that used with `attach_block_notifier'.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT kdu_block *
      open_block(kdu_coords block_idx, int *return_tpart=NULL,
                 kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Code-block access is bracketed by calls to `open_block' and
           `close_block'. It is currently illegal to have more than one
           code-block open per-thread at the same time.  In fact, the
           underlying structure manages storage for a single `kdu_block'
           object, in the main `kdu_codestream' machinery, and one for
           each thread within a `kdu_thread_env' environment.
           [//]
           It is also illegal to open any given block a second time without
           first closing the tile (input codestreams) or the relevant
           precinct (interchange codestreams) and re-opening it.  Moreover,
           such activities are permitted only with interchange codestreams
           or input codestreams which have been set up for persistence by
           calling the `kdu_codestream::set_persistent' function.
           [//]
           This restriction on multiple openings of any given code-block is
           important for the efficiency of the system, since it allows the
           internal machinery to determine when resources can be destroyed,
           recycled or temporarily swapped out of memory.  Applications which
           do wish to open blocks multiple times will generally be happy to
           close and re-open the tile (or precinct) anyway.
           [//]
           The returned `kdu_block' object contains all necessary
           information for encoding or decoding the code-block, including its
           dimensions, coding mode switches and so forth.
           [//]
           For input codestreams (created using the particular form of the
           overloaded `kdu_codestream::create' function which takes a
           compressed data source), the block will have all coding pass and
           code-byte information intact, ready to be decoded.
           [//]
           Otherwise (output or interchange codestreams), all relevant members
           of the returned `kdu_block' object will be initialized to the empty
           state, ready to accept newly generated (or copied) code bytes.
           [//]
           It is worth noting that opening a block may have far reaching
           implications for the internal code-stream management machinery.
           Precincts may be created for the first time; packets may be read
           and de-sequenced; tile-parts may be opened and so forth.  There is
           no restriction on the order in which code-blocks may be opened,
           although different orders can have very different implications
           for the amount of the code-stream which must be buffered
           internally, especially if the compressed data source does not
           support seeking, or the code-stream does not provide seeking
           hints (these are optional JPEG2000 marker segments).
           [//]
           Although this function may be used with interchange codestreams,
           its namesake in the `kdu_precinct' interface is recommended
           instead for interchange applications.  For more on this, consult
           the comments appearing with `kdu_precinct::open_block'
           and `kdu_precinct::close_block'.
         [ARG: block_idx]
           Must provide indices inside the valid region identified by the
           `get_valid_blocks' function.
         [ARG: return_tpart]
           For input codestreams, this argument (if not NULL) is used to
           return the index of the tile-part to which the block belongs
           (starting from 0).  For interchange and output codestreams,
           a negative number will usually be returned via this argument if it
           is used.
         [ARG: env]
           You must supply a non-NULL `env' reference if there is any chance
           that this codestream (or any other codestream with which it shares
           storage via `kdu_codestream::share_buffering') is being
           simultaneously accessed from another thread.  When you do supply a
           non-NULL `env' argument, the returned block is actually part of the
           local storage associated with the thread itself, so each thread may
           have a separate open block.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
    KDU_EXPORT void
      close_block(kdu_block *block, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Recycles the `kdu_block' object recovered by a previous call to
           `kdu_subband::open_block' to the internal code-stream management
           machinery, for later re-use.
           [//]
           For interchange and output codestreams, the function also causes
           the compressed data to be copied into more efficient internal
           structures until we are ready to generate the final code-stream
           (or code-stream packets).
           [//]
           Although this function may be used with interchange codestreams,
           its namesake in the `kdu_precinct' interface is recommended
           instead for interchange applications.  For more on this, consult
           the comments appearing with `kdu_precinct::open_block'
           and `kdu_precinct::close_block'.
         [ARG: env]
           If the block was opened with a non-NULL `env' reference, exactly
           the same `env' argument must be supplied here.
      */
    KDU_EXPORT kdu_uint16
      get_conservative_slope_threshold();
      /* [SYNOPSIS]
           This function is provided for block encoders to estimate the
           number of coding passes which they actually need to process, given
           that many of the coding passes may end up being discarded during
           rate allocation.
           [//]
           The function returns a conservatively estimated
           lower bound to the distortion-length slope threshold which will be
           used by the PCRD-opt algorithm during final rate allocation
           (usually inside `kdu_codestream::flush').  The coder is responsible
           for translating this into an estimate of the number of coding
           passes which must be processed.  The function returns 1 if there
           is no information available on which to base estimates -- such
           information will be available only if
           `kdu_codestream::set_max_bytes' or
           `kdu_codestream::set_min_slope_threshold' has been called.
      */
  // --------------------------------------------------------------------------
  private: // Interface state
    kd_subband *state;
  };

/*****************************************************************************/
/*                                 kdu_precinct                              */
/*****************************************************************************/

class kdu_precinct {
  /* [BIND: interface]
     [SYNOPSIS]
     Objects of this class are only interfaces, having the size of a single
     pointer.  Copying the object has no effect on the underlying state
     information, but simply serves to provide another interface (or
     reference) to it.  There is no destructor, because destroying
     an interface has no impact on the underlying state.
     [//]
     To obtain a valid `kdu_precinct' interface into the internal
     code-stream management machinery, you must call
     `kdu_resolution::open_precinct', which is available only for
     interchange codestreams.
  */
  // --------------------------------------------------------------------------
  public: // Lifecycle/identification member functions
    kdu_precinct() { state = NULL; }
      /* [SYNOPSIS]
         Creates an empty interface.  You should not
         invoke any of the member functions, other than `exists'
         or `operator!' on such an object.
      */
    kdu_precinct(kd_precinct *state) { this->state = state; }
      /* [SYNOPSIS]
         You should never need to call this function yourself.  It is used
         by the internal codestream management machinery to provide the
         interface with a valid precinct reference.
      */
    bool exists() { return (state==NULL)?false:true; }
      /* [SYNOPSIS]
         Indicates whether or not the interface is empty.  Use
         `kdu_resolution::open_precinct' to obtain a non-empty precinct
         interface into the internal code-stream management machinery.
      */
    bool operator!() { return (state==NULL)?true:false; }
      /* [SYNOPSIS]
         Opposite of `exists'.  Returns true if the interface is empty. */
    KDU_EXPORT bool check_loaded();
      /* [SYNOPSIS]
           Returns false unless all code-blocks belonging to the precinct
           have been loaded.  Code-blocks are loaded by opening them using
           `kdu_precinct::open_block' or `kdu_subband::open_block', filling
           in the relevant data, and then closing them using
           `kdu_precinct::close_block' or `kdu_subband::close_block'.  A
           precinct's blocks become unloaded once the `kdu_precinct::close'
           function has been called.  Currently, this is the only condition
           which unloads them, but we may well introduce other unloading
           conditions in the future.   For example, it may make sense to
           unload all precincts which belong to a newly opened tile, but do
           not belong to the current region of interest, as specified in
           calls to `kdu_codestream::apply_input_restrictions'.  By making
           sure you check whether or not the precinct is loaded before
           attempting to generate or simulate packet data, you can
           avoid any dependence on such policies.
      */
    KDU_EXPORT kdu_long get_unique_id();
      /* [SYNOPSIS]
           The return value from this function uniquely identifies the
           precinct amongst all precincts associated with the compressed
           image representation.  This unique identifier is identical to that
           used by the `kdu_compressed_source::set_precinct_scope' function
           to recover a precinct's packets from a cached compressed data
           source.  The formula used to determine the unique identifier is
           described with that function.  This connection facilitates the
           recovery of packet data in a server which may then be transmitted
           to a caching client in client-server applications.
      */
    KDU_EXPORT bool
      get_valid_blocks(int band_idx, kdu_dims &indices);
      /* [SYNOPSIS]
           Returns false if the precinct does not belong to a resolution level
           which includes the band indicated by `band_idx'.  Otherwise,
           configures the `indices' argument to identify the range of indices
           which may be used to access code-blocks beloning to this precinct
           within the indicated subband.  If this range of indices is non-empty,
           the function returns true; otherwise, it returns false.
           [//]
           Upon return, `indices.area' yields the total number of code-blocks
           in the subband which belong to the present precinct and
           `indices.pos' identifies the coordinates of the first (top left)
           block in the precinct-band.  As with all of the index manipulation
           functions, indices may well turn out to be negative as a result of
           geometric manipulations.
           [//]
           As for `kdu_subband::get_valid_blocks'the interpretation of the
           `band_idx' argument is affected by the `transpose' argument
           supplied in any call to `kdu_codestream::change_appearance'.
           [//]
           It is important to understand two distinctions between this
           function and its namesake, `kdu_subband::get_valid_blocks'
           interface.  Firstly, the range of returned code-block indices
           is restricted to those belonging to the precinct; and secondly,
           the range of indices includes all code-blocks belonging to the
           precinct, even if some of these do not lie within the current
           region of interest, as established by
           `kdu_codestream::apply_input_restrictions' function.  This
           is important, because a precinct is not considered loaded until
           all of its code-blocks have been written, regardless of whether
           or not they lie within the current region of interest.
         [ARG: band_idx]
           This argument has the same interpretation as its namesake in the
           `kdu_resolution::access_subband' function.
      */
    KDU_EXPORT kdu_block *
      open_block(int band_idx, kdu_coords block_idx, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This function plays the same role as its namesakes,
           `kdu_subband::open_block', except that the `block_idx' coordinates
           lie within the range returned by `kdu_precinct::get_valid_blocks',
           rather than that returned by `kdu_subband::get_valid_blocks'.
           As mentioned in the description of `kdu_precinct::get_valid_blocks'
           above, this range may actually be larger than the range of block
           indices which are admissible for the `kdu_subband::open_block'
           function.  This is because the range includes all code-blocks in
           the precinct-band, even though only some of the precinct's
           code-blocks contribute to the region of interest.
           [//]
           After opening a code-block using this function, the caller fills
           in its contents with compressed code bytes and other summary
           information, exactly as described in connection with the
           `kdu_subband::open_block' function.  The block must then be closed
           using `close_block' (preferably), or `kdu_subband::close_block',
           before another can be opened.
           [//]
           For `interchange codestreams', the `open_block' and `close_block'
           functions defined here are recommended for opening and closing
           code-blocks instead of those provided by `kdu_subband', both
           because they allow all of the precinct's blocks to be opened and
           also because they are somewhat more efficient in their
           implementation.  Nevertheless, the `kdu_subband::open_block' and
           `kdu_subband::close_block' functions may be used.  In fact, it is
           permissible to open a block using `kdu_precinct::open_block' and
           close it using `kdu_subband::close_block' or to open it using
           `kdu_subband::open_block' and close it using
           `kdu_precinct::close_block'.
         [ARG: band_idx]
           This argument has the same interpretation as its namesake in the
           `kdu_resolution::access_subband' function.
       */
    KDU_EXPORT void
      close_block(kdu_block *block, kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           Closes a code-block opened using `kdu_precinct::open_block' (or
           possibly `kdu_subband::open_block').  See the discussion appearing
           with those functions for more information.
      */
    KDU_EXPORT bool
      size_packets(int &cumulative_packets, int &cumulative_bytes,
                   bool &is_significant);
      /* [SYNOPSIS]
           This function may be used by server applications to determine an
           appropriate number of packets to deliver to a client in a single
           batch.  Applications interact with the `size_packets' and
           `get_packets' functions in an alternating fashion.
           [//]
           The application first calls `size_packets' one or more times to
           determine the number of bytes associated with a certain number of
           packets or vice-versa (see below).  It then calls `get_packets' to
           actually generate the data bytes from a range of packets.
           Thereafter, the application may go back to sizing packets again
           and so forth.  Between sizing and generating packet data, the
           internal packet coding state machinery is reset, which incurs some
           computational effort.  For this reason, it is desirable to keep
           the number of switches between sizing and generating packet data
           to a minimum.
           [//]
           During a single sizing session (i.e., after the last generation
           session, if any, and before the next one), the internal machinery
           keeps track of the number of packets which have been sized already.
           It also keeps track of the total number of bytes represented by
           these sized packets.  If the number of sized packets is less than
           the value of `cumulative_packets' on entry, or if the total size
           of all packets sized so far (starting from the first packet of the
           precinct) is less than `cumulative_bytes', the function sizes one
           or more additional packets.  This continues until the number of
           sized packets reaches or exceeds the value of `cumulative_packets'
           and the total number of bytes associated with these sized packets
           reaches or exceeds the value of `cumulative_bytes'.  The function
           returns after setting the value of `cumulative_packets' to reflect
           the total number of sized packets and the value of
           `cumulative_bytes' to reflect the total number of bytes associated
           with all packets sized so far.
           [//]
           Once the number of sized packets reaches the number of apparent
           quality layers, as set by any call to
           `kdu_codestream::apply_input_restrictions', the function will
           return without attempting to size further packets.
           [//]
           The behaviour described above may be exploited in a variety of
           ways.  If a server has already transmitted some number of packets
           to a client, the number of bytes associated with these packets may
           be determined by setting `cumulative_bytes' to 0 and setting
           `cumulative_packets' equal to the number of packets which have been
           transmitted.  Alternatively, if the server remembers the number of
           bytes which have been transmitted, this may be converted into a
           number of packets by calling the function with `cumulative_bytes'
           equal to the number of transmitted bytes and `cumulative_packets'
           equal to 0.  The server may then use the function to determine the
           number of additional layers which can be transmitted, subject to
           a given size restriction.  The results of such simulations may be
           used to drive the `get_packets' function.  In this way, it is
           possible to implement memory efficient servers whose state
           information consists of as little as a single 16-bit integer
           (sufficient for packets consisting of only a few blocks) for each
           precinct, representing the number of bytes actually transmitted
           for that precinct's packets.
           [//]
           Remember, that after generating packet data with `get_packets',
           sizing calls start from scratch with the first packet in the
           precinct.
         [ARG: is_significant]
           Set to false if the packets and bytes represented by the
           `cumulative_packets' and `cumulative_bytes' arguments upon
           return represent one or more significant code-block samples.
           Otherwise, they represent only the signalling bytes required
           to indicate that nothing is significant yet.
         [RETURNS]
           False if one or more of the precinct's code-blocks have yet to
           be loaded (`check_loaded' would return false); in this case,
           the function does nothing else.
      */
    KDU_EXPORT bool
      get_packets(int leading_skip_packets, int leading_skip_bytes,
                  int &cumulative_packets, int &cumulative_bytes,
                  kdu_output *out);
      /* [SYNOPSIS]
           This function plays a complementary role to that of `size_packets',
           except that it actually generates packet data, rather than just
           simulating and sizing packets.  As with that function, packets
           are generated in sequence and the function may be called multiple
           times.  The first call sets the state of the internal machinery to
           indicate that packets are now being generated, rather than sized,
           and it resets internal counters identifying the number of packets
           which have been generated so far and the cumulative number of bytes
           associated with those packets. In each call to the function,
           zero or more additional packets are generated until the total
           number of packets which have been generated so far reaches or
           exceeds the value of `cumulative_packets' and the number of bytes
           associated with these packets reaches or exceeds the value of
           `cumulative_bytes'.
           [//]
           Each generated packet is exported to the supplied `kdu_output'
           derived object, `out', except in the event that the packet index
           (starting from 0) is less than the value of `leading_skip_packets'
           or the cumulative number of bytes associated with all packets
           generated so far is less than the value of `leading_skip_bytes'.
           Note carefully, that each packet is either discarded or written
           to `out' as a whole.
           [//]
           On exit, the values of `cumulative_packets' and `cumulative_bytes'
           are adjusted to reflect the total number of packets which have been
           generated and the total number of bytes associated with these
           packets, regardless of whether these packets were all written out
           to the `out' object.
           [//]
           If further sizing is performed after some packet data has been
           generated using the current function, sizing starts from scratch
           with the first packet again.  Any subsequent generation of packet
           data then also starts from scratch, so that the same packets may
           be generated multiple times.  The principle purpose of the
           `leading_skip_bytes' argument is to skip over previously generated
           packets when the application must ping-pong between sizing and
           generating data.
           [//]
           The curious reader may well wonder at this point how the function
           determines which code-block contributions should belong to which
           packet.  This decision is based on the conventions described with
           the `kdu_block::pass_slopes' member array.  In particular,
           the function assumes that code-block contributions which are
           assigned a pass slope of 0xFFFF-q belong to quality layer q, where
           the quality layer indices start from q=0.  In most cases, the
           code-block data will be recovered from a separate input codestream,
           in which case the pass slope values will be set up to
           have exactly this interpretation already.
         [RETURNS]
           False if one or more of the precinct's code-blocks have yet to
           be loaded (`check_loaded' would return false); in this case,
           the function does nothing else.
         [ARG: out]
           This argument can be NULL, in which case output is discarded.
      */
    KDU_EXPORT void restart();
      /* [SYNOPSIS]
           Call this function only after all code-blocks have been loaded
           into the precinct (`check_loaded' returns true) to force the packet
           sizing and/or packet data generation processes associated with
           `size_packets' and `get_packets' to start from scratch (start from
           the first layer of the packet) when they are next called.  Calls
           to this function are generally required only when we need to
           backtrack through previously generated packets and start sizing
           again.  As already described above, restarts occur implicitly
           when `get_packets' is first called after sizing packets or when
           `size_packets' is first called after generating packet data with
           `get_packets'.
      */
    KDU_EXPORT void close(kdu_thread_env *env=NULL);
      /* [SYNOPSIS]
           This function may be called as soon as all packet data of interest
           have been recovered using the `get_packets' member function.  All
           compressed data will be destroyed immediately once this function has
           been called.  The precinct may be re-opened, but its code-blocks
           must be re-loaded before any packet data can be extracted again.
           Note that precincts are NOT automatically closed when their
           containing tile is closed.
         [ARG: env]
           You must supply a non-NULL `env' reference if there is any chance
           that this codestream (or any other codestream with which it shares
           storage via `kdu_codestream::share_buffering') is being
           simultaneously accessed from another thread.
           [//]
           Note that a non-NULL `env' pointer is unique to the calling
           thread -- in most cases `env' points to the `kdu_thread_env' object
           that was explicitly created and the calling thread is the
           thread-group owner (the one that created it, unless you have
           explicitly invoked `kdu_thread_entity::change_group_owner_thread').
           Otherwise, `env' identifies a worker thread that was added to
           the thread group -- in this case, the worker thread is executing
           a scheduled job from which this function is being called.
      */
  private:
    kd_precinct *state;
  };

/*****************************************************************************/
/*                                   kdu_block                               */
/*****************************************************************************/

struct kdu_block {
  /* [BIND: reference]
     [SYNOPSIS]
     This structure is used for intermediate storage of code-block
     information.  Compressed code-blocks are stored in a much more efficient
     form, which is translated to and from the form in this structure by
     the `kdu_subband::open_block', `kdu_precinct::open_block',
     `kdu_subband::close_block' and `kdu_subband::close_block' functions.
     The present form of the structure is designed to facilitate block
     coding activities.
     [//]
     Unlike `kdu_codestream', `kdu_tile', `kdu_tile_comp', `kdu_resolution',
     `kdu_subband' and `kdu_precinct', this structure is not simply an
     interface to the internal machinery.  It provides publically accessible
     member functions for efficient manipulation by block encoder and
     decoder implementations.
     [//]
     To obtain a pointer to a `kdu_block' structure, you will need to use
     one of the functions, `kdu_subband::open_block' (most common) or
     `kdu_precinct::open_block' (usually for interactive servers).  Do not
     construct your own instance of `kdu_block'!
     [//]
     The `kdu_block' structure manages shared storage for encoders and
     decoders.  Specifically, the `sample_buffer' and `context_buffer' arrays
     are provided for the benefit of the block encoding and decoding
     implementations.  Their size is not automatically determined by the
     code-block dimensions.  Instead, it is the responsibility of the block
     coding implementation to check the size of these arrays and augment them,
     if necessary, using the `set_max_samples' and `set_max_contexts'
     functions.
   */
  // --------------------------------------------------------------------------
  private: // Member functions
    friend struct kd_codestream;
    friend class kdu_thread_env;
    kdu_block();
    ~kdu_block();
  public: // Member functions
    int get_max_passes()
      { /* [SYNOPSIS] Retrieves the public `max_passes' member.  This is
          useful for language bindings which offer only function interfaces. */
        return max_passes;
      }
    KDU_EXPORT void
      set_max_passes(int new_passes, bool copy_existing=true);
      /* [SYNOPSIS]
           This function should be called to augment the number of
           coding passes which can be stored in the structure.  For
           efficiency, call the function only when you know that there
           is a potential problem with the current allocation, by testing
           the public data member, `max_passes'.
         [ARG: new_passes]
           Total number of coding passes required; function does nothing if
           sufficient storage already exists for these coding passes.
         [ARG: copy_existing]
           If true, any existing coding pass information will be copied
           into newly allocated (enlarged) arrays.
      */
    int max_passes;
      /* [SYNOPSIS]
         Current size of the `pass_lengths' and `pass_slopes' arrays.
      */
    // ------------------------------------------------------------------------
    int get_max_bytes()
      { /* [SYNOPSIS] Retrieves the public `max_bytes' member.  This is
          useful for language bindings which offer only function interfaces. */
        return max_bytes;
      }
    KDU_EXPORT void
      set_max_bytes(int new_bytes, bool copy_existing=true);
      /* [SYNOPSIS]
           This function should be called to augment the number of
           code bytes which can be stored in the structure.  For
           efficiency, call the function only when you know that there
           is a potential problem with the current allocation, by testing
           the public data member, `max_bytes'.
           [//]
           An encoder may check the available storage between coding passes
           to minimize overhead, exploiting the fact that there is a
           well-defined upper bound to the number of bytes which the MQ
           coder is able to generate in any coding pass.
         [ARG: new_bytes]
           Total number of code bytes for which storage is required; function
           does nothing if sufficient storage already exists.
         [ARG: copy_existing]
           If true, any existing code bytes will be copied into the newly
           allocated (enlarged) byte buffer.
      */
    int max_bytes;
      /* [SYNOPSIS]
         Current size of the `byte_buffer' array.
      */
    kdu_byte *byte_buffer;
      /* [SYNOPSIS]
         This array is allocated in such a way as to allow access to
         elements with indices in the range -1 through `max_bytes'-1.
         This can be useful when working with the MQ encoder.
         [//]
         Use `max_bytes' and `set_max_bytes' to make sure this array
         contains sufficient elements.  Never allocate the array yourself.
      */
    // ------------------------------------------------------------------------
    void set_max_samples(int new_samples)
      { 
        if (max_samples >= new_samples) return;
        if (blk_sizes[2] < new_samples) allocate_mem_blk(2,new_samples);
        max_samples = blk_sizes[2];
        sample_buffer = blk_aligned[2];
      }
      /* [SYNOPSIS]
           This function should be called to augment the number of
           code-block samples which can be stored in the `sample_buffer'
           array.  You should call this function if the `max_samples' value
           is currently too small. 
      */
    int max_samples;
      /* [SYNOPSIS]
         Current size of the `sample_buffer' array.
      */
    kdu_int32 *sample_buffer;
      /* [SYNOPSIS]
         Not used by the internal code-stream management machinery itself,
         this array provides a convenient shared resource for the benefit
         of block encoder and decoder implementations.  Use `max_samples'
         and `set_max_samples' to make sure the array contains sufficient
         elements.  Never allocate the array yourself.
         [//]
         From v7.0 onwards, this buffer is guaranteed to be aligned on a
         32-byte boundary to facilitate efficient memory transfers.
         [//]
         Note that `context_buffer' and `sample_buffer' may both share a
         single allocated block of memory, so calls to either
         `set_max_samples' or `set_max_contexts' may invalidate any pointer
         variable that you may have set equal to this member variable.
      */
    // ------------------------------------------------------------------------
    void set_max_contexts(int new_contexts)
      { 
        if (max_contexts >= new_contexts) return;
        if (blk_sizes[1] < new_contexts) allocate_mem_blk(1,new_contexts);
        max_contexts = blk_sizes[1];
        context_buffer = blk_aligned[1];
      }
      /* [SYNOPSIS]
           This function should be called to augment the number of
           elements which can be stored in the `context_buffer'
           array.  You should call this function if the `max_contexts' value
           is too small.
      */
    int max_contexts;
      /* [SYNOPSIS]
         Current size of the `context_buffer' array.
      */
    kdu_int32 *context_buffer;
      /* [SYNOPSIS]
         Not used by the internal code-stream management machinery itself,
         this array provides a convenient shared resource for the benefit
         of block encoder and decoder implementations.  Use `max_contexts'
         and `set_max_contexts' to make sure the array contains sufficient
         elements.  Never allocate the array yourself.
         [//]
         Note that `context_buffer' and `sample_buffer' may both share a
         single allocated block of memory, so calls to either
         `set_max_samples' or `set_max_contexts' may invalidate any pointer
         variable that you may have set equal to this member variable.
      */
    // ------------------------------------------------------------------------
    int map_storage(int contexts, int samples, int retained_state)
      { 
        int len0 = contexts + ((8-contexts) & 7); // Multiple of 8 integers
        int len1 = samples + ((8-samples) & 7); // Multiple of 8 integers
        int result = blk0_retained_state;
        if (blk_sizes[0] < (len0+len1))
          { allocate_mem_blk(0,len0+len1); result=0; }
        kdu_int32 *buf = blk_aligned[0];
        if ((len0 != 0) && (context_buffer != buf))
          { context_buffer=buf; max_contexts=0; result=0; }
        buf += len0;
        if ((len1 != 0) && (sample_buffer != buf))
          { sample_buffer=buf; max_samples=0; result=0; }
        blk0_retained_state = retained_state;
        return result;
      }
      /* [SYNOPSIS]
         This function is intended to improve memory localization and help
         avoid the need for some unnecessary initialization operations.  It
         is intended for use by block encoding and decoding machinery.
         [//]
         To understand this function, you should first realize that the
         `max_contexts' and `max_samples' members identify the
         currently allocated sizes of distinct blocks of memory to which
         the `context_buffer' and `sample_buffer' members point.
         The present function is able to arrange for either or both of these
         members to point into a separate contiguous block of memory.  When
         this happens, the corresponding byte limit is set to 0, so that
         parts of an application that expect to be able to independently
         adjust the size of the context buffer, sample buffer or byte buffer
         will be forced to invoke `set_max_contexts' or `set_max_samples',
         as appropriate.
         [>>] If `contexts' is non-zero, the `context_buffer' member
              is made to point to the start of the contiguous block of memory
              associated with this function and `max_contexts' is set to 0.
              Space is set aside for `contexts' 32-bit integers to reside
              within the contiguous block of memory, aligned on a 32-byte
              boundary.
         [>>] If `samples' is non-zero, the `sample_buffer' member is made
              to point into the contiguous block of memory associated with
              this function and `max_samples' is set to 0.  Space is set
              aside for `samples' 32-bit integers to reside within the
              contiguous block of memory and the `sample_buffer' is aligned
              on a 32-byte boundary.
         [//]
         Note that `contexts' is special, in that any remapped
         `context_buffer' member always points to the start of the contiguous
         block of memory associated with this function.  If `contexts' is 0,
         this role is taken by the sample buffer.
         [//]
         The function also records any `retained_state' value internally,
         returning the value last recorded by a call to this function, or
         else 0.  The value 0 is returned if the function has never been
         called, if the contiguous block of memory needed to be reallocated
         as a result of this call, or if the call caused either the
         `context_buffer' or `sample_buffer' pointer to be
         changed.  Typically the caller uses `retained_state' and the
         function's return value to determine whether some portion of the
         `context_buffer' can be used without re-initialization.
      */
  // --------------------------------------------------------------------------
  public: // Parameters controlling the behaviour of the encoder or decoder
    kdu_coords size;
      /* [SYNOPSIS]
         Records the dimensions of the code-block.  These are the
         true dimensions, used by the block encoder or decoder -- they
         are unaffected by regions of interest or geometric transformations.
      */
    kdu_coords get_size()
      { /* [SYNOPSIS] Retrieves the public `size' member.  This is useful for
           language bindings which offer only function interfaces. */
        return size;
      }
    void set_size(kdu_coords new_size)
      { /* [SYNOPSIS] Sets the public `size' member.  This is useful for
           language bindings which offer only function interfaces. */
        size = new_size;
      }
    kdu_dims region;
      /* [SYNOPSIS]
         Identifies the region of interest inside the block.  This
         region is not corrected for any prevailing geometric transformations,
         which may be in force.  `region.pos' identifies the upper left
         hand corner of the region, relative to the upper left hand corner
         of the code-block.  Thus, if the region of interest is the entire
         code-block, `region.pos' will equal (0,0) and `region.size' will
         equal `size', regardless of the geometric transformation flags
         described.
      */
    kdu_dims get_region()
      { /* [SYNOPSIS] Retrieves the public `region' member.  This is useful for
           language bindings which offer only function interfaces. */
        return region;
      }
    void set_region(kdu_dims new_region)
      { /* [SYNOPSIS] Sets the public `region' member.  This is useful for
           language bindings which offer only function interfaces. */
        region = new_region;
      }
    // ------------------------------------------------------------------------
    bool transpose;
      /* [SYNOPSIS]
         Together, the `transpose', `vflip' and `hflip' fields identify any
         geometric transformations which need to be applied to the
         code-block samples after they have been decoded, or before they are
         encoded.  These quantities are to be interpreted as follows:
         [>>] During decoding, the block is first decoded over its full size.
              If indicated `region' is then extracted from the block and
              transformed as follows.  If `transpose' is true, the extracted
              region is first transposed.  After any such transposition, the
              `vflip' and `hflip' flags are used to flip the region in the
              vertical and horizontal directions, respectively.  Note that all
              of these operations can be collapsed into a function which copies
              data out of the `region' of the block into a buffer used for the
              inverse DWT.
         [>>] During encoding, the region of interest must necessarily be
              the entire block.  The `vflip' and `hflip' flags control
              whether or not the original sample data is flipped vertically
              and horizontally.  Then, if `transpose' is true, the
              appropriately flipped sample block is transposed.  Finally,
              the transposed block is encoded.
      */
    bool get_transpose()
      { /* [SYNOPSIS] Retrieves the public `transpose' member.  This is useful
           for language bindings which offer only function interfaces. */
        return transpose;
      }
    void set_transpose(bool new_transpose)
      { /* [SYNOPSIS] Sets the public `transpose' member.  This is useful for
           language bindings which offer only function interfaces. */
        transpose = new_transpose;
      }
    // ------------------------------------------------------------------------
    bool vflip;
      /* [SYNOPSIS] See description of `transpose'. */
    bool get_vflip()
      { /* [SYNOPSIS] Retrieves the public `vflip' member.  This is useful
           for language bindings which offer only function interfaces. */
        return vflip;
      }
    void set_vflip(bool new_vflip)
      { /* [SYNOPSIS] Sets the public `vflip' member.  This is useful for
           language bindings which offer only function interfaces. */
        vflip = new_vflip;
      }
    // ------------------------------------------------------------------------
    bool hflip;
      /* [SYNOPSIS] See description of `transpose'. */
    bool get_hflip()
      { /* [SYNOPSIS] Retrieves the public `hflip' member.  This is useful
           for language bindings which offer only function interfaces. */
        return hflip;
      }
    void set_hflip(bool new_hflip)
      { /* [SYNOPSIS] Sets the public `hflip' member.  This is useful for
           language bindings which offer only function interfaces. */
        hflip = new_hflip;
      }
    // ------------------------------------------------------------------------
    int modes;
      /* [SYNOPSIS]
         Holds the logical OR of the coding mode flags.  These flags may
         be tested using the `Cmodes_BYPASS', `Cmodes_RESET', `Cmodes_RESTART',
         `Cmodes_CAUSAL', `Cmodes_ERTERM', `Cmodes_SEGMARK',
         `Cmodes_BYPASS_E1' and `Cmodes_BYPASS_E2' macros defined in
         "kdu_params.h".  For more information on the coding modes, see the
         description of the `Cmodes' attribute offered by the `cod_params'
         parameter class.
      */
    int get_modes()
      { /* [SYNOPSIS] Retrieves the public `modes' member.  This is useful
           for language bindings which offer only function interfaces. */
        return modes;
      }
    void set_modes(int new_modes)
      { /* [SYNOPSIS] Sets the public `modes' member.  This is useful for
           language bindings which offer only function interfaces. */
        modes = new_modes;
      }
    // ------------------------------------------------------------------------
    int orientation;
      /* [SYNOPSIS]
         Holds one of LL_BAND, HL_BAND (horizontally high-pass),
         LH_BAND (vertically high-pass) or HH_BAND.  The subband
         orientation affects context formation for block encoding and
         decoding.  The value of this member is unaffected by the geometric
         transformation flags, since it has nothing to do with apparent
         dimensions.
      */
    int get_orientation()
      { /* [SYNOPSIS] Retrieves the public `orientation' member. This is useful
           for language bindings which offer only function interfaces. */
        return orientation;
      }
    void set_orientation(int new_orientation)
      { /* [SYNOPSIS] Sets the public `orientation' member.  This is useful for
           language bindings which offer only function interfaces. */
        orientation = new_orientation;
      }
    // ------------------------------------------------------------------------    
    bool resilient; // Encourages a decoder to attempt error concealment.
      /* [SYNOPSIS]
         Encourages a block decoder to attempt to locate and conceal errors
         in the embedded bit-stream it is decoding.  For a discussion of how
         this may be accomplished, the reader is referred to Section 12.4.3
         in the book by Taubman and Marcellin.
         [//]
         For more information regarding the interpretation of the resilient
         mode, consult the comments appearing with the declaration of
         `kdu_codestream::set_resilient'.
      */
    bool fussy;
      /* [SYNOPSIS]
         Encourages a block decoder to check for compliance, generating an
         error message through `kdu_error' if any problems are detected.
         [//]
         For more information regarding the interpretation of the resilient
         mode, consult the comments appearing with the declaration of
         `kdu_codestream::set_fussy'.
      */
    int K_max_prime;
      /* [SYNOPSIS]
         Maximum number of magnitude bit-planes for the subband, including
         ROI adjustments.  See `kdu_subband::get_K_max_prime' for further
         discussion of this quantity.
      */
  // --------------------------------------------------------------------------
  public: // Data produced by the encoder or consumed by the decoder
    int missing_msbs;
      /* [SYNOPSIS]
         Written by the encoder; read by the decoder.  For an explanation of
         this quantity, refer to Section 8.4.2 in the book by Taubman and
         Marcellin.
      */
    int get_missing_msbs()
      { /* [SYNOPSIS] Retrieves the public `missing_msbs' member.  This is
          useful for language bindings which offer only function interfaces. */
        return missing_msbs;
      }
    void set_missing_msbs(int new_msbs)
      { /* [SYNOPSIS] Sets the public `missing_msbs' member.  This is useful
           for language bindings which offer only function interfaces. */
        missing_msbs = new_msbs;
      }
    //-------------------------------------------------------------------------
    int num_passes;
      /* [SYNOPSIS]
         Written by the encoder; read by the decoder.  Number of coding
         passes generated (encoder) or available (decoder), starting from
         the first bit-plane after the initial `missing_msbs' bit-planes.
      */
    int get_num_passes()
      { /* [SYNOPSIS] Retrieves the public `num_passes' member.  This is
          useful for language bindings which offer only function interfaces. */
        return num_passes;
      }
    void set_num_passes(int new_passes)
      { /* [SYNOPSIS] Sets the public `num_passes' member.  This is useful
           for language bindings which offer only function interfaces. */
        num_passes = new_passes;
      }
    int *pass_lengths;
      /* [SYNOPSIS]
         Entries are written by the encoder; read by the decoder.  The
         first entry holds the total number of code bytes associated with
         the first coding pass of the first bit-plane following the initial
         `missing_msbs' empty bit-planes.  Subsequent entries identify
         lengths of the incremental contributions made by each consecutive
         coding pass to the total embedded bit-stream for the code-block.
         [//]
         Use `max_passes' and `set_max_passes' to make sure this array
         contains sufficient elements.  Do not allocate the array yourself.
      */
    void get_pass_lengths(int *buffer)
      { /* [SYNOPSIS] Transfers all elements of the public `pass_lengths'
           array to the supplied `buffer', which must be allocated sufficiently
           large to accommodate all `num_passes' coding passes.  The function
           is useful for language bindings which offer only function
           interfaces. */
        for (int i=0; i < num_passes; i++) buffer[i] = pass_lengths[i];
      }
    void set_pass_lengths(int *buffer)
      { /* [SYNOPSIS] Transfers all elements of the supplied `buffer' to the
           public `pass_lengths' array, which must be allocated sufficiently
           large to accommodate all `num_passes' coding passes.  The function
           is useful for language bindings which offer only function
           interfaces. */
        for (int i=0; i < num_passes; i++) pass_lengths[i] = buffer[i];
      }
    // ------------------------------------------------------------------------
    kdu_uint16 *pass_slopes;
      /* [SYNOPSIS]
         The entries in this array must be strictly decreasing, with the
         exception that 0's may be interspersed into the sequence.
         [>>] When used for compression, the entries in this array usually
              hold a suitably normalized and shifted version of log(lambda(z)),
              where lambda(z) is the distortion-length slope value for any
              point on the convex HULL of the operational distortion-rate
              curve for the code-block (see Section 8.2 of the book by Taubman
              and Marcellin for more details).  Zero values correspond to
              points not on the convex HULL.  The final coding pass
              contributed by any given code-block to any quality layer can
              never have a zero-valued slope.
         [>>] When used for transcoding, or with interchange codestreams
              (i.e., when the internal code-stream management machinery was
              created using the particular form of the overloaded
              `kdu_codestream::create' function which takes neither a
              compressed data source, nor a compressed data target), the
              entries of the `pass_slopes' array should be set equal to
              0xFFFF minus the index (starting from 0) of the quality layer
              to which the coding pass contributes, except where a later
              coding pass contributes to the same quality layer, in which
              case the `pass_slopes' entry should be 0.  For more details,
              consult the comments appearing with the definition of
              `kdu_codestream::trans_out' and `kdu_precinct::get_packets'.
         [>>] When used for input, the `pass_slopes' array is filled out
              following exactly the rules described above for transcoding.
         [//]
         Use `max_passes' and `set_max_passes' to make sure this array
         contains sufficient elements.  Never allocate the array yourself.
      */
    void get_pass_slopes(int *buffer)
      { /* [SYNOPSIS] Transfers all elements of the public `pass_slopes'
           array to the supplied `buffer', which must be allocated sufficiently
           large to accommodate all `num_passes' coding passes.  The function
           is useful for language bindings which offer only function
           interfaces. */
        for (int i=0; i < num_passes; i++) buffer[i] = (int) pass_slopes[i];
      }
    void set_pass_slopes(int *buffer)
      { /* [SYNOPSIS] Transfers all elements of the supplied `buffer' to the
           public `pass_slopes' array, which must be allocated sufficiently
           large to accommodate all `num_passes' coding passes.  Note that
           coding pass slopes must be in the range 0 to 65535.  The function
           is useful for language bindings which offer only function
           interfaces. */
        for (int i=0; i < num_passes; i++)
          pass_slopes[i] = (kdu_uint16) buffer[i];
      }
    // ------------------------------------------------------------------------
    void get_buffered_bytes(kdu_byte *buffer, int first_idx, int num_bytes)
      { /* [SYNOPSIS] Transfers `num_bytes' elements of the public
           `byte_buffer' array to the supplied `buffer', starting from the
           element with index `first_idx' (indices as low as -1 may legally
           be used).  The function is useful for language bindings which
           offer only function interfaces. */
        for (int i=0; i < num_bytes; i++)
          buffer[i] = byte_buffer[i+first_idx];
      }
    void set_buffered_bytes(kdu_byte *buffer, int first_idx, int num_bytes)
      { /* [SYNOPSIS] Transfers `num_bytes' elements from the supplied `buffer'
           to the public `byte_buffer' array, starting at the `byte_buffer'
           element with index `first_idx' (indices as low as -1 may legally
           be used).  The function is useful for language bindings which
           offer only function interfaces. */
        for (int i=0; i < num_bytes; i++)
          byte_buffer[i+first_idx] = buffer[i];
      }
  // --------------------------------------------------------------------------
  public: // State information shared by all encoders/decoders
    bool errors_detected;
      /* [SYNOPSIS]
         True if a warning message has already been issued by the block
         decoder when operating in resilient mode -- saves a massive number
         of warnings from being delivered when a heavily corrupted code-stream
         is encountered.
      */
    bool insufficient_precision_detected;
      /* [SYNOPSIS]
         True if a warning message has already been issued by the block encoder
         to indicate that the 32-bit implementation has insufficient
         precision to represent ROI foreground and background regions
         completely losslessly.
      */
  // --------------------------------------------------------------------------
  public: // Members used to collect statistics
    int start_timing()
      {
      /* [SYNOPSIS]
           If the block encoder or decoder supports the gathering of timing
           statistics, it should do so by calling this function at the start
           of a timing loop and `finish_timing' at the end of the loop; the
           number of times to execute the loop is the return value from the
           function.
      */
        if (cpu_iterations == 0) return 1;
        timer.reset();
        return cpu_iterations;
      }
    void finish_timing()
      { /* [SYNOPSIS] See `start_timing'. */
        if (cpu_iterations == 0) return;
        cpu_time += timer.get_ellapsed_seconds();
        cpu_unique_samples += size.x*size.y;
      }
    void initialize_timing(int iterations)
      { /* [SYNOPSIS]
             Used to configure the gathering of timing stats.  A value of 0
             for the `iterations' argument means nothing will be timed and
             the block encoder or decoder should execute only once.  For
            more information see `kdu_codestream::set_timing_stats'. */
        assert(iterations >= 0);
        this->cpu_iterations = iterations;
        cpu_time = 0.0;
        cpu_unique_samples = 0;
      }
    double get_timing_stats(kdu_long &unique_samples, double &time_wasted)
      {
      /* [SYNOPSIS]
           The return value is the calculated number of seconds required to
           process `unique_samples' once.  The `time_wasted' argument returns
           the number of CPU seconds wasted by iterating over a timing loop
           so as to reduce the impact of the calls to `clock' on the final
           time.
      */
        unique_samples = cpu_unique_samples;
        double once_time = cpu_time;
        if (cpu_iterations > 1) once_time /= cpu_iterations;
        time_wasted = cpu_time - once_time;
        return once_time;
      }
  // --------------------------------------------------------------------------
  private: // State members used by the above functions
    int *pass_handle; // Handle for deallocating the block of memory used to
      // store `pass_lengths' and `pass_slopes' arrays.  NULL if these
      // buffers are not dynamically allocated.
    kdu_byte *byte_handle; // Handle for deallocating `byte_buffer'.
    kdu_int32 *blk_handles[3]; // Entry 0 used by `map_storage'; others used by
    kdu_int32 *blk_aligned[3]; // `set_max_contexts' and `set_max_samples'.
    int blk_sizes[3]; // Available aligned 32-bit ints in above blocks
    int blk0_retained_state; // See `map_storage'
    int cpu_iterations; // 0 unless the block coder is to be timed.
    kdu_clock timer;
    double cpu_time;  
    kdu_long cpu_unique_samples; // timed samples=unique samples*num iterations
  // --------------------------------------------------------------------------
  private: // Helper functions used to manage `blk_...' members.
    KDU_EXPORT void allocate_mem_blk(int which, int new_size);
      /* Manages storage associated with `blk_handles', `blk_aligned' and
         `blk_sizes'.  `which' is one of 0, 1 or 2, identifying which block
         is being allocated.  Deallocates any existing block of storage as
         determined by `blk_handles[which]'. */
    void donate_storage(kdu_byte *blk, int blk_bytes)
      { 
        if ((blk_aligned[0] != NULL) || (blk_bytes < 4096)) return;
        int offset = ((-_addr_to_kdu_int32(blk)) & 31);
        blk += offset;  blk_bytes -= offset;  blk_bytes &= ~31;
        blk_aligned[0] = (kdu_int32 *) blk;
        blk_sizes[0] = blk_bytes >> 2;
      }
      /* This function is used to contribute an initial block of storage for
         use by the `map_storage' function.  This storage will not be
         deallocated, but the `map_storage' function may need to allocate
         a new block of storage that will be deallocated when the object is
         destroyed.  The supplied block of storage will not be used if any
         memory has already been allocated by the `map_storage' function.
         The `blk' supplied here need not have any particular alignment
         attributes.  This function is usually invoked automatically from
         within `kdu_thread_env::donate_stack_block'. */
  // --------------------------------------------------------------------------
  private: // Navigation information installed by `kdu_subband::open_block'.
    friend class kdu_subband;
    friend class kdu_precinct;
    kd_precinct *precinct; // Precinct to which the block belongs.
    kd_block *block; // Internal block associated with this object.
  // --------------------------------------------------------------------------
  private: // Internal blocks of storage for typically smaller arrays
    int internal_pass_lengths[96]; // Should be more than enough
    kdu_uint16 internal_pass_slopes[96]; // Should be more than enough
    kdu_byte internal_byte_buffer[16384]; // Usually sufficient
  };

/*****************************************************************************/
/*                               kdu_thread_env                              */
/*****************************************************************************/

#define KDU_TRANSFORM_THREAD_DOMAIN  "Transform-Thread-Domain"
#define KDU_CODING_THREAD_DOMAIN     "Block-Coder-Thread-Domain"
#define KDU_CODESTREAM_THREAD_DOMAIN "Codestream-Background-Thread-Domain"

class kdu_thread_env : public kdu_thread_entity {
  /* [BIND: reference]
     [SYNOPSIS]
       Forenote: In Version 7.0, Kakadu's multi-threaded system has been
       totally re-designed from the ground up.  As far as possible, this has
       been done in a way that will have little impact on applications.
       However, you need will need to note the following changes:
       [>>] `kdu_thread_queue' objects may now be constructed by the
            application and passed to `kdu_thread_entity::attach_queue',
            although it is also possible to obtain internally created
            thread queues using a `kdu_thread_entity::add_queue' function
            that is backward compatible with that found in earlier versions
            of Kakadu so long as the first argument is NULL (which it almost
            always was at the application level).
       [>>] The `kdu_thread_entity::synchronize' function has essentially been
            replaced by `kdu_thread_entity::join' (although the meaning is
            not quite the same).
       [>>] There is a special `kdu_thread_env::cs_terminate' function that
            should be called prior to `kdu_codestream::destroy' or
            `kdu_codestream::restart' to terminate background processing
            activity that may be associated with a codestream.  This is not
            something that was required in the past.
       [>>] The new system has much better memory management, allowing you
            to work with any number of codestreams concurrently, some
            of which might share their memory resources through
            `kdu_codestream::share_buffering', while some might be
            configured for input and others for output, without any of
            the complex memory management considerations for shared
            thread groups that applied in previous versions of Kakadu.
       [>>] The new system should yield much better utilization of
            parallel processing resources where more than 2 CPU cores are
            available, having a lighter-weight distributed scheduling
            algorithm and mechanisms that encourage the design of
            processing modules that do work only when they know they will
            not have to block on dependencies.
       [//]
       This object is required for multi-threaded processing within a
       single `kdu_codestream'.  The main reason why this may be interesting
       is to exploit the availability of multiple physical processors.
       Kakadu's implementation goes to quite some effort to minimize thread
       blocking and avoid cache coherency bottlenecks which might arise in
       such multi-processor environments, so as to keep all processors active
       close to 100% of the time.  To do this, you need to first create
       a group of working threads, following the steps below:
       [>>] Create an instance of the `kdu_thread_env' class, either on the
            stack (local variable) or on the heap (using `new').
       [>>] Use the base member function, `kdu_thread_entity::add_thread' to
            add as many additional working threads to the group as you see
            fit.  A good strategy is to wind up with at least one thread
            for each physical processor.
       [//]
       Once you have created the thread group, it is important that you
       let the `kdu_codestream' machinery know about it.  The best way to
       do this is by passing a `kdu_thread_env' reference to the relevant
       `kdu_codestream::create' function, although you can wait until you
       first call `kdu_codestream::open_tile' or a related function before
       supplying a `kdu_thread_env' reference; this might be important if
       you want to open a codestream before committing to the construction of
       a multi-threaded environment.
       [//]
       You should be aware that each `kdu_codestream' object maintains a
       special derived version of the `kdu_thread_context' class that
       maintains safe synchronization objects and special memory management
       structures.  This codestream-specific thread context, may actually
       create its own thread queue (see `kdu_thread_queue') and schedule
       background processing jobs within a special background work domain.
       You do not directly interact with these queues.  What is important
       is that you do not invoke `kdu_codestream::destroy' until all
       multi-threaded processing within the codestream has stopped.  To
       do this, you must either supply the codestream in a call to the
       current object's `cs_terminate' function, or else you must issue
       a global call (one with no `root_queue') to `kdu_thread_entity::join'
       or `kdu_thread_entity::terminate', or destroy the multi-threaded
       processing environment using the inherited `kdu_thread_entity::destroy'
       function.
       [//]
       You may pass a `kdu_thread_env' reference into any of Kakadu's
       processing objects.  In the core system, these include
       all the objects declared in "kdu_sample_processing.h" and
       "kdu_block_coding.h", notably
       [>>] `kdu_multi_analysis' and `kdu_multi_synthesis';
       [>>] `kdu_analysis' and `kdu_synthesis'; and
       [>>] `kdu_encoder' and `kdu_decoder'.
       [//]
       Other higher level support objects, such as those defined in the
       "apps/support" directory also should be able to accept a thread group,
       so as to split their work across the available threads.
       [//]
       It is usually a good idea to create a top-level `kdu_thread_queue'
       object for each tile, or each tile-component (if you are scheduling
       the processing of tile-components yourself) which you want to process
       concurrently.  This is most easily done using the base member
       `kdu_thread_entity::add_queue', but `kdu_thread_entity::attach_queue'
       may be used for more sophisticated derived `kdu_thread_queue' objects.
       You can then pass these queue into the above objects when they are
       constructed, so that any internally created job queues will be added
       as sub-queues (or sub-sub-queues) of the supplied top-level queue.
       There are many benefits to the queue hierarchies formed in this way,
       as described in connection with the `kdu_thread_entity::attach_queue'
       function.  Most notably, the `kdu_thread_entity::join' function allows
       you to wait upon the completion of all work within a queue and all of
       its sub-queues, detaching them from the thread group once this happens.
       [//]
       As for termination and exceptions, this is what you need to know:
       [>>] If you handle an exception thrown from any context which involves
            your `kdu_thread_env' object, you should call the base member
            function `kdu_thread_entity::handle_exception'.  Actually, it is
            safe to call that function any time at all -- in case you have
            doubts about the context in which you are handling an exception.
       [>>] You can wait upon and detach any portion of the queue hierarchy,
            using the base member function, `kdu_thread_entity::join'.
            Alternatively, you can (perhaps prematurely) terminate and
            detach any portion of the queue hierarchy, using the base
            member function, `kdu_thread_entity::terminate'.  In
            order to ensure that any background processing within a
            codestream completes (or terminates prematurely) you
            should use the `cs_terminate' function offered
            by the present object, since you do not have direct access to
            the background processing queues that might be created inside a
            `kdu_codestream' object.
       [//]
       The use of various interface functions offered by the `kdu_codestream'
       interface and its descendants can interfere with multi-threaded
       processing.  To avoid problems, you should pass a reference to
       your `kdu_thread_env' object into any such functions which can accept
       one.  Notably, you should identify your `kdu_thread_env' object
       when using any of the following:
       [>>] `kdu_codestream::open_tile'  and  `kdu_tile::close';
       [>>] `kdu_codestream::flush' and `kdu_codestream::trans_out';
       [>>] `kdu_subband::open_block' and `kdu_subband::close_block'.
       [//]
       Remember: multi-threaded processing is an advanced feature.  It is
       designed to be as used with as little difficulty as possible, but it
       is not required for Kakadu to do everything you want.  To reap
       the full advantage of multi-threaded processing, your platform
       should host multiple physical (or virtual) processors and you
       will need to know something about the number of available processors.
       The `kdu_get_num_processors' function might help you in this, but
       there is no completely general software mechanism to detect the
       number of processors in a system.  You may find that it helps to
       customize some of the architecture constants defined in "kdu_arch.h",
       notably:
       [>>] `KDU_MAX_L2_CACHE_LINE' and
       [>>] `KDU_CODE_BUFFER_ALIGN'.
  */
  public: // Member functions
    KDU_EXPORT kdu_thread_env();
    KDU_EXPORT virtual ~kdu_thread_env();
    virtual kdu_thread_entity *new_instance() { return new kdu_thread_env; }
      /* [SYNOPSIS]
           Overrides `kdu_thread_entity::new_instance' to ensure that all
           threads in a cooperating thread group are managed by objects with
           the same derived class, having the `kdu_thread_entity' base class.
      */
    kdu_block *get_block() { return &block; }
      /* [SYNOPSIS]
           This function is used to access the thread-specific
           `kdu_block' object that allows each thread to carry
           out block encoding and decoding operations in parallel.
      */
    KDU_EXPORT bool
      cs_terminate(kdu_codestream codestream, kdu_exception *exc_code=NULL);
      /* [SYNOPSIS]
           This function is very similar to `kdu_thread_entity::terminate' and
           uses it within its implementation.  The function requests early
           termination of and then joins with any background processing
           queue associated with the `codestream'; before returning true,
           the function also removes the internal multi-threaded processing
           context from the `codestream'.  The context will, however, be
           created again if you invoke the `kdu_codestream::restart' function
           with a non-NULL `env' argument -- this also applies to other
           interface functions, such as `kdu_codestream::open_tile'.
           [//]
           It is NOT SAFE to invoke this function on the same `codestream'
           simultaneously from multiple threads, since that may lead to a
           race condition regarding the destruction of the underlying thread
           context.
           [//]
           Before invoking this function, you should generally terminate or
           join with any other thread queues whose scheduled jobs might
           interact with the `codestream'; this is done by appropriate calls
           to `kdu_thread_entity::terminate' or `kdu_thread_entity::join'.
           [//]
           It is worth noting that global calls (those that provide no
           `root_queue' argument) to `kdu_thread_entity::join' and
           `kdu_thread_entity::terminate' both effectively terminate any
           background processing queues associated with codestreams, so that
           nobody is blocked.  Background codestream processing queues have
           no notion of being `all_done', so `kdu_thread_queue::all_done'
           is not normally called for these queues; however, these queues
           override `kdu_thread_queue::notify_join' to detect join attempts
           and they effectively use this information to behave as if
           `kdu_thread_queue::request_termination' had been called.
           [//]
           After a global call to `kdu_thread_entity::terminate' or
           `kdu_thread_entity::join', you should be prepared to re-instantiate
           the background processing machinery if you wish to continue using
           it -- this must be done by calling the present function, since it
           destroys the internal thread context, allowing it to be recreated
           from scratch when next needed.
         [RETURNS]
           As with `kdu_thread_entity::terminate', you may need to be careful
           of false returns.  If the function returns false, it means that
           an internal error has occurred, causing one or more threads to
           enter `kdu_thread_entity::handle_exception'.  If you called
           this function from within a scheduled job, you will need to
           pay special attention to avoiding possible race conditions if
           you choose to access resources that might be asynchronously
           cleaned up by another thread.
       */
  protected: // Functions
    virtual void donate_stack_block(kdu_byte *buffer, int num_bytes)
      { block.donate_storage(buffer,num_bytes); }
  private: // Data
    kdu_block block;
  };

#endif // KDU_COMPRESSED_H
