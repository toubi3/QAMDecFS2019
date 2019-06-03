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
void vPhase_Detection(void *pvParameters);
void vWrite_Display(void *pvParameters);
#define Process_Phase_detectionA ( 1 << 0 )
#define Process_Phase_detectionB ( 1 << 1 )
#define peak_array_length (3)

TaskHandle_t ledTask;
TaskHandle_t my_read_DMA;
TaskHandle_t my_phase_detection;
TaskHandle_t my_Display;
TaskHandle_t TaskDMAHandler;
EventGroupHandle_t xPhaseDetectionEventGroup;

int high_peak_a = 127;
int high_peak = 127;
int low_peak_a = 127;
int low_peak = 127;
int count_array_a = 0;
int count_array_b = 0;
int position_high_peak_a, position_high_peak_b, position_low_peak_a, position_low_peak_b;
int peak_array_H[64], peak_array_L[64], position_array_H[64], position_array_L[64];
int sinus_status;		// 0 = rising up; 1 = rising down
int count_after_peak;




void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	xTaskCreate( vLedBlink, (const char *) "ledBlink", configMINIMAL_STACK_SIZE+10, NULL, 1, &ledTask);
	xTaskCreate( vRead_DMA, (const char *) "read_DMA", configMINIMAL_STACK_SIZE+100, NULL, 1, &my_read_DMA);
	xTaskCreate( vPhase_Detection, (const char *) "phase_detect", configMINIMAL_STACK_SIZE+10, NULL, 1, &my_phase_detection);
	xTaskCreate( vWrite_Display, (const char *) "display", configMINIMAL_STACK_SIZE+10, NULL, 1, &my_Display);
	xTaskCreate( vTask_DMAHandler, (const char *) "dmaHandler", configMINIMAL_STACK_SIZE + 100, NULL, 1, &TaskDMAHandler);		
	xSignalProcessEventGroup = xEventGroupCreate();
	xPhaseDetectionEventGroup = xEventGroupCreate();
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

void vPhase_Detection(void *pvParameters) // becomes peak array and position -> detects the phase shift
{
	int uxBits;
	int periode_H,periode_L,periode_mittelwert;
	int phase_H,phase_L;
	int phase_detect_results;
	int pruef_variable;
	int i=1;
	for (;;)
	{
		uxBits = xEventGroupWaitBits(
								xPhaseDetectionEventGroup,  /* The event group being tested. */
								Process_Phase_detectionA | Process_Phase_detectionB, /* The bits within the event group to wait for. */
								pdTRUE,        /* Bits should be cleared before returning. */
								pdFALSE,       /* Don't wait for both bits, either bit will do. */
								portMAX_DELAY );/* Wait a maximum for either bit to be set. */								

		/*Phase Detection Buffer A*/ 
		if (uxBits & Process_Phase_detectionA) // if Bit phase detection is set -> read out peak array
		{
			periode_H = position_array_H[0]*4;
			periode_L = position_array_L[0]/3*4;
			periode_mittelwert = position_array_H[0] + position_array_L[0]; //(periode_H + periode_L)/2;
			for (i=1;i<peak_array_length;i++)
			{
				phase_H = (periode_mittelwert*(i+1) - position_array_H[i]);
				phase_L = (periode_mittelwert*(i+1) - position_array_L[i]);
				// Phase High detect
				if(phase_H > 13)
				{
					 if (phase_H >= 18)
					 {
						//phase ist -90 grad
						 pruef_variable = phase_H;
					 }
					 else
					 {
						//phase ist 0 grad
						 pruef_variable = phase_H/3*4;
					 }
				}
				else
				{
					if (phase_H <= 8)
					{
						//phase ist 180 grad
						 pruef_variable = phase_H * 4;
					}
					else//
					{
						//phase ist + 90 grad
						 pruef_variable = phase_H;
					}
				}
				// Phase Low detect
				if(phase_L >= 13)
				{
					 if (phase_L >= 18)
					 {
						 //phase ist +90 grad
						 //pruef_variable = phase_L;
					 }
					 else
					 {
						 //phase ist 180 grad
						 //pruef_variable = phase_L/3*4;
					 }
				}
				else
				{
					if ((phase_L <= 6)&&(phase_L >=3))
					{
						//phase ist 0 grad
						 //pruef_variable = phase_L * 4;
					}
					else//
					{
						 //phase ist -90 grad
						 //pruef_variable = phase_L;
					}
				}
				if(pruef_variable > periode_mittelwert)
				{
					periode_mittelwert = periode_mittelwert-1; //if prüf variable bigger than periode, then decrease periode 
				}
				else if (pruef_variable < periode_mittelwert)
				{
					periode_mittelwert = periode_mittelwert + 1; //if prüf variable bigger than periode, then increase periode 
				}
			}

		}
		
		/*Phase Detection Buffer B*/ 	
		else if (uxBits & Process_Phase_detectionB)// if Bit phase detection is set -> read out peak array
		{
			for (i=1;i<peak_array_length;i++)
			{
				phase_H = (periode_mittelwert*(i+1) - position_array_H[i]);
				phase_L = (periode_mittelwert*(i+1) - position_array_L[i]);
				// Phase High detect
				if(phase_H > 13)
				{
					 if (phase_H >= 18)
					 {
						//phase ist -90 grad
						 pruef_variable = phase_H;
					 }
					 else
					 {
						//phase ist 0 grad
						 pruef_variable = phase_H/3*4;
					 }
				}
				else
				{
					if (phase_H <= 8)
					{
						//phase ist 180 grad
						 pruef_variable = phase_H * 4;
					}
					else//
					{
						//phase ist + 90 grad
						 pruef_variable = phase_H;
					}
				}
				// Phase Low detect
				if(phase_L >= 13)
				{
					 if (phase_L >= 18)
					 {
						 //phase ist +90 grad
						 //pruef_variable = phase_L;
					 }
					 else
					 {
						 //phase ist 180 grad
						 //pruef_variable = phase_L/3*4;
					 }
				}
				else
				{
					if ((phase_L <= 6)&&(phase_L >=3))
					{
						//phase ist 0 grad
						 //pruef_variable = phase_L * 4;
					}
					else//
					{
						 //phase ist -90 grad
						 //pruef_variable = phase_L;
					}
				}
				if(pruef_variable > periode_mittelwert)
				{
					periode_mittelwert = periode_mittelwert-1; //if prüf variable bigger than periode, then decrease periode 
				}
				else if (pruef_variable < periode_mittelwert)
				{
					periode_mittelwert = periode_mittelwert + 1; //if prüf variable bigger than periode, then increase periode 
				}
			}
		}
		
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}

void vRead_DMA(void *pvParameters)
{
	int count_array_position_H, count_array_position_L = 0;
	int i = 0;
	int flag_H;
	int flag_L;
	//int count_array_a = 0;
	EventBits_t uxBits;
	BaseType_t xResult;
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
			for(i=0;i<buffer_length;i++)
			{
				// HIGH PEAK A
				if(count_array_a >= peak_array_length) // check array a
				{
					count_array_a = 0;
					/* Set bit 0 and bit 4 in xEventGroup. */
					xResult = xEventGroupSetBits(
					xPhaseDetectionEventGroup,		/* The event group being updated. */
					Process_Phase_detectionB			/* The bits being set. */
					);
				}
				else
				{	
					if (buffer_a[i] > 127)
					{
						if (buffer_a[i] > high_peak)	//if buffer bigger than current high_peak
						{
							if (buffer_a[i] > 220)
							{
								high_peak = buffer_a[i];	// store new peak
								position_high_peak_a = i;	// store array position of new peak
								flag_H = 1;
								flag_L = 0;
							}
						}	
						else 
						{
							if (buffer_a[i] > 160 )
							{
								count_after_peak++;
							}
							else if(buffer_a[i] < 160)
							{
								if(flag_H == 1)
								{
									if (count_after_peak > 3)
									{
										peak_array_H[count_array_position_H] = high_peak;	//
										position_array_H[count_array_position_H] = position_high_peak_a;
										count_array_position_H++;
										count_array_a++; // Count up to activate event bit when 16 peaks got collected
										high_peak = 127;
										flag_H = 0;
										count_after_peak = 0;
									}
								}
							}
						}
					}
					// LOW PEAK	A
					else 
					{
						if (buffer_a[i] < low_peak)		//if buffer bigger than current high_peak
						{
							if (buffer_a[i] < 40)
							{
								low_peak = buffer_a[i];	// store new peak
								position_low_peak_a = i;	// store array position of new peak
								flag_L = 1;
								flag_H = 0;
							}
						}
						else
						{
							if (buffer_a[i] < 100)
							{
								count_after_peak++;
							}
							else if (buffer_a[i] > 100)
							{
								if (flag_L == 1)
								{
									if (count_after_peak > 4)
									{
										peak_array_L[count_array_position_L] = low_peak;
										position_array_L[count_array_position_L] = position_low_peak_a;
										count_array_position_L++;
										count_array_a++; // Count up to activate event bit when 16 peaks got collected
										low_peak = 127;
										flag_L = 0;
										count_after_peak = 0;
									}
								}
							}
						}	
					}	
				}
			}
		}
		else if (uxBits & Process_Signal_BufferB)
		{
			for (i=0;i<buffer_length;i++)
			{
				if(count_array_b >= peak_array_length) // check array b
				{
					count_array_b = 0;
					/* Set bit 0 and bit 4 in xEventGroup. */
					xResult = xEventGroupSetBits(
					xPhaseDetectionEventGroup,		/* The event group being updated. */
					Process_Phase_detectionA			/* The bits being set. */
					);
				}
				else
				{
					// HIGH PEAK B
					if (buffer_a[i] > 127)
					{
						if (buffer_a[i] > high_peak)	//if buffer bigger than current high_peak
						{
							if (buffer_a[i] > 220)
							{
								high_peak = buffer_a[i];	// store new peak
								position_high_peak_a = i;	// store array position of new peak
								flag_H = 1;
								flag_L = 0;
							}
						}
						else
						{
							if (buffer_a[i] > 160 )
							{
								count_after_peak++;
							}
							else if(buffer_a[i] < 160)
							{
								if(flag_H == 1)
								{
									if (count_after_peak > 3)
									{
										peak_array_H[count_array_position_H] = high_peak;	//
										position_array_H[count_array_position_H] = position_high_peak_a;
										count_array_position_H++;
										count_array_b++; // Count up to activate event bit when 16 peaks got collected
										high_peak = 127;
										flag_H = 0;
										count_after_peak = 0;
									}
								}
							}
						}
					}
					// LOW PEAK	B				
					else
					{
						if (buffer_a[i] < low_peak)		//if buffer bigger than current high_peak
						{
							if (buffer_a[i] < 40)
							{
								low_peak = buffer_a[i];	// store new peak
								position_low_peak_a = i;	// store array position of new peak
								flag_L = 1;
								flag_H = 0;
							}
						}
						else
						{
							if (buffer_a[i] < 100)
							{
								count_after_peak++;
							}
							else if (buffer_a[i] > 100)
							{
								if (flag_L == 1)
								{
									if (count_after_peak > 4)
									{
										peak_array_L[count_array_position_L] = low_peak;
										position_array_L[count_array_position_L] = position_low_peak_a;
										count_array_position_L++;
										count_array_b++; // Count up to activate event bit when 16 peaks got collected
										low_peak = 127;
										flag_L = 0;
										count_after_peak = 0;
									}
								}
							}
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
			//vDisplayWriteStringAtPos(3,0,"Hoi");
			vTaskStartScheduler();
			vTaskDelay(100 / portTICK_RATE_MS);
			
	}
}
