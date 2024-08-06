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

#include "ApplCommon.h"

#include "ApplUartService.h"

static TaskHandle_t ScanService, PrintService;
QueueHandle_t UartScanQueue, UartPrintQueue;

typedef data_t RxData_t;
typedef uint32_t TxData_t;

// Uart Rx call back function

static volatile uint8_t RxBuff[MAX_BUFFER_SIZE] = {0};

extern current_task_t current_program;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

	if (huart == &UART_INSTANCE)
	{
		static uint8_t count = 0;

		BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
		memcpy(huart->pRxBuffPtr, (void *)&RxBuff[count], 1);

		HAL_UART_Transmit(&UART_INSTANCE, (void *)&RxBuff[count], 1, portMAX_DELAY);

		if (count >= MAX_BUFFER_SIZE - 2 || RxBuff[count] == '\r' || RxBuff[count] == '\n')
		{ // 2 because, to add null char

			RxBuff[++count] = 0; // add null to the end in place of /n or /r or full

			xTaskNotifyFromISR(ScanService, count + 1, eSetValueWithOverwrite, &pxHigherPriorityTaskWoken);

			count = 0; // reset count
			portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
		}
		else
		{
			count++;
		}
		HAL_UART_Receive_IT(&UART_INSTANCE, (uint8_t *)&RxBuff[count], 1);
	}
}

void ApplUartScanService(void *pvdata)
{

	uint32_t count = 0;
	while (1)
	{

		if (xTaskNotifyWait(0, 0, &count, portMAX_DELAY))
		{
			SEGGER_SYSVIEW_PrintfTarget(__func__);

			uint8_t *ptr = pvPortMalloc(count);
			memcpy((uint8_t *)ptr, (uint8_t *)&RxBuff[0], count);
			RxData_t RxD = {
				.data = ptr,
				.size = count};
			xQueueSend(UartScanQueue, &RxD, portMAX_DELAY);
		}
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
	SEGGER_SYSVIEW_PrintfTarget(__func__);
	configASSERT(HAL_OK == HAL_UART_Receive_IT(&UART_INSTANCE, (uint8_t *)&RxBuff[0], 1));

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
