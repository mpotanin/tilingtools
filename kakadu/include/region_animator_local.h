/*****************************************************************************/
// File: region_animator_local.h [scope = APPS/SUPPORT]
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
   Provides local definitions used exclusively within the implementation
of the `kdu_region_animator' object.
******************************************************************************/
#ifndef REGION_ANIMATOR_LOCAL_H
#define REGION_ANIMATOR_LOCAL_H

#include "kdu_region_animator.h"

// Defined here:
struct kdra_layer;
struct kdra_frame;
struct kdra_frame_req;
struct kdra_client_req;

/*****************************************************************************/
/*                               kdra_layer                                  */
/*****************************************************************************/

struct kdra_layer {
  public: // Member functions
    kdra_layer()
      { layer_idx=0; remapping_ids[0]=remapping_ids[1]=-1;
        active_discard_levels=0; scale_tolerance=0.0; next=NULL; }
    void configure(jpx_source *src, kdu_dims source_dims,
                   kdu_dims target_dims, double scale,
                   kdu_coords ref_comp_subsampling[]);
      /* On entry, the `layer_idx' member has been set and the compositing
         layer and its first codestream should both be accessible (at least
         at the file format level) from `src'. The purpose of this function
         is to determine values for `ref_subs', `active_discard_levels' and
         the derivative quantities `low_subs' and `high_subs'.  The
         `ref_comp_subsampling' array holds 33 entries -- it point to the
         member of the same name within `kdu_region_animator', which allows
         sub-sampling values discovered for other codestreams to be used
         if the main codestream header is not yet available for this
         compositing layer's primary codestream. */
    void find_min_max_jpip_woi_scales(double min_scale[], double max_scale[]);
      /* Plays exactly the same role as its namesake in the `kdrc_stream'
         object used to implement `kdu_region_compositor'.  Each of the two
         arguments is a 2-element array whose values correspond to a JPIP
         "fsiz" request which specifies "round-to-nearest" (first entry) and
         "round-up" (second entry), respectively.  Values are based upon the
         primary codestream used by the compositing layer and, in particular,
         its "reference image component" -- these properties have already
         been determined when this object was created.  The key features of
         interest to this function are the reference component's sub-sampling
         factors (relative to the high resolution codestream grid) and the
         effective sub-sampling factors for the reference component that
         arise at `active_discard_levels' (`active_subs'), with one more
         discarded resolution level (`low_subs') and with one less discarded
         resolution level (`high_subs').  The function tries to find
         lower and upper bounds for the amount of extra scaling (beyond
         the global scaling factor that was used to arrive at the value of
         `active_subs') that should be applied to the full composited image
         size and any region of interest, such that a JPIP server will
         arrive at the decision that the number of discarded resolution
         levels for this codestream should be equal to `active_discard_levels'.
         The main reason we need to hunt for these values is that the
         JPIP request syntax is based on the assumption that the full set
         of resolution levels for a codestream produces imagery with a
         natural resolution equal to that of the high resolution grid,
         whereas from a signal processing point of view, the full resolution
         imagery has a natural resolution which is smaller by a factor
         that is given by `ref_subs'.  Accordingly, if `ref_subs' is not
         equal to (1,1), the best rendering of the composited surface at
         some scale requires the JPIP server to be coerced into believing
         that the scale is larger by the factor `ref_subs'.  Additional
         complexities are introduced when Part-2 non-Mallat Wavelet
         transform styles are used. */
  public: // Data
    int layer_idx; // Compositing layer index
    int remapping_ids[2];
    int active_discard_levels; // Desired number of discard levels for scale
    double jpip_correction_scale_x; // See below
    double jpip_correction_scale_y; // See below
    kdu_coords active_subs; // Sub-sampling factors at `active_discard_levels'
    kdu_coords low_subs; // Sub-sampling at `active_discard_levels'+1
    kdu_coords high_subs; // Sub-sampling at `active_discard_levels'-1
    double scale_tolerance;
    kdra_layer *next;
  };
  /* A list of these objects is used to build JPIP window of interest
     requests -- see `kdra_frame::create_layer_descriptions'.
     The `jpip_correction_scale_...' values correspond to the amount of
     scaling that needs to be applied to the image produced after taking
     `active_discard_levels' into account, in order to achieve the `scale'
     value passed to `configure', multiplied by the reference image
     component's sub-sampling factors.  These factors need to be taken
     into account because the JPIP-defined procedure for determining
     the number of resolution levels to discard from a codestream is
     based on the assumption that full scale rendering (without any
     discarding of resolution levels) produces an image whose dimensions
     are those of the high resolution reference grid (i.e., the codestream
     canvas). */

/*****************************************************************************/
/*                               kdra_frame                                  */
/*****************************************************************************/

struct kdra_frame {
  public: // Member functions
    kdra_frame(kdu_region_animator *the_owner)
      { this->owner = the_owner; init(); }
    void init();
      /* Initializes everything to the "not-yet-known" state. */
    bool is_final_frame()
      { 
        if (owner->metadata_driven)
          return (next == NULL);
        else if (owner->video_range_end == owner->video_range_start)
          return true; // There is only one frame
        else if (owner->repeat_mode)
          return false; // Repeated animations have no end
        else if (owner->reverse_mode)
          return (frame_idx == owner->video_range_start);
        else
          return (frame_idx == owner->video_range_end);
      }
      /* Used in the insertion of conditional frame steps (see notes below)
         to ensure that such frame steps are not introduced to the very
         last frame in an animation.  The function returns true only if it
         can verify that there can be no further frames returned by
         `advance_animation', apart from conditional frame-steps. */
    void remove_conditional_frame_step()
      { 
        conditional_step_display_time=-1.0; min_conditional_step_gap=0.0; 
        conditional_step_is_auto_refresh=false;
      }
    void reset_pan_params()
      { 
        cur_roi_pan_pos = next_roi_pan_pos = roi_pan_duration = 0.0;
        roi_pan_acceleration = roi_pan_max_ds_dt = -1.0;
      }
    bool is_metadata_incomplete()
      { return incomplete_descendants || incomplete_link_target; }
    void set_metadata_incomplete(bool top_level, bool numlist_above)
      { // Used when creating a frame to hold incomplete metadata
        assert(!is_metadata_incomplete());
        if ((incomplete_meta_prev = owner->incomplete_meta_tail) == NULL)
          owner->incomplete_meta_head = owner->incomplete_meta_tail = this;
        else
          owner->incomplete_meta_tail =
            owner->incomplete_meta_tail->incomplete_meta_next = this;
        incomplete_link_target = incomplete_descendants = true;
        incomplete_top_level = top_level;
        incomplete_numlist_above = numlist_above;
      }
    void set_metadata_complete()
      { // Used to remove the incomplete metadata status from a frame
        if (!is_metadata_incomplete()) return;
        kdra_frame *nxt = this->incomplete_meta_next;
        kdra_frame *prv = this->incomplete_meta_prev;
        if (prv == NULL)
          { assert(this == owner->incomplete_meta_head);
            owner->incomplete_meta_head = nxt; }
        else
          prv->incomplete_meta_next = nxt;
        if (nxt == NULL)
          { assert(this == owner->incomplete_meta_tail);
            owner->incomplete_meta_tail = prv; }
        else
          nxt->incomplete_meta_prev = prv;
        incomplete_meta_prev = incomplete_meta_next = NULL;
        incomplete_link_target = incomplete_descendants = false;
        incomplete_top_level = incomplete_numlist_above = false;
        incomplete_prev = jpx_metanode();
      }
    bool in_auto_pan_preamble()
      { return (roi_pan_acceleration > 0.0) && (cur_roi_pan_pos < 1.0); }
    double calculate_duration(bool ignore_roi_pan_duration=false);
      /* Calculates the animation frame's original display duration using the
         prevailing timing parameters.  The returned value ignores any
         conditional frame steps, returning the actual duration of the
         original frame, if available, or else 0.0.
            Note that if this frame is engaged in an auto-pan preamble, and
         `ignore_roi_pan_duration' is false, the present function returns the
         duration of the portion of the preamble that is currently installed
         in the `roi_pan_duration' member, without considering earlier or
         later portions of the preamble that might exist, or the dwell period
         of the frame.  Otherwise, the returned duration will be reported as
         the dwell period for the frame, whether it is metadata-driven or
         otherwise.
      */
    double calculate_expected_total_duration(float scale);
      /* Similar to `calculate_duration', except that this function can
         be used ahead of the point at which the frame is presented
         to the application via `kdu_region_animator::advance_animation'.
         This function returns the expected total duration of the frame,
         taking into account the entire duration of any auto-pan preamble
         (this generally depends upon the rendering `scale'), as well as
         the dwell period of the frame.
            Note that this function is used only to generate JPIP imagery
         requests -- see `kdu_region_animator::generate_imagery_requests'. */
    int expand_incomplete_metadata();
      /* This function is invoked only if one of the `incomplete...'
         flags is true.  If `incomplete_link_target' is true, the
         function first looks for targets of any link that might be
         associated with `metanode' -- link targets that are descended from
         appropriate ROI or number list nodes can generate animation frames.
         If `incomplete_descendants' is true, the function expands
         descendants of `metanode' into new animation frames as
         necessary, recursively invoking itself on those frames.  Once all
         descendants and link targets of a node have been found, the function
         removes the current object (self-efacing), unless it has been
         converted into a complete frame.  The function returns the total
         number of frame objects (including incomplete frames) that it
         creates, plus 1 (to account for the current object), minus the
         number of frame objects that it removes (including the current object
         if it gets removed).  The return value is inevitably non-negative. */
    int extend_numlist_match_span();
      /* This function is provided specifically to address the fact that
         a number list might be capable of producing multiple metadata-driven
         animation frames.  The function does nothing unless invoked on an
         object marked with `last_match' and `may_extend_match' both true.
         The function returns 1 if a new last matching metanode is
         discovered -- the existing node is not removed or replaced, but
         may later be removed within `advance_animation'.  The function might
         set `may_extend_match' to false if it determines that there will be
         no more matches.  In composited frame mode, the function relies
         upon the `jpx_frm' and `frame_idx' members being valid -- if not
         these are discovered here (`find_source_info' finds these parameters).
         In single layer mode, the function relies upon `frame_idx' being
         the compositing layer index associated with a current match; if the
         match has not yet been discovered, the function tries to discover it
         before progressing.
            The function does not modify `owner->current', but the caller may
         do this based on the outcome of the function.
            The function returns 0 if it makes no changes.  If, however, the
         function is invoked on a node that has not yet had its `frame_idx'
         resolved, it is possible that the function will fail to resolve it
         at all and be forced to remove the current node, returning -1.  This
         should only happen when the function is called from within
         `expand_incomplete_metadata'. */
    void instantiate_next_numlist_match();
      /* This function does nothing unless we are somewhere inside a
         span of metadata-driven animation frames that were inferred by
         matching imagery (JPX frames or, in single layer mode, compositing
         layers) against a single `numlist' specification.  In forward
         play mode, the function returns immediately if the current frame
         has `last_match' true; otherwise, it tries to find and insert a
         new matching animation frame between the current one and the next --
         this might fail, meaning only that the next animation frame is
         already holding the next match within the span.  In reverse play
         mode, the function does the same thing, except that it does nothing
         if `first_match' is true, since in reverse play mode, the
         animation frames appear in reverse order within the list. */
    int calculate_metadata_id_in_span(kdra_frame *tgt);
      /* This function calculates what the `metadata_frame_id' member for
         the `tgt' frame should be, based on the current frame's
         `metadata_frame_id', where the current frame must belong to the
         same span of related metadata-driven animation frames as `tgt'.
         The function is used by functions such as `extend_numlist_match_span'
         which add new metadata-driven frames to the related span of matches
         on demand.  However it is also used when metadata frame ID's need
         to be relabeled to make sure that the relationships within the span
         are maintained. */
    void reconcile_match_track_info();
      /* This function is invoked on a frame that lies within a span of
         metadata driven frames that are all derived by numlist matching on
         a common `numlist' member, with the first in the span marked with
         `first_match'=true and the last in the span marked with
         `last_match'=true.  The function is called when a numlist search
          may have produced a change in the `match_track_idx' or the
          `match_track_or_above' member (`match_track_idx' may have increased
          or `match_track_or_above' may have become false).  The function
          propagates these changes to all other frames in the span (there
          should typically be only a few), so that future searches for
          other members of the span will not yield frames that belong to
          an incompatible JPX presentation track. */
    int find_source_info();
      /* This function is called if `have_all_source_info' is
         false.  If `frame_idx' < 0 (the object must be in the
         metadata-driven mode), the function attempts to find a frame that
         is compatible with `numlist'; if one is found, the function
         proceeds to try to complete the other source properties.
            If the frame index turns out to place it beyond the end of the
         source, or a frame that matches `numlist' does not exist, the
         function returns -1 and also removes the current object
         (self-efacing), so you must not access it after a -ve return.
            In addition to `frame_idx', the function fills out
         `source_duration' (if it is -ve, meaning not already known),
         `frame_size' and `frame_roi'.  It can happen that some
         of this information cannot yet be determined, due to insufficient
         information in a dynamic cache.  If this happens, the function
         leaves the `have_all_source_info' flag false.  Indeed, it may
         be that even the metadata required to instantiate the frame in the
         first place is not yet available  (i.e., `is_metadata_incomplete'
         returns true), in which case the function also returns with
         `have_all_source_info' false.
            Finally, it may be that all required metadata is available but
         some of the main codestream headers are missing, in which case the
         function also returns with `have_all_source_info' false.
         In all these cases, the function returns 0, which indicates to
         the caller that the current object has not been removed, but some
         source frame information is still missing.
            If all the required information can be recovered, the function
         sets `have_all_source_info' to true before returning 1. */
    bool request_unknown_layers(kdu_window *client_window);
      /* This function may eventually be invoked if the above function is
         unable to access one or more compositing layers that are associated
         with the current frame -- applies only when the frame is derived from
         a JPX source.  For each compositing layer that cannot be opened, the
         function adds a jpx-layer codestream context to the `client_window'.
         Returns true if one or more compositing layers was found to require
         such treatment.  Typically, the caller adds a `KDU_MRQ_STREAM'
         metadata request for JPX compositing layer header boxes and
         JPX codestream header boxes, using `client_window->add_metareq',
         if this function returns true.  However, only one such metareq
         (for each box type) need be added, while this function may be
         called many times to prepare requests for the metadata relevant
         to multiple future frames with unknown layer metadata. */
    void construct_window_request(const kdu_dims &viewport,
                                  const kdu_dims &user_roi, double scale,
                                  kdu_window &window);
      /* On entry, `window' is empty.  This function sets it to reflect
         an appropriate window-of-interest request for this frame.  The
         way in which the request is written depends upon whether the
         frame represents a composited JPX frame or just a compositing layer.
         Moreover, if the frame is metadata-driven and has a metadata-defined
         region of interest (stored in `frame_roi'), the installed window
         of interest is based on `frame_roi' intersected with a translated
         version of `viewport' that is centred over `frame_roi'.  Otherwise,
         the installed window of interest is based on the `user_roi'.
         If `user_roi' is empty then `viewport' is also an empty region and
         the installed window of interest involves no image samples; only
         headers. */
  private: // Helper functions
    kdra_layer *
      create_layer_descriptions(const kdu_dims &region, double scale);
      /* This function returns a linked list of compositing layers that
         are required to render the frame within the given `region' of
         interest.  If `region' is empty, all compositing layers are
         returned.  The `scale' parameter is used to configure the
         `active_discard_levels' member of each `kdra_layer' object in
         the returned list, along with the associated sub-sampling factors
         `active_subs', `low_subs' and `high_subs'.  The policy used
         to determine the reference image component and `active_discard_levels'
         value for the primary codestream of each compositing layer is
         intended to mtch that used by `kdu_region_compositor'. */
  public: // Links
    kdu_region_animator *owner;
  public: // Data members used to discover metadata-driven frames
    jpx_metanode metanode; // Non-empty if and only if metadata-driven
    bool incomplete_link_target; // See below
    bool incomplete_descendants; // See below
    bool incomplete_top_level;   // See below
    bool incomplete_numlist_above; // See below
    jpx_metanode incomplete_prev; // See below
    kdra_frame *incomplete_meta_next; // For list of frames
    kdra_frame *incomplete_meta_prev; // with incomplete metadata
  public: // Data members determined for metadata-driven frames; these
          // are evaluated when the metadata is first discovered, but
          // some aspects might be updated if more information is found.
    kdu_dims metanode_roi; // Empty unless metanode has region of interest
    jpx_metanode numlist; // Keeps track of imagery associations
    int single_numlist_layer; // See below
    int metadata_frame_id;
    bool first_match; // These start out both being true for metadata nodes;
    bool last_match;  // once complete, or at a later time, the metanode may
       // be split into a first matching frame and a last matching frame.
       // Intermediate matching frames may be introduced, but are temporary.
    bool may_extend_match; // True if `last_match' is true, but there might
       // potentially be more matches -- call `extend_numlist_match_span'.
    bool match_track_or_above; // If `match_track' is only lower bound
    kdu_uint32 match_track_idx; // Ensures we find matching frames in a
                                // consistent JPX presentation track.
  public: // Data members that may be found by `find_source_frame_info'
    int frame_idx; // -ve if not yet resolved; c.layer in single layer mode
    jpx_frame jpx_frm; // For JPX files only
    double source_duration; // Original frame duration (source-based)
    kdu_coords frame_size; // Dimensions of the frame at full size
    kdu_dims frame_roi; // Region of interest, if any, on the full frame
    bool have_all_source_info; // If all of the above are available
  public: // Data members related to client requests
    kdra_frame_req *frame_reqs;
  public: // Other state variables
    bool generated; // Set by `note_frame_generated'
    double cur_roi_pan_pos; // See below
    double next_roi_pan_pos; // See below
    double roi_pan_acceleration; // See below
    double roi_pan_max_ds_dt; // See below
    double roi_pan_duration; // Time taken to get from cur to next pan pos
    double display_time; // Becomes >= 0 when frame becomes `current'
    double activation_time; // System time when frame last became `current'
    double cur_display_time; // See below
    double conditional_step_display_time; // See below
    double min_conditional_step_gap; // See below
    bool conditional_step_is_auto_refresh; // See below
  public: // Links
    kdra_frame *next;
    kdra_frame *prev;
  };
  /* Notes:
        The `incomplete_xxx' members are used only if this object is being
     used to keep track of a metanode for which insufficient information
     was available to recover actual animation frames during the relevant
     call to `kdu_region_animator::add_metanode'.  If `metanode' was a link
     node, but the target of the link could not be resolved yet, the
     `incomplete_link_target' flag is set.  Otherwise, if `metanode's
     may potentially yield animation frames but could not all be recovered
     yet, the `incomplete_descendant' flag is set.  In this second case,
     `incomplete_prev' identifies the last descendant of `metanode' that
     has actually been resolved -- may be empty if none have yet been
     resolved; this is passed as the `ref' argument to
     `metanode.get_next_descendant'.  The `incomplete_top_level' flag is
     set if this frame was added directly by
     `kdu_region_animator::add_metanode'.  The `incomplete_numlist_above'
     flag indicates whether a number list node has already been encountered
     during the recursive search for metadata with which to create frames --
     this recursive search is implemented by the
     `expand_incomplete_frame_metadata' function.
        Once a metadata node has been resolved, so that a metadata-driven
     animation frame is available, the possibility may exist for the metadata
     node to be expanded into multiple animation frames, each revealing the
     metadata's compositing layers or a region of interest in a different way.
     We refer to the complete collection of metadata-driven animation frames
     that are derived from a single `metanode' as the node's "span".  The
     first and last members of the span are tagged with the `first_match' and
     `last_match' flags.  In many (perhaps most) cases, a single frame
     represents the entire span, so `first_match' and `last_match' are both
     true.  However, if the span can be split, this is done by the
     `extend_numlist_match_span'.  Once the span has been split into separate
     first and last frames, intermediate frames may be created, but those are
     temporary -- they will be removed once the animation runs sufficiently
     past them.  The first and last frames in a span, however, are not
     removed, except that in some cases the discovery of new content in a
     dynamic cache may allow the `extend_numlist_match_span' to advance the
     last frame in the span, rendering the previous last frame an intermediate
     temporary frame.
        In frame composition mode, metadata-driven animation frames are
     discovered using the powerful `jpx_composition::find_numlist_match'
     function.  The `match_track_idx' and `match_track_or_above' members are
     used to implement the consistent frame discovery algorithm described
     in connection with that function.  In particular, at each point in the
     search for new matching frames, `match_track_idx' holds the minimum
     JPX presentation track index that is consistent with the frames
     that have been discovered so far to match the `numlist' specifications,
     while `match_track_or_above' is true if all of these frames are
     compatible with higher presentation track indices; otherwise, only
     those frames that have `match_track_idx' as their presentation track
     index, or that belong to the last presentation track in their context
     (top-level or JPX container) can be considered compatible.  These
     constraints are supplied to `jpx_composition::find_numlist_match'.
     It is worth noting that not all matched frames necessarily become
     `kdra_frame' animation frames.  In particular, we do not allow two
     consecutive `kdra_frame's to match exactly the same compositing layers
     from the `numlist' specification.
        In single layer mode, the number of metadata-driven frames that belong
     to a span is discovered using the `jpx_metanode::count_numlist_layers'
     function and each specific metadata-driven frame is characterised by its
     `single_numlist_layer' member.  That member is not a compositing layer
     index, but rather an enumerator that lies in the range 0 through C-1
     where C is the count returned by `numlist.count_numlist_layers'.  This
     enumerator is passed to `jpx_metanode::get_numlist_layer' with a
     `rep_idx' of -1.
        If `conditional_step_display_time' has a positive value, an extra
     "frame-step" has been inserted within the nominal span of the frame.
     This may be done by `insert_conditional_frame_step' or by
     `notify_client_progress' and then only to the animation's current
     presentation frame.  At the same time, each of these functions sets the
     `min_conditional_step_gap' member to reflect the condition under which
     the frame step will be introduced when `advance_animation' is next
     called -- a frame step is introduced only if its display time is
     strictly less than `min_conditional_step_gap' seconds less than the
     start of the next animation frame, unless of course there is no next
     animation frame that is available.  If and when `advance_animation'
     advances to a conditional frame step, the `cur_display_time' member is
     set equal to the `conditional_step_display_time' and the conditional
     frame step is cancelled -- see `remove_conditional_frame_step'.
     Conditional frame steps are also removed if the animation playback
     direction is reversed.
        The `conditional_step_is_auto_refresh' member identifies whether an
     existent conditional frame step has been inserted by
     `notify_client_progress', as an auto-refresh step, or by
     `insert_conditional_frame_step', as an application-defined frame step.
     It is important to distinguish between these two cases, because
     application-defined frame steps always override auto-refresh frame
     steps.
        `cur_roi_pan_pos', `next_roi_pan_pos', `roi_pan_acceleration' and
     `roi_pan_max_ds_dt' members are relevant only if this is a
     metadata-driven animation frame with `num_clayers' > 0 and the
     previous animation frame has the same `frame_idx'.  In that case,
     `advance_animation' synthesizes multiple virtual frames in order to
     introduce an auto-pan preamble, whereby the view and ROI pans from
     those of the previous frame to those of the current frame.  Frames
     that are undergoing or have undergone an auto-pan preamble have their
     `roi_pan_acceleration' members set to a positive value.  If the
     auto-pan preamble is complete, `roi_pan_acceleration' remains
     non-negative but `cur_roi_pan_pos' equals 1.0, meaning that the pan
     has arrived at its final destination.  While a frame is undergoing
     an auto-pan preamble, its `display_time' and `cur_display_time'
     members are identical; both are incremented as the `cur_roi_pan_pos'
     member increases, to represent the display time associated with the
     start of the current segment within the pan preamble.  Once the
     frame has finished any auto-pan preamble, the `display_time' member
     is frozen, but `cur_display_time' may increase if conditional
     frame steps are executed.  Auto-panning is configured (and updated) by
     calls to `get_roi_and_mod_viewport'.
   */

/*****************************************************************************/
/*                             kdra_frame_req                                */
/*****************************************************************************/

struct kdra_frame_req {
  public: // Member functions
    kdra_frame_req() { init(); }
    ~kdra_frame_req()
      { 
        assert(frm == NULL);
        assert((client_reqs[0] == NULL) && (client_reqs[1] == NULL));
      }
    void init()
      { 
        frm = NULL;
        expected_display_start = expected_display_end = -1.0;
        requested_fraction = 0.0;
        client_reqs[0] = client_reqs[1] = NULL;
        next_in_frame = next = prev = NULL;
      }
    void detach(bool from_tail);
      /* Removes any references to this object from `kdra_client_req' objects
         and from `kdra_frame' objects, resetting the relevant member
         variables.  If `from_tail' is true, the frame request that is being
         detached is the last one that references its `frm' member, rather
         than the first -- this happens only when invoking the
         `kdu_region_animator::trim_imagery_requests' function. */
    void note_newly_initialized_display_time();
      /* Called when `frm->display_time' is set to a positive value for the
         first time.  This function takes the opportunity to update the
         `expected_display_start' and `expected_display_end' members of
         the current element and all following elements in the list, setting
         the current record's expected_display_start' member equal to the
         new `display_time' value and the adjusting all other values so that
         their display times are contiguous and their display durations are
         unaltered. */
  public: // Data
    kdra_frame *frm; // Points to the frame to which the request refers
    double expected_display_start;
    double expected_display_end;
    double requested_fraction; // > 0 and < 1 if frame needs a second request
    kdra_client_req *client_reqs[2]; // See below
    kdra_frame_req *next_in_frame; // Next request to same `frm', if any
    kdra_frame_req *next; // Next request in temporal order
    kdra_frame_req *prev; // Previous request in temporal order
  };
  /* Notes:
       This object is used to keep track of frames that have been (or are
     about to be) requested via a `kdu_client' object.  The reason for
     having a separate `kdra_frame_req' object, as opposed to just augmenting
     the `kdra_frame' object is that a given frame may be the subject of
     multiple non-consecutive client requests -- this can happen if the
     animation is to be looped (repeat mode).  Conceptually, the
     `kdra_frame_req' objects are used to build a list of consecutively
     requested frames that may occupy a limited segment of the `kdra_frame'
     list, but may also wrap around the complete sequence of `kdra_frame'
     list one or more times, like a spiral staircase.  The oldest
     element on the `kdra_frame_req' list that references any given frame
     is identified via the `kdra_frame::frame_reqs' member.  This oldest
     element may reference more recent entries on the `kdra_frame_req' list
     that reference the same frame, via its `next_in_frame' member.
        The head of the `kdra_frame_req' list always points to the current
     animation frame, or the head of the `kdra_frame' list if there is no
     current animation frame.  As the animation advances, the head of
     the `kdra_frame_req' list is removed, which adjusts the corresponding
     `kdra_frame::frame_reqs' member -- if `kdra_frame::frame_reqs' becomes
     NULL, it is possible for the frame itself to be removed by
     `kdu_region_animator::remove_frame'.
        `client_reqs' contains up to two pointers to `kdra_client_req'
     objects that represent pending client requests.  There can be up to
     two client requests for any given frame, which allows for the fact that
     the first request was aggregated with earlier frames, while the latter
     request may refer to just this frame -- this only happens where a
     sequence of closely spaced frames are followed by one with long duration.
  */

/*****************************************************************************/
/*                             kdra_client_req                               */
/*****************************************************************************/

struct kdra_client_req {
  public: // Member functions
    kdra_client_req()  { init(); }
    ~kdra_client_req()
      { 
        assert(first_frame == NULL); // Must have detached frames already
      }
    void init()
      { 
        window.init();
        user_layers=-1; user_roi = user_viewport = kdu_dims(); custom_id = 0;
        cumulative_display_usecs=cumulative_request_usecs=service_usecs = 0;
        first_frame = last_frame = NULL;  next=prev=NULL;
      }
    void attach_frame_req(kdra_frame_req *frq)
      { 
        if (last_frame == NULL)
          first_frame = frq;
        last_frame = frq;
        if (frq->client_reqs[0] == NULL)
          frq->client_reqs[0] = this;
        else
          { 
            assert(frq->client_reqs[1] == NULL);
            frq->client_reqs[1] = this;
          }
      }
    void detach_frame_reqs()
      { // Zero out all references to this object from frame reqs
        for (kdra_frame_req *frq=first_frame; frq != NULL; frq=frq->next)
          { 
            if (frq->client_reqs[0] == this)
              frq->client_reqs[0] = NULL;
            else if (frq->client_reqs[1] == this)
              frq->client_reqs[1] = NULL;
            else
              assert(0);
            if (frq == last_frame)
              break;
          }
        first_frame = last_frame = NULL;
      }
  public: // Data
    kdu_window window;
    int user_layers; // max_quality_layers arg to `generate_imagery_requests'
    kdu_dims user_roi; // `roi' argument to `generate_imagery_requests'
    kdu_dims user_viewport; // `viewport' arg to `generate_imagery_requests'
    kdu_long custom_id; // Non-zero if request has been posted
    kdu_long cumulative_display_usecs; // See below
    kdu_long cumulative_request_usecs; // See below
    kdu_long requested_usecs; // Value passed to `kdu_client::post_window'
    kdu_long service_usecs; // 0 means no response data yet
    kdra_frame_req *first_frame; // First frame covered by this request
    kdra_frame_req *last_frame; // Last frame included by this request
    kdra_client_req *next;
    kdra_client_req *prev;
  };
  /* Notes:
        The animator object does not and should not try to implement any
     kind of control loop to regulate the timing behaviour associated with
     its interaction with `kdu_client'.  This is because the `kdu_client'
     object already implements its own disciplined machinery to try to
     satisfy the expectations of timed requests passed to its `post_window'
     function with non-zero `preferred_service_usecs' values.  The `kdu_client'
     object attempts to guarantee that consecutive timed requests will
     receive a cumulative service time that matches their cumulative
     requested service times, measured using the same timebase as
     `kdu_client::sync_timing' is invoked regularly.  That function is
     very important.  In the implementation here, `kdu_client::sync_timing'
     is passed a version of the next display event time (coming from the
     frame refresh machinery), that has been converted to microseconds and
     expressed relative to a common reference time -- we refer to this as
     the "reference display clock".  The `kdu_client::sync_timing' function
     keeps track of the reference display clock values and compares them with
     its own internal clock, transparently making minor adjustments to the
     preferred service times that are passed to `kdu_client::post_window'
     so that it can use its own internal clock to manage the timed requests
     while still meeting the expectations of the application.  The
     `kdu_client::sync_timing' function also returns the amount of time
     that it expects to lapse between the point at which it is called
     and the point at which the next request to be posted (assuming the
     caller will post one) first starts to get served.  This allows us to
     figure out how far ahead we need to post our requests and also
     whether we need to ask for smaller amounts of service time than would
     nominally be associated with the frame interval, so that the
     client-server communication can catch up with the display process.
     With these things in mind, we now describe the interpretation of the
     timing member variables here.
        The `cumulative_display_usecs' member essentially holds the
     reference display clock value corresponding to the point at which
     we expect the last frame (or fraction of a frame) associated with this
     request to finish its display.  This value is based upon the relevant
     `kdra_frame_req::expected_display_end' value (corrected for fractional
     frame requests), converted to a reference display clock value.  The
     value is evaluated at the point when the request is posted and is
     not updated in any way after that point, even though the expected
     display start/end times for frames may change as new information
     comes to light.
        The `cumulative_request_usecs' member holds the sum of all
     `requested_usecs' values from this request and all previous requests,
     since the request list was last empty. The commitment of the `kdu_client'
     object is to endeavour to achieve a cumulative service time for these
     requests which matches the `cumulative_request_usecs' value, although
     there will be delay.
        Note that the reference display clock's base offset is chosen such
     that `cumulative_display_usecs' would be 0 if the end time for a
     request's frames corresponded to the next frame display event time
     passed to `generate_imagery_requests' at the point when the request
     list was last found to be empty.
        Generally speaking, we want `cumulative_request_usecs' and
     `cumulative_request_usecs' to advance together, so we normally select
     the `requested_usecs' value that is passed to `kdu_client::post_window'
     simply by taking the difference between the current and previous
     request's `cumulative_display_usecs' values.  However, if the
     `kdu_region_animator::generate_imagery_requests' function determines
     that the client-server communication is running behind (based on
     information returned by `kdu_client::sync_timing'), so that its requests
     might not be served in time, the `requested_usecs' value is reduced by
     up to 50%.  Note that there is no ongoing closed feedback loop here --
     just a mechanism for initially asking for less data so that the service
     process can catch up with the needs of the independent display process.
        The `service_usecs' value keeps track of the amount of service time
     that a request has actually experienced.  The value is not used in a
     very comprehensive manner, but only to determine whether new service
     data is likely to have arrived from the request's frames -- this, in
     turn, may trigger the need for frame refresh events where a frame has
     a very long nominal display time.
        The `first_frame' and `last_frame' members may become NULL if all
     frames that were associated with a request are removed or associated
     with new requests. */

#endif // REGION_ANIMATOR_LOCAL_H
