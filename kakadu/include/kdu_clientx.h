/*****************************************************************************/
// File: kdu_clientx.h [scope = APPS/COMPRESSED_IO]
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
  Defines a JPX-specific version of the `kdu_client_translator' interface,
for translating JPX codestream context requests into individual codestream
window requests for the purpose of determining what image elements already
exist in the client's cache.
******************************************************************************/

#ifndef KDU_CLIENTX_H
#define KDU_CLIENTX_H

#include "kdu_client.h"
#include "jp2.h"

// Defined here:
class kdu_clientx;

// Defined elsewhere:
struct kdcx_entity_container;
class kdcx_context_mappings;


/*****************************************************************************/
/*                                kdu_clientx                                */
/*****************************************************************************/

class kdu_clientx : public kdu_client_translator {
  /* [BIND: reference]
     [SYNOPSIS]
       Provides an implementation of the `kdu_client_translator' interface
       for the specific case of JP2/JPX files.  It is not strictly necessary
       to provide any translator for JP2 files, but since JP2 is a subset
       of JPX, there is no problem providing the implementation here.  All
       the translation work is done inside the `update' member.  If it finds
       the data source not to be JPX-compatible, all translation calls
       will fail, producing null/false results, as appropriate.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdu_clientx();
    KDU_AUX_EXPORT virtual ~kdu_clientx();
      /* [SYNOPSIS]
           Do not destroy the object while it is still in use by a
           `kdu_client' object.  If the translator has already been
           passed to `kdu_client::install_context_translator', you must
           either destroy the `kdu_client' object, or pass a NULL
           argument to `kdu_client::install_context_translator' before
           destroying the translator itself.
      */
    KDU_AUX_EXPORT virtual void close();
      /* [SYNOPSIS]
           Overrides `kdu_client_translator::close' to clean up internal
           resources.
      */
    KDU_AUX_EXPORT virtual bool update();
      /* [SYNOPSIS]
           Overrides `kdu_client_translator::update' to actually update
           the internal representation, based on any new information which
           might have arrived in the cache.
      */
    KDU_AUX_EXPORT virtual kdu_window_context
      access_context(int context_type, int context_idx, int remapping_ids[]);
      /* [SYNOPSIS]
           Provides the implementation of
           `kdu_client_translator::access_context'.
      */
  private: // Data members used to store file-wide status
    jp2_family_src src;
    jp2_input_box top_box; // Current incomplete top-level box
    jp2_input_box jp2h_sub; // First incomplete sub-box of jp2 header box
    jp2_input_box comp_box; // Top-level composition box
    jp2_input_box comp_sub; // Sub-box of composition box
    bool started; // False if `update' has not yet been called
    bool not_compatible; // True if file format is found to be incompatible
    bool is_jp2; // If file is plain JP2, rather than JPX
  private: // Data members used to keep track of top-level entities
    bool have_multi_codestream_box;
    bool top_level_complete; // If no need to parse any further top level boxes
    kdcx_context_mappings *top_context_mappings;
    int num_top_jp2c_or_frag; // Num JP2C or fragment boxes found so far
    int num_top_jpch; // Num top level JPCH boxes found so far
    int num_top_jplh; // Number of top level JPLH boxes found so far
  private: // Data members used to keep track of JPX containers
    kdcx_entity_container *containers;
    kdcx_entity_container *last_container;
    kdcx_entity_container *last_initialized_container;
    kdcx_entity_container *first_unfinished_container;
  };

#endif // KDU_CLIENTX_H
