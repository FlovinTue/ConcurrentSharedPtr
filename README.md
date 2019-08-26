# atomic_shared_ptr 
(previously ConcurrentSharedPtr)

My take on an atomic shared pointer. 

- Lock-Free (if used with a lock-free allocator)
- It uses an interface resembling that of an std::atomic type
- Uses internal versioning to protect against ABA problems
- Does not (currently) support the use of differing memory orderings
- Does not (currently) use std::shared_ptr, but a structure of similar design

Instructions:

Include atomic_shared_ptr.h, atomic_oword.h and go.
Optionally add atomic_shared_ptr.natvis to your project for debug info viewing in Visual Studio


There are four different versions of compare_exchange_strong, two of which take versioned_raw_ptr as expected value. These are non-reference counted representations of shared_ptr/atomic_shared_ptr. It is recommended to use these with compare_exchange if shared ownership is not needed of the expected out value, as the shared_ptr variants incur extra costs upon failure.
