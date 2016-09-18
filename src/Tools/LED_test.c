/*
 * Author: Copyright (C) Rudolf Boeddeker  Date: 13.07.2012
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
/*
   LED Access

   smart card red OTP green mass storage yellow (= red + green)

 */

static volatile u8 LED_StartUpActiv = TRUE;
static volatile u8 LED_StartUpSdCardOnline = FALSE; // Offline = red LED flashing on
static volatile u8 LED_StartUpScCardOnline = FALSE; // Offline = red LED on

static volatile u8 LED_GreenToggleFlag_u8 = LED_OFF;
static volatile u8 LED_RedToggleFlag_u8 = LED_OFF;
static volatile u8 LED_RedGreenToggleFlag_u8 = LED_OFF;


/*******************************************************************************

  LED_ScCardOnline_v

  Changes
  Date      Author          Info
  04.11.14  RB              Implementation of function

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void LED_ScCardOnline_v (void)
{
    LED_StartUpScCardOnline = TRUE;
}

/*******************************************************************************

  LED_SdCardOnline_v

  Changes
  Date      Author          Info
  04.11.14  RB              Implementation of function

  Reviews
  Date      Reviewer        Info

*******************************************************************************/

void LED_SdCardOnline_v (void)
{
    LED_StartUpSdCardOnline = TRUE;
}

/*******************************************************************************

  LED_GreenToggle

  Changes
  Date      Author          Info
  04.11.14  RB              Change to manager architecture

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_GreenToggle (void)
{
    if (LED_OFF == LED_GreenToggleFlag_u8)
    {
        // gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
        LED_GreenToggleFlag_u8 = LED_ON;
    }
    else
    {
        // gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
        LED_GreenToggleFlag_u8 = LED_OFF;
    }
    LED_Manager10ms_v ();       // Update LEDs
}

/*******************************************************************************

  LED_GreenOff

  Changes
  Date      Author          Info
  04.11.14  RB              Change to manager architecture

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_GreenOff (void)
{
    // gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
    LED_GreenToggleFlag_u8 = LED_OFF;
    LED_Manager10ms_v ();       // Update LEDs
}

/*******************************************************************************

  LED_GreenOn

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_GreenOn (void)
{
    // gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
    LED_GreenToggleFlag_u8 = LED_ON;
    LED_Manager10ms_v ();       // Update LEDs
}

/*******************************************************************************

  LED_RedToggle

  Changes
  Date      Author          Info
  04.11.14  RB              Change to manager architecture

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedToggle (void)
{
    if (LED_OFF == LED_RedToggleFlag_u8)
    {
        // gpio_set_gpio_pin (TOOL_LED_RED_PIN);
        LED_RedToggleFlag_u8 = LED_ON;
    }
    else
    {
        // gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
        LED_RedToggleFlag_u8 = LED_OFF;
    }
    LED_Manager10ms_v ();       // Update LEDs
}

/*******************************************************************************

  LED_RedOff

  Changes
  Date      Author          Info
  04.11.14  RB              Change to manager architecture

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedOff (void)
{
    // gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
    LED_RedToggleFlag_u8 = LED_OFF;
    LED_Manager10ms_v ();       // Update LEDs
}

/*******************************************************************************

  LED_RedOn

  Changes
  Date      Author          Info
  04.11.14  RB              Change to manager architecture

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedOn (void)
{
    // gpio_set_gpio_pin (TOOL_LED_RED_PIN);
    LED_RedToggleFlag_u8 = LED_ON;

    LED_Manager10ms_v ();       // Update LEDs
}

/*******************************************************************************

  LED_RedGreenToggle

  Changes
  Date      Author          Info
  04.11.14  RB              Change to manager architecture

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedGreenToggle (void)
{
    if (LED_OFF == LED_RedToggleFlag_u8)
    {
        // gpio_set_gpio_pin (TOOL_LED_RED_PIN);
        // gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
        LED_RedToggleFlag_u8 = LED_ON;
    }
    else
    {
        /*
           if (LED_OFF == LED_RedToggleFlag_u8) { gpio_clr_gpio_pin (TOOL_LED_RED_PIN); } if (LED_OFF == LED_GreenToggleFlag_u8) { gpio_clr_gpio_pin
           (TOOL_LED_GREEN_PIN); } */
        LED_RedGreenToggleFlag_u8 = LED_OFF;
    }
    LED_Manager10ms_v ();       // Update LEDs
}

/*******************************************************************************

  LED_RedGreenOff

  Changes
  Date      Author          Info
  04.11.14  RB              Change to manager architecture

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedGreenOff (void)
{
    /*
       if (LED_OFF == LED_GreenToggleFlag_u8) { gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN); } if (LED_OFF == LED_RedToggleFlag_u8) { gpio_clr_gpio_pin
       (TOOL_LED_RED_PIN); } */
    LED_RedGreenToggleFlag_u8 = LED_OFF;
    LED_Manager10ms_v ();       // Update LEDs
}


/*******************************************************************************

  LED_RedGreenOn

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void LED_RedGreenOn (void)
{
    /*
       gpio_set_gpio_pin (TOOL_LED_RED_PIN); gpio_set_gpio_pin (TOOL_LED_GREEN_PIN); */
    LED_RedGreenToggleFlag_u8 = LED_ON;
    LED_Manager10ms_v ();       // Update LEDs
}


/*******************************************************************************

  LED_Manager10ms_v

  Called every 10 ms

  Changes
  Date      Author          Info
  04.11.14  RB              Implementation of function

  Reviews
  Date      Reviewer        Info

*******************************************************************************/
#define LED_FLASH_DELAY 30  // = 300 ms

void LED_Manager10ms_v (void)
{
    u8 StateRedLed_u8 = LED_OFF;
    u8 StateGreenLed_u8 = LED_OFF;
    u8 StateRedLedFlashing_u8 = LED_OFF;
    static u8 FlashCounter_u8 = 0;

    if (TRUE == LED_StartUpActiv)   // Startup usage
    {
        if (FALSE == LED_StartUpSdCardOnline)
        {
            StateRedLedFlashing_u8 = LED_ON;
        }
        if (FALSE == LED_StartUpScCardOnline)
        {
            StateRedLed_u8 = LED_ON;
        }

        if ((TRUE == LED_StartUpSdCardOnline) && (TRUE == LED_StartUpScCardOnline))
        {
            LED_StartUpActiv = FALSE;
        }
    }
    else
    {   // Normal LED usage
        StateRedLed_u8 = LED_RedToggleFlag_u8;
        StateGreenLed_u8 = LED_GreenToggleFlag_u8;

        if (LED_ON == LED_RedGreenToggleFlag_u8)    // Red-green has higher prio
        {
            StateRedLed_u8 = LED_ON;
            StateGreenLed_u8 = LED_ON;
        }
    }

    if (0 != ISO7816_GetLockCounter())
    {
      StateRedLedFlashing_u8 = LED_ON;
    }

    // Flash controller
    FlashCounter_u8++;
    if (LED_ON == StateRedLedFlashing_u8)
    {
        if (LED_FLASH_DELAY / 2 > FlashCounter_u8)
        {
            StateRedLed_u8 = LED_ON;
        }
        else
        {
            StateRedLed_u8 = LED_OFF;
        }
    }

    if (LED_FLASH_DELAY == FlashCounter_u8) // Restart counter
    {
        FlashCounter_u8 = 0;
    }

    // Set the configuration
    if (LED_ON == StateRedLed_u8)
    {
        gpio_set_gpio_pin (TOOL_LED_RED_PIN);
    }
    else
    {
        gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
    }

    if (LED_ON == StateGreenLed_u8)
    {
        gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
    }
    else
    {
        gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
    }

}


/*******************************************************************************

  IBN_LED_Tests

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/


void IBN_LED_Tests (unsigned char nParamsGet_u8, unsigned char CMD_u8, unsigned int Param_u32, unsigned char* String_pu8)
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
        case 0:
            if (0 == Param_u32)
            {
                gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
            }
            else
            {
                gpio_set_gpio_pin (TOOL_LED_RED_PIN);
            }
            break;

        case 1:
            if (0 == Param_u32)
            {
                gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
            }
            else
            {
                gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
            }
            break;
        case 2:
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
