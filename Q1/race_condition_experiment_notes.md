
In this exercise, I used the values given to try and recreate the race issue, but it wasn't sufficient.

By increasing the number of threads contending, for the same variable, I was able to get better results.

Also learned the use of std::lock_guard and how it unlocks the mutex as it goes out of scope as well.

!(TerminalScreenshot.png)
The above shows the unsafe counter value changing and not being 0 as expected, at the end of 3 runs. The safe counter is 0 at the end of all runs, within expectations.

Screenshots of runs of Time Profiler and Processor Trace, giving IPC data as well as time in threads. As expected, the safe threads take more time.

!(PTTP.png)
!(TP.png)