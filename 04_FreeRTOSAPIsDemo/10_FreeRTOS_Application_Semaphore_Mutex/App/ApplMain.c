/*
 * Appl.c
 *
 *  Created on: Jul 4, 2024
 *      Author: akshay
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "main.h"

#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "ApplMain.h"
#include "ApplCommon.h"
#include "ApplUartService.h"
#include "ApplRTC.h"
#include "ApplLED.h"

current_task_t current_program = MENU_TASK;
SemaphoreHandle_t xMutexCurrTask;
void menu_task(void *param)
{

	NotifyMenuTask();

	while (1)
	{

		if (xTaskNotifyWait(0, 0, 0, portMAX_DELAY))
		{

			if (!isCurrentTask(MENU_TASK))
			{
				continue;
			}

			SEGGER_SYSVIEW_PrintfTarget(__func__);

			char MsgBanner[] = {
				"\n\r============="
				"\n\r==Main Menu=="
				"\n\r============="
				"\n\r1. Date and Time Menu"
				"\n\r2. Led menu"
				"\n\rEnter your choice:"};

			msg_send(MsgBanner);
			char RxMsg[128] = {0};

			if ((msg_recieve(&RxMsg[0]) - 2) > 1) // -2: '\r' + '\0'
			{
				NotifyMenuTask();
				PrintInvalidInput();
				continue;
			}

			uint8_t choice = 0;
			sscanf(RxMsg, "%hhu", &choice);

			switch (choice)
			{
			case 1: // RTC
				extern TaskHandle_t RTCService;
				 xSemaphoreTake(xMutexCurrTask, portMAX_DELAY);
				{
					current_program = RTC_TASK;
					xTaskNotify(RTCService, 0, eNoAction);
				}
				 xSemaphoreGive(xMutexCurrTask);
				break;
			case 2: // led  task
				extern TaskHandle_t LedService;
				xSemaphoreTake(xMutexCurrTask, portMAX_DELAY);
				{
					current_program = LED_TASK;
					xTaskNotify(LedService, 0, eNoAction);
				}
				xSemaphoreGive(xMutexCurrTask);
				break;
			default:
				PrintInvalidInput();
				NotifyMenuTask();
			}
		}
	}
}

TaskHandle_t menutask;

void ApplMain()
{

	SEGGER_SYSVIEW_PrintfTarget(__func__);

	ApplUartInit();

	ApplRTCInit();

	ApplLedInit();
	char MsgBanner[] = {"\n\r****************************"
						"\n\r*  Welcome to Application  *"
						"\n\r****************************"};
	msg_send(MsgBanner);
	BaseType_t xReturn =
		xTaskCreate(
			&menu_task,				 // Task body
			TASK_NAME_DEFAULT(MENU), // task name
			TASK_STACK_SIZE_DEFAULT, // task size
			NULL,					 // task params
			TASK_PRIO_DEFAULT,		 // task priority
			&menutask);				 // task handle

	configASSERT(xReturn == pdTRUE);

	xMutexCurrTask = xSemaphoreCreateMutex();

	{
		char msg[30] = {0};
		snprintf(msg, 30, "Starting Scheduler...\n");
		SEGGER_SYSVIEW_PrintfTarget(msg);
	}

	vTaskStartScheduler();
}

void vApplicationIdleHook()
{
	// We can call WFI : Wait for interrupt, i.e.,

	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	return;
}
