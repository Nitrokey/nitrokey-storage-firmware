/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 14.04.2011
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
 * FileAccessInterface.c
 *
 *  Created on: 14.04.2011
 *      Author: RB
 *
 *  Most based on ushell_task.c functions
 */


#include "global.h"
#include "tools.h"
#include "FreeRTOS.h"
#include "task.h"

#include "conf_explorer.h"

#include "string.h"

/* FAT includes */
#include "file.h"
#include "navigation.h"
#include "ctrl_access.h"
#include "fsaccess.h"

#include "Tools/TIME_MEASURING.h"
#include "FileAccessInterface.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

#ifdef INTERPRETER_ENABLE
// #define FAI_DEBUG_PRINT 1
#endif

#ifdef FAI_DEBUG_PRINT
int CI_LocalPrintf (char* szFormat, ...);
int CI_TickLocalPrintf (char* szFormat, ...);
#else
#define CI_LocalPrintf(...)
#define CI_TickLocalPrintf(...)
#endif


/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

char* IBN_FileSystemErrorText (u8 FS_Error_u8);
void ushell_path_valid_syntac (char* path);

extern Ctrl_status sd_mmc_mci_unit_state_e;

/*******************************************************************************

 Local declarations

*******************************************************************************/

// To manage a file system shortcut
static Fs_index FAI_MarkIndex;

/*******************************************************************************

  FAI_Init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FAI_Init (void)
{
    return (TRUE);
}

/*******************************************************************************

  FAI_InitLun

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FAI_InitLun (u8 Lun_u8)
{
u8 Ret_u8;

    b_fsaccess_init ();

    fs_g_nav.u8_lun = Lun_u8;   // 0 = On chip RAM

#if LUN_2 == ENABLE
    virtual_test_unit_ready ();
#endif

#if LUN_0 == ENABLE
    sd_mmc_mci_test_unit_ready_1 ();    // For linux
#endif

    Ret_u8 = fat_mount ();

#ifdef FAI_DEBUG_PRINT
    if (TRUE == Ret_u8)
    {
        CI_TickLocalPrintf ("Mount LUN %d OK\n\r", Lun_u8);
    }
    else
    {
        CI_TickLocalPrintf ("Mount LUN %d FAIL - %d - %s\n\r", Lun_u8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
    }
#endif // FAI_DEBUG_PRINT
    return (Ret_u8);
}

/*******************************************************************************

  FAI_MarkLunAsChanged

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


/*******************************************************************************

  FAI_MarkLunAsChanged

*******************************************************************************/

u8 FAI_MarkLunAsChanged (u8 Lun_u8)
{
    switch (Lun_u8)
    {
#if LUN_2 == ENABLE
        case 1:
            virtual_unit_state_e = CTRL_BUSY;
            CI_LocalPrintf ("LUN %d CTRL_BUSY\n\r", Lun_u8);
            break;
#endif
        case 0:
            sd_mmc_mci_unit_state_e = CTRL_BUSY;
            CI_LocalPrintf ("LUN %d CTRL_BUSY\n\r", Lun_u8);
            break;
        case 2:
            sd_mmc_mci_unit_state_e = CTRL_NO_PRESENT;
            CI_LocalPrintf ("LUN %d CTRL_NO_PRESENT\n\r", Lun_u8);
            break;

    }

    vTaskDelay (1000);

    switch (Lun_u8)
    {
#if LUN_2 == ENABLE
        case 1:
            virtual_unit_state_e = CTRL_GOOD;
            break;
#endif
        case 0:
        case 2:
            sd_mmc_mci_unit_state_e = CTRL_GOOD;
            break;
    }
    return (TRUE);
}

/*******************************************************************************

  FAI_Write

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FAI_Write (u8 Lun_u8)
{
s32 FileID_s32;
s32 Ret_s32;

    if (FALSE == FAI_InitLun (Lun_u8))
    {
        CI_LocalPrintf ("InitLun  = Lun %d - %d - %s\n\r", Lun_u8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    FileID_s32 = open ("test.txt", O_CREAT | O_RDWR);
    if (0 > FileID_s32)
    {
        CI_LocalPrintf ("Open  = %d - %d - %s\n\r", FileID_s32, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    Ret_s32 = write (FileID_s32, "Test\n", 6);
    if (0 != Ret_s32)
    {
        CI_LocalPrintf ("Write  = %d - %d - %s\n\r", Ret_s32, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        close (FileID_s32);
        return (FALSE);
    }

    Ret_s32 = close (FileID_s32);
    if (0 != Ret_s32)
    {
        CI_LocalPrintf ("Close  = %d - %d - %s\n\r", Ret_s32, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }

    FAI_MarkLunAsChanged (Lun_u8);

    return (TRUE);
}

/*******************************************************************************

  FAI_PrintDirEntry

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void FAI_PrintDirEntry (void)
{
    PTR_CACHE ptr_entry;

    ptr_entry = fat_get_ptr_entry ();

    if (FS_ENTRY_END == *ptr_entry) // end of directory
    {
        fs_g_status = FS_ERR_ENTRY_EMPTY;
        return;
    }

    fs_g_status = FS_ERR_ENTRY_BAD; // by default this entry is different then bad

    CI_LocalPrintf ("%-11.11s", ptr_entry);

    if (FS_ENTRY_DEL == *ptr_entry)
    {
        CI_LocalPrintf (" DEL");
    }
    else if (FS_ATTR_LFN_ENTRY == ptr_entry[11])
    {
        CI_LocalPrintf (" LON");
    }
    else
    {
        CI_LocalPrintf ("    ");
    }
    CI_LocalPrintf ("\n\r");
}

/*******************************************************************************

  FAI_ShowDirContent

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FAI_ShowDirContent (void)
{
    u8 u8_nb;

    u8_nb = 0;
    while (1)
    {
        if (0xFF == u8_nb)
        {
            return 0;   // All short name exist
        }
        u8_nb++;    // Try next short name

        fs_g_nav_fast.u16_entry_pos_sel_file = 0;   // Go to beginning of directory

        // Scan directory
        while (1)
        {
            if (FALSE == fat_read_dir ())   // Read directory
            {
                if (FS_ERR_OUT_LIST == fs_g_status)
                {
                    return u8_nb;   // short name don't exist, then good number
                }
                return 0;   // System or Disk Error
            }

            FAI_PrintDirEntry ();

            if (FS_ERR_ENTRY_EMPTY == fs_g_status)
            {
                return u8_nb;   // Short name don't exist, then good number
            }

            fs_g_nav_fast.u16_entry_pos_sel_file++; // Go to next entry
        }
    }


}

/*******************************************************************************

  FAI_mount

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FAI_mount (u8 Lun_u8)
{
U8 drive_lun_u8;
Fs_index sav_index;

    // Compute the logical unit number of drive
    drive_lun_u8 = Lun_u8;

    // Check lun number
    if (drive_lun_u8 >= nav_drive_nb ())
    {
        CI_LocalPrintf ("FAI_mount: Lun %d not found\n\r", Lun_u8);
        return (FALSE);
    }

    // Mount drive
    sav_index = nav_getindex ();    // Save previous position
    if (nav_drive_set (drive_lun_u8))
    {
        if (nav_partition_mount ())
        {
            return (TRUE);  // Here, drive mounted
        }
    }

    CI_LocalPrintf ("FAI_mount: Lun %d not mountable\n\r", Lun_u8);
    nav_gotoindex (&sav_index); // Restore previous position

    return (FALSE);
}

/*******************************************************************************

  FAI_free_space

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void FAI_free_space (u8 Param_u8)
{
u8 u8_tmp;
Fs_index sav_index = nav_getindex ();   // Save current position

    for (u8_tmp = 0; u8_tmp < nav_drive_nb (); u8_tmp++)
    {
        nav_drive_set (u8_tmp); // Select drive
        if (!nav_partition_mount ())    // Mount drive
            continue;

        // Display drive letter name (a, b...)
        CI_LocalPrintf ("Lun %d: %s\r\n", u8_tmp, mem_name (u8_tmp));
        if (0 != Param_u8)  // Choose command option
        {
            // Long and exact fonction
            CI_LocalPrintf ("Free space: %llu Bytes / %llu Bytes\n\r",
                            (U64) (nav_partition_freespace () << FS_SHIFT_B_TO_SECTOR), (U64) (nav_partition_space () << FS_SHIFT_B_TO_SECTOR));
        }
        else
        {
            // Otherwise use fast command
            CI_LocalPrintf ("Free space: %u %%\n\r", nav_partition_freespace_percent ());
        }
    }
    nav_gotoindex (&sav_index); // Restore position
}


/*******************************************************************************

  FAI_nb_drive

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void FAI_nb_drive (void)
{
    u8 tmp_u8;

    CI_LocalPrintf ("Memory interface available:\r\n");
    for (tmp_u8 = 0; tmp_u8 < nav_drive_nb (); tmp_u8++)
    {
        CI_LocalPrintf ("Lun %d: %s\r\n", tmp_u8, mem_name (tmp_u8));
    }
}

/*******************************************************************************

  FAI_Dir

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


void FAI_Dir (u8 more_u8)
{
u8 str_char[FAI_MAX_FILE_PATH_LENGTH];
u16 u16_nb_file, u16_nb_dir, last_i;
u8 ext_filter = FALSE;

    // ** Print drive name
    CI_LocalPrintf ("Lun %d: volume is %s\r\n", nav_drive_get (), mem_name (nav_drive_get ()));
    CI_LocalPrintf ("Lun uses ");
    switch (nav_partition_type ())
    {
        case FS_TYPE_FAT_12:
            CI_LocalPrintf ("FAT12\n\r");
            break;

        case FS_TYPE_FAT_16:
            CI_LocalPrintf ("FAT16\n\r");
            break;

        case FS_TYPE_FAT_32:
            CI_LocalPrintf ("FAT32\n\r");
            break;

        default:
            CI_LocalPrintf ("an unknown partition type\r\n");
            return;
    }

    // ** Print directory name
    if (!nav_dir_name ((FS_STRING) str_char, FAI_MAX_FILE_PATH_LENGTH))
    {
        return;
    }

    CI_LocalPrintf ("Dir name is %s\n\r", str_char);

    // Print files list
    CI_LocalPrintf ("          Size  Name\n\r");

    // Init loop at the begining of directory
    nav_filelist_reset ();
    u16_nb_file = 0;
    u16_nb_dir = 0;
    last_i = 0;
    // For each file in list
    while (nav_filelist_set (0, FS_FIND_NEXT))
    {
        if (!ext_filter)
        {
            // No extension filter
            if (nav_file_isdir ())
            {
                CI_LocalPrintf ("Dir ");
                u16_nb_dir++;   // count the number of directory
            }
            else
            {
                CI_LocalPrintf ("    ");
            }
        }
        else
        {
            // If extension filter then ignore directories
            if (nav_file_isdir ())
                continue;
        }
        u16_nb_file++;  // count the total of files (directories and files)

        // Display file
        nav_file_name ((FS_STRING) str_char, MAX_FILE_PATH_LENGTH, FS_NAME_GET, TRUE);
        CI_LocalPrintf ("%10lu  %-30.30s", nav_file_lgt (), str_char);

        nav_file_dateget ((FS_STRING) str_char, FS_DATE_LAST_WRITE);
        CI_LocalPrintf ("  %s\n\r", str_char);
    }

    // Display total number
    CI_LocalPrintf (" %4i Files\r\n", u16_nb_file - u16_nb_dir);
    CI_LocalPrintf (" %4i Dir\r\n", u16_nb_dir);
}

/*******************************************************************************

  FAI_cd

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FAI_cd (u8 * Path_pu8)
{
    if (NULL == Path_pu8)
    {
        return (TRUE);
    }

    // Add '\' at the end of path, else the nav_setcwd select the directory but don't enter into.
    ushell_path_valid_syntac ((char *) Path_pu8);

    if (0 == strcmp ((char *) Path_pu8, "..\\"))
    {
        nav_dir_gotoparent ();
        return (TRUE);
    }

    CI_LocalPrintf ("Change to %s\n\r", Path_pu8);

    // Call file system routine
    if (FALSE == nav_setcwd ((FS_STRING) Path_pu8, TRUE, FALSE))
    {
        CI_LocalPrintf ("CD - Can't change to %s - %d - %s\n\r", Path_pu8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }
    return (TRUE);
}

/*******************************************************************************

  FAI_touch

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


void FAI_touch (u8 * Name_pu8)
{
    if (NULL == Name_pu8)
    {
        return;
    }

    if (FALSE == nav_file_create ((FS_STRING) Name_pu8))
    {
        CI_LocalPrintf ("Can't touch %s - %d - %s\n\r", Name_pu8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
    }
}


/*******************************************************************************

  FAI_mkdir

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 FAI_mkdir (u8 * Name_pu8)
{
    if (NULL == Name_pu8)
    {
        CI_LocalPrintf ("Can't mkdir, no name\n\r");
        return (FALSE);
    }

    if (FALSE == nav_dir_make ((FS_STRING) Name_pu8))
    {
        CI_LocalPrintf ("Can't mkdir %s - %d - %s\n\r", Name_pu8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return (FALSE);
    }
    return (TRUE);
}

/*******************************************************************************

  FAI_cat

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void FAI_cat (u8 * Name_pu8, u8 More_u8)
{
u8 Char_u8;

    if (NULL == Name_pu8)
    {
        return;
    }

    // Select file
    if (!nav_setcwd ((FS_STRING) Name_pu8, TRUE, FALSE))
    {
        CI_LocalPrintf ("Can't open %s - %d - %s\n\r", Name_pu8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
        return;
    }

    // Open file
    file_open (FOPEN_MODE_R);
    while (file_eof () == FALSE)
    {
        // Display a character
        Char_u8 = file_getc ();
        CI_LocalPrintf ("%c", Char_u8);
    }
    file_close ();
}

/*******************************************************************************

  FAI_UpdateContend

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void FAI_UpdateContend (u8 Lun_u8)
{
Fs_index sav_index = nav_getindex ();   // Save current position

    nav_drive_set (1);  // Select drive
    nav_drive_set (0);  // Select drive
    nav_drive_set (Lun_u8); // Select drive
    nav_gotoindex (&sav_index); // Restore position
}

/*******************************************************************************

  FAI_DeleteFile

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void FAI_DeleteFile (u8 * Filename_pu8)
{
u8 i = 0;
Fs_index sav_index;

    if (Filename_pu8[0] == 0)
    {
        return;
    }

    // Save the position
    sav_index = nav_getindex ();

    while (1)
    {
        // Restore the position
        nav_gotoindex (&sav_index);

        // Select file or directory
        if (FALSE == nav_setcwd ((FS_STRING) Filename_pu8, TRUE, FALSE))
        {
            // CI_LocalPrintf ("Can't select %s - %d - %s\n\r",Filename_pu8,fs_g_status, IBN_FileSystemErrorText(fs_g_status));
            break;
        }

        // Delete file or directory
        if (FALSE == nav_file_del (FALSE))
        {
            CI_LocalPrintf ("Can't delete %s - %d - %s\n\r", Filename_pu8, fs_g_status, IBN_FileSystemErrorText (fs_g_status));
            break;
        }
        i++;
    }
    CI_TickLocalPrintf ("FAI_DeleteFile: %d files deleted\n\r", i);


}

/*******************************************************************************

  FAI_CopyFile

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u32 FAI_CopyFile (u8 * SourceFilename_pu8, u8 * DestFilename_pu8)
{
Fs_index sav_index;
U8 CopyStatus_u8;

    if (0 == SourceFilename_pu8)
    {
        return (FALSE);
    }

    if (0 == DestFilename_pu8)
    {
        return (FALSE);
    }

    // Save the position
    sav_index = nav_getindex ();

    // Select source file
    if (!nav_setcwd ((FS_STRING) SourceFilename_pu8, TRUE, FALSE))
    {
        CI_TickLocalPrintf ("FAI_CopyFile: -%s- not found\n\r", SourceFilename_pu8);
        return (FALSE);
    }


    // Get name of source to be used as same destination name
    nav_file_name ((FS_STRING) SourceFilename_pu8, MAX_FILE_PATH_LENGTH, FS_NAME_GET, TRUE);

    // Mark this selected file like source file
    if (FALSE == nav_file_copy ())
    {
        CI_TickLocalPrintf ("FAI_CopyFile: Can't select %s for copy\n\r", SourceFilename_pu8);
        goto cp_end;
    }

    // Select destination
    if (0 == DestFilename_pu8)
    {
        if (FALSE == nav_gotoindex (&FAI_MarkIndex))
        {
            goto cp_end;
        }
    }
    else
    {
        // g_s_arg[1] exists, then go to this destination
        if (FALSE != nav_setcwd ((FS_STRING) DestFilename_pu8, TRUE, FALSE))
        {
            CI_TickLocalPrintf ("FAI_CopyFile: %s found\n\r", DestFilename_pu8);
            goto cp_end;
        }
    }

    // Set the name destination and start paste
    if (!nav_file_paste_start ((FS_STRING) DestFilename_pu8))
    {
        CI_TickLocalPrintf ("FAI_CopyFile: Fails\n\r");
        goto cp_end;
    }

    // Performs copy
    do
    {
        CopyStatus_u8 = nav_file_paste_state (FALSE);
    } while (CopyStatus_u8 == COPY_BUSY);

    // Check status of copy action
    if (CopyStatus_u8 == COPY_FAIL)
    {
        CI_TickLocalPrintf ("FAI_CopyFile: Fails (1)\n\r");
        goto cp_end;
    }

  cp_end:
    // Restore the position
    nav_gotoindex (&sav_index);

    return (TRUE);
}
