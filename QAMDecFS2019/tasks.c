/*
 * tasks.c
 *
 * Created: 05.04.2019 08:50:00
 *  Author: Claudio Hediger
 */ 

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "tasks.h"
#include "dma.h"

uint8_t count_buffer_a = 0;
uint8_t count_buffer_b = 0;
uint8_t position_array_a;
uint8_t position_array_b;
//EventGroupHandle_t xDMAProcessEventGroup;
//EventGroupHandle_t xSignalProcessEventGroup;


void vTask_DMAHandler(void *pvParameters) 
{
	//Do things and Stuff with DMA!
	xDMAProcessEventGroup = xEventGroupCreate();
	EventBits_t uxBits;
	BaseType_t xResult;
	int i,n,count_after_peak = 0;
	PORTF.DIRSET = PIN1_bm; /*LED1*/
	PORTF.DIRSET = PIN2_bm; /*LED2*/
	PORTE.DIRSET = PIN0_bm;
	PORTE.DIRSET = PIN1_bm;
	
	while(1)
	{
		uxBits = xEventGroupWaitBits(
		xDMAProcessEventGroup,   /* The event group being tested. */
		DMA_EVT_GRP_BufferA | DMA_EVT_GRP_BufferB, /* The bits within the event group to wait for. */
		pdTRUE,        /* Bits should be cleared before returning. */
		pdFALSE,       /* Don't wait for both bits, either bit will do. */
		portMAX_DELAY );/* Wait a maximum for either bit to be set. */
			
		//Check Event bits
		if(uxBits & DMA_EVT_GRP_BufferA)
		{
			//Do stuff with BufferA
			//buffer_a ....
			for (i=0;i<buffer_length;i++)//Detect signal 
			{
				if (buffer_a[i] >= 20)
				{
					xResult = xEventGroupSetBits(
								xSignalProcessEventGroup,		/* The event group being updated. */
								Process_Signal_BufferB			/* The bits being set. */
								);
					if( xResult & Process_Signal_BufferB )		//test if Eventgroup bit is set
					{
						//count_array_a++;
					}
					count_after_peak = 0;
				}
				
				else											// if value is under threshold, it shall stop calculating the signal
				{
					count_after_peak++;							// wait 100 counts to make sure that signal has stopped
					if (count_after_peak >=100)					// no signal stop calculating -> set event bits to 0
					{											// no signal stop calculating -> set event bits to 0
						xResult = xEventGroupClearBits(			// clear event bits
											xSignalProcessEventGroup,
											Process_Signal_BufferA|Process_Signal_BufferB
											);												
					}
				}
			}
			count_buffer_a++;
			
			//Debug Output
			PORTF.OUT = (PORTF.OUT & (0xFF - 0x02));
			PORTF.OUT |= 0x04;	
		}
		
		else //When it was not DMA_EVT_GRP_BufferA, then it was probably B. Since we only use two bits!
		{						
			//Do stuff with BufferB
			//buffer_b ....
			for (i=0;i<buffer_length;i++)
			{				
				if (buffer_b[i] >= 20)
				{
				xResult = xEventGroupSetBits(
									xSignalProcessEventGroup,   /* The event group being updated. */
									Process_Signal_BufferA		/* The bits being set. */
									);	
				}
				if(xResult & Process_Signal_BufferA)			//test if Eventgroup bit is set
				{
					{
						//count_array_b++;
					}
					count_buffer_a = 0;
				}
				else											// if value is under threshold, it shall stop calculating the signal
				{
					count_after_peak++;					
					if (count_after_peak >=100)					// wait 100 counts to make sure that signal has stopped
					{									
						xResult = xEventGroupClearBits(			// clear event bits
												xSignalProcessEventGroup,
												Process_Signal_BufferA|Process_Signal_BufferB
												);										// no signal stop calculating -> set event bits to 0
						}
				}
			}
			count_buffer_b++;
			//Debug Output
			PORTF.OUT = (PORTF.OUT & (0xFF - 0x04));
			PORTF.OUT |= 0x02;
		}
	vTaskDelay(100 / portTICK_RATE_MS);
	}
	
}