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

#include "dma_config.h"				
#include "init.h"
#include "utils.h"
#include "errorHandler.h"
#include "NHD0420Driver.h"

#include "double_buffer_read_out.h"
#include "read_peaks.h"		  
#include "phase_detection.h"	

extern void vApplicationIdleHook( void );
void vLedBlink(void *pvParameters);
//void vRead_DMA(void *pvParameters);
void vWrite_Display(void *pvParameters);

TaskHandle_t ledTask;
TaskHandle_t my_read_Peaks;
TaskHandle_t my_phase_detection;
TaskHandle_t my_Display;
TaskHandle_t TaskDMAHandler;


void vApplicationIdleHook( void )
{	
	
}

int main(void)
{
    resetReason_t reason = getResetReason();

	vInitClock();
	vInitDisplay();
	
	xTaskCreate( vLedBlink, (const char *) "ledBlink", configMINIMAL_STACK_SIZE+10, NULL, 1, &ledTask);
	xTaskCreate( vRead_Peaks, (const char *) "read_Peaks", configMINIMAL_STACK_SIZE+100, NULL, 1, &my_read_Peaks);
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
	PORTE.DIRSET = PIN3_bm; /*LED1*/
	PORTE.OUT = 0x08;
	for(;;) {
		PORTE.OUTTGL = 0x08;				
		vTaskDelay(100 / portTICK_RATE_MS);
	}
}


void vWrite_Display(void *pvParameters){

	for (;;)
	{
		
			vDisplayClear();
			vDisplayWriteStringAtPos(0,0,"FreeRTOS 10.0.1");
			vDisplayWriteStringAtPos(1,0,"a: %d b: %d",buffer_a[1],buffer_b[1]);
			vDisplayWriteStringAtPos(2,0,"H: %d L: %d ",high_peak, low_peak);
			//vDisplayWriteStringAtPos(3,0,"Control: %lu ",control_result );
			//vDisplayWriteStringAtPos(3,0,"Hoi");
			vTaskStartScheduler();
			vTaskDelay(100 / portTICK_RATE_MS);
			
	}
}
