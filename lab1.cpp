/**
 * ECE 315 Lab 1
 *
 * Copyright 2019 Nathan Klapstein, Thomas Lorincz
 */

#include <predef.h>
#include <stdio.h>
#include <ctype.h>
#include <startnet.h>
#include <autoupdate.h>
#include <smarttrap.h>
#include <taskmon.h>
#include <NetworkDebug.h>
#include <pinconstant.h>
#include <pins.h>
#include <basictypes.h>
#include "LCD.h"
#include "bitmaps.h"
#include "error_wrapper.h"
#include "point.h"


extern "C" {
void UserMain(void *pd);
void StartTask1(void);
void Task1Main(void *pd);
void StartTask2(void);
void Task2Main(void *pd);
void StartTask3(void);
void Task3Main(void *pd);
}


/* Task stacks for all the user tasks */
/* If you add a new task you'll need to add a new stack for that task */
DWORD Task1Stk[USER_TASK_STK_SIZE] __attribute__(( aligned( 4 )));
DWORD Task2Stk[USER_TASK_STK_SIZE] __attribute__(( aligned( 4 )));
DWORD Task3Stk[USER_TASK_STK_SIZE] __attribute__(( aligned( 4 )));


const char *AppName = "Nathan Klapstein, Thomas Lorincz";


/* User task priorities always based on MAIN_PRIO */
/* The priorities between MAIN_PRIO and the IDLE_PRIO are available */
#define TASK1_PRIO    MAIN_PRIO + 1
#define TASK2_PRIO    MAIN_PRIO + 2
#define TASK3_PRIO    MAIN_PRIO + 3

#define WAIT_FOREVER 0
#define ONE_SECOND TICKS_PER_SECOND-19 // approximately 1 second

LCD myLCD;
OS_SEM sem1, sem2, sem3, sem4;

/**
 * Main entry point for Lab 1
 *
 * @param pd
 */
void UserMain(void *pd) {
    BYTE err = OS_NO_ERR;
    char *welcome = "Welcome";

    InitializeStack();
    OSChangePrio(MAIN_PRIO);
    EnableAutoUpdate();
    StartHTTP();
    EnableTaskMonitor();

#ifndef _DEBUG
    EnableSmartTraps();
#endif

#ifdef _DEBUG
    InitializeNetworkGDB_and_Wait();
#endif

    iprintf("Application started: %s\n", AppName);

    OSSemInit(&sem1, 1);
    OSSemInit(&sem2, 0);
    OSSemInit(&sem3, 0);
    OSSemInit(&sem4, 0);

    myLCD.Init();
    myLCD.Clear();

    StartTask1();
    StartTask2();
    StartTask3();

    while (1) {
        OSTimeDly(TICKS_PER_SECOND);
    }
}

/**
 * Draw a $ sprite at the current position on the LCD.
 */
void drawDollar() {
    const BYTE dollar[7] = {0x00, 0x24, 0x2a, 0x7f, 0x2a, 0x12, 0x00};
    myLCD.DrawChar(dollar);
}

/**
 * Task starter for Task1
 */
void StartTask1(void) {
    BYTE err = OS_NO_ERR;

    err = display_error("StartTask1 fail:",
                        OSTaskCreatewName(Task1Main,
                                          (void *) NULL,
                                          (void *) &Task1Stk[USER_TASK_STK_SIZE],
                                          (void *) &Task1Stk[0],
                                          TASK1_PRIO, "Task 1"));
}

/**
 * Task1 definition
 *
 * Prints the sprite on the upper third of the screen.
 *
 * @param pd
 */
void Task1Main(void *pd) {
    while (1) {
        OSSemPend(&sem1, 0);
        myLCD.Clear();
        for (int i = LINE1_END; i >= LINE1_ORIGIN; i--) {
            myLCD.Move(char_index[i]);
            drawDollar();
            OSTimeDly(TICKS_PER_SECOND);
        }
        myLCD.Move(char_index[LINE2_ORIGIN]);
        drawDollar();
        OSTimeDly(TICKS_PER_SECOND);
        OSSemPost(&sem2);

        OSSemPend(&sem1, 0);
        myLCD.Move(char_index[LINE2_END]);
        drawDollar();
        OSTimeDly(TICKS_PER_SECOND);
        OSSemPost(&sem1);
    }
}

/**
 * Task starter for Task2
 */
void StartTask2(void) {
    BYTE err = OS_NO_ERR;

    err = display_error("StartTask2 fail:",
                        OSTaskCreatewName(Task2Main,
                                          (void *) NULL,
                                          (void *) &Task2Stk[USER_TASK_STK_SIZE],
                                          (void *) &Task2Stk[0],
                                          TASK2_PRIO, "Task 2"));
}

/**
 * Task2 definition
 *
 * Prints the sprite on the left and right middle third of the screen.
 *
 * @param pd
 */
void Task2Main(void *pd) {
    while (1) {
        OSSemPend(&sem2, 0);
        myLCD.Move(char_index[LINE3_ORIGIN]);
        drawDollar();
        OSTimeDly(TICKS_PER_SECOND);
        myLCD.Move(char_index[LINE4_ORIGIN]);
        drawDollar();
        OSTimeDly(TICKS_PER_SECOND);
        OSSemPost(&sem3);

        OSSemPend(&sem4, 0);
        myLCD.Move(char_index[LINE4_END]);
        drawDollar();
        OSTimeDly(TICKS_PER_SECOND);
        myLCD.Move(char_index[LINE3_END]);
        drawDollar();
        OSTimeDly(TICKS_PER_SECOND);
        OSSemPost(&sem1);
    }
}

/**
 * Task starter for Task3
 */
void StartTask3(void) {
    BYTE err = OS_NO_ERR;

    err = display_error("StartTask3 fail:",
                        OSTaskCreatewName(Task3Main,
                                          (void *) NULL,
                                          (void *) &Task3Stk[USER_TASK_STK_SIZE],
                                          (void *) &Task3Stk[0],
                                          TASK3_PRIO, "Task 3"));
}

/**
 * Task3 definition
 *
 * Prints the sprite on the lower third of the screen.
 *
 * @param pd
 */
void Task3Main(void *pd) {
    while (1) {
        OSSemPend(&sem3, 0);
        myLCD.Move(char_index[LINE5_ORIGIN]);
        drawDollar();
        OSTimeDly(TICKS_PER_SECOND);
        for (int i = LINE6_ORIGIN; i <= LINE6_END; i++) {
            myLCD.Move(char_index[i]);
            drawDollar();
            OSTimeDly(TICKS_PER_SECOND);
        }
        myLCD.Move(char_index[LINE5_END]);
        drawDollar();
        OSTimeDly(TICKS_PER_SECOND);
        OSSemPost(&sem4);

    }
}
