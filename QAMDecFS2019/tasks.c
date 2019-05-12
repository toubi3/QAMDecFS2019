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
uint16_t count_array_a;
uint16_t count_array_b;
uint8_t position_array_a;
uint8_t position_array_b;
EventGroupHandle_t xDMAProcessEventGroup;
EventGroupHandle_t xSignalProcessEventGroup;

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
			for (i=0;i<2047;i++)//Detect signal 
			{
				if (buffer_a[i] >= 20)
				{
					xResult = xEventGroupSetBits(
								xSignalProcessEventGroup,   /* The event group being updated. */
								Process_Signal_BufferB /* The bits being set. */
								);
					if( xResult & Process_Signal_BufferB )
					{
						count_array_a++;
					}
					count_after_peak = 0;
				//i = 0;
				}
				else
				{
					count_after_peak++;
					if (count_after_peak >=100)
					{
						//kein signal mehr, stoppe berechnung
					}
				}
					
				//{
					//PORTE.OUT &= ~0x02;
					//PORTE.OUT |= 0x01;
					//i = 0;
				//}
				//else
				//{
					//PORTE.OUT &= ~0x01;
					//PORTE.OUT |= 0x02;
					//for(n=0;n<20;n++)
					//{
						//wenn pegel für 20 werte kleiner als 10 ist -> kein signal mehr -> stop timer
					//}
				//}
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
			i = i;
			for (i=0;i<2047;i++)
			{				
				if (buffer_b[i] >= 20)
				{
				xResult = xEventGroupSetBits(
									xSignalProcessEventGroup,   /* The event group being updated. */
									Process_Signal_BufferA /* The bits being set. */
									);
				if( xResult & Process_Signal_BufferA )
					{
						count_array_b++;
					}
					count_buffer_a = 0;
				}
				else
				{
					count_after_peak++;
					if (count_after_peak >=100)
					{
						//kein signal mehr, stoppe berechnung
					}
				}
				 //detect signal
				//{
				//	i = 0;
					//PORTE.OUT &= ~0x02;
					//PORTE.OUT |= 0x01;
				//}
				//else {
					//PORTE.OUT &= ~0x01;
					//PORTE.OUT |= 0x02;
					//for(n=0;n<20;n++){
						//wenn pegel für 20 werte kleiner als 10 ist -> kein signal mehr -> stop timer
					//}
				//}
			}
			count_buffer_b++;
			//Debug Output
			PORTF.OUT = (PORTF.OUT & (0xFF - 0x04));
			PORTF.OUT |= 0x02;
		}
	vTaskDelay(100 / portTICK_RATE_MS);
	}
	
}