

# Minimal RTOS Kernel (MiROS)
### Designed with simplicity in mind to demonstrate the core concepts of a Real Time Operating System
- a usable, minimal RTOS that can multi-task up to 32 threads
- illustrates the core concepts of how a RTOS kernel is designed and written in C
- shows how to implement preemptive scheduling of threads
- uses a fast priority-based scheduler
- designed to meet hard real-time demands using thread priorities
- demonstrates using RMS (Rate Monotonic Scheduling) sometimes also called RMA
- from the excellent tutorials by Quantum Leaps, LLC!

### Rate Monotonic Scheduling (RMS)
Priority-based scheduling of blocking threads allows for meeting hard real-time deadlines. Priorities can be assigned to threads using concepts from RMS:
- assign higher priorities to threads with higher rates than threads with lower rates (ie, shorter period threads get higher priority)
- find the CPU utilization of each thread by calculating the thread's execution time and dividing by the thread's period:  C(n)/T(n)
- add the CPU utilization of all threads:  C(1)/T(1) + ... + C(n)/T(n)
- if the total utilization is below the theoretical bound, all threads in the set are guaranteed to meet their hard real-time deadlines
- in practice, keep the utilization below 70%

For example, in main.c there are two periodic threads, blinky1 and blinky2:

	blinky1 has an execution time of 1.2ms and a period of 2ms, so the cpu utilization is 60%

	blinky2 has an execution time of 3.6ms and a period of 54ms, so the cpu utilization is 6%

	60% + 6% = 66%, which is below 70% so both threads will be able to meet their hard real-time deadlines

Additional considerations:
- use worst case for aperiodic tasks (shortest time between activations
	and longest execution time)
- prioritize soft real time threads lower 


### Source Files
```
MiROS/tm4c123-keil/miros.h & miros.c     MiROS API and implementation
MiROS/tm4c123-keil/bsp.h & bsp.h         Board Support Package for TivaC
MiROS/tm4c123-keil/main.c                User Application 
```

### Sub-directories
```
|
+---CMSIS           - CMSIS (Cortex Microcontroller Software Interface Standard)
|
+---ek-tm4c123gxl   - support code for EK-TM4C123GXL (TivaC Launchpad) board
|
\---tm4c123-keil
        ...
        lesson.uvprojx - KEIL project for TM4C123 (TivaC LaunchPad)
```
