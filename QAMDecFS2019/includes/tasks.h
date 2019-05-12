/*
 * tasks.h
 *
 * Created: 05.04.2019 09:05:34
 *  Author: Claudio Hediger
 */ 


#ifndef TASKS_H_
#define TASKS_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

void vTask_DMAHandler(void *pvParameters);
void vRead_DMA(void *pvParameters);

#define DMA_EVT_GRP_BufferA  ( 1 << 0 )
#define DMA_EVT_GRP_BufferB  ( 1 << 1 )
#define Process_Signal_BufferA ( 1 << 0 )
#define Process_Signal_BufferB ( 1 << 1 )

uint8_t count_buffer_a;
uint8_t count_buffer_b;
uint16_t count_array_a;
uint16_t count_array_b;
uint8_t buffer_a_array[250];
uint8_t buffer_b_array[250];

EventGroupHandle_t xDMAProcessEventGroup;
EventGroupHandle_t xSignalProcessEventGroup;


#endif /* TASKS_H_ */