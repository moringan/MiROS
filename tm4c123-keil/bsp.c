/* Board Support Package (BSP) for the EK-TM4C123GXL board */
#include <stdint.h>  /* Standard integers. WG14/N843 C99 Standard */

#include "bsp.h"
#include "miros.h"
#include "qassert.h"

#include "TM4C123GH6PM.h" /* the TM4C MCU Peripheral Access Layer (TI) */

/* on-board LEDs */
#define LED_RED   (1U << 1)
#define LED_BLUE  (1U << 2)
#define LED_GREEN (1U << 3)
#define SWITCH_2  (1U << 0)

static uint32_t volatile l_tickCtr;

/* ISRs  ===============================================*/
void SysTick_Handler(void) {

	OS_tick();
	/* critical section */
	__asm volatile ("cpsid i" : : : "memory");
	OSSched();
	__asm volatile ("cpsie i" : : : "memory");

}

void Timer1A_IRQHandler(void){
	/* ISR for timer1a */
	if ((TIMER1->RIS & (1u << 0)) == 1U) {
		TIMER1->ICR |= (1U << 0); //clear the timeout flag
		BSP_ledBlueToggle();
	}
}


/* OS Callback functions ========================================= */ 

/* callback to configure and start interrupts */
void OS_onStartup(void) {
    SystemCoreClockUpdate();
	uint32_t time = SystemCoreClock / BSP_TICKS_PER_SEC;
    SysTick_Config(SystemCoreClock / BSP_TICKS_PER_SEC);
	
	/* Set the SysTick interrupt priority to the highest level */
	NVIC_SetPriority(SysTick_IRQn, 0U);
	
	/* User enabled interrupt and their priorities ... */
	/*NVIC->ISER[0] |= (1U << 21); //timer 1A interrupt enable
	NVIC_SetPriority(TIMER1A_IRQn, 1U);*/
}

/* application can perform processing here */
void OS_onIdle(void) {
	/* stop the CPU and wait for interrupt */
	__WFI(); 

}


/* BSP functions ===========================================================*/

void BSP_checkTimer(void) {
	/* use this to poll for the timer timeout flag
		if you don't want to use the interrupt */
	if ((TIMER1->RIS & (1u << 0)) == 1U) {
		TIMER1->ICR = (1U << 0); //clear the timeout flag
		BSP_ledBlueToggle();
	}
}

void BSP_initTIMER1A(void) {
	/* TIMER1A decrements from 0x02FAF080 to zero (1 second)
	   then triggers the timer1a interrupt */
	SYSCTL->RCGCTIMER |= (1U << 1); // enable clock for timer1a
	TIMER1->CTL &= ~(1U << 0); //disable before configuring
	TIMER1->CTL &= ~(1U << 8); //disable before configuring
	TIMER1->CFG = 0x0U; // 32 bit mode
	TIMER1->TAMR |= 0x02; //periodic
	TIMER1->TAMR &= ~(1<<4); //down counter
	TIMER1->TAILR = 0x02FAF080; //timer period (50mhz)
	
	TIMER1->ICR = (1U << 0); //clear the timeout flag
	TIMER1->IMR |= (1U << 0); //enable TIMER1A interrupt mask
	TIMER1->CTL |= (1U << 0); //enable timer
}


void BSP_init(void) {
    SYSCTL->RCGCGPIO  |= (1U << 5); /* enable Run mode for GPIOF */
    SYSCTL->GPIOHBCTL |= (1U << 5); /* enable AHB for GPIOF */
	
    GPIOF_AHB->DIR |= (LED_RED | LED_BLUE | LED_GREEN);
    GPIOF_AHB->DEN |= (LED_RED | LED_BLUE | LED_GREEN);
	
	BSP_initTIMER1A();
}


/* LED functions ================================================== */
/* uses the more complex GPIO data register mapping which reduces race conditions */

void BSP_ledRedOn(void) {
    GPIOF_AHB->DATA_Bits[LED_RED] = LED_RED;
}

void BSP_ledRedOff(void) {
    GPIOF_AHB->DATA_Bits[LED_RED] = 0U;
}

void BSP_ledBlueOn(void) {
    GPIOF_AHB->DATA_Bits[LED_BLUE] = LED_BLUE;
}

void BSP_ledBlueOff(void) {
    GPIOF_AHB->DATA_Bits[LED_BLUE] = 0U;
}

void BSP_ledGreenOn(void) {
    GPIOF_AHB->DATA_Bits[LED_GREEN] = LED_GREEN;
}

void BSP_ledGreenOff(void) {
    GPIOF_AHB->DATA_Bits[LED_GREEN] = 0U;
}

void BSP_ledGreenToggle(void) {
	GPIOF_AHB->DATA_Bits[LED_GREEN] ^= LED_GREEN;
}
void BSP_ledBlueToggle(void) {
	GPIOF_AHB->DATA_Bits[LED_BLUE] ^= LED_BLUE;
}
void BSP_ledRedToggle(void) {
	GPIOF_AHB->DATA_Bits[LED_RED] ^= LED_RED;
}
/* ======================================================================= */

_Noreturn void Q_onAssert(char const * const module, int const id) {
    (void)module; // unused parameter
    (void)id;     // unused parameter
#ifndef NDEBUG
    // light up all LEDs
    GPIOF_AHB->DATA_Bits[LED_GREEN | LED_RED | LED_BLUE] = 0xFFU;
    // for debugging, hang on in an endless loop...
    for (;;) {
    }
#endif
    NVIC_SystemReset();
}
//............................................................................
_Noreturn void assert_failed(char const * const module, int const id);
_Noreturn void assert_failed(char const * const module, int const id) {
    Q_onAssert(module, id);
}