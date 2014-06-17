/*****************************************************************************/
// File: kdu_client.h [scope = APPS/COMPRESSED_IO]
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
  Describes a complete caching compressed data source for use in the client
of an interactive client-server application.
******************************************************************************/

#ifndef KDU_CLIENT_H
#define KDU_CLIENT_H

#include "kdu_cache.h"
#include "kdu_client_window.h"

// Defined here:
class kdu_client_translator;
class kdu_client_notifier;
class kdu_client;

// Defined elsewhere
class kdcs_timer;
class kdcs_channel_monitor;
class kdcs_message_block;
struct kdc_request;
struct kdc_request_dependency;
struct kdc_chunk_gap;
class kdc_model_manager;
class kdc_primary;
class kdc_cid;
struct kdc_request_queue;

// Status flags returned by `kdu_client::get_window_in_progress'
#define KDU_CLIENT_WINDOW_IS_MOST_RECENT      ((int) 0x01)
#define KDU_CLIENT_WINDOW_RESPONSE_STARTED    ((int) 0x02)
#define KDU_CLIENT_WINDOW_RESPONSE_TERMINATED ((int) 0x04)
#define KDU_CLIENT_WINDOW_IS_COMPLETE         ((int) 0x08)
#define KDU_CLIENT_WINDOW_UNREQUESTED         ((int) 0x10)
#define KDU_CLIENT_WINDOW_UNREPLIED           ((int) 0x20)

/*****************************************************************************/
/*                              kdu_client_mode                              */
/*****************************************************************************/

enum kdu_client_mode {
  KDU_CLIENT_MODE_AUTO=1,
  KDU_CLIENT_MODE_INTERACTIVE=2,
  KDU_CLIENT_MODE_NON_INTERACTIVE=3
};

/*****************************************************************************/
/*                           kdu_client_translator                           */
/*****************************************************************************/

class kdu_client_translator {
  /* [BIND: reference]
     [SYNOPSIS]
       Base class for objects which can be supplied to
       `kdu_client::install_context_translator' for use in translating
       codestream contexts into their constituent codestreams, and
       translating the view window into one which is appropriate for each
       of the codestreams derived from a codestream context.  The interface
       functions provided by this object are designed to match the
       translating functions provided by the `kdu_serve_target' object, from
       which file-format-specific server components are derived.
       [//]
       This class was first defined in Kakadu v4.2 to address the need for
       translating JPX compositing layers and compositing instructions so
       that cache model statements could be delivered correctly to a server
       (for stateless services and for efficient re-use of data cached from
       a previous session).  The translator for JPX files is embodied by the
       `kdu_clientx' object, which is, in some sense, the client version of
       `kdu_servex'.  In the future, however, translators for other file
       formats such as MJ2 and JPM could be implemented on top of this
       interface and then used as-is, without modifying the `kdu_client'
       implementation itself.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdu_client_translator();
    virtual ~kdu_client_translator() { return; }
    virtual void init(kdu_cache *main_cache)
      { close(); aux_cache.attach_to(main_cache); }
      /* [SYNOPSIS]
           This function first calls `close' and then attaches the
           internal auxiliary `kdu_cache' object to the `main_cache' so
           that meta data-bins can be asynchronously read by the
           implementation.  You may need to override this function to
           perform additional initialization steps.
      */
    virtual void close() { aux_cache.close(); }
      /* [SYNOPSIS]
           Destroys all internal resources, detaching from the main
           cache installed by `init'.  This function will be called from
           `kdu_client' when the client-server connection becomes
           disconnected.
      */
    virtual bool update() { return false; }
      /* [SYNOPSIS]
           This function is called from inside `kdu_client' when the
           member functions below are about to be used to translate
           codestream contexts.  It parses as much of the file format
           as possible, returning true if any new information was
           parsed.  If sufficient information has already been parsed
           to answer all relevant requests, the function returns false
           immediately, without doing any work.
      */
    virtual kdu_window_context
      access_context(int context_type, int context_idx, int remapping_ids[])
      { return kdu_window_context(); }
      /* [SYNOPSIS]
           Designed to mirror `kdu_serve_target::access_context'.
           [//]
           This function is used to translate codestream context requests into
           codestreams and view windows, expressed relative to the individual
           codestreams that belong to the context.
           [//]
           A codestream context might be a JPX compositing layer or an
           MJ2 video track, and others may be added in the future.  The
           context is generally a higher level imagery object which may be
           represented by one or more codestreams.  The context is identified
           by the `context_type' and `context_idx' members, possibly
           modified by the `remapping_ids' array that is explained below.
         [RETURNS]
           An abstract interface whose member functions allow you to
           inspect the codestreams and codestream components that belong
           to the context, as well as remapping regions from a JPIP
           request to the individual codestreams that comprise the context.
           [//]
           If the function is unable to translate this context, it returns
           an empty interface.  This situation may change in the future if
           more data becomes available in the cache, allowing the context
           to be translated.  On the other hand, the context might simply
           not exist.  The return value does not distinguish between these
           two types of failure.
         [ARG: context_type]
           Identifies the type of context.  Currently defined context type
           identifiers (in "kdu_client_window.h") are:
           [>>] `KDU_JPIP_CONTEXT_JPXL' (for JPX compositing layer contexts);
           [>>] `KDU_JPIP_CONTEXT_MJ2T' (for MJ2 video tracks).
           [//]
           For more information on these, see the definition of
           `kdu_sampled_range::context_type'.
         [ARG: context_idx]
           Identifies the specific index of the JPX compositing layer,
           MJ2 video track or other context.  Note that JPX compositing
           layers are numbered from 0, while MJ2 video track numbers start
           from 1.
         [ARG: remapping_ids]
           Array containing two integers, whose values may alter the
           context membership and/or the way in which view windows are to
           be mapped onto each member codestream.  For more information on
           the interpretation of remapping id values, see the comments
           appearing with `kdu_sampled_range::remapping_ids' and
           `kdu_sampled_range::context_type'.
           [//]
           If the supplied `remapping_ids' values cannot be processed as-is,
           they may be modified by this function, but all of the
           `kds_channel_mappings' interface functions which accept a
           `remapping_ids' argument must be prepared to operate
           correctly with the modified remapping values.
      */
    protected: // Data members, shared by all file-format implementations
      kdu_cache aux_cache; // Attached to the main client cache; allows
                           // asynchronous reading of meta data-bins.
  };

/*****************************************************************************/
/*                            kdu_client_notifier                            */
/*****************************************************************************/

class kdu_client_notifier {
  /* [BIND: reference]
     [SYNOPSIS]
       Base class, derived by an application and delivered to
       `kdu_client' for the purpose of receiving notification messages when
       information is transferred from the server.
  */
  public:
    kdu_client_notifier() { return; }
    virtual ~kdu_client_notifier() { return; }
    virtual void notify() { return; }
    /* [BIND: callback]
       [SYNOPSIS]
         Called whenever an event occurs, such as a change in the status
         message (see `kdu_client::get_status') or a change in the contents
         of the cache associated with the `kdu_client' object.
    */
  };

/*****************************************************************************/
/*                                kdu_client                                 */
/*****************************************************************************/

class kdu_client : public kdu_cache {
  /* [BIND: reference]
     [SYNOPSIS]
     Implements a full JPIP network client, building on the services offered
     by `kdu_cache'.
     [//]
     BEFORE USING THIS OBJECT IN YOUR APPLICATION, READ THE FOLLOWING ADVICE:
     [>>] Because errors may be generated through `kdu_error' (and perhaps
          warnings through `kdu_warning') both in API calls from your
          application and inside the network management thread created by
          this object, you must at least use a thread-safe message handler
          (e.g., derived from `kdu_thread_safe_message') when you configure
          the error and warning handlers (see `kdu_customize_errors' and
          `kdu_customize_warnings').   Your message handler is expected to
          throw an exception of type `kdu_exception' inside
          `kdu_message::flush' if the `end_of_message' argument is true --
          much better than letting the entire process exit, which is the
          default behaviour if you don't throw an exception.  Message
          handlers may be implemented in a non-native language such as
          Java or C#, in which case exceptions which they throw in their
          override of `kdu_message::flush' will ultimately be converted
          into `kdu_exception'-valued C++ exceptions.
     [>>] Perhaps less obvious is the fact that your error/warning message
          handler must not wait for activity on the main application thread.
          Otherwise, your application can become deadlocked inside one of
          the API calls offered by this function if the network management
          thread is simultaneously generating an error.  Blocking on the
          main application thread can happen in subtle ways in GUI
          applications, since many GUI environments rely upon the main
          thread to dispatch all windowing messages; indeed some are not
          even thread safe.
     [>>] To avoid any potential problems with multi-threaded errors and
          warnings in interactive environments, you are strongly advised to
          implement your error/warning message handlers on top of
          `kdu_message_queue', which itself derives from
          `kdu_thread_safe_message'.  This object builds a queue of messages,
          each of which is bracketed by the usual `kdu_message::start_message'
          and `kdu_message::flush(true)' calls, which are automatically
          inserted by the `kdu_error' and `kdu_warning' services.  Your
          derived object can override `kdu_message::flush' to send a
          message to the window management thread requesting it to iteratively
          invoke `kdu_message_queue::pop_message' and display the relevant
          message text in appropriate interactive windows.  Examples of this
          for Windows and MAC-OSX platforms are provided in the "kdu_winshow"
          and "kdu_macshow" demo applications.  If you don't do something like
          this, blocking calls to `kdu_client::disconnect', for example, run
          a significant risk of deadlocking an interactive application if
          the underlying platform does not support true multi-threaded
          windowing.
     [//]
     This single network client can be used for everything from a single
     shot request (populate the cache based on a single window of interest)
     to multiple persistent request queues.  The notion of a request queue
     deserves some explanation here so that you can make sense of the
     interface documentation:
     [>>] JPIP offers both stateless servicing of requests and stateful
          request sessions.  You can choose which flavour you prefer, but
          stateful serving of requests (in which the server keeps track of
          the client's cache state) is the most efficient for ongoing
          interactive communications.
     [>>] A stateful JPIP session is characterized by one or more JPIP
          channels, each of which has a unique identifier; any of these
          channel identifiers effectively identifies the session.  You do
          not need to worry about the mechanics of JPIP channels or channel
          identifiers when using the `kdu_client' API, but it is worth
          knowing that JPIP channels are associated with stateful sessions and
          there can be multiple JPIP channels associated with the same session.
     [>>] Each JPIP channel effectively manages its own request queue.  Within
          the channel, new requests may generally pre-empt earlier requests,
          so that the earlier request's response may be truncated or even
          empty.  This is generally a good idea, since interactive users
          may change their window of interest frequently and desire a high
          level of responsiveness to their new interests.  Pre-emptive
          behaviour like this can only be managed by the JPIP server itself
          in the context of a stateful session.  For stateless communications,
          maintaining responsiveness is the client's responsibility.
     [>>] The present object provides the application with an interface
          which offers any number of real or virtual JPIP channels,
          regardless of whether the server supports stateful sessions and
          regardless of the number of JPIP channels the server is capable
          of supporting.  The abstraction used to deliver this behaviour
          is that of a "request queue".
     [>>] Each request queue is associated with some underlying communication
          channel.  Ideally, each request queue corresponds to a separate
          JPIP channel, but servers may not offer sessions or multiple JPIP
          channels.  If the server offers only one stateful session channel,
          for example, the client interleaves requests from the different
          request queues onto that channel -- of course, this will cause
          some degree of pre-emption and context swapping by the server,
          with efficiency implications, but it is better than creating
          separate clients with distinct caches.  If the connection is
          stateless, the client explicitly limits the length of each response
          based upon estimates of the underlying channel conditions, so that
          responsiveness will not be lost.  For stateless communications,
          the client always interleaves requests from multiple request
          queues onto the same underlying communication channel, so as to
          avoid consuming excessive amounts of server processing resources
          (stateless requests are more expensive for the server to handle).
          Regardless of whether communication is stateful or stateless, the
          client manages its own version of request pre-emption.  In
          particular, requests which arrive on a request queue can pre-empt
          earlier requests which have not yet been delivered to the server.
     [>>] In addition to the explicit request queues mentioned above, the
          object provides a special internal "OOB queue" that can be used
          to quickly retrieve answers to (typically short) window-of-interest
          requests by posting them to the `post_oob_window' function -- this
          is particularly useful for implementing responsive browsing contexts
          for metadata while imagery is being concurrently streamed.  Here,
          OOB stands for "Out-Of-Band" and the intent is to route OOB requests
          around the normal queueing mechanism so as to achieve faster
          responses.
     [//]
     The following is a brief summary of the way in which
     this object is expected to be used:
     [>>] The client application uses the `connect' function to
          initiate communication with the server.  This starts a new thread
          of execution internally to manage the communication process.
          The cache may be used immediately, if desired, although its
          contents remain empty until the server connection has been
          established.  The `connect' function always creates a single
          initial request queue, which may be used to post requests even
          before communication has actually been established.
     [>>] The client application determines whether or not the remote
          object is a JP2-family file, by invoking
          `kdu_cache::get_databin_length' on meta data-bin 0, using its
          `is_complete' member to determine whether a returned length
          of 0 means that meta data-bin 0 is empty, or simply that nothing
          is yet known about the data-bin, based on communication with the
          server so far (the server might not even have been connected yet).
     [>>] If the `kdu_cache::get_databin_length' function returns a length of
          0 for meta data-bin 0, and the data-bin is complete, the image
          corresponds to a single raw JPEG2000 code-stream and the client
          application must wait until the main header data-bin is complete,
          calling `kdu_cache::get_databin_length' again (this time querying
          the state of the code-stream main header data-bin) to determine
          when this has occurred.  At that time, the present object may be
          passed directly to `kdu_codestream::create'.
     [>>] If the `kdu_cache::get_databin_length' function identifies meta
          data-bin 0 as non-empty, the image source is a JP2-family file.
          The client application uses the present object to open a
          `jp2_family_src' object, which is then used to open a `jp2_source'
          or `jpx_source' object (or any other suitable file format parser).
          In the case where a `jp2_source' object is used (this is a relatively
          elementary file format parser), the application calls
          `jp2_source::read_header' until it returns true (waiting for new
          cache contents between such calls); at that point, the `jp2_source'
          object is passed to `kdu_codestream::create'.
          In the case of a `jpx_source' object, the application calls
          `jpx_source::open' until it returns non-zero (waiting for new
          cache contents between such calls).  Many of the `jpx_source'
          object's API function provide special return values to indicate
          whether or not the information of interest is available from the
          cache at the time when the function is called -- if not, and
          the application has already posted an appropriate window of interest,
          it has only to wait until more data becomes available in the cache
          are retry the function.
     [>>] The client application may use the `disconnect' function to
          disconnect from the server, while continuing to use the cache.
          By contrast, the `close' function both disconnects from the
          server (if still connected) and discards the contents of the cache.
          The `disconnect' function can also be used to delete a single
          request queue (and any communication resources exclusively
          associated with that request queue) while leaving others connected.
     [>>] Additional request queues may be added with the `add_queue'
          function.  In fact, this function may be invoked any time after
          the call to `connect', even if communication has not yet been
          established with the server.  In a typical application, multiple
          request queues might be associated with multiple open viewing
          windows, so that an interactive user can select distinct windows
          of interest (region of interest, zoom factor of interest,
          components of interest, codestreams of interest, etc.) within
          each viewing window.
     [//]
     The `kdu_client' object provides a number of status functions for
     monitoring the state of the network connection.  These are
     [>>] `is_active' -- returns true from the point at which `connect'
          is called, remaining true until `close' is called, even if all
          connections with the server have been dropped or none were ever
          completed.
     [>>] `is_alive' -- allows you to check whether the communication
          channel used by a single request queue (or by any of the request
          queues) is alive.  Request queues whose underlying
          communication channel is still in the process
          of being established are considered to be alive.
     [>>] `is_idle' -- allows you to determine whether or not a request
          queue (or all request queues) are idle, in the sense that the
          server has fully responded to existing requests on the queue,
          but the queue is alive.
  */
  public: // Member functions
    KDU_AUX_EXPORT kdu_client();
    KDU_AUX_EXPORT virtual ~kdu_client();
      /* [SYNOPSIS]
           Invokes `close' before destroying the object.
      */
    KDU_AUX_EXPORT static const char *
      check_compatible_url(const char *url,
                           bool resource_component_must_exist,
                           const char **port_start=NULL,
                           const char **resource_start=NULL,
                           const char **query_start=NULL);
      /* [SYNOPSIS]
           This static function provides a useful service to applications
           prior to calling `connect'.  It identifies whether or not the
           supplied `url' string might represent a JPIP URL and returns
           information concerning its major components.  To be a compatible
           URL, the `url' string must be of the general form:
           [>>] "<prot>://<HOST>[:<port>]/<resource>[?<query>]", where
           [>>] <HOST> is a <hostname>, <IPv4 literal> or '['<IP literal>']'
           [>>] Notice that the <HOST> and <port> components follow the
                conventions outlined in RFC 3986.  Specifically, the optional
                <port> suffix is separated by a colon and IPv6 literal
                addresses should be enclosed in square brackets.  IPv4 literal
                addresses may appear in bare form or enclosed within square
                brackets.
           [>>] The <prot> prefix must be one of "jpip" or "http" -- case
                insensitive.  The port number, if present, should be a decimal
                integer in the range 0 to (2^16)-1, although the port suffix
                is not specifically examined by this function.
           [>>] It is expected that <hostname>, <resource> and <query>
                components have been hex-hex encoded, if necessary, so that
                they do not contain non-URI-legal characters or any
                characters which could cause ambiguity in the interpretation
                of the `url'.  In particular, while "&" and "?" are URI-legal,
                they should be hex-hex encoded where found within the hostname
                or resource components of the `url'.
           [//]
           If the string appears to have the above form, the function returns
           a pointer to the start of the <HOST> component.  Additionally:
           if `port_start' is non-NULL, it is used to return the start of
           any <port> component (NULL if there is none); if `resource_start'
           is non-NULL, it is used to return the start of the <resource>
           component; and if `query_start' is non-NULL it is used to return
           the start of any <query> component (NULL if there is none).
         [RETURNS]
           NULL if the `url' string does not appear to be a compatible JPIP
           URL, else the start of the <hostname> component of the URL within
           the `url' sltring.
         [ARG: resource_component_must_exist]
           If false, the function does not actually require the "<resource>"
           component to be present within the `url' string.  In this case,
           it returns non-NULL so long as a compatible "<prot>://" prefix is
           detected; the `resource_start' argument may be used to return
           information about the presence or absence of the otherwise
           mandatory <resource> component.
         [ARG: port_start]
           If non-NULL, this argument is used to return a pointer to the start
           of any <port> sub-string within the `url' string; if there is none,
           *`port_start' is set to NULL.
         [ARG: resource_start]
           If non-NULL, this argument is used to return a pointer to the start
           of the <resource> sub-string within the `url' string.  If
           `resource_component_must_exist' is true, a <resource> sub-string
           must be present for `url' to be a compatible URL.  However, if
           `resource_component_must_exist' is false and the "/" separator
           is not found within the text which follows the "<prot>://" prefix,
           the function will set *`resource_start' to NULL.
         [ARG: query_start]
           If non-NULL, this argument is used to return a pointer to the start
           of any <query> sub-string within the `url' string; if there is none,
           *`query_start' is set to NULL.
      */
    KDU_AUX_EXPORT void
      install_context_translator(kdu_client_translator *translator);
      /* [SYNOPSIS]
           You may call this function at any time, to install a translator
           for context requests.  Translators are required only if you
           wish to issue requests which involve a codestream context
           (see `kdu_window::contexts'), and the client-server communications
           are stateless, or information from a previous cached session is
           to be reused.  In these cases, the context translator serves to
           help the client determine what codestreams are being (implicitly)
           requested, and what each of their effective request windows are.
           This information, in turn, allows the client to inform the server
           of relevant data-bins which it already has in its cache, either
           fully or in part.  If you do not install a translator,
           communications involving codestream contexts may become
           unnecessarily inefficient under the circumstances described above.
           [//]
           Conceivably, a different translator might be created to handle
           each different type of image file format which can be accessed
           using a JPIP client.  For the moment, a single translator serves
           the needs of all JP2/JPX files, while a separate translator may
           be required for MJ2 (motion JPEG2000) files, and another one might
           be required for JPM (compound document) files.
           [//]
           You can install the translator prior to or after the call to
           `connect'.  If you have several possible translators available,
           you might want to wait until after `connect' has been called
           and sufficient information has been received to determine the
           file format, before installing a translator.
           [//]
           You may replace an existing translator, which has already been,
           installed, but you may only do this when the client is not
           active, as identified by the `is_active' function.  To
           remove an existing translator, without replacing it, supply
           NULL for the `translator' argument.
      */
   void install_notifier(kdu_client_notifier *notifier)
      { acquire_lock(); this->notifier = notifier; release_lock(); }
      /* [SYNOPSIS]
           Provides a generic mechanism for the network management thread
           to notify the application when the connection state changes or
           when new data is placed in the cache.  Specifically, the
           application provides an appropriate object, derived from the
           abstract base class, `kdu_client_notifier'.  The object's virtual
           `notify' function is called from within the network management
           thread whenever any of the following conditions occur:
           [>>] Connection with the server is completed;
           [>>] The server has acknowledged a request for a new window into
                the image, possibly modifying the requested window to suit
                its needs (call `get_window_in_progress' to learn about any
                changes to the window which is actually being served);
           [>>] One or more new messages from the server have been received
                and used to augment the cache; or
           [>>] The server has disconnected -- in this case,
                `notifier->notify' is called immediately before the network
                management thread is terminated, but `is_alive' is guaranteed
                to be false from immediately before the point at which the
                notification was raised, so as to avoid possible race
                conditions in the client application.
           [//]
           A typical notifier would post a message on an interactive client's
           message queue so as to wake the application up if necessary.  A
           more intelligent notifier may choose to wake the application up in
           this way only if a substantial change has occurred in the cache --
           something which may be determined with the aid of the
           `kdu_cache::get_transferred_bytes' function.
           [//]
           The following functions should never be called from within the
           `notifier->notify' call, since that call is generated from within
           the network management thread: `connect', `add_queue', `close',
           `disconnect' and `post_window'.
           [//]
           We note that the notifier remains installed after its `notify'
           function has been called, so there is no need to re-install the
           notifier.  If you wish to remove the notifier, the
           `install_notifier' function may be called with a NULL argument.
           [//]
           You may install the notifier before or after the first call to
           `connect'; however, the call to `connect' starts a network
           management thread, and the notifier will be removed as the final
           step in the termination of this thread - thus, you cannot
           expect the notifier to remain installed after an unsuccessful call
           to `connect'.  The notifier will also be removed by a call to
           `close'.
           [//]
           In the future, you can expect a more powerful notification
           mechanism to be offered by Kakadu, providing more detailed
           information about the relevant events -- this will exist side
           by side with the current method, for backward compatibility.
      */
    void set_primary_timeout(kdu_long timeout_usecs)
      { if (timeout_usecs < 1000) timeout_usecs = 1000;
        this->primary_connection_timeout_usecs = timeout_usecs; }
      /* [SYNOPSIS]
           When a new primary connection is created (ultimately a TCP socket)
           the value passed to this function governs the amount of time
           (in microseconds) that can transpire before the server sends back
           a response to the initial request.  If no response is received in
           this period, the connection attempt times out and will not be
           retried -- `is_alive' then returns false, for the request queue
           that tried the connection (or for all request queues if the
           initial connection attempt to the server timed out).
      */
    void set_aux_tcp_timeout(kdu_long timeout_usecs)
      { if (timeout_usecs < 1000) timeout_usecs = 1000;
        this->aux_connection_timeout_usecs = timeout_usecs; }
      /* [SYNOPSIS]
           Same as `set_primary_timeout' but for the auxiliary TCP
           channel associated with an HTTP-TCP transport.
      */
    KDU_AUX_EXPORT virtual int
      connect(const char *server, const char *proxy,
              const char *request, const char *channel_transport,
              const char *cache_dir, kdu_client_mode mode=KDU_CLIENT_MODE_AUTO,
              const char *compatible_url=NULL);
      /* [SYNOPSIS]
           Creates a new thread of execution to manage network communications
           and initiate a connection with the appropriate server.  The
           function returns immediately, without blocking on the success of
           the connection.  The application may then monitor the value
           returned by `is_active' and `is_alive' to determine when the
           connection is actually established, if at all.  The application
           may also monitor transfer statistics using
           `kdu_cache::get_transferred_bytes' and it may receive notification
           of network activity by supplying a `kdu_client_notifier'-derived
           object to the `install_notifier' function.
           [//]
           The function creates an initial request queue to which requests can
           be posted by `post_window' (in interactive mode -- see `mode'),
           returning the queue identifier.  In interactive mode, additional
           request queues can be created with `add_queue', even before the
           request completes.
           [//]
           Before calling this function, you might want to consider overriding
           the default timeout values associated with primary and/or
           auxiliary (see `channel_transport') communication channels.  By
           default, the primary connection attempt times out after 3 seconds.
           If an auxiliary TCP channel is granted in the reply, a further
           5 seconds is allowed to complete the auxiliary connection.  These
           parameters can be modified at any time via
           `set_primary_timeout' and `set_aux_tcp_timeout'.
           [//]
           The network management thread is terminated once there are no
           more alive request queues -- request queues are considered
           to be alive if the associated communication channel is alive or
           in the process of being established.  One way to kill the
           network management thread is to invoke `close', since this
           performs a hard close on all the open communication channels.
           A more graceful approach is to invoke the `disconnect' function
           on all request queues (one by one, or all at once).  Request
           queues may also disconnect asynchronously if their underlying
           communication channels are dropped by the server or through
           some communication error.
           [//]
           You may call this function again, to establish a new connection.
           However, you should note that `close' will be called automatically
           for you if you have not already invoked that function.  If
           a previous connection was gracefully closed down through one or
           more successful calls to `disconnect', the object may be able to
           re-use an established TCP channel in a subsequent `connect'
           attempt.  For more on this, consult the description of the
           `disconnect' function.
           [//]
           The present function may itself issue a terminal error message
           through `kdu_error' if there is something wrong with any of the
           supplied arguments.  For this reason, you should generally provide
           try/catch protection when calling this function if you don't want
           your application to die in the event of a bad call.
         [RETURNS]
           A request queue identifier, which can be used with calls to
           `post_window', `get_window_in_progress', `is_idle' and quite
           a few other functions.  In practice, when you first connect
           to the server, the initial request queue is assigned an identifier
           of 0, which is always the value returned by this function.
           [//]
           In summary, this function always returns 0 (or else generates
           an error), but you should use the return value in calls which
           require a request queue identifier without assuming that the
           identifier is 0.
         [ARG: server]
           Holds the host name or IP literal address of the server to be
           contacted, together with optional port information.  This string
           must follow the same conventions as those outlined for the <HOST>
           and <port> components of the string supplied to
           `check_compatible_url'.  Specifically, an optional decimal numeric
           <port> suffix may appear at the end of the string, separated by a
           colon, while the remainder of the string is either a host name
           string (address to be resolved), a dotted IPv4 literal address, or
           an IPv4 or IPv6 literal address enclosed in square brackets.
           Host name strings are hex-hex decoded prior to resolution.
           [//]
           The default HTTP port number of 80 is assumed if none is provided.
           Communication with the `server' machine proceeds over HTTP, and
           may involve intermediate proxies, as described in connection with
           the `proxy' argument.
           [//]
           This argument may be NULL only if the `compatible_url' argument
           is non-NULL, in which case `compatible_url' must pass the test
           associated with the `check_compatible_url' function, and the
           `server' string is obtained from the <HOST> and <port> components
           of the `compatible_url'.
         [ARG: proxy]
           Same syntax as `server', but gives the host (and optionally the
           port number) of the machine with which the initial TCP connection
           should be established.  This may either be the server itself, or
           an HTTP proxy server.  May be NULL, or point to an empty string,
           if no proxy is to be used.
           [//]
           As for `server', the function anticipates potential hex-hex
           encoding of any host name component of the `proxy' string and
           performs hex-hex decoding prior to any host resolution attempts.
         [ARG: request]
           This argument may be NULL only if `compatible_url' is non-NULL, in
           which case the `compatible_url' string must pass the test
           associated with `check_compatible_url' and contain a valid
           <resource> sub-string; the entire contents of `compatible_url',
           commencing from the <resource> sub-string, are interpreted as the
           `request' in that case.  In either case, the `request' string may
           not be empty.
           [//]
           As explained in connection with `check_compatible_url', the
           `request' string may consist of both a <resource> component and a
           <query> component (the latter is optional) and hex-hex encoding
           of each component is expected.  Hex-hex decoding of any query
           component is performed separately on each request field, so that
           hex-hex encoded '&' characters within the contents of any query
           field will not be mistaken for query field separators.
         [ARG: channel_transport]
           If NULL or a pointer to an empty string or the string "none", no
           attempt will be made to establish a JPIP channel.  In this case,
           no attempt will be made to establish a stateful communication
           session; each request is delivered to the server (possibly through
           the proxy) in a self-contained manner, with all relevant cache
           contents identified using appropriate JPIP-defined header lines.
           When used in the interactive mode (see `mode'), this may
           involve somewhat lengthy requests, and may cause the server to go
           to quite a bit of additional work, re-creating the context for each
           and every request.
           [//]
           If `channel_transport' holds the string "http" and the mode is
           interactive (see `mode'), the client's first request asks
           for a channel with HTTP as the transport.  If the server refuses
           to grant a channel, communication will continue as if the
           `channel_transport' argument had been NULL.
           [//]
           If `channel_transport' holds the string "http-tcp", the behaviour
           is the same as if it were "http", except that a second TCP
           channel is established for the return data.  This transport
           variant is somewhat more efficient for both client and server,
           but requires an additional TCP channel and cannot be used from
           within domains which mandate that all external communication
           proceed through HTTP proxies.  If the server does not support
           the "http-tcp" transport, it may fall back to an HTTP transported
           channel.  This is because the client's request to the server
           includes a request field of the form "cnew=http-tcp,http", giving
           the server both options.
           [//]
           If `channel_transport' holds the string "http-udp", the behaviour
           is similar to "http-tcp", except that the return data is transported
           over an auxiliary UDP channel instead of a TCP channel.  This
           transport variant has the potential to be the most efficient of
           all, since there is no unnecessary delay incurred trying to
           order transmitted packets or wait for the retransmission of lost
           packets.  In this mode, the client determines when UDP datagrams
           are likely to have been lost and sends negative acknowledgement
           messages at appropriate junctures.  The server also monitors
           UDP acknowledgement messages and implements its own strategy for
           dealing with potentially lost datagrams.  As with "http-tcp", the
           client leaves the server to monitor channel behaviour and implement
           appropriate flow control.  If the server does not support the
           "http-udp" transport, it may fall back to an "http-tcp" scheme
           or even a plain "http" transported channel.  This is because the
           client's request to the server includes a request field of the
           form "cnew=http-udp,http-tcp,http", giving the server all three
           options.
           [//]
           It is worth noting that wherever a channel is requested, the
           server may respond with the address of a different host to be
           used in all future requests; redirection of this form is
           handled automatically by the internal machinery.
         [ARG: cache_dir]
           If non-NULL, this argument provides the path name for a directory
           in which cached data may be saved at the end of an interactive
           communication session.  The directory is also searched at the
           beginning of an interactive session, to see if information is
           already available for the image in question.  If the argument
           is NULL, or points to an empty string, the cache contents will
           not be saved and no previously cached information may be re-used
           here.  Files written to or read from the cache directory have the
           ".kjc" suffix, which stands for (Kakadu JPIP Cache).  These
           files commence with some details which may be used to re-establish
           connection with the server, if necessary (not currently
           implemented) followed by the cached data itself, stored as a
           concatenated list of data-bins.  The format of these files is
           private to the current implementation and subject to change in
           subsequent releases of the Kakadu software, although the files are
           at least guaranteed to have an initial header which can be used
           for version-validation purposes.
         [ARG: mode]
           Determines whether the client is to operate in interactive or
           non-interactive modes.  This argument may take on one of three
           values, as follows:
           [>>] `KDU_CLIENT_MODE_INTERACTIVE' -- in this mode, the
                application may post new requests to `post_window'.  New
                request queues may also be created in interactive mode using
                `add_queue'.
           [>>] `KDU_CLIENT_MODE_NON_INTERACTIVE' -- in this mode, all calls
                to `post_window' are ignored and the `request' string
                (or `compatible_url' string) is expected to express the
                application's interests completely.  In non-interactive mode,
                the client issues a single request to the server and collects
                the response, closing the channel upon completion.  If the
                `cache_dir' argument is non-NULL and there is at least one
                cache file which appears to be compatible with the request,
                the client executes exactly two requests: one to obtain the
                target-id from the server, so as to determine the compatibility
                of any cached contents; and one to request the information of
                interest.
           [>>] `KDU_CLIENT_MODE_AUTO' -- in this case, the `connect' function
                automatically decides whether to use the interactive or
                non-interactive mode, based on the form of the request, as
                found in the `request' or `compatible_url' arguments.
                If the request string contains a query component (i.e., if
                it is of the form <resource>?<query>, where "?" is the
                query separator), the <query> string may contain multiple
                fields, each of the form <name>=<value>, separated by the
                usual "&" character.  If a <query> string contains anything
                other than the "target" or "subtarget" fields, the
                non-interactive mode will be selected; otherwise, the
                interacive mode is selected.
         [ARG: compatible_url]
           This optional argument allows you to avoid explicitly extracting
           the `server' and `request' sub-strings from a compatible JPIP
           URL string, letting the present function do that for you.  In this
           case, you may set either or both of the `server' and `request'
           arguments to NULL.  However, you do need to first check that
           the `check_compatible_url' function returns true when supplied with
           the `compatible_url' string and the `resource_component_must_exist'
           argument to that function is true.  If the `request' argument
           is non-NULL, the <resource> component of `compatible_url' will not
           be used.  Similarly, if the `server' argument is non-NULL, the
           <HOST> and <port> components of `compatible_url' will not be used.
           If both `request' and `server' are non-NULL, the `compatible_url'
           string will not be used at all, but may still be supplied if you
           like.
      */
    bool is_interactive() { return (!non_interactive) && is_alive(-1); }
      /* [SYNOPSIS]
           Returns true if the client is operating in the interactive mode
           and at least one request queue is still alive.  For more on
           interactive vs. non-interactive mode, see the `mode' argument
           to `connect'.
      */
    bool is_one_time_request() { return non_interactive; }
      /* [SYNOPSIS]
           Returns true if the client is operating in the non-interactive
           mode, as determined by the `mode' argument passed to `connect'.
           The return value from this function remains the same from the
           call to `connect' until `close' is called (potentially from
           inside a subsequent `connect' call).
      */
    bool connect_request_has_non_empty_window()
      { return initial_connection_window_non_empty; }
      /* [SYNOPSIS]
           Returns true if the request passed in the call to `connect'
           supplied a non-empty window of interest.  This means that a query
           string was supplied, which contained request fields that affected
           the initialization of a `kdu_window' object for the first request.
           This could mean that a specific region of interest was requested,
           specific codestreams or codestream contexts were requested,
           specific image components were requested, or specific metadata
           was requested, for example.  The application might use this
           information to decide whether to wait for the server to reply
           with enough data to open the relevant imagery, or issue its own
           overriding requests for the imagery it thinks should be most
           relevant.
           [//]
           The value returned by this function remains constant from the
           call to `connect' until a subsequent call to `close' (potentially
           issued implicitly from another call to `connect').
      */
    KDU_AUX_EXPORT virtual const char *get_target_name();
      /* [SYNOPSIS]
           This function returns a pointer to an internally managed string,
           which will remain valid at least until `close' is called.  If
           the `connect' function has not been called since the last call to
           `close' (i.e., if `is_active' returns false), this function returns
           the constant string "<no target>".  Otherwise the returned string
           was formed from the parameters passed to `connect'.  If the
           `connect' request string contained a `target' query parameter,
           that string is used; otherwise, the <resource> component of the
           request string is used.
           [//]
           If the requested resource has a sub-target (i.e., a byte range
           is identified within the primary target), the string returned by
           this function includes the sub-target information immediately
           before any file extension.
           [//]
           The function also decodes any hex-hex encoding which may have
           been used to ensure URI-legal names in the original call to
           `connect'.
      */
    KDU_AUX_EXPORT virtual bool
      check_compatible_connection(const char *server,
                                  const char *request,
                                  kdu_client_mode mode=KDU_CLIENT_MODE_AUTO,
                                  const char *compatible_url=NULL);
      /* [SYNOPSIS]
           This function is commonly used before calling `add_queue' to
           determine whether the object is currently connected in a manner
           which is compatible with a new connection that might otherwise
           need to be established.  If so, the caller can add a queue to
           the current object, rather than creating a new `kdu_client' object
           and constructing a connection from scratch.
           [//]
           If the client has no alive connections, or is executing in
           non-interactive mode (i.e., if `is_interactive' returns false), the
           function can still return true so long as the request and `mode'
           are compatible; although in this case the caller will not be able
           to add request queues or post window requests via `post_window'.
           [//]
           Compatibility for requests which contain query fields other than
           `target' or `subtarget' deserves some additional explanation.
           Firstly, a request for interactive communication (as determined
           by `mode') can only be compatible with a client object which is
           already in the interactive mode and vice-versa, noting that the
           interactivity of the request may need to be determined from the
           presence or absence of query fields other than `target' or
           `subtarget' if `mode' is `KDU_CLIENT_MODE_AUTO'.  Secondly, if
           there are query fields other than `target' or `subtarget', the
           request is considered compatible only if the intended mode is
           non-interactive and the current object is also in the
           non-interactive mode, with exactly the same query.
         [RETURNS]
           True if `is_active' returns true (note that the connection does
           not need to still be alive though) and the most recent call to
           `connect' was supplied with `server', `request' and
           `compatible_url' arguments which are compatible with those
           supplied here.
         [ARG: server]
           Provides the host name/address and (optionally) port components of
           the connection, if non-NULL.  Otherwise, this information must be
           recovered from `compatible_url'.  In either case, the host
           information must be compatible with that recovered by the most
           recent call to `connect', either recovered from its `server'
           argument or its `compatible_url' argument, or else the function
           returns false.
         [ARG: request]
           Provides the request component of the connection (resource name +
           an optional query string component).  Otherwise, the request
           component is recovered from `compatible_url' (the entire suffix
           of the `compatible_url' string, commencing with the <resource>
           sub-string).  In either case, the request component must be
           compatible with that recovered by the most recent call to
           `connect', either from its `request' argument or its
           `compatible_url' argument, or else the function returns false.
         [ARG: mode]
           This is the `mode' argument that the caller would supply to
           another `kdu_client' object's `connect' function.  If `mode'
           is `KDU_CLIENT_MODE_AUTO', the function determines whether the
           caller is interested in interactive behaviour based on the
           presence of any query fields other than `target' and `subtarget',
           using the same procedure as `connect'.  Whether the intended
           client mode is interactive or non-interactive affects the
           compatibility.
      */
    KDU_AUX_EXPORT virtual int add_queue();
      /* [SYNOPSIS]
           In non-interactive mode or if there are no more alive connections
           to the server (i.e. if `is_interactive' returns false), this
           function fails to create a new queue, returning -1.  Otherwise, the
           function creates a new request queue, returning the identifier you
           can use to post requests, disconnect and otherwise utilize the new
           queue.  If the server supports multiple JPIP channels and
           communication is not stateless, request queues may wind up being
           associated with distinct JPIP channels; this is generally the most
           efficient approach.  Otherwise, the client machinery takes care of
           interleaving requests onto an existing stateful or stateless
           communication channel.
         [RETURNS]
           A non-negative request queue identifier, or else -1, indicating
           that a new queue cannot be created.  This happens only
           under the following conditions:
           [>>] The `connect' function has not yet been called.
           [>>] All connections have been dropped, by a call to `close',
                `disconnect', or for some other reason (e.g., the server
                may have dropped the connection); this condition can be
                checked by calling `is_alive' with a -ve `queue_id' argument.
           [>>] The original call to `connect' configured the client for
                non-interactive communications -- i.e., `is_one_time_request'
                should be returning true and `is_interactive' should be
                returning false.
      */
    bool is_active() { return active_state; }
      /* [SYNOPSIS]
           Returns true from the point at which `connect' is called until the
           `close' function is called.  This means that the function may
           return true before a network connection with the server has
           actually been established, and it may return true after the
           network connection has been closed.
      */
    bool target_started() { return target_request_successful; }
      /* [SYNOPSIS]
           Returns true if the server has replied to the client's first
           request, indicating that the requested target (as returned by
           `get_target_name') is actually available.  One important reason
           for calling this function is to wait until you can be sure that
           any local cache file associated with the target has been opened
           internally, so that attempts to open imagery will be able to use
           the contents of any content cached from previous browsing
           sessions.
           [//]
           To this end, you are guaranteed that this function will not return
           true until the relevant reply has been processed, including
           associated activities such as searching for and opening a local
           cache file.  Moreover, you are also guaranteed that a notification
           will be posted to any installed `kdu_client_notifier' object
           immediately after the point at which this function will report
           true.
      */
    KDU_AUX_EXPORT virtual bool is_alive(int queue_id=-1);
      /* [SYNOPSIS]
           Returns true so long as a request queue is alive (or all request
           queues are alive).  A request queue is considered alive from the
           point at which it is created by `connect' or `add_queue' (even
           though it may take a while for actual communication to be
           established) until the point at which all associated communication
           is permanently stopped.  This may happen due to loss of the
           communication channel (e.g., the server may shut it down) or as
           a result of a call to `disconnect' or `close'.
           [//]
           Note that the `disconnect' function may supply a time limit for
           closure to complete, in which case the request queue may remain
           alive for a period of time after the call to `disconnect' returns.
           See that function for more information.
         [RETURNS]
           True if the the supplied `queue_id' identifies any request
           queue which is associated with a connected communication channel
           or one which is still being established.
         [ARG: queue_id]
           One of the identifiers returned by `connect' or `add_queue', or
           else a negative integer.  If negative, the function checks
           whether there are any communication channels which are alive.  If
           the argument is non-negative, but does not refer to any request
           queue identifier ever returned by `connect' or `add_queue', the
           function behaves as if supplied with the identifier of a
           request queue which has disconnected.
      */
    KDU_AUX_EXPORT virtual bool is_idle(int queue_id=-1);
      /* [SYNOPSIS]
           Returns true if the server has finished responding to all
           pending window requests on the indicated request queue and
           the `is_alive' function would return true if passed the same
           `queue_id'.
           [//]
           The state of this function reverts to false immediately after
           any call to `post_window' which returns true (i.e., any call which
           would cause a request to be sent to the server).  See that function
           for more information.
           [//]
           Note that the internal machinery goes to considerable effort
           to ensure that a queue is not identified as idle until all
           response data has been received for the most recent request
           issued on the queue, even if some of that response data may have
           been carried on separate JPIP channels, associated with other
           request queues whose requested content overlaps that for the
           queue in question.  This is done by keeping track of all
           requests on other request queues whose response data might
           contain part of the response to requests on any given queue,
           and waiting until all such dependencies have been resolved before
           a request can be truly considered complete.
         [RETURNS]
           True if `is_alive' would return true with the same
           `queue_id' argument and if the identified request queue
           (or all request queues, if `queue_id' < 0) is idle.
         [ARG: queue_id]
           One of the identifiers returned by `connect' or `add_queue', or
           else a negative integer.  If a negative value is supplied, the
           function returns true if there is at least one communication
           channel still alive and ALL of the communication channels are
           currently idle (i.e., connected, with no outstanding data to be
           delivered for any request queue), including the special internal
           "OOB queue" that is associated with calls to `post_oob_window'.
      */
    KDU_AUX_EXPORT virtual void disconnect(bool keep_transport_open=false,
                                           int timeout_milliseconds=2000,
                                           int queue_id=-1,
                                           bool wait_for_completion=true);
      /* [SYNOPSIS]
           Use this function to gracefully close request queues and the
           associated communication channels (once all request queues using
           the communication channel have disconnected).  Unlike `close',
           this function also leaves the object open for reading data
           from the cache.
           [//]
           This function may safely be called at any time.  If the `close'
           function has not been called since the last call to `connect',
           this function leaves `is_active' returning true, but will
           eventually cause `is_alive' to return false when invoked with the
           same `queue_id'.
           [//]
           After this function has been called, you will not be able to post
           any more window changes to the request queue via `post_window',
           even though the request queue may remain alive for some time
           (if `wait_for_completion' is false), in the sense that
           `is_alive' does not immediately return false.
           [//]
           The function actually causes a pre-emptive request to be posted as
           the last request in the queue, which involves an empty window of
           interest, to encourage the queue to become idle as soon as
           possible.  The function then notifies the thread management
           function that the request queue should be closed once it becomes
           idle, unless the `timeout_milliseconds' period expires first.
         [ARG: keep_transport_open]
           This argument does not necessarily have an immediate effect.  Its
           purpose is to try to keep a TCP channel open beyond the point at
           which all request channels have disconnected, so that the channel
           can be re-used in a later call to `connect'.  This can be useful
           in automated applications, which need to establish multiple
           connections in sequence with minimal overhead.
           [//]
           If this argument is true, the function puts the underlying primary
           communication channel for the identified request queue (or any
           request queue if `queue_id' is -1) in the "keep-alive" state,
           unless there is another primary channel already in the "keep-alive"
           state.  In practice, the primary channel will not be kept alive if
           it is closed by the server, or if an error occurs on some request
           channel which is using it.
           [//]
           If `keep_transport_open' is false, the function cancels the
           "keep-alive" status of any primary TCP channel, not just the one
           associated with an identified request queue, closing that channel
           if it is no longer in use.  The function may be used in this way
           to kill a channel that was being kept alive, even after all request
           queues have disconnected and, indeed, even after a call to `close'.
         [ARG: timeout_milliseconds]
           Specifies the maximum amount of time the network management thread
           should wait for the request queue to become idle before closing
           it down.  If this time limit expires, the forced closure of the
           request queue will also cause forced shutdown of the relevant
           underlying communication channels and any other request queues
           which may be using them.
           [//]
           If you have multiple request queues open and they happen to be
           sharing a common primary HTTP request channel (e.g., because
           the server was unwilling to assign multiple JPIP channels),
           you should be aware that forced termination of the request queue
           due to a timeout will generally cause the primary channel to
           be shut down.  This means that your other request queues will also
           be disconnected.  To avoid this, you are recommended to specify
           a timeout which is quite long, unless you are in the process of
           closing all request queues associated with the client.
           [//]
           You can always reduce the timeout by calling this function again.
         [ARG: wait_for_completion]
           If true, the function blocks the caller until the request queue
           ceases to be alive.  As explained above, this means that the
           request queue must become idle, or the specified timeout must
           expire.  If false, the function returns immediately, so that
           subsequent calls to `is_alive' may return true for some time.
           [//]
           If you need to specify a long timeout, for the reasons outlined
           above under `timeout_milliseconds', it is usually best not to
           wait.  Waiting usually makes more sense when closing all request
           queues associated with the client, in which case a short timeout
           should do no harm.
         [ARG: queue_id]
           One of the request queue identifiers returned by a previous call
           to `connect' or `add_queue', or else a negative integer, in which
           case all request queues will be disconnected with the same
           parameters.  If the indicated queue identifier was never issued
           by a call to `connect' or `add_queue' or was previously
           disconnected, the function does nothing except potentially
           remove the "keep-alive" state of a primary TCP channel, as
           discussed in the description of the `keep_transport_open'
           argument -- this may cause a previously saved TCP transport
           channel to be closed.
      */
    KDU_AUX_EXPORT virtual bool close();
      /* [SYNOPSIS]
           Effectively, this function disconnects all open communication
           channels immediately and discards the entire contents of the
           cache, after which all calls to `is_alive' and `is_active' will
           to return false.  It is safe to call this function at any time,
           regardless of whether or not there is any connection.
           [//]
           If `disconnect' has already been used to disconnect all request
           queues and their communication channels, and if that function has
           saved an underlying transport channel, the present function will
           not destroy it -- that will happen only if a subsequent call to
           `connect' cannot use it, if `disconnect' is called explicitly with
           `keep_transport_open'=false, or if the object is destroyed.
      */
    KDU_AUX_EXPORT virtual bool
      post_window(const kdu_window *window, int queue_id=0,
                  bool preemptive=true, const kdu_window_prefs *prefs=NULL,
                  kdu_long custom_id=0, kdu_long service_usecs=0);
      /* [SYNOPSIS]
           Requests that a message be delivered to the server identifying
           a change in the user's window of interest into the compressed
           image and/or a change in the user's service preferences.  The
           message may not be delivered immediately; it may not
           even be delivered at all, if the function is called again
           specifying a different access window for the same request queue,
           with `preemptive'=true, before the function has a chance to deliver
           a request for the current window to the server.
           [//]
           You should note that each request queue (as identified by the
           `queue_id' argument) behaves independently, in the sense that
           newly posted window requests can only preempt exising ones within
           the same queue.
           [//]
           If `preemptive' is false, the nominal behaviour is to issue this
           window request to the server in such a way that any outstanding
           posted window request for the queue is served completely before
           service of this one begins.  However, this behaviour is affected
           by the `service_usecs' argument supplied with previously
           posted windows -- see the extensive discussion of `service_usecs'
           below.
           [//]
           It is important to note that the server may not be willing to
           serve some windows of interest in the exact form that they are
           requested.  When this happens, the server's response is required
           to identify modified attributes of the window which it is able to
           service.  The client may learn about the actual window for which
           data is being served by calling `get_window_in_progress'.  The
           service preferences signalled by `prefs' may also affect whether
           or not the server actually makes any modifications.  In particular,
           the `KDU_WINDOW_PREF_FULL' and `KDU_WINDOW_PREF_PROGRESSIVE'
           options are particularly important for manipulating the way in
           which the server treats large request windows.
           [//]
           This function may be called at any time after a request queue
           is instantiated by `connect' or `add_queue', without
           necessarily waiting for the connection to be established or for
           sufficient headers to arrive for a call to `open' to succeed.
           This can be useful, since it allows an initial window to be
           requested, while the initial transfer of mandatory headers is
           in progress, or even before it starts, thereby avoiding the
           latency associated with extra round-trip times.
           [//]
           The `custom_id' value can be used to associate each request posted
           here with an application-specified identifier (defaults to 0).
           One purpose of this is to make it easier to match requests
           that are in progress (see `get_window_in_progress') with those
           that were posted, in applications where many requests may be posted
           in sequence.  The `custom_id' value also allows you to obtain
           detailed status information about specific requests that have been
           posted (see `get_window_info').
           [//]
           We now describe the role of the `service_usecs' argument and the
           concept of "timed requests".  A "timed request sequence" is
           initiated by issuing a pre-emptive call to this function
           with `service_usecs' > 0; if the preceding request had
           `service_usecs'<=0, a new request with `service_usecs' > 0
           will be interpreted as preemptive, regardless of the value of
           the `preemptive' argument, but you should avoid such usage.
           [//]
           After this initiating `post_window' call, subsequent calls with
           `service_usecs' > 0 and `preemptive'=false add to the
           sequence of timed requests.  Let T_j denote the `service_usecs'
           value (a time, measured in microseconds) that corresponds to
           the j'th call to this function, where j=J0 corresponds to the
           first call in a given timed request sequence and j=J1
           corresponds to the first call that does not belong to the timed
           request sequence -- this is either a call with `service_usecs'=0
           (regular request) or a request with `preemptive'=true and
           `service_usecs'>0 (start of a new timed request sequence).  Let
           C_j be the cumulative service time associated with requests
           J0 through to j.  That is, C_j = sum_{J0<=i<=j} T_i.  The
           internal machinery aims to construct requests in such a way
           that the cumulative amount of time devoted to the transmission
           of response data for requests J0 through to j is approximately
           equal to C_j, for each j in the range J0 <= j < J1.  Although
           any given request might receive more or less service time than
           requested, these deviations should not accumulate to yield
           larger mismatches over time.
           [//]
           The cumulative service time guarantee is an important commitment
           that the `kdu_client' object makes to the application.  Moreover,
           it commits to measure the service time using the application's
           own reference clock, so long as the application regularly invokes
           the `sync_timing' function.  Naturally, the application must play
           its part.  If the application does not post requests in a timely
           manner, the communication channel may go idle.  Even in this case,
           the internal machinery aims to honour its service time commitment,
           but this is achieved by treating channel idle time as a kind of
           service and foreshortening or even discarding posted requests
           that arrive too late -- i.e., when faced with the situation in
           which the application has not provided requests to send, the
           internal implementation attributes idle channel time as
           foregone service time for future posted windows of interest.
           If the application does not cancel a timed request sequence (by
           starting a new one or posting a window with `service_usecs'=0),
           considerable idle time may accumulate, but this is exactly
           the behaviour that simplifies the design of video/animation
           applications.  It means that the application can supply
           `service_usecs' values that correspond exactly to the separation
           between the display times for frames in a video or animation,
           and be guaranteed that (apart from a bounded offset) the
           response data will be timed to match the animation display process.
           [//]
           Typically, if the application's display timing changes in some
           way, it is appropriate to start a new sequence of timed requests
           by posting the first such request with `preemptive'=true.  If an
           animation completes, it is advisable for the application to
           explicitly cancel any existing timed request sequence; if the
           application has no specific window of interest to post when
           it ends a timed request sequence, it can cancel the sequence
           by calling this function with `service_usecs'=0 and
           `window'=NULL.  The special NULL `window' argument does not
           actually cause a request to be sent to the server but it does
           cancel the timed request sequence.
           [//]
           It is worth noting that request queues offered by `kdu_client'
           may be forced to share an underlying JPIP channel, if the
           server does not offer sufficient physical JPIP channels to
           accommodate the request queues that the application opens.  In
           this case, if any of the request queues associated with a JPIP
           channel has started a timed request sequence, all request queues
           associated with the channel are serviced using timed requests --
           this is achieved internally by splitting non-timed requests up
           into timed pieces (as many as required), adopting service times
           that are in line with those used by the queue(s) that are
           actually posting timed requests.  These activities should be
           largely transparent to the application, except possibly for
           increased timing jitter in the cumulative service times C_j.
           Of course, the amount of physical channel time received by a
           request queue will be reduced if it has to share the channel with
           other request queues; however, the service time will appear to
           be exactly what was requested (only the data rate will appear
           to be lower).
           [//]
           Timed requests may be implemented internally using either
           the JPIP "len" (byte limit) request field, or the JPIP "twait"
           (timed wait) request field, to the extent that it is supported
           by the server.
         [RETURNS]
           False if `queue_id' does not refer to a request queue
           which is currently alive, or if the call otherwise has no effect.
           Specifically, the call returns false under one of the following
           conditions:
           [>>] The supplied `window' is: a subset of a recent window which is
                known to have been completely delivered by the server; AND
                either the request queue is idle or posting is not
                `preemptive'.  If this second condition does not hold,
                posting the window to the queue's list of requests can serve
                the purpose of pre-empting earlier requests that are in
                progress or not yet sent, so the function will not return
                false, even if the request is completely subsumed by a
                previous completely served request.  This can actually be
                a useful way of temporarily halting the flow of data from
                a server.
           [>>] All three of the following hold: a) the supplied `window' is
                identical in every respect to the last one successfully
                posted; b) the `custom_id' value is identical to that used
                with the last successfully posted window; and c) either
                `preemptive' is false in this call, or the last successfully
                posted window was `preemptive'.  If this last condition is
                not satisfied, this window is being pre-emptively posted,
                while the previous one was not, which means that this posting
                does have an effect.
           [//]
           The dependence of the last clause above upon the `custom_id'
           values is important for some applications.  In particular, these
           conditions mean that requests which have distinct `custom_id'
           values will always be assigned unique slots in the request queue,
           even if their windows of interest are identical, except only in
           the case where it can be determined that the request has been
           completely served in full.
           [//]
           It follows that the request queue may contain multiple consecutive
           identical window requests, all but the first of which are
           non-preemptive, differing only in their `custom_id' values --
           something that is not sent to the server.  From the application's
           point of view at least, these will all appear as distinct requests
           that may be sent to the server, unless they are pre-empted by a
           later request.
         [ARG: window]
           This argument may be NULL, in which case the call does not
           actually request anything, but may still be useful for cancelling
           a "timed request sequence" (see above) or for modifying preferences
           (see `prefs').
         [ARG: preemptive]
           If you call this function with `preemptive'=true (best for
           interactive image browsing), the new window will pre-empt any
           undelivered requests and potentially pre-empt requests which
           have been sent to the server but for which the server has not
           yet generated the complete response.  That is, requests may be
           pre-empted within the client's queue of undelivered requests, or
           within the server's queue of unprocessed requests).  This provides
           a useful way of discarding a queue of waiting non-preemptive
           window requests.
           [//]
           If false, the posted window will not pre-empt previous requests,
           either within the client's queue of undelivered requests, or
           within the server's queue of unprocessed requests.  However,
           if the previous request was posted with a non-zero
           `service_usecs' argument, that request will be effectively
           pre-empted after it has received the relevant amount of service
           time -- see the detailed discussion of timed requests above.
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions.  If the queue identifier is
           invalid or the relevant queue is no longer alive, the
           function simply returns false.
         [ARG: prefs]
           If non-NULL, the function updates its internal understanding of
           the application's service preferences to reflect any changes
           supplied via the `prefs' object.  These changes will be passed to
           the server with the next actual request which is sent.  Even if
           requests are not delivered immediately, preference changes are
           accumulated internally between successive calls to this function,
           until a request is actually sent to the server.
           [//]
           It is important to note that the function does not replace existing
           preferences wholesale, based upon a `prefs' object supplied here.
           Instead, replacement proceeds within "related-pref-sets", based on
           whether or not any preference information is supplied within the
           "related-pref-set".  This is accomplished using the
           `kdu_window_prefs::update' function, whose documentation you might
           like to peruse to understand "related-pref-sets" better.
           [//]
           Kakadu's server currently supports quite a few of the preference
           options which are offered by JPIP.  Amongst these, probably the
           most useful are those which affect whether or not the requested
           window can be limited by the server (`KDU_WINDOW_PREF_PROGRESSIVE')
           for the most efficient quality progressive service, or whether
           the server should try to serve the whole thing even if spatially
           progressive delivery is required (`KDU_WINDOW_PREF_FULLWINDOW'),
           along with those which affect the order in which the relevant
           codestreams are delivered (`KDU_CODESEQ_PREF_FWD',
           `KDU_CODESEQ_PREF_BWD' or the default `KDU_CODESEQ_PREF_ILVD').
           [//]
           Kakadu's demonstration viewers ("kdu_macshow" and "kdu_winshow")
           use the preferences mentioned above to facilitate the most
           effective interactive browsing experience based on the way the
           user manipulates a focus window.
           [//]
           You are strongly advised NOT to supply any "required" preferences
           at the moment.  Although required preferences are supported in
           the implementation of Kakadu's client and server, if a server
           does not support some preference option that you identify as
           "required" (as opposed to just "preferred"), the server is
           obliged to respond with an error message.  Currently, the
           `kdu_client' implementation does not provide specific handling
           for preference-related error messages, so that they will be
           treated like any other server error, terminating ongoing
           communication.  In the future, special processing should be
           introduced to catch these errors and re-issue requests without
           the required preference while remembering which types of
           preferences are not supported by the server.
           [//]
           You should be aware that preferences are managed separately for
           each `queue_id'.
         [ARG: custom_id]
           Arbitrary identifier supplied by the caller.  This identifier is
           not explicitly communicated to the server, but serves three
           purposes:
           [>>] The identifier is used by applications to identify specific
                posted windows in calls to `get_window_info'.
           [>>] The identifier can be used to simplify the recognition of
                specific posted windows when examining the information
                returned by `get_window_in_progress'.
           [>>] Provision of distinct identifiers provides a mechanism for
                the application to ensure that calls to this function result
                in distinct entries in the request queue, even if an identical
                request is already in progress.
         [ARG: service_usecs]
           This argument plays an important role in animation/video
           applications.  If this argument is greater than 0, the response
           data for this request notionally occupies `service_usecs'
           microseconds of transmission time on the communication channel
           (or a virtual communication channel, with reduced data rate,
           if there are multiple request queues sharing the same physical
           JPIP channel).  As mentioned in the extensive discussion of
           timed request sequence above, the internal implementation does
           not gurantee that the actual amount of service time for a given
           request will be exactly equal to `service_usecs' (it might not
           even be close), but it does provide guarantees on the cumulative
           service time experienced by a sequence of timed requests.
           [//]
           Applications that are interested in using timed requests should
           also invoke `sync_timing' regularly, at least to reconcile
           any differences that may accumulate between the application's
           time clock and the one used internally to this object -- this
           can probably be ignored if the application bases its timing
           expectations entirely upon the system real-time clock, but it
           is best not to make any assumptions about the clocks that are
           used.
           [//]
           The `get_timed_request_horizon' function also provides important
           guidance regarding the point at which an application should
           issue the next request in a timed sequence, so as to avoid
           getting too far ahead of or behind the communication process,
           which is important for maintaining a responsive service in
           interactive applications.
      */
    KDU_AUX_EXPORT kdu_long
      sync_timing(int queue_id, kdu_long app_time_usecs,
                  bool expect_preemptive);
      /* [SYNOPSIS]
           This function plays two important roles when working with timed
           requests -- requests posted to `post_window' with a non-zero
           `service_usecs' argument.  First, it notifies the
           client of the current time (in microseconds), as measured by
           the clock that the application is using to determine the
           `service_usecs' values that it is passing to the
           client.  The function compares the running rate of this clock
           with that of its own internal clock to determine whether it
           should make small adjustments to the service times that
           are passed to `post_window'.  The intent is to honour an agreement
           with the application that the cumulative service times
           associated with a timed request sequence should correspond to the
           actual cumulative time that is devoted to serving those requests,
           measured in the time base of the application's clock.  The
           application's clock is not required to run at exactly the same
           rate as "real time", but it is expected to have a similar running
           rate -- only minor corrections are made internally, to lock
           the internal and external time bases.
           [//]
           Note that each request queue separately keeps track of the
           relationship between the application's clock and its own internal
           clock.  This means that the application may use clocks which run
           at different rates with each request queue, although it is not
           obvious why it would want to do this.
           [//]
           The second purpose of the function is to return the amount of
           time, again measured in microseconds, that is currently
           expected to transpire before any sequence of timed requests
           that have been posted have received the cumulative service
           time requested (as supplied via `post_window's `service_usecs'
           argument).  If the most recent request was not a timed request,
           or if the `expect_preemptive' argument is true, the return value
           represents an estimate of the number of microseconds that will
           ellapse between a pre-emptive timed request that is posted
           immediately and receipt of the first byte of response data
           for that request.
         [RETURNS]
           It is important to realize that the function's return values may
           initially be only very approximate (even a guess) until some
           return data arrives from the server in response to timed
           requests.  However, it is expected that the function be called
           regularly, so that these guesses gradually become more
           reliable.
           [//]
           It is worth noting that legitimate return values may be negative.
           This happens if the application has not posted its requests in
           a timely fashion so that the point at which the next request
           should have been posted has already lapsed.  This is not a
           disaster, but it means that the next posted request in a timed
           sequence may be foreshortened or even discarded, as explained
           in the extensive discussion of timed requests found in the
           documentation of `post_window'.
           [//]
           If the queue does not exist or is not alive, a negative value
           with very large absolute value will be returned.
      */
    KDU_AUX_EXPORT kdu_long
      get_timed_request_horizon(int queue_id, bool expect_preemptive); 
      /* [SYNOPSIS]
           This function is used in conjunction with calls to
           `post_window' that specify a non-zero `service_usecs'
           argument.  It helps the application to avoid posting more
           window of interest requests than are necessary to keep the
           request queue primed.
           [//]
           Typically, the application would call this function each time it
           receives notification of the arrival of new data in the client's
           cache via its `kdu_client_notifier' object, to determine whether
           or not additional window of interest requests should be posted
           using `post_window'.
         [RETURNS]
           Returns a threshold on the cumulative number of microseconds
           associated with `service_usecs' values passed to new
           requests (not yet posted) beyond which there is no need for
           additional requests to be posted at the current time to keep the
           request queue primed.
           [//]
           The return value may be 0 or even negative, in which case there
           is no need for any new requests to be posted at the current time.
           [//]
           In the event that the identified request queue does not exist,
           or is not alive, the function is guaranteed to return a -ve value.
         [ARG: expect_preemptive]
           If true, the caller is expecting its next call to `post_window'
           to be pre-emptive -- i.e., the first call in a sequence of
           timed requests.  This makes a difference, because a pre-emptive
           call will cancel any outstanding requests that may already
           be in the pipeline, which may increase the estimated request
           horizon.
      */
    KDU_AUX_EXPORT kdu_long
      trim_timed_requests(int queue_id, kdu_long &custom_id,
                          bool &partially_sent);
      /* [SYNOPSIS]
           This function is intended to help improve the responsiveness
           of interactive applications that are using timed requests.
           As discussed in connection with `post_window', the mechanism for
           supporting video and animated content with JPIP is to post a
           sequence of timed window of interest requests -- i.e.,
           `post_window' calls with a non-zero `service_usecs' argument.
           All but the first request in such a sequence is non-preemptive;
           moreover, to keep the communication channel responsive, the client
           breaks long requests down into smaller increments and tries to
           avoid issuing these incremental requests to the server any
           earlier than necessary, subject to full utilization of the
           available communication bandwidth.
           [//]
           The purpose of this function is to clear the client's internal
           queue of as-yet unissued requests, without actually interrupting
           any timed request sequence -- the caller can then append new
           timed requests to the sequence.  This is useful if an interactive
           user changes the playback rate, the region of interest, the
           resolution of interest, and so forth, since it allows the request
           queue to be modified to reflect the new content of interest,
           without incurring the unnecessary delay of waiting for enqueued
           timed requests to complete.
           [//]
           A similar objective could be accomplished by starting a new
           sequence of timed requests, with a pre-emptive call to
           `post_window'.  However, pre-empting existing requests introduces
           both timing uncertainty (depends on whether the server
           pre-empts its own response stream) and content uncertainty
           (cannot be sure which video/animation frames' content was
           in flight prior to pre-emption).  The present function avoids
           these uncertainties by: 1) removing only those requests that
           have not yet been sent to the server; and 2) providing information
           regarding the cumulative amount of requested service time that
           is represented by the removed requests, so that the caller
           can back up to the point that corresponds to the first unissued
           request and post new requests onto the timed request sequence,
           starting from that point.
         [RETURNS]
           Returns the cumulative service time (measured in microseconds)
           that is being reclaimed by this call.  The effect is equivalent
           to having posted timed requests whose `service_usecs' values
           (as supplied to `post_window') were smaller in total by the
           returned amount.  The return value might not necessarily agree
           with the `service_usecs' values actually supplied for the
           discarded requests, but it does represent the actual amount
           by which the cumulative requested service time is decreased
           and the cumulative requested service time is the subject
           of the client's cumulative service guarantee for timed request
           sequences, as discussed with `post_window'.
           [//]
           Typically, a video/animation application will choose
           `service_usecs' values in its calls to `post_window' such that
           the cumulative sum of these `service_usecs' values grows at
           exactly the same rate as the display end points for the
           relevant animation frames.  Specifically, let E_k denote the
           time at which display of frame k is supposed to end and let
           C_k denote the cumulative `service_usecs' value associated with
           requests for frames up to and including frame k.  Then, the
           requested service time for frame k is C_k - C_{k-1} and this
           should typically be identical to E_k - E_{k-1}, except that
           the first few frames may be assigned a reduced amount of service
           to account for communication delay.  After executing the current
           function, suppose that the value returned via `custom_id'
           corresponds to a request for frame j.  Then the next request
           posted via `post_window' should be for frame j.  Suppose the
           return value is R.  Then the application should adjust its
           accumulators so that C_{j-1} = C_k-R (where k was the last frame
           for which a request was posted prior to this call) and
           E_{j-1} = E_k-R.  After making these adjustments, the application
           can post a request for frame j that has service time
           E_j - E_{j-1} = E_j - (E_k-R).  That way, the gap between C_k
           and E_k will remain constant into the future.  In an ideal world,
           the return value R is identical to the separation between original
           frame end times E_k and E_{j-1}.  In practice, however, the
           return value may be quite different due to internal
           compensation for prematurely terminated requests, channel idle
           time and the impact of other request queues that might be
           sharing the physical JPIP channel.  As a result, the value
           E_j - (E_k-R) may even turn out to be <= 0, so the application
           might choose to skip that frame and move on.
           [//]
           For further guidance on the use of this function, the reader
           is recommended to review the implementation of
           `kdu_region_animator::generate_imagery_requests', which
           follows the strategy outlined above, except that it also
           aggregates multiple animation frames into composite requests,
           to the extent that this is possible and the aggregated
           "super frame" has a sufficiently short duration.
           [//]
           Returns -1 if the `queue_id' argument does not correspond to a
           valid live request queue.
           [//]
           Returns 0 if no requests are discarded, which invariably happens
           if the request queue is not in timed request mode -- i.e., if
           the last posted window of interest did not have a service time.
         [ARG: queue_id]
           Identifies the request queue whose unissued requests are to
           be discarded.  This is one of the request queue identifiers
           returned by the `connect' or `add_queue' functions.  If the
           request queue identifier is invalid or the relevant queue is no
           longer alive, the function returns -1.
         [ARG: custom_id]
           Used to return the custom id value of the earliest posted
           window of interest for which anything was discarded.  This
           corresponds to the value supplied as the `custom_id' argument
           in the relevant call to `post_window'.  If the function returns
           a value that is less than or equal to 0, `custom_id' is not
           modified.
           [//]
           Typically, the application would identify the animation frames
           belonging to the window of interest corresponding to `custom_id'
           and issue new calls to `post_window' starting from those frames.
         [ARG: partially_sent]
           It is possible that the window of interest identified via
           `custom_id' was internally decomposed into a sequence of requests,
           some of which may have been issued by the server, so that only some
           portion of the requested service time is being discarded here.
           In fact, this is exactly what we expect to happen if `post_window'
           calls supply large `service_usecs' values.  This argument is
           used to indicate whether or not at least one request associated
           with the window of interest with `custom_id' has been sent to
           the server already.  As with `custom_id', the value of this
           argument is not set unless the function returns a positive value.
      */
    KDU_AUX_EXPORT virtual bool
      get_window_in_progress(kdu_window *window, int queue_id=0,
                             int *status_flags=NULL, kdu_long *custom_id=NULL);
      /* [SYNOPSIS]
           This function may be used to learn about the window
           which the server is currently servicing within a given request
           queue, as identified by the `queue_id' argument.  To be specific,
           the current service window is interpreted as the window
           associated with the most recent JPIP request to which the
           server has sent a reply paragraph -- the actual response data may
           take considerably longer to arrive, but the reply paragraph informs
           the present object of the server's intent to respond to the
           window, along with any dimensional changes the server has made
           within its discretion.
           [//]
           If the request queue is currently idle, meaning that the server
           has finished serving all outstanding requests for the queue,
           the present function will continue to identify the most
           recently serviced window as the one which is in progress, since
           it is still the most recent window associated with a server reply.
           [//]
           If the indicated request queue is not alive (i.e., if `is_alive'
           would return false when invoked with the same `queue_id' value), or
           if the request queue has been disconnected using `disconnect', the
           function returns false after invoking `window->init' on any
           supplied `window'.
           [//]
           Finally, if no reply has yet been received by the server, there
           is considered to be no current service window and so this
           function also returns false after invoking `window->init' on any
           supplied `window'.
         [RETURNS]
           True if the current service window on the indicated request queue
           corresponds to the window which was most recently requested via a
           call to `post_window' which returned  true.  Also returns true
           if no successful call to `post_window' has yet been generated, but
           the client has received a reply paragraph to a request which it
           synthesized internally for this request queue (based on the
           parameters passed to `connect').  Otherwise, the
           function returns false, meaning that the server has not yet
           finished serving a previous window request on this queue, or a
           new request message has yet to be sent to the server.
         [ARG: window]
           This argument may be NULL, in which case the caller is interested
           only in the function's return value and/or `status_flags'.
           If non-NULL, the function modifies the various members of this
           object to indicate the current service window.
           [//]
           Note that the `window->resolution' member will be set to reflect
           the dimensions of the image resolution which is currently being
           served.  These can, and probably should, be used in posting new
           window requests at the same image resolution.
           [//]
           If there is no current service window, because no server reply
           has ever been received for requests delivered from this request
           queue, or because the queue is no longer alive, the `window'
           object will be set to its initialized state (i.e., the function
           automatically invokes `window->init').
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions.  If the request queue identifier is
           invalid or the relevant queue is no longer alive, the
           function returns false after invoking `window->init' on any
           supplied `window'.
         [ARG: status_flags]
           You can use this argument to receive more detailed information
           about the status of the request associated with the window for
           which this function is returning information.  The defined flags
           are as follows:
           [>>] `KDU_CLIENT_WINDOW_IS_MOST_RECENT' -- this flag is set if
                and only if the function is returning true.
           [>>] `KDU_CLIENT_WINDOW_RESPONSE_STARTED' -- this flag is set if
                at least one byte of response data has been received for
                the request associated with this window, or the response is
                complete.
           [>>] `KDU_CLIENT_WINDOW_RESPONSE_TERMINATED' -- this flag is set
                if the server has finished responding to the request
                associated with this window and there are no internal
                duplicates of the request which have been delivered or are
                waiting to be delivered (internal duplicates are created
                to implement the client's flow control algorithm or to
                synthesize virtual JPIP channels by interleaving requests
                over a real JPIP channel).
           [>>] `KDU_CLIENT_WINDOW_IS_COMPLETE' -- this flag is set if the
                server has finished responding to the request associated
                with this window and the response data renders the client's
                knowledge of the source complete, with respect to the
                requested elements.  There is no need to post any further
                requests with the same parameters, although doing so will
                cause no harm.  Note that an initial call to this function
                might return the `KDU_CLIENT_WINDOW_RESPONSE_TERMINATED' flag
                without the `KDU_CLIENT_WINDOW_IS_COMPLETE' flag, while a
                subsequent call to the function might report that exactly
                the same request is now complete by setting the
                `KDU_CLIENT_WINDOW_IS_COMPLETE' flag.  This is because there
                may be other request queues, with overlapping requests in
                progress, and until the response data from all such requests
                has been received (possibly on separate JPIP channels), we
                cannot be sure that we actually have the complete response to
                this request.
         [ARG: custom_id]
           If non-NULL, this argument is used to return the value of the
           `custom_id' that was passed in the relevant call to `post_window'.
      */
    KDU_AUX_EXPORT bool
      get_window_info(int queue_id, int &status_flags, kdu_long &custom_id,
                      kdu_window *window=NULL, kdu_long *service_usecs=NULL);
      /* [SYNOPSIS]
           This function is similar to `get_window_in_progress' but may be
           used to learn about the status of a broader set of successfully
           posted windows or to find the most recently posted window with
           a particular status.  The utility of the `status_flags' argument
           here is greater than in `get_window_in_progress' and the function
           can also provide information about the amount of time for which
           response data has been arriving -- the service time.
           [//]
           The `status_flags' and `custom_id' values both have significance
           on entry to the function, and both are potentially modified by
           the function, unless the return value is false.  If `status_flags'
           is -ve on entry, the entry value of `custom_id' determines the
           posted window for which the function is being asked to return
           information.  Otherwise, the function is being asked to return
           information about the most recently posted window for which any
           of the supplied flags is valid; for more on this, see the
           description of `status_flags'.
           [//]
           Windows that have been successfully posted take various states
           within their queue, based on information received
           from or sent to the server.  Specifically, a posted window can be
           in one of the following states:
           [>>] pending request -- no request has yet been sent to the server.
           [>>] pending reply -- no reply to an issued request yet.
           [>>] replied -- server reply paragraph has been received.
           [>>] response started -- at least some response data has been
                received (or else the response has been terminated).
           [>>] response terminated -- all response data received (usually
                identified by the server's EOR message), although the request
                might have been pre-empted by a later one.
           [//]
           If two consecutive successfully posted windows have identical
           windows of interest, but distinct `custom_id' values, they
           are both assigned places in the request queue -- so long as the
           second one at least is non-preemptive.
         [RETURNS]
           The function returns false if no posted window can be
           found, that matches the information provided via `status_flags'
           and `custom_id'.  This may happen if the request queue is no
           longer alive (see `is_alive'), if the window was never posted,
           if the window was removed from the queue due to the appearance of
           a later pre-emptive request, or if all communication for the window
           has finished (terminated state) and there is at least one later
           window for which a reply has been received from the server, so
           that the terminated window drops off the end of the queue.
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions.  If the request queue identifier is
           invalid or the relevant queue is no longer alive, the
           function returns false.
         [ARG: status_flags]
           This argument has a related interpretation to its namesake in
           `get_window_in_progress', but there is an expanded set of possible
           flags.  However, this argument is used both to constrain the
           request (entry value) and, if the function returns true, to learn
           about the status of the relevant posted window (exit value).  The
           defined flags have the following interpretation:
           [>>] `KDU_CLIENT_WINDOW_UNREQUESTED' -- this flag means that
                the relevant posted window is still pending the issuance of
                a request to the server.
           [>>] `KDU_CLIENT_WINDOW_UNREPLIED' -- this flag means that
                the relevant posted window is still pending the receipt of a
                server reply paragraph.  Unrequested windows are of course
                also unreplied.
           [>>] `KDU_CLIENT_WINDOW_IS_MOST_RECENT' -- this flag means that
                the relevant posted window is the most recent one to be
                requested from the server.
           [>>] `KDU_CLIENT_WINDOW_RESPONSE_STARTED' -- this flag means that
                at least one byte of response data has been received for
                the relevant posted window, or the response is complete.
           [>>] `KDU_CLIENT_WINDOW_RESPONSE_TERMINATED' -- this flag means
                that the server has finished responding to the request
                associated with the relevant posted window and there are no
                internal duplicates of the request which have been delivered
                or are waiting to be delivered (internal duplicates are
                created to implement the client's flow control algorithm or
                to synthesize virtual JPIP channels by interleaving requests
                over a real JPIP channel).  Windows for which this the
                response is terminated of course also have the
                response-started status.
           [>>] `KDU_CLIENT_WINDOW_IS_COMPLETE' -- this flag is set if the
                server has finished responding to the request associated
                with this window and the response data renders the client's
                knowledge of the source complete, with respect to the
                requested elements.  There is no need to post any further
                requests with the same parameters, although doing so will
                cause no harm.
           [//]
           If `status_flags' is -ve on entry, the function is being asked to
           return information about the most recently requested window that
           was posted using the `custom_id' values provided by that argument.
           [//]
           Otherwise, the value of `custom_id' on entry is ignored and the
           function is being asked to return information for the most recently
           requested window for which at least one of the supplied status
           flags will be set on exit.
           [//]
           On exit, `status_flags' is modified to reflect the status of the
           relevant posted window, unless the function returns false.
         [ARG: custom_id]
           Used to return (and possibly specify) the custom-id value associated
           with the relevant posted window.  As mentioned above, if
           `status_flags' is 0 on entry, the function looks for the most
           recently posted request for which `custom_id' was supplied to the
           `post_window' function.  Otherwise, this argument is used only to
           return the posted window's `custom_id'.  The value of this argument
           is not modified unless the function returns true and `status_flags'
           was negative on entry.
         [ARG: window]
           This argument may be NULL, in which case the caller is not
           interested in recovering details of the window of interest
           associated with `custom_id'.  If non-NULL, the information
           retrieved by this argument might differ from that passed to
           `post_window' if a reply has already been received from the
           server -- this is explained with `get_window_in_process'.
           [//]
           If the function returns false, it does not modify the contents
           of any `window' object passed here -- note that this behaviour
           is different from that of `get_window_in_progress'.
         [ARG: service_usecs]
           If non-NULL, this argument is used to return the total number of
           microseconds that have ellapsed from the point at which the first
           response data arrived from the server for the relevant posted window
           until the current time, or the point at which the response data for
           the window request was terminated, whichever comes first.  If no
           response data has been received, the value is set to 0.  If any
           response data has been received, the value is set to a positive
           quantity (i.e., at least 1 microsecond, even if the amount of
           service time is strictly less than 1 microsecond).
      */
    KDU_AUX_EXPORT virtual bool
      post_oob_window(const kdu_window *window, int caller_id=0,
                      bool preemptive=true);
      /* [SYNOPSIS]
           This function is conceptually similar to `post_window', but with
           the following very important differences:
           [>>] Requests for this window of interest are associated with a
                special internal "OOB queue" (read OOB as "Out-Of-Band") that
                is shared by all calls to `post_oob_window'.
           [>>] Within the internal OOB queue, requests are recorded along
                with the `caller_id' values supplied the corresponding call
                to this function.  If `preemptive' is true, any earlier
                outstanding request with the same `caller_id' is pre-empted;
                however, no window request posted using this function will
                pre-empt one with a different `caller_id'.  For this reason,
                you are expected to post only small requests that can most
                likely be answered quickly.  Typically, you would only post
                urgent metadata-only window requests via this function.
           [>>] If possible, the object tries to associate the internal
                OOB queue with its own JPIP channel so that its requests
                can be handled by the server concurrently with requests
                posted via `post_window'.  However, if the server does not
                offer sufficient JPIP channels, the internal implementation
                schedules requests from the OOB queue onto the JPIP
                channel which is likely to give the fastest response -- in
                practice, this is any JPIP channel which is idle or, failing
                that, the one whose last request was sent least recently.
                Again, one should bear in mind that OOB requests will not
                be pre-empted by any requests other than OOB requests
                with the same `caller_id', so a OOB request may interrupt
                the regular flow of responses on some shared JPIP channel.
                For these reasons, you are expected to post only small
                requests that can most likely be answered quickly.
           [//]
           The principle motivation for this function is the need to
           obtain fast responses to small metadata requests that are
           generated when an interactive user clicks on some item of
           metadata with a view to recovering its descendants.  Ideally,
           this should happen in a similar timeframe to that which would
           be exhibited by a typical web browsing application.  On the
           other hand progressive imagery delivery typically runs in epochs
           whose duration may be on the order of 1 second or even more.  The
           solution is to schedule these small metadata requests on their
           own JPIP channel, where one is available.
           [//]
           Note that the internal "OOB queue" is automatically created when
           this function is first invoked and it is automatically closed once
           all other queues have been closed via `disconnect' or `close'.
           [//]
           You can query the progress of a posted OOB window by calling the
           `get_oob_window_progress' function.
         [RETURNS]
           False if the call has no effect.  This may be the case if there
           is already a request on the internal OOB queue that has been
           fully answered and had a window of interest that contains the
           supplied `window'.  It may also be the case if there is already
           a containing request on the OOB queue which has not yet
           been fully answered but has the same `caller_id' and
           `preemptive' is false.  It may also be the case if there is
           a containing request on the OOB queue which is identical in
           every respect to the current one, including `caller_id',
           regardless of whether `preemptive' is true or not.
           [//]
           If the function returns true, the window is held in the
           internal OOB queue until a suitable request message can be sent
           to the server.  This message will eventually be sent, unless a new
           (pre-emptive) call to `post_oob_window' arrives in the mean time
           with the same `caller_id'.
         [ARG: caller_id]
           Arbitrary integer identifier that the caller can use to
           distinguish between different requests that may have
           been posted to the OOB queue.  If `preemptive' is true, there
           will never be more than one request with any given `caller_id'
           on the OOB queue at any given time.  Typically, this function
           is used to collect metadata in response to user interaction within
           some metadata browsing context (e.g., something like a web
           browser) and the `caller_id' would identify each distinct
           metadata browsing context (e.g., a browser window).
         [ARG: preemptive]
           If this argument is false, a request will be queued for the
           relevant window of interest.  It will not pre-empt the ongoing
           response to any previously posted window with the same or a
           different `caller_id'.  That said, if the OOB queue's requests
           need to be interleaved onto a JPIP channel that is shared with
           other request queues, requests on those queues are temporarily
           pre-empted and later re-issued once the OOB queue is empty.
           [//]
           If you call this function with `preemptive'=true, the new window
           will pre-empt any undelivered requests with the same `caller_id'
           and potentially pre-empt requests which have been sent to the
           server but for which the server has not yet generated the complete
           response, so long as the `caller_id' of such requests is the
           same.
      */
    KDU_AUX_EXPORT virtual bool
      get_oob_window_in_progress(kdu_window *window, int caller_id=0,
                                 int *status_flags=NULL);
      /* [SYNOPSIS]
           This function plays the same role as `get_window_in_progress',
           except that it retrieves information about windows of interest
           that were posted to the internal "OOB queue" via the
           `post_oob_window', with the same value for `caller_id'.
      */
    KDU_AUX_EXPORT virtual const char *get_status(int queue_id=0);
      /* [SYNOPSIS]
           Returns a pointer to a null-terminated character string which
           best describes the current state of the identified request queue.
           [//]
           Even if the `queue_id' does not correspond to a valid
           channel, or `connect' has not even been called yet, the
           function always returns a pointer to some valid string, which is
           a constant resource (i.e., a resource which will not be deleted,
           even if the value returned by this function changes).
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions.  If the queue identifier is
           invalid or the relevant request queue is no longer alive, the
           function returns an appropriate string to indicate that this is
           the case.
      */
    KDU_AUX_EXPORT virtual bool
      get_timing_info(int queue_id, double *request_rtt=NULL,
                      double *suggested_min_posting_interval=NULL);
      /* [SYNOPSIS]
           This function may be used to retrieve current timing information for
           a specific request queue.  At present, the function is able to
           return up to two pieces of information: the observed request round
           trip time; and a suggested lower bound on the interval between
           calls to `post_window' when issuing a sequence of "timed requests".
           These are explained further below.
         [RETURNS]
           True if `queue_id' is valid.  Otherwise, the function ignores
           the other arguments and returns false.
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect' or
           `add_queue' functions, or else a -ve value, which is interpreted as
           referring to the internal "OOB" request queue (see
           `post_oob_window').  If you specify any other value,
           the function returns false, ignoring the remaining arguments.
         [ARG: request_rtt]
           If non-NULL, the caller is requesting information about the
           average request round-trip-time.  This is the time taken between
           the point at which a request is issued to the server and the
           first JPIP response message is received by the client, assuming
           that the request is pre-emptive or the request queue is idle at
           the point when the request is issued -- it makes no sense to
           measure round trip times for requests that are required to wait
           for a previous request.
           [//]
       .   If the request queue in question is using a JPIP channel over which
           no JPIP messages have yet been received, the function sets
           the value of `request_rtt' to -1.0.  Otherwise, the function sets
           the value of `request_rtt' to the most up-to-date estimate for the
           queue in question, measured in seconds.
         [ARG: suggested_min_posting_interval]
           If non-NULL, the caller is requesting a suggested lower bound on
           the interval (measured in seconds) between calls to `post_window',
           assuming that the caller wants each such call to be associated
           with response data from the server.  The value returned via this
           argument is likely to be quite small if the underlying client-server
           communication involves a stateful session, since then the client
           is content to pipeline requests.  However, if communication
           is stateless (corresponding to an empty or "none" value for
           the `channel_transport' argument to `connect'), requests cannot
           be pipelined and the value returned by this argument will typically
           be some multiple of the `request_rtt'.
      */
    KDU_AUX_EXPORT virtual kdu_long
      get_received_bytes(int queue_id=-1, double *non_idle_seconds=NULL,
                         double *seconds_since_first_active=NULL);
      /* [SYNOPSIS]
           Returns the total number of bytes which have been received from
           the server, including all headers and other overhead information,
           in response to requests posted to the indicated request queue.
           [//]
           This function differs from `kdu_cache::get_transferred_bytes'
           in three respects: firstly, it can be used to query information
           about transfer over a specific request queue; second, it returns
           only the amount of data transferred by the server, not including
           additional bytes which may have been retrieved from a local cache;
           finally, it includes all the transfered overhead.
         [ARG: queue_id]
           One of the request queue identifiers returned by the `connect'
           or `add_queue' functions, or else a negative integer.  In the
           latter case, the function returns the total number of received
           bytes belonging to all request queues, including the internal
           "OOB queue" (see `post_oob_window'), since the `connect'
           function was last called.  If this argument is not negative and
           does not refer to an alive request queue (see `is_alive'), the
           function returns 0.  Thus, once all queues have been removed via
           the `disconnect' function, this function will return 0 for all
           non-negative `queue_id' values.
         [ARG: non_idle_seconds]
           If this argument is non-NULL, it is used to return the total number
           of seconds for which the indicated request queue has been
           non-idle, or (if `queue_id' < 0) the total number of seconds for
           which the aggregate of all request queues managed by the client
           over its lifetime has been non-idle (this includes the internal
           "OOB queue" mentioned above).  A request queue is non-idle
           between the point at which it issues a request and the point at
           which it becomes idle, as identified by the `is_idle' member
           function.  The client is non-idle between the point at which any
           request queue issues a request and the point at which all request
           queues become idle.
         [ARG: seconds_since_first_active]
           If non-NULL, this argument is used to return the total number of
           seconds since the indicated request queue first became active, or
           (if `queue_id' < 0) the total number of seconds since any request
           queue in the client first became active (including the internal
           "OOB queue" mentioned above).  A request queue becomes
           active when it first issues a request.  During the time spent
           resolving network addresses or attempting to connect TCP channels
           prior to issuing a request, the queue is not considered active.
      */
  private: // Startup function
    friend void _kd_start_client_thread_(void *);
    void thread_start(); // Called from the network thread startup procedure
  private: // Internal functions which are called only once per `connect' call
    void run(); // Called from `thread_start'
    void thread_cleanup(); // Called once `run' returns or if it never ran
  private: // Mutex locking/unlocking functions for use by management thread
    void acquire_management_lock(kdu_long &current_time);
      /* Note that this function always re-evaluates the `current_time' so
         that it can safely be called with an uninitialized `current_time'
         variable if required. */
    void release_management_lock()
      {
        if (!management_lock_acquired) return;
        management_lock_acquired = false; mutex.unlock();
      }
  private: // Helper functions used to manage primary channel life cycle
    kdc_primary *add_primary_channel(const char *host,
                                     kdu_uint16 default_port,
                                     bool host_is_proxy);
      /* Creates a new `kdc_primary' object, adding it to the list.  The new
         object's immediate server name (and optionally port number)
         are derived by parsing the `host' string.  The actual
         `kdc_tcp_channel' is not created at this point, nor is the IP
         address of the host resolved.  These happen when an attempt is made
         to use the channel. */
    void release_primary_channel(kdc_primary *primary);
      /* If there are any CID's using the primary channel when this function
         is called, they are automatically released as a first step. */
  private: // Helper functions used to manage CID (JPIP channel) life cycle
    kdc_cid *add_cid(kdc_primary *primary, const char *server_name,
                     const char *resource_name);
      /* Creates a new `kdc_cid' object, adding it to the list.  The new
         object will use the identified `primary' channel and its requests
         with use the identified `server_name' and `resource_name' strings. */
    void release_cid(kdc_cid *cid);
      /* Performs all relevant cleanup, including closure of channels.  If
         there are any request queues still using this CID, they are
         automatically released first. */
  private: // Helper functions used to manage request queue life cycle
    kdc_request_queue *add_request_queue(kdc_cid *cid);
      /* Creates a new request queue, adding it to the list.  The new queue
         will use the identified `cid' for its initial (and perhaps ongoing)
         communications.  The queue is set up with `just_started'=true, which
         will cause the first request to include `cnew' and any
         one-time-request components, as required.  The response to a `cnew'
         request may cause a new `kdc_cid' object to be created and the
         request queue to be bound to this new object behind the scenes.
         The new queue's identifier is obtained by using the current object's
         `next_request_queue_id' state variable. */
    void release_request_queue(kdc_request_queue *queue);
      /* If this is the last request queue associated with a CID, the CID is
         also released; this, in turn, may cause primary channels to be
         released as well.  Before returning, this function always signals
         the `disconnect_event' to wake an application thread which might be
         waiting on the release of a request queue. */
  private: // Cache file I/O -- future versions will move this into `kdu_cache'
    bool look_for_compatible_cache_file(kdu_long &current_time);
      /* Called when the `target_id' first becomes available, if a cache
         directory was supplied to `connect'.  The function explores various
         possible cache files, based on the one named in `cache_path', until
         it encounters one for which `read_cache_contents' returns 1, or
         decides that there is no compatible cache file.  It is not sufficient
         to invoke `read_cache_contents' on the initial `cache_path' alone,
         since there may be multiple cache files, corresponding to resources
         with the same name but different target-id's.  In any event, by the
         time this function exits, the `cache_path' member holds the pathname
         of the file into which the cache contents should be saved, when the
         client connection dies.
            Returns true if any cache file's contents were read into the
         cache.  If so, the caller may need to ensure that an extra request
         is added to the relevant request queue so that the server can be
         informed of cache contents.
            Note that this function is called from a context in which the
         management mutex is locked.  Internally, the mutex is unlocked and
         then relocked again to avoid suspending the application during
         a potentially lengthy search. */
    int read_cache_contents(const char *path, const char *target_id);
      /* Attempts to open the cache file whose name is found in `path' and
         read its contents into the cache.  Returns 1 if a file was found
         and read into the cache.  Returns -1 if a file was found, but the
         file has an incompatible `target_id'.  Returns 0 if the file was
         not found.  If the function encounters a file whose version number
         is incompatible with the current version, the function returns -2,
         without making any attempt to check the `target_id'.  The caller
         may wish to delete any such files.  If any error is encountered
         in the format of a file, the function automatically deletes it and
         returns 0. */
    bool save_cache_contents(const char *path, const char *target_id,
                             const char *host_name, const char *resource_name,
                             const char *target_name,
                             const char *sub_target_name);
      /* Saves the contents of the cache to the file whose name is found in
         `path', recording the `target_id' (for cache consistency validation),
         along with sufficient details to connect to the server and open the
         resource again -- these are summarized by the `server_name',
         `resource_name', `target_name' and `sub_target_name' strings.  Either
         or both of the last two may be NULL (for example, the `resource_name'
         might be sufficient to identify the target resource.
         Returns false if the cache file cannot be opened. */
  private: // Helper functions for recyclable resources
    friend struct kdc_request;
    kdc_request *alloc_request();
    void recycle_request(kdc_request *req);
    kdc_request_dependency *alloc_dependency();
    void recycle_dependencies(kdc_request_dependency *list);
    kdc_chunk_gap *alloc_chunk_gap();
    void recycle_chunk_gaps(kdc_chunk_gap *list);
  private: // Other helper functions
    void signal_status()
      { // Called when some status string changes
        if (notifier != NULL) notifier->notify();
      }
    int *get_scratch_ints(int len);
      /* Return a temporary array with space for at least `len' integers.
         The function generates an error if `len' is ridiculously large. */
    char *make_temp_string(const char *src, int max_copy_chars);
      /* Makes a temporary copy of the first `max_copy_chars' characters from
         `src', appending a null-terminator.  The copied string may be shorter
         than `max_copy_chars' if a null terminator is encountered first
         within `src'.  The array is allocated and managed internally and may
         be overwritten by a subsequent call to this function.  The function
         generates an error if the string to be copied is ridiculously
         long. */
    kdc_model_manager *add_model_manager(kdu_long codestream_id);
      /* Called whenever a code-stream main header data-bin becomes complete
         in the cache, this function adds a new manager for the server's
         cache model for that code-stream.  In stateful sessions, it is not
         necessary to create a new client model manager for a code-stream
         whose main header was completed during the current session, since
         the server has a properly synchronized cache model for such
         code-streams.  Returns a pointer to the relevant model manager. */
    bool signal_model_corrections(kdu_window &ref_window,
                                  kdcs_message_block &block);
      /* Called when generating a request message to be sent to the server.
         The purpose of this function is to identify elements from the
         client's cache which the server is not expected to know about and
         signal them using a `model' request field.  For stateless requests,
         the server cannot be expected to know anything about the client's
         cache.  For stateful sessions, the server cannot be expected to
         know about any information which the client received in a
         previous session, stored in a `.kjc' file.
         [//]
         The function is best called from within a try/catch construct to
         catch any exceptions generated from ill-constructed code-stream
         headers.
         [//]
         Returns true if any cache model information was written.  Returns
         false otherwise.  This information may be used to assist in
         determining whether or not the request/response pair is cacheable
         by intermediate HTTP proxies.
      */
    bool parse_query_string(char *query, kdc_request *req,
                            bool create_target_strings,
                            bool &contains_non_target_fields);
      /* Extracts all known JPIP request fields from `query', modifying it so
         that only only the unparsed request fields remain.
         If `req' is non-NULL, the function uses any parsed request fields
         which relate to the window of interest to set members of
         `req->window'.  The function also parses any "len" request fields
         to set the `req->byte_limit' field, if possible; indeed, it may or
         may not be able to parse other fields.  You are responsible for
         ensuring that `req->init()' has already been called.
         If `create_target_strings' is true, the function uses
         request fields which relate to the requested target (if found) to
         allocate and fill `target_name' and/or `sub_target_name' strings for
         the current object, as appropriate.  Otherwise, the function checks
         any encountered target or sub-target strings for compatibility with
         those found in the current object's `target_name' or
         `sub_target_name' members, as appropriate.
         [//]
         The `contains_non_target_fields' argument is set to indicate whether
         or not the `query' string was found to contain any request fields
         other than those which identify the target or sub-target.  If so,
         the query string should be considered a one-time request, but the
         function does not itself set the `non_interactive' member of the
         current object.  Unrecognized query fields which are left behind in
         the `query' string are of course also considered to be non-target
         request fields.
         [//]
         The function returns true if all request fields could be parsed and
         (if `create_target_strings' is false) there was no mismatch detected
         between target fields encountered in the `query' string and the
         current object's `target_name' and `sub_target_name' strings.
       */
    void obliterating_request_issued()
      { obliterating_requests_in_flight++; }
    void obliterating_request_replied();
      /* This function decrements the `obliterating_requests_in_flight'
         variable.  Once this variable reaches zero, newly issued requests
         can be trusted again.  For this reason, when it does reach zero, the
         function examines all request queues whose final request was marked
         as untrusted yet was not timed (i.e., `service_time' is
         zero) and duplicates that request. */
  private: // Synchronization and other shared resources
    friend struct kdc_request_queue;
    friend class kdc_cid;
    friend class kdc_primary;
    kdu_thread thread; // Separate network management thread
    kdu_mutex mutex; // For guarding access by app & network management thread
    bool management_lock_acquired; // Used by `acquire_management_lock'
    kdu_event disconnect_event; // Signalled when a request queue is released
    kdcs_timer *timer; // Lasts the entire life of the `kdu_client'
    kdcs_channel_monitor *monitor; // Lasts the entire life of the `kdu_client'
    kdu_client_notifier *notifier; // Reference to application-supplied object
    kdu_client_translator *context_translator; // Ref to app-supplied object
  private: // Configurable timeouts
    kdu_long primary_connection_timeout_usecs;
    kdu_long aux_connection_timeout_usecs;
  private: // Members relevant to all request queues
    char *host_name; // Copy of the `server' string supplied to `connect' --
        // this string retains any hex-hex encoding from its supplier.
    char *resource_name; // Resource part of `request' supplied to `connect' --
        // this string retains any hex-hex encoding from its supplier.
    char *target_name; // NULL, or else derived from the query part of the
        // `request' supplied to connect; retains original hex-hex encoding.
    char *sub_target_name; // NULL, else derived from query part of `request'
        // supplied to connect; retains any (highly improbably) hex-hex coding
    char *processed_target_name; // Returned by `get_target_name' -- this name
        // has been hex-hex decoded from `resource_name' or target/sub-target.
    const char *one_time_query; // Points into `resource_name' buffer -- holds
        // original request fields which could not be interpreted; retains any
        // original hex-hex encoding.
    char *cache_path; // For `read_cache_contents' & `save_cache_contents'
    char target_id[256]; // Empty string until we know the target-id
    char requested_transport[41]; // The `cnew' request made for each channel
    bool initial_connection_window_non_empty; // Set inside `connect'
    bool check_for_local_cache; // False once we have tried to load cache file
    bool is_stateless; // True until a session is granted
    bool target_request_successful; // See the `target_started' function
    bool active_state; // True from `connect' until `close'
    bool non_interactive; // True from `connect' until `close' if appropriate
    bool image_done; // If the server has completely served the entire target
    bool close_requested; // Set by `close'; informs network management thread
    bool session_limit_reached; // If JPIP_EOR_SESSION_LIMIT_REACHED is found
    bool session_untrusted; // If data may have been lost in a way the server
               // does not know about and hence cannot react to in the future
    int obliterating_requests_in_flight; // See `kdc_request::obliterating'
  private: // Global statistics
    const char *final_status; // Status string used once all queues are closed
    kdu_long total_received_bytes; // Total bytes received from server
    kdu_long client_start_time_usecs; // Time client sent first request
    kdu_long last_start_time_usecs; // Time @ 1st request since all queues idle
    kdu_long active_usecs; // Total non-idle time, exlcuding any period since
                           // `last_start_time_usecs' became non-negative.
  private: // Communication state and resources
    kdc_request *free_requests; // List of recycled request structures
    kdc_request_dependency *free_dependencies; // List of recycled dependencies
    kdc_chunk_gap *free_chunk_gaps; // List of recycled chunk gaps
    kdc_primary *primary_channels; // List of primary communication channels
    kdc_cid *cids; // List of JPIP Channel-ID's and associated channel state
    kdc_request_queue *request_queues; // List of all alive request queues
    int next_request_queue_id; // Used to make sure request queues are unique
    kdc_model_manager *model_managers; // Active codestream cache managers
    kdu_long next_disconnect_usecs; // -ve if no queue waiting to disconnect
    bool have_queues_ready_to_close; // So `run' doesn't always have to check
  private: // Temporary resources
    int max_scratch_chars; // Includes space for any null-terminator
    char *scratch_chars;
    int max_scratch_ints;
    int *scratch_ints;
  };
  /* Implementation Notes:
        The implementation of this object involves a network management
     thread.  Most of the private helper functions are invoked only from
     within the network managment thread, while the application's thread (or
     threads) invoke calls only to the public member functions, such as
     `connect', `add_queue', `disconnect', `post_window',
     `get_window_in_progress', `is_active', `is_alive' and `is_idle'.  Calls
     to these application API functions generally hold a lock on the `mutex'
     while they are in progress, to avoid unexpected state changes.  However,
     to avoid delaying the application, the network management thread releases
     its lock on the mutex during operations which might take a while, such
     as waiting on network events and resolving network addresses.  In
     order to ensure that the context in which the network management thread
     is not accidentally corrupted by the application while the mutex is
     unlocked, the implementation of application API function must be
     careful never to release/delete any elements from lists.  In particular,
     no application call may result in the removal of an active primary
     channel and active request (one which is not on the `first_unrequested'
     list) or any request queue. */


#endif // KDU_CLIENT_H
