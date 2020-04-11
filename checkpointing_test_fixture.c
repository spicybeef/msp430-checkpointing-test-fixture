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

#include <stdio.h>
#include "driverlib.h"
#include "checkpointing_test_fixture.h"
#include "console.h"
#include "utils.h"

#define AES_MINIMUM_CHUNK_SIZE (16) // Size of data to be encrypted/decrypted (must be multiple of 16)
static uint8_t dataAESencrypted[AES_MINIMUM_CHUNK_SIZE]; // Encrypted data
//static uint8_t dataAESdecrypted[AES_MINIMUM_CHUNK_SIZE]; // Decrypted data, not used right now
static char message[AES_MINIMUM_CHUNK_SIZE] = {0};

// Our total workload size
#define TOTAL_WORKLOAD_SIZE_BYTES (10 * 1024ULL * 1024ULL)

volatile checkpointingObj_t checkpointingObj;

static arrayOfStrings_t workloadScalingStrings =
{
    ANSI_COLOR_MAGENTA"No Scaling"ANSI_COLOR_RESET,
    ANSI_COLOR_MAGENTA"Linear Scaling"ANSI_COLOR_RESET,
    ANSI_COLOR_MAGENTA"Random Scaling"ANSI_COLOR_RESET,
    ANSI_COLOR_MAGENTA"Counter Scaling"ANSI_COLOR_RESET,
};

static const uint16_t chunkScaleLut[CHUNK_SCALE_MAX] =
{
    1024,   // CHUNK_SCALE_1024
    512,    // CHUNK_SCALE_512
    256,    // CHUNK_SCALE_256
    128,    // CHUNK_SCALE_128
    64,     // CHUNK_SCALE_64
    32,     // CHUNK_SCALE_32
    16,     // CHUNK_SCALE_16
};

void Checkpointing_Init(void)
{
    // Reset our runtime variables
    checkpointingObj.powerLoss = false;
    checkpointingObj.currentlyWorking = false;
    checkpointingObj.startingChunkScale = CHUNK_SCALE_1024;
    checkpointingObj.deadTimeMicroseconds = 500;
    checkpointingObj.totalWorkloadSizeBytes = TOTAL_WORKLOAD_SIZE_BYTES;
    checkpointingObj.policy = WORKLOAD_SCALING_LINEAR;
};

functionResult_e PowerLossEmu_Setup(unsigned int numArgs, int args[])
{
    unsigned int i;

    // Print current settings
    Console_Print("Old settings:");
    Console_PrintDivider();
    Checkpointing_CurrentSettings(0, 0);
    Console_PrintDivider();

    // Get new settings
    checkpointingObj.totalWorkloadSizeBytes = (1024ULL*1024ULL*Console_PromptForInt("Total workload size (MB): "));
    Console_Print("Choose a starting chunk size:");
    for (i = 0; i < CHUNK_SCALE_MAX; i++)
    {
        Console_PrintNoEol("[%u] - %u ", i, chunkScaleLut[i]);
    }
    Console_PrintNewLine();
    checkpointingObj.startingChunkScale = (chunkScale_e)((0x7)&Console_PromptForInt("Starting chunk size: "));
    checkpointingObj.deadTimeMicroseconds = Console_PromptForInt("Enter dead-time (ms): ");
    Console_Print("Choose a workload policy");
    Console_Print("[0]-linear, [1]-random, [2]-counter");
    checkpointingObj.policy = (workloadScalingPolicy_e)(((0x7)&Console_PromptForInt("Enter workload policy: ")) + 1);

    Console_PrintDivider();
    Console_Print("New settings:");
    Checkpointing_CurrentSettings(0, 0);
    Console_PrintDivider();

    return SUCCESS;
}

functionResult_e Checkpointing_CurrentSettings(unsigned int numArgs, int args[])
{
    Console_Print("Total workload size size: %llu B", checkpointingObj.totalWorkloadSizeBytes);
    Console_Print("Starting chunk size: %u B", chunkScaleLut[(unsigned int)checkpointingObj.startingChunkScale]);
    Console_Print("Dead-time between workloads: %lu ms", checkpointingObj.deadTimeMicroseconds);
    Console_Print("Current workload scaling policy: %s", workloadScalingStrings[(unsigned int)checkpointingObj.policy]);

    return SUCCESS;
}

functionResult_e Checkpointing_WorkloadLoop(unsigned int numArgs, int args[])
{
    uint32_t startTicks;
    uint32_t currentTicks;
    uint32_t workloadStart;
    uint32_t workloadEnd;
    uint32_t progressTicks;

    // Reset runtime variables
    checkpointingObj.currentChunkScale = checkpointingObj.startingChunkScale;
    checkpointingObj.bytesProcessed = 0;
    checkpointingObj.workloadFails = 0;

    // Print current settings
    Console_Print("Current settings:");
    Console_PrintDivider();
    Checkpointing_CurrentSettings(0, 0);
    Console_PrintDivider();

    // Wait for the first power-loss pulse from the power-loss emulator
    checkpointingObj.powerLoss = false;
    Console_Print("Waiting for power-loss emulator sync...");
    while (!checkpointingObj.powerLoss);
    checkpointingObj.powerLoss = false;
    Console_Print(ANSI_COLOR_GREEN"SYNC!"ANSI_COLOR_RESET);

    // Reset parameters
    Checkpointing_Init();
    // Print current settings
    Console_PrintDivider();
    Checkpointing_CurrentSettings(0, 0);
    Console_PrintDivider();

    Console_Print("Beginning workload...");
    // Turn off green LED (will be turned on for completion)
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN1);

    workloadStart = Utils_GetUptimeMicroseconds();
    progressTicks = workloadStart;
    // Main loop
    for (;;)
    {
        // Perform our AES workload
        Checkpointing_DoAes();

        // Wait for a dead-time, simulates work that needs to be performed in
        // between our workloads.
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
        while ((currentTicks - startTicks) < checkpointingObj.deadTimeMicroseconds);

        if ((Utils_GetUptimeMicroseconds() - progressTicks) > 1000000UL)
        {
            progressTicks = Utils_GetUptimeMicroseconds();
            Console_PutChar('.');
            fflush(stdout);
        }

        if (checkpointingObj.bytesProcessed >= TOTAL_WORKLOAD_SIZE_BYTES)
        {
            // We're done! Leave the workload loop
            break;
        }

        if (Console_CheckForKey() != 0)
        {
            break;
        }
    }
    workloadEnd = Utils_GetUptimeMicroseconds();

    Console_PrintNewLine();
    Console_Print("Workload complete!");
    Console_PrintDivider();
    Console_Print("Processed %llu bytes", checkpointingObj.bytesProcessed);
    Console_Print("Took %f s", (workloadEnd - workloadStart)/1000000.0);
    Console_PrintDivider();
    // Turn on green LED for completion
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN1);

    return SUCCESS;
}

/**
 * @brief      Mark that work has started
 */
void Checkpointing_MarkWorkStart(void)
{
    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN1);
    checkpointingObj.currentlyWorking = true;
}

/**
 * @brief      Mark that work has ended
 */
void Checkpointing_MarkWorkEnd(void)
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN1);
    checkpointingObj.currentlyWorking = false;
}

/**
 * @brief      Perform our work.
 */
void Checkpointing_DoAes(void)
{
    uint16_t i;
    // Copy the string we want to encrypt to our buffer
    const char stringToEncrypt[] = "Meat popsicle";
    memcpy(message, stringToEncrypt, sizeof(stringToEncrypt));

    // Do stuff
    // Signal that work is starting
    Checkpointing_MarkWorkStart();
    for (i = 0; i < chunkScaleLut[(unsigned int)checkpointingObj.currentChunkScale]; i += AES_MINIMUM_CHUNK_SIZE)
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
        checkpointingObj.bytesProcessed += chunkScaleLut[(unsigned int)checkpointingObj.currentChunkScale];
        // Reset any previous failures since we've passed this one
        if (checkpointingObj.workloadFails)
        {
            checkpointingObj.workloadFails = 0;
        }
    }
    else
    {
        switch (checkpointingObj.policy)
        {
            case WORKLOAD_SCALING_NONE:
                // We don't touch the chunk size
                break;
            case WORKLOAD_SCALING_LINEAR:
                // Chunk failed. We won't count this as work performed and we need to
                // modify our behaviour.
                checkpointingObj.workloadFails++;
                // We'll allow 3 workload to fail until we change our policy so that
                // we don't change policies right away
                if (checkpointingObj.workloadFails >= 3)
                {
                    checkpointingObj.workloadFails = 0;
                    // Workload scales linearly by 2 every failure. Don't go past min
                    if (checkpointingObj.currentChunkScale != CHUNK_SCALE_16)
                    {
                        checkpointingObj.currentChunkScale = (chunkScale_e)((unsigned int)checkpointingObj.currentChunkScale + 1);
                    }
                }
                break;
            case WORKLOAD_SCALING_RANDOM:
            case WORKLOAD_SCALING_COUNTER:
                break;
            default:
                Console_Print(ANSI_COLOR_RED"Invalid policy!"ANSI_COLOR_RESET);
                break;
        }
    }

    // Reset any power-loss since we've handled it by now
    checkpointingObj.powerLoss = false;

}
