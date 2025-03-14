/****************************************************************************
* MInimal Real-time Operating System (MiROS), ARM-CLANG port.
* version 0.26
*
* This software is a teaching aid to illustrate the concepts underlying
* a Real-Time Operating System (RTOS). The main goal of the software is
* simplicity and clear presentation of the concepts, but without dealing
* with various corner cases, portability, or error handling. For these
* reasons, the software is generally NOT intended or recommended for use
* in commercial applications.
*
* From the excellent tutorials by Quantum Leaps!
*
* Copyright (C) 2025 CMS. All Rights Reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <https://www.gnu.org/licenses/>.
*
* Git repo:
* https://github.com/moringan
****************************************************************************/
#ifndef MIROS_H
#define MIROS_H

#include <stdint.h>

/* Thread Control Block (TCB) */
typedef struct {
	void *sp;  /* stack pointer */
	uint32_t timeout; /* timeout delay down-counter */
	uint8_t prio; /* thread priority */
} OSThread;

typedef void (*OSThreadHandler)();

/* MiROS API ============================================================*/

void OSInit(void *stkSto, uint32_t stkSize);

/* callback to handle the idle condition */
void OS_onIdle(void);

/* blocking delay, needed to run other threads */
void OS_delay(uint32_t ticks);

/* processes all timeouts of delay functions */
void OS_tick(void);

/* transfer control to the OS and run the threads */
void OS_run(void);

/* callback to configure and start interrupts */
void OS_onStartup(void);

/* this function must be called with interrupts DISABLED */
void OSSched(void);

/* build the thread stack frame, set priority of thread */
void OSThread_start (
	OSThread *me,
	uint8_t prio,
	OSThreadHandler threadhandler,
	void *stkSto, uint32_t stkSize);


#endif /* MIROS_H */