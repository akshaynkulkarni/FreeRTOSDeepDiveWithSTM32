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

const char* led_names[] = {"LED1", "LED2", "LED3"};


uint16_t LED_PINS[] = {LED1_Pin , LED3_Pin, LD2_Pin};
uint16_t LED_Delays[] = {400U, 800U , 1000U};
GPIO_TypeDef *LED_PORTS[] = {LED1_GPIO_Port,LED3_GPIO_Port, LD2_GPIO_Port};

void sometask(void *params) {

	uint8_t taskno = (*((const uint8_t *)params));

	char msg[30] = {0};
	snprintf(msg, 30,"Toggling %s", led_names[taskno]);

	while (1) {

		HAL_GPIO_TogglePin(LED_PORTS[taskno], LED_PINS[taskno]);

		SEGGER_SYSVIEW_PrintfTarget(msg);

	#if (configUSE_PREEMPTION == 0) // cooperative sched policy
    	taskYEILD();
	#elif (configNO_DELAY == 0)
    	vTaskDelay(LED_Delays[taskno]/ portTICK_RATE_MS);
    	//HAL_Delay(LED_Delays[taskno]);
    #endif
  }
	//Never reach
	vTaskDelete(NULL);

}

void ApplMain() {
	  static uint8_t task0_param = 0;
	  static uint8_t task1_param = 1;
	  static uint8_t task2_param = 2;
	  const uint16_t TASK_STACK_SIZE =  (2U * 1024U/4) ;// 2kb
	  TaskHandle_t task2, task1, task0;
	  {
		  char msg[30] = {0};
		  snprintf(msg, 30,"Creating tasks...\n");
		  SEGGER_SYSVIEW_PrintfTarget(msg);
		  HAL_Delay(10);
	  }

	  BaseType_t xReturn0 =
	      xTaskCreate(&sometask, "LED_task_0", TASK_STACK_SIZE,
	                  (void *)&task0_param, 1, &task0);

	  if (xReturn0 != pdPASS) {
	    printf("ApplMain: Failed to create task: LED_task_0\n");
	    vTaskDelete(task0);
	  }

	  BaseType_t xReturn1 =
	      xTaskCreate(&sometask, "LED_task_1", TASK_STACK_SIZE,
	                  (void *)&task1_param, 1, &task1);

	  if (xReturn1 != pdPASS) {
		  printf("ApplMain: Failed to create task: LED_task_1\n");
	    vTaskDelete(task1);
	  }

	  BaseType_t xReturn2 =
	      xTaskCreate(&sometask, "LED_task_2", TASK_STACK_SIZE,
	                  (void *)&task2_param, 1, &task2);

	  if (xReturn2 != pdPASS) {
		  printf("ApplMain: Failed to create task: LED_task_2\n");
	    vTaskDelete(task2);
	  }
	  {
		  char msg[30] = {0};
		  snprintf(msg, 30,"Exiting AppMain()...\n");
		  SEGGER_SYSVIEW_PrintfTarget(msg);
	  }
	  vTaskDelete(NULL);

}

void vApplicationIdleHook() {
	return;
}
