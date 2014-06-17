/*****************************************************************************/
// File: kdu_elementary.h [scope = CORESYS/COMMON]
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
   Elementary data types and extensively used constants, plus important
elementary classes that have all-inline implementations.  It is possible that
the definitions here might need to be changed for some architectures.
   Note that from version 7.2.1, the most portable definitions have been
split off into the new "kdu_ubiquitous.h" header file, that is included
from here.  See that file's header for an explanation as to why this has
been done.
******************************************************************************/

#ifndef KDU_ELEMENTARY_H
#define KDU_ELEMENTARY_H
#include <new>
#include <limits.h>
#include <string.h>
#include "kdu_ubiquitous.h" // Look here to find some things that used to be
                            // located within "kdu_elementary.h" itself.


/* ========================================================================= */
/*                        Multi-Threading Primitives                         */
/* ========================================================================= */

/*****************************************************************************/
/*                       Threading and Timing Support                        */
/*****************************************************************************/

#include <time.h>

#ifdef KDU_WINDOWS_OS
#  ifndef KDU_NO_THREADS // Disable multi-threading by defining KDU_NO_THREADS
#    define KDU_THREADS
#    define KDU_WIN_THREADS
#    include <intrin.h>
#    ifdef _MSC_VER
#      define KDU_MSC_ATOMICS
#      pragma intrinsic (_InterlockedExchangeAdd)
#      pragma intrinsic (_InterlockedOr)
#      pragma intrinsic (_InterlockedAnd)
#      pragma intrinsic (_InterlockedExchange)
#      pragma intrinsic (_InterlockedCompareExchange)
#      ifdef KDU_POINTERS64
#        pragma intrinsic (_InterlockedExchangeAdd64)
#        pragma intrinsic (_InterlockedExchange64)
#        pragma intrinsic (_InterlockedCompareExchange64)
#        pragma intrinsic (_InterlockedExchangePointer)
#        pragma intrinsic (_InterlockedCompareExchangePointer)
#        pragma intrinsic (_mm_pause)
#      endif
#    elif (defined __GNUC__)||(defined __APPLE__)||(defined __INTEL_COMPILER)
#      define KDU_GNU_ATOMICS
#    else
#      error "Note sure how to support atomics -- define `KDU_NO_THREADS'."
#    endif // Non-MSC compilation under windows
#  endif // !KDU_NO_THREADS
#elif (defined __GNUC__) || (defined __APPLE__) || (defined __INTEL_COMPILER)
#  include <unistd.h>
#  ifndef HAVE_CLOCK_GETTIME
#    include <sys/time.h>
#  endif // HAVE_CLOCK_GETTIME
#  define KDU_TIMESPEC_EXISTS
   struct kdu_timespec : public timespec {
     bool get_time()
       { 
#      ifdef HAVE_CLOCK_GETTIME
         return (clock_gettime(this) == 0);
#      else // Assume we HAVE_GETTIMEOFDAY
         struct timeval val;
         tv_sec = 0;  tv_nsec = 0; // In case of premature return
         if (gettimeofday(&val,NULL) != 0) return false;
         tv_sec=val.tv_sec; tv_nsec=val.tv_usec*1000; return true;
#      endif // HAVE_CLOCK_GETIME
       }
     };
#  ifndef KDU_NO_THREADS // Disable multi-threading by defining KDU_NO_THREADS
#    define KDU_THREADS
#    define KDU_PTHREADS
#    define KDU_GNU_ATOMICS
#    include <pthread.h>
#    if (defined __APPLE__)
#      include <mach/mach_init.h>
#      include <mach/thread_policy.h>
#      include <mach/thread_act.h>
#      include <mach/task.h>
#      include <mach/semaphore.h>
#    else // Use Posix semaphores
#      include <semaphore.h>
#    endif // !__APPLE__
#    if ((defined sun) || (defined __sun))
#      include <sched.h>
#    endif // Solaris
#  endif // !KDU_NO_THREADS
#endif

/*****************************************************************************/
/*                               kdu_clock                                   */
/*****************************************************************************/

class kdu_clock {
  /* [SYNOPSIS]
       This class is provided primarily as a convenience for timing
       the execution of programs or parts of programs.  The intent is to
       overcome a weakness in the implementation of the ANSII C `clock'
       function, in that this function returns ellapsed CPU time on
       some operating systems and ellapsed system clock time on others.
       For a more comprehensive timing solution, you might be interested
       in the `kdcs_timer' class defined as part of Kakadu's communication
       support system.  The present object, however, is designed to be
       extremely simple and implemented entirely in-line, as with everything
       in the "kdu_elementary.h" header.
  */
  public: // Member functions
    kdu_clock()
      { 
#     ifdef KDU_TIMESPEC_EXISTS
        state.get_time();
#     else
        state = clock();
#     endif // !KDU_TIMESPEC_EXISTS
      }
    bool measures_real_time()
      { /* [SYNOPSIS]
             Returns true if the object is supposed to measure ellapsed
             system time (real time).  Every effort is made to ensure that
             this is true.  Otherwise, it is possible (but not certain)
             that the measured time will be ellapsed CPU time. */
#     ifdef KDU_TIMESPEC_EXISTS
        return true;
#     elif (defined KDU_WINDOWS_OS)
        return true;
#     else
        return false;
#     endif
      }
    void reset()
      { /* [SYNOPSIS] See `get_ellapsed_seconds'. */
#     ifdef KDU_TIMESPEC_EXISTS
        state.get_time();
#     else
        state = clock();
#     endif // !KDU_TIMESPEC_EXISTS        
      }
    double get_ellapsed_seconds()
      { /* [SYNOPSIS]
             This function returns the number of seconds that have ellapsed
             since the object was constructed, the last call to `reset', or
             the last call to this function, whichever happened most recently.
             The returned value might only have millisecond precision, or it
             might have nanosecond precision.  If an error is encountered
             for some reason, the function returns 0. */
#     ifdef KDU_TIMESPEC_EXISTS
        kdu_timespec old_state = state;
        if (!state.get_time())
          return 0.0;
        return (((double) state.tv_sec)-((double) old_state.tv_sec)) +
          0.000000001*(((double) state.tv_nsec)-((double) old_state.tv_nsec));
#     else
        clock_t old_state = state;
        state = clock();
        return ((double)(state-old_state)) * (1.0/CLOCKS_PER_SEC);
#     endif // !KDU_TIMESPEC_EXISTS
      }
  private: // Data
# ifdef KDU_TIMESPEC_EXISTS
    kdu_timespec state;
# else
    clock_t state;
# endif // !KDU_TIMESPEC_EXISTS
  };

/*****************************************************************************/
/*                             kdu_thread_object                             */
/*****************************************************************************/

class kdu_thread_object {
  /* [SYNOPSIS]
       This class provides a base from which to derive custom objects
       that you may wish to register via `kdu_thread::add_thread_object' so
       as to ensure that the objects are deleted when the thread returns
       from its entry-point function.  This feature is used, for example, in
       the implementation of Kakadu's Java language bindings, to ensure that
       the thread specific JNI environment created when a Java function is
       invoked from within a Kakadu thread will be detached immediately
       before the thread exits.
  */
  public: // Member functions
    kdu_thread_object(const char *name_string=NULL)
      { /* [SYNOPSIS]
             If `name_string' is NULL (default), the constructed
             object can be registered with `kdu_thread::add_thread_object',
             but cannot be retrieved via `kdu_thread::find_thread_object'.
             This is fine if all you want to do is ensure that the object
             gets deleted when the thread returns from its entry-point
             function.
        */
        name = NULL; next = NULL;
        if (name_string == NULL) return;
        name = new char[strlen(name_string)+1];
        strcpy(name,name_string);
      }
    virtual ~kdu_thread_object()
      { if (name != NULL) delete[] name; }
  private: // private functions
    friend class kdu_thread;
    bool check_name(const char *string)
      { 
        return ((string != NULL) && (name != NULL) && (*string == *name) &&
                (strcmp(string,name) == 0));
      }
  private: // Data
    char *name;
    kdu_thread_object *next;
  };

/*****************************************************************************/
/*                           kdu_thread_startproc                            */
/*****************************************************************************/

#ifdef KDU_WIN_THREADS
    typedef DWORD kdu_thread_startproc_result;
#   define KDU_THREAD_STARTPROC_CALL_CONVENTION WINAPI
#   define KDU_THREAD_STARTPROC_ZERO_RESULT ((DWORD) 0)
#else
    typedef void * kdu_thread_startproc_result;
#   define KDU_THREAD_STARTPROC_CALL_CONVENTION
#   define KDU_THREAD_STARTPROC_ZERO_RESULT NULL
#endif

typedef kdu_thread_startproc_result
    (KDU_THREAD_STARTPROC_CALL_CONVENTION *kdu_thread_startproc)(void *);

extern kdu_thread_startproc_result
  KDU_THREAD_STARTPROC_CALL_CONVENTION kd_thread_create_entry_point(void *);
   /* This function is used to actually launch new threads from
      `kdu_thread::create'.  We declare it extern only so that it can be
      identified as a friend of `kdu_thread', but we do not export this
      function from the core system library with KDU_EXPORT, because the
      function should never be directly invoked by an application. */

/*****************************************************************************/
/*                                kdu_thread                                 */
/*****************************************************************************/

class kdu_thread {
  /* [SYNOPSIS]
       The `kdu_thread' object provides you with a platform independent
       mechanism for creating, destroying and manipulating threads of
       execution on Windows, Unix/Linux and MAC operating systems at
       least -- basically, any operating system which supports
       the "pthreads" or Windows threads interfaces, assuming the necessary
       definitions are set up in "kdu_elementary.h".
       [//]
       The `kdu_thread' object can also be initialized with the `set_to_self'
       function to refer to the calling thread, but in this case the
       `destroy' function does nothing and the `add_thread_object' function
       will not save thread objects for deletion when the thread is
       destroyed.
       [//]
       If a new thread is created by calling `kdu_thread::create', the
       executing thread can recover a reference to its `kdu_thread' object
       by invoking the static `get_thread_ref' member function.  One reason
       you might be interested in this is that it allows custom objects
       to be registered with the thread via `kdu_thread::add_thread_object',
       ensuring that they will be deleted when the thread returns from
       its entry point function.
  */
  public: // Member functions
    kdu_thread()
      { /* [SYNOPSIS] You need to call `create' explicitly, or else the
                      `exists' function will continue to return false. */
        thread_objects = NULL;  run_start_proc=NULL;  run_start_arg=NULL;
        can_destroy = false;
#       if defined KDU_WIN_THREADS
          thread=NULL; thread_id=0;
#       elif defined KDU_PTHREADS
          thread_valid=false;
#       endif
      }
    bool exists()
      { /* [SYNOPSIS]
             Returns true if the thread has been successfully created by
             a call to `create' that has not been matched by a completed
             call to `destroy', or if a successful call to
             `set_to_self' has been processed. */
        if (can_destroy)
          return true;
#       if defined KDU_WIN_THREADS
          return (thread != NULL);
#       elif defined KDU_PTHREADS
          return thread_valid;
#       else
          return false;
#       endif
      }
    bool operator!() { return !exists(); }
      /* [SYNOPSIS] Opposite of `exists'. */
    bool equals(kdu_thread &rhs) const
      { /* [SYNOPSIS]
             You can use this function to reliably determine whether or
             not two `kdu_thread' objects refer to the same underlying
             threads.  This does not mean that the objects are identical
             in every respect, though.  The most common application for
             the function is to compare a `kdu_thread' object with another
             that has been inialized using `set_to_self' so as to determine
             whether the calling thread is identical to that referenced
             by the object. */
#       if defined KDU_WIN_THREADS
          return ((rhs.thread != NULL) && (this->thread != NULL) &&
                  (rhs.thread_id == this->thread_id));
#       elif defined KDU_PTHREADS
          return (rhs.thread_valid && this->thread_valid &&
                  (pthread_equal(rhs.thread,this->thread) != 0));
#       else
          return false;
#       endif
      }
    bool operator==(kdu_thread &rhs) { return equals(rhs); }
      /* [SYNOPSIS] Same as `equals'. */
    bool operator!=(kdu_thread &rhs) { return !equals(rhs); }
      /* [SYNOPSIS] Opposite of `equals'. */
    bool set_to_self()
      { /* [SYNOPSIS]
             You can use this function to create a reference to the
             caller's own thread.  The resulting object can be passed to
             `equals', `get_priority', `set_priority' and `set_cpu_affinity'.
             It can also be passed to `destroy', although this will not
             do anything.
             [//]
             If you want to obtain a reference to a `kdu_thread' object
             from within the thread that it was used to create, this can
             be done using the static `get_thread_ref' function.
           [RETURNS]
             False if thread functionality is not supported on the current
             platform.  This problem can usually be resolved by
             creating the appropriate definitions in "kdu_elementary.h".
             [//]
             The function also returns false if the object is the subject
             of a previous successful call to `create' and the matching
             `destroy' call has not yet been received.
        */
        if (can_destroy)
          return false;
#       if defined KDU_WIN_THREADS
          thread = GetCurrentThread();  thread_id = GetCurrentThreadId();
          return true;
#       elif defined KDU_PTHREADS
          thread = pthread_self();
#         if (defined __APPLE__)
            mach_thread_id = pthread_mach_thread_np(thread);
#         endif // __APPLE__
          return thread_valid = true;
#       else
          return false;
#       endif
      }
    bool check_self() const
      { /* [SYNOPSIS]
             Returns true if the calling thread is the one that is
             associated with the current object.
        */
        kdu_thread me; me.set_to_self();
        return this->equals(me);
      }
    KDU_EXPORT static kdu_thread *get_thread_ref();
      /* [SYNOPSIS]
           If `kdu_thread::create' is used to create a new thread of
           execution, that thread can later find a reference to its own
           `kdu_thread' object (the one that will eventually be used to
           to `destroy' the thread) by calling this static function.
      */
    KDU_EXPORT bool create(kdu_thread_startproc start_proc, void *start_arg);
      /* [SYNOPSIS]
           Creates a new thread of execution, with `start_proc' as its
           entry-point and `start_arg' as the parameter passed to
           `start_proc' on entry.
           [//]
           If this function is called with a `start_proc' NULL, then
           `start_arg' is ignored and an object is created to represent
           the calling thread.  This may or may not differ from
           `set_to_self', depending on whether or not other resources
           are associated with a created thread.  However, once
           `create' has been called, you are always allowed (if not obliged)
           to call `destroy'.
         [RETURNS]
           False if thread creation is not supported on the current platform
           or if the operating system is unwilling to allocate any more
           threads to the process.  The former problem may be resolved by
           creating the appropriate definitions in "kdu_elementary.h".
      */
    KDU_EXPORT bool destroy();
      /* [SYNOPSIS]
           Suspends the caller until the thread has terminated.  If this
           function is called from the same thread as the one that is
           being destroyed, the function does not wait for the thread
           to terminate (for obvious reasons) but it does destroy any
           other resources created by the `create' function.
           [//]
           Note that this function can be called on an object that has
           not been created using `create', but in that case it will
           return false.
         [RETURNS]
           False if `create' was never successfully called or if something
           goes wrong.
      */
    KDU_EXPORT bool add_thread_object(kdu_thread_object *obj);
      /* [SYNOPSIS]
           You can use this function to add `obj' to an internal list of
           objects that will be deleted when the thread returns from its
           entry-point function.
           [//]
           If the `check_self' function returns false, or
           `kdu_thread::create' was not used to create the thread of
           execution, this function does nothing, returning false.
           [//]
           The function returns true if `obj' was already on the list of
           if the function added it to the list; in either case,
           responsibility for deleting `obj' has been transferred to the
           `kdu_thread' object.
      */
    KDU_EXPORT kdu_thread_object *find_thread_object(const char *name);
      /* [SYNOPSIS]
           You can use this function to search for thread objects that
           have been saved by `add_thread' for deletion when
           `destroy' is called.  However, you can only search for named
           `kdu_thread_object' objects.  If a `kdu_thread_object' with
           no name (constructed with a NULL name string) was passed to
           `add_thread_object', it cannot be retrieved from the internal
           list by this function.
      */
    KDU_EXPORT bool set_cpu_affinity(kdu_long affinity_mask);
      /* [SYNOPSIS]
           The purpose of this function is to indicate preferences for
           the logical CPU's the thread should run on.  This function
           might not be implemented on all operating systems and the
           `affinity_mask' might be interpreted differently on different
           operating systems.
           [//]
           The principle interpretation of `affinity_mask' is that
           it holds a set of flag bits, with one flag for each logical
           CPU on which the thread would like to be able to run.  The
           LSB corresponds to the first logical CPU in the system.
           No more than 64 logical CPU's can be identified using
           this mechanism.  Currently, this interpretation holds under
           Windows and Linux.
           [//]
           Under OSX, the `affinity_mask' must be converted to a 32-bit
           integer that identifies groups of threads that wish to be
           scheduled on logical CPU's that have some shared physical
           resource (shared L2 cache, shared L3 cache, same die, etc.).
           To do this, the implementation currently forms this 32-bit
           identifier from the index of the least significant non-zero bit
           in the `affinity_mask' and the index of the most significant
           non-zero bit in the `affinity_mask'.  In most practical
           applications, this is likely to produce the desired outcome,
           unless you assign affinity masks to different threads whose
           set bits partially overlap with one another.
           [//]
           Although this function should be executable from any thread
           in the system, experience shows that some platforms do not behave
           correctly unless the function is invoked from within the thread
           whose processor affinity is being modified.
         [RETURNS]
           False if the thread has not been successfully created, or the
           `affinity_mask' is invalid, or the calling thread does not
           have permission to set the CPU affinity of another thread, or
           operating support has not yet been extended to offer this
           specific feature.
      */
    KDU_EXPORT int get_priority(int &min_priority, int &max_priority);
      /* [SYNOPSIS]
           Returns the current priority of the thread and sets `min_priority'
           and `max_priority' to the minimum and maximum priorities that
           should be used in successful calls to `set_priority'.
         [RETURNS]
           False if the thread has not been successfully created.
      */
    KDU_EXPORT bool set_priority(int priority);
      /* [SYNOPSIS]
           Returns true if the threads priority was successfully changed.
           Use `get_priority' to find the current priority and a reasonable
           range of priority values to use in calls to this function.
         [RETURNS]
           False if the thread has not been successfully created, or the
           `priority' value is invalid, or the calling thread does not
           have permission to set the priority of another thread.
      */
    static bool micro_pause()
      { /* [SYNOPSIS]
             This function is intended for use within spin-locks and related
             constructs built from atomic interlocked operations.  Between
             each iteration of a polling spin-lock that is testing for a
             condition believed to be imminent, the CPU should be paused
             for long enough to allow it to catch up with changes in the
             external memory state.  On some processors, a special
             instruction is provided for this purpose.  On some platforms
             this function might not do anything at all, in which case it
             returns false, leaving the loop to spin at maximum speed.
        */
#       if (defined KDU_WIN_THREADS)
          _mm_pause();
          return true;
#       elif (defined KDU_PTHREADS)
#         if ((defined __i386__) || (defined __x86_64__))
              __asm__ __volatile__(" rep; nop\n");
#         else
              __sync_synchronize(); // Actually just a full memory barrier,
                 // which might not be quite the same as the pause, but
                 // should implement some memory-system-dependent delay on
                 // any multi-processor system.
#         endif
#       endif
        return false;
      }
    static bool yield()
      { /* [SYNOPSIS]
             Yields the thread so that the scheduler can run other threads.
             This function is intended for use within spin-locks and related
             constructs built from atomic interlocked operations, in which
             a short wait turns out to be unexpectedly long -- probably
             because the operation that another thread is expected to
             perform within a few clock cycles is interrupted by that
             thread being rescheduled.  Yielding our own thread's execution
             slice increases the likelihood that the other thread will be
             brought back to life, especially important if there is only
             one physical processor.
             [//]
             Depending on the platform, the yield operation may be limited
             to threads of the same or higher priority only.  Returns true
             if the feature is implemented; else returns false.  You should
             not use this function except within a "spin-loop" which
             expects the condition it is waiting for to become available
             imminently, but finds that it has taken longer than expected
             (typically based on a spin count), which may mean that the
             CPU needs to be yielded back to the scheduler so that it
             can satisfy the condition.
        */
#       if (defined KDU_WIN_THREADS)
          SwitchToThread();
          return true;
#       elif (!defined KDU_PTHREADS)
          return false;
#       elif (defined __APPLE__)
          pthread_yield_np();
          return true;
#       elif ((defined sun) || (defined __sun))
          sched_yield();
          return true;
#       else
          pthread_yield();
          return true;
#       endif
      }
  private: // Internal functions
    friend kdu_thread_startproc_result KDU_THREAD_STARTPROC_CALL_CONVENTION
      kd_thread_create_entry_point(void *);
    void cleanup_thread_objects();
  private: // Data
    bool can_destroy;
    kdu_thread_startproc run_start_proc; // These members keep track of start
    void *run_start_arg; // parameters between calls to `start'and `destroy'.
    kdu_thread_object *thread_objects; // Must be NULL unless `can_destroy'
#   if defined KDU_WIN_THREADS
      HANDLE thread; // NULL if empty
      DWORD thread_id;
#   elif defined KDU_PTHREADS
      pthread_t thread;
      bool thread_valid; // False if empty
#     if (defined __APPLE__)
        thread_act_t mach_thread_id;
#     endif // __APPLE__
#   endif
  };

/*****************************************************************************/
/*                              kdu_semaphore                                */
/*****************************************************************************/

class kdu_semaphore {
  /* [SYNOPSIS]
       The `kdu_semaphore' object implements a conventional semaphore on
       Windows, Unix/Linux and MAC operating systems at least, assuming
       the necessary definitions are set up in "kdu_elementary.h".
       [//]
       In many cases, `kdu_semaphore' and `kdu_event' may be used to
       accomplish similar ends; similarly, in many cases `kdu_semaphore'
       and `kdu_mutex' may be used to accomplish similar synchronization
       objectives.  However, each of these three synchronization primitives
       has their advantages and disadvantages.  If you need to implement
       a simple critical section, use `kdu_mutex'.  If you need to wait on
       a condition but do not expect to be doing this from within a critical
       section, a `kdu_semaphore' is likely to be most efficient.  Otherwise,
       use `kdu_event', which requires the locking and unlocking of a mutex
       around the point where a condition is waited upon.
  */
  public: // Member functions
    kdu_semaphore()
      { /* [SYNOPSIS] You need to call `create' explicitly, or else the
                      `exists' function will continue to return false. */
#       if defined KDU_WIN_THREADS
          semaphore=NULL;
#       elif defined KDU_PTHREADS
          semaphore_valid=false;
#       endif
      }
    bool exists()
      { /* [SYNOPSIS]
             Returns true if the object has been successfully created in
             a call to `create' that has not been matched by a call to
             `destroy'. */
#       if defined KDU_WIN_THREADS
          return (semaphore != NULL);
#       elif defined KDU_PTHREADS
          return semaphore_valid;
#       else
          return false;
#       endif
      }
    bool operator!() { return !exists(); }
      /* [SYNOPSIS] Opposite of `exists'. */
    bool create(int initial_value)
      { /* [SYNOPSIS]
             You must explicitly call `create' and `destroy'.
             The constructor and destructor for the `kdu_semaphore' object
             do not create or destroy the underlying synchronization
             primitive itself.
           [RETURNS]
             False if semaphore creation is not supported on the current
             platform or if the operating system is unwilling to allocate
             any more synchronization primitives to the process.  The former
             problem may be resolved by creating the appropriate definitions
             in "kdu_elementary.h".
           [ARG: initial_value]
             Initial value for the semaphore.  Calls to `wait' attempt to
             decrement the count, returning only when the result of such
             decrementing would be non-negative.
        */
#       if defined KDU_WIN_THREADS
          semaphore = CreateSemaphore(NULL,initial_value,INT_MAX,NULL);
#       elif defined KDU_PTHREADS
#         if (defined __APPLE__)
            semaphore_valid =
              (semaphore_create(mach_task_self(),&semaphore,SYNC_POLICY_FIFO,
                                initial_value) == KERN_SUCCESS);
#         else // Use POSIX semaphores
            semaphore_valid =
              (sem_init(&semaphore,0,(unsigned int)initial_value) == 0);
#         endif // !__APPLE__
#       endif
        return exists();
      }
    bool destroy()
      { /* [SYNOPSIS]
             You must explicitly call `create' and `destroy'.
             The constructor and and destructor for the `kdu_semaphore'
             object do not create or destroy the underlying synchronization
             primitive itself.
           [RETURNS]
             False if the semaphore has not been successfully created. */
        bool result = false;
#       if defined KDU_WIN_THREADS
          result = ((semaphore != NULL) && (CloseHandle(semaphore)==TRUE));
          semaphore = NULL;
#       elif defined KDU_PTHREADS
#         if (defined __APPLE__)
            if (semaphore_valid &&
                (semaphore_destroy(mach_task_self(),
                                   semaphore) != KERN_SUCCESS))
              result = false;
#         else // Using POSIX semaphores
            if (semaphore_valid && (sem_destroy(&semaphore) != 0))
              result = false;
#         endif // !__APPLE__
          semaphore_valid = false;
#       endif
        return result;
      }
    bool wait()
      { /* [SYNOPSIS]
             This function is used to wait for the semaphore to become
             positive, whereupon it is atomically decremented by 1 and the
             function returns.
           [RETURNS]
             False if the semaphore has not been successfully created
        */
#       if (defined KDU_WIN_THREADS)
          return ((semaphore != NULL) &&
                  (WaitForSingleObject(semaphore,INFINITE) == WAIT_OBJECT_0));
#       elif (defined KDU_PTHREADS)
#         if (defined __APPLE__)
            return (semaphore_valid &&
                    (semaphore_wait(semaphore) == KERN_SUCCESS));
#         else // Using POSIX semaphores
            return (semaphore_valid && (sem_wait(&semaphore) == 0));
#         endif // !__APPLE__
#       else
          return false;
#       endif
      }
    bool signal()
      { /* [SYNOPSIS]
             Use this function to increment the semaphore by 1, waking up
             any thread that may have yielded execution in a call to
             `wait' unless this would reduce the semaphore's count below 0.
           [RETURNS]
             False if the semaphore has not been successfully created.
        */
#       if (defined KDU_WIN_THREADS)
          return ((semaphore != NULL) && ReleaseSemaphore(semaphore,1,NULL));
#       elif (defined KDU_PTHREADS)
#         if (defined __APPLE__)
            return (semaphore_valid &&
                    (semaphore_signal(semaphore) == KERN_SUCCESS));
#         else // Using POSIX semaphores
            return (semaphore_valid && (sem_post(&semaphore) == 0));
#         endif // !__APPLE__
#       else
          return false;
#       endif
      }
  private: // Data
#   if defined KDU_WIN_THREADS
      HANDLE semaphore; // NULL if none
#   elif defined KDU_PTHREADS
      bool semaphore_valid;
#     if (defined __APPLE__)
        semaphore_t semaphore;
#     else // Use POSIX semaphores
        sem_t semaphore;
#     endif // !__APPLE__
#   endif
  };

/*****************************************************************************/
/*                                 kdu_mutex                                 */
/*****************************************************************************/

class kdu_mutex {
  /* [SYNOPSIS]
       The `kdu_mutex' object implements a conventional mutex (i.e., a mutual
       exclusion synchronization object), on Windows, Unix/Linux and MAC
       operating systems at least -- basically, any operating system which
       supports the "pthreads" or Windows threads interfaces, assuming the
       necessary definitions are set up in "kdu_elementary.h".
       [//]
       From Kakadu version 7.0, the `create' function offers some optional
       customization features which may result in the instantiation of
       different types of locking primitives, depending on the platform.
  */
  public: // Member functions
    kdu_mutex()
      { /* [SYNOPSIS] You need to call `create' explicitly, or else the
                      `exists' function will continue to return false. */
#       if defined KDU_WIN_THREADS
          mutex=NULL; critical_section_valid=false;
#       elif defined KDU_PTHREADS
          mutex_valid=false;
#       endif
      }
    bool exists()
      { /* [SYNOPSIS]
             Returns true if the object has been successfully created in
             a call to `create' that has not been matched by a call to
             `destroy'. */
#       if defined KDU_WIN_THREADS
          return (critical_section_valid || (mutex != NULL));
#       elif defined KDU_PTHREADS
          return mutex_valid;
#       else
          return false;
#       endif
      }
    bool operator!() { return !exists(); }
      /* [SYNOPSIS] Opposite of `exists'. */
    bool create(int max_pre_spin=1)
      { /* [SYNOPSIS]
             You must explicitly call `create' and `destroy'.
             The constructor and destructor for the `kdu_mutex' object
             do not create or destroy the underlying synchronization
             primitive itself.
           [RETURNS]
             False if creation of the relevant synchronization object is
             not supported on the current platform or if the operating system
             is unwilling to allocate any more synchronization primitives to
             the process.  The former problem may be resolved by creating the
             appropriate definitions in "kdu_elementary.h".
           [ARG: max_pre_spin]
             This argument currently only has an effect under Windows, where
             a "mutex" is used if `max_pre_spin' <= 0, while a "critical
             section" object is used otherwise.  In the latter case, the
             `max_pre_spin' argument is used to set the number of times a
             spin-lock is tried prior to entering the kernel to block.
        */
#       if defined KDU_WIN_THREADS
          if (max_pre_spin <= 0)
            mutex = CreateMutex(NULL,FALSE,NULL);
          else if (InitializeCriticalSectionAndSpinCount(&critical_section,
                                                         (DWORD)max_pre_spin))
            critical_section_valid = true;
#       elif defined KDU_PTHREADS
          mutex_valid = (pthread_mutex_init(&mutex,NULL) == 0);
#       endif
        return exists();
      }
    bool destroy()
      { /* [SYNOPSIS]
             You must explicitly call `create' and `destroy'.
             The constructor and and destructor for the `kdu_mutex' object
             do not create or destroy the underlying synchronization
             primitive itself.
           [RETURNS]
             False if the mutex has not been successfully created. */
        bool result = false;
#       if defined KDU_WIN_THREADS
          if (critical_section_valid)
            { DeleteCriticalSection(&critical_section); result = true; }
          else
            result = ((mutex != NULL) && (CloseHandle(mutex)==TRUE));
          mutex = NULL; critical_section_valid = false;
#       elif defined KDU_PTHREADS
          result = (mutex_valid && (pthread_mutex_destroy(&mutex)==0));
          mutex_valid = false;
#       endif
        return result;
      }
    bool lock()
      { /* [SYNOPSIS]
             Blocks the caller until the mutex is available.  You should
             take steps to avoid blocking on a mutex which you have already
             locked within the same thread of execution.
           [RETURNS]
             False if the mutex has not been successfully created. */
#       if defined KDU_WIN_THREADS
          if (critical_section_valid)
            { EnterCriticalSection(&critical_section); return true; }
          return ((mutex != NULL) &&
                  (WaitForSingleObject(mutex,INFINITE) == WAIT_OBJECT_0));
#       elif defined KDU_PTHREADS
          return (mutex_valid && (pthread_mutex_lock(&mutex)==0));
#       else
          return false;
#       endif
      }
    bool try_lock()
      { /* [SYNOPSIS]
             Same as `lock', except that the call is non-blocking.  If the
             mutex is already locked by another thread, the function returns
             false immediately.
           [RETURNS]
             False if the mutex is currently locked by another thread, or
             has not been successfully created. */
#       if defined KDU_WIN_THREADS
          if (critical_section_valid)
            return (TryEnterCriticalSection(&critical_section) != 0);
          return ((mutex != NULL) &&
                  (WaitForSingleObject(mutex,0) == WAIT_OBJECT_0));
#       elif defined KDU_PTHREADS
          return (mutex_valid && (pthread_mutex_trylock(&mutex)==0));
#       else
          return false;
#       endif
      }
    bool unlock()
      { /* [SYNOPSIS]
             Releases a previously locked mutex and unblocks any other
             thread which might be waiting on the mutex.
           [RETURNS]
             False if the mutex has not been successfully created. */
#       if defined KDU_WIN_THREADS
          if (critical_section_valid)
            { LeaveCriticalSection(&critical_section); return true; }
          return ((mutex != NULL) && (ReleaseMutex(mutex)==TRUE));
#       elif defined KDU_PTHREADS
          return (mutex_valid && (pthread_mutex_unlock(&mutex)==0));
#       else
          return false;
#       endif
      }
  private: // Data
#   if defined KDU_WIN_THREADS
      HANDLE mutex; // NULL if empty
      CRITICAL_SECTION critical_section;
      bool critical_section_valid; // If initialized
#   elif defined KDU_PTHREADS
      friend class kdu_event;
      pthread_mutex_t mutex;
      bool mutex_valid; // False if empty
#   endif
  };

/*****************************************************************************/
/*                                 kdu_event                                 */
/*****************************************************************************/

class kdu_event {
  /* [SYNOPSIS]
       The `kdu_event' object provides similar functionality to the Windows
       Event object, except that you must supply a locked mutex to the
       `wait' function.  In most cases, this actually simplifies
       synchronization with Windows Event objects.  Perhaps more importantly,
       though, it enables the behaviour to be correctly implemented also
       with pthreads condition objects in Unix.
  */
  public: // Member functions
    kdu_event()
      { /* [SYNOPSIS] You need to call `create' explicitly, or else the
                      `exists' function will continue to return false. */
#       if defined KDU_WIN_THREADS
          event = NULL;
#       elif defined KDU_PTHREADS
          cond_exists = manual_reset = event_is_set = false;
#       endif
      }
    bool exists()
      { /* [SYNOPSIS]
             Returns true if the object has been successfully created in
             a call to `create' that has not been matched by a call to
             `destroy'. */
#       if defined KDU_WIN_THREADS
          return (event != NULL);
#       elif defined KDU_PTHREADS
          return cond_exists;
#       else
          return false;
#       endif
      }
    bool operator!() { return !exists(); }
      /* [SYNOPSIS] Opposite of `exists'. */
    bool create(bool manual_reset)
      /* [SYNOPSIS]
             You must explicitly call `create' and `destroy'.
             The constructor and and destructor for the `kdu_event' object
             do not create or destroy the underlying synchronization
             primitive itself.
             [//]
             If `manual_reset' is true, the event object remains set from the
             point at which `set' is called, until `reset' is called.  During
             this interval, multiple waiting threads may be woken up --
             depending on whether a waiting thread invokes `reset' before
             another waiting thread is woken.
             [//]
             If `manual_reset' is false, the event is automatically reset once
             any thread's call to `wait' succeeds.  If there are other
             threads which are also waiting for the event, they will not
             be woken up until the `set' function is called again.
             [//]
             As a general rule, the automatic reset approach should be
             preferred over manual reset for efficiency reasons and it is
             best to minimize the number of threads which might wait on the
             event.
           [RETURNS]
             False if event creation is not supported on the current platform
             or if the operating system is unwilling to allocate any more
             synchronization primitives to the process.  The former problem
             may be resolved by creating the appropriate definitions in
             "kdu_elementary.h".
      */
      {
#       if defined KDU_WIN_THREADS
          event = CreateEvent(NULL,(manual_reset?TRUE:FALSE),FALSE,NULL);
#       elif defined KDU_PTHREADS
          cond_exists = (pthread_cond_init(&cond,NULL) == 0);
          this->manual_reset = manual_reset;
          this->event_is_set = false;
#       endif
        return exists();
      }
    bool destroy()
      { /* [SYNOPSIS]
             You must explicitly call `create' and `destroy'.
             The constructor and and destructor for the `kdu_mutex' object
             do not create or destroy the underlying synchronization
             primitive itself.
           [RETURNS]
             False if the event has not been successfully created. */
        bool result = false;
#       if defined KDU_WIN_THREADS
          result = (event != NULL) && (CloseHandle(event) == TRUE);
          event = NULL;
#       elif defined KDU_PTHREADS
          result = (cond_exists && (pthread_cond_destroy(&cond)==0));
          cond_exists = event_is_set = manual_reset = false;
#       endif
        return result;
      }
    bool set()
      { /* [SYNOPSIS]
             Set the synchronization object to the signalled state.  This
             causes any waiting thread to wake up and return from its call
             to `wait'.  Any future call to `wait' by this or any other
             thread will also return immediately, until such point as
             `reset' is called.  If the `create' function was supplied with
             `manual_reset' = false, the condition created by this present
             function is reset as soon as any single thread returns
             from a current or future `wait' call.  See `create' for more
             on this.
           [RETURNS]
             False if the event has not been successfully created. */
#       if defined KDU_WIN_THREADS
          return ((event != NULL) && (SetEvent(event) == TRUE));
#       elif defined KDU_PTHREADS
          if (event_is_set) return true;
          event_is_set = true;
          if (manual_reset)
            return (pthread_cond_broadcast(&cond) == 0);
          else
            return (pthread_cond_signal(&cond) == 0);
#       else
          return false;
#       endif
      }
    bool reset()
      { /* [SYNOPSIS]
             See `set' and `create' for an explanation.
           [RETURNS]
             False if the event has not been successfully created. */
#       if defined KDU_WIN_THREADS
          return ((event != NULL) && (ResetEvent(event) == TRUE));
#       elif defined KDU_PTHREADS
          event_is_set = false;  return true;
#       else
          return false;
#       endif
      }
    bool wait(kdu_mutex &mutex)
      { /* [SYNOPSIS]
             Blocks the caller until the synchronization object becomes
             signalled by a prior or future call to `set'.  If the object
             is already signalled, the function returns immediately.  In
             the case of an auto-reset object (`create'd with `manual_reset'
             = false), the function returns the object to the non-singalled
             state after a successful return.
             [//]
             The supplied `mutex' must have been locked by the caller;
             moreover all threads which call this event's `wait' function
             must lock the same mutex.  Upon return the `mutex' will again
             be locked.  If a blocking wait is required, the mutex will
             be unlocked during the wait.
           [RETURNS]
             False if the event has not been successfully created, or if
             an error (e.g., deadlock) is detected by the kernel. */
#       if defined KDU_WIN_THREADS
          mutex.unlock();
          bool result = (event != NULL) &&
            (WaitForSingleObject(event,INFINITE) == WAIT_OBJECT_0);
          mutex.lock();
          return result;
#       elif defined KDU_PTHREADS
          bool result = cond_exists;
          while (result && !event_is_set)
            result = (pthread_cond_wait(&cond,&(mutex.mutex)) == 0);
          if (!manual_reset) event_is_set = false;
          return result;
#       else
          return false;
#       endif
      }
    bool timed_wait(kdu_mutex &mutex, int microseconds)
      { /* [SYNOPSIS]
             Blocks the caller until the synchronization object becomes
             signalled by a prior or future call to `set', or the indicated
             number of microseconds elapse.  Apart from the time limit, this
             function is identical to `wait'.  If the available synchronization
             tools are unable to time their wait with microsecond precision,
             the function may wait longer than the specified number of
             microseconds, but it will not perform a poll, unless
             `microseconds' is equal to 0.
           [RETURNS]
             False if `wait' would return false or the time limit expires. */
#       if defined KDU_WIN_THREADS
          mutex.unlock();
          bool result = (event != NULL) &&
            (WaitForSingleObject(event,(microseconds+999)/1000) ==
             WAIT_OBJECT_0);
          mutex.lock();
          return result;
#       elif defined KDU_PTHREADS
          bool result = event_is_set;
          if (!result)
            {
              kdu_timespec deadline;
              if ((!cond_exists) || !deadline.get_time()) return false;
              deadline.tv_sec += microseconds / 1000000;
              if ((deadline.tv_nsec+=(microseconds%1000000)*1000)>=1000000000)
                { deadline.tv_sec++; deadline.tv_nsec -= 1000000000; }
              pthread_cond_timedwait(&cond,&(mutex.mutex),&deadline);
              result = event_is_set;
            }
          if (!manual_reset)
            event_is_set = false;
          return result;
#       else
          return false;
#       endif
      }
  private: // Data
#   if defined KDU_WIN_THREADS
      HANDLE event; // NULL if empty
#   elif defined KDU_PTHREADS
      pthread_cond_t cond;
      bool event_is_set; // The state of the event variable
      bool manual_reset;
      bool cond_exists; // False if `cond' is meaningless
#   endif
  };

/* ========================================================================= */
/*                        Atomic Operation Primitives                        */
/* ========================================================================= */

// NB: Whereas the multi-threading synchronization machinery above is
// dependent upon compilation directives `KDU_WIN_THREADS' and `KDU_PTHREADS',
// the atomc data types and operations below are dependent upon a
// separate set of compilation directives `KDU_MSC_ATOMICS' and
// `KDU_GNU_ATOMICS'.  These are all defined up above.  The reason for
// treating atomics differently from standard multi-threading primitives
// is to support correct builds for Windows platforms under GCC (as used by
// the MINGW64 makefiles). At the time of this writing, MINGW64 does not
// seem to correctly implement the full set of Windows-native atomic
// intrinsics, but it does correctly implement the native GCC atomic
// built-ins.

/*****************************************************************************/
/*                           kdu_interlocked_int32                           */
/*****************************************************************************/

struct kdu_interlocked_int32 {
  /* [SYNOPSIS]
       This object has the size of a single 32-bit integer, but offers
       in-line functions to support atomic read-modify-write style
       operations for efficient inter-thread communication.  All operations
       are accompanied by appropriate memory barriers, unless otherwise
       noted.
  */
  public: // Member functions
    void set(kdu_int32 val)
      { /* [SYNOPSIS] This function has no memory barrier. */
        state = (long) val;
      }
    kdu_int32 get() const
      { /* [SYNOPSIS] Retrieve the value without any memory barriers. */
        return (kdu_int32) state;
      }
    kdu_int32 get_add(kdu_int32 val)
      { /* [SYNOPSIS] Add `val' to the variable and return the original
           value of the variable (prior to addition), without any memory
           barriers or any guarantee of atomicity in the addition.
           This function is provided ONLY FOR SAFE CONTEXTS in
           which only one thread can possibly be interacting with the
           variable.  Otherwise, use `exchange_add', which offers the same
           functionality but is performed atomically with appropriate
           memory barriers. */
        kdu_int32 old_val = (kdu_int32) state;
        state += (long) val;
        return old_val;
      }
    kdu_int32 add_get(kdu_int32 val)
      { /* [SYNOPSIS] Same as `get_add', except that it returns the new
           value of the variable, after adding `get'.  Again, there are
           no memory barriers and there is no guarantee of atomicity in
           the addition. */
        state += (long) val;
        return (kdu_int32) state;
      }
    kdu_int32 exchange(kdu_int32 new_val)
      { /* [SYNOPSIS] Atomically writes `new_val' to the variable and returns
           the previous value of the variable.  The operation can be considered
           to offer a full memory barrier. */
        long old_val;
#       if (defined KDU_MSC_ATOMICS)
          old_val=_InterlockedExchange((long volatile *)&state,(long)new_val);
#       elif (defined KDU_GNU_ATOMICS)
          old_val = state;
          while (!__sync_bool_compare_and_swap(&state,old_val,(long)new_val))
            old_val = state;
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_val = state;  state = (long) new_val;
#       endif
        return (kdu_int32) old_val;
      }
    kdu_int32 exchange_add(kdu_int32 val)
      { /* [SYNOPSIS] Atomically adds `val' to the variable, returning
           the original value (prior to the addition).  The operation can
           be considered to offer a full memory barrier. */
        long old_val;
#       if (defined KDU_MSC_ATOMICS)
#         ifndef KDU_POINTERS64
            old_val=_InterlockedExchangeAdd((long volatile *)&state,(long)val);
#         else // KDU_PONTERS64
            old_val=(true)?
              _InterlockedExchangeAdd((long volatile *)&state,(long)val):
              (long)_InterlockedExchangeAdd64((kdu_int64 volatile *)NULL,0);
              /* Note: the above conditional assignment has no real effect;
                 the second branch is never taken, but this is the workaround
                 required to avoid compiler failure in VC2010 and VC2008 when
                 building managed code that imports this header file. */
#         endif // KDU_POINTERS64
#       elif (defined KDU_GNU_ATOMICS)
          old_val = __sync_fetch_and_add(&state,(long)val);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_val = state;  state += (long) val;
#       endif
        return (kdu_int32) old_val;
      }
    kdu_int32 exchange_or(kdu_int32 val)
      { /* [SYNOPSIS] Atomically OR's `val' with the variable, returning
           the original value (prior to the logical OR).  The operation can
           be considered to offer a full memory barrier. */
        long old_val;
#       if (defined KDU_MSC_ATOMICS)
          old_val=_InterlockedOr((long volatile *)&state,(long)val);
#       elif (defined KDU_GNU_ATOMICS)
          old_val = __sync_fetch_and_or(&state,(long)val);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_val = state;  state |= (long) val;
#       endif
        return (kdu_int32) old_val;
      }
    kdu_int32 exchange_and(kdu_int32 val)
      { /* [SYNOPSIS] Atomically AND's `val' with the variable, returning
           the original value (prior to the logical AND).  The operation can
           be considered to offer a full memory barrier. */
        long old_val;
#       if (defined KDU_MSC_ATOMICS)
          old_val=_InterlockedAnd((long volatile *)&state,(long)val);
#       elif (defined KDU_GNU_ATOMICS)
          old_val = __sync_fetch_and_and(&state,(long)val);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_val = state;  state &= (long) val;
#       endif
        return (kdu_int32) old_val;
      }
    bool compare_and_set(kdu_int32 ref_val, kdu_int32 set_val)
      { /* [SYNOPSIS] Atomically compares the variable with the supplied
           `ref_val', returning false if the comparison fails (not equal),
           but returning true and setting the variable to `set_val' if the
           comparison succeeds (equal). */
#       if (defined KDU_MSC_ATOMICS)
          long old_val =
            _InterlockedCompareExchange((long volatile *)&state,
                                        (long)set_val,(long)ref_val);
          return (old_val == (long) ref_val);
#       elif (defined KDU_GNU_ATOMICS)
          return __sync_bool_compare_and_swap(&state,ref_val,set_val);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          if (state == (long) ref_val)
            { state = (long) set_val; return true; }
          return false;
#       endif
      }
  private: // Data
    long state;
  };

/*****************************************************************************/
/*                           kdu_interlocked_int64                           */
/*****************************************************************************/

#ifdef KDU_POINTERS64
#  define KDU_HAVE_INTERLOCKED_INT64
struct kdu_interlocked_int64 {
  /* [SYNOPSIS]
       This object has the size of a single 64-bit integer, but offers
       in-line functions to support atomic read-modify-write style
       operations for efficient inter-thread communication.  All operations
       are accompanied by appropriate memory barriers, unless otherwise
       noted.
       [//]
       Note that this object might only exist on 64-bit platforms.  Code
       that uses this object should be surrounded by conditional compilation
       statements that depend on the macro `KDU_HAVE_INTERLOCKED_INT64'
       being defined.
  */
  public: // Member functions
    void set(kdu_int64 val)
      { /* [SYNOPSIS] This function has no memory barrier. */
        state = val;
      }
    kdu_int64 get() const
      { /* [SYNOPSIS] Retrieve the value without any memory barriers. */
        return state;
      }
    kdu_int64 get_add(kdu_int64 val)
      { /* [SYNOPSIS] Add `val' to the variable and return the original
           value of the variable (prior to addition), without any memory
           barriers or any guarantee of atomicity in the addition.
           This function is provided ONLY FOR SAFE CONTEXTS in
           which only one thread can possibly be interacting with the
           variable.  Otherwise, use `exchange_add', which offers the same
           functionality but is performed atomically with appropriate
           memory barriers. */
        kdu_int64 old_val = state;
        state += val;
        return old_val;
      }
    kdu_int64 add_get(kdu_int64 val)
      { /* [SYNOPSIS] Same as `get_add', except that it returns the new
           value of the variable, after adding `get'.  Again, there are
           no memory barriers and there is no guarantee of atomicity in
           the addition. */
        state += val;
        return state;
      }
    kdu_int64 exchange(kdu_int64 new_val)
      { /* [SYNOPSIS] Atomically writes `new_val' to the variable and returns
           the previous value of the variable.  The operation can be considered
           to offer a full memory barrier. */
        kdu_int64 old_val;
#       if (defined KDU_MSC_ATOMICS)
          old_val=_InterlockedExchange64((kdu_int64 volatile *)&state,new_val);
#       elif (defined KDU_GNU_ATOMICS)
          old_val = state;
          while (!__sync_bool_compare_and_swap(&state,old_val,new_val))
            old_val = state;
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_val = state;  state = new_val;
#       endif
        return old_val;
      }
    kdu_int64 exchange_add(kdu_int64 val)
      { /* [SYNOPSIS] Atomically adds `val' to the variable, returning
           the original value (prior to the addition).  The operation can
           be considered to offer a full memory barrier. */
        kdu_int64 old_val;
#       if (defined KDU_MSC_ATOMICS)
          old_val=_InterlockedExchangeAdd64((kdu_int64 volatile *)&state,val);
#       elif (defined KDU_GNU_ATOMICS)
          old_val = __sync_fetch_and_add(&state,val);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_val = state;  state += val;
#       endif
        return old_val;
      }
    kdu_int64 exchange_or(kdu_int64 val)
      { /* [SYNOPSIS] Performs an atomic logical OR of the variable with
           the supplied value, returning the original value (prior to
           the logical OR).  The operation can be considered a full memory
           barrier. */
        kdu_int64 old_val;
#       if (defined KDU_MSC_ATOMICS)
          old_val = _InterlockedOr64((kdu_int64 volatile *)&state,val);
#       elif (defined KDU_GNU_ATOMICS)
          old_val = __sync_fetch_and_or(&state,val);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_val = state; state |= val;
#       endif
        return old_val;
      }
    kdu_int64 exchange_and(kdu_int64 val)
      { /* [SYNOPSIS] Performs an atomic logical AND of the variable with
           the supplied value, returning the original value (prior to
           the logical AND).  The operation can be considered a full memory
           barrier. */
        kdu_int64 old_val;
#       if (defined KDU_MSC_ATOMICS)
          old_val = _InterlockedAnd64((kdu_int64 volatile *)&state,val);
#       elif (defined KDU_GNU_ATOMICS)
          old_val = __sync_fetch_and_and(&state,val);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_val = state; state &= val;
#       endif
        return old_val;
      }
    bool compare_and_set(kdu_int64 ref_val, kdu_int64 set_val)
      { /* [SYNOPSIS] Atomically compares the variable with the supplied
           `ref_val', returning false if the comparison fails (not equal),
           but returning true and setting the variable to `set_val' if the
           comparison succeeds (equal). */
#       if (defined KDU_MSC_ATOMICS)
          kdu_int64 old_val =
            _InterlockedCompareExchange64((kdu_int64 volatile *)&state,
                                          set_val,ref_val);
          return (old_val == ref_val);
#       elif (defined KDU_GNU_ATOMICS)
          return __sync_bool_compare_and_swap(&state,ref_val,set_val);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          if (state == ref_val)
            { state = set_val; return true; }
          return false;
#       endif
      }
  private: // Data
    kdu_int64 state;
  };
#endif // KDU_HAVE_INTERLOCKED_INT64

/*****************************************************************************/
/*                            kdu_interlocked_ptr                            */
/*****************************************************************************/

struct kdu_interlocked_ptr {
  /* [SYNOPSIS]
       This object has the size of a single address (or pointer), but offers
       in-line functions to support atomic read-modify-write style
       operations for efficient inter-thread communication.  All operations
       are accompanied by appropriate memory barriers, unless otherwise
       indicated.
  */
  public: // Member functions
    void set(void *ptr)
      { /* [SYNOPSIS] This function has no memory barrier. */
        state = ptr;
      }
    void *get() const
      { /* [SYNOPSIS] Retrieve the value without any memory barriers. */
        return state;
      }
    void *exchange(void *new_ptr)
      { /* [SYNOPSIS] Atomically writes `new_ptr' to the address and returns
           the previously held address.  The operation can be considered
           to offer a full memory barrier. */
#       if (defined KDU_MSC_ATOMICS)
#         ifdef KDU_POINTERS64
            return _InterlockedExchangePointer((void * volatile *)&state,
                                               new_ptr);
#         else // 32-bit pointers
            return (void *) _InterlockedExchange((long volatile *)&state,
                                                 (long) new_ptr);
#         endif // 32-bit pointers
#       elif (defined KDU_GNU_ATOMICS)
          void *old_ptr = state;
          while (!__sync_bool_compare_and_swap(&state,old_ptr,new_ptr))
            old_ptr = state;
          return old_ptr;
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          void *old_ptr = state;  state = new_ptr;
          return old_ptr;
#       endif
      }
    void *exchange_add(int offset)
      { /* [SYNOPSIS] Atomically adds `offset' to the address, returning
           the original address (prior to the addition). The operation can
           be considered to offer a full memory barrier. */
        void *old_ptr;
#       if (defined KDU_MSC_ATOMICS)
#         ifdef KDU_POINTERS64
            old_ptr = (void *)
              _InterlockedExchangeAdd64((kdu_int64 volatile *)&state,offset);
#         else // 32-bit pointers
            old_ptr = (void *)
              _InterlockedExchangeAdd((long volatile *)&state,(long)offset);
#         endif // 32-bit pointers
#       elif (defined KDU_GNU_ATOMICS)
          old_ptr = (void *)
            __sync_fetch_and_add(((char **)&state),
                                 (char *)_kdu_int32_to_addr(offset));
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          old_ptr = state;
          state = (void *)(((kdu_byte *) state) + offset);
#       endif        
        return old_ptr;
      }
    bool compare_and_set(void *ref_ptr, void *set_ptr)
      { /* [SYNOPSIS] Atomically compares the variable with the supplied
           `ref_ptr', returning false if the comparison fails (not equal),
           but returning true and setting the variable to `set_ptr' if the
           comparison succeeds (equal). */
#       if (defined KDU_MSC_ATOMICS)
#         ifdef KDU_POINTERS64
            void *old_ptr =
              _InterlockedCompareExchangePointer((void * volatile *)&state,
                                                 set_ptr,ref_ptr);
#         else // 32-bit pointers
            void *old_ptr = (void *)
              _InterlockedCompareExchange((long volatile *)&state,
                                          (long)set_ptr,(long)ref_ptr);
#         endif // 32-bit pointers
          return (old_ptr == ref_ptr);
#       elif (defined KDU_GNU_ATOMICS)
          return __sync_bool_compare_and_swap(&state,ref_ptr,set_ptr);
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       else
          if (state == ref_ptr)
            { state = set_ptr; return true; }
          return false;
#       endif
      }
    bool validate(void *ref_ptr)
      { /* [SYNOPSIS] Conveniently wraps up the memory barrier and testing
           operation required to verify that the value of the variable still
           equals the value supplied as `ref_ptr'.  One typical application
           for this is the robust implementation of hazard pointers. */
#       if (defined KDU_MSC_ATOMICS)
          MemoryBarrier();
#       elif (defined KDU_GNU_ATOMICS)
          __sync_synchronize();
#       elif ((defined KDU_PTHREADS) || (defined KDU_WIN_THREADS))
#         error "Compilation directive logic error -- maybe a mistyped symbol!"
#       endif
        return (state == ref_ptr);
      }
  private: // Data
    void *state;
  };

#endif // KDU_ELEMENTARY_H
