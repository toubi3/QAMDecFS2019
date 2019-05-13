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

int high_peak_a = 127;
int high_peak = 127;
int low_peak_a = 127;
int low_peak = 127;
int position_high_peak_a, position_high_peak_b, position_low_peak_a, position_low_peak_b;
int count_after_high_peak, count_after_low_peak;
int sinus_status;		// 0 = rising up; 1 = rising down


void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	xTaskCreate( vLedBlink, (const char *) "ledBlink", configMINIMAL_STACK_SIZE+10, NULL, 1, &ledTask);
	xTaskCreate( vRead_DMA, (const char *) "ledBlink", configMINIMAL_STACK_SIZE+500, NULL, 1, &my_read_DMA);
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
	int count_array_position_H, count_array_position_L = 0;
	int i = 0;
	int pos_peak_array[2048], neg_peak_array[2048], position_array_H[2048], position_array_L[2048];
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
			i = 0;
			for(i=0;i<3;i++)
			{
				// HIGH PEAK A
				if (buffer_a[i] > 127)
				{
					if (buffer_a[i] > high_peak)	//if buffer bigger than current high_peak
					{
						if (buffer_a[i] > 220)
						{
							high_peak = buffer_a[i];	// store new peak
							position_high_peak_a = i;	// store array position of new peak
						}
					}	
					else 
					{
						if(buffer_a[i] < 160)
						{
							pos_peak_array[count_array_position_H] = high_peak;	//
							position_array_H[count_array_position_H] = position_high_peak_a;
							count_array_position_H++;
							high_peak = 127;
						}
					}
				}
				// LOW PEAK	A
				else 
				{
					if (buffer_a[i] < low_peak)		//if buffer bigger than current high_peak
					{
						if (buffer_a[i] < 35)
						{
							low_peak = buffer_a[i];	// store new peak
							position_low_peak_a = i;	// store array position of new peak
						}
					}
					else
					{
						if (buffer_a[i] > 100)
						{
							neg_peak_array[count_array_position_L] = low_peak;
							position_array_L[count_array_position_L] = position_low_peak_a;
							count_array_position_L++;
							low_peak = 127;
						}
					}	
				}	
			}
		}
		else if (uxBits & Process_Signal_BufferB)
		{
			for (i=0;i<3;i++)
			{
				// HIGH PEAK B
				if (buffer_b[i] > 127)
				{
					if (buffer_b[i] > high_peak)	//if buffer bigger than current high_peak
					{
						if (buffer_b[i] > 220)
						{
							high_peak = buffer_b[i];	// store new peak
							position_high_peak_b = i;	// store array position of new peak
						}
					}
					else
					{
						if(buffer_b[i] < 160)
						{
							pos_peak_array[count_array_position_H] = high_peak;	//
							position_array_H[count_array_position_H] = position_high_peak_b;
							count_array_position_H++;
							high_peak = 127;
						}
					}
				}
				// LOW PEAK	B				
				else
				{
					if (buffer_b[i] < low_peak)		//if buffer bigger than current high_peak
					{
						if (buffer_b[i] < 35)
						{
							low_peak = buffer_b[i];	// store new peak
							position_low_peak_b = i;	// store array position of new peak
						}
					}
					else
					{
						if (buffer_b[i] > 100)
						{
							neg_peak_array[count_array_position_L] = low_peak;
							position_array_L[count_array_position_L] = position_low_peak_b;
							count_array_position_L++;
							low_peak = 127;
						}
					}
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
			vDisplayWriteStringAtPos(1,0,"a: %d b: %d",buffer_a[1],buffer_b[1]);
			vDisplayWriteStringAtPos(2,0,"H: %d L: %d ",high_peak, low_peak);
			vDisplayWriteStringAtPos(3,0,"Hoi");
			vTaskStartScheduler();
			vTaskDelay(100 / portTICK_RATE_MS);
			
	}
}
