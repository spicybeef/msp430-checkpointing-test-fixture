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

#include "driverlib.h"
#include "checkpointing_test_fixture.h"
#include "console.h"
#include "utils.h"

#define AES_ENCRYPTION_DATA_SIZE (16) // Size of data to be encrypted/decrypted (must be multiple of 16)
static uint8_t dataAESencrypted[AES_ENCRYPTION_DATA_SIZE]; // Encrypted data
//static uint8_t dataAESdecrypted[AES_ENCRYPTION_DATA_SIZE]; // Decrypted data, not used right now
static char message[AES_ENCRYPTION_DATA_SIZE] = {0};

// Our total workload size (3MB will yield about 30s of work)
#define TOTAL_WORKLOAD_SIZE_BYTES (3145728)

volatile checkpointingObj_t checkpointingObj;

void Checkpointing_Init(void)
{
    // Reset our runtime variables
    checkpointingObj.powerLoss = false;
    checkpointingObj.currentlyWorking = false;
    checkpointingObj.currentChunkSize = 1024;
    checkpointingObj.bytesProcessed = 0;
}

/**
 * @brief      This executes the policy that we're current employing to scale
 *             our workloads. This will modify the current chunk size.
 */
void Checkpointing_ExecuteWorkloadPolicy(void)
{
    // No policy yet!
}

functionResult_e Checkpointing_WorkloadLoop(unsigned int numArgs, int args[])
{
    uint32_t startTicks;
    uint32_t currentTicks;

    // Main loop
    for (;;)
    {
        // Perform our AES workload
        Checkpointing_DoAes();

        // Wait 1ms, simulates work that needs to be performed in between our
        // workloads.
        startTicks = Utils_GetUptimeMicroseconds();
        do
        {
            // If we encounter a power-loss here, that's ok!, But reset the
            // timer. This will also reset the power-loss flag experienced
            // during our workload.
            if (checkpointingObj.powerLoss)
            {
                checkpointingObj.powerLoss = false;
                startTicks = Utils_GetUptimeMicroseconds();
            }
            currentTicks = Utils_GetUptimeMicroseconds();
        }
        while ((currentTicks - startTicks) < 1000);

        if (checkpointingObj.bytesProcessed >= TOTAL_WORKLOAD_SIZE_BYTES)
        {
            // We're done! Leave the workload loop
            break;
        }
    }

    // Turn on green LED for completion
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN1);

    return SUCCESS;
}

/**
 * @brief      Mark that work has started
 */
void Checkpointing_MarkWorkStart(void)
{
    GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN1);
    checkpointingObj.currentlyWorking = true;
}

/**
 * @brief      Mark that work has ended
 */
void Checkpointing_MarkWorkEnd(void)
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN1);
    checkpointingObj.currentlyWorking = false;
}

/**
 * @brief      Perform our work.
 */
void Checkpointing_DoAes(void)
{
    uint16_t i;
    // Copy the string we want to encrypt to our buffer
    const char stringToEncrypt[] = "I am a meat popsicle.           ";
    memcpy(message, stringToEncrypt, sizeof(stringToEncrypt));

    // Do stuff
    // Signal that work is starting
    Checkpointing_MarkWorkStart();
    for (i = 0; i < checkpointingObj.currentChunkSize; i += AES_ENCRYPTION_DATA_SIZE)
    {
        // Encrypt data with preloaded cipher key. For this fixture, we will be
        // performing work on the same message (no real work is being done, just
        // counting how many successful chunks we've accomplished.
        AES256_encryptData(AES256_BASE, (uint8_t*)(message), dataAESencrypted);
        // Check if we need to abort our current chunk
        if (checkpointingObj.powerLoss)
        {
            // If we raised a power-loss flag, it means that at some point during our
            // current chunk we encountered a power-loss. This chunk is no
            // longer valid. Break out of loop.
            break;
        }
    }
    // Signal that work has halted
    Checkpointing_MarkWorkEnd();
    if (!checkpointingObj.powerLoss)
    {
        // If we're here, the chunk successfully executed! Add to our total
        // bytes processed accumulator.
        checkpointingObj.bytesProcessed += checkpointingObj.currentChunkSize;
    }
    else
    {
        // Chunk failed. We won't count this as work performed and we need to
        // modify our behaviour.
        Checkpointing_ExecuteWorkloadPolicy();
    }

}
