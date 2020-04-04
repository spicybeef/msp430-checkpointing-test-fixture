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

#ifndef UARTLIB_H

#include <stddef.h>
#include <file.h>

/*!
 *  @brief      UART data mode settings
 *
 *  This enumeration defines the data mode for read and write.
 *  If the DataMode is text for write, write will add a return before a newline
 *  character.  If the DataMode is text for a read, read will replace a return
 *  with a newline.  This effectively treats all device line endings as LF and
 *  all host PC line endings as CRLF.
 */
typedef enum
{
    UART_DATA_BINARY,       /*!< Data is not processed */
    UART_DATA_TEXT,         /*!< Data is processed according to above */
} UartLib_DataMode_e;

/*!
 *  @brief      UART return mode settings
 *
 *  This enumeration defines the return modes for UART_read and UART_readPolling.
 *  UART_RETURN_FULL unblocks or performs a callback when the read buffer has
 *  been filled.
 *  UART_RETURN_NEWLINE unblocks or performs a callback whenever a newline
 *  character has been received.
 */
typedef enum
{
    UART_RETURN_FULL,       /*! Unblock/callback when buffer is full. */
    UART_RETURN_NEWLINE,    /*! Unblock/callback when newline character is received. */
} UartLib_ReturnMode_e;

/*!
 *  @brief      UART echo settings
 *
 *  This enumeration defines if the driver will echo data.
 */
typedef enum
{
    UART_ECHO_OFF = 0,      /*!< Data is not echoed */
    UART_ECHO_ON = 1,       /*!< Data is echoed */
} UartLib_Echo_e;


typedef struct
{
    /* UartLib Control Variables */
    UartLib_DataMode_e      readDataMode;   /* Type of data being read */
    UartLib_DataMode_e      writeDataMode;  /* Type of data being written */
    UartLib_ReturnMode_e    readReturnMode; /* Receive return mode */
    UartLib_Echo_e          readEcho;       /* Echo received data back */

    /* UartLib Write Variables */
    const void              *writeBuf;      /* Buffer data pointer */
    size_t                  writeCount;     /* Number of Chars sent */
    size_t                  writeSize;      /* Chars remaining in buffer */
    bool                    writeCR;        /* Write a return character */

    /* UartLib Receive Cariables */
    void                    *readBuf;       /* Buffer data pointer */
    size_t                  readCount;      /* Number of Chars read */
    size_t                  readSize;       /* Chars remaining in buffer */
} UartLib_Object_t;

int UartLib_DeviceClose(int fd);
off_t UartLib_DeviceLSeek(int fd, off_t offset, int origin);
int UartLib_DeviceOpen(const char *path, unsigned flags, int mode);
int UartLib_DeviceRead(int fd, char *buffer, unsigned size);
int UartLib_DeviceWrite(int fd, const char *buffer, unsigned size);
int UartLib_DeviceUnlink(const char *path);
int UartLib_DeviceRename(const char *old_name, const char *new_name);
int UartLib_ReadPolling(void *buffer, size_t size);
int UartLib_WritePolling(const void *buffer, size_t size);
inline void UartLib_WriteData(void);
inline void UartLib_ReadData(void);

#endif // UARTLIB_H
