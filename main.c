/* @file
 * main.c for XBH Server on Tiva-C
 * (C) 2014, John Pham, George Mason University <jpham4@gmu.edu>
 *
 * Based on project.c from TI Tiva Firmware Development Package (2.1.0.12573)
 * Copyright (c) 2013-2014 Texas Instruments Incorporated.
 *
 * Also based on main.c from FreeRTOS Demo v7.6.0 (C) 2013
 */
//
//*****************************************************************************
//
// project.c - Simple project to use as a starting point for more complex
//             projects.
//
// Copyright (c) 2013-2014 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions
//   are met:
// 
//   Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// 
//   Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimer in the
//   documentation and/or other materials provided with the  
//   distribution.
// 
//   Neither the name of Texas Instruments Incorporated nor the names of
//   its contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
// This is part of revision 2.1.0.12573 of the Tiva Firmware Development Package.
//
//*****************************************************************************

//Std C lib
#include <stdbool.h>
#include <stdint.h>
#include <inc/hw_memmap.h>
#include <driverlib/debug.h>
#include <driverlib/gpio.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/sysctl.h>

// FreeRTOS includes
#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

// Hardware abstraction layer
#include "hal/hal.h"
#include "util.h"

/* Delay between cycles of the 'check' task. */
// Make it 30s
#define mainCHECK_DELAY						( ( portTickType ) 30000 / portTICK_RATE_MS )


/**
 * Clock rate in HZ
 */
uint32_t g_syshz;

/* From FreeRTOS demo main.
 * A 'check' task.  The check task only executes every 30 seconds but has a
 * high priority so is guaranteed to get processor time.  Its function is to
 * check that all the other tasks are still operational and that no errors have
 * been detected at any time.  If no errors have every been detected 'PASS' is
 * written to the display (via the print task) - if an error has ever been
 * detected the message is changed to 'FAIL'.  The position of the message is
 * changed for each write.
 */
//static void vCheckTask( void *pvParameters ) {/*{{{*/
//    portBASE_TYPE xErrorOccurred = pdFALSE;
//    portTickType xLastExecutionTime;
//    const portCHAR *pcPassMessage = "PASS";
//    const portCHAR *pcFailMessage = "FAIL";
//
//    /* Initialise xLastExecutionTime so the first call to vTaskDelayUntil()
//       works correctly. */
//    xLastExecutionTime = xTaskGetTickCount();
//
//    for( ;; )
//    {
//        /* Perform this check every mainCHECK_DELAY milliseconds. */
//        vTaskDelayUntil( &xLastExecutionTime, mainCHECK_DELAY );
//
//        /* Has an error been found in any task? */
//
//        if( xAreIntegerMathsTaskStillRunning() != pdTRUE )
//        {
//            xErrorOccurred = pdTRUE;
//        }
//
//        if( xArePollingQueuesStillRunning() != pdTRUE )
//        {
//            xErrorOccurred = pdTRUE;
//        }
//
//        if( xAreSemaphoreTasksStillRunning() != pdTRUE )
//        {
//            xErrorOccurred = pdTRUE;
//        }
//
//        if( xAreBlockingQueuesStillRunning() != pdTRUE )
//        {
//            xErrorOccurred = pdTRUE;
//        }
//
//        /* Send either a pass or fail message.  If an error is found it is
//           never cleared again.  We do not write directly to the LCD, but instead
//           queue a message for display by the print task. */
//        if( xErrorOccurred == pdTRUE )
//        {
//            xQueueSend( xPrintQueue, &pcFailMessage, portMAX_DELAY );
//        }
//        else
//        {
//            xQueueSend( xPrintQueue, &pcPassMessage, portMAX_DELAY );
//        }
//    }
//}/*}}}*/

void vApplicationStackOverflowHook(TaskHandle_t *pxTask, char *pcTaskName){
    while(1);
}


int main(void) {
    HAL_setup();

    DEBUG_OUT("Starting Scheduler\n");

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* Will only get here if there was insufficient heap to start the
	scheduler. */
    return 0;

}

    


