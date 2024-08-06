/*
 * Appl.c
 *
 *  Created on: Jul 4, 2024
 *      Author: akshay
 */

#include <stdio.h>
#include "ApplMain.h"
#include "FreeRTOS.h"
#include "task.h"

#include "main.h"

#include "stm32f4xx_hal_gpio.h"

#define DELAY		(10/portTICK_RATE_MS)
#define DELAY_1		(200/portTICK_RATE_MS)

#define configNO_DELAY		0

const char* led_names[] = {"LED1", "LED2"};

uint16_t LED_PINS[] = {LED1_Pin, LD2_Pin};
uint16_t LED_Delays[] = {800U , 1000U};
GPIO_TypeDef *LED_PORTS[] = {LED1_GPIO_Port, LD2_GPIO_Port};

// task handles:
TaskHandle_t SwapTaskHandle;



void sometask(void *params) {

	uint8_t taskno = (*((const uint8_t *)params));

	char msg[30] = {0};
	snprintf(msg, 30,"Toggling %s\n", led_names[taskno]);

	while (1) {

		HAL_GPIO_TogglePin(LED_PORTS[taskno], LED_PINS[taskno]);

		SEGGER_SYSVIEW_PrintfTarget(msg);

		vTaskDelay(pdMS_TO_TICKS(LED_Delays[taskno]));
	}
	//Never reach
	vTaskDelete(NULL);

}


void swap_priority(void* param) {

	while(1) {
		if(xTaskNotifyWait( 0, 0, NULL, pdMS_TO_TICKS(10000))) {
			portENTER_CRITICAL();

			TaskHandle_t currentTaskHandle = xTaskGetHandle(NULL);
			vTaskPrioritySet( currentTaskHandle, 4);

			TaskHandle_t T1TaskHandle = xTaskGetHandle("LED_task0");
			TaskHandle_t T2TaskHandle = xTaskGetHandle("LED_task1");

			BaseType_t p1 = uxTaskPriorityGet(T1TaskHandle);
			BaseType_t p2 = uxTaskPriorityGet(T2TaskHandle);

			vTaskPrioritySet( T1TaskHandle, p2 );
			vTaskPrioritySet( T2TaskHandle, p1 );

			vTaskPrioritySet( currentTaskHandle, 1 );
			portEXIT_CRITICAL();

		}
	}


}

void ApplMain() {
	  static uint8_t task_param[2] = { 0, 1};

	  const uint16_t TASK_STACK_SIZE =  (2U * 1024U/4) ;// 2kb

	  {
		  char msg[30] = {0};
		  snprintf(msg, 30,"Creating tasks...\n");
		  SEGGER_SYSVIEW_PrintfTarget(msg);
		  HAL_Delay(10);
	  }

	  for(uint8_t i = 0; i < 2; ++i) {
		  char task_name[12] = {0};
		  snprintf(task_name, sizeof(task_name),"LED_task%u", i);

		  BaseType_t xReturn =
			  xTaskCreate(
					  &sometask, // Task body
					  task_name, // task name
					  TASK_STACK_SIZE, // task size
					  (void *)&task_param[i], // task params
					  (i + 1) * 2, // task priority
					  NULL); // task handle

		  if (!xReturn) {
			printf("ApplMain: Failed to create task: %s\n", task_name);
			vTaskDelete(NULL);
		  }
	  }

	  BaseType_t xReturn =
		  xTaskCreate(
				  &swap_priority, // Task body
				  "SwapPrioTask", // task name
				  TASK_STACK_SIZE, // task size
				  NULL, // task params
				  1, // task priority
				  &SwapTaskHandle); // task handle

	  if (!xReturn) {
		printf("ApplMain: Failed to create task: %s\n", "SwapPrioTask");
		vTaskDelete(SwapTaskHandle);
	  }


	  vTaskDelete(NULL);

}

void handle_button_press() {
	char msg[40] = {0};

	snprintf(msg, sizeof(msg),"Swapping the priority....\n");
	SEGGER_SYSVIEW_PrintfTarget(msg);
	BaseType_t xHigherPriorityTaskWoken;
	xTaskNotifyFromISR(SwapTaskHandle, 0, eNoAction, &xHigherPriorityTaskWoken);

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	return;
}


void vApplicationIdleHook() {
	// We can call WFI : Wait for interrupt, i.e.,

	HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	return;
}
