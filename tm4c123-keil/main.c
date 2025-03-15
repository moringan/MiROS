#include <stdint.h>
#include "bsp.h"
#include "miros.h"

#include "TM4C123GH6PM.h" 

uint32_t stack_blinky1[40] __attribute__ ((aligned (8)));
OSThread blinky1; /* pointer to current top of stack */
void main_blinky1() {
	/* while loop has a total period of 2 ms */ 
	/* you can verify using a general purpose
	   timer like this: */
	uint32_t begin;
    uint32_t end;
    float time_ms;
	/* while loop to be measured */
	while (1) {
		
		begin = TIMER1->TAV; 
		uint32_t volatile i;
		for (i = 1500U; i!=0U; --i) { // for loop takes 1.2ms, slightly 
			BSP_ledGreenOn();         // longer than system clock tick
			BSP_ledGreenOff();
		}
		OS_delay(1U); // blocks for 1 tick, until the next system interrupt
		end = TIMER1->TAV;
		
		/* now calculate the execution time in ms */
		time_ms = (begin - end) * .00000002 * (float)1000; /* 50mhz so clock tick is 20ps */
    }
}


uint32_t stack_blinky2[40] __attribute__ ((aligned (8)));
OSThread blinky2;
void main_blinky2() {
    /* 54 ms total period */ 
	while (1) {
		uint32_t volatile i;
		for (i = 3*1500U; i!=0U; --i) { // 3.6ms
			BSP_ledGreenOn();
			BSP_ledGreenOff();
		}
		OS_delay(50U);
    }
	
}

uint32_t stack_idleThread[40] __attribute__ ((aligned (8)));

int main(void) {
	BSP_init();
	
	OSInit(stack_idleThread, sizeof(stack_idleThread)); 
	
	OSThread_start( &blinky1,
					5U,
					&main_blinky1,
					stack_blinky1, sizeof(stack_blinky1) );
	
	OSThread_start( &blinky2,
					1U,
					&main_blinky2,
					stack_blinky2, sizeof(stack_blinky2) );
	

    OS_run();
	
	return 0; // never reached, OS_run() does not return
		
}
