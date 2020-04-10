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

#include <stdbool.h>

#include "driverlib.h"
#include "interrupts.h"
#include "init.h"
#include "utils.h"
#include "console.h"
#include "checkpointing_test_fixture.h"

/*
 * Timer0_A1 Interrupt Vector handler
 *
 */
#pragma vector = TIMER0_A1_VECTOR
__interrupt void TIMER0_A1_ISR(void)
{
    // Update our system tick
    uptimeTicksMicroseconds += TIMER_PERIOD_TICKS;
    // Clear the interrupt
    Timer_A_clearTimerInterrupt(TIMER_A0_BASE);
}

/*
 * PORT8_VECTOR Interrupt Vector handler
 *
 */
#pragma vector=PORT8_VECTOR
__interrupt void PORT8_ISR(void)
{
    // Signal that power loss has occurred
    checkpointingObj.powerLoss = true;
    // P8.1 IFG cleared
    GPIO_clearInterrupt(GPIO_PORT_P8, GPIO_PIN1);
}


