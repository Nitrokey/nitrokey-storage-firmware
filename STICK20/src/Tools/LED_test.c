/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 13.07.2012
*
* This file is part of GPF Crypto Stick 2
*
* GPF Crypto Stick 2  is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* GPF Crypto Stick is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with GPF Crypto Stick. If not, see <http://www.gnu.org/licenses/>.
*/


/*
 * Led_test.c
 *
 *  Created on: 13.07.2012
 *      Author: RB
 */


#include "compiler.h"
#include "preprocessor.h"
#include "board.h"
#include "gpio.h"
#include "ctrl_access.h"

#include "global.h"
#include "tools.h"
#include "LED_test.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

//#define TOOL_LED_RED_PIN       AVR32_PIN_PX41
//#define TOOL_LED_GREEN_PIN     AVR32_PIN_PX45

#define LED_OFF       0
#define LED_ON        1

/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/


static u8 LED_GreenToggleFlag_u8      = LED_OFF;
static u8 LED_RedToggleFlag_u8        = LED_OFF;
static u8 LED_RedGreenToggleFlag_u8   = LED_OFF;

/*******************************************************************************

  LED_GreenToggle

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_GreenToggle (void)
{
  if (LED_OFF == LED_GreenToggleFlag_u8)
  {
    gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
    LED_GreenToggleFlag_u8 = LED_ON;
  }
  else
  {
    gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
    LED_GreenToggleFlag_u8 = LED_OFF;
  }
}

/*******************************************************************************

  LED_GreenOff

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_GreenOff (void)
{
  gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
  LED_GreenToggleFlag_u8 = LED_OFF;
}

/*******************************************************************************

  LED_GreenOn

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_GreenOn (void)
{
  gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
  LED_GreenToggleFlag_u8 = LED_ON;
}

/*******************************************************************************

  LED_RedToggle

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedToggle (void)
{
  if (LED_OFF == LED_RedToggleFlag_u8)
  {
    gpio_set_gpio_pin (TOOL_LED_RED_PIN);
    LED_RedToggleFlag_u8 = LED_ON;
  }
  else
  {
    gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
    LED_RedToggleFlag_u8 = LED_OFF;
  }
}

/*******************************************************************************

  LED_RedOff

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedOff (void)
{
  gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
  LED_RedToggleFlag_u8 = LED_OFF;
}

/*******************************************************************************

  LED_RedOn

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedOn (void)
{
  gpio_set_gpio_pin (TOOL_LED_RED_PIN);
  LED_RedToggleFlag_u8 = LED_ON;
}

/*******************************************************************************

  LED_RedGreenToggle

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedGreenToggle (void)
{
  if (LED_OFF == LED_RedToggleFlag_u8)
  {
    gpio_set_gpio_pin (TOOL_LED_RED_PIN);
    gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
    LED_RedToggleFlag_u8 = LED_ON;
  }
  else
  {
    if (LED_OFF == LED_RedToggleFlag_u8)
    {
      gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
    }
    if (LED_OFF == LED_GreenToggleFlag_u8)
    {
      gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
    }
    LED_RedGreenToggleFlag_u8 = LED_OFF;
  }
}

/*******************************************************************************

  LED_RedGreenOff

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedGreenOff (void)
{
  if (LED_OFF == LED_GreenToggleFlag_u8)
  {
    gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
  }
  if (LED_OFF == LED_RedToggleFlag_u8)
  {
    gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
  }
  LED_RedGreenToggleFlag_u8 = LED_OFF;
}

/*******************************************************************************

  LED_RedGreenOn

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedGreenOn (void)
{
  gpio_set_gpio_pin (TOOL_LED_RED_PIN);
  gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
  LED_RedGreenToggleFlag_u8 = LED_ON;
}

/*******************************************************************************

  IBN_LED_Tests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


void IBN_LED_Tests (unsigned char nParamsGet_u8,unsigned char CMD_u8,unsigned int Param_u32,unsigned char *String_pu8)
{
  if (0 == nParamsGet_u8)
  {
    CI_LocalPrintf ("LED test functions\r\n");
    CI_LocalPrintf ("\r\n");
    CI_LocalPrintf ("0   [0,1]  LED RED       off,on\r\n");
    CI_LocalPrintf ("1   [0,1]  LED GREEN     off,on\r\n");
    CI_LocalPrintf ("2   [0,1]  LED RED+GREEN off,on\r\n");
    CI_LocalPrintf ("\r\n");
    return;
  }
  switch (CMD_u8)
  {
    case 0 :
      if (0 == Param_u32)
      {
        gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
      }
      else
      {
        gpio_set_gpio_pin (TOOL_LED_RED_PIN);
      }
      break;

    case 1 :
      if (0 == Param_u32)
      {
        gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
      }
      else
      {
        gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
      }
      break;
    case 2 :
      if (0 == Param_u32)
      {
        gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
        gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
      }
      else
      {
        gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
        gpio_set_gpio_pin (TOOL_LED_RED_PIN);
      }
      break;
  }
}
