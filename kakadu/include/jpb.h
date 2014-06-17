/*****************************************************************************/
// File: jpb.h [scope = APPS/COMPRESSED-IO]
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
/*****************************************************************************
 Description:
    Defines JP2 box types and classes to support the elementary broadcast
 streams defined in Annex M of IS15444-1 (with amendments).
******************************************************************************/

#ifndef JPB_H
#define JPB_H
#include "jp2.h"
#include "kdu_video_io.h"

// Classes defined here
class jpb_source;
class jpb_target;

// Classes defined elsewhere
struct jb_source;
struct jb_target;

// Colour space constants
#define JPB_UNKNOWN_SPACE            ((kdu_byte) 0x0000)
#define JPB_SRGB_SPACE               ((kdu_byte) 0x0001)
#define JPB_601_SPACE                ((kdu_byte) 0x0002)
#define JPB_709_SPACE                ((kdu_byte) 0x0003)
#define JPB_LUV_SPACE                ((kdu_byte) 0x0004)
#define JPB_XYZ_SPACE                ((kdu_byte) 0x0005)

/* ========================================================================= */
/*                            External Functions                             */
/* ========================================================================= */

extern KDU_AUX_EXPORT void
  jpb_add_box_descriptions(jp2_box_textualizer &textualizer);
  /* [SYNOPSIS]
       Use this function to augment the `textualizer' with any textualization
       functions that are offered for broadcast-stream box-types.  See
       `jp2_add_box_descriptions' for more information on how this works.
  */
 

/* ========================================================================= */
/*                                 Classes                                   */
/* ========================================================================= */

/*****************************************************************************/
/*                                jpb_source                                 */
/*****************************************************************************/

class jpb_source : public kdu_compressed_video_source {
  /* [BIND: reference]
     [SYNOPSIS]
       Supports reading and random access into elementary broadcast
       streams.  An elementary broadcast stream is a sequence of
       `jpb_elementary_stream_4cc' super-boxes, as described in Annex M of
       IS15444-1 (with amendments).  Each such super-box describes a single
       frame and each frame may consist of one or two fields, each of which
       is a JPEG2000 codestream.  Sub-boxes of the `jpb_elementary_stream_4cc'
       super-box provide the metadata required to interpret each frame's
       imagery, including temporal aspects such as frame-rate and time codes,
       as well as colour space.  Since each frame has its own metadata, it
       is conceivable that these attributes change from frame to frame,
       although this seems unlikely to occur in practice.
       [//]
       This object builds on top of the abstract base type
       `kdu_compressed_video_source', which itself derives from
       `kdu_compressed_source'.  This means that the object supports
       elementary sequential frame/field opening/closing operations via
       `open_image' and `close_image', and that the object may be passed
       directly to a `kdu_codestream::create' call, in order to open,
       parse and decompress the codestream associated with a current open
       image.
       [//]
       The object also supports a degree of random access via
       `seek_to_frame', and it allows multiple codestreams to be opened
       concurrently via the `open_stream' function.  These features are
       similar to those offered by `mj2_source' and `mj2_video_source' for
       working with the more sophisticated MJ2 file format.
       [//]
       Objects of this class are merely interfaces to an internal
       implementation object.  Copying a `jpb_source' object simply
       duplicates the reference to this internal object.  For this
       reason, `jpb_source' has no meaningful destructor.  To destroy the
       internal object you must invoke the explicit `close' function.
  */
  // --------------------------------------------------------------------------
  public: // Functions specific to this object
    jpb_source() { state = NULL; }
      /* [SYNOPSIS]
           Constructs an empty interface -- one whose `exists' function
           returns false.  Use `open' to open a new broadcast stream.
      */
    jpb_source(jb_source *state) { this->state = state; }
      /* [SYNOPSIS]
           Applications have no meaningful way to invoke this constructor
           except with a NULL argument.
      */
    bool exists() const { return (state != NULL); }
      /* [SYNOPSIS]
           Returns true if there is an open file associated with the object.
      */
    bool operator!() const { return (state == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if there is an open file
           associated with the object.
      */
    KDU_AUX_EXPORT int
      open(jp2_family_src *src, bool return_if_incompatible=false);
      /* [SYNOPSIS]
           This function opens an elementary broadcast stream.  If the first
           top-level box of the data source is not an elementary broadcast
           stream box, having type code `jpb_elementary_stream_4cc', an
           error will be generated through `kdu_error', unless
           `return_if_incompatible' is true.
           [//]
           It is illegal to invoke this function on an object which has
           previously been opened, but has not yet been closed.
           [//]
           The current implementation of this object does not support dynamic
           caching data sources (those whose `jp2_family_src::uses_cache'
           function returns true).  However, all of the interface functions
           are designed to support sources for which the data in a cache might
           not yet be available.  This allows us to upgrade the internal
           implementation later without affecting the design of applications
           which use it.
         [RETURNS]
           Three possible values, as follows:
           [>>] 0 if insufficient information is available from the `src'
                object to complete the opening operation.  In practice,
                this can occur only if the `src' object is fueled by a
                dynamic cache (a `kdu_cache' object).
           [>>] 1 if successful.
           [>>] -1 if the data source does not commence with an
                elementary broadcast stream super-box.  This value will not
                be returned unless `return_if_incompatible' is true.  If it
                is returned, the object will be left in the closed state.
         [ARG: src]
           A previously opened `jp2_family_src' object.  Note that you
           must explicitly close or destroy that object, once you are done
           using it.  The present object's `close' function will not do
           this for you.
         [ARG: return_if_incompatible]
           If false, an error will be generated through `kdu_error' if the
           data source does not commence with an elementary broadcast stream
           super-box.  If true, incompatibility will not generate an
           error, but the function will return -1, leaving the application
           free to pass the `src' object to another file format reader.
      */
    KDU_AUX_EXPORT bool close();
      /* [SYNOPSIS]
           Destroys the internal object and resets the state of the
           interface so that `exists' returns false.  It is safe to close
           an object which was never opened.
      */
    KDU_AUX_EXPORT jp2_family_src *get_ultimate_src();
      /* [SYNOPSIS]
           Returns a pointer to the `src' object which was passed to `open',
           or NULL if the object is not currently open.
      */
    KDU_AUX_EXPORT kdu_byte get_frame_space() const;
      /* [SYNOPSIS]
           Returns an 8-bit identifier that represents the colour space
           associated with the currently open image.  If no image is
           currently open, the function returns the colour space for the next
           image which will be opened by `open_image', or `JPB_UNKNOWN_SPACE'
           (equals 0) if there is no next image.  Valid return values are
           as follows:
           [>>] `JPB_UNKNOWN_SPACE' -- the colour space is not known.
           [>>] `JPB_SRGB_SPACE' -- the colour space is sRGB, as defined by
                IEC 61966-2-1:1999.
           [>>] `JPB_601_SPACE' -- the YUV colour space defined by
                Rec. ITU-R BT.601-6.
           [>>] `JPB_709_SPACE' -- the YUV colour space defined by
                Rec. ITU-R BT.709-5.
           [>>] `JPB_LUV_SPACE' -- a particular opponent colour space
                that is derived by gamma-correcting the Y component from the
                underlying XYZ tri-stimulus coordinates, but in which the
                chrominance components are linearly related to XYZ.
                Specifically, analog quantities L = max{0, (77/1024)*log_2(Y),
                U = 4X / (X+15Y+3Z) and V = 9Y / (X+15Y+3Z), are converted
                to n-bit digital integers (n can be different for each
                image component) using: DL = floor(2^(n-8)*[1+L*253.999999..]);
                DU = floor(2^(n-8)*[35/64 + (406+43/64)*U]); and
                DV = floor(2^(n-8)*[35/64 + (406+43/64)*V]).  Note that the
                DU and DV parameters require a 2's complement signed
                representation, whereas DL requires an unsigned representation.
           [>>] `JPB_XYZ_SPACE' -- the XYZ colour space used for digital
                cinema, as defined by ISO26428-1.
      */
    KDU_AUX_EXPORT kdu_uint32 get_frame_timecode();
      /* [SYNOPSIS]
           Returns the 32-bit timecode associated with the frame to which
           the currently open image belongs.  If no image is currently open,
           the function returns the time code for the frame to which the
           next open image would belong if `open_image' were called.  If
           there is no currently open image and there is also no next image,
           the function returns 0xFFFFFFFF.
           [//]
           Note that the `get_frame_instant' function simply accumulates
           the durations of successive frames, without regard for their
           timecodes.  The timecode, however, may be useful in identifying
           frames that have been dropped, or where no explicit frame rate
           information is available -- i.e., when `get_frame_period' returns 0.
      */
  // --------------------------------------------------------------------------
  public: // Overrides for functions declared in `kdu_compressed_video_source'
    KDU_AUX_EXPORT virtual kdu_uint32 get_timescale();
      /* [SYNOPSIS]
           Gets the number of ticks per second, which defines the time scale
           used to describe frame periods -- see `get_frame_period'.  It
           is important to realize that this value may change from frame to
           frame.  To that end, it should be noted that the value returned
           by this function corresponds to the frame to which any currently
           open image belongs (see `open_image') or, if no image is
           currently open, to the frame associated with the next image that
           would be opened by a call to `open_image'.  If the frame whose
           timescale would be returned by this function turns out to have
           no frame-rate information, this function returns the timescale
           associated with the most recent frame for which frame-rate
           information was available -- essentially a frame which has no
           frame-rate information inherits the timescale of the previous
           frame, for the purposes of this function.  If no image is
           currently open and there is no further image that can be opened,
           the value returned by this function corresponds to the timescale
           of the last frame in the video sequence for which frame-rate
           information was available.
           [//]
           The function only returns 0 if the frame for which the timescale
           would be returned has no frame-rate information and the same is
           true for all preceding frames.
      */
    KDU_AUX_EXPORT virtual kdu_long get_frame_instant();
      /* [SYNOPSIS]
           This function returns the starting time for the frame associated
           with any open image, or the frame associated with the image that
           will be opened by the next call to `open_image' if none is
           currently open, expressed as a multiple of the number of ticks
           per second returned by `get_timescale'.  In the typical case,
           where the timescale remains constant throughout a video, the
           values returned by this function for successive frames are
           separated precisely by the relevant frame duration, as returned
           by `get_frame_period'.
           [//]
           If no image is currently open and no further images can be
           opened, this function returns the duration of the video, expressed
           with respect to the timescale returned by `get_timescale'.
           [//]
           An important difference between timecodes and the frame start
           times reported by this function is that timecodes may change
           discontinuously or even non-monotonically, if the video was
           stitched together from multiple sources, whereas the ratios
           between the frame instants returned by this function and the
           corresponding timescales returned by `get_timescale' are guaranteed
           to increase monotonically.  It can, however, happen that some
           frames are assigned the same frame instant.
           [//]
           In the event that the sequence of frames contains some frames
           for which no frame-rate information is available, the object
           treats these frames as having zero duration.
      */
    KDU_AUX_EXPORT virtual kdu_long get_frame_period();
      /* [SYNOPSIS]
           Returns the number of ticks associated with the frame to which the
           currently open image belongs.  If no image is currently open, the
           function returns the frame period associated with the frame to which
           the next open image would belong if `open_image' were called.
           [//]
           This function returns 0 if no frame-rate information is available
           for the frame in question or if there is no open image and no
           further images can be opened.
           [//]
           Note that if the video is interlaced, there are two images (fields)
           in each frame period.
      */
    KDU_AUX_EXPORT virtual kdu_field_order get_field_order();
      /* [SYNOPSIS]
           See `kdu_compressed_source::get_field_order'.  Note that the
           elementary broadcast stream specification allows the field order
           (and the number of fields) to change from frame to frame, although
           this seems highly unlikely in practice.  Also, it can happen
           that the value returned by this function is
           `KDU_FIELDS_UNKNOWN', meaning that the content is interlaced,
           but the order of the fields is not actually known.  Hopefully,
           this should not usually happen in practice.
      */
    KDU_AUX_EXPORT virtual void set_field_mode(int which);
      /* [SYNOPSIS]
           See `kdu_compressed_source::set_field_mode'.
      */
    KDU_AUX_EXPORT virtual bool seek_to_frame(int frame_idx);
      /* [SYNOPSIS]
           Attempts to set the index (starting from 0) of the frame to be
           opened by the next call to `open_image' to `frame_idx'.  This
           call could be expensive, since elementary broadcast streams
           contain no frame indexing information.  Internally, the object
           keeps track of the locations of frames that have been seen already,
           but for frames that have not yet been seen, seeking requires
           the frames to be visited sequentially, until the desired frame is
           found or the end of the data source is encountered.
           [//]
           You should avoid executing this function if a call to
           `open_image' has not been bracketed by a corresponding call to
           `close_image'.
           [//]
           If the video is interlaced and the field mode is 2
           (see `set_field_mode'), the next call to `open_image' will
           open the first field of the indicated frame, even if `frame_idx'
           was already the current frame and the first field has recently
           been opened and closed.
         [RETURNS]
           False if the indicated frame does not exist.
      */
    KDU_AUX_EXPORT virtual int open_image();
      /* [SYNOPSIS]
           See `kdu_compressed_video_source::open_image' for details.  Note,
           however, that an error message will be generated (through
           `kdu_error') if you attempt to open more than one image
           simultaneously from the same source.  If you need to open
           multiple codestreams at once, use the `open_stream' member function.
      */
    KDU_AUX_EXPORT virtual void close_image();
      /* [SYNOPSIS]
           Each successful call to `open_image' must be bracketed by a call to
           `close_image'.  Does nothing if no image (field or frame) is
           currently open.
      */
    KDU_AUX_EXPORT virtual int
      open_stream(int field_idx, jp2_input_box *input_box);
      /* [SYNOPSIS]
           This function is provided to allow you to access multiple
           codestreams simultaneously.  Rather than opening the object's
           own internal `jp2_input_box' object, the function opens the
           supplied `input_box' at the start of the relevant codestream.
           You need not close this box before invoking the `seek_to_frame'
           function to advance to another frame.  You can open as many
           codestreams as you like in this way.
           [//]
           The frame whose codestream is opened by this function is the
           one which would otherwise be used by the next call to `open_image'.
           However, neither the field nor the frame index are advanced by
           calling this function.  In order to open a different frame, you
           will generally use the `seek_to_frame' function first.  The
           particular field to be opened within the frame is identified by
           the `field_idx' argument, which may hold 0 or 1.  The
           interpretation of this argument is unaffected by any calls to
           `set_field_mode'.
           [//]
           IMPORTANT NOTE ON THREAD-SAFETY:
           In multi-threaded environments, this function allows you to
           construct applications in which separate threads concurrently
           manipulate different frames; indeed, this function itself might
           be invoked by different threads, each accessing a codestream of
           interest to decompress or render in some way.  The natural question,
           then, is whether this is safe.
           [>>] If you do intend to work with multiple codestreams
                concurrently, you should make sure that the
                `jp2_family_src' that is passed to `open'
                is itself thread-safe.  What this means in practice is that
                you should either override the `jp2_family_src::acquire_lock'
                and `jp2_family_src::release_lock' functions yourself in a
                derived class, or you should use the
                `jp2_threadsafe_family_src' object.
           [>>] Then you can be sure that this function and all operations on
                the `input_box' objects that are opened will be thread safe,
                so long as you do not use `open_image' and `close_image'.
         [RETURNS]
           The frame index associated with the opened codestream box,
           or -1 if the requested field does not exist or if the frame
           which would be accessed by the next call to `open_image' does
           not exist.
         [ARG: field_idx]
           0 for the first field in the frame; 1 for the second field in
           the frame.  Other values will result in a return value of -1.
           A value of 1 will also produce a return value of -1 if the
           video is not interlaced.
         [ARG: input_box]
           Pointer to a box which is not currently open.  Box is open upon
           return unless the function's return value is negative.
      */
  // --------------------------------------------------------------------------
  public: // Overrides for functions declared in `kdu_compressed_source'
    KDU_AUX_EXPORT virtual int read(kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           Used by `kdu_codestream' to read compressed data from an open image.
           An error is generated through `kdu_error' if no image is open
           when this function is invoked.  The function will not read past
           the data associated with the currently open image.
           See `kdu_compressed_source::read' for an explanation.
      */
    KDU_AUX_EXPORT virtual int get_capabilities();
      /* [SYNOPSIS]
           For an introduction to source capabilities, consult the comments
           appearing with the declaration of `kdu_compressed_source'.
      */
    KDU_AUX_EXPORT virtual bool seek(kdu_long offset);
      /* [SNOPSIS]
           See the description of `kdu_compressed_source::seek', but note that
           the `seek' functionality implemented here deliberately prevents
           seeking beyond the bounds of the currently open image.  An
           error message is generated (through `kdu_error') if no image is
           open when this function is invoked.  Note also that the default
           seek origin is the start of the JPEG2000 code-stream associated
           with the currently open image.
      */
    KDU_AUX_EXPORT virtual kdu_long get_pos();
      /* [SYNOPSIS]
           See `kdu_compressed_source::get_pos' for an explanation.
           As with `seek', there must be an open image and
           positions are expressed relative to the start of the
           code-stream associated with that image.
      */
  // --------------------------------------------------------------------------
  private: // Data
    jb_source *state;
  };

/*****************************************************************************/
/*                                jpb_target                                 */
/*****************************************************************************/

#define JPB_TIMEFLAG_NDF         ((int) 128)
#define JPB_TIMEFLAG_DF2         ((int) 1)
#define JPB_TIMEFLAG_DF4         ((int) 2)
#define JPB_TIMEFLAG_FRAME_PAIRS ((int) 256)

class jpb_target : public kdu_compressed_video_target {
  /* [BIND: reference]
     [SYNOPSIS]
       Supports generation of elementary broadcast streams.  An elementary
       broadcast stream is a sequence of `jpb_elementary_stream_4cc'
       super-boxes, as described in Annex M of IS15444-1.  Each such
       super-box describes a single frame and each frame may consist of one
       or two fields, each of which is a JPEG2000 codestream.  Sub-boxes of
       the `jpb_elementary_stream_4cc' super-box provide the metadata required
       to interpret each frame's imagery, including temporal aspects such as
       frame-rate and time codes, as well as colour space.  Since each frame
       has its own metadata, these attributes are permitted to change from
       frame to frame; however, this object currently does not support this
       capability.  This object copies the same metadata into each and every
       frame that is generated.  To change the frame rate, field encoding
       or colour space between consecutive frames, you will have to
       construct multiple `jpb_target' objects and concatenate the
       elementary stream super-boxes that they produce.
       [//]
       This object builds on top of the abstract base type
       `kdu_compressed_video_target', which itself derives from
       `kdu_compressed_target'.  This means that the object supports
       elementary sequential frame/field opening/closing operations via
       `open_image' and `close_image', and that the object may be passed
       directly to a `kdu_codestream::create' call, in order to generate
       and write compressed content to an open image.
       [//]
       Objects of this class are merely interfaces to an internal
       implementation object.  Copying a `jpb_target' object simply
       duplicates the reference to this internal object.  For this
       reason, `jpb_target' has no meaningful destructor.  To destroy the
       internal object you must invoke the explicit `close' function.
  */
  // --------------------------------------------------------------------------
  public: // Functions specific to this object
    jpb_target() { state = NULL; }
      /* [SYNOPSIS]
           Constructs an empty interface -- one whose `exists' function
           returns false.  Use `open' to open a new broadcast stream.
      */
    jpb_target(jb_target *state) { this->state = state; }
      /* [SYNOPSIS]
           Applications have no meaningful way to invoke this constructor
           except with a NULL argument.
      */
    bool exists() const { return (state != NULL); }
      /* [SYNOPSIS]
           Returns true if there is an open file associated with the object.
      */
    bool operator!() const { return (state == NULL); }
      /* [SYNOPSIS]
           Opposite of `exists', returning false if there is an open file
           associated with the object.
      */
    KDU_AUX_EXPORT void
      open(jp2_family_tgt *tgt, kdu_uint16 timescale,
           kdu_uint16 frame_duration, kdu_field_order field_order,
           kdu_byte frame_space, kdu_uint32 max_bitrate,
           kdu_uint32 initial_timecode=0, int timecode_flags=JPB_TIMEFLAG_DF2);
      /* [SYNOPSIS]
           Opens the object to write its output to the indicated `tgt' object.
           The remaining arguments configure the metadata that will be recorded
           with each and every frame.  The interpretation of these arguments
           is as follows.
         [ARG: timescale]
           Number of ticks per second, with respect to which the
           `frame_duration' value is to be assessed.  If `frame_duration'
           and `timescale' have a common divisor, they will be simplified
           before recording the frame metadata, as required by the standard.
           Typically, for simple integer frame rates, the `timescale' should
           be the frame rate and the `frame_duration' should be 1.  In the
           case of a more complicated frame rate like NTSC, the `timescale'
           should be 30000 and the `frame_duration' should be 1001.
           [//]
           Although the standard allows for the possibility that the frame
           rate is left unspecified, this object does not support such an
           option, because then the timecodes could not be automatically
           generated.  For this reason, neither `timescale' nor
           `frame_duration' may be 0.
         [ARG: frame_duration]
           See `timescale'.
         [ARG: field_order]
           Set this to one of the following:
           [>>] `KDU_FIELDS_NONE' -- progressive video; one field/frame.
           [>>] `KDU_FIELDS_TOP_FIRST' -- interlaced video; two
                fields/frame; first field of the frame holds the uppermost
                scan line.
           [>>] `KDU_FIELDS_TOP_SECOND' -- interlaced video; two
                fields/frame; second field of the frame holds the uppermost
                scan line.
         [ARG: frame_space]
           Must be one of the following colour spaces:
           [>>] `JPB_UNKNOWN_SPACE' -- the colour space is not known.
           [>>] `JPB_SRGB_SPACE' -- the colour space is sRGB, as defined by
                IEC 61966-2-1:1999.
           [>>] `JPB_601_SPACE' -- the YUV colour space defined by
                Rec. ITU-R BT.601-6.
           [>>] `JPB_709_SPACE' -- the YUV colour space defined by
                Rec. ITU-R BT.709-5.
           [>>] `JPB_LUV_SPACE' -- a particular opponent colour space
                that is derived by gamma-correcting the Y component from the
                underlying XYZ tri-stimulus coordinates, but in which the
                chrominance components are linearly related to XYZ.
                Specifically, analog quantities L = max{0,(77/1024)*log_2(Y),
                U = 4X / (X+15Y+3Z) and V = 9Y / (X+15Y+3Z), are converted
                to n-bit digital integers (n can be different for each
                image component) using: DL=floor(2^(n-8)*[1+L*253.999999..]);
                DU = floor(2^(n-8)*[35/64 + (406+43/64)*U]); and
                DV = floor(2^(n-8)*[35/64 + (406+43/64)*V]).  Note that the
                DU and DV parameters require a 2's complement signed
                representation, whereas DL requires an unsigned
                representation.
           [>>] `JPB_XYZ_SPACE' -- the XYZ colour space used for digital
                cinema, as defined by ISO26428-1.
         [ARG: max_bitrate]
           This value is recorded in the metadata and also checked for
           consistency.  The maximum bit-rate is expressed in bits per second,
           which is internally converted into a limit on the maximum size
           for the codestream representing each video field.  If a codestream
           turns out to exceed this limit, an error will be generated.  Note
           that the codestream size limit applies to the entire contents of
           the contiguous codestream boxes that represent each field/frame.
           [//]
           The `max_bitrate' value should be consistent with the broadcast
           profile that is targeted by the individual codestreams, although
           this is not explicitly checked.  Specifically, the `max_bitrate'
           should not exceed the following limits:
           [>>] `Sbroadcast' levels 1, 2 and 3: `max_bitrate' <= 200 * 10^6
           [>>] `Sbroadcast' level 4: `max_bitrate' <= 400 * 10^6
           [>>] `Sbroadcast' level 5: `max_bitrate' <= 800 * 10^6
           [>>] `Sbroadcast' level 6: `max_bitrate' <= 1600 * 10^6
           [>>] `Sbroadcast' level 7: no restriction.
         [ARG: initial_timecode]
           Timecodes for consecutive frames are generated automatically,
           starting from the supplied `initial_timecode'.  The 4-byte
           timecode represents 4 BCD (Binary Coded Decimal) quantities,
           corresponding to HH:MM:SS:FF.  Here, HH is the number of hours and
           is stored in the most significant byte of the time code, MM is
           minutes, SS is seconds, and FF is a frame count within the second.
           [//]
           The generation of timecodes is dependent upon certain flags that
           may be passed via the `timecode_flags' argument.
           [//]
           It is also possible to explicitly override the automatic timecode
           generation logic by invoking the `set_next_timecode' function
           prior to the final `close_image' call of a frame.
         [ARG: timecode_flags]
           This argument modifies the way in which timecodes are generated.
           Possible flags are as follows:
           [>>] `JPB_TIMEFLAG_NDF' -- if present, a non-dropframe timecode
                generation convention is followed, as explained below.
           [>>] `JPB_TIMEFLAG_DF2' -- if present, and `JPB_TIMEFLAG_NDF' is
                absent, a dropframe convention is used for timecode generation
                (where needed) dropping frame counts FF=0 and FF=1 from
                certain seconds, as explained below.  This flag is included
                by default and is appropriate for most standard video
                frame-rates.
           [>>] `JPB_TIMEFLAG_DF4' -- if present, and `JPB_TIMEFLAG_NDF' is
                not present, a dropframe convention is used for timecode
                generation (where needed) dropping frame counts FF=0, FF=1,
                FF=2 and FF=3 from certain seconds, as explained below.  This
                flag is included by default.
           [>>] `JPB_TIMEFLAG_FRAME_PAIRS' -- if this flag is present,
                each pair of consecutive frames is assigned the same
                timecode and the FF (frame count) component of the timecode
                increments only every two frames.  This option may be
                appropriate for 50Hz and 60Hz progressive video, at least
                according to SMPTE guidelines.
           [//]
           We now briefly explain the dropframe system.  Let D=0 if
           `JPB_TIMEFLAG_NDF' is present, else D=4 if `JPB_TIMEFLAG_DF4'
           is present, else D=2 if `JPB_TIMEFLAG_DF2' is present, else D=1.
           Furthermore, let P=2 if `JPB_TIMEFLAG_FRAME_PAIRS' is present,
           else P=1.  We first explain non-dropframe timecode generation,
           corresponding to the case in which D=0.  In this case,
           the timecode's SS (seconds) field is incremented every K
           frames, where K is the smallest multiple of P that is no smaller
           than `timescale'/`frame_duration' (i.e., the frame-rate).
           If K = P (very low frame-rate), the timecode's SS field may
           be incremented by more than 1 at a time in order to keep the
           HH:MM:SS part of the timecode on track; apart from this case,
           however, if K is not exactly equal to the frame-rate, the
           timecode clock will run more slowly than the true frame clock.
           [//]
           When D > 0, a dropframe policy is employed for cases in which
           K is not exactly equal to the frame-rate -- i.e., where the
           frame-rate is not an exact integer multiple of P.  The drop-frame
           policy involves skipping of the initial D frame count values
           (FF part of the timecode) from certain seconds so as to keep
           the timecode synchronized (at least approximately) with the
           true frame clock.  The internal machinery implements an algorithm
           to automatically determine good boundaries at which to introduce
           the dropped frame counts.  This algorithm drops frame counts at
           consistent intervals of duration T, wherever needed.  For NTSC,
           the interval is T = 1 minute and the internal algorithm replicates
           the conventional NTSC drop frame timecoding mechanism.  More
           generally, the interval T is chosen to be the largest multiple of
           1 second, 10 seconds, 1 minute or 10 minutes, such that drop frames
           in the time code are required at most once per interval.
           The dropcode algorithm's state is reset whenever the HH (hour
           counter) part of the timecode increments, for consistency with
           the standard NTSC algorithm.
      */
    KDU_AUX_EXPORT void set_next_timecode(kdu_uint32 timecode);
      /* [SYNOPSIS]
           You may use this function to override the automatic timecode
           generation algorithm.  The value supplied here becomes the
           timecode that is next written when a frame's last field is
           closed by a call to `close_image'.
      */
    KDU_AUX_EXPORT kdu_uint32 get_next_timecode() const;
      /* [SYNOPSIS]
           Returns the time code for the next frame that would be written;
           this is useful if you need to find the `initial_timecode' value
           to pass to `open' when opening a new instance of this object
           (e.g., to represent frames with a different colour space,
           maximum bit-rate, etc.).
      */
    KDU_AUX_EXPORT kdu_uint32 get_last_timecode() const;
      /* [SYNOPSIS]
           Returns the time code of the last complete frame that has been
           written.  If nothing has been written, returns the same value as
           `get_next_timecode'.
      */
    KDU_AUX_EXPORT bool close();
      /* [SYNOPSIS]
           Destroys the internal object and resets the state of the
           interface so that `exists' returns false.  It is safe to close
           an object which was never opened.
      */
  // --------------------------------------------------------------------------
  public: // Overrides for functions declared in `kdu_compressed_video_target'
    KDU_AUX_EXPORT virtual void open_image();
      /* [SYNOPSIS]
           Call this function to initiate the generation of a new video frame
           or field.  In non-interlaced (progressive) mode, each frame consists
           of a single image.  However, for interlaced video, there are two
           images (fields) per frame.
           [//]
           After calling this function, the present object may be passed into
           `kdu_codestream::create' to generate the JPEG2000 code-stream
           representing the open video image.  Once the code-stream has been
           fully generated (usually performed by `kdu_codestream::flush'),
           the image must be closed using `close_image'.  A new video image
           can then be opened.
      */
    KDU_AUX_EXPORT virtual void close_image(kdu_codestream codestream);
      /* [SYNOPSIS]
           Each call to `open_image' must be bracketed by a call to
           `close_image'.  The caller should supply a non-empty `codestream'
           interface, which was used to generate the compressed data for
           the image (field or frame) just being closed.  Its member functions
           may be used for checking profile consistency.  If `codestream'
           is an empty interface, no consistency checking will be applied, but
           everything will still work.
      */
  // --------------------------------------------------------------------------
  public: // Overrides for functions declared in `kdu_compressed_target'
    KDU_AUX_EXPORT virtual bool write(const kdu_byte *buf, int num_bytes);
      /* [SYNOPSIS]
           Used by `kdu_codestream' to write compressed data to an open field.
           An error is generated through `kdu_error' if no field is open
           when this function is invoked.  See `kdu_compressed_target::write'
           for more information.
      */
    KDU_AUX_EXPORT virtual bool start_rewrite(kdu_long backtrack);
      /* [SYNOPSIS]
           Implements the backtracking feature of the abstract
           `kdu_compressed_target' base class.  This is required to ensure
           that TLM marker segments can be correctly written and these are
           a required feature of codestreams with the broadcast profile.
      */
    KDU_AUX_EXPORT virtual bool end_rewrite();
      /* [SYNOPSIS]
           Overwrites the `kdu_compressed_target::end_rewrite' function to
           complete the implementation of backtracking -- see `start_rewrite'.
      */
  // --------------------------------------------------------------------------
  private: // Data
    jb_target *state;
  };

#endif // JPB_H
