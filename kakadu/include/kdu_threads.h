/*****************************************************************************/
// File: kdu_threads.h [scope = CORESYS/COMMON]
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
   This header defines the core multi-threaded processing architecture used
by Kakadu.  Multi-processing is not required to build the Kakadu core system,
but these definitions allow you to exploit the availability of multiple
physical processors without having to worry about any of the details.
   The objects defined here are implemented in "kdu_threads.cpp".
******************************************************************************/

#ifndef KDU_THREADS_H
#define KDU_THREADS_H

#include <assert.h>
#include <string.h>
#include "kdu_elementary.h"
#include "kdu_arch.h"

// Objects defined here
class kdu_thread_entity_condition;
class kdu_thread_entity;
class kdu_thread_dependency_monitor;
class kdu_thread_queue;
class kdu_thread_job;
class kdu_run_queue;
class kdu_run_queue_job;
class kdu_thread_context;
struct kd_thread_lock;
struct kd_thread_grouperr;

// Objects defined elsewhere
struct kd_thread_job_hzp;
struct kd_thread_palette;
struct kd_thread_palette_ref;
struct kd_thread_group;
struct kd_thread_domain;
struct kd_thread_domain_sequence;

/*===========================================================================*/
/*       Design Constants for the Kakadu Multi-Threading Architecture        */
/*===========================================================================*/

/*****************************************************************************/
/*                    KDU_MAX_THREADS and KDU_MAX_DOMAINS                    */
/*****************************************************************************/

/* Notes:
      Define max threads in a group so that we can atomically manipulate
   bit masks with one bit for each thread -- if really necessary, we could
   change this in the future, but systems with a large number of CPU's
   probably should be running a 64-bit OS anyway.  The maximum number
   of domains is not constrained in the same way as the maximum number of
   threads, but there is no harm in allowing for one domain per thread.
   [//]
   The values defined below are the maximum values supported by the
   implementation.  There are some places in the Kakadu core system when
   the cost of resources is proportional to the value of `KDU_MAX_THREADS'.
   The most significant example of this is the allocation of an array of
   1+KDU_MAX_THREADS `kd_compressed_stats' object in `kdu_codestream' objects
   that employ rate control during codestream generation.  This can get
   quite expensive, so a lightweight implementation might consider using
   smaller values for `KDU_MAX_THREADS', noting that the values must be exact
   powers of 2 and should probably be no smaller than 8.
*/
#ifndef KDU_THREADS
#  define KDU_LOG2_MAX_THREADS 0
#else
#  ifdef KDU_HAVE_INTERLOCKED_INT64
#    define KDU_LOG2_MAX_THREADS 6
#  else
#    define KDU_LOG2_MAX_THREADS 5
#  endif // !KDU_POINTERS64
#endif

#  define KDU_MAX_THREADS (1<<KDU_LOG2_MAX_THREADS)
#  define KDU_MAX_DOMAINS KDU_MAX_THREADS
#  define KD_THREAD_PALETTES (2*KDU_MAX_DOMAINS+2*KDU_MAX_THREADS)

/*****************************************************************************/
/*                         Other Design Constants                            */
/*****************************************************************************/

/* Notes:
     Each thread essentially builds a stack of conditions that can be
     signalled, so that while the thread is waiting on a condition, it
     pushes the condition onto the stack and moves on to a new processing
     context where it can continue to do work, possibly waiting on further
     conditions in a recursive fashion.  The condition stack is not kept
     on the thread's execution stack, because that may leave signalling
     threads in a vulnerable position if the execution stack is unwound
     to handle an exception.  Instead, the condition stack is created from
     a fixed array of "initial" thread conditions that is part of the
     `kdu_thread_entity' object itself.  If more conditions are required,
     they are allocated from the heap.  The constant below provides more
     conditions than are likely to be required in most cases, so that
     heap allocation should be rare. */

#define KDU_INITIAL_THREAD_CONDITIONS 8

/*****************************************************************************/
/*                                 Flags                                     */
/*****************************************************************************/

#define KDU_THREAD_QUEUE_BACKGROUND ((int) 1)
  /* [SYNOPSIS]
       Flag that may be passed to `kdu_thread_entity::attach_queue'.
  */

#define KDU_THREAD_QUEUE_SAFE_CONTEXT ((int) 2)
  /* [SYNOPSIS]
       Flag that may be passed to `kdu_thread_entity::attach_queue'.
  */

#define KDU_THREAD_JOB_AUTO_BIND_ONCE ((int) 1)
  /* [SYNOPSIS]
       Flag that may be passed to `kdu_thread_queue::schedule_job'.
  */

#define KDU_THREAD_JOB_REBIND_0 ((int) 2)
  /* [SYNOPSIS]
       Flag that may be passed to `kdu_thread_queue::schedule_job'.
  */



/*===========================================================================*/
/*                           Class Definitions                               */
/*===========================================================================*/

/*****************************************************************************/
/*                      kdu_thread_entity_condition                          */
/*****************************************************************************/

class kdu_thread_entity_condition {
  /* [BIND: reference]
     [SYNOPSIS]
       References to objects of this class are used to wait upon
       conditions while doing work in the background -- generally better
       than just waiting upon a synchronization object like
       `kdu_event'.  The relevant functions are
       `kdu_thread_entity::get_condition',
       `kdu_thread_entity::wait_for_condition' and
       `kdu_thread_entity::signal_condition'.
  */
  private: // Everything is private!  Totally opaque to application.
    friend class kdu_thread_entity;
    bool signalled; // If condition has been signalled
    bool is_dynamic; // If `delete' is needed to free the object's memory
    int thread_idx; // Index of the thread to which the condition belongs
    const char *debug_text; // String passed to `wait_for_condition'
    kdu_thread_entity_condition *link; // See below
  };
  /* Notes:
       Each thread maintains two collections of conditions: 1) a list of
     free (recycled) conditions, connected via their `link' fields; and 2) a
     stack of active conditions, in which `link' points to the previous
     condition in the stack.  As discussed at the end of `kdu_thread_entity',
     the condition stack grows each time a thread enters its
     `kdu_thread_entity::process_jobs' function and shrinks each time the
     thread returns from that function.
  */

/*****************************************************************************/
/*                             kdu_thread_entity                             */
/*****************************************************************************/

class kdu_thread_entity {
  /* [BIND: reference]
     [SYNOPSIS]
       This object represents the state of a running thread.  Threads
       generally belong to groups of cooperating workers.  Each such group
       has an owner, whose `kdu_thread_entity' object was directly created
       using `create'.  The other working threads are all created using
       the owner's `add_thread' member.  When the owner is destroyed, the
       entire group is destroyed along with it -- after waiting for all
       workers to go idle.  When one thread in a group generates an error
       through `kdu_error', resulting in a call to `handle_exception', the
       remaining threads in the group will also throw exceptions when they
       next invoke any (or almost any) of the threading API functions
       defined here.  This helps ensure that the group behaves as a single
       working entity.  In each case, exception catching and rethrowing is
       restricted to exceptions of type `kdu_exception'.
       [//]
       All Kakadu's multi-threading support is designed to work correctly
       even if the group owner is the only thread in its group -- in that
       case, the group owner generally gets shunted around to do the various
       scheduled jobs and support for physical multi-threading support is
       not required (for example, everything should work even if Kakadu
       is compiled with `KDU_NO_THREADS' defined).
       [//]
       This object is designed to be sub-classed.  When you do so, your
       derived object will be used by every thread within the same working
       group.  This is achieved by correctly implementing the
       `new_instance' function in your derived class.
       [//]
       Kakadu version 7 involves a completely new multi-threading system
       to that found in earlier versions.  The two most important differences
       are: a) the new system involves distributed scheduling of jobs
       so that threads can continue working without ever entering a shared
       critical section; b) the new system provides a generic mechanism for
       keeping track of dependencies so that jobs need not be scheduled for
       execution until they are free to run uninhibited.  This prevents a lot
       of the needless blocking on shared resources that occurred in earlier
       versions of Kakadu.  There are plenty of other differences: for
       example, mutex locks and distributed memory management are now
       partitioned by codestream, where appropriate, so there are no
       efficiency losses incurred by using a single thread group to process
       multiple codestreams.
       [//]
       In order to support all these changes, the notion of a
       `kdu_thread_queue' has changed radically, being only superficially
       similar to its namesake in previous versions.  Moreover, the
       abstract base class `kdu_worker' has been replaced by a related yet
       quite different base class `kdu_thread_job'.  These are explained
       briefly below.
       [//]
       As in previous versions of Kakadu, thread queues are organized into
       a hierarchy.  New queues may be created as top-level queues or else
       as sub-queues of other queues.  Each queue still has the notion of
       "completion" and it is possible to wait for completion of a queue,
       together with all of its sub-queues.  There is no longer any such
       thing as a synchronized job, however.  Instead, applications
       generally derive from the `kdu_thread_queue' base class to implement
       and schedule jobs directly from within the derived implementation.  To
       facilitate this, a special `kdu_thread_queue::update_dependencies'
       function is provided, which allows jobs within other queues (even
       other domains) to notify a `kdu_thread_queue' object of the
       availability or removal of dependencies that may affect the runnability
       of jobs that it may choose to schedule.  This is a more flexible and
       powerful feature than the synchronized job of before.
       [//]
       One important concept that was not present in previous versions of
       Kakadu is the "work domain", or simply "domain".  All thread queues
       must be assigned to domains when they are added to a thread group
       using `kdu_thread_entity::add_queue'.  There is a default domain
       (one whose domain name is a NULL or empty string), but the only
       thread queues that can be assigned to the default domain are those
       that schedule no jobs.  When jobs are scheduled, they are appended
       to the domain to which their queues belong.  Within a domain, jobs
       are launched in the same order that they are scheduled -- although
       of course, this statement cannot be taken too strictly because
       scheduling and launching of jobs are all performed by asynchronous
       threads, so delays incurred mid-way through a scheduling or launching
       step might alter the apparent order.  Typically, all threads in a
       thread group belong to the default domain, which means that they
       have no preference for where they do work; these threads service
       domains in a round-robin fashion so long as work remains to be done.
       It is also possible to assign threads to a particular work domain
       when they are added via the `kdu_thread_entity::add_thread'
       function.  This does not prevent them from doing work in another
       domain, but it does mean that they will preferentially pull jobs
       from their assigned domain.
       [//]
       Another important concept, not present in previous versions of
       Kakadu, is "background domains".  Background domains are similar to
       other work domains, except that their jobs are either not urgent or
       may be subject to unbounded delays (e.g., I/O operations).  Background
       jobs are normally scheduled well ahead of the point when they are
       needed.  Whenever a thread is waiting for a condition that has not
       yet arrived, it processes any jobs that are available to run, so as
       to avoid yielding its executable context -- that can be expensive.
       The main reason for creating a special category for background jobs
       is to indicate that they should not be performed by these waiting
       threads -- the only exception is when all threads are waiting for
       some condition to occur, in which case any waiting thread may perform
       outstanding background jobs.  Background jobs are used to
       pre-parse codestream content during decompression and to flush
       codestream content during compression.  However, they may have other
       uses.  Background domains are identified as such when a background
       thread queue which can schedule jobs is first attached to the domain.
       This happens if `attach_queue' is passed the special
       `KDU_THREAD_QUEUE_BACKGROUND' flag along with a queue whose
       `kdu_thread_queue::get_max_jobs' function returns non-zero.
  */
  public: // Members
    KDU_EXPORT kdu_thread_entity();
      /* [SYNOPSIS]
           The constructor is not a completely safe place for meaningful
           construction.  For this reason, you must call `create' to
           render the constructed object functional.  Prior to this point,
           the `exists' function will return false.  After a call to
           `destroy', the `create' function can actually be called again.
      */
    virtual ~kdu_thread_entity();
      /* [SYNOPSIS]
           While the destructor will perform all required cleanup, the
           caller may be suspended for some time while waiting for worker
           threads to complete.  For this reason, you are encouraged to
           explicitly call the `destroy' function first.  This also provides
           you with information on whether all threads terminated normally
           or a failure was caught.
      */
    KDU_EXPORT void *operator new(size_t size);
      /* This function allocates the thread object in such a way that it
         occupies a whole number of L2 cache lines, thereby maximizing
         cache utilization efficiency. */
    KDU_EXPORT void operator delete(void *ptr);
      /* This function deletes the memory allocated using the custom new
         operator defined above. */
    virtual kdu_thread_entity *new_instance() { return new kdu_thread_entity; }
      /* [SYNOPSIS]
           The `add_thread' function uses this function to create new
           `kdu_thread_entity' objects for each new worker thread.  You
           can override this virtual function to create objects of your own
           derived class, thereby ensuring that all worker threads will
           also use the derived class.  This is particularly convenient if
           you want each thread in a group to manage additional
           thread-specific data.  By careful implementation of the function
           override, you can also arrange to inherit parameters from the
           thread which creates new workers.
      */
    bool exists() const
      { return (group != NULL); }
      /* [SYNOPSIS]
           Returns true only between calls to `create' and `destroy'.
           You should not use a `kdu_thread_entity' object until it has
           been created.
      */
    bool operator!() const
      { return !exists(); }
      /* [SYNOPSIS]
           Opposite of `exists'.
      */
    bool is_group_owner() const
      { return (group != NULL) && (thread_idx == 0); }
      /* [SYNOPSIS]
           As explained in the introduction to this object, each thread
           entity belongs to a group which has exactly one owner.  This
           function indicates whether or not this object is the owner for
           its group.  The `kdu_thread_entity' object passed into functions
           installed by `kdu_thread_job::set_job_func' may well not be the
           owner of its group.
      */
    int get_thread_id() const { return thread_idx; }
      /* [SYNOPSIS]
           Returns the identity of this `kdu_thread_entity' object.  The
           return value will be 0 if this is the group owner (see
           `is_group_owner') or the `create' function has not yet been
           called.  Otherwise, the object refers to a worker thread and will
           have an identity greater than 0.  The returned identity values
           lie in the range 0 to N-1, where N is the number of threads in
           the thread group -- see `get_num_threads'.
      */
    bool check_current_thread() const
      { 
        if (!exists()) return false;
        kdu_thread me; me.set_to_self();
        return thread.equals(me);
      }
      /* [SYNOPSIS]
           Returns true if the calling thread is the one that is associated
           with this object.  This function is mostly invoked internally
           (perhaps only in debug mode) in order to verify that certain
           functions which must be invoked by the object's own thread are
           being called correctly.
           [//]
           In general, only the thread that is associated with a given
           `kdu_thread_entity' object should be using it in any way.  It
           is not wise to leave a `kdu_thread_entity' reference lying
           around for another thread to use, even if it belongs to the
           same thread group.
      */
    bool change_group_owner_thread()
      { 
        if (!is_group_owner()) return false;
        thread.set_to_self();
        return true;
      }
      /* [SYNOPSIS]
           This function is provided for applications which manage their own
           threads in addition to those associated with a `kdu_thread_entity'
           object.  In some cases, one application-defined thread invokes
           `kdu_thread_entity::create', making it the thread group owner.
           However, another application thread would subsequently like to
           use the `kdu_thread_entity' object.  This is allowed only if you
           first call this function, changing the internal record of
           the thread that is understood to be the group owner.
           [//]
           Thereafter, functions that may be called only by the group owner
           must be invoked by the last thread to call this function.
           [//]
           You can change the group owner thread as often as you like, but
           it is your responsibility to make certain that the previous
           group owner thread does not invoke any of the object's functions
           thereafter.
           [//]
           Internal worker threads may not execute this function.
           [//]
           If you Kakadu's Java Native Interface methods, you must be sure
           that any thread which becomes the group owner is a thread that
           was either created by Java or has been explicitly attached to
           the Java VM -- otherwise, an exception may be thrown
           if the owner thread passes into a native call back function
           that is implemented in a Java-derived class.
         [RETURNS]
           False if this `kdu_thread_entity' object is not the one that
           belongs to the owner of its thread group -- i.e. if
           `is_group_owner' returns false.
      */
    KDU_EXPORT void create(kdu_long cpu_affinity=0,
                           bool also_set_owner_affinity=true);
      /* [SYNOPSIS]
           You must call this function after construction to prepare the
           thread entity for use.  Prior to this point `exists' will return
           false.  After creation, the present object will become the owner
           of a thread group, to which workers may be added using the
           `add_thread' function.
         [ARG: cpu_affinity]
           This argument can be used to associate the threads which
           collaborate in this group with one or more specific logical
           CPU's.  If the value is zero, threads can run on any CPU.
           Otherwise, the operating system is requested to schedule the
           threads in the group on a particular set of logical CPU's.
           [//]
           To understand the interpretation of the `cpu_affinity' argument,
           you should consult the comments that appear with the
           `kdu_thread::set_cpu_affinity' function, to which the `cpu_affinity'
           value is ultimately passed.  Usually, it is safe to assume that the
           `cpu_affinity' is a bitmask, with each non-zero bit corresponding
           to a logical CPU that the threads can run on, but you should
           read the more detailed comments in `kdu_thread::set_cpu_affinity'.
           [//]
           After this call, each new thread that is added to the group passes
           the `cpu_affinity' value to `kdu_thread::set_cpu_affinity' to
           set its own affinity as it starts up.
         [ARG: also_set_owner_affinity]
           If true, and `cpu_affinity' is non-zero, the `cpu_affinity'
           argument is also used to set the CPU affinity of the calling
           thread (the thread group owner).  This is the default.  However,
           it might make sense to treat the group owner differently,
           because the thread group owner usually does a particular set
           of tasks that other threads in the group might not be able to
           do (any tasks that are not scheduled as jobs).
           [//]
           Note that a thread can always directly set its own CPU affinity
           via the `kdu_thread::set_cpu_affinity' function.
      */
    KDU_EXPORT bool destroy();
      /* [SYNOPSIS]
           This function does nothing unless the object has been created.
           In that case, it returns it to the non-created state, allowing
           `create' to be called again if desired.  The function essentially
           calls `terminate' after first invoking `handle_exception' to
           force termination as quickly as possible; then it destroys all
           resources associated with the thread group.
           [//]
           Only the group owner may call `destroy'.
         [RETURNS]
           False if any thread in the group failed (through `kdu_error'),
           throwing an exception prior to the call to `destroy'.
      */
    KDU_EXPORT void set_min_thread_concurrency(int min_concurrency=0);
      /* [SYNOPSIS]
           The role of the thread concurrency may seem a little mysterious
           and perhaps it should.  In fact, this function is provided only
           to deal with operating systems (notably Solaris), which do not
           automatically create at least as many kernel threads as there are
           processors, thereby preventing the process from exploiting the
           availability of multiple physical CPU's through multiple threads.
           This problem is resolved internally (in a PTHREADS environment)
           by using `pthread_setconcurrency' to set the number of kernel
           threads (threads associated with distinct processors) equal to
           the total number of threads in the thread group (including the
           group owner) or the `min_concurrency' value, whichever is larger.
           [//]
           The default value for `min_concurrency' (i.e., 0) is fine for
           most applications, in which case there is no need to call this
           function.  However, when there are multiple distinct thread
           groups, they may compete with each other to set the
           thread concurrency level for the overall process.  You may avoid
           this problem by supplying a non-zero `min_concurrency' argument
           to this function.
           [//]
           For operating systems which do not pay any attention to
           `pthread_concurrency', it does not matter what value you supply
           for this argument and there will be no need ever to call this
           function, although it does not matter if you do.
      */
    KDU_EXPORT int get_num_threads(const char *domain_name=NULL);
      /* [SYNOPSIS]
           You may use this function to determine the total number of
           threads which are currently associated with the thread group or
           with a specific "domain" within the thread group.  Of course,
           this value is generally changed by calls to `add_thread'.
           [//]
           It is worth providing here a brief expansion on the idea of
           domains.  Domains are created automatically by calls to
           `add_thread', `attach_queue' or `add_queue', based on the
           `domain_name' supplied to those functions.  There is also a
           default domain that has no name.  The group owner belongs to this
           default domain and calls to `add_thread' that arrive with a NULL
           `domain_name' argument (or an empty string) add threads to the
           default domain.  Since domains may also be created by calls to
           `add_queue' and `attach_queue' some domains may have no threads
           associated with them.  This is actually quite normal.  The only
           reason for associating threads with a domain is to give them a
           preference for doing work in that domain -- nevertheless if a
           thread's domain has no jobs to do, it will pull jobs from
           any domain.  For more information on how threads do work in
           different domains, see the descriptions of `add_thread' and
           `attach_queue'.
           [//]
           If the `domain_name' argument supplied here is NULL, the function
           returns the total number of threads in the entire thread group.
           This includes the group owner itself, unless `create' has not
           yet been called, in which case the function returns 0.
           [//]
           If `domain_name' is an empty string, the function returns the
           number of threads assigned to the default domain only.  This
           is always at least one (group owner), unless the `create'
           function has not yet been called.
      */
    KDU_EXPORT bool add_thread(const char *domain_name=NULL);
      /* [SYNOPSIS]
           This function is used to add worker threads to the group owned
           by the current thread.  The caller, therefore, is usually the
           group owner, but this is not required.  The new theads are
           automatically launched with their own `kdu_thread_entity' objects.
           They will not terminate until `destroy' is called.
           [//]
           A reasonable policy is to add a total of P-1 workers, where
           P is the number of physical processors (CPU's) on the platform.
           The value of P can be difficult to recover consistently across
           different platforms, since POSIX deliberately (for some good
           reasons) refrains from providing a standard mechanism for
           discovering the number of processors across UNIX platforms.
           However, the `kdu_get_num_processors' function makes an attempt
           to discover the value, returning false if it cannot.
           [//]
           You can assign threads to specific domains, based on the
           `domain_name' argument.  If `domain_name' is NULL or points to
           an empty string, the new thread is added to the group's default
           domain (the one to which the group owner belongs).  If the
           `domain_name' string has not been seen before, a new
           multi-threaded domain will automatically be created here.
           [//]
           Threads added using this function are able to do work scheduled
           by queues within any domain, so mentioning a `domain_name' in
           this call only assigns the thread a preference to do work in
           the identified domain.
           [//]
           This function should not throw any exceptions.  If another thread
           in the thread group has already passed into `handle_exception',
           the current call will just return false immediately.
         [ARG: domain_name]
           If NULL or an empty string, the function is targeting the default
           domain for the thread group.  Otherwise, the function searches
           for an existing domain with the same name (full string compare, not
           just string address check), creating one if necessary, and adds
           a new thread to that domain.
         [RETURNS]
           False if insufficient resources are available to offer a new thread
           of execution, or if a multi-threading implementation for the
           present architecture does not exist in the definitions provided
           in "kdu_elementary.h".  You may wish to check them for your
           platform.
      */
    KDU_EXPORT bool
      declare_first_owner_wait_safe(bool is_safe);
      /* [SYNOPSIS]
           This function needs to be understood in conjunction with the
           `queue_flags' argument to `attach_queue'.  In brief, Kakadu's
           multi-threading sub-system has the notion a "working wait" state,
           where a thread calls into `wait_for_condition', `join' or
           `terminate' to wait for a condition, but while it is waiting the
           thread actually executes outstanding jobs that have been scheduled
           through any of the attached `kdu_thread_queue's.  When a queue is
           attached, the supplied `queue_flags' argument can be used to
           constrain the contexts in which its jobs are run.  In particular,
           some jobs should not be run by threads that are in a working wait
           state, because these jobs might wind up waiting on a condition
           that is blocked by the working wait state.
           [//]
           This function determines how the thread group owner (see
           `is_group_owner') thread should be treated when it first enters
           a working wait state.  Unlike worker threads, the group owner
           can only execute jobs from within the working wait state, so its
           resources may be underutilized if it cannot perform certain types
           of jobs.  Moreover, unlike worker threads, when the group owner
           first enters a working wait state, the point at which it does so
           is well defined by the program flow.  In many cases, therefore,
           it can be determined that the group owner's first entry to a
           working wait state is a safe context for running jobs that
           must otherwise be run only from non-waiting worker threads.
           [//]
           Specifically, passing `is_safe'=true to this function will cause
           the group owner to be elligible for running jobs that have been
           scheduled from within a queue attached with the
           `KDU_THREAD_QUEUE_SAFE_CONTEXT' flag, so long as the group owner
           has not recursively entered the working wait state.  That is,
           to be elligible to run jobs that require a safe context, the
           group owner may be inside `join', `terminate' or
           `wait_for_condition', so long as it has not entered these
           functions from within a job that it is already executing in a
           working wait state.
           [//]
           Before concluding this description, it is worth providing some
           concrete examples of where this function should or should not
           be used.  Kakadu's multi-threading sub-system is used extensively
           for parallel realization of block coding and DWT analysis/synthesis
           operations.  Usually, this is done by instantiating
           `kdu_multi_analysis' or `kdu_multi_synthesis' objects and then
           pushing (analysis) or pulling (synthesis) image data to/from
           these objects from a single thread -- almost always the group
           owner of a multi-threaded processing group whose workers participate
           in the underlying processing.  This model is very nice because
           the push/pull management thread sees a single-threaded API and
           does not have to worry about synchronization issues, except that it
           must `join' or `terminate' on instantiated thread queues before
           cleaning them up.  What actually happens is that the push/pull
           thread encounters conditions which might block its progress
           internally and enters the working wait state, participating as a
           processor of scheduled jobs, until it can proceed with what
           appears to be a single threaded processing model.  In this case,
           it is NOT GENERALLY SAFE for the group owner to be considered
           elligible to run custom jobs that the require a safe context
           (these are jobs that the application is deploying separately).
           The reasons for this are explained with the `attach_queue'
           function.
           [//]
           However, Kakadu's multi-threading sub-system provides an
           alternate, potentially more efficient approach for using the
           multi-threaded data processing machinery that is accessible using
           objects like `kdu_multi_analysis', `kdu_multi_synthesis' and
           the lower level objects `kdu_analysis', `kdu_synthesis',
           `kdu_encoder' and `kdu_decoder'.  In this alternate approach,
           the push/pull data processing calls are themselves executed from
           a suitable scheduled job, that runs only until the data processing
           objects (typically `kdu_multi_analysis' or `kdu_multi_synthesis')
           report that further push/pull calls may encounter blocking
           conditions, forcing the caller to enter a working wait.  At that
           point the running push/pull job finishes and another one is
           automatically scheduled once the potentially blocking conditions
           have been cleared.  This execution model has the potential to be
           more efficient for platforms with a large number of CPU's because
           it allows the push/pull functionality to resume as soon as the
           blocking condition is cleared and a thread (any thread) is
           available to do work.  By contrast, the simpler approach
           described in the previous paragraph requires a specific thread
           (typically the group owner) to perform all push/pull calls, so
           if it is temporarily blocked and goes away to do other work,
           it cannot resume the push/pull operations until this other work
           is finished.  This may cause the push/pull activity to be
           suspended so long that the complete multi-processing system
           stalls, causing a large number of thread context switches and
           under-utilization of available processing resources.  In this
           more efficient processing paradigm, the group owner typically
           spends all of its time in a working wait state, waiting for the
           actual processing to be performed by other jobs -- usually, the
           group owner sets things up and then invokes `join' to wait until
           everything is done.  In this case, the group owner's working wait
           does not present a dependency for any other job in the system, so
           it is always safe for the group owner to participate in any kind
           of work from within its outer-most working wait.  For this reason,
           applications that are based on this paradigm, should generally
           use the present function first to declare the group owner as
           elligible to run any kind of job from its outer-most working
           wait state.
           [//]
           For further guidance on how to implement the second processing
           paradigm described in the above paragraph, the reader is referred
           to the `kdu_run_queue' object, which wraps up most of the
           functionality required to run what would normally be a
           single-threaded push/pull processing pipeline from within a
           scheduled job that runs until it would potentially block and
           is automatically rescheduled once the blocking conditions
           go away.  Whether or not this processing paradigm actually
           improves processing throughput depends on many things, and one
           should note that frequently migrating the top-level
           push/pull processing calls between threads could actually have
           a negative impact on cache utilization.
           [//]
           Note that this present function may only be invoked from the
           group owner thread itself.
         [RETURNS]
           The value passed for `is_safe' within a previous call to this
           function, if any, or else false.  The default condition for
           a thread group on which this function has never been called, is
           that the group owner thread does not offer a safe context for
           running sensitive jobs.
         [ARG: is_safe]
           If true, the group owner is declared to be elligible to run
           jobs that require a safe context, so long as it is within an
           outer-most call to `join', `terminate' or `wait_for_condition',
           not having re-entered a working wait state from within a scheduled
           job.
           [//]
           If false, the group owner's elligibility to run such jobs is
           revoked.  This is the default condition, in case this function is
           never called.
      */
    KDU_EXPORT void
      set_yield_frequency(int worker_yield_freq);
      /* [SYNOPSIS]
           Since it could be very disruptive for a thread to be pre-empted
           by the operating system at an arbitrary point, worker threads
           explicitly yield their execution context periodically, at points
           where this is not likely to be disruptive.  This allows other
           tasks to be performed by the operating system, so that once a
           thread's time slice expires it is likely to be rescheduled
           immediately.
           [//]
           This function allows you to control the frequency with which
           threads yield their execution back to the operating system.  It
           is not completely clear how expensive or how extensive thread
           yielding actually is.  If the operating system has no other
           work pending, a context switch will probably not occur.  In any
           case, this is at least a moderately expensive system call, so
           one should probably avoid yield frequencies close to 1.  On the
           other hand, if yielding is too infrequent it may be ineffective.
           For example, consider a video processing application in which
           other threads (not managed by this object) are responsible for
           frame refresh and/or image I/O being performed at regular
           intervals.  One would like to be sure that at least one thread
           yields its execution at least once per frame period so that the
           application's processing pipeline does not get starved or
           over-full, causing a lot of threads to go idle.  The
           `get_job_count_stats' function can be used to estimate reasonable
           yield frequencies, as explained in the description of that
           function.
           [//]
           It is worth noting that threads which are doing work while
           inside the `wait_for_condition' function do not yield execution
           as a result of doing jobs, since this might be disruptive.  This
           necessarily means that the group owner thread (the one that
           invoked `create') never explicitly yields its execution --
           although you can do this yourself if you like.  This means that
           the present function only affects worker threads -- those created
           using `add_thread'.
         [ARG: worker_yield_freq]
           Typical number of jobs executed by a worker thread (not inside
           `wait_for_condition') before it invokes the `kdu_thread::yield'
           function.  Calls to this function do not synchronize thread yield
           events, so over time the yield instances will become randomized;
           this is probably a good thing.
           [//]
           It is legal to supply a value of 0 for this argument, in which
           case explicit yielding is suspended.
           [//]
           The default yield frequency is currently 100, but that might be
           too high for systems with lots of threads or which require
           frequent attention to other work.
      */
    KDU_EXPORT kdu_long
      get_job_count_stats(kdu_long &group_owner_job_count);
      /* [SYNOPSIS]
           This function retrieves what might only be an estimate of the
           total number of jobs performed by all threads since the last
           time this function was called.  For maximum reliability, you should
           endeavour to invoke this function only from one thread (or at
           least only from one thread at a time).  In practice, it probably
           never makes sense to invoke the function from any thread other
           than the group owner thread.  As a courtesy, the number of jobs
           performed by the group owner itself is returned separately via
           the `group_owner_job_count' argument -- this value is also
           included in the overall count returned by the function.
           [//]
           The information reported by this function may be useful in
           determining good values to supply to the `set_yield_frequency'
           function.  For example, in a video processing application that
           has other per-frame tasks to perform on other threads, it is
           a good idea to make sure that the working threads yield their
           execution context explicitly from time to time, in such a way
           that this happens at least once per frame.  In order to make this
           happen, an application might invoke the present function once
           each frame, divide the resulting job count by the total number
           of threads, and use this for the yield frequency, so that each
           thread on average yields execution once per frame -- of course if
           there is no other work to do, yielding execution should not
           actually result in a context switch.
      */
    KDU_EXPORT bool
      attach_queue(kdu_thread_queue *queue, kdu_thread_queue *super_queue,
                   const char *domain_name, kdu_long min_sequencing_idx=0,
                   int queue_flags=0);
      /* [SYNOPSIS]
           All processing in Kakadu's multi-threading environments is
           ultimately associated with thread queues.  Unlike previous
           versions of Kakadu, the `kdu_thread_queue' is no longer an opaque
           internal object; instead, major processing entities in your
           system may be based on objects that you derive from
           `kdu_thread_queue'.  For this reason, you supply the (optionally)
           derived object to this function as its `queue' argument.  It is,
           however, possible to use the base `kdu_thread_queue' object as
           it stands without any derivation.  It is also possible to
           request a special internally allocated `kdu_thread_queue' object
           by using the `add_queue' function -- provided mainly for
           backward compatibility with previous versions of Kakadu.
           [//]
           Conceptually, processing jobs are scheduled by invoking
           `kdu_thread_queue::schedule_jobs' and threads in the relevant
           domain are used to perform these jobs, wherever possible.  However,
           the internal implementation need not (and currently does not) look
           like this at all.  Jobs added via the queue's
           `kdu_thread_queue::schedule_jobs' function are directly added
           to a single domain-specific job pool and performed both by threads
           that are associated with the domain (first choice) or by any
           other threads.  The term "queue" is still used, suggesting that
           each `kdu_thread_queue' might have its own separate queue of jobs
           in some implementations.  However, the threading environment no
           longer provides any guaranteed way of waiting for such hypothetical
           job queues to empty; at the level of the `kdu_thread_entity' object,
           it is possible only to wait until a thread queue identifies itself
           as completely "finished" via its `kdu_thread_queue::all_done'
           function.  One waits for this to happen using the important
           `join' function.  It is also possible to encourage a queue (or
           sub-tree of related queues) to finish early by calling the
           `terminate' function, which is closely related to (actually
           implemented on top of) the `join' function.  See the descriptions
           of `join' and `terminate' for more information on how these
           things work.
           [//]
           Once a `queue' has been added using this function, it remains
           associated with the thread group until it is removed by
           a call to `join' (possibly as part of a call to `terminate'), even
           though the queue might meanwhile identify itself as being
           "finished".  Moreover, so long as a queue remains associated
           with the thread group, you can add other queues to it as
           descendants, by passing it as the `super_queue' argument
           to this function.  The role of super queues is primarily to
           provide a simple means for applications to wait for completion
           of a collection of related queues, via calls to `join' or
           `terminate'.
           [//]
           A typical way to use super-queues in the Kakadu environment is
           as follows: 1) each tile processor may be associated with a
           single top-level queue; 2) each tile-component processor may be
           associated with a sub-queue of the corresponding tile queue;
           3) each subband may be associated with a sub-queue of the
           corresponding tile-component queue; and 4) each codestream may
           be associated with a special internal queue that handles background
           reading, writing or speculative memory management for the
           codestream data itself.
           [//]
           As with `add_thread', this function has a `domain_name' argument
           that may cause the creation of a new domain (initially without any
           associated threads) if it is found to be unique.  Domains survive
           until the thread group is destroyed, so you should not be supplying
           an endless string of new domain names here.
           [//]
           Unlike `add_thread', this function must be supplied with
           a non-NULL `domain_name' argument that does not refer to an
           empty string, except in the special case of queues that do not
           schedule jobs.  That is, if a queue does schedule jobs, meaning
           that its `kdu_thread_queue::get_max_jobs' function returns a
           non-zero value, the queue may not be added to a thread group's
           "default domain".  This ensures that threads within the default
           domain (of which the group owner is always one) are always equally
           prepared to do work within any domain.
           [//]
           It is worth noting that any thread may create queues, not just the
           group owner thread.  It follows that any thread may create new
           domains.
           [//]
           The `queue_flags' argument allows queues to be assigned specific
           attributes as they are attached.  You should be aware that the
           attributes supplied here are added to the thread domain associated
           with `domain_name', even if it already exists, and affect the way
           in which all of its jobs are processed.  For this reason, the
           thread queue attributes should really be understood as attributes
           for all the jobs that are scheduled via thread queues associated
           with the domain.  Currently, only two attribute flags are defined,
           as follows:
           [>>] `KDU_THREAD_QUEUE_SAFE_CONTEXT' -- if this flag is present,
                jobs scheduled to the thread domain may not be launched by any
                thread that is currently in an unsafe "working wait" state.
                Threads enter a working wait state by calling
                `wait_for_condition', `join' or `terminate' -- they remain in
                this state until the condition they are waiting for occurs,
                and during this time their resources may be used to execute
                other jobs that have been scheduled.  This is generally
                beneficial, because it minimizes the likelihood that a thread
                becomes idle.  However, it is possible for deadlocks to occur
                in some circumstances.  For example, consider a job that
                enters a working wait by calling `wait_for_condition' to wait
                for a condition (A); suppose this thread executes another job
                while waiting and this job attempts to `join' upon the
                completion of a thread queue that cannot finish until the
                first job passes condition A.  This leaves the thread waiting
                upon itself indefinitely.  Considering this, where an
                application schedules its own custom jobs, it is usually
                safest to do so through a queue with the
                `KDU_THREAD_QUEUE_SAFE_CONTEXT' attribute.  In this case,
                the queues jobs can be executed by a thread that is not in
                a working wait, or by the main "group owner" thread for the
                multi-threaded environment, if `declare_first_owner_wait_safe'
                has been called and the group owner thread has not recursively
                entered the working wait state.  For more on this last
                condition, refer to the documentation accompanying the
                `declare_first_owner_wait_safe' function.  If your custom
                jobs never invoke `join', `terminate' or `wait_for_condition',
                either directly or indirectly, there is no need to
                schedule them with the safe context restriction.
           [>>] `KDU_THREAD_QUEUE_BACKGROUND' -- As mentioned in
                the introductory notes to the `kdu_thread_entity' object,
                jobs scheduled within a background thread domain (i.e.,
                jobs with this attribute) might experience long delays and
                are often scheduled well ahead of the point at which they
                are needed.  For this reason, background jobs are not
                performed within a "working wait" (i.e., not performed by
                a thread that is inside `wait_for_condition', `join' or
                `terminate') unless all threads in the entire thread group
                are in the working wait state.
           [//]
           Evidently, the `KDU_THREAD_QUEUE_BACKGROUND' and
           `KDU_THREAD_QUEUE_SAFE_CONTEXT' attributes have very similar
           implications.  The main difference is that background jobs can
           potentially be executed within a working wait if there are no
           other options, whereas safe-context jobs cannot.  In fact,
           safe-context jobs make no sense unless the thread group has at
           least two threads (at least one worker thread, in addition to the
           group owner), whereas background jobs can still make sense even
           if there is actually only one thread.
           [//]
           Both background and safe-context thread queues are also treated
           slightly differently with respect to calls to `join' and
           `terminate'.  Specifically, these calls always wait for completion
           of regular queues before waiting for the completion of background
           or safe-context queues with the same `super_queue' parent.
           Similarly, top-level background/safe-context queues are joined
           after top-level regular queues.
           [//]
           The role of the `min_sequencing_idx' argument is perhaps the most
           mysterious.  Associated with each queue is a sequencing index
           that is the larger of `min_sequencing_idx' and the sequencing
           index of any `super_queue' to which it is added as a descendant.
           When `kdu_thread_queue::schedule_jobs' is used to schedule new jobs,
           they are assigned the queue's sequencing index.  The system
           attempts to launch jobs in such a way that those with smaller
           sequencing indices all run before those with larger sequencing
           indices; moreover, it tries to do this across all jobs in all
           domains.  Of course, this might not be possible if all jobs with
           sequencing index N have been launched and then, some time later,
           new jobs are scheduled that have a sequencing index smaller than N.
           [//]
           Distinct sequencing indices can actually consume significant
           resources internally, so you are advised to use them sparingly,
           advancing the index by 1 between tiles or sets of tiles you wish
           to have processed before other ones, or between codestreams you
           wish to have processed in order.  The main reason for introducing
           sequencing indices at all is to allow you to create queues and
           commence processing on a new data set (e.g., tiles of a codestream
           or codestreams within a larger set) while some asynchronous work
           completes on an older data set -- this way, you can avoid having
           some threads go idle as work comes to a close on the old data set.
           [//]
           You should note that the value of `min_sequencing_idx' passed
           to the present function is generally expected to be non-decreasing.
           In fact, you would do best to start with a sequence index of 0
           and increment the index by 1 each time the need arises.  If
           you have already added a queue with one sequencing index, you
           should not later add one with a smaller sequencing index.  If
           you do, the index will be increased automatically to ensure
           monotonicity.  Nevertheless, the function does attempt to remove
           wrap-around effects by judging sequence index S1 to be smaller
           than sequence index S2 only if (kdu_long)(S2-S1) is positive.  In
           this way, it should be possible to increment sequence indices
           indefinitely without violating the ordering convention.
           [//]
           This function may throw an exception if something has failed
           inside the thread group (probably another thread has thrown
           an exception and passed through `handle_exception').
         [RETURNS]
           False if the queue cannot be added -- could be because the
           `domain_name' string refers to the thread group's default domain,
           or because `queue' already belongs to a thread group.
         [ARG: queue]
           If `queue' is already associated with the thread group via a
           previous call to `add_group', the function returns false.  If
           `join' has previously been used to remove the queue from a
           thread group, however, it may be added back again (or to a
           different thread group) using this function.
         [ARG: super_queue]
           If NULL, `queue' is added as a top-level queue within the relevant
           domain.  Otherwise, `queue' is added as a descendant of the
           supplied super-queue.  If `super_queue' does not belong to this
           thread group already, the function returns false.
         [ARG: domain_name]
           If NULL or an empty string, the function returns false, unless
           `queue->get_max_jobs' returns 0.  Queues that can schedule jobs
           must be added to a non-default domain.  If the domain does not
           already exist, the function creates it.
         [ARG: min_sequencing_idx]
           See the extensive discussion of sequencing indices appearing in
           the notes above.
         [ARG: queue_flags]
           Refer to the extensive discussion above.  Note that if you are
           attaching your own queue for the purpose of scheduling your own
           custom jobs, you should generally supply the
           `KDU_THREAD_QUEUE_SAFE_CONTEXT' flag, unless you are sure that your
           custom jobs will not invoke `wait_for_condition', `join' or
           `terminate'.
      */
    KDU_EXPORT kdu_thread_queue *
      add_queue(kdu_thread_dependency_monitor *monitor,
                kdu_thread_queue *super_queue, const char *domain_name,
                kdu_long min_sequencing_idx=0);
      /* [SYNOPSIS]
           This function is provided mostly as a convenience to facilitate
           the transition from versions of Kakadu prior to version 7.0.
           In earlier versions, `kdu_thread_queue' was an opaque object
           allocated by the thread entity.  Now, `kdu_thread_queue' is an
           object that you can derive from and that you are responsible
           for allocating and destroying.  However, you can use this
           function to request an internally allocated `kdu_thread_queue'
           object that will automatically be cleaned up by the thread entity.
           It is an error to attempt to delete the returned object.
           [//]
           The object is automatically deleted by calls to `terminate' or
           `join' that wait upon this object either directly or through a
           parent.  Failing this, the object is deleted when the thread
           group -- happens when the group owner invokes `destroy'.
           [//]
           In order to ensure that `join' and `delete' can remove queues
           allocated by this function, they are handled rather differently
           from the queues passed directly into `attach_queue'.  Those
           queues are unlinked from the thread group whenever
           `handle_exception' is called, whereas that would cause queues
           allocated by this function to be destroyed, which might cause
           problems if they are accessed by some part of the application.
           To avoid these difficulties, the following policies are enforced:
           [>>] Queues allocated by this function are not unlinked from the
                thread group by any function other than `join' or `terminate'.
           [>>] Any `super_queue' supplied in this function must itself have
                been allocated using the present function.  That is,
                internally allocated queues may not be sub-queues of
                application-owned "attached" queues.  The reverse, however,
                is perfectly acceptable -- a queue allocated using this
                function may be passed as a `super_queue' to either the
                `add_queue' or `attach_queue' function.
         [RETURNS]
           NULL if a queue could not be created, either because `super_queue'
           was not created using the present function, or for any of the
           reasons that would cause the `attach_queue' function to return
           false.
         [ARG: monitor]
           In many cases, this argument will be NULL.  However, considering
           that there is no opportunity to derive from an internally
           allocated `kdu_thread_queue' object, a convenient way to arrange
           for some action to be taken when all of the dependencies
           associated with descendent queues are satisfied is to perform this
           action within the `monitor->update' function of a derived
           `kdu_thread_dependency_monitor' object.  The `monitor' object
           is simply passed to `kdu_thread_queue::set_dependency_monitor'
           when the queue is created -- see that function for more
           information.
      */
    KDU_EXPORT void advance_work_domains();
      /* [SYNOPSIS]
           There is not normally any need for you to call this function
           yourself, since work domains are automatically advanced when the
           thread processes scheduled jobs, waits upon a condition (see
           `wait_for_condition') and when calls to `attach_queue' cause the
           domain sequence within a work domain to advance.  However, it might
           be useful to call this function if a thread performs neither of
           the above activities at regular intervals -- this would be unusual.
           [//]
           To understand what this function does, you should first read the
           comments appearing with `attach_queue', noting the interpretation
           of sequence indices.  Thread queues that schedule jobs are
           associated with a work domain (identified by the `domain_name'
           argument to `attach_queue') and also have a sequence index.
           In some applications, it is very useful to advance the sequence
           index associated with newly created queues in a meaningful way,
           so that the jobs scheduled by these queues (and all their
           descendants) are scheduled for execution only after those from
           queues with earlier sequence indices, within the same domain.
           Internally, each thread keeps its own notion of the most recent
           sequence within each domain, from which jobs may need to be
           launched.  Only once all threads have advanced their record of
           the most recent sequence can the associated scheduling resources
           be recycled.  Although the amount of memory required for scheduling
           may be small, this memory may grow indefinitely if a thread never
           makes any attempt to change its notion of the most recent sequence
           within the various domains in which it can do work.  As mentioned
           above, this might happen if the thread never attempts to execute
           scheduled jobs -- note that threads which enter `wait_for_condition'
           generally execute scheduled jobs while they are waiting.
           [//]
           Calls to the present function ensure that the calling thread's
           notion of the most recent sequence within each work domain can
           be advanced as required.  As mentioned above, it is highly unlikely
           that an application needs to invoke this function explicitly.
           This is because most applications allow worker threads to perform
           jobs of limited duration (i.e., the job eventually returns and
           then work domains are examined for new jobs to perform), while the
           main thread that constructed the thread group invokes either
           `wait_for_condition' or `attach_queue', directly or indirectly,
           at regular intervals.
           [//]
           The only case in which you might need to call the function
           explicitly is where the main thread or a worker thread executing
           a scheduled job remains in a state where it neither returns nor
           calls any of the above mentioned functions (these are often
           called indirectly from within Kakadu's data processing objects
           like `kdu_multi_analysis' or `kdu_multi_synthesis') while
           other threads attach queues with progressively increasing
           sequence indices.  Usually an application of this form must
           provide some external method for the threads to synchronize
           with one another since synchronization is most naturally
           implemented by calls to `wait_for_condition'.
      */
    kdu_thread_entity_condition *get_condition() const
      { assert(check_current_thread());  return cur_condition; }
      /* [SYNOPSIS]
           This is the first of three functions that are used to set up,
           wait for and signal conditions that might not be immediately
           available.  The mechanisms are discussed more concretely in
           connection with `wait_for_condition'.  What you need to know
           here is that:
           [>>] The function returns a non-NULL pointer that should be
                considered opaque.  The pointer can be passed to both
                `wait_for_condition' and `signal_condition', but not
                manipulated in any other way.
           [>>] This function might be called again from within
                `wait_for_condition' if that function dispatches the thread to
                do other jobs while waiting -- in that case, though, the
                returned pointer will be different so that each waiting
                context has a unique condition.
           [>>] It is illegal to call this function from any thread other
                that the one that is associated with this `kdu_thread_entity'
                object; that is, it is not possible for one thread to use
                this function to discover another thread's condition.  This
                means, in particular, that in order for a thread to awake
                from a call to `wait_for_condition' (apart from
                error/destruction conditions), the thread must have first
                called this function itself and recorded the condition
                somewhere so that it could be passed to `signal_condition'.
           [//]
           Note that this function does not throw exceptions and generally
           has very low overhead (currently just an barrier-less read from
           the thread's internal state).
         [RETURNS]
           NULL only if the `create' function has never been called.
      */
    KDU_EXPORT void wait_for_condition(const char *debug_text=NULL);
      /* [SYNOPSIS]
           This function plays a very important role in maximizing thread
           processing resources.  The call must arrive from within the
           current object's thread -- if another thread calls this function
           an error may be generated, or the behaviour may be completely
           unpredictable, depending on the implementation.
           [//]
           Typically, the caller has previously invoked `get_condition' and
           left behind a record of the resulting pointer for some other
           thread (possibly itself) to pick up in the future and pass to
           `signal_condition', once the relevant condition is fulfilled.
           This is the mechanism that should be used by any
           processing agent (typically derived from `kdu_thread_queue') if
           a caller asks for data that depends on the availability of
           resources from other asynchronous processing agents.
           [//]
           The function may ultimately invoke `kdu_thread::wait_for_signal',
           but before doing so, the caller will be dispatched to process
           any outstanding jobs which it can legitimately perform within
           the thread group's domains -- basically, any outstanding jobs in
           any domain.  In doing so, the thread may pass back into this
           function to wait for another condition.  Conditions are set up on
           an internal stack, so the pointer returned by `get_condition' will
           generally differ from that returned by a previous call, if the
           thread has subsequently entered (but not yet left) the
           `wait_for_condition' function.
           [//]
           You should be aware that calls to this function may throw an
           exception if any part of the multi-threaded system fails for
           some reason.
           [//]
           You should also note that calls to `get_condition' do not reset
           the "signalled" state of a thread condition, but the present
           function does reset the "signalled" state of the thread condition
           when it returns.
           [//]
           This function is used quite extensively inside the core
           data processing machinery associated with the `kdu_encoder',
           `kdu_decoder', `kdu_multi_analysis' and `kdu_multi_synthesis'
           objects, each of which creates and attaches queues, schedules
           jobs, and implements waits in order to make push/pull data
           calls look synchronous.  As discussed with the `attach_queue'
           function, these waits are "working waits", because other work
           can be done by the thread while it waits for the condition to
           occur.  Entering the working wait state within those objects is
           generally safe with regard to deadlock possibilities.  However,
           if you intend to create your own queues for scheduling your
           own custom jobs, it is conceivable that deadlocks could be
           created if your custom jobs invoke `wait_for_condition' themselves.
           To avoid this risk, you are advised to pass the
           `KDU_THREAD_QUEUE_SAFE_CONTEXT' flag to `attach_queue' when
           attaching your custom queues.
         [ARG: debug_text]
           To assist in identifying the cause of any deadlock that might occur
           you are encouraged to provide a text string here that explains
           what you are waiting for.  This string will be included in the
           debugging report printed in the event that a deadlock is detected.
           Of course, we do not expect deadlocks, because the system has been
           designed very carefully, but bugs are possible and, more likely,
           a deadlock may result from misuse of some of the most sensitive
           multi-threading API calls.
      */
    KDU_EXPORT void signal_condition(kdu_thread_entity_condition *cond);
      /* [SYNOPSIS]
           This function may be invoked from any thread in order to signal
           the fact that a condition has occurred.  The purpose of this is
           to all the thread associated with that condition to return from
           a call to `wait_for_condition'; if the condition is signalled
           before the thread waits, it will return immediately from the
           `wait_for_condition' call.
           [//]
           The `cond' pointer itself must have previously been obtained by
           the `get_condition' function; moreover, that function must be
           invoked from within the same thread of execution that is to
           be signalled by this function.
           [//]
           As a courtesy, to cover cases in which non-NULL condition
           references are written and then re-NULL'ed right before an
           application invokes this function, the function does absolutely
           nothing if `cond' happens to be NULL on entry.
           [//]
           This function should never throw an exception, even if other
           threads in the system have failed, invoking `handle_exception'.
           The call is designed to be fast and efficient.
           [//]
           It is actually safe to invoke this function from any thread,
           even if it does not belong to the same thread group as the
           current object -- this may happen if the function is called
           indirectly from within `kdu_thread_queue::force_detach'.
      */
    KDU_EXPORT bool
      join(kdu_thread_queue *root_queue, bool descendants_only=false,
           kdu_exception *exc_code=NULL);
      /* [SYNOPSIS]
           This is a very important function.  In a way, it is the dual of
           `add_thread', because the only ways to undo the associations
           created by `add_thread' are through `join', `handle_exception',
           `terminate' or `destroy'.  Of these, the only ones that can
           remove specific queues from the thread group are `join' and
           `terminate', while `terminate' is actually implemented on top
           of `join'.
           [//]
           The function is normally invoked from the group owner, but need
           not be.  In fact, it is possible to create a special thread
           queue with only one job, whose purpose is to `join' with other
           other thread queues and clean them up.  However, in this case,
           very special attention needs to be paid to false returns, as
           explained further below.  Moreover, in this case, you should
           be very careful to schedule such jobs within queues that have
           been attached using the `KDU_THREAD_QUEUE_SAFE_CONTEXT'
           queue attribute in the `attach_queue' call, so as to avoid the
           possibility of inadvertent deadlocks.
           [//]
           If `descendants_only' is true, the function waits for all queues
           descended from `root_queue' to identify themselves as "finished"
           via the `kdu_thread_queue::all_done' function.  If
           `descendants_only' is false, the function also waits for the
           `root_queue' to identify itself as "finished".  The function clears
           the thread group of all internal records that might relate to these
           "finished" queues before returning.  For internally allocated
           queues obtained using `add_queue', the queue itself is deleted
           during this unlinking process.  Otherwise, you are responsible for
           destroying the unlinked `kdu_thread_queue' objects that you
           passed to `attach_queue', since these are often derived objects
           that may be linked into your application in complex ways.
           [//]
           If `root_queue' is NULL, this call waits for all queues in the
           thread group to identify themselves as "finished" before returning.
           [//]
           The name `join' is used because of the conceptual similarities
           between this function and the notion of joining for Posix threads
           (and processes).  Importantly, while waiting for the queues to
           finish their work, the calling thread itself participates in any
           outstanding work that remains to be done.  This means that the
           caller may internally get scheduled to do outstanding jobs; these
           may include jobs that do not directly relate to the queues that
           the caller is waiting upon.  For this reason, `join' is considered
           to place the caller in a "working wait state", in the same way
           that `wait_for_condition' does.  Working wait states are discussed
           in connection with the `attach_queue' function, where the spectre
           of deadlock is raised.  To be sure of avoiding deadlock it is best
           to invoke `join' only from the group owner or from a job that
           has been scheduled within a queue attached using the
           `KDU_THREAD_QUEUE_SAFE_CONTEXT' attribute flag.
           [//]
           Generally, it is safe for multiple threads to concurrently `join'
           with a single queue, or for threads to `join' with the descendant
           of a super-queue which is being `join'ed by another thread.  In
           fact, it is even allowed for a thread which is executing `join' to
           indirectly call `join' again (remember that the thread is
           dispatched to do other work if possible), even if it `join's
           a second time with the same queue, although these things cannot
           happen if the `join' calls only come from
           `KDU_THREAD_QUEUE_SAFE_CONTEXT' jobs (see `attach_queue') and this
           is advisable unless you are prepared to conduct your own
           detailed analysis of the dependencies that might be set up
           by a job not being able to proceed until `join' returns.
           [//]
           This function will never throw an exception.  Instead, all
           internal exceptions are caught.  However, if any thread in the
           group invokes the `handle_exception' function, either before or
           while this function is being called, the function will produce
           a false return value.  The exception itself may be recovered
           via a non-NULL `exc_code' argument.
           [//]
           You may need to pay very special attention to false return values,
           especially if you are using threads in a complex manner (e.g.,
           executing `join' from within a thread other than the thread group
           owner).  Here are the things you should know (the considerations
           apply equally to `join' and `terminate'):
           [>>] If `join' or `terminate' returns false, it tries to ensure
                that all processing associated with the queues that are being
                waited upon has completed.  Since an exception has occurred,
                no thread will execute any new jobs after those that are
                currently being executed finish.  With this in mind, the most
                natural approach is to wait until no thread is executing
                scheduled jobs.  This can work well if `join' and `terminate'
                are not themselves invoked from within scheduled jobs (see
                `kdu_thread_queue::schedule_jobs'); in this typical case,
                the caller is also necessarily the thread group owner.
           [>>] To cater for cases where `join' or `terminate' are invoke from
                within a scheduled job, if an exception occurs or has occurred,
                and `join' or `terminate' are invoked from within a scheduled
                job, the function actually waits until every thread is either
                not executing a scheduled job or else waiting within `join'
                or `terminate'.  In this way, deadlock is avoided, but one
                cannot be sure of the order in which `join' or `terminate'
                calls return false in different threads, in the event of an
                exception.
           [>>] In view of the above considerations, if you do call this
                function from within a scheduled job and it returns false,
                the safest policy is to immediately throw an exception (of
                type `kdu_exception') avoiding the temptation to access
                resources that might be asynchronously cleaned up in another
                thread that was also waiting in `join' or `terminate'.
         [RETURNS]
           False if any thread in the group invoked its `handle_exception'
           function either before or during the execution of this function.
           Handling of this case is discussed extensively above.
         [ARG: root_queue]
           If NULL, the function waits until all queues in the thread group
           identify themselves as "finished".  Otherwise, only those
           jobs which are related to the supplied `root_queue' must complete,
           according to the interpretation supplied by `descendants_only'.
           [//]
           If `root_queue' is non-NULL, but is not currently attached to
           the thread group, the function does not have to wait on anything
           before returning true; however, if an exception has occurred,
           the function will return false only after waiting to be sure
           that no other threads are executing scheduled jobs, unless they
           are themselves waiting within `join' or `terminate'.
         [ARG: descendants_only]
           If true and `root_queue' is non-NULL, only those queues which are
           descendants of `root_queue' will be waited upon.  Otherwise, the
           `root_queue' will also be waited upon.  Only those queues that are
           waited upon are actually removed from the system.  Thus, if
           `descendants_only' is true and `root_queue' is non-NULL, the
           `root_queue' will still be "added" into the thread group when
           this function returns -- i.e., there is no need to add it back
           again via `attach_queue'.
         [ARG: exc_code]
           If non-NULL, the referenced variable will be set to the value of
           the `exc_code' which was passed to `handle_exception' by
           any failed thread.  This is meaningful only if the function
           returns false.  Note that a special value of `KDU_MEMORY_EXCEPTION'
           means that a `std::bad_alloc' exception was caught and saved.  If
           you intend to rethrow the exception after this function returns,
           you should ideally check for this value and rethrow the exception
           as `std::bad_alloc'.
      */
    KDU_EXPORT bool
      terminate(kdu_thread_queue *root_queue, bool descendants_only=false,
                kdu_exception *exc_code=NULL);
      /* [SYNOPSIS]
           This function plays a very similar role to `join', except that
           it attempts to coerce the relevant queues to enter the "finished"
           state prematurely.  This is done initially by invoking
           `kdu_thread_queue::request_termination' on the `root_queue'
           (if non-NULL and `descendants_only' is false) or on each of
           its descendants (if non-NULL and `descendants_only' is true) or
           on all top-level queues (if `root_queue' is NULL).  Thereafter,
           the function actually just invokes `join'.
           [//]
           What actually happens depends upon the implementation of the
           `kdu_thread_queue::request_termination' function.  The base
           implementation of that overridable virtual function, which
           should always be invoked from any override, ensures that as soon
           as the queue's `kdu_thread_queue::all_done' function is invoked,
           all of its descendants will also receive a call to their
           `kdu_thread_queue::request_termination' function.  In this way,
           termination requests ripple down the queue hierarchy as each
           queue responds to its own request by invoking its
           `kdu_thread_queue::all_done' function.  Ideally, this happens
           quickly.
           [//]
           Normally, a queue should invoke its `kdu_thread_queue::all_done'
           function only once it has finished all work, which means that its
           descendants should expect that they will not be accessed by data
           processing agents associated with any parent queue.  Assuming
           that queue hierarchies correspond to work hierarchies, implemented
           through derived instances of the `kdu_thread_queue' class, this
           means that once a queue has received a call to its
           `kdu_thread_queue::request_termination' function, there should
           be no need for it to respond to any further requests to process
           new data.  Normally, the queue should avoid all future scheduling
           of jobs once it receieves the termination request and arrange to
           invoke its `kdu_thread_queue::all_done' function as soon as
           the last outstanding schedule job has finished executing.  It
           is perfectly reasonable also for the individual jobs to
           terminate their work prematurely, as soon as they detect that
           their queue has received a termination request.
           [//]
           What all this means is that the `terminate' function is designed
           with the objective of orderly premature shutdown in mind.  There
           is no guarantee that work which has already been launched will
           be completed in the usual way after `terminate' has been called,
           although the `terminate' call does not affect other queues which
           are independent of those being terminated.
           [//]
           As with `join', this function will never throw an exception.
           Instead, all internal exceptions are caught.  However, if any
           thread in the group invokes the `handle_exception' function,
           either before or after this function is called, the function will
           produce a return value of false.  The exception itself may be
           recovered via a non-NULL `exc_code' argument.
           [//]
           Handling of false returns is discussed extensively in connection
           with the `join' function -- the same considerations apply to both
           `join' and `terminate'.
         [RETURNS]
           False if any thread in the group invoked its `handle_exception'
           function either before or during the execution of this function.
           Handling of this case is discussed extensively in connection with
           the `join' function.
         [ARG: root_queue]
           If NULL, the function issues termination requests to all top-level
           queues and waits until they have all identified themselves as
           "finished".  Otherwise, only those jobs which are related to the
           supplied `root_queue' are affected by the call, according to the
           interpretation supplied by `descendants_only'.
           [//]
           If `root_queue' is non-NULL, but is not currently attached to
           the thread group, the function does not have to wait on anything
           before returning true; however, if an exception has occurred,
           the function will return false only after waiting to be sure
           that no other threads are executing scheduled jobs, unless they
           are themselves waiting within `join' or `terminate'.
         [ARG: descendants_only]
           If true and `root_queue' is non-NULL, only those queues which are
           descendants of `root_queue' will be terminated and waited upon.
           Otherwise, the `root_queue' will also be terminated and waited
           upon.  Only those queues that are waited upon are actually
           removed from the system once done.  Thus, if `descendants_only' is
           true and `root_queue' is non-NULL, the `root_queue' will still be
           "added" into the thread group when this function returns -- i.e.,
           there is no need to add it back again via `attach_queue'.
         [ARG: exc_code]
           If non-NULL, the referenced variable will be set to the value of
           the `exc_code' which was passed to `handle_exception' by
           any failed thread.  This is meaningful only if the function
           returns false.  Note that a special value of `KDU_MEMORY_EXCEPTION'
           means that a `std::bad_alloc' exception was caught and saved.  If
           you intend to rethrow the exception after this function returns,
           you should ideally check for this value and rethrow the exception
           as `std::bad_alloc'.
      */
    KDU_EXPORT virtual void handle_exception(kdu_exception exc_code);
      /* [SYNOPSIS]
           As explained in the introductory comments, this function
           is required to handle errors generated through `kdu_error'.
           These errors either terminate the entire process, or else they
           throw an exception of type `kdu_exception'.  In the former case,
           no cleanup is required.  When an exception is caught, however, two
           housekeeping operations are required: 1) any locks currently held
           by the thread must be released so as to avoid deadlocking other
           threads; 2) all `kdu_thread_queue' objects that were passed
           to the `attach_queue' function are unlinked from the thread
           group -- those queues that were obtained using `add_queue',
           however, remain linked into the system until `join', `terminate'
           or `destroy' deletes them.
           [//]
           In addition to these housekeeping operations, the thread group is
           marked as having failed so that the exception can be replicated in
           other threads within the group when they enter relevant API
           functions.  The `exc_code' supplied here will be used when
           replicating the exception in other threads.
           [//]
           After handling an exception, no further jobs will be scheduled
           or performed within the thread group and calls to `join' will
           return false immediately (i.e., without having to wait for
           anything).  In fact, `handle_exception' automatically removes
           all queues from the thread group.  Once a call to this function
           has occurred, it is important that the group owner invoke
           `destroy' before destroying resources that might have been
           involved in ongoing work at the time when the exception was
           encountered.
      */
  protected: // Functions that can potentially be overridden
    virtual void process_jobs(kdu_thread_entity_condition *check_cond);
      /* You would not normally override this function, but it is possible.
         In that case, an overridden version of the function should invoke
         the base version at some point.  This is the entry-point function
         for a newly created worker thread (created by `add_thread').  The
         function may also be invoked (recursively) from within
         `wait_for_condition'.  Generally, the function enters a loop,
         pulling jobs from the most relevant queues and executing them,
         idling when there are no jobs to be done.  The loop may be
         terminated by a call to the thread group's `destroy' function or
         by the occurrence of the condition associated with a call to
         `wait_for_condition' -- the latter is tested if the function was
         invoked with a non-NULL `check_cond' argument.
      */
  private: // Helper functions
    void pre_launch();
      /* This function is called automatically when a non-group-owner
         thread is first launched, after which the `process_jobs' function
         is entered.  One thing done by this function is to lock the group
         mutex temporarily and scan through the list of thread contexts,
         invoking `kdu_thread_context::num_threads_changed'. */
    void pre_destroy() { if (!is_group_owner()) group = NULL; }
      /* This function is called automatically when a non-group-owner
         thread finishes so that the safety check in the destructor will
         not fail.  This is done only for code verification purposes.  You
         should not call the function yourself from an application. */
    void lock_group_mutex();
    void unlock_group_mutex();  
      /* Use these in place of directly locking and unlocking the
         group->mutex object, wherever possible, since they allow safe
         recursive locking. */
    void generate_deadlock_error_and_report();
      /* Called when we detect that all threads in the group are waiting. */
    kdu_thread_entity_condition *push_condition()
      { // Adds a condition to the stack topped by `cur_condition'; returns
        // the new top of the condition stack.
        kdu_thread_entity_condition *cond = free_conditions;
        if (cond == NULL)
          { cond = new kdu_thread_entity_condition; cond->is_dynamic=true; }
        else
          free_conditions = cond->link;
        cond->signalled = false; cond->thread_idx = this->thread_idx;
        cond->link = cur_condition;  cur_condition = cond;
        cond->debug_text = NULL;
        return cond;
      }
    kdu_thread_entity_condition *pop_condition()
      { // Pops the `cur_condition' object from the condition stack; returns
        // the new top of the condition stack.
        kdu_thread_entity_condition *cond = cur_condition;
        cur_condition = cond->link;
        cond->link = free_conditions;  free_conditions = cond;
        return cur_condition;
      }
    void send_termination_requests(kdu_thread_queue *queue,
                                   bool descendants_only);
      /* This function is used by `terminate' and also by `queue->all_done'.
         If the group mutex has not been locked in a surrounding context,
         it is locked here before proceeding and unlocked before returning.
            If `descendants_only' is false and `queue' is non-NULL, the
         function manipulates `queue->waiting_for_done' to record the special
         "pending termination" code of 2, unless it was already 0.  In the
         first case, `queue->request_termination' is called, while in the
         latter case the function recursively invokes itself on the
         descendants of `queue'.
            If `queue' is NULL, the function recursively invokes itself on
         all top-level queues with `descendants_only' set to false.
            If `queue' is non-NULL and `descendants_only' is true, the
         function recursively invokes itself on the descendants of `queue'
         without modifying `queue' itself. */
    void wait_for_exceptional_join();
      /* This function is called only from inside `join' or `terminate' if
         an exception occurs or is found to have occurred.  As explained with
         `join', the function must wait until all threads either finish
         executing scheduled jobs or are waiting inside this same function. */
  protected: // Functions you can override in custom thread entities
    virtual void
      donate_stack_block(kdu_byte *buffer, int num_bytes) { return; }
      /* This function may be called when a thread function starts up, to
         donate a portion of its stack for use as working memory.  The
         default implementation does nothing.  However, this function may
         be overridden within a derived class to do something useful with
         the storage.  In practice, `kdu_thread_env' overrides this function
         and donates the storage for use with the `kdu_block::map_storage'
         function, so that storage used by efficient block encoding/decoding
         machinery can be kept on the stack (unless it is too big to fit).
         This can help reduce memory fragmentation to gain some increments
         in cache utilization efficiency. */
  private: // Data
    friend kdu_thread_startproc_result
           KDU_THREAD_STARTPROC_CALL_CONVENTION worker_startproc(void *);
    friend class kdu_thread_context;
    friend class kdu_thread_queue;
    friend struct kd_thread_group;
    friend struct kd_thread_domain;
    int thread_idx; // Index within `group->threads'
    kdu_thread thread;
    kd_thread_group *group; // Created by the owning thread entity.
    kd_thread_grouperr *grouperr; // Access to group failure conditions
    kd_thread_job_hzp *hzp; // Points to this thread's hzd ptr within `group'
    kd_thread_domain *thread_domain; // See below
    int num_work_domains; // Valid elements in the following array; see below
    kd_thread_domain_sequence *work_domains[KDU_MAX_DOMAINS]; // See below
    int alt_work_idx; // See below
    int job_counter; // Number of jobs done since thread started
    int yield_counter; // Counts with `job_counter' but wraps at `yield_freq'
    int yield_freq; // 0 if yielding is disabled
    bool yield_outstanding; // True if yield required after next job
    bool first_wait_safe; // Set by `declare_first_owner_wait_safe'
    bool in_process_jobs; // True if the thread is executing in `process_jobs'
    bool in_process_jobs_with_cond; // As above with non-NULL `cond' argument
    int group_mutex_lock_count; // Allows safe recursive locking of group mutex
  private: // Thread-specific palette heap
    int next_palette_idx;
    kd_thread_palette *palette_heap[KD_THREAD_PALETTES];
  private: // Thread conditions
    kdu_thread_entity_condition *cur_condition; // Top of the condition stack
    kdu_thread_entity_condition *free_conditions; // Recycling list
    kdu_thread_entity_condition condition_store[KDU_INITIAL_THREAD_CONDITIONS];
  };
  /* Notes:
        The `condition_store' array holds a fixed set of "pre-allocated"
     `kdu_thread_entity_condition' objects that are available without
     recourse to dynamic memory allocation.  If additional condition objects
     need to be allocated, they are marked as such using the
     `kdu_thread_entity_condition::is_dynamic' flag.
        The `cur_condition' member points to the object's current thread
     condition, upon which the thread may wait; this is the object returned
     by a call to `get_condition'.  Each time the `process_jobs' function
     enters, it pulls an object from the `free_conditions' list and adds
     it to the top of the condition stack, making `cur_condition' point to
     this new object.  Each time the `process_jobs' function returns, it
     pops an object from the top of the condition stack, returning it to
     the `free_conditions' list and adjusting `cur_condition' accordingly.
     The group owner starts out with a non-NULL `cur_condition' pointer
     because it can do work without ever entering `process_jobs'.
        The `thread_domain' member identifies the domain to which a thread
     belongs, as supplied in the `add_thread' function.  At least the
     group owner will belong to the default domain that has no actual
     domain name.
        Only the first `num_work_domains' entries of the `work_domains' array
     are valid.  All of these contain non-NULL pointers that correspond
     to domains in which the thread can do work.  Noting that no jobs
     can be scheduled in the default domain, it follows that the default
     domain has no entry in the `work_domains' list.  It also follows that
     threads that belong to the default domain may initially have empty
     `work_domains' arrays.  If a thread belongs to a specific domain
     (not the default one), the first entry in its `work_domains' array
     points to an object that is associated with that domain.  Other
     entries in the array are used to keep track of work that can be
     done in other domains.  Each pointer in the `work_domains' list
     references an entry in the relevant domain's sequence-list.
     Each element of the sequence list manages jobs scheduled by the
     thread queues that have the corresponding sequence index.  Each thread
     makes its own way through each domain's sequence list, adjusting
     associated counters so that disused entries in the list can be cleaned
     up.
        The `alt_work_idx' member helps to balance the way in which threads
     do work within domains other than one with which they may be associated.
     Specifically, `alt_work_idx' hold the index of the next entry in
     the `work_domains' array to be considered when looking for jobs to do.
     If the thread does not belong to the default domain, the first entry
     of the `work_domains' array is special, as identified above, and
     so `alt_work_idx' should never be 0.
  */

/*****************************************************************************/
/*                      kdu_thread_dependency_monitor                        */
/*****************************************************************************/

class kdu_thread_dependency_monitor {
  /* [SYNOPSIS]
       Abstract base class for dependency monitors that may be used to
       detect important transitions in the number of dependencies
       associated with a `kdu_thread_queue' object.  See the introductory
       comments associated with `kdu_thread_queue', as well as the
       `kdu_thread_queue::set_dependency_monitor' function, for an explanation
       of this object's purpose.
       [//]
       The object is primarily of interest for `kdu_thread_queue' objects
       that are internally allocated via the `kdu_thread_entity::add_queue'
       function.  One reason for this is that internally allocated queues
       cannot be sub-queues of an application supplied queue, so there is
       no way to capture dependency changes that propagate through such
       queues via a derived parent queue with an overridden
       `kdu_thread_queue::update_dependencies' function.
  */
  public: // Member functions
    virtual ~kdu_thread_dependency_monitor()=0;
      /* [SYNOPSIS]
           This is a pure virtual destructor; it must be implemented
           in a derived class.
      */
    virtual void update(int new_dependencies,
                        int delta_max_dependencies,
                        kdu_thread_entity *caller)=0;
      /* [SYNOPSIS]
           When this object is attached to a `kdu_thread_queue' object
           using `kdu_thread_queue::set_dependency_monitor', the present
           function is invoked with exactly the same arguments as any parent
           queue's `update_dependencies' function would be invoked if there
           were a parent.
           [//]
           This is a pure virtual member function; it must be implemented
           in a derived class.
      */
  };

/*****************************************************************************/
/*                              kdu_thread_queue                             */
/*****************************************************************************/

class kdu_thread_queue {
  /* [BIND: reference]
     [SYNOPSIS]
       Objects of this class are either internally allocated by
       `kdu_thread_entity::add_queue' or externally created and passed to
       `kdu_thread_entity::attach_queue', both of which leave the object
       linked into the relevant thread group, either as top-level queues
       or as descendants of other "super queues".
       [//]
       There are two types of thread queue objects: those that can schedule
       jobs and those that cannot.  Thread queues that cannot schedule jobs
       are still useful for organizing other queues into an appropriate
       hierarchy, for modifying the sequence indices that would otherwise
       be assigned to their descendants when added with
       `kdu_thread_entity::attach_queue', and for keeping track of "fan-out"
       type dependencies (see below).  When used as-is, without any
       derivation, the base `kdu_thread_queue' object cannot schedule jobs
       because the default implementation of its `get_max_jobs'
       function returns 0.  The queues created internally by
       `kdu_thread_entity::add_queue' are all of this form.
       [//]
       All work done by the multi-threaded processing system must be
       initiated by calls to `kdu_thread_queue::schedule_jobs' (or the
       somewhat simpler, `kdu_thread_queue::schedule_job'), which can
       only occur within derived versions of this class.  Such a derived
       class must override `get_max_jobs' at the very least.  Once all
       jobs have completed, the derived class implementation must invoke
       `all_done'.
       [//]
       The object provides a mechanism for keeping track of dependencies
       between queues.  This mechanism is implemented via the overridable
       `update_dependencies' function, which is typically invoked from
       other queues (typically, but not necessarily descendant queues).
       The base implementation of this function assumes a "fan-out" style of
       dependency between a queue and its descendants, by which we mean that
       a queue is considered to have blocking dependencies if any of its
       descendants has a blocking dependency.  The base implementation of
       `update_dependencies' implements interlocked atomic accumulators that
       keep track of the number of descendants that appear to have blocking
       dependencies as well as the number of descendants that are able to
       have blocking dependencies at any point in the future.  If the
       number of depencencies transitions to 0, any parent queue's
       `update_dependencies' function is used to notify the parent that it
       has one less blocking dependency.  Conversely, if the number of
       dependencies transitions away from 0, any parent queue's
       `update_dependencies' function is used to notify the parent that it
       has an additional blocking dependency.
       [//]
       There are several ways for an application to use this dependency
       information:
       [>>] Sophisticated implementations can derive new objects from
            `kdu_thread_queue', overriding `update_dependencies' and using
            the dependency signalling mechanism to schedule jobs as soon as
            they are free from blocking dependencies.  The information
            supplied regarding the number of possible future dependencies
            can be used to help the object determine when to set the
            `all_scheduled' argument to true in calls to
            `schedule_jobs' or `schedule_job'.
       [>>] Another strategy is to use the `set_dependency_monitor'
            function to install a `kdu_thread_dependency_monitor' object
            within the queue; the base implementation invokes any such
            object's `kdu_thread_dependency_monitor::update' function whenever
            the number of dependencies transitions from <= 0 to > 0 or from
            > 0 to <= 0.
       [>>] Finally, it is possible to receive dependency notifications
            through a parent, because the parent queue's `update_dependencies'
            function is called each time the number of dependencies
            transitions to a positive value or away from a positive value.
       [//]
       If a queue has no parent and no `kdu_thread_dependency_monitor' object
       installed, the base implementation of `update_dependencies' actually
       does nothing.  This can be useful, since it saves the overhead
       of updating interlocked atomic variables, which incurs some delay.
       [//]
       Every thread queue that can schedule jobs must be able to keep
       track of the number of jobs that it has in-flight, in some way
       or another.  A job is in-flight if it has been scheduled via
       `schedule_jobs', but has not yet finished execution.  The
       need to keep track of the number of jobs that are in flight arises
       because the `kdu_thread_entity::terminate' function cannot
       reliably terminate all processing machinery (in a timely fashion)
       unless thread queues that schedule jobs are able to respond
       efficiently to a `request_termination' call.
  */
  public: // Member functions
    KDU_EXPORT kdu_thread_queue();
    KDU_EXPORT virtual ~kdu_thread_queue();
      /* [SYNOPSIS]
           If there is a risk that the queue might be destroyed from a context
           in which it is still attached to a thread group -- i.e.,
           an appropriate call to `kdu_thread_entity::handle_exception',
           `kdu_thread_entity::terminate' or `kdu_thread_entity::join'
           has not been issued -- you should consider invoking
           `force_detach' first as a measure (not entirely safe) or last
           resort.  See the description of that function for a more
           complete explanation.
      */
    bool is_attached() { return (this->group != NULL); }
      /* [SYNOPSIS]
           Returns true if the this object has been attached to a thread
           group via `kdu_thread_entity::attach_queue' (or if it was created
           using `kdu_thread_entity::add_queue') and the object has not
           yet been detached.  Queues are detached from a thread group by
           `kdu_thread_entity::join', `kdu_thread_entity::terminate',
           `kdu_thread_entity::handle_exception' or
           `kdu_thread_entity::destroy'.  Note that for queues created (and
           hence owned) by `kdu_thread_entity::add_queue', this function
           should never return false because once the queue is detached it
           is also destroyed -- i.e., you should not be calling this
           function beyond then.
      */
    KDU_EXPORT void force_detach(kdu_thread_entity *caller=NULL);
      /* [SYNOPSIS]
           This function is provided as a last resort prior to destroying a
           `kdu_thread_queue' object, in case there has been no call to
           `kdu_thread_entity::join' or `kdu_thread_entity::terminate' to
           detach the queue from any thread group to which it might still
           be attached.  This should not happen, but an application programmer
           could easily forget to invoke `kdu_thread_entity::terminate' if
           there is good reason to believe that all processing has finished
           for other reasons.  Nevertheless, the destructor should not be
           directly invoked without first completely detaching the queue
           from its thread group.
           [//]
           The present function does not wait for processing to finish
           within the queue or any of its descendants.  Instead, it just
           unlinks the queue and its descendants from the queue hierarchy.
           As noted above, this might produce unsafe results if processing
           is still continuing.
           [//]
           The function does nothing if `is_detached' returns false.
           [//]
           Note that the `caller' argument may be NULL if not known.  In
           fact, the function may be invoked from any thread at all, but
           any race conditions that this might cause (e.g., another thread
           destroying the thread group concurrently) are the caller's
           responsibility.
           [//]
           Because the function needs to lock an internal mutex belonging
           to the thread group, if `caller' is NULL, you must make sure
           that you never invoke this function (indirectly or directly)
           from within the `kdu_thread_queue::request_termination' function,
           since that function is itself called from a context in which the
           same mutex is locked, leading to an inevitable deadlock, which
           cannot be detected and avoided using the `caller' argument --
           Kakadu's core `kdu_mutex' object is not necessarily safe against
           recursive locking.
      */
    kdu_long get_sequence_idx() { return sequence_idx; }
      /* [SYNOPSIS]
           Returns the sequence index used for jobs scheduled by this
           object's `schedule_jobs' function.  This index is the larger
           of the `min_sequence_idx' value passed to `add_queue' when the
           queue was added and the sequence index of any parent
           (`super_queue') that was passed in the same call, or possibly
           larger again if some adjustments in the sequence index turned
           out to be required.  See `kdu_thread_entity::attach_queue' for
           more on the interpretation of sequence indices.  Knowing a queue's
           sequence index can be useful in helping you create other queues
           with the same or a larger sequence index.
      */
  public: // Member functions you would override in a derived class that
          // get called from other parts of the system.  These functions
          // usually need to be overridden in any thread queue that
          // actually schedules jobs.
    virtual int get_max_jobs() { return 0; }
      /* [SYNOPSIS]
           Before attaching the object to a thread group using
           `kdu_thread_entity::attach_queue', a derived class must
           know the maximum number of `kdu_thread_job' objects it may
           need to work with.  This is an upper bound on the maximum
           number of jobs that can be scheduled (but not yet launched) at
           any given time.  
           [//]
           The default implementation of this function returns 0, meaning
           that you cannot call `schedule_jobs' without first overriding
           this function with one that returns a non-zero value.
           [//]
           If this function returns N > 0 the `kdu_thread_entity::attach_queue'
           function assigns sufficient resources for the queue to have at
           most N jobs in-flight at any given time.  It is worth explaining
           here how job scheduling actually works in a derived
           `kdu_thread_queue'.  The basic elements are as follows:
           [>>] Jobs are implemented by deriving a suitable processing
                class from `kdu_thread_job' -- usually, these belong to the
                derived `kdu_thread_queue' object, but they might be used
                by multiple queues.
           [>>] To schedule jobs at any point after the call to
                `kdu_thread_entity::attach_queue', the `bind_jobs' function
                must be used to bind the relevant `kdu_thread_job' objects
                with internal resources that were transferred to the queue in
                response to its `get_max_jobs' function.  Usually, the
                `bind_jobs' function is called only once, after which
                `schedule_jobs' may be invoked any number of times. In some
                cases, it may be useful to pass `kdu_thread_job' objects
                between different queues.  In this case, the `bind_jobs'
                function may need to be called prior to each call to
                `schedule_job' or `schedule_jobs'; alternatively, the
                `schedule_job' can be invoked with an "auto-bind" directive.
      */
    virtual void
      set_dependency_monitor(kdu_thread_dependency_monitor *monitor)
        { this->dependency_monitor = monitor; }
      /* [SYNOPSIS]
           The role of this function is explained in the introductory
           notes for the `kdu_thread_queue' object.  Any dependency
           changes that might be passed to a parent queue's
           `update_dependencies' function are also passed to the
           `monitor->update' function, regardless of whether or not there
           is a parent queue.  If there is a parent queue, the
           `monitor->update' function is called first.
           [//]
           If you choose to call this function explicitly, you will
           probably want to do so before any calls to `update_dependencies'
           are generated from elsewhere, since such calls will be
           disregarded if there is neither a parent queue nor a
           dependency monitor in place.
           [//]
           In practice, dependency monitors are most likely to be used only
           with queues that are created by `kdu_thread_entity::add_queue'.
           That function is takes an intended dependency monitor for the
           created queue as its first argument, so you will rarely need
           to explicitly invoke the present function explicitly.
      */
    virtual bool
      update_dependencies(kdu_int32 new_dependencies,
                          kdu_int32 delta_max_dependencies,
                          kdu_thread_entity *caller)
      { 
        if (skip_dependency_propagation)
          return false;
        assert(caller != NULL);
        if (delta_max_dependencies != 0)
          { // Note: we expect this arg to be 0 in the majority of calls
            kdu_int32 old_max =
              max_dependency_count.exchange_add(delta_max_dependencies);
            kdu_int32 new_max = old_max + delta_max_dependencies;
            if ((old_max <= 0) && (new_max > 0))
              delta_max_dependencies=1;
            else if ((new_max <= 0) && (old_max < 0))
              delta_max_dependencies=-1;
            else
              delta_max_dependencies = 0;
          }
        kdu_int32 old_val = dependency_count.exchange_add(new_dependencies);
        kdu_int32 new_val = old_val + new_dependencies;        
        if ((old_val <= 0) && (new_val > 0))
          { 
            if (!propagate_dependencies(1,delta_max_dependencies,caller))
              skip_dependency_propagation = true;
          }
        else if ((new_val <= 0) && (old_val > 0))
          { 
            if (!propagate_dependencies(-1,delta_max_dependencies,caller))
              skip_dependency_propagation = true;
          }
        else if (delta_max_dependencies != 0)
          { 
            if (!propagate_dependencies(0,delta_max_dependencies,caller))
              skip_dependency_propagation = true;
          }
        return !skip_dependency_propagation;
      }
      /* [SYNOPSIS]
           The base implementation of this function provides an automatic
           mechanism for passing information about potentially blocking
           dependencies between a queue and its parent, allowing applications
           to check whether or not a potentially blocking dependency exists
           within any sub-tree of the queue hierarchy.  More sophisticated
           processing engines may derive from `kdu_thread_queue', overriding
           this function and using the derived information to schedule jobs
           in such a way that the scheduled jobs should be free from
           dependencies.  This certainly happens within the implementation
           of `kdu_encoder', `kdu_decoder', `kdu_multi_analysis' and
           `kdu_multi_synthesis', but the approach may be extended to any
           application-defined class that wants to participate in an
           efficient multi-threaded system.
           [//]
           The function's two arguments are normally interpreted as follows:
           [>>] `new_dependencies' is interpreted as a change in the number
                of potentially blocking dependencies represented by objects
                that invoke this function.  Typically, each such object is
                either in a potentially blocking state or not; if so, the
                next attempt to "use" some service offered by the object
                might block the caller; otherwise, the next attempt to use
                the service will not block the caller.  With this in mind,
                we would normally expect `new_dependencies' to take values
                of 0, 1 or -1, where 1 means that there is one more object
                that might block progress of machinery that relies on the
                services of all objects that call into this function, while
                -1 means that there is one less object that might block
                such progress.
           [>>] `delta_max_dependencies' is interpreted as a change
                in the maximum number of possible dependencies.  This means
                that the sum of all `new_dependencies' values passed to
                this function should not exceed the sum of all
                `delta_max_dependencies' values supplied to the function.
                However, if calls to this function arrive from different
                threads, it is possible that this condition is temporarily
                violated.  For an implementation to determine that there are
                no future potential blocking dependencies, it should wait
                for the condition in which the cumulative sum of all
                `new_dependencies' values passed to this function AND the
                cumulative sum of all `delta_max_dependencies' values supplied
                to this function to both become 0.  One way to
                determine this in a robust way is to keep a single interlocked
                variable that stores both sums.
           [>>] Objects that invoke this function should pass a positive
                value for `delta_max_dependencies' in their first call to this
                function (unless they will never call the function),
                identifying the maximum value of the cumulative
                sum of all `new_dependencies' values they will pass to the
                function in the future.  Negative values for
                `delta_max_dependencies' should then be passed only in their
                final call to this function, so that the sum of supplied
                `delta_max_dependencies' values returns to 0.  Typically, each
                object that presents a service with possibly blocking
                dependencies passes `delta_max_dependencies'=1 in its first
                call to this function and `delta_max_dependencies'=-1 in its
                last call to this function, because, as mentioned above, the
                cumulative sum of `new_depenencies' values supplied by each
                such object is always either 1 or 0.
           [>>] Some derived thread queue implementations will use only the
                `new_dependencies' values, ignoring the values passed via
                `delta_max_dependencies'.
           [//]
           The base implementation maintains two internal counters, D and M,
           that are initialized to 0 and implements a fan-out interpretation
           of dependencies in order to propagate them to any parent queue or
           dependency monitor.  Specifically, each call to this function
           atomically adds `new_dependencies' to D and `delta_max_dependencies'
           to M.  Following the policy described above, one would expect
           0 <= D <= M to hold at all times.  However, in some cases different
           threads might be used to signal increments and decrements to the
           number of dependencies (this is exactly what happens in the
           implementation of `kdu_decoder' and `kdu_encoder' objects), so
           the calls to increase and to decrease the number of dependencies
           might experience different delays and arrive out of order, leading
           to the possibility that instantaneous values of D could be < 0 and
           instantaneous values of M could potentially be smaller than D.
           However, it should never happen that M becomes negative, so long
           as all calls that supply positive values of `delta_max_dependencies'
           arrive during object creation, before anything happens that could
           generate calls with negative values from any thread.
           The base implementation propagates dependency information to any
           parent queue (or installed `kdu_thread_dependency_monitor') as
           follows:
           [>>] If the value of D transitions from <= 0 to > 0, the overall
                system represented by this queue and its service objects is
                considered to present a potentially blocking dependency to
                its parent, so the parent queue's `update_dependencies'
                function is invoked with a `new_dependencies' argument of 1.
                Conversely, if D transitions from > 0 to <= 0, the parent
                queue's `update_dependencies' function is invoked with a
                `new_dependencies' argument of -1.
           [>>] If the value of M transitions from <= 0 to > 0, the overall
                system represented by this queue and its service objects is
                considered to be able to present blocking dependencies, so
                any parent queue's `update_dependencies' function is invoked
                with a `delta_max_dependencies' argument of 1 -- as noted
                above, this typically happens during object creation/startup.
                Conversely, if the value of M transitions from > 0 to <= 0,
                the overall system represented by this queue and its service
                objects is considered to be unable to present any further
                blocking dependencies, so any parent queue's
                `update_dependencies' function is invoked with a
                `delta_max_dependencies' argument of -1.  As stated above,
                we do not expect M ever to become -ve, but it is possible
                that M becomes zero before D becomes 0, if a call that
                notionally decrements D before the one decrementing M is
                substantially delayed.  The base implementation itself does
                not ever compare M with D.
           [//]
           To put the above discussion into context, we now give some
           examples of "potentially blocking dependencies".
           [>>] A `kdu_decoder' object which does not currently have any
                buffered rows of decoded subband samples represents a
                potentially blocking dependency for any processing agent
                that might need to invoke `kdu_decoder::pull'.  Any
                attempt to pull synthesized image samples from a
                `kdu_synthesis' object that involves this `kdu_decoder'
                amongst its dependencies may potentially block, depending
                on whether or not the `kdu_synthesis::pull' call required
                new data from that subband.
           [>>] Similarly, a `kdu_encoder' object whose subband sample buffer
                is currently full (waiting for encoding) represents a
                potentially blocking dependency for any processing agent
                that might need to invoke `kdu_encoder::push'.  This kind
                of dependency might block a `kdu_analysis::push' call if
                the wavelet transform operation generates new data for that
                subband.
           [>>] A `kdu_multi_synthesis' object presents a potentially blocking
                dependency to other objects that use it, if the availability
                of data for any image component is questionable -- this
                happens if that image component's DWT synthesis tree involves
                potentially blocking dependencies and there are no buffered
                image lines available for the component.  A
                `kdu_multi_synthesis' object that identifies itself as
                unblocked has the property that at least one line can be
                pulled from each active image component without risk of
                blocking the caller.  Applications might create larger
                queues to manage the `kdu_multi_synthesis' dependencies
                associated with a whole row of tile processing engines,
                each represented by a `kdu_multi_synthesis' object.
           [>>] Similarly, a `kdu_multi_analysis' object is considered
                unblocked if at least one line can be pushed into each
                active image component without blocking the caller;
                otherwise, it will present itself as having potentially
                blocking dependencies.
           [//]
           If `update_dependencies' is overridden in a derived class,
           alternate interpretations may be ascribed to the `new_dependencies'
           and `delta_max_dependencies' arguments for custom applications,
           so long as these are understood by other queues that might invoke
           the object's `update_dependencies' function while doing work.
           The `kdu_decoder' object maintains an internal queue for
           block decoding operations whose `update_dependencies' function
           may be invoked by asynchronous code-block parsing operations with
           a custom interpretation for the set of code-blocks for which
           compressed data has been parsed by the machinery embodied via
           the `kdu_codestream' interface.
           [//]
           You should remember that this function is generally invoked
           asynchronously -- potentially from any thread.  Therefore, if
           you do choose to override the function, it is your responsibility
           to synchronize access to any state information that threads
           might otherwise access concurrently.
           [//]
           It is worth noting that objects which use this function to
           avoid scheduling jobs until there are no dependencies present
           a major risk of deadlock to the overall multi-threaded system.
           This is pretty much an unavoidable price to pay for the efficiency
           that arises from scheduling work only when the dependencies are
           sorted out.  The implementation and invocation of this
           function must be undertaken with the greatest of care.  Kakadu's
           core system provides a number of highly efficient data processing
           entities that override this function so as to schedule work that
           should not block.  These data processing entities, such as
           `kdu_encoder', `kdu_decoder', `kdu_multi_analysis' and
           `kdu_multi_synthesis' actually use interlocked atomic variables
           for synchronization, as opposed to a simpler, more conventional
           critical section approach based on mutexes.  It is recommended
           that a first implementation of any queue-derived processing
           entity that schedules work based on a dependency analysis should
           use critical sections initially, to be quite sure of avoiding
           race conditions that could lead to a scheduling deadlock.  It is
           also recommended that all objects that present potentially blocking
           services implement a method based on
           `kdu_thread_entity::wait_for_condition' and
           `kdu_thread_entity::signal_condition' to ensure that appropriate
           blocking will actually occur if dependencies are not satisfied
           when a "parent" object attempts to use these services.
         [RETURNS]
           The function should return false only if it is known that
           dependency information is not of interest.  This in turn is used
           to determine a correct return value for the `propagate_dependencies'
           function.  The base implementation uses the return values to
           determine, on the very first call, whether dependency information
           is being used.  If not, the base implementation of this function
           recognizes that there is no further need to keep track of
           or propagate dependency information, which saves potentially
           time consuming manipulation of the synchronized state variables.
         [ARG: caller]
           Identifies the calling thread.  This is important if an
           override of the function uses the dependency information to
           schedule its own jobs, since `caller' must be passed to
           the `schedule_jobs' function.
      */
  protected: // Member functions you would either implement or call from
             // within a derived class.
    bool propagate_dependencies(kdu_int32 new_dependencies,
                                kdu_int32 delta_max_dependencies,
                                kdu_thread_entity *caller)
      { 
        if (parent != NULL)
          return parent->update_dependencies(new_dependencies,
                                             delta_max_dependencies,caller);
        if (dependency_monitor == NULL)
          return false;
        dependency_monitor->update(new_dependencies,delta_max_dependencies,
                                   caller);
        return true;
      }
      /* [SYNOPSIS]
           This function is provided for the benefit of implementations
           that override `update_dependencies' and want to pass on
           dependency information to a potential parent queue and/or
           dependency monitor in their own way.  If there is no parent or
           dependency monitor, the function does nothing;
           otherwise, it passes the supplied parameters to the parent's
           `update_dependencies' function and/or the dependency monitor's
           `kdu_thread_dependency_monitor::update' function, as appropriate.
           Dependency monitors are always notified first.
         [RETURNS]
           True if there is a dependency monitor or if dependencies appear
           to be of interest to a parent.
      */
    virtual void
      notify_join(kdu_thread_entity *caller) { return; }
      /* [SYNOPSIS]
           This function is invoked only on top-level queues within a
           thread group, when a call to `kdu_thread_entity::join'
           attempts to join upon their completion and `request_notification'
           has not been called.  The base implementation does nothing.  The
           main reason for providing this function is to facilitate the
           implementation of background processing queues which may not
           have any good way of knowing when to call `all_done'.
           Such a queue might use calls to this function to help it figure
           out when all its work is likely to be complete.
           [//]
           This function is overridden and used by the special background
           processing queue created within a `kdu_codestream' object that
           is used for multi-threaded processing.  These queues treat calls
           to the present function in the same way as calls to
           `request_termination'.  The system `kdu_thread_entity::join'
           function takes care to join upon top-level background processing
           queues last, so that this function is not called until other
           queues have been successfully joined upon.  This helps to ensure
           that global calls to `kdu_thread_entity::join' (those which specify
           no `root_queue') do not prematurely terminate background processing
           queues that might be needed by other scheduled jobs.  However,
           all this said, it is far better to avoid global calls to
           `kdu_thread_entity::join' and explicitly terminate background
           codestream processing machinery once everything else is done by
           calling `kdu_thread_env::cs_terminate'.
           [//]
           The function is delicate because it may be invoked from any
           thread and because the thread group's internal mutex is locked
           while this function is in progress.  For these reasons, the
           implementation should do as little as possible and return
           promptly.  Most importantly, the implementation should not
           directly or indirectly invoke any of the member functions
           advertised in this documentation, apart from `all_done'.
           Of course, other threads (e.g., those being used to execute
           jobs for the object) are free to invoke those functions).
      */
    virtual void
      request_termination(kdu_thread_entity *caller) { return; }
      /* [SYNOPSIS]
           This function may be invoked in response to a call to
           `kdu_thread_entity::terminate'.  The function is only called
           if necessary, which means that it will not be called if the
           queue does not schedule jobs and it will not be called if the
           `all_done' function has already been called.
           [//]
           It is expected that any derived `kdu_thread_queue' class that does
           schedule jobs will provide an implementation (override) for this
           function.
           [//]
           The function is delicate because it may be invoked from any
           thread and because the thread group's internal mutex is locked
           while this function is in progress.  Also, as explained below,
           implementations may need to invoke `all_done' from within this
           function yet they need to be extremely careful to ensure that
           this only happens when there are no scheduled jobs "in-flight"
           that may result in access to the derived thread queue's member
           variables or virtual functions after it has been cleaned up.
           [//]
           As a general rule, the implementation should do as little as
           possible and return promptly, being sure to satisfy the following
           expectations:
           [>>] The function is expected to ensure that further scheduling
                of jobs stops, or is stopped as soon as possible (there may
                be some timing uncertainty between asynchronous threads of
                course).
           [>>] The function must be able to unambiguously determine whether
                or not there are any jobs in-flight.  An in-flight job is one
                that has been scheduled (or is being scheduled) but whose
                job function has not yet returned.
           [>>] If there are in-flight jobs, the function must ensure that
                the last of these will invoke `all_done' before finishing;
                in this case, the present function should simply return
                without doing anything more.
           [>>] If there are no in-flight jobs, the function must itself
                invoke `all_done' before returning.
           [//]
           The above responsibilities ensure that the `all_done' function
           will eventually be called and that processing associated with the
           queue will stop as soon as possible.  You should not do any other
           processing inside this function.  Most importantly, you must be
           careful not to call `kdu_thread_queue::force_detach' or
           `kdu_thead_context::leave_group', either directly or
           indirectly from within this function, since those functions
           necessarily lock the thread group's internal mutex without
           any `kdu_thread_entity' argument to help them avoid
           recursive locks that could result in deadlock.
           [//]
           Although there is no possibility that the resources associated
           with a queue will be cleaned up while this function is in
           progress (because the lock that is held prevents the queue from
           being detached until the function returns, which also prevents
           calls to `kdu_thread_entity::join' or `kdu_thread_entity::terminate'
           from returning), there is a potential race scenario that you
           should consider when implementing this function, as follows:
           [>>] It is possible that a call to `request_termination' is
                triggered by an application recognizing that all processing
                within a queue is complete and wanting to be sure that the
                queue can be cleaned up.  Moreover, it is possible that this
                happens immediately after the last scheduled job for the
                queue completes and immediately before the thread that
                performed that job itself invokes `all_done'.  This can
                result in the second call to `all_done' (not from within
                this function) occurring after the queue is been cleaned
                up -- likely to result in a segmentation fault or other
                erratic and hard to trace behaviour.
           [>>] To avoid such situations, the implementation of the queue
                should be particularly careful in determining whether or
                not there are in-flight jobs.  The best way to ensure a
                robust implementation is to keep a record of jobs that have
                been scheduled (updated immediately before the call to
                `schedule_jobs', not after) and to have jobs remove themselves
                from this record immediately before returning from their job
                function.  Moreover, as jobs remove themselves from the
                record of "in-flight" jobs they should check for the condition
                in which they are the last in-flight job and a call to
                `all_done' is required; if so, they should leave the record
                untouched (so they continue to appear as "in-flight") and
                call `all_done' immediately before returning.  Meanwhile,
                calls to `request_termination' should invoke `all_done' if
                and only if the record indicates that there are no in-flight
                jobs at all.  This type of approach can be made to 100%
                robust with an entirely lockless programming model, for
                maximum efficiency in highly multi-threaded environments.
                Alternatively, the implementation can use critical sections
                to simplify the programming and correctness-proof effort.
      */
  protected: // Member functions you would call only from a derived class
    KDU_EXPORT void all_done(kdu_thread_entity *caller);
      /* [SYNOPSIS]
           If you are implementing a custom `kdu_thread_queue' object, this
           is a very important function to understand.  If `get_max_jobs'
           returned non-zero when the queue was attached to the thread group
           via `kdu_thread_entity::attach_queue', this function
           must be called from within the derived object once all jobs have
           been executed -- i.e., when the queue's work is finished.  What
           this means is that you implementation must keep track of jobs
           that have finished executing.
           [//]
           It is very easy to make the mistake of accessing elements of a
           derived `kdu_thread_queue' after `all_done' has been called.
           This can lead to unexpected race conditions in the following way:
           [>>] Typically, derived `kdu_thread_queue' objects belong to
                data processing machinery that is allocated on the heap and
                ultimately needs to be destroyed.
           [>>] Before deleting (directly or indirectly) a `kdu_thread_queue'
                object, the `kdu_thread_entity::join' or
                `kdu_thread_entity::terminate' function must be used to
                wait until all processing is done.
           [>>] In view of the above, any attempt to access a derived
                `kdu_thread_queue' object after its `all_done' function has
                been called might result in access to deallocated memory.
           [//]
           In view of the above, it is recommended that derived
           `kdu_thread_queue' objects invoke `all_done' as the final statement
           within a job processing function and that they invoke the function
           only from within job processing functions, unless they can be
           certain that all scheduled jobs (or about to be scheduled jobs)
           have been executed -- i.e., that there are no "in-flight" jobs.
           Functions offered by a derived `kdu_thread_queue' that are invoked
           only while the thread queue is attached to its thread group
           (typically, this includes functions accessed by a parent
           `kdu_thread_queue' such that the parent object must be all done
           before any attempt will be made to destroy it or any of its
           descendants) can safely be accessed beyond the call to `all_done'.
           [//]
           The internal implementation looks out for multiple calls to
           `all_done', doing nothing if the function has already been called
           (all tests are atomic and robust to access by concurrent threads).
           Nevertheless, unless you have good reasons to believe otherwise,
           it is an error if the implementation of a thread queue makes it
           possible (e.g., due to a race condition) for `all_done' to be
           called multiple times.  This is because, once `all_done' has been
           called, it is possible that a call to `kdu_thread_entity::join' or
           `kdu_thread_entity::terminate' will return and the caller will
           proceed to clean up the present object.
      */
    KDU_EXPORT void
      bind_jobs(kdu_thread_job * const jobs[], int num_jobs,
                kdu_uint32 range_start=0);
      /* [SYNOPSIS]
           An introduction to the `bind_jobs' and `schedule_jobs' functions
           is given in the comments appearing with `get_max_jobs'.
           [//]
           In most cases, `kdu_thread_job' objects are allocated from within
           a derived `kdu_thread_queue' object that keeps an array of
           references to the `kdu_thread_job' objects.  In these cases, the
           function is generally called only once, immediately after the call
           to `kdu_thread_entity::attach_queue'; the function binds the
           jobs in the supplied array to a collection of internal resources
           that were transferred from the thread group to the queue at the
           time it was attached.  Once this function has been called, the
           `schedule_jobs' function can be used to schedule jobs.
           [//]
           In other cases, `kdu_thread_job's may be used with more than one
           queue.  In this case, `bind_jobs' should be used to bind them to
           a suitable internal resource prior to calling the
           `schedule_jobs' function.
           [//]
           In most cases, the `num_jobs' argument supplied here will be
           identical to the value N returned by `get_max_jobs' and
           `range_start' will be 0; however, you may bind any sub-range of
           the N job references allocated to the queue by
           `kdu_thread_entity::attach_queue'.
         [ARG: jobs]
           Array of `num_jobs' pointers to instances of a class derived
           from `kdu_thread_job'.
         [ARG: num_jobs]
           This function binds `num_jobs' of the job references allocated
           by `kdu_thread_entity::attach_queue', skipping the first
           `range_start' allocated references.  It follows that
           `range_start'+`num_jobs' must be less than or equal to the
           value returned by `get_max_jobs'.
         [ARG: range_start]
           See `num_jobs' for an explanation.
      */
    KDU_EXPORT void
      schedule_jobs(kdu_thread_job * const jobs[], int num_jobs,
                    kdu_thread_entity *caller, bool all_scheduled=false);
      /* [SYNOPSIS]
           Use this function to schedule one or more jobs for execution on
           appropriate threads.  Within a queue, jobs are executed in the
           order that they are scheduled.  Currently, the internal
           implementation strives to execute all jobs within a domain in the
           order that they are scheduled; this is expected to yield the best
           utilization of resources.
           [//]
           When scheduling multiple jobs concurrently using this function,
           you do need to be able to guarantee that all of the jobs passed
           to this function will be executed (well, at least launched) before
           any of them is scheduled again.  Otherwise, you should schedule
           the jobs one at a time (not quite as efficient).
           [//]
           This function may throw an exception if any thread in the thread
           group to which the queue is attached has invoked
           `kdu_thread_entity::handle_exception'.
           [//]
           An error is generated if the queue has not been attached to the
           caller's thread group, or if `bind_jobs' has not been used
           correctly.
         [ARG: all_scheduled]
           This optional argument is usually false, but may be set to true
           if this is the last job scheduling call that will occur for
           the present queue.  If the implementation knows and can guarantee
           this, then some efficiency improvements can result for applications
           in which other queues with later sequence indices (see
           `kdu_thread_entity::attach_queue') also have jobs to perform.  The
           system keeps track of the number of queues that are using a
           given work domain, with a given sequence index; this number is
           decremented when the last job is scheduled, or when the `all_done'
           function is executed.  When this number reaches 0, all threads in
           the system become aware of the fact that the sequence is no longer
           active, so that processing of jobs from queues with later sequence
           indices becomes their first priority.  By setting this argument to
           true when you schedule the last job for a queue, you ensure that
           this process happens as early as possible.
           [//]
           Note that it is usually very difficult to robustly identify the
           `all_scheduled' condition if multiple threads are able to
           concurrently schedule jobs for the same thread-queue.  It is
           easy to accidentally believe that a thread is the last one to
           schedule jobs, just because all other scheduling threads appear
           to have performed their scheduling.  However, one of those threads
           could be delayed indefinitely within the call to this function,
           causing the scheduling calls to become misordered.
      */
    KDU_EXPORT void
      schedule_job(kdu_thread_job *job, kdu_thread_entity *caller,
                   bool all_scheduled=false,
                   int bind_options=KDU_THREAD_JOB_AUTO_BIND_ONCE);
      /* [SYNOPSIS]
           This function is provided as an alterative to `schedule_jobs'
           for cases in which jobs are scheduled one at a time.  Be sure to
           read the documentation of `schedule_jobs' first, if there is
           anything you do not understand about this function.
           [//]
           If the `bind_jobs' function has not yet been invoked, this function
           can automatically perform the required binding operation,
           based on the supplied `bind_options' argument.
           [//]
           Note that automatic binding can become inefficient if a queue has
           many jobs, and may not be appropriate at all in some circumstances.
           If you have many jobs or are using your jobs in an unusual manner,
           you should always resort to the explicit use of the `bind_jobs'
           and `schedule_jobs' functions.
         [ARG: bind_options]
           Takes one of the following values:
           [>>] 0 -- in this case, the supplied `job' must already have
                been bound via a call to `bind_jobs' or a previous call to
                this function with a non-zero `bind_options' argument.
                If the `job' has not already been bound, an error will be
                generated through `kdu_error'.
           [>>] `KDU_THREAD_JOB_AUTO_BIND_ONCE' -- in most cases, this is the
                option you should select when using this function (it is also
                the default).  If the `job' is already bound, no change is
                made; otherwise, the function automatically binds `job' to
                the "next" available internal resource that was allocated
                by `kdu_thread_entity::attach_queue'; the number of such
                resources is the value returned by `get_max_jobs' during
                queue attachment.   Auto-binding uses an internal atomic
                counter to keep track of the next internal resource to be
                used, so this function can safely be called asynchronously
                from multiple threads.  If all such resources have already
                been bound, or this function has previously been used with
                the `KDU_THREAD_JOB_REBIND_0' option, an error is generated
                through `kdu_error'.
           [>>] `KDU_THREAD_JOB_REBIND_0' -- in this case the function ignores
                any internal resource to which `job' might already
                be bound and automatically rebinds it to the very first
                resource that was allocated during the call to
                `kdu_thread_entity::attach_queue' function.  This can be
                useful if `job' is scheduled sometimes on one queue and
                sometimes on another, but it is not generally appropriate
                for queues that have multiple jobs.  An error will be
                generated through `kdu_error' if you attempt to mix this
                option with `KDU_THREAD_JOB_AUTO_BIND_ONCE' on the same
                queue.
      */
    KDU_EXPORT void note_all_scheduled(kdu_thread_entity *caller);
      /* [SYNOPSIS]
           This function provides an alternate mechanism for informing the
           internal machinery that all jobs that will ever be scheduled via
           this queue have been scheduled.  The preferred mechanism is to
           set the optional `all_scheduled' argument of `schedule_jobs' or
           `schedule_job' to true.  However, this function can come in handy
           if a running job discovers, while running, that it is the last
           job that will ever be scheduled.
           [//]
           As a general principle, you should avoid using this function
           except from within the context of a running job that was
           scheduled via the queue.  In such a context, there is absolutely
           no risk that the queue will reach the point where all executing
           jobs finish running and the `all_done' function is called while
           this function is still being called -- this can result in
           complex race conditions if calls to `all_done' release other
           threads to clean up the queue's resources.
           [//]
           Of course, when invoking this function you do need to be quite
           sure that no other thread can invoke `schedule_job' or
           `schedule_jobs' again.
      */
  private: // Helper functions
    void all_complete(kdu_thread_entity *caller);
      /* Called from `all_done' to detach the queue from its
         domain sequence, decrement the completion count field C, within
         `completion_state', and take appropriate action to propagate
         these changes to parent queues and wake joining threads as
         required. */
    void link_to_thread_group(kdu_thread_entity *caller);
      /* This function is called only from a context in which the thread
         group's mutex is locked.  The function adds the current object
         into the group, setting its `group', `next_sibling' and
         `prev_sibling' members accordingly.  If `parent' is non-NULL, the
         object is linked into the parent's `descendants' list; otherwise it
         is linked into the `group's `top_queues' list. */
    void unlink_from_thread_group(kdu_thread_entity *caller,
                                  bool unless_belongs_to_group=false);
      /* This function is called only from a context in which the thread
         group's mutex is locked.  The `group' member must be non-NULL.
         If `parent' is non-NULL, the function unlinks the object from the
         parent queue's `descendants' list; otherwise, it unlinks the object
         from the thread group's `top_queues' list.
            This function invokes itself recursively, as required to unlink
         any remaining descendant queues.
            Note that this function may be called from within (or in
         connection with) a call to `handle_exception'.
            If `unless_belongs_to_group' is true, the function does not unlink
         queues that are marked with the `belongs_to_group' flag, but it
         still passes recursively through its descendants, which may cause
         some of them to be unlinked.
            Note that if the `belongs_to_group' flag is true and
         `unless_belongs_to_group' is false, the function actually deletes
         the queue, after returning from its recursive descent.  Thus, the
         function should always be considered self-efacing. */
  private: // Data
    friend class kdu_thread_entity;
    kd_thread_group *group; // Non-NULL if we have been added to the group
    bool belongs_to_group; // If created by `add_queue'
    int flags; // Copied from call to `kdu_thread_entity::attach_queue'
    kdu_thread_queue *next_sibling; // Thread queues are held in a doubly
    kdu_thread_queue *prev_sibling; // linked list for safe cleanup
    kdu_thread_queue *parent;
    kdu_thread_queue *descendants;
    kdu_thread_dependency_monitor *dependency_monitor;
    bool skip_dependency_propagation; // See `update_dependencies'
    kdu_long sequence_idx;
    kd_thread_domain_sequence *domain_sequence;
    const char *last_domain_name; // Used for debugging strange conditions
    int registered_max_jobs; // Non-zero if queue can schedule jobs
    kd_thread_palette_ref *palette_refs; // List of `registered_max_jobs' refs
  private: // Synchronization variables
    kdu_interlocked_int32 completion_state; // See below 
    kdu_thread_entity_condition *completion_waiter; // See below
    kdu_interlocked_int32 auto_bind_count; // Num previous auto-bind calls
    kdu_interlocked_int32 dependency_count;     // DO NOT exposes these as
    kdu_interlocked_int32 max_dependency_count; // protected members!
  };
  /* Notes:
     If `parent' is non-NULL, the `next_sibling' and `prev_sibling'
     members manage a doubly-linked list that is headed by the `parent'
     object's `descendants' pointer.  Otherwise, `next_in_group' and
     `prev_in_group' manage the queue's membership of the doubly-linked
     list headed by `group->top_queues'.
     [//]
     The `palette_refs' member points to a list of `registered_max_jobs'
     palette references, connected via their `next' members.
     This list is created within the `kdu_thread_entity::attach_queue'
     function and used to implement the `bind_jobs' function and the
     auto-bind capabilities of the `schedule_job' function.  The `all_done'
     function transfers all elements of this list back to the thread
     group from which they were allocated.
     [//]
     The `completion_state' member has a rich structure, designed so as to
     minimize the amount of interaction required with atomic interlocked
     variables.  The structure is as follows:
     -- S (bit-0) is a 1-bit flag that holds 1 if the `all_scheduled'=true
        condition has NOT YET been encountered in a call to `schedule_jobs' or
        `schedule_job'.
     -- D (bit-1) is a 1-bit flag that holds 1 if the `all_done' function has
        NOT YET been called.
     -- T (bit-2) is a 1-bit flag that holds 1 if D is non-zero at the
        point when `request_termination' is called.  In this case, once
        D becomes zero (i.e., once `all_done' is called), the termination
        request must be propagated to the queue's descendants.
     -- W (bit-3) is a 1-bit flag that holds 1 if a call to
        `kdu_thread_entity::join' or `kdu_thread_entity::terminate' is
        waiting for the completion counter C to become 0.  If this bit is
        set when C becomes 0, the group mutex should be locked first and then
        the `completion_waiter' member should be accessed to determine the
        condition that needs to be signalled, resetting `completion_waiter'
        to NULL before releasing the group mutex again.
     -- C (bits 4,5,...) counts the total number of queues (including the
        current one and all of its immediate descendants for which the
        completion counter is non-zero) that are preventing a current or
        future call to `kdu_thread_entity::join' from progressing.  If
        either the D or S bits are set, the current queue contributes 1 to
        the count C.  Once `all_done' and `all_complete' have been called,
        the value of C is decremented by 1.  If this leaves C=0, any
        parent queue's completion count C is also decremented by 1, and
        so forth.  Importantly, once D and S become 0, the completion count
        C need not be decremented immediately; this can be deferred until
        after all interaction with the queue from within `all_done' or
        `all_complete' is over, avoiding possible race conditions with
        an asynchronous call to `kdu_thread_entity::join' that might encounter
        the C=0 condition, return and clean up with queue.
     [//]
     It can happen that multiple threads attempt to join on completion of a
     queue.  To deal with this situation, the `completion_waiter' is
     manipulated only while the group mutex is held.  If a thread attempts
     to join on completion of the queue and needs to wait, it deposits its
     condition reference into the `completion_waiter' member and sets the
     `W' flag in `completion_state', but it remembers the previous contents
     of the `completion_waiter' member.  Once signalled with the completion
     event, this thread signals any pre-existing `completion_waiter' object,
     which ensures that the event gets propagated down the chain of all
     joined threads.
     [//]
     It is possible that multiple threads attempt to `join' to different
     points in a queue's ancestry.  In this case, the leaves will receive
     termination signalling before their ancestors, but there is no guarantee
     which thread will wake up first to lock the group mutex and detect
     termination.  In this way, it can happen that a thread cleans up
     a sub-tree of the queue hierarchy while other threads are still waiting
     to clean up queues within the sub-tree.  This does not present a problem
     so long as we are sure that the successful `join' calls will not
     provoke application code to actually delete (or re-use) the
     `kdu_thread_queue' objects asynchronously.
   */

/*****************************************************************************/
/*                               kdu_thread_job                              */
/*****************************************************************************/

typedef void (*kdu_thread_job_func)(kdu_thread_job *, kdu_thread_entity *); 

class kdu_thread_job {
  /* [SYNOPSIS]
       Objects of this class are passed to `kdu_thread_queue::add_job'.
       The object deliberately offers no virtual functions -- so there cannot
       be a virtual destructor and the actual function which does the work
       is stored in a function pointer rather than in a vtable as an
       overridable virtual function.  The reason for this is that objects
       derived from `kdu_thread_job' may be nothing other than specially
       allocated blocks of storage that are designed as sandboxes for
       threads to play around in without interfering with the memory used
       by other threads.
       [//]
       Typically, you would allocate storage to a derived object, invoke
       `set_job_func' to set up the processing function and then pass the
       object to `kdu_thread_queue::add_job' for scheduling.  The processing
       function passed to `set_job_func' receives a pointer to the object
       itself as its first argument.  To invoke this function explicitly
       (outside the multi-threaded scheduling system) you may find it
       convenient to call the in-line `do_job' function.
  */
  public: // Member functions
    void set_job_func(kdu_thread_job_func func)
      { this->job_func = func; this->palette_ref = NULL; }
      /* [SYNOPSIS]
           Be sure to call this function before passing a reference to this
           object to the `kdu_thread_queue::bind_jobs' function.  Note
           that this function also removes the connection between this
           object and any internal thread queue resource to which it was
           bound by a prior call to `kdu_thread_queue::bind_jobs' or
           an auto-bind call to `kdu_thread_queue::schedule_job'. It follows
           that this function should always be called first, before binding.
      */
    void do_job(kdu_thread_entity *caller) { this->job_func(this,caller); }
      /* [SYNOPSIS]
           Normally invoked only from within the core multi-threading
           sub-system, to invoke the job function installed by the
           `set_job_func' function.  However, you can call this function
           directly.
      */
  private: // Data
      kdu_thread_job_func job_func;
      kd_thread_palette_ref *palette_ref; // NULL until job is bound
      friend class kdu_thread_queue;
      friend struct kd_thread_group;
  };

/*****************************************************************************/
/*                               kdu_run_queue                               */
/*****************************************************************************/

class kdu_run_queue : public kdu_thread_queue {
  /* [BIND: reference]
     [SYNOPSIS]
       This object provides a mechanism for implementing a potentially very
       efficient multi-threaded data processing paradigm, based on the
       dependency propagation funcionality provided by `kdu_thread_queue'
       derived objects.  To understand this paradigm, we recommend that
       you begin by thoroughly reading the discussion appearing with the
       `kdu_thread_entity::declare_first_owner_wait_safe' function and
       then read the discussion below.
       [//]
       The basic idea behind the intended processing paradigm is to
       perform all processing steps and calls that would normally be
       performed by the group owner thread from within a special "processor"
       job that can be scheduled to run on any thread.  Typically, a
       data processing application either pulls image samples from
       `kdu_multi_synthesis' objects via `kdu_multi_synthesis::pull_line',
       or else it pushes image samples to `kdu_multi_analysis' objects
       via `kdu_multi_analysis::exchange_line'.  Alternatively, the lower
       level `kdu_analysis', `kdu_synthesis', `kdu_encoder' and/or
       `kdu_decoder' objects might be used; these derive from `kdu_push_ifc'
       and `kdu_pull_ifc' and also offer data push/pull functions.  The
       push/pull calls must arrive in sequence and are normally generated
       from a single thread, typically the owner of a thread group, whose
       worker threads are internally deployed to perform most of the work.
       In the processing paradigm described here, these calls still arrive
       in sequence, but they are delivered from within a special
       `kdu_thread_job' object's job function, and the purpose of this
       present object is to make sure that only one "processor" job can be
       running at any given time.  At the same time, the object allows the
       processor job to be run on the most appropriate thread, which
       may change over time.  This is achieved with the aid of the
       `check_continue' function.
       [//]
       Here is how you would typically implement this processing paradigm:
       [>>] Pass an instance of the present object as the `env_queue' argument
            when constructing/creating push/pull objects such as
            `kdu_multi_analysis', `kdu_multi_synthesis', `kdu_encoder',
            `kdu_decoder', `kdu_analysis' or `kdu_synthesis'.  This object
            collects dependency analysis information passed on by those
            objects.  The present object must first be attached to the
            thread environment using `kdu_thread_entity::attach_queue' -- more
            on this below.
       [>>] Derive your own class from `kdu_thread_job' and
            register the "processor" function using this object's
            `kdu_thread_job::set_job_func'.  Generally, there will only be
            one `kdu_thread_job' object, although there might be many
            instances of the `kdu_run_queue' object -- you do not need to
            derive a new class from `kdu_run_queue'.  Make sure that the
            `kdu_thread_job'-derived object, or one that it knows about,
            maintains sufficient state to pick up processing from where it
            left off, because the "processor" job function needs to be able
            to run for a while, return before all work is complete, then
            be rescheduled and run again, potentially on a different thread.
       [>>] Provide a means for the group owner (or some other "master"
            thread) to wait until all processing is done.  One way to do
            this (perhaps the best way) is to build your `kdu_thread_job'
            derived object on top of `kdu_run_queue_job', which offers
            `kdu_run_queue_job::signal_done' and `kdu_run_queue_job::wait_done'
            functions.  In the remainder of this description, we will assume
            that this is what you do, although the present object can be
            used with other job object designs.  You should arrange for the
            "processor" job to invoke `signal_done' once all processing is
            complete.  Make sure that your `kdu_thread_job'-derived object
            knows about all relevant `kdu_run_queue' objects -- these might
            even be created from within your "processor" function.
       [>>] Your "processor" function will typically perform the push/pull
            calls on objects like `kdu_multi_analysis' or `kdu_multi_synthesis'
            for which the present `kdu_run_queue' object acts as a parent
            queue.  Typically, each time a line of image data has been pushed
            or pulled to all relevant image components, your "processor"
            function should invoke the all important `check_continue'
            function -- this is an extremely inexpensive call, implemented
            in-line, that often performs nothing other than a simple
            comparison on a variable that is likely to be in the calling
            thread's cache.  If the function returns true, you continue
            processing.  If the function returns false, your "processor"
            function must return, without doing anything else, confident
            in the knowledge that it will be scheduled to run again once
            the potentially blocking conditions that caused the false return
            have been cleared.
       [>>] From your master control thread (typically the group owner),
            invoke the `kdu_run_queue_job::do_job' function and then, once it
            returns, invoke `kdu_run_queue_job::wait_done'.  If the initial
            call to your "processor" function succeeded in doing all
            processing work before returning, it will have invoked
            `kdu_run_queue_job::signal_done' and so the call to
            `kdu_run_queue_job::wait_done' will return immediately.
            Otherwise, the "processor" function terminated upon discovery of
            a potentially blocking condition in some instance of the
            `kdu_run_queue' class, but another instance of the job will be
            scheduled once the blocking condition goes away, so that
            instance of the "processing job", potentially running on a
            different thread, will be the one that allows the call to
            `kdu_run_queue_job::wait_done' to return.  The
            `kdu_run_queue_job::wait_done' function enters a "working wait
            state" by invoking `kdu_thread_entity::wait_for_condition'.  In
            this state, your master thread (typically the group owner)
            itself may be the one that executes the next instance of the
            "processor" function, or else it may do other work within the
            system.  In any event, threads go idle only when there is no
            work anywhere to be done and the first available thread will
            execute the "processor" function as soon as the blocking
            dependencies have been cleared.
       [>>] Before you can schedule your special "processor" job via this
            object, you need to do the following: 1) invoke `activate';
            2) call `kdu_thread_entity::attach_queue', with the
            `KDU_THREAD_QUEUE_SAFE_CONTEXT' flag so that your
            "processor" job always runs in a safe context; and
            3) call `kdu_thread_entity::declare_first_owner_wait_safe'
            some time before you first call `kdu_run_queue_job::wait_done',
            following the advice given above.  Step 1 must be performed
            before step 2 so that jobs can be scheduled.  Step 3 is optional,
            but always recommended so long as the group owner is the one
            that calls `kdu_run_queue_job::wait_done'.  Moreover, if your
            thread group has only one thread (the group owner), step 3 is
            required; otherwise, there will be no context in which to run
            your "processor" job.  Step 3 would normally be performed
            immediately before the call to `kdu_run_queue_job::wait_done'
            mentioned above.
       [>>] Before attempting to invoke `kdu_thread_entity::join' on a
            run queue that has been activated (see `activate'), you
            must explicitly call `deactivate'; otherwise, the call to
            `kdu_thread_entity::join' will sit around forever, waiting for
            the queue to declare that it will no longer schedule any
            jobs.
       [//]
       Note that this object never invokes the
       `kdu_thread_queue::propagate_dependencies' function, so it does not
       pass dependency information up to parent queues.  This is because
       the whole point of the object is to catch dependency data and use
       it to manage the scheduling of the special "processor" function,
       through calls to `check_continue'.
  */
public: // Member functions
  kdu_run_queue()
    { is_active=false; acc_new_dependencies=0;
      num_dependencies.set(0); job_to_schedule=NULL; }
  virtual int get_max_jobs() { return (is_active)?1:0; }
    /* [SYNOPSIS]
         Returns 1, because instances of this object should never
         allow more than one job to be in-flight at any given time.
    */
  void activate()
    { 
      assert(!is_attached());
      is_active=true;
    }
    /* [SYNOPSIS]
         This function must be called before the queue is attached using
         `kdu_thread_entity::attach_queue', unless you have no intention
         of ever calling the `check_continue' function.  In the latter case,
         no jobs can be scheduled, so the queue can be attached as one that
         does no work and calls to `update_dependencies' will be discarded
         so that they present no computational burden at all.
         [//]
         Once attached, a `kdu_run_queue' object that has been activated
         needs to be deactivated before you can successfully join upon
         it (see `kdu_thread_entity::join').  The same goes for calls to
         `kdu_thread_entity::terminate'.  If you do need to support
         premature termination, you will have to provide a derived version
         of this object that ensures that implements the `request_termination'
         function and ensure that either that function calls `all_done'
         (if it is safe) or else the "processor" job calls `deactivate'.
    */
  void deactivate(kdu_thread_entity *caller)
    { 
      if (is_active && is_attached())
        this->all_done(caller);
      is_active=false;
    }
    /* [SYNOPSIS]
         This function does nothing unless `activate' was called.  In this
         case, it is essential that this function gets called after all
         calls to `check_continue' have occurred and before you attempt
         to join on the queue (via `kdu_thread_entity::join').  All this
         function actually does is call `all_done' and put the object in
         the inactive state.  If the `activate' function was not called,
         this function will not do anything, but is safe to call.
    */
  virtual bool update_dependencies(kdu_int32 new_dependencies,
                                   kdu_int32 delta_max_dependencies,
                                   kdu_thread_entity *caller)
    { 
      if (!is_active)
        return false;
      if (new_dependencies > 0)
        acc_new_dependencies += new_dependencies;
      else if ((new_dependencies < 0) &&
               (num_dependencies.exchange_add(new_dependencies) ==
                -new_dependencies))
        { // Dependency count has transitioned to 0
          assert(acc_new_dependencies == 0); // Job cannot be running
          assert(job_to_schedule != NULL);
          kdu_thread_job *job = job_to_schedule;  job_to_schedule = NULL;
          schedule_job(job,caller,false,KDU_THREAD_JOB_REBIND_0);
        }
      return true;
    }
    /* [SYNOPSIS]
         Implements the policy described in `check_continue'.  Specifically,
         this function accumulates positive values of `new_dependencies'
         in an internal state variable, assuming that all calls that involve
         such positive values arrive from within the thread that is
         currently executing the "processor" job function described in
         the introductory comments to `kdu_run_queue'.  You do need to
         be extremely careful that once `check_continue' returns false,
         your "processor" function returns immediately, without manipulating
         the state of this or any other object that is not inherently
         thread safe, since a new copy of the processor function could be
         running in another thread from that point onwards.
         [//]
         For efficiency reasons, this function ignores `delta_max_dependencies'
         and does not pass dependency information along to any parent queue.
         For this reason, instance of this object will usually be attached
         as top-level queues within the relevant thread group.
    */
  bool check_continue(kdu_thread_job *job, kdu_thread_entity *caller)
    { 
      if ((acc_new_dependencies == 0) || !is_active)
        return true;
      assert(job_to_schedule == NULL);
      job_to_schedule = job;
      kdu_int32 acc_val = acc_new_dependencies;  acc_new_dependencies = 0;
      kdu_int32 new_val = num_dependencies.exchange_add(acc_val)+acc_val;
      assert(new_val >= 0);
      if (new_val > 0)
        return false; // Potentially blocking dependencies exist
      job_to_schedule = NULL; // Make sure can't have 2 jobs in flight at once
      return true;
    }
    /* [SYNOPSIS]
         This function embodies the key feature of the `kdu_run_queue'
         object.  The running "processor" (see introductory comments)
         should call this function if it thinks that new potential blocking
         conditions may have arisen on any of the present object's
         sub-queues.  If this turns out to be the case, the function
         returns false, meaning that the caller should return immediately
         so that it stops processing.  In this case, the function also sets
         things up so that the supplied `job' will be scheduled to run as
         soon as the blocking conditions are cleared; the `job' object is
         normally set up by the caller to continue running the "processor".
         If the function returns true, the running processor should continue
         its activities.
         [//]
         It is worth STRESSING that if the function returns false, it is
         almost certainly DANGEROUS for the caller to do anything other
         than return immediately from the running "processor" function.  It
         should not modify any state information that might be accessed
         when the "processor" function is launched again -- something that
         can be very tempting to do.  This is because the next instance of
         the "processor" function might be launched on a different thread,
         at any point after this function returns, or even slightly before it
         actually returns.  It can be very tempting to record the fact that
         the function returned false within some state variable that affects
         the next call to the function, but this would be a serious error.
         If you need to do that, record the false return status prior to
         calling this function and revoke it only after the function returns
         true, thereby avoiding potential race conditions with the next
         scheduled invocation of the processor function.
         [//]
         You should be sure that `activate' was called before a reference
         to this object was passed to `kdu_thread_entity::attach_queue' if
         you wish this function to behave in the manner described above.
         Otherwise, the function will just return true immediately, without
         checking anything, because run queues that have not been activated
         cannot schedule jobs.
         [//]
         Calls to this function normally amount to nothing other than an
         in-line comparison of a non-atomic internal state variable with
         zero -- i.e., they are extremely cheap and can be invoked as
         frequently as desired without any significant impact on processing
         efficiency.  To achieve this, the function assumes that all
         calls to the object's `update_dependencies' function that involve
         positive `new_dependencies' values are generated within the same
         thread as the running processor, while other calls may arise from
         different threads.  This assumption is certainly correct if the
         object's sub-queues are associated with `kdu_encoder',
         `kdu_decoder', `kdu_analysis', `kdu_synthesis',
         `kdu_multi_analysis' or `kdu_multi_synthesis' objects and all
         calls into these objects that push or pull image data are
         generated by the running processor -- this is the way you are
         expected to use this object.  This same pattern of behaviour would
         most likely be true for other processing queues that you might
         implement and use with this object, because hierarchical
         processing structures naturally encounter new dependencies only
         due to the action the parent, while dependencies are naturally
         cleared only by processing within its descendants.
         [//]
         In any event, this object catches positive dependency changes and
         accumulates them in an internal state variable that it expects
         never to be manipulated by anything other than the running processor.
         Moreover, because there is at most one running processor at any
         given time, there is no need to worry about race conditions arising
         in the manipulation of this internal state variable.  If there were,
         things would be much more treacherous and inefficient.
    */
  private: // Data
    bool is_active;
    kdu_int32 acc_new_dependencies;
    kdu_thread_job *job_to_schedule;
    kdu_byte _sep1[KDU_MAX_L2_CACHE_LINE];
    kdu_interlocked_int32 num_dependencies;
    kdu_byte _sep2[KDU_MAX_L2_CACHE_LINE];
};

/*****************************************************************************/
/*                              kdu_run_queue_job                            */
/*****************************************************************************/

class kdu_run_queue_job : public kdu_thread_job {
  /* [SYNOPSIS]
       This object is provided as a convenience to help you implement the
       potentially very efficient multi-threaded processing approach
       described in connection with `kdu_run_queue'.  Do not forget to
       invoke `init' before first entering the job function for the first
       time, and do not forget to invoke the base `kdu_thread_job::init'
       function to register your job "processor" function.  You might
       do both of these things from within a derived object's `init'
       function.
       [//]
       Typically, you would derive an application specific processing
       object from this one.  The `kdu_run_queue' object, on the other
       hand, typically gets used as-is, without derivation, as a top-level
       parent queue for data processing queues that your "processor"
       function uses to push/pull data.  You may create many such
       `kdu_run_queue' objects at once and use them with only one
       instance of your derived `kdu_run_queue_job' processing object,
       passing it to the relevant `kdu_run_queue::check_continue' function
       whenever the appearance of potential blocking conditions needs to
       be checked.
       [//]
       The present object assumes that a master thread (typically a thread
       group owner) is interested only in discovering that all processing
       has been completed, via calls to `wait_done'.  However, you could
       easily modify or extend the approach here to allow a master thread
       to wait upon the completion of some portion of the processing
       before returning from a modified version of `wait_done'.  This would
       allow the master thread to successively wait for and advance
       through a series of processing milestones, doing other work in
       between these milestones.
  */
  public: // Member functions
    kdu_run_queue_job() { cond = NULL; }
    void init(kdu_thread_entity *caller) { cond = caller->get_condition(); }
      /* [SYNOPSIS]
           Call this function before you first enter the job function.
           You should call this function from the master thread (almost
           invariably the thread group owner) that is described in the
           documentation of `kdu_run_queue'.  This function just sets up
           the initial `kdu_thread_entity_condition' pointer to reference
           the calling thread's active condition object so that it can
           be signalled from within the job function.
      */
    void signal_done(kdu_thread_entity *caller)
      { 
        assert(cond != NULL);
        caller->signal_condition(cond);
      }
      /* [SYNOPSIS]
           As described in `kdu_run_queue', this function should be called
           from within the job function, once all processing is complete.
           If you are following the suggestions given in connection with
           `kdu_run_queue', the master thread (almost invariably the thread
           group owner) will wait upon the signalling of this condition by
           invoking `wait_done' immediate after directly invoking the job
           function one time.
      */
    void wait_done(kdu_thread_entity *caller)
      { 
        assert(cond == caller->get_condition());
        caller->wait_for_condition();
      }
      /* [SYNOPSIS]
           Use this function to wait upon the condition signalled by the
           `signal_done' function.
      */
  private: // Data
    kdu_thread_entity_condition *cond;
};

/*****************************************************************************/
/*                               kd_thread_lock                              */
/*****************************************************************************/

struct kd_thread_lock {
    kdu_mutex mutex;
    kdu_thread_entity *holder; // Entity which currently holds the lock
  };

/*****************************************************************************/
/*                            kd_thread_grouperr                             */
/*****************************************************************************/

struct kd_thread_grouperr {
    bool failed;
    kdu_exception failure_code;
  };

/*****************************************************************************/
/*                            kdu_thread_context                             */
/*****************************************************************************/

class kdu_thread_context {
  /* [SYNOPSIS]
       Derived versions of this class are used to maintain state information
       for a specific processing context within a Kakadu thread group.
       The thread group itself is managed by `kdu_thread_entity' (all
       `kdu_thread_entity' objects in the group have a common internal
       reference to a single group, with one being the group owner).
       Instances of the `kdu_thread_context' class add and remove themselves
       from thread groups using the `enter_group' and `leave_group' functions.
       [//]
       One purpose of thread groups is to maintain a set of "safe" mutexes
       that can be used by members of the thread group to enter and leave
       critical sections within the relevant processing context.  These
       mutexes are "safe" because a thread which has locked any of these
       mutexes and subsequently fails inside the critical section will not
       wind up deadlocking other threads that subsequently try to lock
       the mutex.  Instead, when a thread fails, throwing an exception,
       `kdu_thread_entity::handle_exception' should be called at some point
       and this leads to any lock the thread is holding being removed.
       Moreover, any other threads which try to acquire the lock will
       themselves throw the exception.  This helps ensure that the group
       behaves as a single working entity.  In each case, exception catching
       and rethrowing is restricted to exceptions of type `kdu_exception'.
       [//]
       The most important use of this object is in forming codestream-specific
       contexts to manage critical sections within the core codestream
       management machinery associated with `kdu_codestream'.  This context
       is automatically created and added to the thread group when a
       `kdu_thread_entity' object is first passed into a `kdu_codestream'
       interface function.  That context is removed and destroyed by
       `kdu_codestream::destroy' or the destruction of the thread group,
       whichever comes first.  The codestream-specific thread context 
       provides sophisticated multi-threaded memory management features,
       a set of critical section locks, and even adds its own thread
       queues for background codestream maintainance processing.
  */
  public: // Member functions
    kdu_thread_context()
      { 
        group = NULL; grouperr = NULL;
        num_locks = 0; locks = NULL; lock_handle = NULL;
        next = prev = NULL;
      }
    virtual ~kdu_thread_context() { leave_group(); }
    KDU_EXPORT virtual void enter_group(kdu_thread_entity *caller);
      /* [SYNOPSIS]
           This function is used to attach the object to the thread
           group to which the `caller' belongs.  The function also
           creates and initializes mutual exclusion locks for the object
           (based on the value returned by `get_num_locks').
           [//]
           If you override this function to create additional functionality
           of your own, you may well be interested in knowing the number
           of threads in the thread group.  Of course, this number can
           change with calls to `kdu_thread_entity::add_thread' and
           `kdu_thread_entity::add_single_threaded_domain'.  To this end,
           the current object's `num_threads_changed' function is called
           automatically both from within the current function and any
           time the number of threads actually does change.
      */
    KDU_EXPORT virtual void leave_group(kdu_thread_entity *caller=NULL);
      /* [SYNOPSIS]
           This function is used to detach the object from any thread group
           to which it may be attached by a previous call to `enter_group'.
           The function is invoked automatically from the destructor, if
           necessary, and also from `kdu_thread_entity::destroy'.
           [//]
           In the above-mentioned contexts, and perhaps others, the
           identity of the calling thread is not directly available, so
           the `caller' argument will be NULL.  In such cases, the
           function must not be invoked from within a critical section
           in which the thread group's mutex is locked.  In particular,
           it must not be invoked from within a call to
           `kdu_thread_queue::request_termination'.
      */
    int check_group(kdu_thread_entity *caller)
      { return (caller->group != this->group)?-1:caller->thread_idx; }
      /* [SYNOPSIS]
           Returns -1 if the `caller' belongs to a different thread group
           to this thread context.  Otherwise, the function returns the
           thread index associated with the `caller', which is guaranteed
           to lie in the range 0 to `KDU_MAX_THREADS'-1, with 0 corresponding
           to the group owner.
      */
    void acquire_lock(int lock_id, kdu_thread_entity *thrd,
                      bool allow_exceptions=true)
      { 
        kd_thread_lock *lock = locks + lock_id;
        assert((lock_id >= 0) && (lock_id < num_locks) &&
               (group==thrd->group) && (lock->holder != thrd));
        if (allow_exceptions && grouperr->failed)
          {
            if (grouperr->failure_code == KDU_MEMORY_EXCEPTION)
              throw std::bad_alloc();
            else
              throw grouperr->failure_code;
          }
        lock->mutex.lock(); lock->holder = thrd;
      }
      /* [SYNOPSIS]
           Mutexes locked by this function are fast, non-recursive mutexes.
           You must not try to acquire the same lock a second time, without
           first releasing it.
           [//]
           The `thrd' object must be one of the threads in the thread group
           with which this object has been registered.
           [//]
           This function may throw an exception of type
           `kdu_exception' or `std::bad_alloc' (converted from
           `KDU_MEMORY_EXCEPTION'), if any other thread in the
           group terminates unexpectedly, invoking its `handle_exception'
           function.  To avoid this (e.g., when performing cleanup operations
           after an exception has already been thrown), set the
           `allow_exceptions' argument to false.
         [ARG: lock_id]
           Must be in the range 0 to L-1, where L is the value returned by
           `get_num_locks'.
         [ARG: allow_exceptions]
           If true, an exception will be thrown if the thread group is found
           to have failed somewhere.  This behaviour ensures that all
           participating threads in the group will eventually throw
           exceptions.
      */
    bool try_lock(int lock_id, kdu_thread_entity *thrd,
                  bool allow_exceptions=true)
      { 
        kd_thread_lock *lock = locks + lock_id;
        assert((lock_id >= 0) && (lock_id < num_locks) &&
               (group == thrd->group) && (lock->holder != thrd));
        if (allow_exceptions && grouperr->failed)
          {
            if (grouperr->failure_code == KDU_MEMORY_EXCEPTION)
              throw std::bad_alloc();
            else
              throw grouperr->failure_code;
          }
        if (!lock->mutex.try_lock()) return false;
        lock->holder = thrd;  return true;
      }
      /* [SYNOPSIS]
           Same as `acquire_lock', except that the function does not block.
           If the mutex is already locked by another thread, the function
           returns false immediately.
      */
    void release_lock(int lock_id, kdu_thread_entity *thrd)
      { 
        kd_thread_lock *lock = locks + lock_id;
        assert((lock_id >= 0) && (lock_id < num_locks) &&
               (group == thrd->group) && (lock->holder == thrd));
        lock->holder = NULL;  lock->mutex.unlock();
      }
      /* [SYNOPSIS]
           This function is used to leave a critical section entered with
           a correspond call to `acquire_lock' or a successful call to
           `try_lock'.
      */
    bool check_lock(int lock_id, kdu_thread_entity *thrd) const
      { 
        kd_thread_lock *lock = locks + lock_id;
        assert((lock_id >= 0) && (lock_id < num_locks) &&
               (group == thrd->group));
        return (lock->holder == thrd);
      }
      /* [SYNOPSIS]
           Returns true if `thrd' is currently holding the lock identified by
           `lock_id' -- you should, of course, only invoke this function from
           the actual thread that is associated with `thrd'.
      */
  protected: // Member functions to override
    virtual int get_num_locks() { return 0; }
      /* [SYNOPSIS]
           Override this function in a derived class if you want your
           thread context to provide mutual exclusion locks -- this is
           usually the case.  These "safe" locks are available via the
           `acquire_lock' and `release_lock' member functions as soon as
           the thread context has been registered with a thread group
           via `kdu_thread_entity::add_context'.  The locks are "safe"
           in the sense that a thread which blocks on one of these locks
           will throw an exception if another thread in the same group
           invokes `kdu_thread_entity::handle_exception' (generally in
           response to an exception being thrown).
      */
    virtual void num_threads_changed(int num_threads) { return; }
      /* [SYNOPSIS]
           This function is provided solely to allow extended objects to
           receive notification if the number of threads changes in a
           thread group to which they are attached.  The values supplied to
           `num_threads' in each successive call (between `enter_group' and
           `leave_group') will be non-decreasing.  The function is also
           called from within `enter_group' so an extended class can
           perform thread-count specific resource allocation/initialization
           inside this function.
           [//]
           This function is always invoked from within a critical section
           in which the thread group's mutex is locked.  As a result, you
           should generally avoid calling other functions offered by
           `kdu_thread_entity' or `kdu_thread_queue' from within this
           function (this is why the function is not supplied with any
           `kdu_thread_entity' object reference) and you should not call
           `leave_group' from inside this function.
           [//]
           When a new thread is launched, the present function will be
           called (and return) before the thread has an opportunity to
           perform any scheduled jobs, so this is a safe place in which to
           allocate resources that the thread might need to access.
      */
    virtual void handle_exception(kdu_thread_entity *caller);
      /* [SYNOPSIS]
           Called from inside `kdu_thread_entity::handle_exception', the
           base implementation of this function causes any mutual exclusion
           lock that the `caller' thread has taken out (via `acquire_lock')
           to be released; the `grouperr' member should identify that an
           exception has occurred, so that any thread unblocked by releasing
           such mutex locks will immediately throw an exception from within
           its `acquire_lock' call.
           [//]
           Each thread in the associated thread group that throws an
           exception should eventually call this function, allowing that
           thread to release resources which might be left behind after
           the exception.  Once this function has been called by any
           thread in the group, the state of the thread group is
           permanently changed; most notably, some of the API functions
           will throw exceptions immediately when called from any thread
           (this is documented with the relevant interface function
           descriptions) and previously blocked calls to
           `kdu_thread_context::acquire_lock' will throw exceptions rather
           than succeeding.
           [//]
           The thread group owner will call this function as a precaution
           from within `kdu_thread_entity::destroy', just in case it is
           holding onto a lock that was left behind by an exception caught
           within the application and the exception handler failed to call
           `kdu_thread_entity::handle_exception'.
           [//]
           You may find it useful to override this function to perform
           additional exception handling.  Note, however, that this
           function does not (and MUST NOT) itself call `leave_group'
           or otherwise cause the object to be removed from its thread
           group.
      */
  private: // Data
    friend class kdu_thread_entity;
    kd_thread_group *group;
    kd_thread_grouperr *grouperr; // Identical to `group->grouperr'
    int num_locks;
    kd_thread_lock *locks;
    kd_thread_lock *lock_handle; // Allows for placement of `locks' in L2 lines
    kdu_thread_context *next, *prev;
  };

#endif // KDU_THREADS_H
