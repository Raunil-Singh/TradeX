<h1>MPMC Queues, Lock-free</h1>
Lock-free MPMC queues were tough to even think about. Mutexes offer limited access to resources, which means less thinking of the consequences of pre-emption by other queues and simplified the problem of data races.
Lock-free means no thread is blocked. Atomics are the only tools you possess.
<h2> First Idea </h2>
In my first version, I reasoned that in an ever changing situation, you can only look at a snapshot of the values and make a decision based on that. This led to the idea of storing local copies, and then make all decisions based on that. I also had a CAS (Compare and Swap) which would allow me to determine whether a new snapshot looked like the previous one, and then change the value as I liked upon success of the check.

This led to the rudimentary logic:-
<h3>Producer:</h3>
acquire tail and head, making all writes visible, make a local copy of the value?
do a local check for fullness (tail + 1 == head). retry if true later (spinning)
after making sure queue is not full, compare if tail == local copy, if so, store and update tail.
if tail != local copy, update local copy's value and go back to the fullness check

<h3>Consumer: </h3>
acquire head and tail, making all previous writes visible,  make a local copy
do a check for emptiness (head == tail), spin if true
after making sure quene not empty, compare if head == local_copy, if yes consume and update head
if head != local_copy, uupdate local copy and go back to emptiness check

This had its issues, which didn't seem too obvious to me.
The main one was, that I didn't seem to get that a "slot" in the queue could be in 3 states at any instant- **acquired, written/filled and consumed**. Any threads that say both claimed their local copies of tail, could then try to write to it. But if one slept, the other woke up, wrote to it, and then went back to sleep, just for the latter to wake up and rewrite the data the former thread had written. This is known as the **ABA** problem.

<h2>Vyukov's Queue</h2>
Vyukov's queue was simple. It determined that we can encode the idea of generation and state in one go, where generations represented the cycles of acquire, filled and consumed the queue had been through, as well as its current state, using a property called **"Sequence"**. Head and tail both would be ever incrementated, but by taking the remainder of their values using queue size as divisor, we could determine the index to be written to, and the tail itself gave a sense of the generation (can be thought of as division). Only threads possessing the right generation would be given access to the slot, and even if competing threads acquired the correct generation, CAS ensured that only one thread win's the race. Very neat.

Role of generation :- idk

<h2>Atomics Everywhere, and why Memory Fences are used</h2>
So far, I had only used atomics for head and tail, and hadn't changed much else. Also, I was operating on the false assumption of how loads and stores of atomic operations and the safeguards of atomic operations worked.

<h3>Why did slots as well need to be atomic</h3>
The core idea of lock-free programming is that you can never guarantee your thread will make any progress when given the cpu time slice by the scheduler. A producer may acquire the slot, and update the sequence accordingly. But unless the sequence itself was atomic, we would never be able to guarantee that the consumer, if also looking at the sequence at that instant, actually saw that the sequence was updated and the slot was filled and ready for consumption. Atomicity would guarantee coherence by letting the core on which the consumer was running that its cache line was invalid and that it needed to fetch a new copy. 

<h3> Memory fences and how they save you </h3>
When a producer is allowed to write into the slot, it usually updates data as well as sequence. The consumer will also update the sequence, and consume the data. Those are two memory operations, which if reordered, could lead the sequence to be updated first from the producer, and the data loaded by the consumer, when data wasn't filled by the producer here. This is where **Acquire-Release** saves us. Acquire disallows any memory operation to be reordered after the memory operation to which it was attached. Release disallows any memory operation to be reordered before the memory operation to which it was attached. This allows us to ensure that data is filled before sequence update by the producer, and sequence is checked first for validity before the data is loaded from the slot. Thus, the Acquire-Release pair allows us to ensure safety from data races.


<h3> Some other optimizations</h3>
Solved cache-line ping pong due to false sharing on global variables, by padding to cache line widths.
Solved false sharing due to a global order-count variable and gave fixed workload to producer threads.
Implemented back-off using "yield" on arm machines which does a spin of cpu cycles to reduce contention for CAS, with spins increasing with failure, and finally backing off when failure rate became too high. 
