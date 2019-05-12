/*
 * QAMDecFS2019.c
 *
 * Created: 20.03.2018 18:32:07
 * Author : chaos
 */ 

//#include <avr/io.h>
#include "avr_compiler.h"
#include "pmic_driver.h"
#include "TC_driver.h"
#include "clksys_driver.h"
#include "sleepConfig.h"
#include "port_driver.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "stack_macros.h"

#include "mem_check.h"

#include "dma.h"				
#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

#include "tasks.h"				  

extern void vApplicationIdleHook( void );
void vLedBlink(void *pvParameters);
void vRead_DMA(void *pvParameters);
void vWrite_Display(void *pvParameters);

TaskHandle_t ledTask;
TaskHandle_t my_read_DMA;
TaskHandle_t my_Display;
TaskHandle_t TaskDMAHandler;

int high_peak_a, high_peak_b,low_peak_a,low_peak_b;

void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	xTaskCreate( vLedBlink, (const char *) "ledBlink", configMINIMAL_STACK_SIZE+10, NULL, 1, &ledTask);
	xTaskCreate( vRead_DMA, (const char *) "ledBlink", configMINIMAL_STACK_SIZE+10, NULL, 1, &my_read_DMA);
	xTaskCreate( vWrite_Display, (const char *) "ledBlink", configMINIMAL_STACK_SIZE+10, NULL, 1, &my_Display);
	xTaskCreate( vTask_DMAHandler, (const char *) "dmaHandler", configMINIMAL_STACK_SIZE + 100, NULL, 1, &TaskDMAHandler);		
	xSignalProcessEventGroup = xEventGroupCreate();
	vInitDMA();			

	vDisplayClear();
	vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
	vDisplayWriteStringAtPos(1,0,"EDUBoard 1.0");
	vDisplayWriteStringAtPos(2,0,"Template");
	vDisplayWriteStringAtPos(3,0,"ResetReason: %d", reason);
	vTaskStartScheduler();
	return 0;
}

void vLedBlink(void *pvParameters) {
	(void) pvParameters;
	PORTF.DIRSET = PIN0_bm; /*LED1*/
	PORTF.OUT = 0x01;
	for(;;) {
		//PORTF.OUTTGL = 0x01;				
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}
void vRead_DMA(void *pvParameters)
{
	int pos_peak,neg_peak = 0;
	int i = 0;
	EventBits_t uxBits;
	for (;;)
	{			
				uxBits = xEventGroupWaitBits(
				xSignalProcessEventGroup,   /* The event group being tested. */
				Process_Signal_BufferA | Process_Signal_BufferB, /* The bits within the event group to wait for. */
				pdTRUE,        /* Bits should be cleared before returning. */
				pdFALSE,       /* Don't wait for both bits, either bit will do. */
				portMAX_DELAY );/* Wait a maximum for either bit to be set. */
								
		//process signal values
			if (uxBits & Process_Signal_BufferA) // if "BufferA" bit is set, read out bufferA
			{
				for(i=0;i<200;i++)
				{
					if (buffer_a[i] > high_peak_a)
						{
							high_peak_a = buffer_a[i];
						}				
				}
			}
			else
			{
				for (i=0;i<200;i++)
				{
					if (buffer_b[i] > high_peak_b)
						{
							high_peak_b = buffer_b[i];
						}
				}
			}
		vTaskDelay(100 / portTICK_RATE_MS);
	}
	
}
void vWrite_Display(void *pvParameters){
	EventBits_t uxBits;
	for (;;)
	{
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
			vDisplayWriteStringAtPos(1,0,"buffer_a:b %d : %d",buffer_a[1],buffer_b[1]);
			vDisplayWriteStringAtPos(2,0,"peak: %d  : %d ",high_peak_a,high_peak_b);
			vDisplayWriteStringAtPos(3,0,"a:%d   b:%d",count_array_a,count_array_b);
			vTaskStartScheduler();
			vTaskDelay(100 / portTICK_RATE_MS);
			
	}
}
