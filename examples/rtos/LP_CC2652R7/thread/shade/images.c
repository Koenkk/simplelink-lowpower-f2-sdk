/******************************************************************************

 @file images.c

 @brief graphics image data definition for lcd display for
        shade example application.

 Group: CMCU, LPC
 Target Device: cc13xx_cc26xx

 ******************************************************************************
 
 Copyright (c) 2017-2022, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 
 
 *****************************************************************************/

/******************************************************************************
 Includes
 *****************************************************************************/
#include "images.h"

/******************************************************************************
 Constants and definitions
 *****************************************************************************/
/* shade open graphics image data */
static const uint8_t shadeOpenImgData[] =
{
0xfe, 0xfe, 0xfe, 0xfe, 0xff, 0xfe, 0x04, 0x31, 0x08, 0x3b, 0x02, 0x77,
0x0a, 0x2f, 0x0c, 0x39, 0x06, 0x73, 0x0a, 0x2f, 0x02, 0x43, 0x06, 0x73,
0x0a, 0x2f, 0x02, 0x43, 0x08, 0x71, 0x0a, 0x2f, 0x04, 0x41, 0x08, 0x71,
0xfe, 0x04, 0xff, 0xfe, 0x04, 0x31, 0x02, 0x39, 0x10, 0x07, 0x06, 0x61,
0x0a, 0x31, 0x08, 0x33, 0x12, 0x07, 0x02, 0x63, 0x0a, 0x33, 0x08, 0x2f,
0x16, 0x0f, 0x0c, 0x4f, 0x0a, 0x35, 0x0a, 0x09, 0x00, 0x11, 0x24, 0x03,
0x06, 0x03, 0x0e, 0x4d, 0x0a, 0x37, 0x18, 0x0b, 0x26, 0x03, 0x06, 0x03,
0x0e, 0x4d, 0xfe, 0x04, 0xff, 0xfe, 0x04, 0x6d, 0x0c, 0x0b, 0x06, 0x09,
0x0a, 0x4d, 0x0a, 0x59, 0x20, 0x0b, 0x06, 0x09, 0x0a, 0x4d, 0x0a, 0x59,
0x26, 0x03, 0x08, 0x01, 0x14, 0x4b, 0x0a, 0x5b, 0x24, 0x03, 0x06, 0x03,
0x16, 0x49, 0x0a, 0x5d, 0x22, 0x03, 0x06, 0x03, 0x16, 0x49, 0xfe, 0x04,
0xff, 0xfe, 0x04, 0x63, 0x1a, 0x03, 0x06, 0x03, 0x18, 0x49, 0x0a, 0x63,
0x1a, 0x03, 0x06, 0x03, 0x16, 0x4b, 0x0a, 0x65, 0x18, 0x0f, 0x12, 0x0f,
0x02, 0x05, 0x08, 0x03, 0x0a, 0x1b, 0x0a, 0x65, 0x0a, 0x05, 0x06, 0x13,
0x0c, 0x01, 0x00, 0x05, 0x2e, 0x17, 0x0a, 0x67, 0x06, 0x07, 0x06, 0x13,
0x2a, 0x07, 0x04, 0x09, 0x06, 0x13, 0xfe, 0x04, 0xff, 0xfe, 0x04, 0x77,
0x02, 0x01, 0x1e, 0x41, 0x02, 0x0f, 0x0a, 0x77, 0x02, 0x03, 0x16, 0x01,
0x06, 0x3d, 0x04, 0x0d, 0x0a, 0x77, 0x02, 0x03, 0x12, 0x4d, 0x02, 0x0d,
0x0a, 0x77, 0x02, 0x05, 0x12, 0x4b, 0x02, 0x0d, 0x0a, 0x77, 0x02, 0x05,
0x16, 0x43, 0x04, 0x0f, 0xfe, 0x04, 0xff, 0xfe, 0x04, 0x77, 0x02, 0x0d,
0x16, 0x23, 0x12, 0x19, 0x0a, 0x77, 0x02, 0x1f, 0x16, 0x0f, 0x10, 0x1d,
0x0a, 0x77, 0x02, 0x25, 0x24, 0x2b, 0x0a, 0x77, 0x02, 0x37, 0x0e, 0x2f,
0x0a, 0x77, 0x02, 0x77, 0xfe, 0x04, 0xff, 0xfe, 0x04, 0x77, 0x02, 0x1f,
0x02, 0x53, 0x0a, 0x77, 0x02, 0x1f, 0x04, 0x51, 0x0a, 0x77, 0x02, 0x1f,
0x04, 0x51, 0x0a, 0x77, 0x02, 0x1d, 0x08, 0x4f, 0x0a, 0x77, 0x02, 0x1b,
0x0a, 0x4f, 0xfe, 0x04, 0xff, 0xfe, 0x04, 0x31, 0x12, 0x31, 0x02, 0x15,
0x14, 0x4b, 0x0a, 0x31, 0x12, 0x31, 0x02, 0x13, 0x18, 0x49, 0x3e, 0x0f,
0x4a, 0x19, 0x86, 0x13, 0x46, 0x1d, 0x52, 0x19, 0x30, 0x2b, 0x02, 0x11,
0x1e, 0x45, 0xfe, 0x04, 0xff, 0xfe, 0x04, 0x13, 0x3e, 0x23, 0x02, 0x0b,
0x28, 0x41, 0x0a, 0x13, 0x3e, 0x23, 0x02, 0x09, 0x2a, 0x41, 0x60, 0x21,
0x02, 0x09, 0x2c, 0x3f, 0x60, 0x21, 0x02, 0x07, 0x32, 0x3b, 0x60, 0x15,
0x00, 0x03, 0x00, 0x03, 0x02, 0x05, 0x48, 0x27, 0xfe, 0x04, 0xff, 0xfe,
0x5a, 0x11, 0x0e, 0x03, 0x4e, 0x01, 0x86, 0x11, 0x0e, 0x03, 0x4e, 0x01,
0x86, 0x11, 0x0e, 0x03, 0x4e, 0x01, 0x44, 0x0f, 0x0a, 0x0f, 0x14, 0x11,
0x0e, 0x03, 0x4e, 0x01, 0x44, 0x0f, 0x0a, 0x0f, 0x14, 0x11, 0x0e, 0x03,
0xfe, 0x7c, 0xff, 0xfe, 0x18, 0x0f, 0x0a, 0x0f, 0x14, 0x11, 0x0e, 0x03,
0x96, 0x0f, 0x0a, 0x0f, 0x14, 0x0f, 0x10, 0x03, 0x96, 0x0f, 0x0a, 0x0f,
0x16, 0x0d, 0x10, 0x03, 0xda, 0x0d, 0x10, 0x03, 0xda, 0x0d, 0x10, 0x03,
0xfe, 0x7c, 0xff, 0xfe, 0x7c, 0x03, 0xfa, 0x03, 0x00, 0x03, 0x68, 0x03,
0x86, 0x03, 0x00, 0x03, 0x68, 0x03, 0x86, 0x03, 0x00, 0x03, 0x68, 0x03,
0x7c, 0x03, 0x04, 0x03, 0x00, 0x03, 0x68, 0x03, 0xfe, 0x08, 0xff, 0xfe,
0x08, 0x0f, 0x06, 0x03, 0x00, 0x0d, 0x0a, 0x0f, 0x08, 0x0d, 0x04, 0x11,
0x00, 0x11, 0x06, 0x0d, 0x02, 0x0f, 0x06, 0x0f, 0x08, 0x0d, 0x00, 0x03,
0x12, 0x05, 0x04, 0x05, 0x04, 0x07, 0x06, 0x05, 0x06, 0x05, 0x06, 0x03,
0x06, 0x05, 0x04, 0x05, 0x06, 0x03, 0x04, 0x03, 0x00, 0x07, 0x04, 0x05,
0x04, 0x07, 0x06, 0x05, 0x06, 0x03, 0x04, 0x05, 0x06, 0x03, 0x06, 0x05,
0x06, 0x07, 0x10, 0x03, 0x0a, 0x05, 0x02, 0x05, 0x0a, 0x05, 0x04, 0x03,
0x0a, 0x03, 0x02, 0x05, 0x08, 0x03, 0x06, 0x03, 0x04, 0x03, 0x00, 0x05,
0x08, 0x03, 0x04, 0x05, 0x08, 0x03, 0x0a, 0x03, 0x12, 0x03, 0x02, 0x05,
0x0a, 0x05, 0x10, 0x03, 0x0c, 0x03, 0x02, 0x03, 0x0e, 0x03, 0x02, 0x03,
0x0c, 0x03, 0x02, 0x03, 0x0a, 0x03, 0x06, 0x03, 0x04, 0x03, 0x00, 0x03,
0x0a, 0x03, 0x04, 0x03, 0x08, 0x03, 0x0c, 0x03, 0x12, 0x03, 0x02, 0x03,
0x0c, 0x05, 0x10, 0x01, 0x0e, 0x03, 0x02, 0x03, 0x0e, 0x05, 0x00, 0x05,
0x0a, 0x03, 0x02, 0x03, 0x0c, 0x03, 0x04, 0x03, 0x04, 0x03, 0x00, 0x03,
0x0a, 0x03, 0x04, 0x03, 0x08, 0x05, 0x0a, 0x03, 0x08, 0x0d, 0x00, 0x05,
0x0e, 0x03, 0xfe, 0x08, 0xff, 0xfe, 0x06, 0x03, 0x0c, 0x03, 0x02, 0x05,
0x0c, 0x03, 0x02, 0x05, 0x12, 0x03, 0x0c, 0x03, 0x04, 0x03, 0x04, 0x03,
0x00, 0x03, 0x0a, 0x03, 0x04, 0x03, 0x08, 0x05, 0x12, 0x03, 0x08, 0x05,
0x02, 0x03, 0x0c, 0x05, 0x10, 0x05, 0x08, 0x03, 0x04, 0x07, 0x08, 0x05,
0x04, 0x05, 0x08, 0x03, 0x02, 0x03, 0x0c, 0x03, 0x04, 0x05, 0x02, 0x03,
0x00, 0x03, 0x0a, 0x03, 0x04, 0x03, 0x0a, 0x05, 0x08, 0x03, 0x02, 0x03,
0x08, 0x05, 0x02, 0x05, 0x08, 0x07, 0x12, 0x11, 0x04, 0x15, 0x08, 0x0f,
0x04, 0x03, 0x0c, 0x03, 0x06, 0x0b, 0x00, 0x03, 0x0a, 0x03, 0x04, 0x03,
0x0c, 0x0f, 0x04, 0x07, 0x00, 0x09, 0x04, 0x15, 0x14, 0x0b, 0x08, 0x03,
0x02, 0x0b, 0x0c, 0x0b, 0x06, 0x03, 0x0c, 0x03, 0x06, 0x0b, 0x00, 0x03,
0x0a, 0x03, 0x04, 0x03, 0x0e, 0x0b, 0x08, 0x0b, 0x00, 0x03, 0x06, 0x0b,
0x02, 0x03, 0x2a, 0x03, 0x56, 0x03, 0xfe, 0x7c, 0xff, 0xfe, 0x20, 0x03,
0x56, 0x03, 0x9e, 0x03, 0x56, 0x03, 0xfa, 0x03, 0xfa, 0x03, 0xfa, 0x03,
0xfe, 0x7c, 0xff, 0xfe, 0xfe, 0xfe
};

/* Shade open image palette information  */
static const uint32_t shadeOpenImgPalette[]=
{
 0x000000,   0xffffff
};

/* Shade open image information  */
const Graphics_Image Images_shadeOpen =
{
    .bPP = IMAGE_FMT_1BPP_COMP_RLE7,
    .xSize = 128,
    .ySize = 128,
    .numColors = 2,
    .pPalette = shadeOpenImgPalette,
    .pPixel = shadeOpenImgData,
};

/* shade closed graphics image data */
static const uint8_t shadeClosedImgData[] =
{
0xfe, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff,
0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff,
0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff,
0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff,
0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Shade closed image palette information  */
static const uint32_t shadeClosedImgPalette[]=
{
    0x000000,   0xffffff
};

/* Shade closed image information  */
const Graphics_Image Images_shadeClosed =
{
    .bPP = IMAGE_FMT_1BPP_COMP_RLE7,
    .xSize = 128,
    .ySize = 128,
    .numColors = 2,
    .pPalette = shadeClosedImgPalette,
    .pPixel = shadeClosedImgData,
};

/* Shade drawn image data  */
static const uint8_t shadeDrawnImgData[] =
{
0xfe, 0xfe, 0xfe, 0x04, 0x37, 0x04, 0x33, 0x08, 0x77, 0x0a, 0x37, 0x02,
0x3b, 0x02, 0x77, 0x0a, 0x35, 0x04, 0x3b, 0x02, 0x77, 0x0a, 0x31, 0x08,
0x3b, 0x02, 0x77, 0x0a, 0x2f, 0x0c, 0x39, 0x06, 0x73, 0x0a, 0x2f, 0x02,
0x43, 0x06, 0x73, 0x0a, 0x2f, 0x02, 0x43, 0x08, 0x71, 0x0a, 0x2f, 0x04,
0x41, 0x08, 0x71, 0x0a, 0x31, 0x04, 0x3f, 0x08, 0x71, 0x0a, 0x31, 0x02,
0x41, 0x08, 0x09, 0x04, 0x61, 0x0a, 0x2f, 0x04, 0x3b, 0x0e, 0x07, 0x06,
0x61, 0x0a, 0x31, 0x02, 0x39, 0x10, 0x07, 0x06, 0x61, 0x0a, 0x31, 0x08,
0x33, 0x12, 0x07, 0x02, 0x63, 0x0a, 0x33, 0x08, 0x2f, 0x16, 0x0f, 0x0c,
0x4f, 0x0a, 0x35, 0x0a, 0x09, 0x00, 0x11, 0x24, 0x03, 0x06, 0x03, 0x0e,
0x4d, 0x0a, 0x37, 0x18, 0x0b, 0x26, 0x03, 0x06, 0x03, 0x0e, 0x4d, 0x0a,
0x41, 0x1e, 0x0b, 0x14, 0x03, 0x08, 0x01, 0x10, 0x4d, 0x0a, 0x4f, 0x0e,
0x0d, 0x0e, 0x09, 0x06, 0x0b, 0x08, 0x4d, 0x0a, 0x6d, 0x0e, 0x09, 0x06,
0x09, 0x0a, 0x4d, 0x0a, 0x6d, 0x0c, 0x0b, 0x06, 0x09, 0x0a, 0x4d, 0x0a,
0x59, 0x20, 0x0b, 0x06, 0x09, 0x0a, 0x4d, 0x0a, 0x59, 0x26, 0x03, 0x08,
0x01, 0x14, 0x4b, 0x0a, 0x5b, 0x24, 0x03, 0x06, 0x03, 0x16, 0x49, 0x0a,
0x5d, 0x22, 0x03, 0x06, 0x03, 0x16, 0x49, 0x0a, 0x5f, 0x20, 0x03, 0x06,
0x03, 0x16, 0x49, 0x0a, 0x61, 0x1e, 0x03, 0x06, 0x03, 0x16, 0x49, 0x0a,
0x63, 0x1c, 0x01, 0x08, 0x01, 0x18, 0x49, 0x0a, 0x63, 0x1a, 0x03, 0x06,
0x03, 0x18, 0x49, 0x0a, 0x63, 0x1a, 0x03, 0x06, 0x03, 0x16, 0x4b, 0x0a,
0x65, 0x18, 0x0f, 0x12, 0x0f, 0x02, 0x05, 0x08, 0x03, 0x0a, 0x1b, 0x0a,
0x65, 0x0a, 0x05, 0x06, 0x13, 0x0c, 0x01, 0x00, 0x05, 0x2e, 0x17, 0x0a,
0x67, 0x06, 0x07, 0x06, 0x13, 0x2a, 0x07, 0x04, 0x09, 0x06, 0x13, 0x0a,
0x77, 0x08, 0x0f, 0x1a, 0x29, 0x08, 0x11, 0x0a, 0x77, 0x0c, 0x09, 0x0c,
0x3f, 0x04, 0x0f, 0x0a, 0x77, 0x02, 0x01, 0x1c, 0x43, 0x02, 0x0f, 0x0a,
0x77, 0x02, 0x01, 0x1e, 0x41, 0x02, 0x0f, 0x0a, 0x77, 0x02, 0x03, 0x16,
0x01, 0x06, 0x3d, 0x04, 0x0d, 0x0a, 0x77, 0x02, 0x03, 0x12, 0x4d, 0x02,
0x0d, 0x0a, 0x77, 0x02, 0x05, 0x12, 0x4b, 0x02, 0x0d, 0x0a, 0x77, 0x02,
0x05, 0x16, 0x43, 0x04, 0x0f, 0x0a, 0x77, 0x02, 0x07, 0x10, 0x41, 0x0a,
0x0f, 0x0a, 0x77, 0x02, 0x09, 0x0e, 0x41, 0x06, 0x13, 0x0a, 0x77, 0x02,
0x0b, 0x14, 0x37, 0x04, 0x17, 0x0a, 0x77, 0x02, 0x0d, 0x16, 0x23, 0x12,
0x19, 0x0a, 0x77, 0x02, 0x1f, 0x16, 0x0f, 0x10, 0x1d, 0x0a, 0x77, 0x02,
0x25, 0x24, 0x2b, 0x0a, 0x77, 0x02, 0x37, 0x0e, 0x2f, 0x0a, 0x77, 0x02,
0x77, 0x0a, 0x77, 0x02, 0x77, 0x0a, 0x77, 0x02, 0x77, 0x0a, 0x77, 0x02,
0x21, 0x00, 0x53, 0x0a, 0x77, 0x02, 0x1f, 0x02, 0x53, 0x0a, 0x77, 0x02,
0x1f, 0x04, 0x51, 0x0a, 0x77, 0x02, 0x1f, 0x04, 0x51, 0x0a, 0x77, 0x02,
0x1d, 0x08, 0x4f, 0x0a, 0x77, 0x02, 0x1b, 0x0a, 0x4f, 0x0a, 0x35, 0x0c,
0x33, 0x02, 0x19, 0x0e, 0x4d, 0x0a, 0x35, 0x0c, 0x33, 0x02, 0x19, 0x0e,
0x4d, 0x0a, 0x35, 0x0c, 0x33, 0x02, 0x17, 0x12, 0x4b, 0x0a, 0x31, 0x12,
0x31, 0x02, 0x15, 0x14, 0x4b, 0x0a, 0x31, 0x12, 0x31, 0x02, 0x13, 0x18,
0x49, 0x3e, 0x0f, 0x4a, 0x19, 0x86, 0x13, 0x46, 0x1d, 0x52, 0x19, 0x30,
0x2b, 0x02, 0x11, 0x1e, 0x45, 0x0a, 0x17, 0x3a, 0x23, 0x02, 0x0f, 0x20,
0x45, 0x0a, 0x13, 0x3e, 0x23, 0x02, 0x0d, 0x24, 0x43, 0x0a, 0x13, 0x3e,
0x23, 0x02, 0x0d, 0x24, 0x43, 0x0a, 0x13, 0x3e, 0x23, 0x02, 0x0b, 0x28,
0x41, 0x0a, 0x13, 0x3e, 0x23, 0x02, 0x09, 0x2a, 0x41, 0x60, 0x21, 0x02,
0x09, 0x2c, 0x3f, 0x60, 0x21, 0x02, 0x07, 0x32, 0x3b, 0x60, 0x15, 0x00,
0x03, 0x00, 0x03, 0x02, 0x05, 0x48, 0x27, 0x60, 0x11, 0x62, 0x27, 0x60,
0x11, 0x0e, 0x03, 0x4e, 0x27, 0x60, 0x11, 0x0e, 0x03, 0x4e, 0x27, 0x60,
0x11, 0x0e, 0x03, 0x4e, 0x01, 0x86, 0x11, 0x0e, 0x03, 0x4e, 0x01, 0x86,
0x11, 0x0e, 0x03, 0x4e, 0x01, 0x44, 0x0f, 0x0a, 0x0f, 0x14, 0x11, 0x0e,
0x03, 0x4e, 0x01, 0x44, 0x0f, 0x0a, 0x0f, 0x14, 0x11, 0x0e, 0x03, 0x96,
0x0f, 0x0a, 0x0f, 0x14, 0x11, 0x0e, 0x03, 0x96, 0x0f, 0x0a, 0x0f, 0x14,
0x11, 0x0e, 0x03, 0x96, 0x0f, 0x0a, 0x0f, 0x14, 0x11, 0x0e, 0x03, 0x96,
0x0f, 0x0a, 0x0f, 0x14, 0x11, 0x0e, 0x03, 0x96, 0x0f, 0x0a, 0x0f, 0x14,
0x0f, 0x10, 0x03, 0x96, 0x0f, 0x0a, 0x0f, 0x16, 0x0d, 0x10, 0x03, 0xda,
0x0d, 0x10, 0x03, 0xda, 0x0d, 0x10, 0x03, 0xd8, 0x0f, 0x10, 0x03, 0xda,
0x0b, 0x12, 0x03, 0xfa, 0x03, 0xfa, 0x03, 0xfa, 0x03, 0x00, 0x03, 0x68,
0x03, 0x86, 0x03, 0x00, 0x03, 0x68, 0x03, 0x86, 0x03, 0x00, 0x03, 0x68,
0x03, 0x7c, 0x03, 0x04, 0x03, 0x00, 0x03, 0x68, 0x03, 0x7c, 0x03, 0x04,
0x03, 0x00, 0x03, 0x68, 0x03, 0x7c, 0x03, 0x04, 0x03, 0x00, 0x03, 0x68,
0x03, 0x16, 0x09, 0x10, 0x07, 0x12, 0x07, 0x10, 0x05, 0x0a, 0x0f, 0x00,
0x03, 0x02, 0x07, 0x10, 0x05, 0x06, 0x07, 0x0e, 0x07, 0x0e, 0x07, 0x04,
0x03, 0x12, 0x0f, 0x06, 0x03, 0x00, 0x0d, 0x0a, 0x0f, 0x08, 0x0d, 0x04,
0x11, 0x00, 0x11, 0x06, 0x0d, 0x02, 0x0f, 0x06, 0x0f, 0x08, 0x0d, 0x00,
0x03, 0x12, 0x05, 0x04, 0x05, 0x04, 0x07, 0x06, 0x05, 0x06, 0x05, 0x06,
0x03, 0x06, 0x05, 0x04, 0x05, 0x06, 0x03, 0x04, 0x03, 0x00, 0x07, 0x04,
0x05, 0x04, 0x07, 0x06, 0x05, 0x06, 0x03, 0x04, 0x05, 0x06, 0x03, 0x06,
0x05, 0x06, 0x07, 0x10, 0x03, 0x0a, 0x05, 0x02, 0x05, 0x0a, 0x05, 0x04,
0x03, 0x0a, 0x03, 0x02, 0x05, 0x08, 0x03, 0x06, 0x03, 0x04, 0x03, 0x00,
0x05, 0x08, 0x03, 0x04, 0x05, 0x08, 0x03, 0x0a, 0x03, 0x12, 0x03, 0x02,
0x05, 0x0a, 0x05, 0x10, 0x03, 0x0c, 0x03, 0x02, 0x03, 0x0e, 0x03, 0x02,
0x03, 0x0c, 0x03, 0x02, 0x03, 0x0a, 0x03, 0x06, 0x03, 0x04, 0x03, 0x00,
0x03, 0x0a, 0x03, 0x04, 0x03, 0x08, 0x03, 0x0c, 0x03, 0x12, 0x03, 0x02,
0x03, 0x0c, 0x05, 0x10, 0x01, 0x0e, 0x03, 0x02, 0x03, 0x0e, 0x05, 0x00,
0x05, 0x0a, 0x03, 0x02, 0x03, 0x0c, 0x03, 0x04, 0x03, 0x04, 0x03, 0x00,
0x03, 0x0a, 0x03, 0x04, 0x03, 0x08, 0x05, 0x0a, 0x03, 0x08, 0x0d, 0x00,
0x05, 0x0e, 0x03, 0x10, 0x01, 0x0e, 0x05, 0x00, 0x03, 0x0e, 0x05, 0x00,
0x17, 0x00, 0x03, 0x0c, 0x03, 0x04, 0x03, 0x04, 0x03, 0x00, 0x03, 0x0a,
0x03, 0x04, 0x03, 0x08, 0x17, 0x02, 0x11, 0x00, 0x05, 0x0e, 0x03, 0x10,
0x01, 0x0e, 0x05, 0x00, 0x03, 0x0e, 0x05, 0x00, 0x05, 0x12, 0x03, 0x0c,
0x03, 0x04, 0x03, 0x04, 0x03, 0x00, 0x03, 0x0a, 0x03, 0x04, 0x03, 0x08,
0x05, 0x12, 0x05, 0x08, 0x03, 0x00, 0x05, 0x0e, 0x03, 0x10, 0x03, 0x0c,
0x03, 0x02, 0x03, 0x0e, 0x03, 0x02, 0x03, 0x14, 0x03, 0x0c, 0x03, 0x04,
0x03, 0x04, 0x03, 0x00, 0x03, 0x0a, 0x03, 0x04, 0x03, 0x08, 0x03, 0x14,
0x03, 0x0a, 0x03, 0x02, 0x03, 0x0e, 0x03, 0x10, 0x03, 0x0c, 0x03, 0x02,
0x05, 0x0c, 0x03, 0x02, 0x05, 0x12, 0x03, 0x0c, 0x03, 0x04, 0x03, 0x04,
0x03, 0x00, 0x03, 0x0a, 0x03, 0x04, 0x03, 0x08, 0x05, 0x12, 0x03, 0x08,
0x05, 0x02, 0x03, 0x0c, 0x05, 0x10, 0x05, 0x08, 0x03, 0x04, 0x07, 0x08,
0x05, 0x04, 0x05, 0x08, 0x03, 0x02, 0x03, 0x0c, 0x03, 0x04, 0x05, 0x02,
0x03, 0x00, 0x03, 0x0a, 0x03, 0x04, 0x03, 0x0a, 0x05, 0x08, 0x03, 0x02,
0x03, 0x08, 0x05, 0x02, 0x05, 0x08, 0x07, 0x12, 0x11, 0x04, 0x15, 0x08,
0x0f, 0x04, 0x03, 0x0c, 0x03, 0x06, 0x0b, 0x00, 0x03, 0x0a, 0x03, 0x04,
0x03, 0x0c, 0x0f, 0x04, 0x07, 0x00, 0x09, 0x04, 0x15, 0x14, 0x0b, 0x08,
0x03, 0x02, 0x0b, 0x0c, 0x0b, 0x06, 0x03, 0x0c, 0x03, 0x06, 0x0b, 0x00,
0x03, 0x0a, 0x03, 0x04, 0x03, 0x0e, 0x0b, 0x08, 0x0b, 0x00, 0x03, 0x06,
0x0b, 0x02, 0x03, 0x2a, 0x03, 0x56, 0x03, 0x9e, 0x03, 0x56, 0x03, 0x9e,
0x03, 0x56, 0x03, 0x9e, 0x03, 0x56, 0x03, 0x9e, 0x03, 0x56, 0x03, 0x9e,
0x03, 0x56, 0x03, 0xfa, 0x03, 0xfa, 0x03, 0xfa, 0x03, 0xfa, 0x03, 0xfa,
0x03, 0xfe, 0xfe, 0xfe, 0x7c
};

/* Shade drawn image palette information */
static const uint32_t shadeDrawnImgPalette[]=
{
    0x000000,   0xffffff
};

/* Shade drawn image information */
const Graphics_Image Images_shadeDrawn =
{
    .bPP = IMAGE_FMT_1BPP_COMP_RLE7,
    .xSize = 128,
    .ySize = 128,
    .numColors = 2,
    .pPalette = shadeDrawnImgPalette,
    .pPixel = shadeDrawnImgData,
};
