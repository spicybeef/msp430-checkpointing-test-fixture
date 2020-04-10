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

#ifndef CHECKPOINTING_TEST_FIXTURE_H
#define CHECKPOINTING_TEST_FIXTURE_H

#include <stdbool.h>
#include "console.h"

typedef enum
{
    WORKLOAD_SCALING_LINEAR = 0,
    WORKLOAD_SCALING_RANDOM = 1,
    WORKLOAD_SCALING_COUNTER = 2,
} workloadScalingPolicy_e;

typedef struct
{
    // Power loss flag (raised by the GPIO interrupt)
    bool powerLoss;
    // Active work flag (raised while workload is busy doing work)
    bool currentlyWorking;
    // Starting chunk size
    uint16_t startingChunkSize;
    // Current chunk size
    uint16_t currentChunkSize;
    // Total bytes processed by the workload
    uint32_t bytesProcessed;
    // Deadtime between workloads (simulates data transfer or other work)
    uint32_t deadTimeMicroseconds;
    // Total workload size
    uint32_t totalWorkloadSizeBytes;
    // Workload scaling policy
    workloadScalingPolicy_e policy;
} checkpointingObj_t;

extern volatile checkpointingObj_t checkpointingObj;

void Checkpointing_Init(void);
functionResult_e PowerLossEmu_Setup(unsigned int numArgs, int args[]);
functionResult_e Checkpointing_CurrentSettings(unsigned int numArgs, int args[]);
void Checkpointing_ExecuteWorkloadPolicy(void);
functionResult_e Checkpointing_WorkloadLoop(unsigned int numArgs, int args[]);
void Checkpointing_MarkWorkEnd(void);
void Checkpointing_MarkWorkStart(void);
void Checkpointing_DoAes(void);

#endif // CHECKPOINTING_TEST_FIXTURE_H
