/* This source file is part of the ATMEL AVR32-UC3-SoftwareFramework-1.6.0 Release */

/*This file is prepared for Doxygen automatic documentation generation.*/
/*! \file *********************************************************************
 *
 * \brief Main file of the USB mass-storage example.
 *
 * - Compiler:           IAR EWAVR32 and GNU GCC for AVR32
 * - Supported devices:  All AVR32 devices with a USB module can be used.
 * - AppNote:
 *
 * \author               Atmel Corporation: http://www.atmel.com \n
 *                       Support and FAQ: http://support.atmel.no/
 *
 ******************************************************************************/

/*! \mainpage AVR32 USB Software Framework for Mass Storage
 *
 * \section intro License
 * Use of this program is subject to Atmel's End User License Agreement.
 *
 * Please read the \ref license at the bottom of this page.
 *
 * \section install Description
 * This embedded application source code illustrates how to implement a USB mass-storage application
 * on the AVR32 microcontroller.
 *
 * As the AVR32 implements a device/host USB controller, the embedded application can operate
 * in one of the following USB operating modes:
 *   - USB device;
 *   - USB reduced-host controller;
 *   - USB dual-role device (depending on the ID pin). This is the default configuration used in this example.
 *
 * The next table shows the customization to do (into the conf_usb.h and conf_access.h configuration files)
 * in order to support one of the USB operating modes:
 * <table border="1">
 * <tr>
 * <th></th>
 * <th>USB device</th>
 * <th>USB reduced-host</th>
 * <th>USB dual-role</th>
 * </tr>
 * <tr>
 * <th>conf_usb.h</th>
 * </tr>
 * <tr>
 * <th>USB_DEVICE_FEATURE</th>
 * <td>ENABLED</td>
 * <td>DISABLED</td>
 * <td>ENABLED</td>
 * </tr>
 * <tr>
 * <th>USB_HOST_FEATURE</th>
 * <td>DISABLED</td>
 * <td>ENABLED</td>
 * <td>ENABLED</td>
 * </tr>
 * <tr>
 * <th>conf_access.h</th>
 * </tr>
 * <tr>
 * <th>LUN_USB</th>
 * <td>DISABLED</td>
 * <td>ENABLED</td>
 * <td>ENABLED</td>
 * </tr>
 * <tr>
 * <th>ACCESS_USB</th>
 * <td>ENABLED</td>
 * <td>DISABLED</td>
 * <td>ENABLED</td>
 * </tr>
 * </table>
 *
 * To optimize embedded code/RAM size and reduce the number of source modules, the application can be
 * configured to use one and only one of these operating modes.
 *
 * \section sample About the Sample Application
 * By default the sample code is delivered with a simple preconfigured dual-role USB application.
 * It means that the code generated allows to operate as a device or a host depending on the USB ID pin:
 *
 *   - Attached to a mini-B plug (ID pin unconnected) the application will be used in the device operating mode.
 * Thus, the application can be connected to a system host (e.g. a PC) to operate as a USB mass-storage device (removable drive):
 *
 * \image html appli_evk1100_device.jpg "EVK1100 USB Device Mode"
 * \image html appli_evk1101_device.jpg "EVK1101 USB Device Mode"
 * \image html appli_evk1104_device.jpg "EVK1104 USB Device Mode"
 * \image html appli_evk1105_device.jpg "EVK1105 USB Device Mode"
 *
 *   - Attached to a mini-A plug (ID pin tied to ground) the application will operate in reduced-host mode.
 * This mode allows to connect a USB mass-storage device:
 *
 * \image html appli_evk1100_host.jpg "EVK1100 USB Host Mode"
 * \image html appli_evk1101_host.jpg "EVK1101 USB Host Mode"
 * \image html appli_evk1104_host.jpg "EVK1104 USB Host Mode"
 * \image html appli_evk1105_host.jpg "EVK1105 USB Host Mode"
 *
 *   - In both modes, the application can be connected to a serial terminal (e.g. HyperTerminal under
 * Windows systems or minicom under Linux systems; UART settings: 57600 bauds, 8-bit data, no parity bit,
 * 1 stop bit, no flow control), from where the user can access a simple command line interpreter (uShell)
 * to perform file-system accesses.
 *
 * \section device_use Using the USB Device Mode
 * Connect the application to a USB mass-storage host (e.g. a PC) with a mini-B (embedded side) to A (PC host side) cable.
 * The application will behave as a USB key. It will allow to access files on the on-board virtual, data flash and SD/MMC memories.
 *
 * \section host_use Using the USB Host Mode
 * Connect the application to a USB mass-storage device (e.g. a USB key). The application will behave as a USB mass-storage
 * reduced host. It will allow to exchange files between the on-board virtual, data flash and SD/MMC memories and the mass-storage device.
 *
 * \section uShell Using the USB Shell Terminal
 * Connected to a serial terminal the uShell command line interpreter allows to:
 *   - access the file system (both on-board virtual, data flash and SD/MMC memories and connected USB mass-storage device):
 *     - disk: get the number of drives,
 *     - a:, b:, etc.: go to selected drive,
 *     - mount drivename (a, b, etc.):  go to selected drive,
 *     - format drivename (a, b, etc.): format selected drive,
 *     - fat: get FAT type used by current drive,
 *     - df: get free space information for all connected drives,
 *     - cd dirname: go to selected directory,
 *     - cd..: go to upper directory,
 *     - mark: bookmark current directory,
 *     - goto: go to bookmarked directory,
 *     - ls: list files and directories in current directory,
 *     - rm filename: remove selected file or empty directory,
 *     - rm*: remove all files or empty directories in current directory,
 *     - cp filename: copy filename to bookmarked directory,
 *     - mv src_filename dst_filename: rename selected file,
 *     - mkdir dirname: make directory,
 *     - deltree dirname: remove directory and its content,
 *     - touch filename: create file,
 *     - append filename: append to selected file from terminal input,
 *     - cat filename: list file contents;
 *   - get information about the connected device and to perform miscellaneous commands:
 *     - lsusb: when in USB host mode, get USB information,
 *     - usbsync {device|host}: when in USB host mode, synchronize device or host USB drive,
 *     - suspend: when in USB host mode, suspend USB bus activity,
 *     - resume: when in USB host mode, resume USB bus activity,
 *     - reboot: reset the application,
 *     - help: get help about uShell commands.
 *
 * \section arch Architecture
 * As illustrated in the figure below, the application entry point is located in the mass_storage_example.c file.
 * The main function first performs the initialization of services and tasks and then runs them in an infinite loop.
 *
 * The sample mass-storage application is based on four different tasks:
 *   - The USB task (usb_task.c associated source file) is the task performing the USB low-level
 * enumeration process in device or host mode. Once this task has detected that the USB connection is fully
 * operational, it updates various status flags that can be checked within the high-level applicative tasks.
 *   - The device mass-storage task (device_mass_storage_task.c associated source file) performs
 * SCSI bulk-only protocol decoding and flash memory accesses.
 *   - The host mass-storage task (host_mass_storage_task.c associated file) manages the connected
 * device mass-storage interface by performing SCSI bulk-only protocol encoding and flash memory accesses.
 *   - The USB shell task (ushell_task.c associated file) manages the processing of terminal commands.
 *
 * \image html arch_full.jpg "Architecture Overview"
 *
 * \section config Configuration
 * The sample application is configured to implement both host and device functionalities.
 * Of course it can also be configured to be used only in device or reduced-host mode (see the conf_usb.h file).
 * Depending on the USB operating mode selected, the USB task will call either the USB host task (usb_host_task.c),
 * or the USB device task (usb_device_task.c) to manage USB specification chapter 9 requests.
 *
 * \section files Main Files
 * Example configuration files can be found in SERVICES\\USB\\CLASS\\MASS_STORAGE\\EXAMPLES\\CONF.
 * - mass_storage_example.c: main file;
 * - device_mass_storage_task.c: management of the USB device mass-storage task;
 * - host_mass_storage_task.c: management of the USB host mass-storage task;
 * - conf_access.h: USB disk (LUN) configuration.
 *
 * \section limitations Limitations
 * Some mass-storage devices do not present directly a mass-storage-class
 * interface, which may e.g. be hidden behind a hub-class device. These devices
 * are not supported by this example and the current mass-storage software
 * framework.
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com/products/AVR32/">Atmel AVR32</A>.\n
 * Support and FAQ: http://support.atmel.no/
 *
 * \section license Copyright Notice
 * Copyright (c) 2009 Atmel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an Atmel
 * AVR product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
 *
 */
/*
* This file contains modifications done by Rudolf Boeddeker
* For the modifications applies:
*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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


//_____  I N C L U D E S ___________________________________________________

#ifndef FREERTOS_USED
  #if (defined __GNUC__)
    #include "nlao_cpu.h"
  #endif
#else
  #include <stdio.h>
#endif
#include "compiler.h"
#include "preprocessor.h"
#include "mass_storage_example.h"
#include "board.h"
#include "print_funcs.h"
#include "intc.h"
#include "power_clocks_lib.h"
#include "gpio.h"
#include "ctrl_access.h"
#if (defined AT45DBX_MEM) && (AT45DBX_MEM == ENABLE)
  #include "spi.h"
  #include "conf_at45dbx.h"
#endif
#if (defined SD_MMC_SPI_MEM) && (SD_MMC_SPI_MEM == ENABLE)
  #include "spi.h"
  #include "conf_sd_mmc_spi.h"
#endif
#if ((defined SD_MMC_MCI_0_MEM) && (SD_MMC_MCI_0_MEM == ENABLE)) || ((defined SD_MMC_MCI_1_MEM) && (SD_MMC_MCI_1_MEM == ENABLE))
  #include "mci.h"
  #include "conf_sd_mmc_mci.h"
#endif
#ifdef FREERTOS_USED
  #include "FreeRTOS.h"
  #include "task.h"
#endif
#include "conf_usb.h"
#include "usb_task.h"
#if USB_DEVICE_FEATURE == ENABLED
  #include "device_mass_storage_task.h"
#endif
#if USB_HOST_FEATURE == ENABLED
  #include "host_mass_storage_task.h"
#endif
#include "ushell_task.h"

#include "global.h"
#include "tools.h"
#include "CCID/USART/ISO7816_USART.h"
#include "CCID/USART/ISO7816_ADPU.h"
#include "CCID/USART/ISO7816_Prot_T1.h"
#include "CCID/USART/ISO7816_USART_INT.h"
#include "CCID/LOCAL_ACCESS/OpenPGP_V20.h"
#include "USB_CCID/USB_CCID_task.h"
#include "BUFFERED_SIO.h"
#include "Tools\Interpreter.h"
#include "INTERNAL_WORK/internal_work.h"

#ifdef TIME_MEASURING_ENABLE
	#include "Tools\TIME_MEASURING.h"
#endif

#include "usart.h"
#include "DFU_test.h"
#include "flashc.h"


/*******************************************************************************

 Local defines

*******************************************************************************/

//#define FOSC0_STICK20           8000000                              //!< Osc0 frequency: Hz.
//#define OSC0_STARTUP_STICK20    AVR32_PM_OSCCTRL0_STARTUP_2048_RCOSC  //!< Osc0 startup time: RCOsc periods.


/*******************************************************************************

 Global declarations

*******************************************************************************/

/*******************************************************************************

 External declarations

*******************************************************************************/

/*******************************************************************************

 Local declarations

*******************************************************************************/

static pcl_freq_param_t pcl_freq_param =
{
  .cpu_f        = FCPU_HZ,
  .pba_f        = FPBA_HZ,
  .osc0_f       = FOSC0,
  .osc0_startup = OSC0_STARTUP

};

/*******************************************************************************

  init_hmatrix

  Hmatrix bus configuration

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void init_hmatrix(void)
{
  union
  {
    unsigned long                 scfg;
    avr32_hmatrix_scfg_t          SCFG;
  } u_avr32_hmatrix_scfg;

  // For the internal-flash HMATRIX slave, use last master as default.
  u_avr32_hmatrix_scfg.scfg = AVR32_HMATRIX.scfg[AVR32_HMATRIX_SLAVE_FLASH];
  u_avr32_hmatrix_scfg.SCFG.defmstr_type = AVR32_HMATRIX_DEFMSTR_TYPE_LAST_DEFAULT;
  AVR32_HMATRIX.scfg[AVR32_HMATRIX_SLAVE_FLASH] = u_avr32_hmatrix_scfg.scfg;
}



#if (defined AT45DBX_MEM) && (AT45DBX_MEM == ENABLE)

/*******************************************************************************

  at45dbx_resources_init

  Initializes AT45DBX resources: GPIO, SPI and AT45DBX

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

static void at45dbx_resources_init(void)
{
  static const gpio_map_t AT45DBX_SPI_GPIO_MAP =
  {
    {AT45DBX_SPI_SCK_PIN,          AT45DBX_SPI_SCK_FUNCTION         },  // SPI Clock.
    {AT45DBX_SPI_MISO_PIN,         AT45DBX_SPI_MISO_FUNCTION        },  // MISO.
    {AT45DBX_SPI_MOSI_PIN,         AT45DBX_SPI_MOSI_FUNCTION        },  // MOSI.
#define AT45DBX_ENABLE_NPCS_PIN(NPCS, unused) \
    {AT45DBX_SPI_NPCS##NPCS##_PIN, AT45DBX_SPI_NPCS##NPCS##_FUNCTION},  // Chip Select NPCS.
    MREPEAT(AT45DBX_MEM_CNT, AT45DBX_ENABLE_NPCS_PIN, ~)
#undef AT45DBX_ENABLE_NPCS_PIN
  };

  // SPI options.
  spi_options_t spiOptions =
  {
    .reg          = AT45DBX_SPI_FIRST_NPCS,   // Defined in conf_at45dbx.h.
    .baudrate     = AT45DBX_SPI_MASTER_SPEED, // Defined in conf_at45dbx.h.
    .bits         = AT45DBX_SPI_BITS,         // Defined in conf_at45dbx.h.
    .spck_delay   = 0,
    .trans_delay  = 0,
    .stay_act     = 1,
    .spi_mode     = 0,
    .modfdis      = 1
  };

  // Assign I/Os to SPI.
  gpio_enable_module(AT45DBX_SPI_GPIO_MAP,
                     sizeof(AT45DBX_SPI_GPIO_MAP) / sizeof(AT45DBX_SPI_GPIO_MAP[0]));

  // If the SPI used by the AT45DBX is not enabled.
  if (!spi_is_enabled(AT45DBX_SPI))
  {
    // Initialize as master.
    spi_initMaster(AT45DBX_SPI, &spiOptions);

    // Set selection mode: variable_ps, pcs_decode, delay.
    spi_selectionMode(AT45DBX_SPI, 0, 0, 0);

    // Enable SPI.
    spi_enable(AT45DBX_SPI);
  }

  // Initialize data flash with SPI PB clock.
  at45dbx_init(spiOptions, pcl_freq_param.pba_f);
}

#endif  // AT45DBX_MEM == ENABLE


#if (defined SD_MMC_SPI_MEM) && (SD_MMC_SPI_MEM == ENABLE)

/*******************************************************************************

  sd_mmc_spi_resources_init

  Initializes SD/MMC resources: GPIO, SPI and SD/MMC

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

static void sd_mmc_spi_resources_init(void)
{
  static const gpio_map_t SD_MMC_SPI_GPIO_MAP =
  {
    {SD_MMC_SPI_SCK_PIN,  SD_MMC_SPI_SCK_FUNCTION },  // SPI Clock.
    {SD_MMC_SPI_MISO_PIN, SD_MMC_SPI_MISO_FUNCTION},  // MISO.
    {SD_MMC_SPI_MOSI_PIN, SD_MMC_SPI_MOSI_FUNCTION},  // MOSI.
    {SD_MMC_SPI_NPCS_PIN, SD_MMC_SPI_NPCS_FUNCTION}   // Chip Select NPCS.
  };

  // SPI options.
  spi_options_t spiOptions =
  {
    .reg          = SD_MMC_SPI_NPCS,
    .baudrate     = SD_MMC_SPI_MASTER_SPEED,  // Defined in conf_sd_mmc_spi.h.
    .bits         = SD_MMC_SPI_BITS,          // Defined in conf_sd_mmc_spi.h.
    .spck_delay   = 0,
    .trans_delay  = 0,
    .stay_act     = 1,
    .spi_mode     = 0,
    .modfdis      = 1
  };

  // Assign I/Os to SPI.
  gpio_enable_module(SD_MMC_SPI_GPIO_MAP,
                     sizeof(SD_MMC_SPI_GPIO_MAP) / sizeof(SD_MMC_SPI_GPIO_MAP[0]));

  // If the SPI used by the SD/MMC is not enabled.
  if (!spi_is_enabled(SD_MMC_SPI))
  {
    // Initialize as master.
    spi_initMaster(SD_MMC_SPI, &spiOptions);

    // Set selection mode: variable_ps, pcs_decode, delay.
    spi_selectionMode(SD_MMC_SPI, 0, 0, 0);

    // Enable SPI.
    spi_enable(SD_MMC_SPI);
  }

  // Initialize SD/MMC with SPI PB clock.
  sd_mmc_spi_init(spiOptions, pcl_freq_param.pba_f);
}

#endif  // SD_MMC_SPI_MEM == ENABLE


#if ((defined SD_MMC_MCI_0_MEM) && (SD_MMC_MCI_0_MEM == ENABLE)) || ((defined SD_MMC_MCI_1_MEM) && (SD_MMC_MCI_1_MEM == ENABLE))

/*******************************************************************************

  Test_MCI_Pins

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

/*

int Test_MCI_Pins (void)
{
  long i;

  for (i=0;i<100000000;i++)
  {
    if (0 == i % 2)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_CLK_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_CLK_PIN);
    }

    if (0 == i % 4)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_CMD_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_CMD_PIN);
    }

    if (0 == i % 8)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_DATA0_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_DATA0_PIN);
    }

    if (0 == i % 16)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_DATA1_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_DATA1_PIN);
    }

    if (0 == i % 32)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_DATA2_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_DATA2_PIN);
    }

    if (0 == i % 64)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_DATA3_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_DATA3_PIN);
    }

  }

  return (true);
}
*/
/*******************************************************************************

  Test_LEDs_Pins

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/
/*
int Test_LEDs_Pins (void)
{
  int i;

  for (i=0;i<100000000;i++)
  {
    if (0 == i % 2)
    {
      gpio_set_gpio_pin (TOOL_LED_RED_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
    }

    if (0 == i % 4)
    {
      gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
    }

  }

  return (true);
}
*/

/*******************************************************************************

  Test_ALL_Pins

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int Test_ALL_Pins (void)
{
  long i;

  for (i=0;i<100000000;i++)
  {
    if (0 == i % 2)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_CLK_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_CLK_PIN);
    }

    if (0 == i % 4)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_CMD_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_CMD_PIN);
    }

    if (0 == i % 8)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_DATA0_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_DATA0_PIN);
    }

    if (0 == i % 16)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_DATA1_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_DATA1_PIN);
    }

    if (0 == i % 32)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_DATA2_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_DATA2_PIN);
    }

    if (0 == i % 64)
    {
      gpio_set_gpio_pin (SD_SLOT_8BITS_DATA3_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (SD_SLOT_8BITS_DATA3_PIN);
    }

   // gpio_set_gpio_pin (TOOL_LED_RED_PIN);

    if (0 == i % 16)
    {
      gpio_set_gpio_pin (TOOL_LED_RED_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (TOOL_LED_RED_PIN);
    }
/**/
/**/
//    gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);

    if (0 == i % 16)
    {
      gpio_set_gpio_pin (TOOL_LED_GREEN_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (TOOL_LED_GREEN_PIN);
    }
/**/


    if (0 == i % 256)
    {
      gpio_set_gpio_pin (ISO7816_RST_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (ISO7816_RST_PIN);
    }

    if (0 == i % 256)
    {
      gpio_set_gpio_pin (ISO7816_USART_TX_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (ISO7816_USART_TX_PIN);
    }

    if (0 == i % 256)
    {
      gpio_set_gpio_pin (ISO7816_USART_CLK_PIN);
    }
    else
    {
      gpio_clr_gpio_pin (ISO7816_USART_CLK_PIN);
    }

    if (0 == i % 256)
    {
      SmartcardPowerOn ();
    }
    else
    {
      SmartcardPowerOff ();
    }


  }

  return (true);
}

/*******************************************************************************

  sd_mmc_mci_resources_init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define SD_SLOT_8BITS_CARD_DETECT_STICK20          AVR32_PIN_PB00
#define SD_SLOT_CMD_STICK20                        AVR32_PIN_PA28
#define SD_SLOT_DATA0_STICK20                      AVR32_PIN_PA29


void sd_mmc_mci_resources_init(void)
{
  static const gpio_map_t SD_MMC_MCI_GPIO_MAP =
  {
    {SD_SLOT_8BITS_CLK_PIN,   SD_SLOT_8BITS_CLK_FUNCTION  },  // SD CLK.
    {SD_SLOT_8BITS_CMD_PIN,   SD_SLOT_8BITS_CMD_FUNCTION  },  // SD CMD.
    {SD_SLOT_8BITS_DATA0_PIN, SD_SLOT_8BITS_DATA0_FUNCTION},  // SD DAT[0].
    {SD_SLOT_8BITS_DATA1_PIN, SD_SLOT_8BITS_DATA1_FUNCTION},  // DATA Pin.
    {SD_SLOT_8BITS_DATA2_PIN, SD_SLOT_8BITS_DATA2_FUNCTION},  // DATA Pin.
    {SD_SLOT_8BITS_DATA3_PIN, SD_SLOT_8BITS_DATA3_FUNCTION},  // DATA Pin.
    {SD_SLOT_8BITS_DATA4_PIN, SD_SLOT_8BITS_DATA4_FUNCTION},  // DATA Pin.
    {SD_SLOT_8BITS_DATA5_PIN, SD_SLOT_8BITS_DATA5_FUNCTION},  // DATA Pin.
    {SD_SLOT_8BITS_DATA6_PIN, SD_SLOT_8BITS_DATA6_FUNCTION},  // DATA Pin.
    {SD_SLOT_8BITS_DATA7_PIN, SD_SLOT_8BITS_DATA7_FUNCTION}   // DATA Pin. Todo: Doppeltbelegung bei 100Pin CPU
  };

  // MCI options.
  static const mci_options_t MCI_OPTIONS =
  {
    .card_speed = 400000,
    .card_slot  = SD_SLOT_8BITS, // Default card initialization.
  };

  // Assign I/Os to MCI.
  gpio_enable_module(SD_MMC_MCI_GPIO_MAP,
                     sizeof(SD_MMC_MCI_GPIO_MAP) / sizeof(SD_MMC_MCI_GPIO_MAP[0]));


  // Enable pull-up for Card Detect.
//  gpio_enable_pin_pull_up(SD_SLOT_8BITS_CARD_DETECT);
  gpio_enable_pin_pull_up(SD_SLOT_8BITS_CARD_DETECT_STICK20);

  // Enable pull-up for Write Protect.
  gpio_enable_pin_pull_up(SD_SLOT_8BITS_WRITE_PROTECT);
//
// Für STICK20 microSD Sockel
  gpio_enable_pin_pull_up(SD_SLOT_CMD_STICK20);
  gpio_enable_pin_pull_up(SD_SLOT_DATA0_STICK20);
//
  // Initialize SD/MMC with MCI PB clock.
  sd_mmc_mci_init(&MCI_OPTIONS, pcl_freq_param.pba_f, pcl_freq_param.cpu_f);
}

#endif  // SD_MMC_MCI_0_MEM == ENABLE || SD_MMC_MCI_1_MEM == ENABLE

/*******************************************************************************

  TestUart0

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

#define EXAMPLE_USART               (&AVR32_USART0)
#define EXAMPLE_USART_RX_PIN        AVR32_USART0_RXD_0_0_PIN
#define EXAMPLE_USART_RX_FUNCTION   AVR32_USART0_RXD_0_0_FUNCTION
#define EXAMPLE_USART_TX_PIN        AVR32_USART0_TXD_0_0_PIN
#define EXAMPLE_USART_TX_FUNCTION   AVR32_USART0_TXD_0_0_FUNCTION

int TestUart0 (void)
{
  int i;
  int i1;

  static const gpio_map_t USART_GPIO_MAP =
  {
    {EXAMPLE_USART_RX_PIN, EXAMPLE_USART_RX_FUNCTION},
    {EXAMPLE_USART_TX_PIN, EXAMPLE_USART_TX_FUNCTION}
  };

  // USART options.
  static const usart_options_t USART_OPTIONS =
  {
    .baudrate     = 57600,
    .charlength   = 8,
    .paritytype   = USART_NO_PARITY,
    .stopbits     = USART_1_STOPBIT,
    .channelmode  = USART_NORMAL_CHMODE
  };

// Switch main clock to external oscillator 0 (crystal).
//  pm_switch_to_osc0(&AVR32_PM, FOSC0, OSC0_STARTUP);

  // Assign GPIO to USART.
  gpio_enable_module(USART_GPIO_MAP, sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

  // Initialize USART in RS232 mode.
  usart_init_rs232(EXAMPLE_USART, &USART_OPTIONS, FPBA_HZ); // FOSC0);

  //
  for (i=0;i<10000;i++)
  {
	  usart_putchar (EXAMPLE_USART,'a');
	  for (i1=0;i1<10000;i1++) ;

  }

  return (TRUE);
}

/*******************************************************************************

  SmartCard_test

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

int Simulation_USB_CCID (void);
int nnn = 0;

void SmartCard_test (void)
{

// Enable pull-up TX/RX
//gpio_enable_pin_pull_up(ISO7816_USART_TX_PIN);



//	DelayCounterTest ();
Test_ISO7816_Usart_Pins ();
//	   ISO7816_test ();
// 	  ISO7816_ResetSC_test ();
//	  ISO7816_InitSC ();
//		  ISO7816_APDU_Test ();
 //	LA_OpenPGP_V20_Test ();
//
//	if (nnn == 0)
	{
//	 Simulation_USB_CCID ();
	}
}

/*******************************************************************************

  CCID_Test_task

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CCID_Test_task (void *pvParameters)
{
	portTickType xLastWakeTime;


	xLastWakeTime = xTaskGetTickCount();
//	while (1) ;

	while (TRUE)
	{
		vTaskDelayUntil(&xLastWakeTime, configTSK_CCID_TEST_PERIOD);

		SmartCard_test ();
	}
}

/*******************************************************************************

  CCID_Test_task_init

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

void CCID_Test_task_init(void)
{
  xTaskCreate(CCID_Test_task,
              configTSK_CCID_TEST_NAME,
              configTSK_CCID_TEST_STACK_SIZE,
              NULL,
              configTSK_CCID_TEST_PRIORITY,
              NULL);
}



/*******************************************************************************

  main

  Reviews
  Date      Reviewer        Info
  16.08.13  RB              First review

*******************************************************************************/

// define _evba from exception.S
extern void _evba;

int main(void)
{
	pcl_freq_param_t local_pcl_freq_param;

/*
	USART_Int_Test ();

	return (0);
*/

  // Configure system clocks.
	local_pcl_freq_param = pcl_freq_param;

	if (pcl_configure_clocks(&local_pcl_freq_param) != PASS)
	{
		return 42;
	}

  /* Load the Exception Vector Base Address in the corresponding system register. */
  Set_system_register( AVR32_EVBA, ( int ) &_evba );

  /* Enable exceptions. */
  ENABLE_ALL_EXCEPTIONS();

  /* Initialize interrupt handling. */
  INTC_init_interrupts();




/* ISR TEST
*/
#if (defined AT45DBX_MEM) && (AT45DBX_MEM == ENABLE)
	at45dbx_resources_init();
#endif

#if SD_MMC_SPI_MEM == ENABLE
  sd_mmc_spi_resources_init();
#endif


#if ((defined SD_MMC_MCI_0_MEM) && (SD_MMC_MCI_0_MEM == ENABLE)) || ((defined SD_MMC_MCI_1_MEM) && (SD_MMC_MCI_1_MEM == ENABLE))
  sd_mmc_mci_resources_init();
#endif


#ifdef FREERTOS_USED
  if (!ctrl_access_init())
  {
    return 42;
  }
#endif  // FREERTOS_USED

// Init Hmatrix bus
  init_hmatrix();

// Initialize USB clock.
  pcl_configure_usb_clock();




/*
  SmartCard_test ();
  return 42;

  Test_ALL_Pins ();

  Test_MCI_Pins ();

	Test_LEDs_Pins ();

	TestUart0 ();

  while (1);
  return (0);
*/
#ifndef ISO7816_USE_TASK

#endif

#ifdef TIME_MEASURING_ENABLE
  TIME_MEASURING_Init ();
#endif


// For debugging
  BUFFERED_SIO_Init ();

#ifdef INTERPRETER_ENABLE
  IDF_task_init();
#endif

// Internal work task - FAT Access
  IW_task_init();

// Initialize USB tasks.
  usb_task_init();




#if USB_DEVICE_FEATURE == ENABLED
  #ifdef  USB_MSD
    device_mass_storage_task_init();
  #endif // USB_MSD
#endif


//  CCID_Test_task_init ();
#ifdef  USB_CCID
  USB_CCID_task_init();
#endif

// Protect bootloader
#ifdef STICK_20_A_MUSTER_PROD     //
  flashc_set_bootloader_protected_size (0x2000);
  flashc_lock_external_privileged_fetch (TRUE); // Disable external instruction fetch
#endif

  DFU_DisableFirmwareUpdate ();     // Stick always starts in application mode


//  ushell_task_init(pcl_freq_param.pba_f);

// Start stick
  vTaskStartScheduler();

// It never gets to this point
  return 42;
}
