/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-07-05
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
 * MatrixPassword.c
 *
 *  Created on: 05.07.2013
 *      Author: RB
 */

#include "compiler.h"
#include "preprocessor.h"
#include "board.h"
#include "gpio.h"
#include "flashc.h"
#include "string.h"


#include "global.h"
#include "tools.h"
#include "TIME_MEASURING.h"

#include "OTP/report_protocol.h"
#include "FlashStorage.h"

#include "MatrixPassword.h"

/*******************************************************************************

 Local defines

*******************************************************************************/

#ifdef INTERPRETER_ENABLE
  #define DEBUG_MP_IO
//#define DEBUG_MP_IO_LEVEL_1   // Print matrix
//#define DEBUG_MP_IO_LEVEL_2   // Error messages
#endif

#ifdef DEBUG_MP_IO
  int CI_LocalPrintf (char *szFormat,...);
  int CI_TickLocalPrintf (char *szFormat,...);
#else
  #define CI_LocalPrintf(...)
  #define CI_TickLocalPrintf(...)
  #define CI_StringOut(...)
  #define CI_Print8BitValue(...)
  #define HexPrint(...)
#endif

#define PWM_RAMDOMCHAR_BUFFERSIZE   97     //

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

u32 GetRandomNumber_u32 (u32 Size_u32,u8 *Data_pu8);
u8 LA_RestartSmartcard_u8 (void);

/*******************************************************************************

 Local declarations

*******************************************************************************/

u8 PWM_RandomcharsPointer_u8= 0;
u8 PWM_Randomchars_au8[PWM_RAMDOMCHAR_BUFFERSIZE+1];
u8 PWM_PasswordMatrix_Filled_u8 = FALSE;
s8 PWM_PasswordMatrix_as8[10][10];
u8 PWM_UsedNumber_au8[10];


/*******************************************************************************

  PWM_PrintMatrix

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_PrintMatrix (void)
{
  u32 x,y;

  CI_LocalPrintf ("\r\n");

  for (y=0;y<10;y++)
  {
    for (x=0;x<10;x++)
    {
      if (-1 != PWM_PasswordMatrix_as8[x][y])
      {
        CI_LocalPrintf ("%2d",PWM_PasswordMatrix_as8[x][y]);
      }
      else
      {
        CI_LocalPrintf (" .");
      }
    }
    CI_LocalPrintf ("\r\n");
  }
  CI_LocalPrintf ("\r\n");

  DelayMs (100);

  return (TRUE);
}

/*******************************************************************************

  PWM_InitRandomChar

  Get only 100 random numbers

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_InitRandomChar (void)
{
  u8 i;
  static u8 InitCalls_u8 = 200;   // First call generates random numbers

//
  InitCalls_u8++;
  if (100 > InitCalls_u8)
  {
    return (TRUE);
  }
  InitCalls_u8 = 0;

//  LA_RestartSmartcard_u8 ();    // Todo lock transcation

  if (FALSE == GetRandomNumber_u32 (PWM_RAMDOMCHAR_BUFFERSIZE,PWM_Randomchars_au8))
  {
    CI_LocalPrintf ("GetRandomNumber_u32: Failed to get random numbers from smartcard\r\n");

    // Get local random numbers
    for (i=0;i<PWM_RAMDOMCHAR_BUFFERSIZE;i++)
    {
      PWM_Randomchars_au8[i] = rand () % 10;
    }
  }
  else
  {
    for (i=0;i<PWM_RAMDOMCHAR_BUFFERSIZE;i++)
    {
      PWM_Randomchars_au8[i] = PWM_Randomchars_au8[i] % 10; // Todo: for testing no symmetric random numbers
    }
  }

  PWM_RandomcharsPointer_u8 = 0;

//  LA_RestartSmartcard_u8 ();
  ISO7816_SetLockCounter (0);
  CCID_SmartcardOff_u8 ();          // To avoid smartcard error - after GetRandomNumber_u32

  return (TRUE);
}

/*******************************************************************************

  PWM_InitGenerateMatrix

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_InitGenerateMatrix (void)
{
  u8 i,i1;

// Mark matrix as unfilled
  PWM_PasswordMatrix_Filled_u8 = FALSE;

// Get random numbers
  if (FALSE == PWM_InitRandomChar ())
  {
    return (FALSE);
  }

// Clear used number array
  for (i=0;i<10;i++)
  {
    PWM_UsedNumber_au8[i] = FALSE;
  }

// Clear matrix
  for (i=0;i<10;i++)
  {
    for (i1=0;i1<10;i1++)
    {
      PWM_PasswordMatrix_as8[i][i1] = -1;
    }
  }

  return (TRUE);
}

/*******************************************************************************

  PWM_GetRandomChar

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_GetRandomChar (void)
{
  u8 randomChar_u8;

  randomChar_u8 = PWM_Randomchars_au8[PWM_RandomcharsPointer_u8];

  PWM_RandomcharsPointer_u8++;

  if (PWM_RAMDOMCHAR_BUFFERSIZE <= PWM_RandomcharsPointer_u8)
  {
    PWM_RandomcharsPointer_u8 = 0;
  }

  return (randomChar_u8);
}
/*******************************************************************************

  PWM_PositionValid

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_PositionValid (u8 Number_u8,u8 Pos_x_u8,u8 Pos_y_u8)
{
    u8 i;

// Is the place free
  if (PWM_PasswordMatrix_as8[Pos_x_u8][Pos_y_u8] != -1)
  {
    // No, return false
    return (FALSE);
  }

// check the row , x
    for (i=0;i<10;i++)
    {
      // Don't check the number
      if (i != Pos_x_u8)
      {
        // Is number in the row ?
        if (PWM_PasswordMatrix_as8[i][Pos_y_u8] == Number_u8)
        {
          // Yes, return false
          return (FALSE);
        }
      }
    }

// check the column , y
    for (i=0;i<10;i++)
    {
      // Don't check the number
      if (i != Pos_y_u8)
      {
        // Is number in the column ?
        if (PWM_PasswordMatrix_as8[Pos_x_u8][i] == Number_u8)
        {
          // Yes, return false
          return (FALSE);
        }
      }
    }
    return (TRUE);
}
/*******************************************************************************

  PWM_GetUnusedNumber

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

s8 PWM_GetUnusedNumber (void)
{
  u8 Flag_u8;
  u8 Number_u8;

  // Get the random number
  Number_u8 = PWM_GetRandomChar ();

  // Check for a unused number, if not take the next free one
  Flag_u8 = 0;
  while (TRUE == PWM_UsedNumber_au8[Number_u8])
  {
    Number_u8++;
    if (10 <= Number_u8)
    {
      Number_u8 = 0;
      if (1 == Flag_u8)      // Paranoia
      {
        return (-1);
      }
      Flag_u8 = 1;
    }
  }

  // Set number as used
  PWM_UsedNumber_au8[Number_u8] = TRUE;

  return (Number_u8);
}
/*******************************************************************************

  PWM_GetSwapLineX

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

s8 PWM_GetSwapLineX (u8 Number_u8,u8 YLine_u8)
{
  u8 XPos_u8;
  u8 i;

  // Get a used place where Number_u8 could placed
  for (XPos_u8=0;XPos_u8<10;XPos_u8++)
  {
    // Is the place used
    if (PWM_PasswordMatrix_as8[XPos_u8][YLine_u8] == -1)
    {
      continue;   // No, goto next place
    }

    // Check for Number_u8 in line
    for (i=0;i<10;i++)
    {
      if (PWM_PasswordMatrix_as8[XPos_u8][i] == Number_u8)
      {
         break;   // Number_u8 is in line, the line couln't used
      }
    }
    if (i != 10)
    {
      continue;   // When Number_u8 was found check the next line
    }

    // Line for swapping found
    return (XPos_u8);
  }

  // No line for swapping found
  return (-1);
}
/*******************************************************************************

  PWM_SwapNumber

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_SwapNumber (u8 SwapPosX_u8,u8 BaseLineY_u8)
{
  u8 SwapNumber_u8;
  u8 PosY_u8;
  u8 SwapX2ndPos_u8;
  u8 Save_u8;

  SwapNumber_u8  = PWM_PasswordMatrix_as8[SwapPosX_u8][BaseLineY_u8];

  for (PosY_u8=0;PosY_u8<10;PosY_u8++)
  {
    // If the y pos the baseline the goto next y pos
    if (PosY_u8 == BaseLineY_u8)
    {
      continue;
    }

    // Is the place used, goto next y pos
    if (PWM_PasswordMatrix_as8[SwapPosX_u8][PosY_u8] != -1)
    {
      continue;
    }

    // We found a pos to swap
    // Find the x pos to swap
    for (SwapX2ndPos_u8=0;SwapX2ndPos_u8<10;SwapX2ndPos_u8++)
    {
      // Is the place not used, goto next y pos
      if (PWM_PasswordMatrix_as8[SwapX2ndPos_u8][PosY_u8] != SwapNumber_u8)
      {
        continue;
      }
      // Now check the BaseLineY_u8 for an unused place
      if (-1 == PWM_PasswordMatrix_as8[SwapX2ndPos_u8][BaseLineY_u8])
      {
#ifdef DEBUG_MP_IO_LEVEL_2
        CI_LocalPrintf ("\r\nSwap X1 %d  Y1 %d - X2 %d Y2 %d\r\n",SwapPosX_u8,BaseLineY_u8,SwapX2ndPos_u8,PosY_u8);
#endif
        // Now we can swap
        // Swap the baseline
        PWM_PasswordMatrix_as8[SwapX2ndPos_u8][BaseLineY_u8] = SwapNumber_u8;
        PWM_PasswordMatrix_as8[SwapPosX_u8][BaseLineY_u8]    = -1;

        // Swap the PosY_u8 line
        Save_u8 = PWM_PasswordMatrix_as8[SwapX2ndPos_u8][PosY_u8];
        PWM_PasswordMatrix_as8[SwapX2ndPos_u8][PosY_u8] = PWM_PasswordMatrix_as8[SwapPosX_u8][PosY_u8];
        PWM_PasswordMatrix_as8[SwapPosX_u8][PosY_u8]    = Save_u8;
        return (TRUE);
      }
    }
  }
  return (FALSE);
}
/*******************************************************************************

  PWM_SwapNumberInMatrix

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

s8 PWM_SwapNumberInMatrix (u8 Number_u8,u8 YLine_u8)
{
  s8 XPos_s8;

#ifdef DEBUG_MP_IO_LEVEL_1
  CI_LocalPrintf ("\r\nBefore swap\r\n\r\n");
  PWM_PrintMatrix ();
#endif

// Get swap line
  XPos_s8 = PWM_GetSwapLineX (Number_u8,YLine_u8);

  if (-1 == XPos_s8)
  {
#ifdef DEBUG_MP_IO_LEVEL_1
    CI_LocalPrintf ("\r\swap fail: PWM_GetSwapLineX\r\n\r\n");
    PWM_PrintMatrix ();
#endif
    return (-1);
  }
  else
  {
#ifdef DEBUG_MP_IO_LEVEL_2
    CI_LocalPrintf ("\r\swap: XPos_s8 %d - YLine_u8 %d\r\n\r\n",XPos_s8,YLine_u8);
#endif
  }

// Swap it
  if (FALSE == PWM_SwapNumber (XPos_s8,YLine_u8))
  {
#ifdef DEBUG_MP_IO_LEVEL_2
    CI_LocalPrintf ("\r\swap fail: PWM_SwapNumber\r\n\r\n");
#endif
#ifdef DEBUG_MP_IO_LEVEL_1
    PWM_PrintMatrix ();
#endif
    return (-1);
  }
#ifdef DEBUG_MP_IO_LEVEL_1
  CI_LocalPrintf ("\r\nAfter swap\r\n\r\n");
  PWM_PrintMatrix ();
#endif
  return (XPos_s8);
}
/*******************************************************************************

  PWM_InsetNumberInMatrix

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_InsetNumberInMatrix (u8 Number_u8)
{
  u8 YPos_u8;
  u8 XPos_u8;
  u8 i;
  u8 Ret_u8;

  for (YPos_u8 = 0;YPos_u8 < 10;YPos_u8++)
  {
    // Get a random position of the number
    XPos_u8 = PWM_GetRandomChar ();

    for (i=0;i<10;i++)
    {
      Ret_u8 = PWM_PositionValid (Number_u8,XPos_u8,YPos_u8);
      if (FALSE == Ret_u8)
      {
        XPos_u8++;    // Try the next pos
        if (10 <= XPos_u8)
        {
          XPos_u8 = 0;
        }
      }
      else
      {
        break;
      }
    }

    if (10 == i)
    {
#ifdef DEBUG_MP_IO_LEVEL_2
      CI_LocalPrintf ("\r\nError: Number_u8 %d - XPos_u8 %d - YPos_u8 %d\r\n\r\n",Number_u8,XPos_u8,YPos_u8);
#endif
      XPos_u8 = PWM_SwapNumberInMatrix (Number_u8,YPos_u8);
      if (-1 == XPos_u8)
      {
        return (FALSE); // Can't swap
      }
    }

    PWM_PasswordMatrix_as8[XPos_u8][YPos_u8] = Number_u8;
  }
  return (TRUE);
}
/*******************************************************************************

  PWM_GenerateMatrix

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_GenerateMatrix (void)
{
  u8 i;
  s8 Number_s8;


  if (FALSE == PWM_InitGenerateMatrix ())
  {
    return (FALSE);
  }

  for (i=0;i<10;i++)
  {
    // Get the next number
    Number_s8 = PWM_GetUnusedNumber ();
    if (-1 == Number_s8)
    {
      return (FALSE);
    }

    // Insert number in matrix
    if (FALSE == PWM_InsetNumberInMatrix (Number_s8))
    {
#ifdef DEBUG_MP_IO_LEVEL_1
      PWM_PrintMatrix ();
#endif
      return (FALSE);
    }
#ifdef DEBUG_MP_IO_LEVEL_1
    PWM_PrintMatrix ();
#endif
  }

  return (TRUE);
}
/*******************************************************************************

  PWM_CheckMatrix

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 PWM_CheckMatrix (void)
{
  u8 PosX_u8;
  u8 PosY_u8;
  u8 Save_u8;
  u8 Flag_au8[10];
  u8 Ret_u8;
  u8 CheckOK_u8;
  u8 i;

  CheckOK_u8 = TRUE;


// Check for x lines
  for (PosY_u8=0;PosY_u8<10;PosY_u8++)
  {
    for (i=0;i<10;i++)
    {
      Flag_au8[i] = 0;
    }
    for (PosX_u8=0;PosX_u8<10;PosX_u8++)
    {
      if (-1 != PWM_PasswordMatrix_as8[PosX_u8][PosY_u8])
      {
        Flag_au8[PWM_PasswordMatrix_as8[PosX_u8][PosY_u8]] = 1;
      }
    }
    for (i=0;i<10;i++)
    {
      if (0 == Flag_au8[i])
      {
#ifdef DEBUG_MP_IO_LEVEL_2
        CI_LocalPrintf ("MatrixError: Line x %d Number %d is missung\r\n",PosY_u8,i);
#endif
        CheckOK_u8 = FALSE;
      }
    }
  }

// Check y line for all numbers
  for (PosX_u8=0;PosX_u8<10;PosX_u8++)
  {
    for (i=0;i<10;i++)
    {
      Flag_au8[i] = 0;
    }
    for (PosY_u8=0;PosY_u8<10;PosY_u8++)
    {
      if (-1 != PWM_PasswordMatrix_as8[PosX_u8][PosY_u8])
      {
        Flag_au8[PWM_PasswordMatrix_as8[PosX_u8][PosY_u8]] = 1;
      }
    }
    for (i=0;i<10;i++)
    {
      if (0 == Flag_au8[i])
      {
#ifdef DEBUG_MP_IO_LEVEL_2
        CI_LocalPrintf ("nMatrixError: Line y %d Number %d is missung\r\n",PosX_u8,i);
#endif
        CheckOK_u8 = FALSE;
      }
    }
  }

// Check x line for no doubles
  for (PosY_u8=0;PosY_u8<10;PosY_u8++)
  {
    for (PosX_u8=0;PosX_u8<10;PosX_u8++)
    {
      Save_u8 = PWM_PasswordMatrix_as8[PosX_u8][PosY_u8];

      PWM_PasswordMatrix_as8[PosX_u8][PosY_u8] = -1;

      Ret_u8 = PWM_PositionValid (Save_u8,PosX_u8,PosY_u8);

      PWM_PasswordMatrix_as8[PosX_u8][PosY_u8] = Save_u8;

      if (FALSE == Ret_u8)
      {
#ifdef DEBUG_MP_IO_LEVEL_2
        CI_LocalPrintf ("Matrix Error: Number_u8 %d - XPos_u8 %d - YPos_u8 %d\r\n",Save_u8,PosX_u8,PosY_u8);
#endif
        CheckOK_u8 = FALSE;
      }

    }
  }
  return (CheckOK_u8);
}
/*******************************************************************************

  GetCorrectMatrix

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 *GetCorrectMatrix (void)
{
  u8 Ret_u8;
  u16 i;

  i      = 0;
  Ret_u8 = FALSE;

  while (FALSE == Ret_u8)
  {
    i++;
    Ret_u8 = PWM_GenerateMatrix ();
    if (TRUE == Ret_u8)
    {
      Ret_u8 = PWM_CheckMatrix ();
    }
  }

  PWM_PasswordMatrix_Filled_u8 = TRUE;

  PWM_PrintMatrix ();

#ifdef DEBUG_MP_IO_LEVEL_1
  CI_LocalPrintf ("GetCorrectMatrix: Get correct matrix tries %d times\r\n",i);
#endif

  return ((u8*)PWM_PasswordMatrix_as8);
}

/*******************************************************************************

  MatrixTest1

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 MatrixTest1  (void)
{
  u8 Ret_u8;
  u16 i;
  u16 n;
  u8  n1;
  u8  Foundcolumn_u8;
  u8  PasswordChar_u8;
  u8  PasswordLine_u8;
  u8  AddLine_u8;


  PasswordChar_u8 = 0;
  PasswordLine_u8 = 0;
  AddLine_u8      = 9;


  for (n=0;n<100;n++)
  {
    GetCorrectMatrix ();

    for (i=0;i<10;i++)
    {
      n1 = PWM_PasswordMatrix_as8[i][PasswordLine_u8];
      if (PasswordChar_u8 == n1)
      {
        Foundcolumn_u8 = i;
        break;
      }
    }

    Foundcolumn_u8 += PWM_PasswordMatrix_as8[AddLine_u8][0];
    if (10 <= Foundcolumn_u8)
    {
      Foundcolumn_u8 -= 10;
    }

    CI_LocalPrintf ("Char entered : %d\r\n",Foundcolumn_u8);
    CI_LocalPrintf ("-------------------------------------\r\n");
  }

  return (TRUE);
}


/*******************************************************************************

  GetCorrectMatrixTest

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

u8 GetCorrectMatrixTest  (void)
{
  u8 Ret_u8;
  u16 i;
  u16 n;

  i      = 0;

  for (n=0;n<100;n++)
  {
    Ret_u8 = FALSE;
    while (FALSE == Ret_u8)
    {
      i++;
      Ret_u8 = PWM_GenerateMatrix ();
      if (TRUE == Ret_u8)
      {
        Ret_u8 = PWM_CheckMatrix ();
        if (TRUE == Ret_u8)
        {
          CI_LocalPrintf ("+");
        }
        else
        {
          CI_LocalPrintf ("-");
        }
      }
    }
  }
  CI_LocalPrintf ("\r\nGetCorrectMatrix: Get correct %d matrix call %d times\r\n",n,i);
  return (TRUE);
}
/*******************************************************************************

  ConvertMatrixDataToPassword

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
#define MATRIX_PASSWORD_MAX_LEN   20

u8 ConvertMatrixDataToPassword(u8 *MatrixData_au8)
{
  u8 i;
  u8 n;
  u8 x,y;
  u8 MatrixColumsUserPW_au8[MATRIX_PASSWORD_MAX_LEN];

  if (FALSE == PWM_PasswordMatrix_Filled_u8)
  {
    return (FALSE);
  }

  n = strlen ((char*)MatrixData_au8);

  if ((MATRIX_PASSWORD_MAX_LEN - 1) <= n)   // Boundary check
  {
    return (FALSE);
  }

  ReadMatrixColumsUserPWFromUserPage (MatrixColumsUserPW_au8);


#ifdef DEBUG_MP_IO
  CI_LocalPrintf ("Get matrix col  : ");
  HexPrint (n,MatrixData_au8);
  CI_LocalPrintf ("\r\n");

  CI_LocalPrintf ("Get matrix row  : ");
  HexPrint (n,MatrixColumsUserPW_au8);
  CI_LocalPrintf ("\r\n");

  PWM_PrintMatrix ();
  CI_LocalPrintf ("\r\n");
#endif

  ReadMatrixColumsUserPWFromUserPage (MatrixColumsUserPW_au8);

  for (i=0;i<n;i++)
  {
    y = MatrixColumsUserPW_au8[i] - '0';
    x = MatrixData_au8[i]         - '0';
    MatrixData_au8[i] = PWM_PasswordMatrix_as8[x][y] + '0';
  }

#ifdef DEBUG_MP_IO
  CI_LocalPrintf ("Real password  : ");
  HexPrint (n,MatrixData_au8);
  CI_LocalPrintf ("\r\n");
#endif
  return (TRUE);
}

/*******************************************************************************

  IBN_PWM_Tests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#ifdef ENABLE_IBN_PWM_TESTS

void IBN_PWM_Tests (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8)
{
#ifdef TIME_MEASURING_ENABLE
  u32 Runtime_u32;
#endif
  if (0 == nParamsGet_u8)
  {
    CI_LocalPrintf ("Password matrix test functions\r\n");
    CI_LocalPrintf ("\r\n");
    CI_LocalPrintf ("0   Generate matrix\r\n");
    CI_LocalPrintf ("1   Get Correct matrix test\r\n");
    CI_LocalPrintf ("\r\n");
    return;
  }

  switch (CMD_u8)
  {
    case 0:
      CI_LocalPrintf ("Create matrix : \r\n");
      DelayMs (100);    // For debugging

#ifdef TIME_MEASURING_ENABLE
      TIME_MEASURING_Start (TIME_MEASURING_TIME_15);
#endif

// Work
      if (FALSE == PWM_GenerateMatrix ())
      {
        CI_LocalPrintf ("\r\nCreate matrix fails\r\n");
      }

#ifdef TIME_MEASURING_ENABLE
      Runtime_u32 = TIME_MEASURING_Stop (TIME_MEASURING_TIME_15);
      CI_LocalPrintf ("Runtime %ld usec\n\r",Runtime_u32/TIME_MEASURING_TICKS_IN_USEC);
#endif

      if (FALSE == PWM_CheckMatrix ())
      {
        CI_LocalPrintf ("\r\nCheck matrix fails\r\n");
      }
      else
      {
        CI_LocalPrintf ("\r\nCheck matrix ok\r\n");
      }
      break;

    case 1:
      GetCorrectMatrixTest ();
      break;

    case 2 :
      MatrixTest1();
      break;

    case 3 :
      break;
  }
}
#endif
