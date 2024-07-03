/*
 * Appl.c
 *
 *  Created on: Jul 4, 2024
 *      Author: akshay
 */

#include <ApplMain.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"

#define DELAY		(100/portTICK_RATE_MS)
#define DELAY_1		(200/portTICK_RATE_MS)

#define configNO_DELAY		0

void sometask(void *params) {

	uint32_t taskno = (*((const uint8_t *)params));
	while (1) {
    	printf("Task %u: Hello!\n", taskno);

	#if (configUSE_PREEMPTION == 0) // cooperative sched policy
    	taskYIELD();
	#elif (configNO_DELAY == 0)
    	vTaskDelay(DELAY);
    	#endif
    	}
	vTaskDelete(NULL);
}


void ApplMain() {
	  static uint8_t task0_param = 0;
	  static uint8_t task1_param = 1;
	  const uint16_t TASK_STACK_SIZE =  (2U * 1024U/4) ;// 2kb, StackSize for xTaskCreate is in Words, not bytes
	  TaskHandle_t task0, task1;

	  BaseType_t xReturn0 =
	      xTaskCreate(&sometask, "sometask_0", TASK_STACK_SIZE,
	                  (void *)&task0_param, 1, &task0);

	  if (xReturn0 != pdPASS) {
	    printf("ApplMain: Failed to create task: sometask_0\n");
	    vTaskDelete(task0);
	  }

	  BaseType_t xReturn1 =
	      xTaskCreate(&sometask, "sometask_1", TASK_STACK_SIZE,
	                  (void *)&task1_param, 1, &task1);

	  if (xReturn1 != pdPASS) {
		  printf("ApplMain: Failed to create task: sometask_1\n");
	    vTaskDelete(task1);
	  }

	  vTaskStartScheduler();

	for(;;);
	//never return

}

void vApplicationIdleHook() {
	return;
}
