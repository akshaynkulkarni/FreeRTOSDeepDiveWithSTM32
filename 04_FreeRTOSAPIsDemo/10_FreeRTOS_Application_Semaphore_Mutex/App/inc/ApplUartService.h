/*
 * ApplUartService.h
 *
 *  Created on: Jul 17, 2024
 *      Author: akshay
 */

#ifndef INC_APPLUARTSERVICE_H_
#define INC_APPLUARTSERVICE_H_

extern QueueHandle_t UartScanQueue, UartPrintQueue;

#define UART_INSTANCE huart6

extern UART_HandleTypeDef UART_INSTANCE;

void ApplUartInit();

#endif /* INC_APPLUARTSERVICE_H_ */
