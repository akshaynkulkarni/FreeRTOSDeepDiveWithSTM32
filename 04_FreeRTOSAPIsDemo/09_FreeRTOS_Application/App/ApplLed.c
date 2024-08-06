/*
 * ApplLed.c
 *
 *  Created on: Jul 19, 2024
 *      Author: akshay
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "ApplCommon.h"

#define LED_PINS_MAX (4U)

uint16_t LEDPins[] = {LED1_Pin, LED2_Pin, LED3_Pin, LED4_Pin};

GPIO_TypeDef *LEDPorts[] = {
	LED1_GPIO_Port,
	LED2_GPIO_Port,
	LED3_GPIO_Port,
	LED4_GPIO_Port};

typedef void (*timer_cbk_t)(TimerHandle_t xTimer);

void led_pattern1_cbk(TimerHandle_t xTimer);
void led_pattern2_cbk(TimerHandle_t xTimer);
void led_pattern3_cbk(TimerHandle_t xTimer);
void led_pattern4_cbk(TimerHandle_t xTimer);

timer_cbk_t timer_cbk[] = {
	&led_pattern1_cbk,
	&led_pattern2_cbk,
	&led_pattern3_cbk,
	&led_pattern4_cbk};

TimerHandle_t xTimers[LED_PINS_MAX];
TimerHandle_t *current_timer = NULL;
TaskHandle_t LedService;

void ApplLedService(void *param)
{

	while (1)
	{

		if (xTaskNotifyWait(0, 0, 0, portMAX_DELAY))
		{

			if (!isCurrentTask(LED_TASK))
			{
				continue;
			}

			SEGGER_SYSVIEW_PrintfTarget(__func__);
			// do led stuff
			const char MsgBanner[] = {
				"\n\r============"
				"\n\r==LED Menu=="
				"\n\r============"
				"\n\r0. Stop Effect"
				"\n\r1. Effect 1"
				"\n\r2. Effect 2"
				"\n\r3. Effect 3"
				"\n\r4. Effect 4"
				"\n\rChoose LED effect:"};

			msg_send(MsgBanner);

			char RxMsg[128] = {0};

			if ((msg_recieve(&RxMsg[0]) - 2) > 1) // -2: '\r' + '\0'
			{
				PrintInvalidInput(RxMsg);
				xTaskNotify(LedService, 0, eNoAction);
				continue;
			}
			uint8_t choice = 0;
			sscanf(RxMsg, "%hhu", &choice);

			switch (choice)
			{
			case 1:
			case 2:
			case 3:
			case 4:
				if (current_timer)
					xTimerStop(*current_timer, portMAX_DELAY);
				current_timer = &xTimers[choice - 1];
				xTimerStart(xTimers[choice - 1], portMAX_DELAY);
				break;
			case 0:
			default:
				if (current_timer)
					xTimerStop(*current_timer, portMAX_DELAY);
				current_timer = NULL;
				for (uint8_t i = 0; i < LED_PINS_MAX; i++)
				{
					HAL_GPIO_WritePin(LEDPorts[i], LEDPins[i], GPIO_PIN_RESET);
				}
				if (choice)
				{
					PrintInvalidInput();
				}
			}
			NotifyMenuTask();
		}
	}
}

void ApplLedInit()
{

	char timer_name[10] = {0};
	SEGGER_SYSVIEW_PrintfTarget(__func__);
	// init pins
	for (uint8_t i = 0; i < LED_PINS_MAX; i++)
	{

		snprintf(timer_name, 10, "timer_%u", i);
		HAL_GPIO_WritePin(LEDPorts[i], LEDPins[i], GPIO_PIN_RESET);

		xTimers[i] = xTimerCreate(/* Just a text name, not used by the RTOS
								  kernel. */
								  timer_name,
								  /* The timer period in ticks, must be
								  greater than 0. */
								  pdMS_TO_TICKS(500),
								  /* The timers will auto-reload themselves
								  when they expire. */
								  pdTRUE,
								  /* The ID is used to store a count of the
								  number of times the timer has expired, which
								  is initialised to 0. */
								  (void *)0,
								  /* Each timer calls the same callback when
								  it expires. */
								  timer_cbk[i]);
	}

	BaseType_t xReturn =
		xTaskCreate(
			&ApplLedService,
			TASK_NAME_DEFAULT(LedService), // task name
			TASK_STACK_SIZE_DEFAULT,	   // task size
			NULL,						   // task params
			TASK_PRIO_DEFAULT,			   // task priority
			&LedService);				   // task handle

	configASSERT(xReturn == pdTRUE);
}

void led_pattern1_cbk(TimerHandle_t xTimer)
{
	static uint8_t current_led = 0;

	for (uint8_t i = 0; i < 4; i++)
	{

		if (current_led == i)
		{
			HAL_GPIO_WritePin(LEDPorts[i], LEDPins[i], GPIO_PIN_SET);
		}
		else
		{
			HAL_GPIO_WritePin(LEDPorts[i], LEDPins[i], GPIO_PIN_RESET);
		}
	}
	current_led = (current_led + 1) % LED_PINS_MAX;
}

void led_pattern2_cbk(TimerHandle_t xTimer)
{
	static uint8_t current_led = 0;

	if (current_led)
	{
		HAL_GPIO_WritePin(LEDPorts[0], LEDPins[0], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LEDPorts[1], LEDPins[1], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LEDPorts[2], LEDPins[2], GPIO_PIN_SET);
		HAL_GPIO_WritePin(LEDPorts[3], LEDPins[3], GPIO_PIN_SET);
	}
	else
	{
		HAL_GPIO_WritePin(LEDPorts[3], LEDPins[3], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LEDPorts[2], LEDPins[2], GPIO_PIN_RESET);
		HAL_GPIO_WritePin(LEDPorts[1], LEDPins[1], GPIO_PIN_SET);
		HAL_GPIO_WritePin(LEDPorts[0], LEDPins[0], GPIO_PIN_SET);
	}
	current_led = !current_led;
}

void led_pattern3_cbk(TimerHandle_t xTimer)
{
	static uint8_t current_led = 0, next_led = 1;

	for (uint8_t i = 0; i < 4; i++)
	{

		if (current_led == i)
		{
			HAL_GPIO_WritePin(LEDPorts[current_led], LEDPins[current_led], GPIO_PIN_SET);
			HAL_GPIO_WritePin(LEDPorts[next_led], LEDPins[next_led], GPIO_PIN_SET);
		}
		else if (next_led != i)
		{
			HAL_GPIO_WritePin(LEDPorts[i], LEDPins[i], GPIO_PIN_RESET);
		}
	}
	current_led = (current_led + 1) % LED_PINS_MAX;
	next_led = (next_led + 1) % LED_PINS_MAX;
}

void led_pattern4_cbk(TimerHandle_t xTimer)
{
	static uint8_t current_led = 0;

	for (uint8_t i = 0; i < 4; i++)
	{

		if (current_led == i)
		{
			HAL_GPIO_WritePin(LEDPorts[i], LEDPins[i], GPIO_PIN_RESET);
		}
		else
		{
			HAL_GPIO_WritePin(LEDPorts[i], LEDPins[i], GPIO_PIN_SET);
		}
	}
	current_led = (current_led + 1) % LED_PINS_MAX;
}
