/*
 * ApplCommon.c
 *
 *  Created on: Jul 30, 2024
 *      Author: akshay
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "ApplCommon.h"

void msg_send(const char *msg)
{
	extern QueueHandle_t UartPrintQueue;

	size_t len = strlen(msg) + 1;
	char *pMsg = pvPortMalloc(len);

	memcpy(pMsg, msg, len);
	SEGGER_SYSVIEW_PrintfTarget(__func__);
	xQueueSend(UartPrintQueue, (uint32_t *)&pMsg, portMAX_DELAY);
}

size_t msg_recieve(char *msg)
{
	extern QueueHandle_t UartScanQueue;
	{
		data_t RxMsg;

		xQueueReceive(UartScanQueue, &RxMsg, portMAX_DELAY);

		if (!msg)
		{
			return 0;
		}
		memcpy(msg, RxMsg.data, RxMsg.size);

		vPortFree(RxMsg.data);

		SEGGER_SYSVIEW_PrintfTarget(__func__);
		return RxMsg.size;
	}
}

#define INVALID_MSG "\n\rInvalid choice!!"
void PrintInvalidInput()
{
	char invalidmsg[128] = {0};
	sprintf(invalidmsg, "\n\r%s\n\r", INVALID_MSG);
	msg_send(invalidmsg);
	SEGGER_SYSVIEW_PrintfTarget(invalidmsg);
}

void NotifyMenuTask()
{
	taskENTER_CRITICAL();
	{
		extern current_task_t current_program;
		//extern TaskHandle_t menutask;
		extern SemaphoreHandle_t xSemaMenuTask;
		current_program = MENU_TASK;
		//xTaskNotify(menutask, 0, eNoAction);
		xSemaphoreGive(xSemaMenuTask);
	}
	taskEXIT_CRITICAL();

}

bool isCurrentTask(current_task_t curr)
{
	bool val = false;
	taskENTER_CRITICAL();
	{
		extern current_task_t current_program;
		val = (current_program == curr);
	}
	taskEXIT_CRITICAL();
	return val;
}
