/*
 * ApplCommon.h
 *
 *  Created on: Jul 17, 2024
 *      Author: akshay
 */

#ifndef INC_APPLCOMMON_H_
#define INC_APPLCOMMON_H_
#include <stdbool.h>

#define TASK_STACK_SIZE_DEFAULT (2 * 1024 / 4U) // 2KBs

#define TASK_PRIO_DEFAULT (1U)

#define TASK_NAME_DEFAULT(x) ("Task_" #x)

#define QUEUE_ITEMS_DEFAULT (10U)

#define MAX_BUFFER_SIZE (128U)

#define DELAY_1MS (pdMS_TO_TICKS(1)) // 1ms

typedef struct
{
	uint8_t *data;
	size_t size;
} data_t;

typedef enum
{
	MENU_TASK,
	RTC_TASK,
	LED_TASK
} current_task_t;

extern bool isCurrentTask(current_task_t curr);
extern current_task_t current_program;
extern void PrintChosenInput(uint8_t input);
extern void PrintInvalidInput();
extern void msg_send(const char *msg);

extern size_t msg_recieve();

extern void NotifyMenuTask();
#endif /* INC_APPLCOMMON_H_ */
