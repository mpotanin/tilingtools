/*****************************************************************************/
// File: kdu_region_animator.h [scope = APPS/SUPPORT]
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
   Defines the `kdu_region_animator' object that manages animation timing and
scheduling for rendering/browsing applications.  This object is designed as
a companion to `kdu_region_compositor'.
******************************************************************************/
#ifndef KDU_REGION_ANIMATOR_H
#define KDU_REGION_ANIMATOR_H

#include "kdu_region_compositor.h"
#include "jpx.h"
#include "mj2.h"
#include "kdu_client.h"

// Defined here:
class kdu_region_animator_roi;
class kdu_region_animator;

// Defined elsewhere
struct kdra_frame;
struct kdra_frame_req;
struct kdra_client_req;

#define KDU_ANIMATION_DONE    ((int) -1)
#define KDU_ANIMATION_PENDING ((int) -2)
#define KDU_ANIMATION_PAUSE   ((int) -3)


/*****************************************************************************/
/*                        kdu_region_animator_roi                            */
/*****************************************************************************/

class kdu_region_animator_roi {
  /* [BIND: reference]
     [SYNOPSIS]
       This object can be used to describe a single region of interest
       (always in the coordinate system used by an appropriately initialized
       instance of `kdu_region_compositor') or a dynamic transition between
       two regions of interest.  The object is used to communicate static and
       dynamic regions of interest synthesized by the `kdu_region_animator'
       object.
       [//]
       Transitions start from an "initial_roi" and end at a "final_roi",
       but only a part of this total range may actually be in use, as
       determined by the `set_end_points' function.  The current region
       of interest is given by a position parameter s.  The position must lie
       in the range 0 to 1 but the range may be further restricted. The
       region of interest associated with parameter s is returned by the
       function `get_roi_for_pos'.
       [//]
       The position parameters s is a function of time, that we will denote
       s(t).  Transition dynamics are symmetric with respect to s=1/2.  That
       is, s(t) = 1 - s(1-t), where t ranges between 0 and T, corresponding
       start and end of the complete transition from s=0 to s=1.  The
       complete transition time T is returned by the `init' function.
       [//]   
       During the first half of the transition, ds/dt increases linearly
       (i.e., constant acceleration A=`acceleration') until it reaches
       V=`max_ds_dt', after which it remains constant.  Here, `max_ds_dt' and
       `acceleration' are the parameters passed to the `init' function.
       Let Ta be the time at which the acceleration stops, so that
       deceleration starts at time Tb = T - Ta. Then we must have:
       [>>] Ta = V / A
       [>>] Tb = T - V / A
       [>>] S(Ta) = 0.5*A*Ta^2 = 0.5 * V^2 / A
       [>>] S(Tb) = 1 - S(Ta)
       [//]
       The object provides functions `get_pos_for_time' and `get_time_for_pos'
       that can be used to find the evaluate s(t) and the inverse function
       t(s), respectively, all of which measure time relative to the start of
       the complete transition (i.e., from s=0).
  */
public: // Member functions
  kdu_region_animator_roi() { init(); }
  bool operator!() const { return !is_valid; }
    /* Returns true if and only if `valid' returns false. */
  bool valid() { return is_valid; }
    /* [SYNOPSIS]
       After construction, or a call to `init', you need to call
       `set_end_points' before this function will return true.
    */
  double init(double acceleration=-1.0, double max_ds_dt=-1.0);
    /* [SYNOPSIS]
       Initializes the object's dynamics.  Once this function has been called,
       you may use `get_time_for_pos' and `get_pos_for_time' successfully.
       Other aspects of the object are configured using the
       `set_end_points' function.  If the ROI is to be static (no transition),
       you should pass in a value <= 0 for one or both of the arguments -- this
       ensures that `get_time_for_pos' and `get_pos_for_time' always return
       0.0.  Note that the `valid' function returns true only after
       `set_end_points' have been called.  This function returns the total
       transition time between a starting position of s=0.0 and the final
       position of s=1.0, based on the supplied acceleration and maximum
       speed parameters.
    */
  double set_end_points(double start_pos, double end_pos,
                        kdu_dims initial_roi, kdu_dims final_roi);
    /* [SYNOPSIS]
       Call this function only after `init'.  This function provides the
       `initial_roi' and `final_roi' parameters, that correspond to the s=0
       and s=1 case where s is the position parameter.  It also provides
       the range of valid values for `s' through `start_pos' and `end_pos'
       (these must satisfy 0 <= `start_pos' <= `end_pos' <= 1.0).
          The function returns the time taken to transition from `start_pos'
       to `end_pos'.
          If the ROI is to be static (no transition),
       the function ignores `start_pos', `end_pos' and `initial_roi',
       effectively setting `initial_roi' to `final_roi' -- static ROI's
       correspond to the case in which `init' was passed non-positive
       arguments.
    */
  double get_start_pos() { return start_pos; }
  double get_end_pos() { return end_pos; }
  double get_time_for_pos(double s);
    /* [SYNOPSIS]
       Evaluates the time t, at which position s is attained within the
       dynamic ROI transition.  If the ROI is static (i.e., `init' was passed
       non-positive arguments), the function always returns 0.0.  If
       s=0.0, the function also returns 0.0, since time is measured relative
       to the start of the transition, when the region of interest is
       (`initial_roi' -- see `set_end_points').  The value of s is
       automatically truncated to the interval 0.0 to 1.0.  For more on s
       and transition dynamics, see the notes that appear below.  Note that
       this function does not confine `s' to lie within the `start_pos'
       and `end_pos' variables passed to `set_end_points'.
    */
  double get_pos_for_time(double t);
    /* [SYNOPSIS]
       Inverse of the above function, this one measures the position s (in the
       range 0.0 to 1.0) that corresponds to time t, measured relative to the
       start of the transition, when the region of interest is (`initial_roi').
       If the ROI is static (i.e., `init' was passed non-positive arguments),
       or if `init' has not yet been called, this function always returns 0.0.
    */
  bool get_roi_for_pos(double s, kdu_dims &roi);
    /* [SYNOPSIS]
       This function does nothing, returning false, unless the object is
       valid (i.e., `valid' must return true).  If valid, the function returns
       true, setting the `roi' argument
       to the region of interest that corresponds to position `s',
       where s is automatically truncated to the range 0.0 to 1.0 -- s=0.0
       corresponds to `initial_roi' and s=1.0 corresponds to `final_roi'.
    */
  //---------------------------------------------------------------------------
  private: // Data members that describe the dynamics
    bool static_roi; // If the object describes only a single ROI
    double acceleration; // Used only for dynamic ROI's
    double max_ds_dt; // Used only for dynamic ROI's
  private: // Data members required to determine regions of interest
    kdu_dims final_roi; // If `single_roi', this is the only region used
    kdu_dims initial_roi; // Used only for dynamic ROI's
    bool is_valid; // If false, this structure has no valid information
    double start_pos;
    double end_pos;
  private: // Derived data members
    double trans_pos;
    double time_to_trans_pos; // sqrt(2*trans_pos/acceleration)
    double total_transition_time; // 2*time_to_trans_pos + 
                                  // (1-2*trans_pos)/max_ds_dt
  };

/*****************************************************************************/
/*                           kdu_region_animator                             */
/*****************************************************************************/

class kdu_region_animator {
  /* [BIND: reference]
     [SYNOPSIS]
       This object is designed to work together with `kdu_region_compositor'
       in applications that need advanced animation services.  The object
       manages scheduling of animation frames without actually getting involved
       with rendering aspects.  The object also calculates geometrical
       attributes of the frames (and regions of interest on those frames) that
       are to be scheduled and can use this information to synthesize
       forward looking JPIP requests for the relevant data from a JPIP server.
       [//]
       In addition to regular forward, reverse and repeated playback of
       frames from an MJ2 video track or JPX animation, the object is also
       capable of synthesizing animations based on metadata.  For example,
       a sequence of metadata nodes that identify regions of interest on
       particular compositing layers in a JPX source can be converted into
       an animation between the relevant compositing layers, which focuses
       on the regions of interest.  These metadata-driven animations are
       quite sophisticated, allowing both regions within a layer and also
       whole layers within larger composited frames to become the focus
       of the animation.  The object can synthesize dynamic transitions
       between regions of interest within metadata-driven animations,
       based on acceleration and speed parameters, working together with
       `kdu_region_compositor' to determine (in just-in-time fashion) the
       optimal dynamics for such transitions.
       [//]
       As noted above, an important feature of this object is the ability to
       work with content that is available on a remote server, accessed via
       the JPIP protocol.  The object is able to synthesize the JPIP requests
       required to obtain incomplete metadata for frames that it will need
       in the future, including the metadata required to synthesize a
       metadata-driven animation in the first place.  The object is also
       able to schedule JPIP requests for the image data that it will need
       in the future.
       [//]
       Importantly, this object does not keep any clock of its own.  It relies
       entirely upon the application to provide timing information when various
       interface functions are called.  All timing information is measured
       in seconds -- actually, it does not matter what the unit of measurement
       is, except that the object must be able to interpret display times
       relative to the timing information provided by individual file formats,
       in conjunction with the adjustments supplied via the `set_timing'
       function.
       [//]
       In practical animation applications, it is usually important to
       synchronize the timing information provided by a data source
       (e.g., JPX/MJ2 file) with the actual vertical blanking periods of the
       monitor or other display device.  To be more specific, if a data source
       is supposed to have its frames displayed at precisely 60 frames
       per second and a monitor has a frame rate of 60 frames per second, this
       means that we would like one source frame to be presented to the
       monitor each time it undergoes a vertical retrace (or vsync).  What
       we do not want to do is render frames at 60 frames/second with
       respect to the platform's real-time clock and then find that the
       monitor's refresh rate is actually 59.9 frames/second, since this
       will result in undesirable visual beat frequencies, or even rolling
       video tears, depending on the blitting technology used.  Even though
       these are application issues, it turns out to be very important to
       allow all display timing information to be driven by monitor refresh
       events.  That is, the monitor refresh events should be interpreted
       as regularly spaced ticks of a display clock, regardless of the
       actual system times at which these events might be detected by
       software.  With this in mind, the present object is designed in
       such a way that the display times that need to be passed into
       functions by the application only ever need to correspond to
       discrete display events.
       [//]
       There are, however, two function calls provided by the object which
       require actual system clock times to be supplied.  These are the
       `advance_animation' and the `note_frame_generated' function.  The
       origin for these system clock times is immaterial, since we only
       use them to measure the interval between the point at which the
       animation frame was able to advance and the point at which rendering
       of that frame was completed.
       [//]
       We take care to distinguish system time from display time, because
       we wish to emphasize that the application does not need to implement
       any kind of PLL algorithm to synthesize a high precision real-time
       clock that is synchronized with the monitor's internal clock.
       Whenever we need to measure continuous time (rarely), this is supplied
       as system clock measurements that need not be synchronized with the
       display clock.  Meanwhile, display clock times need never be supplied
       on a continuous basis.
       [//]
       To be more concrete, if the monitor refresh period is T, the display
       clock should take values of T0 + n*T where n is the number of VSYNC
       events that have gone by since the arbitrary time origin T0.  The
       application is never required to supply display times of any form
       other than T0 + n*T for some integer n, maintaining a consistent notion
       of the arbitrary system clock origin T0.  It is important that the
       average separation between monitor VSYNC events is at least close to
       T seconds, as measured in system time, but it does not need to be
       exactly T seconds.
  */
  public: // Configuration member functions
    KDU_AUX_EXPORT kdu_region_animator();
    ~kdu_region_animator() { stop(); }
    bool is_active() { return (jpx_src != NULL) || (mj2_src != NULL); }
      /* [SYNOPSIS]
         Returns true between calls to `start' and `stop'.
      */
    KDU_AUX_EXPORT bool start(int video_range_start, int video_range_end,
                              int initial_video_frame, jpx_source *jpx_in,
                              mj2_source *mj2_in, kdu_uint32 video_track,
                              bool single_layer_mode);
      /* [SYNOPSIS]
         Call this function to start an animation.  For MJ2 sources, `mj2_in'
         is non-NULL, while for JPX sources, `jpx_in' is non-NULL.  In both
         cases, the `video_track' argument supplies the track from which
         video frames are to be played -- the function returns false if this
         track is not valid.  For JPX sources, track 0 is always valid;
         others correspond to global presentation threads.  For MJ2 sources,
         valid track identifiers must be non-zero and must refer to
         video (as opposed to audio) tracks.
         [//]
         For JPX sources, the `single_layer_mode' option causes all frame
         indices to be interpreted as compositing layer indices instead,
         wherever they appear in function arguments (including this function)
         or return values.  In this mode, `video_track' is ignored, but should
         be 0, and `jpx_in' should be non-NULL.
         [//]
         If the function returns false, there is either no supplied data
         source, the `video_track' is invalid, or the identified track has
         no "frames" at all, in the sense defined above.
         [//]
         There are two types of animations: 1) video playback; and
         2) metadata-driven animations.  The first type of animation involves
         a range of frame indices (or compositing layer indices).  The range
         starts from `video_range_start' and finishes at `video_range_end'.
         The value of `video_range_end' must be at least as large as
         `video_range_start' and `video_range_start' may not be -ve.  The
         `set_max_frames' function can be called at a later point
         to inform the object of the maximum number of frames (or compositing
         layers in single layer mode) that the source supports -- or the
         maximum number that are to be considered as existing.  The first
         frame (or compositing layer) in the above-indicated range to be
         visited is given by `initial_video_frame'.  This value must lie
         within the range.  Starting from that frame (or compositing layer),
         the video can be played either forwards or backwards and it can wrap
         around, depending on the settings passed to `set_reverse' and
         `set_repeat'.
         [//]
         The second type of animation is driven by metadata and can be used
         only with JPX sources.  In this case, `start' is called with -ve
         values for `video_range_start', `video_range_end' and
         `initial_video_frame' and `video_track' are ignored, but should be 0.
         The `add_metanode' function may be invoked as often as desired
         before each call to `advance_animation'.  Typically, all
         such calls appear between `start' and the first call to
         `advance_animation', but if new metadata is discovered after the
         animation is part way through, it can easily be added by calling
         `add_metanode' at that point.  As with the video playback mode, the
         metadata-derived frames may be played in reverse order by calling
         `set_reverse'.  However, in this case, metadata supplied to the
         object by calls to `add_metanode' that follow the first call to
         `advance_animation' might not be reached by the animation unless the
         object is in the repeat mode -- see `set_repeat'.
      */
    KDU_AUX_EXPORT int add_metanode(jpx_metanode node);
      /* [SYNOPSIS]
         This function is used with metadata-driven animations.  If
         `start' has not been called since the last call (if any) to `stop',
         or if the last call to `start' had a non-negative `video_range_start'
         argument, this function does nothing, returning 0.
         [//]
         Animation frames are created from all appropriate number list and
         ROI nodes that are encountered amongst the descendants of the
         supplied `node'.  In addition to number lists and ROI nodes, if we
         encounter a link (cross-reference box) at `node' or any of its
         descendants, the link target generates an animation frame if it is
         descended from a suitable number list or ROI node.  Descendants are
         examined recursively, but we do not look below a node that generates
         an animation frame.
         [//]
         The function returns the total number of frames that are added
         during the recursive descent.  If `node'
         or any of the descendants (or link targets) encountered during
         recursive descent is not yet available (can happen if the source of
         data is ultimately a dynamic cache), the function leaves behind an
         "incomplete" frame, from which the expansion can continue once more
         data becomes available.  These incomplete frames are also counted in
         the function's return value, so the function returns non-zero if true
         animation frames are created or if true animation frames might be
         created in the future, once more data becomes available.
         [//]
         The JPX frame/s (or compositing layer/s in single layer mode) to be
         displayed with a given metadata node are determined by the number
         list with which it is associated.  The most reliable method for
         discovering frames from number lists is via the compositing layers
         that they reference; this is because JPX animation frames are
         defined in terms of compositing layers.  If a number list references
         any compositing layers these are used exclusively to determine the
         JPX frames.  If a number list references codestreams but no
         compositing layers, a metadata-driven frame can still be added
         under the following circumstances: 1) In composited frame mode (i.e., 
         not single layer mode), the `jpx_composition::find_numlist_match' is
         used to find matches based on codestream information alone, to the
         extent that this is possible; 2) In single layer mode, a
         metadata-driven frame is added only if the referenced codestreams
         are used by compositing layer 0 -- this is useful, because images
         with only one compositing layer will not generally include
         references to that compositing layer in their number lists, yet
         they will also not generally have any composition box, so that
         only the single layer mode can be used.  If none of these
         conditions applies, nothing is added.
         [//]
         Frames do not need to be played in order; rather, the presentation
         order is driven by the order in which their metadata nodes are
         added using this function.  However, where a single metadata
         node matches multiple frames, those frames will be played in
         their original order and even with their original timing.
      */
    KDU_AUX_EXPORT void stop();
      /* [SYNOPSIS]
         Stops the animation.  This function should be explicitly called
         even if the last frame of the animation, as determined by a
         negative return value from `advance_animation'.
      */
    bool get_metadata_driven() { return metadata_driven; }
      /* [SYNOPSIS]
           Returns true if the object is currently managing a
           metadata-driven animation (see `start' for more on this).
      */
    bool get_single_layer_mode() { return single_layer_mode; }
      /* [SYNOPSIS]
           Returns true if the object is currently managing an animation that
           is constructed from individual JPX compositing layers, rather than
           composited JPX animation frames.  See `start' for more on this.
      */
    kdu_uint32 get_video_track() { return track_idx; }
      /* [SYNOPSIS]
           Always returns 0 if `get_single_layer_mode' returns true.  Note
           that the track index is not relevant for metadata-driven
           animations.
      */
    KDU_AUX_EXPORT void set_repeat(bool repeat);
      /* [SYNOPSIS]
           Use this function to put the function into the "repeating" or
           "non-repeating" mode.  This mode can be changed at any time,
           including between calls to `start' and `stop'.
      */
    bool get_repeat() { return repeat_mode; }
      /* [SYNOPSIS] Returns the current repeating mode state. */
    KDU_AUX_EXPORT void
      set_reverse(bool reverse, double next_display_event_time=-1.0);
      /* [SYNOPSIS]
         Use this function to change the direction in which animation frames
         are returned by the `advance_animation' function.  The playback
         direction can be changed at any time, including between calls to
         `start' and `stop'.
         [//]
         If possible, you should use `next_display_event_time' to pass the
         value of the application's display clock that is associated with
         the next display update event (typically the next display refresh
         time).  This value is interpreted in the same way as all other
         display times passed in and out of this object.  Display time is
         generally related to real system time, but need not advance at
         exactly the same rate as the system clock.  For example, if a display
         has a nominal refresh rate of 60 frames/second, it is desirable to
         ensure that display update events have display times that are
         precise multiples of 1/60 seconds, even if the actual time between
         these display refresh events turns out to be slight different, as
         measured on the system clock.  The introductory notes appearing
         with this object explain why it is that an application may need to
         keep two different clocks, one for true system time, and another
         for notional display time.  The intent is that they run at the
         same rate, but they can run at slightly different rates and they
         can have very different origins.
         [//]
         The `next_display_event_time' is not important if you have not yet
         invoked `advance_animation' since the last call to `start', but if
         you have, passing a non-negative value for `next_display_event_time'
         allows the object to adjust its internal notion of the frame that
         should next be displayed, as well as relevant state parameters
         for that frame, so that reversals in the playback direction can be
         as seamlessly as possible.
         [//]
         If the `next_display_event_time' argument is -ve, the function will
         not be able to make display-synchronized adjustments and so the
         animation may change discontinuously, especially if there are
         metadata-driven frames with dynamic regions of interest.
      */
    bool get_reverse() { return reverse_mode; }
      /* [SYNOPSIS] Returns the current playback direcion. */
    KDU_AUX_EXPORT void set_timing(double custom_frame_rate,
                                   double native_frame_rate_multiplier,
                                   double intra_frame_rate);
      /* [SYNOPSIS]
           To use the native source timing, set `custom_frame_rate' to a
           -ve value.  In single layer mode, there is no real native timing,
           but an assumed native frame rate of 1 compositing layer/second is
           used.
           [//]
           The `intra_frame_rate' applies only if successive animation
           frames use the same JPX frame index (or compositing layer index in
           single layer mode) -- this is possible only when the animation is
           metadata-driven; in that case, each of the animation frames is
           assigned a duration of 1/`intra_frame_rate', unless
           `intra_frame_rate' is <= 0, in which case the metadata-driven
           animation frames use the native or custom frame rate, as
           appropriate.
           [//]
           The `max_pan_rate' argument is expressed in pixels per second.  It
           establishes an upper bound on the rate at which the viewport is
           effectively panned around during a metadata-driven animation.
           If <= 0, there is no limit.  Otherwise, the object uses the
           information passed in calls to `adjust_viewport_for_roi' to
           determine the panning rate, introducing delays and even extra
           pseudo-animation frames as required, to ensure smooth panning.
           Pseudo-animation frames are inserted between two animation frames
           that involve the same actual frame, so that their regions of
           interest can be centred on the viewport without panning the
           composition at a rate that exceeds the limit supplied here.
           [//]
           Note that this function can be called at any point,
           including between calls to `start' and `stop' -- the object
           automatically adjusts its internal timing variables so that
           animation continues, uninterrupted, at the new frame rate.
      */
    KDU_AUX_EXPORT void set_max_frames(int max_frames);
      /* [SYNOPSIS]
           You may call this function if you know the maximum number of
           frames the source supports -- if you like you can specify an even
           smaller value here, but multiple calls to this function cannot
           increase the maximum number of accessible frames, at least until
           a new animation is started by calling `start'.  Remember that in
           the JPX single layer mode, `max_frames' is interpreted as a maximum
           number of compositing layers.  The function is invoked internally
           once the object discovers a bound on the number of frames (or
           compositing layers) supported by the source, but it may not be
           possible to tell whether the end of a source has been reached
           without trying to open the frame -- something that is not done by
           this object.
           [//]
           If the frame index (or compositing layer index) returned by a
           previous call to `advance_animation' turns out to be greater than
           or equal to `max_frames', the `advance_animation' function should
           be called again to get a valid active animation frame.
           [//]
           For metadata-driven animations, this function returns immediately
           with no effect.
      */
    KDU_AUX_EXPORT void retard_animation(double display_time_delay);
      /* [SYNOPSIS]
           You may call this function to add `display_time_delay' seconds of
           delay to the point at which the object believes animation frames
           should be displayed.  This same delay is added to the values
           reported by `get_display_time'.  It is a good idea to arrange
           for `display_time_delay' to be a multiple of the monitor's frame
           interval, assuming that the application's display clock is
           ultimately driven by monitor vertical blanking events.
           [//]
           NOTE: If the current frame forms part of an auto-pan preamble
           (i.e., if the `kdu_region_animator_roi' object recovered via
           `get_roi_and_mod_viewport' reports starting or ending positions
           other than 0.0 and 1.0), the `display_time_delay'
           is not applied immediately.  Instead, the delay is accumulated
           internally until the end of the auto-pan preamble, so as to
           avoid introducing visually annoying jumps in the auto-pan
           dynamics.
      */
    KDU_AUX_EXPORT bool
      insert_conditional_frame_step(double next_display_event_time,
                                    double min_gap);
      /* [SYNOPSIS]
           This function is used to introduce a new conditional animation step
           for the current animation frame.  Conditional animation steps
           appear as new animation frames that have exactly the same frame
           index as the current one, with exactly the same associated
           metadata and other attributes, but a different display time.
           [//]
           Conditional animation steps might be skipped over if they would
           interfere with regular animation frame changes.  In particular,
           if the next regular animation frame has a display time that is
           within `min_gap' seconds of the proposed conditional frame
           step's `display_time', the conditional frame step is skipped.  It
           can happen that the true display time for the next regular animation
           frame is not known at the time when this function is called, or
           that the next animation frame is not ready for display.  It may also
           happen that the display time for the next animation frame changes
           due to adjustments in the time scale -- see `set_timing'.  For
           all of these reasons, the present function makes no judgement about
           whether or not the conditional frame step will be skipped; it
           simply installs the relevant parameters and leaves the
           `advance_animation' function to decide what to do, when the time
           comes.
           [//]
           At most one conditional frame step may exist at any given point.
           If the supplied `next_display_event_time' is earlier than the
           display time associated with an existing conditional frame step,
           the existing one is replaced by the new one; otherwise, the
           existing frame step is left unchanged.  You can think of
           conditional frame steps as refresh events, when the application
           wants to be given an opportunity to render the current animation
           frame differently in some way that is not driven by the animator
           itself.  Note, though, that all timing for conditional frame steps
           is based on the intended display time, as for all animation frames.
           The system time at which the application chooses to render a
           frame and push it onto a presentation queue is up to the
           application.
           [//]
           The function returns true if a new conditional frame step was
           inserted.  This is usually the case, but may be false for one of
           the following reasons:
           [>>] The `advance_animation' frame has not been successfully called
                since the last call to `start', so there is no current frame.
           [>>] The current animation frame has a current display time that is
                equal to or greater than `next_display_event_time'.
           [>>] The internal machinery is implementing a dynamic pan between
                regions of interest, in which case animation frames will appear
                to be synthesized on demand, in order to realize the required
                panning rate.
           [>>] There are no more frames in the animation.
           [>>] A conditional frame step already exists with an earlier
                display time.
           [//]
           If the function does return true, an internal state variable is
           set so that the next call to `next_frame_has_changed'
           returns true.  That function provides a convenient mechanism
           for the application to determine whether a call to
           `advance_animation' might be warranted.
      */
  //---------------------------------------------------------------------------
  public: // Functions used to issue and manage `kdu_client' requests
    void
      configure_request_parameters(int max_outstanding_requests,
                                   double min_aggregation_time)
      { this->max_outstanding_client_requests = max_outstanding_requests;
        this->client_request_min_t_agg = min_aggregation_time; }
      /* [SYNOPSIS]
           The parameters supplied here control behaviour of the algorithm
           implemented by `generate_imagery_requests'.  Briefly, that
           algorithm works as follows:
           [>>] At most `max_outstanding_requests' may be outstanding within
                the `kdu_client' machinery for the relevant client request
                queue.  If the server is not responding, or aggregated
                requests are infeasible, or not sufficiently supported by
                the server, responses may get further and further behind
                the rendering process.  The `max_outstanding_requests'
                argument caps the degree to which this can occur.  This
                might not hold up the animation itself, assuming sufficient
                data is in the client cache to at least access the
                relevant codestream headers.  However, it may force an
                interactive user to backtrack over the video, or to reduce
                the playback rate.  The internal client request logic
                implemented by `generate_imagery_requests' has its own
                mechanism for determining how far ahead of time it should
                issue requests for animation frames, which may lead to
                a much smaller number of outstanding requests than the
                maximum value provided here.
           [>>] If possible, multiple frames may be aggregated into a single
                window of interest request passed to `kdu_client::post_window'.
                The maximum period of time (measured in display time) that
                may be represented by aggregated frames is denoted T_agg.
                For reasons that are explained below, the value of T_agg may
                need to be adjusted to accommodate properties of the
                client-server connection, but `min_aggregation_time' provides
                a lower bound on the value that should be chosen for T_agg.
           [//]
           Aggregation of multiple frames into a single window of interest
           request is not as important if the client-server interaction is
           session based (i.e., stateful), since in this case the client
           is prepared to keep many requests in flight at once.  Even in this
           case, though, request aggregation tends to limit the amount of
           request data that must be sent upstream to the server and may
           allow the server to respond more efficiently.  Typically, an
           aggregation period of perhaps 1/8 or 1/4 of a second is more than
           sufficient for stateful communication.  With this in mind, a
           reasonable choice for the `min_aggregation_time' argument is 0.125.
           Smaller values might be chosen in an attempt to achieve a lower
           latency between interactive commands and the point at which the
           content is rendered, while larger values might be selected to
           encourage increased efficiency.
           [//]
           If the client-server interaction is stateless, involving no
           server assigned session that can preserve state between requests,
           request aggregation is very important.  This is because the client
           will not issue a new request to the server until a prior request's
           response (usually truncated by internal client generated length
           limits) has arrived in full.  The reason for this is that issuing
           new requests before old ones have arrived in full may result in
           the server sending some data multiple times.  In this case, it
           is still fine to supply a small value for `min_aggregation_time',
           such as 1/8, but the actual value of T_agg may be very much
           larger.  The `generate_imagery_requests' function adjusts T_agg
           based on calls to `kdu_client::get_timing_info'.
           [//]
           It is worth pointing out that window of interest aggregation
           might not be possible for metadata generated animations.  In
           particular, when the metadata identifies distinct regions of
           interest within the content, rather than just distinct frames or
           compositing layers, each metadata-generated frame may require its
           own distinct window-of-interest request to be posted to the client.
           [//]
           Frames that have duration greater than T_agg may result in multiple
           calls to `kdu_client::post_window'.
         [ARG: max_outstanding_requests]
           Maximum number of timed requests that the internal machinery
           is prepared to post to `kdu_client::post_window' before any
           can be cleared by the arrival of response data.  A reasonable
           value for this parameter might be 20 -- if the function is never
           called, this is likely to be the default value, although
           default values might change without notice.
         [ARG: min_aggregation_time]
           Lower bound on the T_agg parameter discussed above.  As mentioned,
           frame request aggregation might not be possible in some cases
           such as complex metadata-driven animations, so the lower bound
           might not be respected.  Moreover, the actual aggregation time
           might be much larger if client-server communication is stateless.
           [//]
           Reasonable values for this parameter might be between 0.1 and
           0.4, although smaller values could also be used.  If the function
           is never called, a default value of 0.13 seconds may be selected,
           but the specific default value should not be relied upon if this
           is important to your application.
      */
    void
      configure_auto_refresh(double min_refresh_interval)
      { this->min_auto_refresh_interval = min_refresh_interval; }
      /* [SYNOPSIS]
           The `min_refresh_interval' value affects the automatic
           synthesis of virtual animation frames by the `advance_animation'
           function when the source of data is a dynamic cache that is
           filled by a `kdu_client' object.  If a frame has a long display
           interval, it may be appropriate to render it multiple times
           (refresh), as more data arrives from the server.  The minimum
           separation between such refresh frames is `min_refresh_interval'
           seconds.  Refresh frame steps are only synthesized where the
           arrival of new data is noted by calls to the
           `notify_client_progress' function.
           [//]
           The `min_refresh_interval' value supplied here is also used to
           determine the minimum separation (expressed in terms of frame
           display times) between non-zero returns from the
           `get_suggested_refresh_action' function -- see that function
           for more information on how applications should use refresh
           suggestions.
           [//]
           A reasonable value for the `min_refresh_interval' parameter here
           might be 0.5.  If this function is never called, a default
           value (might be the same as the reasonable value mentioned here)
           will be in force.
      */
    KDU_AUX_EXPORT int
      generate_imagery_requests(kdu_client *client, int queue_id,
                                double next_display_event_time,
                                kdu_dims viewport, kdu_dims roi,
                                float scale, int max_quality_layers=(1<<16));
      /* [SYNOPSIS]
           This function works together with `notify_client_progress' to
           implement the JPIP request features of the animator object.
           The function issues zero, one or maybe more calls to
           `client->post_window', using the supplied `queue_id' and passing
           its own `custom_id' values in these calls so that it can later
           examine the progress of the individual window of interest requests
           that it posts.
           [//]
           If the animator has posted no previous requests,
           or the playback direction has recently changed (see `set_reverse'),
           the first call to `client->post_window' will be pre-emptive.
           Thereafter, subsequent requests are non-preemptive, but the
           `client->set_preferred_service_time' function is used to regulate
           the rate at which requests are ideally serviced.
           [//]
           Two things are worth noting about the internal algorithm that is
           used to synthesize window of interest requests:
           [>>] The algorithm is governed by a set of (usually constant)
                configuration parameters that may be set using the
                `configure_request_parameters' function.
           [>>] The generation of requests can get behind the animation
                process, but when this is the case (or there is insufficient
                slack in the timing), posted windows of interest are assigned
                reduced preferred service times, by a factor of as much as 2,
                until the server catches up, or until the number of
                outstanding requests exceeds a hard limit (see
                `configure_request_parameters').
           [//]
           This function does not itself check on the progress of outstanding
           requests; it only issues window of interest requests for image
           content.  However, it relies on progress information recovered
           by the most recent call to `notify_client_progress'.
           [//]
           The application should be sure to call `next_frame_has_changed' at
           some point after invoking this function, because new animation
           frames may become available as a side effect of the current
           function exploring future requests that can be posted.  The
           `next_frame_has_changed' function should be used by applications
           to trigger calls to `get_suggested_advance_delay' and ultimately
           `advance_animation', so that the animation is advanced at the
           most appropriate point, taking all factors into account.
         [RETURNS]
           The number of distinct calls to `client->post_window' that are
           issued.
         [ARG: queue_id]
           Identifies the client request queue to which the window of
           interest requests should be posted -- this should be 0 unless
           additional queues have been successfully created by
           `client->add_queue'.
         [ARG: next_display_event_time]
           This argument supplies the value of the application's
           display time clock that will be associated with the next display
           update event (i.e. the next frame refresh).  This argument has
           the same interpretation as its namesake in the `advance_animation'
           function.
         [ARG: viewport]
           This argument expresses the location and size of the application's
           viewport into the (possibly composited) image surface, at the
           scale defined by `scale'.  The viewport is expressed with respect
           to the top-left corner of the image surface and has the same
           meaning as viewports that are passed to the
           `kdu_region_comositor::set_buffer_surface' function, assuming
           that the `vflip', `hflip' and `transpose' arguments to
           `kdu_region_compositor::set_scale' are all false.  The viewport
           should have a non-empty intersection with the complete image
           region.
           [//]
           If the animation is metadata-driven and the metadata can be
           used to infer a region of interest, this region is used in
           preference to any region supplied via the `roi' argument and
           the requested region of interest is obtained by re-positioning
           the `viewport' over the centre of the metadata-defined ROI and
           then intersecting the two regions.
           [//]
           Otherwise, if the `roi' argument provides an empty region,
           the `viewport' is taken as the region of interest to be used
           in posting requests to the `client'.
           [//]
           If neither of the above apply, the `viewport' region is ignored
           and the `roi' argument determines the region of interest to be
           used in posting requests to the `client'.
         [ARG: roi]
           If this region is empty, or there is a metadata-driven animation
           with a metadata-defined region of interest, the region of interest
           used in posting requests to the `client' is formed using the
           `viewport', in conjunction with any metadata-defined region of
           interest, as explained for the `viewport' argument.  Otherwise,
           `viewport' is ignored and this argument provides the region
           to be used in posting requests.
         [ARG: scale]
           This argument has the same meaning as it does in calls to
           `kdu_region_compositor::set_scale'.  It is expected that the
           value supplied here would be identical to the scale at which
           the imagery will be rendered, typically via a
           `kdu_region_compositor' object.  The scale plays an important
           role in determining the resolution at which imagery is requested.
         [ARG: max_quality_layers]
           If this value lies in the range 1 to 65535, it is included as
           a quality limit with the generated requests.
      */
    KDU_AUX_EXPORT void
      notify_client_progress(kdu_client *client, int queue_id,
                             double next_display_event_time);
      /* [SYNOPSIS]
           Works together with `generate_imagery_requests' to implement the
           JPIP request features of the animator object.  This function
           would typically be called from a `kdu_client_notifier' object,
           in response to changes in the contents of the JPIP client cache
           represented by the `client' object.  This would usually be followed
           by a call to `generate_imagery_requests', but calls to that
           function may arise in other contexts -- especially when the
           animation is first started or there is some change in the
           animation timing, such as a call to `set_reverse'.
           [//]
           The function uses the `client->get_window_in_progress' and
           `client->get_window_info' functions to determine the status of
           window of interest requests that the object has posted via
           calls to `client->post_window' from inside the
           `generate_imagery_requests' function.
           [//]
           In the spirit of JPIP, we do not hold up progress in the
           animation to wait for the server.  Instead, if insufficient
           response data has arrived for the next frame in the animation
           to be rendered a reasonable amount ahead of its display time (as
           determined by calling `get_estimated_rendering_time'), the
           window of interest requests posted via `generate_imagery_requests'
           are assigned reduced preferred service times, so that the server
           should eventually catch up.  The application may choose to
           supplement this mechanism by introducing its own delay between the
           time at which the first request is sent to the server and the time
           at which the first call to `advance_animation' occurs.
           Alternatively, this may be left up to the interactive user, who
           can always choose to start the animation again if the initial
           frames have insufficient quality.
           [//]
           The application should be sure to call `next_frame_has_changed' at
           some point after invoking this function, because new auto-refresh
           frame steps may be introduced if the current function discovers
           that new service data has arrived for the current animation
           frame since it was last rendered.  The `next_frame_has_changed'
           function should be used by applications to trigger calls to
           `get_suggested_advance_delay' and ultimately `advance_animation',
           so that the animation is advanced at the most appropriate point,
           taking all factors into account.
         [ARG: queue_id]
           Identifies the client request queue to which the window of
           interest requests were posted from within previous calls to
           `generate_imagery_requests'.  It is important that this function
           be given the same `queue_id' value as that function.
         [ARG: next_display_event_time]
           This argument supplies the value of the application's
           display time clock that will be associated with the next display
           update event (i.e. the next frame refresh).  This argument has
           the same interpretation as its namesake in the `advance_animation'
           function.
      */
    KDU_AUX_EXPORT int
      generate_metadata_requests(kdu_window *client_window,
                                 double display_time_limit,
                                 double next_display_event_time);
      /* [SYNOPSIS]
           This function is called when constructing windows of interest to
           send to a JPIP client.  The `client_window' will be used to send
           urgent requests for important metadata over the `kdu_client'
           object's OOB (out-of-band) request channel.
           [//]
           This function does not request actual imagery.  In fact, on entry,
           the `client_window' object has been initialized to the empty state
           except that the `client_window->metadata_only' flag has been set
           to true.
           [//]
           To facilitate correct determination of the metadata requests that
           need to be made, the function attempts to parse the required
           metadata, which may cause frames to be instantiated, so that
           a future call to `next_frame_has_changed' may return true after
           this function returns.
        [RETURNS]
           The function returns the total number of calls to
           `client_window->add_metareq' that were generated inside the call.
           A value of 0 means that the object has no immediate need for any
           additional metadata from the server.
        [ARG: display_time_limit]
           Used to bound the set of frames for which header metadata might
           be requested by this function.
        [ARG: next_display_event_time]
           This argument supplies the value of the application's
           display time clock that will be associated with the next display
           update event (i.e. the next frame refresh).  This argument has
           the same interpretation as its namesake in the `advance_animation'
           function.  This argument allows the function to estimate display
           times for future frames, in the event that the `advance_animation'
           function has not yet been successfully called.
      */
  //---------------------------------------------------------------------------
  public: // Functions used to drive the animation between `start' and `stop'
    bool next_frame_has_changed()
      { bool res=next_frame_changed; next_frame_changed=false;  return res; }
      /* [SYNOPSIS]
           This function plays a particularly important role in applications
           where the ultimate source of data is a dynamic cache whose contents
           grow over time, either in response to client requests or for
           other reasons.  Use it together with `get_suggested_advance_delay'
           to determine the point at which `advance_animation' should be
           called.
           [//]
           The function returns (and then resets) the state of an internal
           variable that identifies whether calls to any of the following
           functions have resulted in the introduction (or possible
           introduction) of a new frame (or frame-step) into the animation,
           that would be returned by the next call to `advance_animation'.
           [>>] `insert_conditional_frame_step' -- this function inserts
                frames that have the same content and frame idx as a current
                frame, but a different display start time.  Frame steps only
                partition an existing frame into smaller steps, the purpose
                of which is application-dependent.  A typical application of
                frame steps is to modulate overlay intensities for a
                frame that has a long playback duration.
           [>>] `notify_client_progress' -- this function may insert
                auto-refresh frames that have the same content and frame idx
                as a current frame, but a different display start time.
                They are effectively identical to the frame steps inserted by
                `insert_conditional_frame_step', except that the former always
                take precedence, in the event that both are available.  The
                purpose of auto-refresh frames is to give the application an
                opportunity to re-render a frame after the arrival of more
                data from a remote server, where the frame has a long duration.
           [>>] `generate_imagery_requests' -- this function tries to
                discover the existence of future frames in the animation so
                that it can generate suitable client requests for the content
                of such frames; this includes metadata-driven frames.  The
                discovery of such frames may result in a next frame becoming
                available in the animation, where previously there was none.
           [>>] `generate_metadata_requests' -- this function also tries to
                discover the existence of future frames in the animation,
                requesting mandatory metadata for them.
           [//]
           For reference, in the "kdu_show" demo applications, this function
           is invoked from within an "on_idle" function that is visited once
           all run-loop processing completes.
           [//]
           If the function returns true, the right response is to invoke
           `get_suggested_advance_delay' to determine how long (if at all)
           the application should ideally wait before invoking
           `advance_animation' -- this waiting time may have been changed
           by the appearance of a new frame or frame step, as described
           above.  If no wait is recommended, the `advance_animation'
           function may be called immediately.  Waits are recommended to
           ensure that remote servers have a chance to transmit as much
           relevant imagery as possible before a frame is rendered -- not
           important if the content is local though.   If a wait of any
           significant duration is recommended, the application should
           arrange to be woken up at the suggested time, whereupon it should
           invoke `get_suggested_advance_delay' again, to make sure that the
           recommended waiting time has not been further extended by the
           availability of a subsequent animation frame that renders the
           use of a synthesized frame-step inappropriate.
           [//]
           The internal state variable, whose value is returned by this
           function, is reset both by calls to this function and by calls
           to `advance_animation'.
      */
    KDU_AUX_EXPORT double
      get_suggested_advance_delay(double next_display_event_time);
      /* [SYNOPSIS]
           This function plays a very important role in the implementation
           of animations that are ultimately fuelled by a dynamic cache,
           whose contents may grow as data is delivered by a remote server
           (e.g., in response to client requests) or for some other reason.
           [//]
           The function determines the display time that would be associated
           with any frame returned the next call to `advance_animation',
           based on all information currently available -- this information
           might change if the contents of a dynamic cache grow in the future
           or if the application deliberately introduces extra frame steps
           via calls to `insert_conditional_frame_step'.  However, the
           application can detect such changes via the `next_frame_has_changed'
           function.
           [//]
           Based upon the deduce next frame display time, this function
           determines a suggested time for the application should invoke
           `advance_animation', allowing a reasonable time for the application
           to render the frame's contents prior to its display time.  This
           allowance is the value returned by `calculate_cpu_allowance', that
           applications can also invoke if they find the need.  The
           function simply returns the difference between the suggested
           rendering time and the supplied `next_display_event_time'.
           [//]
           If the return value is negative or less than some small positive
           threshold that the application might define to avoid waiting for
           very small amounts of time, the application should go ahead and
           invoke `advance_animation' right away.  Otherwise, the application
           is recommended to schedule a wakeup call after the expiry of the
           suggested delay, whereupon it should re-inovke this function, just
           in case the situation has changed.
           [//]
           If the current animation frame (if any) is known to be the
           last one in the animation, the current function always returns
           a negative value, which encourages the application to invoke
           `advance_animation' as soon as possible, whereupon the
           `KDU_ANIMATION_DONE' code will be returned -- the application
           may need to detect this condition as soon as possible.
           [//]
           If the `advance_animation' function would return
           `KDU_ANIMATION_PENDING', based on all currently available
           information, this function returns a very large value (e.g.,
           10 seconds), encouraging the application to go idle until
           something changes -- e.g., arrival of more data from a remote
           server, resulting in the insertion of refresh frames by
           `notify_client_progress' or the discovery of actual new
           animation frames within `generate_imagery_requests'.
         [RETURNS]
           Number of seconds that the caller is recommended to wait before
           invoking `advance_animation'.  If the returned value is <= 0, the
           application should not wait.  If the returned value is +ve,
           the application would typically set up a timer to alert it once
           the relevant time arrives, whereupon it should call this
           function again, in case things have changed.
         [ARG: next_display_event_time]
           Value the display clock is expected to take at the next instant
           in which a rendered frame could potentially be presented to the
           display.
      */
    KDU_AUX_EXPORT int
      advance_animation(double cur_system_time,
                        double last_display_event_time,
                        double next_display_event_time,
                        bool need_refresh, bool skip_undisplayables);
      /* [SYNOPSIS]
           This is the first of two functions that propel the animation
           machinery.  This function requests the animation state machine to
           advance to the next frame to be rendered.  If the action succeeds,
           the function returns the index of the relevant frame (or compositing
           layer in JPX single layer mode), where 0 is the first index in the
           source.  Otherwise the function returns a -ve value, whose
           interpretation is described below.
           [//]
           It is important to realize that the animation sequence may
           visit a particular frame (or compositing layer) more than once --
           this is what happens if successive calls to `add_metanode' involve
           distinct regions of interest on the same physical frame or
           compositing layer.  Thus the frame index returned by this function
           does not uniquely identify the point to which the animation has
           advanced.  For metadata-driven animations, such a unique identifier
           may be obtained by calling `get_metadata_driven_frame_id'.
           [//]
           It is also worth noting that for metadata-driven animations, the
           frame index returned by this function may not be sufficient for
           the application to determine what to display.  This is because
           JPX animation frames are characterized by both a frame number and
           a presentation track index.  To recover a complete description
           of the frame that is to be presented, call `get_current_frame'
           after this function succeeds.
           [//]
           Frames that have zero duration are special in JPX animations,
           having an interpretation as "PAUSE" frames.  This function
           automatically skips over such frames if they appear at the
           start of the animation, but returns the special
           `KDU_ANIMATION_PAUSE' value if it encounters such frames elsewhere
           in the animation.  It is not possible to get past such pause frames
           without invoking `stop' and then `start'.
           If "repetition" is in force (see `set_repeat')
           the leading pause frames are skipped even when looping back to
           the start of the video.  Moreover, if playback is in the reverse
           direction (see `set_reverse'), all pause frames are skipped, since
           the pause semantics make the most sense when playing content in
           its intended direction.  For more on pause frames, see the
           comments appearing with `jpx_composition::get_frame_info'.
           [//]
           This and `note_frame_generated' are the only functions that accept
           time arguments from both the display event clock and the system
           clock.  These two different types of time are discussed at some
           length in the introductory comments appearing with the
           `kdu_region_animator' object.  System times are used to measure
           the amount of time that operations take to perform.  Display times
           are generally derived from a clock that is synchronized to a
           monitor's vertical blanking period.  The underlying assumption is
           that the application should be able to evaluate the system time
           immediately before calling a function, so that arbitrarily small
           time intervals can potentially be measured.  On the other hand,
           the display clock might be discrete, advancing by the nominal
           monitor refresh interval at discrete points.  This object is
           designed in such a way that all display times that the application
           is required to supply as function inputs always correspond to
           these discrete "display event times", so they do not require the
           evaluation of a timer.
           [//]
           The `next_display_event_time' value is used to initialize display
           times on the first successful attempt to advance the animation
           since the object was constructed or `start' was last called; it is
           also used to adjust the animation in the event that `need_refresh'
           is true.  `last_display_event_time' and `next_display_event_time'
           are used to estimate appropriate stitching points for individual
           segments of the auto-pan preambles that may be inserted at
           the start of metadata driven animation frames.  They are also
           used to implement the functionality associated with the
           `skip_undisplayables' argument.
           [//]
           The `cur_system_time' argument is used to collect
           processing throughput statistics, as reported by the
           `get_estimated_rendering_time' function.  The assumption is that
           once this function returns, the application will be in a position
           to start rendering any new animation frame, so that the time
           between this call and a subsequent call to `note_frame_generated'
           provides an indication of how long the processing takes -- this
           can be useful in adjusting the insertion of synthetic animation
           frames (see `insert_conditional_frame_step') in addition to
           providing the application with access to the collected statistics.
           [//]
           If `need_refresh' is false, the function advances to the next
           animation frame (if possible), except that if `skip_undisplayables'
           is true, the function may actually advance by multiple frames.
           Specifically, the next frame is considered to be "undisplayable" if
           there is a frame that follows it and that frame has a display time
           less than or equal to one display event period later than the
           current frame. Here, the display event period is taken to be
           the difference between `next_display_event_time' and
           `last_display_event_time'.  It is important to realize that the
           application is free to advance the animation as far as it likes
           ahead of the display event clock.  That is,
           `next_display_event_time' does not itself constrain the behaviour
           of this function when `need_refresh' is false.  The display
           times associated with frames returned by this function might be
           any number of frame display periods ahead of (or even behind)
           the `next_display_event_time'.
           [//]
           If `need_refresh' is true, the function's behaviour changes
           significantly.  Rather than advancing to the next frame in the
           animation (possibly skipping frames that could not be displayed),
           the function checks through an internal record of frames
           which have recently been generated or started (including the one
           that was the most recent subject of a call to this function) to
           determine the one that should be displayed at the
           `next_display_event_time'.  The function then revisits that frame
           in the animation.  Subsequent calls to this function with
           `need_refresh'=false move forward from that revisited point.  The
           `need_refresh' flag is useful where the application renders frames
           ahead of time into a jitter-absorbtion buffer.  If the viewing
           conditions are changed by the user, some of these frames may need
           to be regenerated.  Since the function never advances to a new
           animation frame when `need_refresh'=true, but only backtracks to
           a previous or current animation frame, the return value may need
           to be negative if there is no current animation frame at all.
           [//]
           After calling this function successfully and generating the frame,
           you should call `note_frame_generated'.  However, there is no reason
           why you cannot invoke `advance_animation' again immediately.
         [RETURNS]
           Non-negative return values indicate success, identifying the
           new frame index to which the animation is advancing.  Other
           return values are as follows:
           [>>] `KDU_ANIMATION_DONE' -- there are no more frames in the
                animation.  If playing forwards, this code
                means that the last frame in the animation was the one
                identified in the last successful call to this function.  If
                playing backwards, this code means that the first frame in
                the animation (often the frame with index 0) was the one
                identified in the last successful call to this function.
                This code would not normally be returned if the repeat mode
                is in force, unless the animation is found to contain no
                frames.
           [>>] `KDU_ANIMATION_PENDING' -- there is currently insufficient
                data available to advance to the next frame, but this
                condition may change in the future.  This code may be
                returned if the ultimate source of data is a dynamic cache
                (typically a cache being filled by communication with a
                JPIP server).
           [>>] `KDU_ANIMATION_PAUSE' -- the frame identified in the last
                successful call to this function had zero duration.  As
                explained with `jpx_composition::get_frame_info', these
                frames have special significance as "pause" frames.
                Once a pauses frame is encountered, calls to this function
                will always return `KDU_ANIMATION_PAUSE'.  The normal response
                would be to wait for user interaction before restarting the
                animation from the pause frame (or just being it).  When an
                animation is newly started, any initial pause frames are
                automatically skipped over.
         [ARG: cur_system_time]
           System clock time, measured by the application at the point when
           this function was called.  The origin for the system clock is
           arbitrary, but the application should ensure that valid
           `cur_system_time' values are always non-negative.
         [ARG:last_display_event_time]
           As with system times, display times must be non-negative.  The
           origin for the display clock is arbitrary, and need not coincide
           with that for the system clock.  Moreover, the system and display
           clocks need not advance at exactly the same rate.  For most
           applications, it is sufficient for `last_display_event_time' to
           have the form n*T (or T0 + n*T) where T is the nominal refresh
           interval of the intended monitor or other display device, and n
           is the number of vertical blanking periods (or VSYNC events) that
           have occurred since some arbitrary point in the past.  We think of
           T as the "display even interval".
           [//]
           This argument is used only to compare with `next_display_event_time'
           in order to estimate the display event interval T, which
           is used to implement the `skip_undisplayables' functionality and to
           determine good autopan pre-amble switching points for
           metadata-driven animation frames.  You should endeavour to ensure
           that the separation between `next_diplay_event_time' and
           `last_display_event_time' is consistently equal to the display
           event interval -- although the application can change this display
           event interval at any time if this is sensible.
         [ARG: next_display_event_time]
           This argument affects the way in which animation frame display
           times are generated.  When an animation begins (right after the
           object is constructed or `start' was last called), the initial
           animation frame's display time is set to `next_display_event_time'.
           If `want_refresh' is true, the value of `next_display_event_time'
           determines the animation frame that is to be refreshed.
         [ARG: want_refresh]
           See explanation given above.
         [ARG: skip_undisplayables]
           The purpose of this argument is to facilitate the implementation
           of "fast forward" services, where frame durations are artificially
           shortened via the `set_timing' function.  In this case, it can
           happen that animation frame display times are bunched so tightly
           together that many such frames should nominally be displayed
           between two consecutive display events.  Rendering all of these
           frames would be a waste of resources.  To this end, if this
           argument is true, the function automatically skips over frames
           that will not be displayed, assuming that the current frame is
           to be displayed.  Specifically, it is considered that frames
           following the current frame will not be displayed if there are
           even later frames whose display time is no later than the
           current frame's display time plus T, where T is the assumed
           display event interval determined by taking the difference
           between `last_display_event_time' and `next_display_event_time'.
      */
    KDU_AUX_EXPORT void
      note_frame_generated(double cur_system_time,
                           double next_display_event_time);
      /* [SYNOPSIS]
           Once you have finished generating a frame, you should call this
           function.  That is, you call `advance_animation' first and then,
           after generating the frame contents, you call
           `note_frame_generated'.
         [ARG: cur_system_time]
           This argument has the same origin and interpretation as its
           namesake, supplied to `advance_animation' -- it is the real
           absolute system time, measured with respect to some consistent
           origin.  This information is used to collect statistics on the
           true rate at which frames are being generated and how long the
           processing is taking.
         [ARG: next_display_event_time]
           This argument also has the same origin and interpretation as its
           namesake, supplied to `advance_animation' -- it is used only to
           initialize display time parameters if this is the first call to
           `note_frame_generated' since the object was constructed or
           `start' was last called.
      */
  //---------------------------------------------------------------------------
  public: // Additional functions required for generating frames
    KDU_AUX_EXPORT int
      get_current_frame(jpx_frame &jpx_frm, mj2_video_source * &mj2_trk);
      /* [SYNOPSIS]
           Returns the frame index of the current frame, as returned by
           the last successful non-pause call to `advance_animation'.
           Along with this, the function also returns sufficient information
           to precisely localize the relevant imagery within its data source.
           [//]
           For MJ2 data sources, the function sets `mj2_trk' to point to
           the MJ2 video track to which the frame belongs and sets `jpx_frm'
           to an empty interface.
           [//]
           For JPX data sources in single layer mode, the function sets
           `mj2_trk' to NULL and `jpx_frm' to an empty interface; in this
           case, the returned "frame index" is actually the absolute
           compositing layer index of the layer to be displayed.
           [//]
           For JPX data sources in composited frame mode, the function sets
           `mj2_trk' to NULL and makes `jpx_frm' an interface to the
           relevant frame.  The caller may use `jpx_frm.get_track_idx' to
           identify the first presentation track to which the frame belongs,
           along with any later tracks with which it may be compatible.
      */
    int get_suggested_refresh_action()
      { 
        int result = (client_refresh_state <= 0)?0:client_refresh_state;
        if (result > 0)
          { client_refresh_state=0; earliest_client_refresh=-1.0; }
        return result;
      }
      /* [SYNOPSIS]
           Returns non-zero if the application is recommended to invoke
           `kdu_region_compositor::refresh' before rendering the most
           recent frame returned by `advance_animation'.  If not using
           `kdu_region_compositor', the application should still take the
           advice that any previously reconstructed imagery that might be
           used in rendering the current animation frame should be
           regenerated from scratch, because more data has arrived in the
           client cache.
           [//]
           This function can only return non-zero if `notify_client_progress'
           is being invoked regularly.  The separation between non-zero
           returns from this function (measured in terms of the display
           times associated with frames returned by `advance_animation') is
           lower bounded by the value passed to the `configure_auto_refresh'
           function.
           [//]
           Note that after a non-zero return from this function, subsequent
           calls will return 0 until a new refresh condition is established
           by the advance of the animation together with the detection of
           new service data within `notify_client_progress' calls.
         [RETURNS]
           0 if there is no need for the application to take any specific
           action.  Otherwise, the function returns 1 if refresh is
           recommended, while it returns 2 if the frame most recently
           returned by `advance_animation' has the sole purpose of marking
           a refresh point -- i.e., it is an auto-refresh frame step that
           subdivides a frame whose duration is otherwise long enough to
           make it worthwhile refreshing and re-rendering the frame multiple
           times to utilize information that may have been arriving
           continuously in the underlying dynamic cache.
      */
    KDU_AUX_EXPORT double get_display_time();
      /* [SYNOPSIS]
           This function returns the display time associated with the current
           animation frame (determined by the most recent successful call to
           `advance_animation') or else -1 if there has been no successful call
           to `advance_animation' since the last call to `start'.
           [//]
           It is expected that the caller places generated frame into
           a jitter absorbtion queue with the returned time stamp, that is used
           by a scheduled presentation process to present and strip frames from
           the jitter absorbtion queue.  This allows the caller to generate
           frames ahead of their presentation times, up to the limits of the
           jitter absorbtion queue.
           [//]
           The display time does not typically change between calls to
           `advance_animation', unless there is an intervening call to
           `retard_animation'.  However, the display time of the very first
           animation frame following a call to `start' or a change in
           animation direction (see `set_reverse') is initially set to the
           `cur_time' value passed to `advance_animation' and then adjusted to
           the `cur_time' value passed to `note_frame_generated'.  This ensures
           that the animation process can start smoothly from the point at
           which a frame first becomes available.  For this reason, you should
           generally derive a frame's time stamp by calling `get_display_time'
           after `note_frame_generated'.
      */
    KDU_AUX_EXPORT jpx_metanode get_current_metanode();
      /* [SYNOPSIS]
           Returns the `jpx_metanode' object that is associated with the
           current animation frame, as supplied to `add_metanode'.  During
           regular video playback (as opposed to metadata-driven animation),
           this function returns an empty interface.
      */
    KDU_AUX_EXPORT int get_metadata_driven_frame_id();
      /* [SYNOPSIS]
           If there is no current frame or the animation is not metadata-driven
           (see `get_metadata_driven'), this function returns -1.  Otherwise,
           it returns an ordinal identifier (starting from 0) for the
           current metadata-driven animation frame.  These ordinal identifiers
           are guaranteed to increase monotonically, but some integer values
           might be skipped.  In particular, this can happen if a number
           list matches multiple JPX frames; relaxing the requirement for
           the metadata-driven frame ID's to be sequential integers allows
           us to ensure the monotonicity when playing forwards or backwards
           without having to figure out ahead of time exactly how many
           frames match the number list.  In any case, the identifiers
           should usually be sequential integers and holes in the sequence
           are not likely to be large.
           [//]
           The ordinal value returned here should be the same regardless of
           whether the animation is played in the forward or reverse
           direction; that is, frame id's should increase consecutively from
           0 in the forward direction and decrease from the maximum value
           down to 0 when the object is in reverse mode (see `set_reverse').
           [//]
           However, in the case where the metadata required to construct
           animation frames arrives incrementally in a JPIP cache, so that
           the number of metadata frames reported by
           `count_metadata_driven_frames' increases over time, the ordinal
           values returned by this function for any given frame may need to
           change as the count increases.  However, this only happens when
           the object is in reverse mode.  Nevertheless, at any given point,
           the ordinal values are guaranteed to be unique, correctly
           sequenced, and in the range from 0 to `count'-1, where `count'
           is the value returned by `count_metadata_driven_frames'.
      */
    KDU_AUX_EXPORT bool count_metadata_driven_frames(int &count);
      /* [SYNOPSIS]
           If the animation is not metadata-driven, this function returns
           true, setting `count' to 0.  Otherwise, the function sets `count'
           to the smallest known exclusive upper bound on the values that
           can be returned by `get_metadata_drive_frame_id', returning
           true if all metadata-driven frames are currently known and
           false otherwise.  A false return invariably means that
           insufficient data is currently available from a JPIP cache.
           [//]
           Note that although the intent is for `count' to represent the
           total number of available metadata-driven frames, in cases where
           a single number list matches a potentially large number of
           JPX frames it is not reasonable to try to discover them all up
           front.  Instead, the function allocates a sufficient number of
           metadata frame-ID's to span all of the matching frames, augmenting
           `count' by this value.  If some of the ID's in this range turn out
           to be not needed, the `count' may be substantially larger than
           the number of actual metadata-driven frames.
           [//]
           The intent is that the `count' returned here be used to determine
           the range for a slider control that allows an interactive user
           to see roughly where the current frame belongs by comparing the
           value returned by `get_metadata_driven_frame_id' with the
           expected range for such values, which is 0 to `count'-1.
      */
    KDU_AUX_EXPORT bool
      get_current_geometry(kdu_coords &full_size, kdu_dims &roi);
      /* [SYNOPSIS]
           This function returns true so long as there is a current animation
           frame, setting the `full_size' and `roi' values.  Otherwise, the
           function returns false, leaving `full_size' and `roi' untouched.
           [//]
           Unlike the functions below, the present function does not need
           access to a configured `kdu_region_compositor' object in order to
           determine the geometry of the frame.  However, the returned
           information may need to be corrected for an intended presentation
           scaling factor and orientation -- information that would be
           supplied to `kdu_region_compositor::set_scale'.  Apart from this,
           the `full_size' vector should agree exactly with the full image
           dimensions returned by an initialized `kdu_region_compositor'
           object whose presentation scaling factor is set to 1.0.  Moreover,
           if there is a region of interest, the region returned via `roi'
           should agree exactly with that which would be recovered from an
           appropriately initialized `kdu_region_compositor' object.  If there
           is no specific region of interest, the `roi' argument is set to an
           empty region.
      */
    KDU_AUX_EXPORT void
      get_roi_and_mod_viewport(kdu_region_compositor *compositor,
                               kdu_region_animator_roi &roi_info,
                               kdu_dims &mod_viewport);
      /* [SYNOPSIS]
           This function returns information about any mapped region of
           interest that may apply to the current animation frame.
           If there is no region of interest or no current animation frame, the
           function does nothing at all.  Otherwise, it records the region of
           interest information within `roi_info' and also modifies
           the contents of `mod_viewport', as explained below.
           [//]
           The function assumes that the `compositor' object has already been
           configured to composit the frame whose index was returned by the
           last call to `advance_animation', but it does not modify the state
           of the `compositor' object in any way.
           [//]
           The purpose of calling this function is: a) to determine a good
           buffer surface region to be rendered; and b) to record any region
           of interest information along with the generated frame buffer once
           it has been rendered.
           [//]
           The information stored within `roi_info' may describe a dynamic
           transition between two regions.  The dynamics of this trajectory
           are described in the comments following the definition of
           `kdu_region_animator_roi'.
           [//]
           `mod_viewport' is used to pass in the location and dimensions of
           the application's current viewport and to return a modified region
           that represents a recommended buffer surface to be rendered for
           the current animation frame.  If there is no region of interest,
           the best thing to do is to render the viewport itself.  However,
           if there is a region of interest, the function generally translates
           `mod_viewport' so as to centre it over the region of interest.
      */
    KDU_AUX_EXPORT static kdu_dims
      adjust_viewport_for_roi(kdu_region_compositor *compositor,
                              kdu_dims &mod_viewport,
                              jpx_metanode metanode);
      /* [SYNOPSIS]
           This is a static version of the above function that derives all the
           necessary information to adjust the viewport from `metanode', as
           opposed to a current animation frame.  If `metanode' is the
           interface returned by `get_current_metanode', this function should
           behave identically to the above one, unless the metadata content has
           somehow changed.  However, you should always use the first form of
           the function when there is an active animation.  The main reason
           for providing this form of the function is to allow an application
           to reconstruct the same parameters as those that were used in the
           most recently displayed frame of an animation, after it has stopped,
           by passing in the `metanode' associated with that most recently
           displayed frame.  Unlike the `get_roi_and_mod_viewport'
           function, this one returns only a single region of interest
           (possibly empty) based upon the `metanode'.
      */
  //---------------------------------------------------------------------------
  public: // Functions used to recover statistics
    double get_avg_frame_rate()
      { return (mean_frame_interval <= 0.0)?-1.0:(1.0/mean_frame_interval); }
      /* [SYNOPSIS]
           This function returns the current observed average rate at which
           the animation frame is being advanced, measured relative to the
           display clock.  For this purpose, conditional frame steps
           introduced via `insert_conditional_frame_step' are not treated
           as animation frame advances.  However, frames that are skipped
           as a result of the `skip_undisplayables' argument to
           `advance_animation' are considered as advances.  The value
           returned by this function can be used by an application to
           report frame rates to an interactive user.
           [//]
           If the `retard_animation' function is never called, these frame
           rates should be identical to the source frame display rates,
           modified in accordance with the parameters supplied to
           `set_timing'. If `retard_animation' is called, it will have the
           effect of spreading the display times associated with consecutive
           frames in the animation, thereby lowering the observed average
           frame rate.
           [//]
           The function returns a negative value if less than two original
           animation frames have been initiated so far.
      */
    double get_estimated_rendering_time()
      { return (max_rendering_time < 0.001)?0.001:max_rendering_time; }
      /* [SYNOPSIS]
           Returns the expected number of seconds required to fully render a
           new animation frame.  The function obtains this information by
           measuring the average change in system time, between calls to
           `advance_animation' and `note_frame_generated'.  However, certain
           types of frames are excluded from this average.  Specifically,
           frame steps introduced by `insert_conditional_frame_step' do not
           contribute, since they do not involve any change in the frame
           index or region of interest.  Returns 0 if nothing can be
           deduced about the throughput of the rendering process.
      */
    double calculate_cpu_allowance()
      { 
        double allowance = 0.1 + 2.0*get_estimated_rendering_time();
        return (allowance < 0.2)?0.2:allowance;
      }
      /* [SYNOPSIS]
           The purpose of this function is to return a consistent reasonable
           time allowance for rendering of an animation frame.  Currently,
           this reasonable allowance is obtained by doubling the value
           returned by `get_estimated_rendering_time' and adding 0.1, but
           this policy could be changed in the future.  By changing things
           in just this one place, all code that depends on such estimates
           should change in a consistent way.
      */
  //---------------------------------------------------------------------------
  private: // Helper functions
    friend struct kdra_frame;
    kdra_frame *get_frame();
      /* Allocates a new `kdra_frame' object, recycling it from the internal
         free list, if possible. */
    void recycle_frame(kdra_frame *frm);
      /* So it can be recycled by future calls to `get_frame'. */
    kdra_frame *
      insert_new_frame(kdra_frame *ref, bool insert_before_ref);
      /* This function allocates a new frame (using `get_frame') and inserts
         it immediately after or before the frame identified by `ref'.  If
         `ref' is NULL, the new frame is inserted after the current `tail'
         (if `insert_before_ref' is false) or before the current `head'
         (if `insert_before_ref' is true) of the frame list. */
    void relabel_metadata_driven_frames();
      /* Required only when metadata-driven frames are discovered
         incrementally by parsing a dynamic cache, and when the reverse
         mode is in force -- in that case, newly discovered frames may
         have incorrectly sequenced `metadata_frame_id' labels. */
    bool remove_frame(kdra_frame *frm, bool force_remove=false);
      /* If `force_remove' is false and the `client_reqs' member is
         non-NULL, this function does nothing, returning false.  If
         `force_remove' is true, the function removes all `kdra_frame_req'
         objects that reference `frm' (discovered via `client_reqs' and
         `kdra_frame_req::next_in_frame' links) before removing the
         frame.  In the latter case, or if `frame_reqs' is NULL on entry, the
         function recycles `frm' and reconnects the frame list, returning
         true.
         [//]
         If `frm'==`current', `current' is moved back until it reaches
         `head' -- if that is not possible, `current' is set to NULL -- if
         that happens, `next_video_frame_idx' is set to the value of
         `current->frame_idx', if appropriate, before it becomes NULL. */
    kdra_frame_req *append_frame_req(kdra_frame *frm);
      /* Appends a new element to the list of `kdra_frame_req' objects
         headed by `frame_reqs', arranging for the new element to refer to
         `frm'. */
    void remove_frame_req(kdra_frame_req *elt, bool from_tail=false);
      /* Removes an element from the `kdra_frame_req' list being maintained
         by the object, invoking `elt->detach' and adjusting the
         `frame_reqs' and `last_frame_req' members accordingly.  If
         `from_tail' is true, the element being removed is currently the
         tail of the `frame_reqs' list; otherwise it must be the head; no
         other options are guaranteed to work 100% correctly, so assertion
         failures will occur if `elt' is neither the current head nor the
         current tail of the list.  Tail removal is relevant only when the
         function is invoked from within `trim_imagery_requests'. */
    kdra_client_req *
      append_client_request(int user_layers,
                            kdu_dims user_roi, kdu_dims user_viewport);
      /* Appends a new element to the list of `kdra_client_req' objects
         headed by `client_requests', incrementing `num_client_requests'
         along the way.  The three arguments are used to make sure we
         do not fail to initialize the `kdra_client_req' members with the
         same names. */
    void remove_client_request(kdra_client_req *elt);
      /* Removes an element from the list of client requests being maintained
         by the object, decrementing the `num_client_requests' count, invoking
         `elt->detach_frame_reqs' and adjusting the `client_requests' and
         `last_client_request' members.  In practice, we either delete
         requests from the head or tail of the list, but not from
         intermediate points; however, this function does not impose
         such restrictions. */
    void abandon_all_client_requests();
      /* This function is called if timing changes (e.g., speeding up the
         playback rate) or inability to push requests into the client
         quickly enough (subject to the client's reported request horizon)
         cause the request process to fall behind the current animation
         frame's display time.  The function is also called upon disruptive
         events, such as calls to `stop' or `set_reverse'.  The function
         removes all internal records of outstanding client requests;
         specifically, it removes all records on the `client_requests'
         and `frame_reqs' lists and sets `num_client_requests' to 0.
         Any subsequent call to `generate_imagery_requests' will cause
         a pre-emptive window request to be posted to `kdu_client'. */
    void initialize_ref_display_clock(double next_display_event_time)
      { 
        assert(next_display_event_time >= 0);
        ref_display_clock_base = next_display_event_time;
      }
    kdu_long display_time_to_ref_usecs(double display_time)
      { 
        assert(ref_display_clock_base >= 0.0); // Make sure intialized
        display_time -= ref_display_clock_base;
        return (kdu_long)(0.5 + 1000000.0*display_time);
      }
      /* The above two functions are used to convert display times (in seconds)
         into a reference display clock that runs in microseconds.  The
         clock is initialized (using the first function) by the
         `generate_imagery_requests' function, if it finds that the client
         request list is currently empty.  The reason for offsetting the
         display clock is to deal with the unlikely event that `kdu_long'
         might be a 32-bit integer, as opposed to a 64-bit integer. */
    void post_last_client_request(kdu_client *client, int queue_id,
                                  kdu_long nominal_display_usecs,
                                  kdu_long last_cumulative_display_usecs,
                                  kdu_long last_cumulative_request_usecs,
                                  kdu_long request_to_display_offset_usecs,
                                  kdu_long cpu_allowance_usecs);
      /* This function is called from within `generate_imagery_requests' once
         a request has been assembled and is ready to post to the `client'
         using the indicated queue.  The function assigns the last client
         request a non-zero `custom_id' value before posting the request.
         On entry, the `last_client_request' object exists and its
         `kdra_client_req::cumulative_display_usecs' member holds the
         reference display clock value associated with the point at which
         the last frame (or fraction of a frame) associated with the request
         is expected to end its display.  The `last_cumulative_display_usecs'
         argument provides the same quantity corresponding to the previous
         request.  The function assigns a `kdra_client_req::requested_usecs'
         value which it passes to `client->post_window'; it then adds this
         value to the `last_cumulative_request_usecs' argument to determine
         the value for `kdra_client_req::cumulative_request_usecs'.
         [//]
         Normally, the computed `requested_usecs' value for the posted
         request is nothing other than the difference between the
         `kdra_client_req::cumulative_display_usecs' value that is present
         on entry and the supplied `last_cumulative_display_usecs' argument.
         If there have been changes to the dispay timing, however, this
         difference could actually be negative.  For this reason, the
         `nominal_display_usecs' argument is provided -- it represents the
         nominal amount of display time that is represented by this request,
         which will normally be equal to the gap between
         `kdra_client_req::cumulative_display_usecs' and
         `last_cumulative_display_usecs', except where there have been
         timing changes.  The `requested_usecs' value is prevented from
         becoming smaller than half of the `nominal_display_usecs' value.
         [//]
         As explained in the notes following the definition of
         the `kdra_client_req' structure, initial assigned values for
         `requested_usecs' are also typically reduced by up to 50%,
         in order to give the client-server communications a chance to catch
         up, so that eventually the requested data arrives prior to the point
         at which it is needed for display.  The function determines whether
         or not this is necessary by using the last two arguments:
         [>>] The `cpu_allowance_usecs' is simply the amount of CPU time that
              we allow for rendering a frame.
         [>>] The `request_to_display_offset_usecs' is our current best guess
              as to the number of microseconds that needs to be added to the
              `kdra_client_req::cumulative_request_usecs' member of any
              request in order to find the reference display clock value
              corresponding to the time at which we expect the requested
              content to have been fully served.  This was computed by the
              caller, based on information returned by the
              `kdu_client::sync_timing' function.
         [//]
         The function makes any adjustments to `requested_usecs' on the basis
         of the fact that we would like the requested content (or at least
         the first `min_auto_refresh_interval' seconds of it) to all be
         available before the point at which we expect the first frame
         associated with the request will need to be displayed.  These are
         only negative adjustments (i.e., reduce the amount of requested
         service so that communications can catch up). */
    void trim_imagery_requests(kdu_client *client, int queue_id);
      /* This function is called from within `generate_imagery_requests' if
         a change in scale or in animation timing is detected, or if the
         viewport or a defined region of interest is changed, or if the
         number of quality layers of interest is changed.  The function
         invokes `client->trim_timed_requests', from which it determines the
         number of trimmed service microseconds T, and the identify of the
         earliest request that was fully or partially trimmed R, for which
         the first requested frame-req was F.
            If the function determines that all outstanding timed requests
         have been trimmed away, so that none have been sent or partially
         sent yet not completely serviced, the function invokes
         `abandon_all_client_requests' and returns -- the caller must thus
         be prepared for the fact that the `client_requests' list may be
         empty upon return.  Otherwise, the function proceeds as follows:
            The function removes all client requests after R and also removes
         request R, unless R is equal to `client_requests' (i.e., head of the
         request list) or R is the first request referenced by its
         `kdra_frame_req::client_reqs' array.  If R is not removed, and R is
         not the first request referenced by F's `client_reqs' array, the
         first such request, if any, is removed, and R is moved to the first
         place in the `client_reqs' array.  Along the way, the function also
         removes all frame-reqs after F.  It is possible that there is no
         F, because the animation display process has moved past all
         frames originally requested with R; in that case, the function
         leaves the `frame_reqs' list empty.
            Prior to removal of any client requests, let CD and CR denote the
         values of the last client request's `cumulative_display_usecs' and
         `cumulative_request_usecs' members.  After removal of any client
         requests, as described above, the corresponding members of
         the (possibly different) `last_client_request' element are adjusted
         to CD-T and CR-T, respectively.
            The above steps either leave the `frame_reqs' list empty, or else
         they leave frame-req F at the tail of the frame requests list (i.e.,
         the element referenced by `last_frame_req'), with exactly one free
         slot in its `kdra_frame_req::client_reqs' array.  In the latter case,
         F's `kdra_frame_req::requested_fraction' member is set to a small
         value (e.g., 0.01); the exact value does not matter, since in any
         event the `generate_imagery_requests' function will set the requested
         fraction to 1.0 after including F in the next request, or possibly
         omitting any request for F if the current display time is too far
         advanced.
      */
    void backup_to_display_event_time(double display_event_time);
      /* This function is used during refresh operations or when the playback
         direction needs to be reversed.  The function examines the `current'
         frame to determine whether its display duration includes the
         `display_event_time'.  If not, the function walks back to the most
         recent frame in the queue whose duration does include
         `display_event_time', adjusting `current' accordingly.  Along the way
         the function removes any conditional frame steps that it encounters
         and also winds all frames which have dynamic pan transitions back to
         their starting point, adjusting their `cur_roi_pan_pos' and
         `next_roi_pan_pos' back to 0 and modifying their `display_time' and
         `cur_display_time' to reflect the time at which the frame would
         have commenced its auto-pan preamble. */
    static kdu_dims
      compute_mapped_roi(kdu_region_compositor *compositor,
                         jpx_metanode numlist, kdu_dims src_roi);
      /* Used by `adjust_viewport_for_roi' and `get_roi_and_mod_viewport' to
         map the source ROI (if any) to a rendering region of interest in
         the coordinate system of the `compositor' object.
            If `numlist' is an empty interface or contains no compositing
         layer references and no codestream references, the function returns
         an empty region.
            The function uses `compositor->get_next_ilayer' to walk through
         all active imagery layers in use by the `compositor' object.  If
         `numlist' references any compositing layers, the function ignores
         all such imagery layers that do not use one of the compositing
         layers referenced by `numlist'.  Also, if `src_roi' is non-empty, or
         if `numlist' references no compositing layers, the function
         ignores all imagery layers that do not use one of the codestreams
         referenced by `numlist'.
            If `src_roi' is empty, the function returns the smallest
         rectangular region that covers all of the imagery layers that are
         not ignored in the manner described above.
           If `src_roi' is non-empty, the only imagery layers that are not
         ignored are those that use one of the codestreams referenced by
         `numlist'.  The `src_roi' region is interpreted with respect to the
         first of these codestreams and inverse mapped back to the domain of
         the imagery layer.  The function returns the smallest rectangular
         region that covers all such inverse mapped copies of `src_roi'. */
    static void
      adjust_viewport_for_roi(kdu_dims mapped_roi, kdu_dims image_dims,
                              kdu_dims &viewport);
      /* Used by `adjust_viewport_for_roi' and `get_roi_and_mod_viewport',
         this function leaves `viewport.size' untouched, but adjusts
         `viewport.pos' so that the viewport is centred over the
         `mapped_roi', if possible, noting that the viewport must lie
         within the region defined by `image_dims'. */
    kdra_frame *instantiate_next_frame(kdra_frame *base);
      /* The object of this function is to instantiate the frame which is
         supposed to follow `base', or the frame which is supposed to
         follow `current' if `base' is NULL, or the very first frame if `base'
         and `current' are both NULL.  By "instantiate", we mean add the
         relevant `kdra_frame' entry to the list, if necessary, successfully
         invoke `expand_incomplete_frame_metadata', if necessary, and find
         valid `frame_idx' and other members for the new frame.  If successful,
         the instantiated frame is returned; otherwise, the function returns
         NULL.  If a frame cannot be instantiated, it is because the animation
         has finished or the metadata required for a metadata-driven
         animation has not yet arrived.  You should note that this function
         may need to remove a frame if it turns out that the frame does not
         actually exist when we come to instantiate it.  In that case, the
         function proceeds to instantiate the next frame, if there is one,
         and so forth. */
    void note_new_frame(kdra_frame *frm);
      /* This function is called when `frm' is first instantiated or when
         its `have_all_source_info' member becomes true for the first time.
         The function's sole purpose is to set the `next_frame_changed' flag
         if `frm' is indeed the next frame that would be returned by
         `advance_animation' -- this allows the application to become aware
         of the availability of a new frame where the last call to
         `advance_animation' returned `KDU_ANIMATION_PENDING'.  The
         `next_frame_changed' flag is reset each time `advance_animation'
         or `next_frame_has_changed' is called. */
    void conditionally_apply_accumulated_delay();
      /* The purpose of this function is to convert display delays that
         have been accumulated through calls to `retard_animation' into
         changes in the display times for future frames.  The function
         is called from within `retard_animation' and also from
         `advance_animation', where display times are assigned.  The main
         reason for having this function is to avoid inserting delays
         into the midst of auto-pan sequence, which would cause unnatural
         jerkiness in the displayed data. */
  //---------------------------------------------------------------------------
  private: // Data members that identify the object's current mode
    bool repeat_mode;
    bool reverse_mode;
    double custom_frame_interval; // -ve if not using a custom frame rate
    double native_frame_interval_multiplier;
    double intra_frame_interval; // Frame interval to use if next or prev
      // animation frame has the same frame index -- only if metadata-driven.
    double max_pan_speed; // Measured in pixels per second
    double pan_acceleration; // Measured in pixels per second per second
  //---------------------------------------------------------------------------
  private: // Source-specific machinery
    jpx_source *jpx_src;
    mj2_source *mj2_src;
    mj2_video_source *mj2_track;
    jpx_composition composition_rules;
    jpx_meta_manager meta_manager;
    kdu_uint32 track_idx;
    bool single_layer_mode; // Only allowed for JPX sources
    bool metadata_driven;
    kdra_frame *incomplete_meta_head; // Head/tail of frame sub-list
    kdra_frame *incomplete_meta_tail; // with incomplete metadata.
  //---------------------------------------------------------------------------
  private: // Frame management
    int video_range_start;
    int video_range_end;
    int next_video_frame_idx; // Used only if `current' is NULL
    int max_source_frames;
    int metadata_frame_count; // Returned by `count_metadata_driven_frames'
    kdra_frame *head; // If metadata-driven all frames are kept on
    kdra_frame *tail; // the list; else, generate & discard on demand
    kdra_frame *current; // Points to the current animation frame
    kdra_frame *cleanup; // Points to next frame to consider cleaning up
    kdra_frame *free_list; // Stores recycled frames
    bool next_frame_changed; // Used to implement `next_frame_has_changed()'
    bool accumulate_display_times; // If false, `current->display_time' is
          // adjusted to the `cur_time' value passed to `note_frame_generated'
  //---------------------------------------------------------------------------
  private: // Client request management
    int min_outstanding_client_requests; // Minimum request ahead, regardless
      // of timing issues, if anything to request; we can always trim extra
      // requests away if conditions change.
    int max_outstanding_client_requests; // Limits unbounded request generation
                                         // Must be greater than above member.
    double client_request_t_agg;
    double client_request_min_t_agg;
    double min_auto_refresh_interval;
    double earliest_client_refresh; // This is a frame display time; see below 
    int client_refresh_state; // Meaningful values: 0, -1, 2; see below
    kdu_window_prefs client_prefs;
    kdra_frame_req *frame_reqs;
    kdra_frame_req *last_frame_req;
    int num_client_requests; // Number of entries on `client_requests' list
    double ref_display_clock_base; // See `display_time_to_ref_usecs'.
    kdra_client_req *client_requests; // Elements are removed from head of list
    kdra_client_req *last_client_request; // as soon as response is complete.
    kdu_long next_request_custom_id;
    float last_client_request_scale; // Used by `generate_imagery_requests' to
                 // determine whether a change in scale (or timing parameters)
                 // may have occurred, requiring regeneration of requests.
    kdu_coords ref_comp_subsampling[33]; // Used to form window-of-interest
                 // requests in an optimal way; ideally, the information here
                 // is updated based on each relevant codestream's main header.
                 // If a codestream header is not available, the values
                 // discovered for previous codestreams can be used instead.
  //---------------------------------------------------------------------------
  private: // Preferred display times and delay info
    double delay_accumulator; // Accumulates animation retards until applicable
    double display_event_reference; // For auto-pan preambles, stitching times
    double display_event_interval; // are aligned to times of the form R+I*k.
  //---------------------------------------------------------------------------
  private: // Statistics
    double last_original_display_time; // For finding mean frame interval
    double mean_frame_interval; // Measured using the display clock
    int num_frame_interval_updates;
    double max_rendering_time; // Peak detector with exponential decay applied
      // to the observed gap (CPU seconds) between `advance_animation' and
      // `note_frame_generated'.
};

/* Notes:
        The elements of the `kdra_frame' list that runs from `head'
     to `tail' are connected via their `next' and `prev' members in the current
     animation order.  That is, `next' points to the next frame to visit, if
     non-NULL, whereas `prev' points to the most recently visited frame.  When
     the `set_reverse' function changes the order of the animation, the
     `next' and `prev' members must be swapped around and `head' and `tail'
     are also swapped.
        In metadata-driven playback, the `kdra_frame' list contains
     all frames, including incomplete frames that may be used to discover
     new animation frames as the relevant metadata becomes available.  In
     this case, if the repeat mode is in force and there is more than one
     frame, `tail->next' points to `head' and `head->prev' points to `tail'.
     Otherwise, when the repeat mode is not in force, `head->prev' and
     `tail->next' are necessarily NULL.
        In regular video playback, `head' and `tail' never form a ring and
     the animation frame list generally contains only the current frame, a
     few previous frames, and possibly a few next frames that have been
     instantiated ahead of time so that their properties can be tested or
     used to generate JPIP client requests.
        The `earliest_client_refresh' and `client_refresh_state' members
     work to implement the `get_suggested_refresh_action()' function's
     behaviour, when the ultimate source of data is a dynamic cache.  The
     refresh behaviour works as follows.  When `notify_client_progress()' is
     called, several things happen.  One of these is to check whether the
     currently active animation frame has received additional service data
     since it was last generated.  If so, an auto-refresh frame step is
     installed within that frame and this installation will be noted by
     any future call to `next_frame_has_changed()' that the application might
     make.  Auto-refresh frame steps cause the application to see additional
     subdivisions of a given animation frame when calling
     `advance_animation()', unless the next true animation frame follows
     too closely on the heels of the frame step.  When an auto-refresh
     frame-step is returned by `advance_animation()', `client_refresh_state'
     is set equal to 2, which means that the application should invoke
     `kdu_region_compositor::refresh' before rendering the frame.
        This mechanism does not cover all circumstances under which refresh
     may be required, because composited frames may re-use compositing layers
     from other frames that would not be regenerated automatically.  To this
     end, it is important that a regular refresh occur so long as data
     continues to arrive from the client.  The `earliest_client_refresh'
     member is used to manage the communication of regular refresh advice
     to the application as follows.  The variable is initially set to -1.0
     and returned to this value each time `get_suggested_refresh_action()'
     returns a positive value.   When `note_frame_generated' is called, if
     `earliest_client_refresh' is still -ve, it is set to the relevant
     display time plus `min_auto_refresh_interval'.  In a subsequent call to
     `notify_client_progress()', if `earliest_client_refresh' is found
     to be non-negative and `client_refresh_state' is 0, the value of
     `client_refresh_state' is set to -1; this value is subsequently
     converted to 1 when `advance_animation' next succeeds with a display
     time that is equal to or greater than `earliest_client_refresh'.
     The value of 1 means that the application is recommended to invoke
     `kdu_region_compositor::refresh' before rendering the frame, but
     the need to do so is less significant than the case above, in which
     `client_refresh_state' is equal to 2.  In either case, after
     `get_suggested_refresh_action()' returns a positive value, the
     `client_refresh_state' is returned to 0.
*/

#endif // kdu_region_animator_H
