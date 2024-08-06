/*
 * ApplRTC.c
 *
 *  Created on: Jul 19, 2024
 *      Author: akshay
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <stdbool.h>

#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "ApplCommon.h"

extern RTC_HandleTypeDef hrtc;

bool isValidDate(uint32_t year, uint32_t month, uint32_t day)
{
	if (year > 99 || year < 1 || month < 1 || month > 12 || day < 1)
	{
		return false;
	}

	int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	return day <= daysInMonth[month - 1];
}

bool isValidTime(uint32_t hour, uint32_t minute, uint32_t second)
{
	return hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 60;
}

bool isValidDateTime(RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate)
{
	return isValidDate(sDate->Year, sDate->Month, sDate->Date) && isValidTime(sTime->Hours, sTime->Minutes, sTime->Seconds);
}

bool ReadAndValidateDateTime(RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate)
{

	/*ask user to enter date andtime */
	char MsgBanner[128] = {"\n\rEnter date (mm dd yy) and time (hh mm ss)"};

	msg_send(MsgBanner);

	char RxMsg[128] = {0};
	msg_recieve(&RxMsg[0]);

	sscanf(RxMsg, "%u%u%u%u%u%u",
		   &sDate->Month, &sDate->Date, &sDate->Year,
		   &sTime->Hours, &sTime->Minutes, &sTime->Seconds);

	if (!isValidDateTime(sTime, sDate))
	{
		memset(MsgBanner, 0, strlen(MsgBanner));
		snprintf(MsgBanner, sizeof(MsgBanner), "\n\r invalid date and time: %0.*s\n\r", 32, RxMsg);
		msg_send(MsgBanner);
		return false;
	}

	return true;
}

void RTCConfigure()
{

	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};

	while (!ReadAndValidateDateTime(&sTime, &sDate))
		;

	sTime.TimeFormat = RTC_HOURFORMAT12_AM;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	sDate.WeekDay = RTC_WEEKDAY_MONDAY;

	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}
}

void RTCDisplay()
{
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0};
	char MsgBanner[128] = {0};

	if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	snprintf(MsgBanner, sizeof(MsgBanner), "\n\rCurrent date (mm/dd/yy): %u/%u/%u"
										   "\n\rCurrent time (hh:mm:ss): %u:%u:%u\n\r",
			 sDate.Month, sDate.Date, sDate.Year,
			 sTime.Hours, sTime.Minutes, sTime.Seconds);

	msg_send(MsgBanner);
	return;
}

void ApplRTCService(void *param)
{

	while (1)
	{
		if (xTaskNotifyWait(0, 0, 0, portMAX_DELAY))
		{

			if (!isCurrentTask(RTC_TASK))
			{
				continue;
			}

			SEGGER_SYSVIEW_PrintfTarget(__func__);
			// do RTC stuff
			const char MsgBanner[] = {
				"\n\r============"
				"\n\r==RTC Menu=="
				"\n\r============"
				"\n\r1. Display Date and Time"
				"\n\r2. Configure Date and Time"
				"\n\rEnter your choice: "};
			msg_send(MsgBanner);

			char RxMsg[128] = {0};

			if ((msg_recieve(&RxMsg[0]) - 2) > 1) // -2: '\r' + '\0'
			{
				PrintInvalidInput();
				NotifyMenuTask();
				continue;
			}
			uint8_t choice = 0;
			sscanf(RxMsg, "%hhu", &choice);

			switch (choice)
			{
			case 1: // Display
				RTCDisplay();
				break;
			case 2: // configure
				RTCDisplay();
				RTCConfigure();
				RTCDisplay();
				break;
			default:
				PrintInvalidInput();
			}

			NotifyMenuTask();
		}
	}
}

TaskHandle_t RTCService;

void ApplRTCInit()
{
	SEGGER_SYSVIEW_PrintfTarget(__func__);
	BaseType_t xReturn =
		xTaskCreate(
			&ApplRTCService,
			TASK_NAME_DEFAULT(RTCService), // task name
			TASK_STACK_SIZE_DEFAULT,	   // task size
			NULL,						   // task params
			TASK_PRIO_DEFAULT,			   // task priority
			&RTCService);				   // task handle

	configASSERT(xReturn == pdTRUE);
}
