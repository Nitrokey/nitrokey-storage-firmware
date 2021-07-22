/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 26.11.2012
 *
 * This file is part of Nitrokey
 *
 * Nitrokey  is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Nitrokey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Nitrokey. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * file_io.c
 *
 *  Created on: 26.11.2012
 *      Author: RB
 */



#include <stdarg.h>
#include "board.h"

#ifdef FREERTOS_USED
#include "FreeRTOS.h"
#include "task.h"
#endif
#include "string.h"
#include "stdio.h"
#include "print_funcs.h"
#include "gpio.h"
#include "pm.h"
#include "adc.h"
#include "time.h"
#include "global.h"
#include "tools.h"
#include "Interpreter.h"
#include "BUFFERED_SIO.h"
#include "FILE_IO/FileAccessInterface.h"
#include "TIME_MEASURING.h"

/* FAT includes */
#include "file.h"
#include "navigation.h"
#include "ctrl_access.h"
#include "fsaccess.h"

#include "Inbetriebnahme.h"
#include "html_io.h"
#include "INTERNAL_WORK/internal_work.h"
#include "HighLevelFunctions/HandleAesStorageKey.h"
#include "DFU_test.h"
#include "SD_Test.h"



/*******************************************************************************

 Local defines

*******************************************************************************/

/*
   #define HTML_FILEIO_LUN 0 // RAM Disk #define SD_UNCRYPTED_FILEIO_LUN 1 #define SD_CRYPTED_FILEIO_LUN 2 */

#define SD_UNCRYPTED_FILEIO_LUN           0
#define SD_CRYPTED_FILEIO_LUN             1

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

void sd_mmc_mci_resources_init (void);

/*******************************************************************************

 Local declarations

*******************************************************************************/

/*******************************************************************************

  SD_UncryptedFileIO_Init_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


u8 SD_UncryptedFileIO_Init_u8 (void)
{

    b_fsaccess_init ();

    // Mount SD drive
    CI_LocalPrintf ("Mount uncrypted SD Card\r\n");

    if (FALSE == FAI_mount (SD_UNCRYPTED_FILEIO_LUN))
    {
        return (FALSE);
    }

    FAI_UpdateContend (1);
    vTaskDelay (100);

    // Show free space
    // CI_LocalPrintf ("\r\n\r\n");
    FAI_free_space (1);

    // Show dirs
    CI_LocalPrintf ("\r\nOld dir\r\n");
    FAI_Dir (0);

    return (TRUE);
}

/*******************************************************************************

  FileIO_UpdateToHost_v

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

extern Ctrl_status sd_mmc_mci_unit_state_e;

void FileIO_UpdateToHost_v (void)
{
#if LUN_2 == ENABLE
    set_virtual_unit_busy ();   // Tell the host that the FAT has changed
#endif
    sd_mmc_mci_unit_state_e = CTRL_BUSY;
}

/*******************************************************************************

  FileIO_CdDir_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FileIO_CdDir_u8 (u8 * Dir_pu8)
{
    // cd dir
    CI_LocalPrintf ("cd to -%s-\r\n", Dir_pu8);
    if (FALSE == FAI_cd ((u8 *) Dir_pu8))
    {
        // Make new dir if not present
        CI_LocalPrintf ("make dir -%s-\r\n", Dir_pu8);
        FAI_mkdir ((u8 *) Dir_pu8);

        CI_LocalPrintf ("cd to -%s-\r\n", Dir_pu8);
        if (FALSE == FAI_cd ((u8 *) Dir_pu8))
        {
            CI_LocalPrintf ("cd to -%s- fails\r\n", Dir_pu8);
            return (FALSE);
        }
    }
    return (TRUE);
}

/*******************************************************************************

  FileIO_AppendText_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FileIO_AppendText_u8 (u8 * Filename_pu8, u8 * Text_pu8)
{
s32 FileID_s32;
s32 Ret_s32;

    FileID_s32 = open ((char *) Filename_pu8, O_CREAT | O_APPEND);
    if (0 > FileID_s32)
    {
        CI_LocalPrintf ("Error open %s - %d - %s\r\n", Filename_pu8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    Ret_s32 = write (FileID_s32, Text_pu8, strlen ((char *) Text_pu8));

    Ret_s32 = close (FileID_s32);
    if (-1 == Ret_s32)
    {
        CI_LocalPrintf ("Close  = %d \r\n", Ret_s32);
    }

    return (TRUE);
}

/*******************************************************************************

  FileIO_AppendBin_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FileIO_AppendBin_u8 (u8 * Filename_pu8, u8 * Bin_pu8, u16 Count_u16)
{
s32 FileID_s32;
s32 Ret_s32;

    FileID_s32 = open ((char *) Filename_pu8, O_CREAT | O_APPEND);
    if (0 > FileID_s32)
    {
        CI_LocalPrintf ("Error open %s - %d - %s\r\n", Filename_pu8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    Ret_s32 = write (FileID_s32, Bin_pu8, Count_u16);

    Ret_s32 = close (FileID_s32);
    if (-1 == Ret_s32)
    {
        CI_LocalPrintf ("Close  = %d \r\n", Ret_s32);
    }

    return (TRUE);
}

/*******************************************************************************

  FileIO_SaveAppImage_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define FILEIO_STATUS_DIR        "status"
#define FILEIO_IMAGE_FILE        "firmware.bin"
#define FILEIO_IMAGE_START       0x80000000
#define FILEIO_IMAGE_SIZE        (498*512)  // 0x3E400 // Don't save the hidden volume and OTP data area below - 0x40000 - // 256 k
#define FILEIO_IMAGE_BLOCKSIZE   4096

u8 FileIO_SaveAppImage_u8 (void)
{
u16 i;
s32 FileID_s32;
s32 Ret_s32;

    // Init uncrypted file io
    SD_UncryptedFileIO_Init_u8 ();

    // Delete possible old file
    FAI_DeleteFile ((u8 *) FILEIO_IMAGE_FILE);

    // Open File
    FileID_s32 = open ((char *) FILEIO_IMAGE_FILE, O_CREAT | O_APPEND);
    if (0 > FileID_s32)
    {
        CI_LocalPrintf ("Error open %s - %d - %s\r\n", FILEIO_IMAGE_FILE, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    // Write data
    for (i = 0; i < (FILEIO_IMAGE_SIZE / FILEIO_IMAGE_BLOCKSIZE); i++)
    {
        CI_LocalPrintf ("Write block = %3d\r\n", i);
        Ret_s32 = write (FileID_s32, (u8 *) (FILEIO_IMAGE_START + i * FILEIO_IMAGE_BLOCKSIZE), FILEIO_IMAGE_BLOCKSIZE);
    }

    // Close file
    Ret_s32 = close (FileID_s32);
    if (-1 == Ret_s32)
    {
        CI_LocalPrintf ("Close  = %d \r\n", Ret_s32);
    }

    // Flush and close the file
    file_flush ();

    FAI_MarkLunAsChanged(0);

    return (TRUE);
}


/*******************************************************************************

  FileIO_SaveAppImage_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
/*
   #define FILEIO_STATUS_DIR "status" #define FILEIO_IMAGE_FILE "status\\app.bin" #define FILEIO_IMAGE_START 0x80000000 #define FILEIO_IMAGE_SIZE
   (498*512) // 0x3E400 // Don't save the hidden volume and OTP data area below - 0x40000 - // 256 k #define FILEIO_IMAGE_BLOCKSIZE 4096

   u8 FileIO_SaveAppImage_u8_old (void) { u16 i; s32 FileID_s32; s32 Ret_s32;

   // Init uncrypted file io SD_UncryptedFileIO_Init_u8 ();

   // Change dir FileIO_CdDir_u8 ((u8*)FILEIO_STATUS_DIR);

   // Delete possible old file FAI_DeleteFile ((u8*)FILEIO_IMAGE_FILE);

   // Open File FileID_s32 = open ((char*)FILEIO_IMAGE_FILE,O_CREAT|O_APPEND); if (0 > FileID_s32) { CI_LocalPrintf ("Error open %s - %d -
   %s\r\n",FILEIO_IMAGE_FILE,fs_g_status, IBN_FileSystemErrorText(fs_g_status)); return (FALSE); }

   // Write data for (i=0;i<(FILEIO_IMAGE_SIZE/FILEIO_IMAGE_BLOCKSIZE);i++) { CI_LocalPrintf ("Write block = %3d\r\n",i); Ret_s32 = write
   (FileID_s32,(u8*)(FILEIO_IMAGE_START + i * FILEIO_IMAGE_BLOCKSIZE),FILEIO_IMAGE_BLOCKSIZE); }

   // Close file Ret_s32 = close (FileID_s32); if (-1 == Ret_s32) { CI_LocalPrintf ("Close = %d \r\n",Ret_s32); }

   // Flush and close the file file_flush ();

   // Inform host // FileIO_UpdateToHost_v ();

   return (TRUE); } */

/*******************************************************************************

  InitStatusFiles_u8

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


#define FILEIO_STATUS_FILE        "status\\status.txt"
#define FILEIO_STATUS_TEXT_START  "Welcome to Nitrokey"


u8 InitStatusFiles_u8 (void)
{

    // Init uncrypted file io
    SD_UncryptedFileIO_Init_u8 ();

    // Change dir
    FileIO_CdDir_u8 ((u8 *) FILEIO_STATUS_DIR);

    // Write test
    FileIO_AppendText_u8 ((u8 *) FILEIO_STATUS_FILE, (u8 *) FILEIO_STATUS_TEXT_START);

    // Flush and close the file
    file_flush ();

    // Inform host
    FileIO_UpdateToHost_v ();

    return (TRUE);
}

#define DEBUG_FILE

#ifdef DEBUG_FILE

#define FILEIO_DEBUG_FILE "debug.txt"

u8 WriteStrToDebugFile (u8 *String_pu8)
{
    SD_UncryptedFileIO_Init_u8 ();
    FileIO_AppendText_u8((u8 *) FILEIO_DEBUG_FILE, String_pu8);
    FileIO_AppendText_u8((u8 *) FILEIO_DEBUG_FILE, "\r\n");
    file_flush();
//    FileIO_UpdateToHost_v ();
    FAI_MarkLunAsChanged(0);

    return (TRUE);
}


int printf_file (char* szFormat, ...)
{
    u8 szBuffer[MAX_TEXT_LENGTH]; // on task stack
    va_list args;
    va_start (args, szFormat);
    vsnprintf ((char *) szBuffer, MAX_TEXT_LENGTH - 1, szFormat, args);
    va_end(args);
    WriteStrToDebugFile (szBuffer);
    return (L_OK);
}


void dump_arr(const char* name, const u8* p, const size_t size){
    printf_file("%s: %x %x %x %x\n", name, p[0], p[1], p[2], p[3]);
}

#else
u8 WriteStrToDebugFile (u8 *String_pu8){return TRUE;}
int printf_file (char* szFormat, ...){return (L_OK);}
#define dump_arr

#endif

/*******************************************************************************

  IBN_FileIo_Tests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void IBN_FileIo_Tests (u8 nParamsGet_u8, u8 CMD_u8, u32 Param_u32, u32 Param_1_u32, u32 Param_2_u32)
{

    if (0 == nParamsGet_u8)
    {
        CI_LocalPrintf ("File test functions\r\n");
        CI_LocalPrintf ("0 Init SD interface\r\n");
        CI_LocalPrintf ("1 Init status files\r\n");
        CI_LocalPrintf ("2 Save app image\r\n");
        CI_LocalPrintf ("\r\n");
        return;
    }

    switch (CMD_u8)
    {
        case 0:
            sd_mmc_mci_resources_init ();
            break;

        case 1:
            InitStatusFiles_u8 ();
            break;

        case 2:
            FileIO_SaveAppImage_u8 ();
            break;

        case 3:
            FileIO_CdDir_u8 ((u8 *) FILEIO_STATUS_DIR);
            break;

        case 4:
            FAI_mkdir ((u8 *) FILEIO_STATUS_DIR);
            break;
    }
}
