# ConcurrentSharedPtr

A thread-safe shared pointer object that allows dynamic garbage collection of resources with ambiguous life times. Lock-free.

Includes needed:

ConcurrentSharedPtr.h

AtomicOWord.h


-------------------------------------------------------------------------------------------------------------------------------


To clarify a bit what is and what is not concurrency safe: Resetting, Assigning from or to, claiming an object to, or moving to a pointer object is concurrency safe, and will properly increment & decrement the relevant counters. Optionally moving from a pointer object can be made safe as well via template argument, or refining the default value in the ConcurrentSharedPtr.h file. 

The accessor methods / operators (->, * , [] etc). are NOT concurrency safe for performance reasons. 

Additionally there are lots of fast unsafe versions of methods, for when an object is known to be thread-private.
