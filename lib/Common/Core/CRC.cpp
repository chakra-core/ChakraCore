//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "CommonCorePch.h"
#include "Core/CRC.h"

unsigned int CalculateCRC32(unsigned int bufferCRC, size_t data)
{
    /* update running CRC calculation with contents of a buffer */

    bufferCRC = bufferCRC ^ 0xffffffffL;
    bufferCRC = crc_32_tab[(bufferCRC ^ data) & 0xFF] ^ (bufferCRC >> 8);
    return (bufferCRC ^ 0xffffffffL);
}

unsigned int CalculateCRC32(const char* in)
{
    unsigned int crc = (unsigned int)-1;
    while (*in != '\0')
    {
        crc = (crc >> 8) ^ crc_32_tab[(crc ^ *in) & 0xFF];
        in++;
    }
    return crc ^ (unsigned int)-1;
}
