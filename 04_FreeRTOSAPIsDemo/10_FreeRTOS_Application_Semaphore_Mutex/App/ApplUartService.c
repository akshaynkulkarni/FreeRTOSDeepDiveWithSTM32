/*
 * ApplUartService.c
 *
 *  Created on: Jul 17, 2024
 *      Author: akshay
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "ApplCommon.h"

#include "ApplUartService.h"

static TaskHandle_t ScanService, PrintService;
QueueHandle_t UartScanQueue, UartPrintQueue;

typedef data_t RxData_t;
typedef uint32_t TxData_t;

extern current_task_t current_program;

SemaphoreHandle_t xSemUartScan;

typedef struct {
	uint8_t Buff[MAX_BUFFER_SIZE];
	uint8_t size;
}UartScanData_t;

#define MAX_BUFFERS	(4U)

static volatile UartScanData_t RxDataBuff[MAX_BUFFERS] = {0}; // circular buffer

static volatile uint8_t consumer_current_buffer = 0;

// Uart Rx call back function
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	static uint8_t producer_buffer_index = 0;
	//extern uint8_t consumer_current_buffer;

	if (huart == &UART_INSTANCE)
	{
		static uint8_t count = 0;

		if((producer_buffer_index + 1) % MAX_BUFFERS == consumer_current_buffer) {
			// buffer is full, cannot process further
			HAL_UART_Receive_IT(&UART_INSTANCE, (uint8_t *)&RxDataBuff[producer_buffer_index].Buff[count], 1);
			return;

		}

		BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
		memcpy(huart->pRxBuffPtr, (void *)&RxDataBuff[producer_buffer_index].Buff[count], 1);

		HAL_UART_Transmit(&UART_INSTANCE, (void *)&RxDataBuff[producer_buffer_index].Buff[count], 1, portMAX_DELAY);

		if (count >= MAX_BUFFER_SIZE - 2 ||
				RxDataBuff[producer_buffer_index].Buff[count] == '\r' ||
				RxDataBuff[producer_buffer_index].Buff[count] == '\n')
		{ // 2 because, to add null char

			RxDataBuff[producer_buffer_index].Buff[++count] = 0; // add null to the end in place of /n or /r or full
			RxDataBuff[producer_buffer_index].size = count + 1;

			producer_buffer_index = (producer_buffer_index + 1) % MAX_BUFFERS;
			count = 0; // reset count

			xSemaphoreGiveFromISR(xSemUartScan, &pxHigherPriorityTaskWoken);

			portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
		}
		else
		{
			count++;
		}
		HAL_UART_Receive_IT(&UART_INSTANCE, (uint8_t *)&RxDataBuff[producer_buffer_index].Buff[count], 1);
	}
}

void ApplUartScanService(void *pvdata)
{

	uint32_t count = 0;

	while ((xSemaphoreTake(xSemUartScan, portMAX_DELAY)))
	{
			SEGGER_SYSVIEW_PrintfTarget(__func__);

			count = RxDataBuff[consumer_current_buffer].size;

			uint8_t *ptr = pvPortMalloc(count);
			memcpy((uint8_t *)ptr, (uint8_t *)& RxDataBuff[consumer_current_buffer].Buff[0], count);

			RxData_t RxD = {
				.data = ptr,
				.size = count};

			xQueueSend(UartScanQueue, &RxD, portMAX_DELAY);
			consumer_current_buffer = (consumer_current_buffer + 1) % MAX_BUFFERS;
	}
}

void ApplUartPrintService(void *pvdata)
{

	TxData_t *TxBuffData;

	while (1)
	{
		if (xQueueReceive(UartPrintQueue, &TxBuffData, portMAX_DELAY))
		{
			SEGGER_SYSVIEW_PrintfTarget(__func__);
			configASSERT(HAL_OK == HAL_UART_Transmit(&UART_INSTANCE, (uint8_t *)TxBuffData, strlen((char *)TxBuffData), portMAX_DELAY));
			vPortFree(TxBuffData);
		}
	}
}

void ApplUartInit()
{
	// UART Rx queue
	UartScanQueue = xQueueCreate(QUEUE_ITEMS_DEFAULT, sizeof(RxData_t));
	configASSERT(UartScanQueue != NULL);
	// UART Tx queue
	UartPrintQueue = xQueueCreate(QUEUE_ITEMS_DEFAULT, sizeof(TxData_t));
	configASSERT(UartPrintQueue != NULL);

	xSemUartScan = xSemaphoreCreateCounting(MAX_BUFFERS, 0);

	SEGGER_SYSVIEW_PrintfTarget(__func__);
	configASSERT(HAL_OK == HAL_UART_Receive_IT(&UART_INSTANCE, (uint8_t *)&RxDataBuff[0].Buff[0], 1));

	BaseType_t xReturn =
		xTaskCreate(
			&ApplUartScanService,
			TASK_NAME_DEFAULT(UartScanService), // task name
			TASK_STACK_SIZE_DEFAULT,			// task size
			NULL,								// task params
			TASK_PRIO_DEFAULT,					// task priority
			&ScanService);						// task handle

	configASSERT(xReturn == pdTRUE);

	xReturn =
		xTaskCreate(
			&ApplUartPrintService,
			TASK_NAME_DEFAULT(UartPrintService), // task name
			TASK_STACK_SIZE_DEFAULT,			 // task size
			NULL,								 // task params
			TASK_PRIO_DEFAULT,					 // task priority
			&PrintService);						 // task handle
	configASSERT(xReturn == pdTRUE);
}
