/*
 * Copyright (c) 2014, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* This file was derived from UARTUtils.c and UARTEUSCIA.c from TIRTOS example files */

#include <stdio.h>
#include "driverlib.h"
#include "uartlib.h"

// stdio buffers
#define IO_BUFF_SIZE (256)
static char stdinBuff[IO_BUFF_SIZE];
static char stdoutBuff[IO_BUFF_SIZE];

static UartLib_Object_t UartLib_Object;

void UartLib_Init(void)
{
    /* Add the UART device to the system. */
    add_device("UART", _MSA, UartLib_DeviceOpen,
               UartLib_DeviceClose, UartLib_DeviceRead,
               UartLib_DeviceWrite, UartLib_DeviceLSeek,
               UartLib_DeviceUnlink, UartLib_DeviceRename);

    /* Open UART0 for writing to stdout and set buffer */
    freopen("UART:0", "w", stdout);
    setvbuf(stdout, stdinBuff, _IOLBF, IO_BUFF_SIZE);

    /* Open UART0 for reading from stdin and set buffer */
    freopen("UART:0", "r", stdin);
    setvbuf(stdin, stdoutBuff, _IOLBF, IO_BUFF_SIZE);
}

int UartLib_DeviceClose(int fd)
{
    return (0);
}

off_t UartLib_DeviceLSeek(int fd, off_t offset, int origin)
{
    return (-1);
}

int UartLib_DeviceOpen(const char *path, unsigned flags, int mode)
{
    UartLib_Object.readDataMode = UART_DATA_TEXT;
    UartLib_Object.writeDataMode = UART_DATA_TEXT;
    UartLib_Object.readReturnMode = UART_RETURN_NEWLINE;
    UartLib_Object.readEcho = UART_ECHO_ON;

    return (0);
}

int UartLib_DeviceRead(int fd, char *buffer, unsigned size)
{
    int ret;

    ret = UartLib_ReadPolling((uint8_t *)buffer, size);

    return (ret);
}

int UartLib_DeviceWrite(int fd, const char *buffer, unsigned size)
{
    int ret;

    ret = UartLib_WritePolling((uint8_t *)buffer, size);

    return (ret);
}

int UartLib_DeviceUnlink(const char *path)
{
    return (-1);
}

int UartLib_DeviceRename(const char *old_name, const char *new_name)
{
    return (-1);
}


int UartLib_ReadPolling(void *buffer, size_t size)
{
    /* Save the data to be read and restore interrupts. */
    UartLib_Object.readBuf = buffer;
    UartLib_Object.readSize = size;
    UartLib_Object.readCount = 0;

    while (UartLib_Object.readSize)
    {
        /* Wait until we have RX a byte */
        while (!EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG));
        UartLib_ReadData();
    }

    return (UartLib_Object.readCount);
}

int UartLib_WritePolling(const void *buffer, size_t size)
{
    /* Save the data to be written and restore interrupts. */
    UartLib_Object.writeBuf = buffer;
    UartLib_Object.writeSize = size;
    UartLib_Object.writeCount = 0;

    while (UartLib_Object.writeSize)
    {
        /* Wait until we can TX a byte */
        while (!EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG));
        UartLib_WriteData();
    }

    return (UartLib_Object.writeCount);
}

void UartLib_WriteData()
{
    /* If mode is TEXT process the characters */
    if (UartLib_Object.writeDataMode == UART_DATA_TEXT)
    {
        if (UartLib_Object.writeCR)
        {
            EUSCI_A_UART_transmitData(EUSCI_A0_BASE, '\r');
            UartLib_Object.writeSize--;
            UartLib_Object.writeCount++;
            UartLib_Object.writeCR = false;
        }
        else
        {
            /* Add a return if next character is a newline. */
           if (*(unsigned char *)UartLib_Object.writeBuf == '\n')
           {
               UartLib_Object.writeSize++;
               UartLib_Object.writeCR = true;
           }
           EUSCI_A_UART_transmitData(EUSCI_A0_BASE, *(unsigned char *)UartLib_Object.writeBuf);
           UartLib_Object.writeBuf = (unsigned char *)UartLib_Object.writeBuf + 1;
           UartLib_Object.writeSize--;
           UartLib_Object.writeCount++;
        }
    }
    else
    {
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, *(unsigned char *)UartLib_Object.writeBuf);
        UartLib_Object.writeBuf = (unsigned char *)UartLib_Object.writeBuf + 1;
        UartLib_Object.writeSize--;
        UartLib_Object.writeCount++;
    }
}

void UartLib_ReadData()
{
    uint8_t readIn;

    /* Receive char */
    readIn = EUSCI_A_UART_receiveData(EUSCI_A0_BASE);

    /* If data mode is set to TEXT replace return with a newline. */
    if (UartLib_Object.readDataMode == UART_DATA_TEXT)
    {
        if (readIn == '\r')
        {
            /* Echo character if enabled. */
            if (UartLib_Object.readEcho)
            {
                /* Wait until TX is ready */
                while (!EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG));
                EUSCI_A_UART_transmitData(EUSCI_A0_BASE, '\r');
            }
            readIn = '\n';
        }
    }

    /* Echo character if enabled. */
    if (UartLib_Object.readEcho)
    {
        /* Wait until TX is ready */
        while (!EUSCI_A_UART_getInterruptStatus(EUSCI_A0_BASE,
                    EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG));
        EUSCI_A_UART_transmitData(EUSCI_A0_BASE, readIn);
    }


    *(unsigned char *)UartLib_Object.readBuf = readIn;
    UartLib_Object.readBuf = (unsigned char *)UartLib_Object.readBuf + 1;
    UartLib_Object.readCount++;
    UartLib_Object.readSize--;

    /* If read return mode is newline, finish if a newline was received. */
    if (UartLib_Object.readReturnMode == UART_RETURN_NEWLINE && readIn == '\n')
    {
        UartLib_Object.readSize = 0;
    }
}
