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

// task handles:
TaskHandle_t button_task_handle, led_task_handle[3];
TaskHandle_t producer_handle, consumer_handle[2];


volatile TaskHandle_t* next_task_handle = &led_task_handle[0];

void sometask(void *params) {

	uint8_t taskno = (*((const uint8_t *)params));

	char msg[30] = {0};
	snprintf(msg, 30,"Toggling %s\n", led_names[taskno]);

	while (1) {

		HAL_GPIO_TogglePin(LED_PORTS[taskno], LED_PINS[taskno]);

		SEGGER_SYSVIEW_PrintfTarget(msg);

		if(xTaskNotifyWait( 0, 0, NULL, LED_Delays[taskno])) { //Notification received
			HAL_GPIO_TogglePin(LED_PORTS[taskno], LED_PINS[taskno]);
			snprintf(msg, 30,"Deleted task: %s\n", led_names[taskno]);
			SEGGER_SYSVIEW_PrintfTarget(msg);
			vTaskDelete(NULL);
		}
  }
	//Never reach
	vTaskDelete(NULL);

}

void button_task( void* pvParameters) {

	GPIO_PinState current_state = 1, previous_state = 1; // Button Not pressed
	uint8_t led_curr = 0;
	char msg[30] = {0};
	while(1) {

		current_state = HAL_GPIO_ReadPin( B1_GPIO_Port, B1_Pin);

		if(current_state == 0 && current_state!= previous_state) {

			xTaskNotify(*next_task_handle, 0, eNoAction);
			snprintf(msg, 30,"Button pressed....\n");
			SEGGER_SYSVIEW_PrintfTarget(msg);

			if(*next_task_handle == led_task_handle[2]) {
				snprintf(msg, 30,"Deleting button Task...\n");
				SEGGER_SYSVIEW_PrintfTarget(msg);

				//trigger consumer producer tasks

				xTaskNotify(producer_handle, 0, eNoAction);
				xTaskNotify(consumer_handle[0], 0, eNoAction);
				xTaskNotify(consumer_handle[1], 0, eNoAction);

				vTaskDelete(NULL);
			}

			next_task_handle = &led_task_handle[++led_curr];

		}
		previous_state = current_state;
		vTaskDelay(pdMS_TO_TICKS(10)); // 10ms
	}
}

static volatile uint32_t counter = 0;

void producer(void *pvParameters) {

	char msg[30];
	while(!xTaskNotifyWait(0, 0, NULL, 1000U));
	while(1) {
		vTaskDelay(50);
		vTaskSuspendAll();
		counter++; // context switch is disabled, (except interrupts). safe to increment counter
		snprintf(msg, sizeof(msg),"Task %s: count %ld\n", "producer", counter);
		SEGGER_SYSVIEW_PrintfTarget(msg);
		xTaskResumeAll();
		vTaskDelay(1000);
	}

}

void consumer(void *pvParameters) {
	uint8_t taskno = (*((const uint8_t *)pvParameters));
	char msg[30];
	while(!xTaskNotifyWait(0, 0, NULL, 1000U));
	while(1) {
		vTaskDelay(50);
		vTaskSuspendAll();
		snprintf(msg, sizeof(msg),"Task %u: count %ld\n", taskno, counter);
		SEGGER_SYSVIEW_PrintfTarget(msg);
		for(int i = 0; i < ((taskno+1) * 10000); ++i);
		xTaskResumeAll();
		vTaskDelay(((taskno+1) * 1000));
	}

}


void ApplMain() {
	  static uint8_t task_param[3] = { 0, 1, 2};

	  const uint16_t TASK_STACK_SIZE =  (2U * 1024U/4) ;// 2kb

	  {
		  char msg[30] = {0};
		  snprintf(msg, 30,"Creating tasks...\n");
		  SEGGER_SYSVIEW_PrintfTarget(msg);
		  HAL_Delay(10);
	  }

	  for(uint8_t i = 0; i < 3; ++i) {
		  char task_name[12] = {0};
		  snprintf(task_name, sizeof(task_name),"LED_task_%u", i);

		  BaseType_t xReturn =
			  xTaskCreate(&sometask, task_name, TASK_STACK_SIZE,
						  (void *)&task_param[i], 1, &led_task_handle[i]);

		  if (!xReturn) {
			printf("ApplMain: Failed to create task: %s\n", task_name);
			vTaskDelete(led_task_handle[i]);
		  }
	  }

	  // Button Task:
	  BaseType_t xReturn =
	      xTaskCreate(&button_task, "Button_task", TASK_STACK_SIZE,
	                  NULL, 1, &button_task_handle);

	  if (!xReturn) {
		  printf("ApplMain: Failed to create task: Button_task\n");
	      vTaskDelete(button_task_handle);
	  }


	  /* p[roducer consumer tasks */
	  xReturn =
	      xTaskCreate(&consumer, "consumer_task1", TASK_STACK_SIZE,
	    		  (void *)&task_param[0], 1, &consumer_handle[0]);

	  if (!xReturn) {
		  printf("ApplMain: Failed to create task: consumer1\n");
	      vTaskDelete(consumer_handle[0]);
	  }
	  xReturn =
	      xTaskCreate(&consumer, "consumer_task2", TASK_STACK_SIZE,
	    		  (void *)&task_param[1], 1, &consumer_handle[1]);

	  if (!xReturn) {
		  printf("ApplMain: Failed to create task: consumer2\n");
	      vTaskDelete(consumer_handle[1]);
	  }
	  xReturn =
	      xTaskCreate(&producer, "producer", TASK_STACK_SIZE,
	                  NULL, 1, &producer_handle);

	  if (!xReturn) {
		  printf("ApplMain: Failed to create task: producer\n");
	      vTaskDelete(producer_handle);
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
