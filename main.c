/*
 * MIT License
 *
 * Copyright (c) 2020 Michel Kakulphimp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <file.h>

#include "driverlib.h"

#include "init.h"
#include "interrupts.h"
#include "menus.h"
#include "uartlib.h"
#include "utils.h"
#include "checkpointing_test_fixture.h"

#pragma PERSISTENT(cipherKey)
uint8_t cipherKey[32] =
{
    0xDE, 0xAD, 0xBE, 0xEF,
    0xBA, 0xDC, 0x0F, 0xEE,
    0xFE, 0xED, 0xBE, 0xEF,
    0xBE, 0xEF, 0xBA, 0xBE,
    0xBA, 0xDF, 0x00, 0x0D,
    0xFE, 0xED, 0xC0, 0xDE,
    0xD0, 0xD0, 0xCA, 0xCA,
    0xCA, 0xFE, 0xBA, 0xBE,
};

void main(void)
{
    bool success;

    // Peripheral initialization
    Gpio_Init();
    Clock_Init();
    Timer_Init();
    Aes_Init(cipherKey);
    success = Uart_Init();
    UartLib_Init();

    // Initialize program variables
    Checkpointing_Init();

    // Setup console interface
    consoleSettings_t consoleSettings = 
    {
        &splashScreen,
        NUM_SPLASH_LINES,
        &mainMenu,
    };
    Console_Init(&consoleSettings);
    // Erase screen
    Console_Print(ERASE_SCREEN);

    if (!success)
    {
        // Turn on red LED for failure
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        goto FOREVER;
    }

    // Enable global interrupts
    __enable_interrupt();

    // Start console interface
    Console_Main(); // Does not return

FOREVER:
    for (;;)
    {
        // Not just diamonds last forever...
        __no_operation();
    }
}
