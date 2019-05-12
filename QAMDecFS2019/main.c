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
	int pos_peak,neg_peak = 0;
	int count_array_position_H, count_array_position_L;
	int i = 0;
	int pos_peak_array[2047], neg_peak_array[2047], position_pos_array[2047], position_neg_array[2047];
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
				for(i=0;i<2047;i++)
				{
					if (buffer_a[i] > high_peak_a)	//if buffer bigger than current high_peak
					{
						high_peak_a = buffer_a[i];	// store new peak
						position_high_peak_a = i;	// store array position of new peak
					}				
					else if(100 < buffer_a[i] < high_peak_a)
					{
						count_after_high_peak++;	//count up -> to make sure, that it's no distortion
						sinus_status = 1;		// status
					}
					if (buffer_a[i] < low_peak_a)		//if buffer bigger than current high_peak
					{
						low_peak_a = buffer_a[i];	// store new peak
						position_low_peak_a = i;	// store array position of new peak
					}
					else if ((100 > buffer_a[i]) &&(buffer_a[i] > low_peak_a))	//if buffer value below 0 and bigger than low peak ***sinus increases***
					{
						count_after_low_peak++;		//count up -> to make sure, that it's no distortion
						sinus_status = 0;		//status
					}
					/* store detected peak values in array:*/
					if ((count_after_high_peak > 50))
					{
						if (high_peak_a > 200)
						{
							pos_peak_array[count_array_position_H] = high_peak_a;					//store peak in array
							position_pos_array[count_array_position_H] = position_high_peak_a;		//store position of peak in array
							count_array_position_H++;												//increase array position
							high_peak_a = 0;														//reset peak value to let it detect peaks again from 0 
						}
					}
					if (count_after_low_peak > 50)
					{
						if (low_peak_a < 50)
						{
							neg_peak_array[count_array_position_L] = low_peak_a;
							position_neg_array[count_array_position_L] = position_low_peak_a;
							count_array_position_L++;
							low_peak_a = 0;
						}
					}
					
					
				}

			}
			else if (uxBits & Process_Signal_BufferB)
			{
				for (i=0;i<2047;i++)
				{
					if (buffer_b[i] > high_peak_b)	//if buffer bigger than current high_peak
					{
						high_peak_b = buffer_b[i];	// store new peak
						position_high_peak_b = i;	// store array position of new peak
					}				
					else if(100 < buffer_b[i] < high_peak_b)
					{
						count_after_high_peak++;	//count up -> to make sure, that it's no distortion
						sinus_status = 1;		// status
					}
					if (buffer_b[i] < low_peak_b)		//if buffer bigger than current high_peak
					{
						low_peak_b = buffer_b[i];	// store new peak
						position_low_peak_b = i;	// store array position of new peak
					}
					else if ((100 > buffer_b[i]) &&(buffer_b[i] > low_peak_b))	//if buffer value below 0 and bigger than low peak ***sinus increases***
					{
						count_after_low_peak++;		//count up -> to make sure, that it's no distortion
						sinus_status = 0;		//status
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
			vDisplayWriteStringAtPos(3,0,"Status: %d",sinus_status);
			vTaskStartScheduler();
			vTaskDelay(100 / portTICK_RATE_MS);
			
	}
}
