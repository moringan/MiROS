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
* Copyright (C) 2025 Mike Stapleton. All Rights Reserved.
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
#include <stdint.h>
#include "miros.h"
#include "qassert.h"
#include "arm_acle.h"


Q_DEFINE_THIS_FILE

/* finds the highest priority thread number */
#define LOG2(x) (32U - __clz(x)) 

OSThread * volatile OS_curr; /* pointer to current thread */
OSThread * volatile OS_next; /* pointer to the next */

OSThread * OS_thread[32 + 1]; /* 32 threads plus the idle thread */

uint32_t OS_readySet; /* bitmask of threads that are ready to run */
uint32_t OS_delayedSet; /* bitmask of threads that are delayed */

OSThread idleThread;


void main_idleThread() {
    while (1) {
		/* callback function, allows application to perform processing */
        OS_onIdle(); 
    }
}

void OS_run(void) {
	
	/* callback to configure and start interrupts */
	OS_onStartup(); 
	/* a 'callback' fucntion is not defined in kernel but by the application */	
	 
	/* Context is switched to 1st scheduled thread and
	   doesn't return */
	__asm volatile ("cpsid i" : : : "memory");
	OSSched();
	__asm volatile ("cpsie i" : : : "memory");
	
	/* this code should never run */
	Q_ERROR();
}

/* initialize the idle thread, stack size left to application
   because it's not known how much stack size is needed */
void OSInit(void *stkSto, uint32_t stkSize) {
	/* first thread made ready is the idle Thread */
	OSThread_start( &idleThread,
					0U,
					&main_idleThread,
					stkSto, stkSize );
	
	/* set the PendSV interrupt priority to the lowest level */
	*(uint32_t volatile *)0xE000ED20 |= (0xFFU << 16);
}

/* decrements the counters of all threads with non-zero timeouts,
   threads with timeouts that just reached zero need to be unblocked */
void OS_tick(void) {
	uint32_t workingSet = OS_delayedSet;
	while (workingSet != 0U) {
		OSThread *t = OS_thread[LOG2(workingSet)];
		uint32_t bit;
		Q_ASSERT((t != (OSThread *)0U) && (t->timeout != 0U));
		
		bit = (1U << (t->prio - 1U));
		--t->timeout;
		if(t->timeout == 0U) {
			OS_readySet |= bit;
			OS_delayedSet &= ~bit;
		}
		workingSet &= ~bit;
	}
}

void OS_delay(uint32_t ticks) {
	uint32_t bit;

	__asm volatile ("cpsid i" : : : "memory");
	
	/* forbid from being called from the idle thread because
	   should never go to the blocked state */
	Q_REQUIRE(OS_curr != OS_thread[0]); 
	/* Q_REQUIRE is just like Q_ASSERT but the name conveys the intent
	   more precisely */
    
	OS_curr->timeout = ticks; /* set the current thread timeout counter */
	bit = (1U << (OS_curr->prio - 1U)); /* if prio 1, first bit in mask is set */
	OS_readySet &= ~bit; /* remove thread from ready bitmask */
	OS_delayedSet |= bit; /* add thread from delayed bitmask */
	OSSched(); /* switch out of current thread and to next available thread */
	
	__asm volatile ("cpsie i" : : : "memory");
}

void OSSched(void) {
/*  priority-based scheduler, chooses the thread 
	with highest priority that is in ready state */

	if(OS_readySet == 0U) { /* idle condition? */
		OS_next = OS_thread[0]; /* idle thread */
	}
	else {
		/* schedule only the thread with the highest priority number */
		OS_next = OS_thread[LOG2(OS_readySet)]; 
		Q_ASSERT(OS_next != (OSThread *)0U);
	}
	/* set PendSV handler to run */
	if(OS_next != OS_curr) {
		*(uint32_t volatile *)0xE000ED04 = (1U << 28);
	}

}

void OSThread_start (
		OSThread *me,  /*  the thread object  */
		uint8_t prio,
		OSThreadHandler threadHandler,
		void *stkSto, uint32_t stkSize ) {

    /* round down stack top to 8 byte boundary 
	   stack grows from hi to lo memory for cortex m
	   -note sp is one word below the top of the stack */
	uint32_t *sp = (uint32_t *)((((uint32_t)stkSto + stkSize) / 8) * 8);
	uint32_t *stk_limit;

	/* check if prio number is in range (32 + 1)
	   and that the thread priority is not already used */
	Q_REQUIRE((prio < Q_DIM(OS_thread)) &&
			(OS_thread[prio] == (OSThread *)0));
	/* a precondition means that it must be satisfied by 
	   the caller of the function, not by the function itself */
			
		
/* fabricate Cortex-M ISR stack frame for sp */
    *(--sp) = (1U << 24);  /* xPSR */
    *(--sp) = (uint32_t)threadHandler; /* PC */
    *(--sp) = 0x0000000EU; /* LR  */
    *(--sp) = 0x0000000CU; /* R12 */
    *(--sp) = 0x00000003U; /* R3  */
    *(--sp) = 0x00000002U; /* R2  */
    *(--sp) = 0x00000001U; /* R1  */
    *(--sp) = 0x00000000U; /* R0  */
    /* additionally, fake registers R4-R11 */
	*(--sp) = 0x0000000BU; /* R11 */
    *(--sp) = 0x0000000AU; /* R10 */
    *(--sp) = 0x00000009U; /* R9 */
    *(--sp) = 0x00000008U; /* R8 */
    *(--sp) = 0x00000007U; /* R7 */
    *(--sp) = 0x00000006U; /* R6 */
    *(--sp) = 0x00000005U; /* R5 */
    *(--sp) = 0x00000004U; /* R4 */
    
	/* save the top of the stack in the thread's attribute */
	me->sp = sp;
	
	/* round up the bottom of the stack to the 8-byte boundary */
	stk_limit = (uint32_t *)(((((uint32_t)stkSto - 1U) / 8) + 1U) * 8);
	
	/* prefill the unused part of the stack with 0xDEADBEEF */
	for (sp = sp - 1U; sp >= stk_limit; --sp) {
		*sp = 0xDEADBEEF;
	}
	
	/* register the thread with the OS */
	OS_thread[prio] = me;
	me->prio = prio;
	
	/* make the thread ready to run */
	if (prio > 0U) {
		OS_readySet |= (1U << (prio - 1U));
	}
	
}

/* inline assembly syntax for Compiler 6 (ARMCLANG) */
__attribute__ ((naked))
void PendSV_Handler(void) {
__asm volatile (
    /* __disable_irq(); */
    "  CPSID         I                 \n"

    /* if (OS_curr != (OSThread *)0) { */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  CMP           r1,#0             \n"
    "  BEQ           PendSV_restore    \n"

    /*     push registers r4-r11 on the stack */
#if (__ARM_ARCH == 6)               // if ARMv6-M...
    "  SUB           sp,sp,#(8*4)     \n" // make room for 8 registers r4-r11
    "  MOV           r0,sp            \n" // r0 := temporary stack pointer
    "  STMIA         r0!,{r4-r7}      \n" // save the low registers
    "  MOV           r4,r8            \n" // move the high registers to low registers...
    "  MOV           r5,r9            \n"
    "  MOV           r6,r10           \n"
    "  MOV           r7,r11           \n"
    "  STMIA         r0!,{r4-r7}      \n" // save the high registers
#else                               // ARMv7-M or higher
    "  PUSH          {r4-r11}          \n"
#endif                              // ARMv7-M or higher

    /*     OS_curr->sp = sp; */
    "  LDR           r1,=OS_curr       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  MOV           r0,sp             \n"
    "  STR           r0,[r1,#0x00]     \n"
    /* } */

    "PendSV_restore:                   \n"
    /* sp = OS_next->sp; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           r0,[r1,#0x00]     \n"
    "  MOV           sp,r0             \n"

    /* OS_curr = OS_next; */
    "  LDR           r1,=OS_next       \n"
    "  LDR           r1,[r1,#0x00]     \n"
    "  LDR           r2,=OS_curr       \n"
    "  STR           r1,[r2,#0x00]     \n"

    /* pop registers r4-r11 */
#if (__ARM_ARCH == 6)               // if ARMv6-M...
    "  MOV           r0,sp             \n" // r0 := top of stack
    "  MOV           r2,r0             \n"
    "  ADDS          r2,r2,#(4*4)      \n" // point r2 to the 4 high registers r7-r11
    "  LDMIA         r2!,{r4-r7}       \n" // pop the 4 high registers into low registers
    "  MOV           r8,r4             \n" // move low registers into high registers
    "  MOV           r9,r5             \n"
    "  MOV           r10,r6            \n"
    "  MOV           r11,r7            \n"
    "  LDMIA         r0!,{r4-r7}       \n" // pop the low registers
    "  ADD           sp,sp,#(8*4)      \n" // remove 8 registers from the stack
#else                               // ARMv7-M or higher
    "  POP           {r4-r11}          \n"
#endif                              // ARMv7-M or higher

    /* __enable_irq(); */
    "  CPSIE         I                 \n"

    /* return to the next thread */
    "  BX            lr                \n"
    );
}