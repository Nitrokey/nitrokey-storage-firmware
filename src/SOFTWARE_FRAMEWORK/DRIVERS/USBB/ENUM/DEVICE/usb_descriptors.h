/* This header file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

/* This file is prepared for Doxygen automatic documentation generation. */
/* ! \file ****************************************************************** \brief USB identifiers. This file contains the USB parameters that
   uniquely identify the USB application through descriptor tables. - Compiler: IAR EWAVR32 and GNU GCC for AVR32 - Supported devices: All AVR32
   devices with a USB module can be used. - AppNote: \author Atmel Corporation: http://www.atmel.com \n Support and FAQ: http://support.atmel.no/
   ************************************************************************* */

/* Copyright (c) 2009 Atmel Corporation. All rights reserved. Redistribution and use in source and binary forms, with or without modification, are
   permitted provided that the following conditions are met: 1. Redistributions of source code must retain the above copyright notice, this list of
   conditions and the following disclaimer. 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
   the following disclaimer in the documentation and/or other materials provided with the distribution. 3. The name of Atmel may not be used to
   endorse or promote products derived from this software without specific prior written permission. 4. This software may only be redistributed and
   used in connection with an Atmel AVR product. THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE EXPRESSLY AND SPECIFICALLY
   DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE */
/*
 * This file contains modifications done by Rudolf Boeddeker
 * For the modifications applies:
 *
 * Author: Copyright (C) Rudolf Boeddeker  Date: 2012-08-18
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


#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_


// _____ I N C L U D E S ____________________________________________________

#include "conf_usb.h"

#if USB_DEVICE_FEATURE == DISABLED
#error usb_descriptors.h is #included although USB_DEVICE_FEATURE is disabled
#endif


#include "usb_standard_request.h"
#include "usb_task.h"


// _____ M A C R O S ________________________________________________________

#define Usb_unicode(c)                    (Usb_format_mcu_to_usb_data(16, (U16)(c)))
#define Usb_get_dev_desc_pointer()        (&(usb_dev_desc.bLength))
#define Usb_get_dev_desc_length()         (sizeof(usb_dev_desc))
#define Usb_get_conf_desc_pointer()       Usb_get_conf_desc_fs_pointer()
#define Usb_get_conf_desc_length()        Usb_get_conf_desc_fs_length()
#define Usb_get_conf_desc_hs_pointer()    (&(usb_conf_desc_hs.cfg.bLength))
#define Usb_get_conf_desc_hs_length()     (sizeof(usb_conf_desc_hs))
#define Usb_get_conf_desc_fs_pointer()    (&(usb_conf_desc_fs.cfg.bLength))
#define Usb_get_conf_desc_fs_length()     (sizeof(usb_conf_desc_fs))
#define Usb_get_qualifier_desc_pointer()  (&(usb_qualifier_desc.bLength))
#define Usb_get_qualifier_desc_length()   (sizeof(usb_qualifier_desc))


// _____ U S B D E F I N E S _____________________________________________

            // USB Device descriptor
#define USB_SPECIFICATION     0x0200
#define DEVICE_CLASS          0 // ! Each configuration has its own class
#define DEVICE_SUB_CLASS      0 // ! Each configuration has its own subclass
#define DEVICE_PROTOCOL       0 // ! Each configuration has its own protocol
#define EP_CONTROL_LENGTH     64
// #define VENDOR_ID ATMEL_VID //! Atmel vendor ID


#ifdef USB_HIGH_SPEED_SUPPORT   // todo : add IDs for KB interface
#if defined USB_MSD && !defined USB_CCID && !defined USB_KB
#define VENDOR_ID             STICK_V20_VID // ! Atmel vendor ID
#define PRODUCT_ID            MSD_HS_PID    // for MSD
#elif !defined USB_MSD && defined USB_CCID && !defined USB_KB
#define VENDOR_ID             STICK_V20_VID // ! STICK_V20 vendor ID
#define PRODUCT_ID            CCID_HS_PID   // for CCID
#elif defined USB_MSD && defined USB_CCID && !defined USB_KB
#define VENDOR_ID             STICK_V20_VID // ! STICK_V20 vendor ID
#define PRODUCT_ID            MSD_CCID_HS_PID   // for both
#elif !defined USB_MSD && !defined USB_CCID && defined USB_KB
#define VENDOR_ID             STICK_V20_VID // ! STICK_V20 vendor ID
#define PRODUCT_ID            KB_PID12_HS_PID   // only for testing
#elif defined USB_MSD && defined USB_CCID && defined USB_KB
#define VENDOR_ID             STICK_V20_VID // STICK_V20 vendor ID
#define PRODUCT_ID            MSD_CCID_HID_HS_PID   // Production id
#elif !defined USB_MSD && defined USB_CCID && defined USB_KB
#define VENDOR_ID             STICK_V20_VID // STICK_V20 vendor ID
#define PRODUCT_ID            CCID_KB_HS_PID    // CCID + HID (KB)
#else
#error "Wrong USB config"
#endif
#else // no USB_HIGH_SPEED_SUPPORT
#if defined USB_MSD && !defined USB_CCID
#define VENDOR_ID             STICK_V20_VID // ! STICK_V20 vendor ID
#define PRODUCT_ID            MSD_FS_PID    // for MSD
#elif !defined USB_MSD && defined USB_CCID
#define VENDOR_ID             STICK_V20_VID // ! STICK_V20 vendor ID
#define PRODUCT_ID            CCID_HS_PID   // for CCID
#elif defined USB_MSD && defined USB_CCID
#define VENDOR_ID             STICK_V20_VID // ! STICK_V20 vendor ID
#define PRODUCT_ID            MSD_CCID_FS_PID   // for both
#elif !defined USB_MSD && !defined USB_CCID && defined USB_KB
#define VENDOR_ID             STICK_V20_VID // ! STICK_V20 vendor ID
// #define PRODUCT_ID KB_HS_PID // for both
#define PRODUCT_ID            KB_PID12_HS_PID   // only for testing
#else
#error "Wrong USB config"
#endif

/*
   #ifdef USB_MSD #define VENDOR_ID ATMEL_VID //! Atmel vendor ID #define PRODUCT_ID MS_SDRAM_LOADER_PID // for both // MS_EXAMPLE_PID // CCID_PID
   #else #define VENDOR_ID STICK_V20_VID //! STICK_V20 vendor ID #define PRODUCT_ID CCID_PID #endif // USB_CCID */
#endif // USB_HIGH_SPEED_SUPPORT





#define RELEASE_NUMBER        0x0104
#define MAN_INDEX             0x01
#define PROD_INDEX            0x02
#define SN_INDEX              0x03
#define NB_CONFIGURATION      1

            // CONFIGURATION
#ifdef USB_CCID
#define NB_CCID_INTERFACE_ADD       1
#else
#define NB_CCID_INTERFACE_ADD       0
#endif // ifndef USB_CCID

#ifdef USB_MSD
#define NB_MSD_INTERFACE_ADD       1
#else
#define NB_MSD_INTERFACE_ADD       0
#endif // ifndef USB_CCID

#ifdef USB_KB
#define NB_KB_INTERFACE_ADD       1
#else
#define NB_KB_INTERFACE_ADD       0
#endif // ifndef USB_CCID


#define NB_INTERFACE       (NB_MSD_INTERFACE_ADD+NB_CCID_INTERFACE_ADD+NB_KB_INTERFACE_ADD)

#define CONF_NB            1    // ! Number of this configuration
#define CONF_INDEX         0
#define CONF_ATTRIBUTES    USB_CONFIG_BUSPOWERED
#define MAX_POWER          50   // 100 mA

#ifdef USB_MSD
            // USB Interface descriptor
#define INTERFACE_NB             0  // ! The number of this interface
#define ALTERNATE                0  // ! The alt setting nb of this interface
#define NB_ENDPOINT              2  // ! The number of endpoints this interface has
#define INTERFACE_CLASS          MS_CLASS   // ! Mass-Storage Class
#define INTERFACE_SUB_CLASS      SCSI_SUBCLASS  // ! SCSI Transparent Command Set Subclass
#define INTERFACE_PROTOCOL       BULK_PROTOCOL  // ! Bulk-Only Transport Protocol
#define INTERFACE_INDEX          0

            // USB Endpoint 1 descriptor FS
#define ENDPOINT_NB_1           (EP_MS_IN | MSK_EP_DIR)
#define EP_ATTRIBUTES_1         TYPE_BULK
#define EP_IN_LENGTH_1_FS       64
#define EP_SIZE_1_FS            EP_IN_LENGTH_1_FS
#define EP_IN_LENGTH_1_HS       512
#define EP_SIZE_1_HS            EP_IN_LENGTH_1_HS
#define EP_INTERVAL_1           0x00    // ! Interrupt polling interval from host

            // USB Endpoint 2 descriptor FS
#define ENDPOINT_NB_2           EP_MS_OUT
#define EP_ATTRIBUTES_2         TYPE_BULK
#define EP_OUT_LENGTH_2_FS      64
#define EP_SIZE_2_FS            EP_OUT_LENGTH_2_FS
#define EP_OUT_LENGTH_2_HS      512
#define EP_SIZE_2_HS            EP_OUT_LENGTH_2_HS
#define EP_INTERVAL_2           0x00    // ! Interrupt polling interval from host
#endif //


#ifdef USB_CCID // USB_MSD

// CCID USB Interface descriptor
#ifndef USB_MSD
#define CCID_INTERFACE_NB        0  // ! The number of this interface
#else
#define CCID_INTERFACE_NB        1  // ! The number of this interface
#endif
#define CCID_ALTERNATE           0  // ! The alt setting nb of this interface
#define CCID_NB_ENDPOINT         3  // ! The number of endpoints this interface has
#define CCID_INTERFACE_CLASS     CCID_CLASS // ! CCID Class
#define CCID_INTERFACE_SUB_CLASS 0  // !
#define CCID_INTERFACE_PROTOCOL  0  // !
#define CCID_INTERFACE_INDEX     0



            // USB Endpoint 3 descriptor - CCID INT
#define ENDPOINT_NB_3           (EP_CCID_INT| MSK_EP_DIR)
#define EP_ATTRIBUTES_3         TYPE_INTERRUPT
#define EP_OUT_LENGTH_3_FS      16
#define EP_SIZE_3_FS            EP_OUT_LENGTH_3_FS
#define EP_OUT_LENGTH_3_HS      16  // 64
#define EP_SIZE_3_HS            EP_OUT_LENGTH_3_HS
#define EP_INTERVAL_3           0x10    // Interrupt polling interval from host

            // USB Endpoint 4 descriptor - CCID OUT
#define ENDPOINT_NB_4           EP_CCID_OUT
#define EP_ATTRIBUTES_4         TYPE_BULK
#define EP_OUT_LENGTH_4_FS      64
#define EP_SIZE_4_FS            EP_OUT_LENGTH_4_FS
#define EP_OUT_LENGTH_4_HS      512
#define EP_SIZE_4_HS            EP_OUT_LENGTH_4_HS
#define EP_INTERVAL_4           0x00    // Interrupt polling interval from host

            // USB Endpoint 5 descriptor - CCID IN
#define ENDPOINT_NB_5           (EP_CCID_IN | MSK_EP_DIR)
#define EP_ATTRIBUTES_5         TYPE_BULK
#define EP_OUT_LENGTH_5_FS      64
#define EP_SIZE_5_FS            EP_OUT_LENGTH_5_FS
#define EP_OUT_LENGTH_5_HS      512
#define EP_SIZE_5_HS            EP_OUT_LENGTH_5_HS
#define EP_INTERVAL_5           0x00    // Interrupt polling interval from host

#endif // USB_CCID


#ifdef USB_KB   // USB_MSD

// CCID USB Interface descriptor
#ifndef USB_MSD
#ifndef USB_CCID
#define KB_INTERFACE_NB        0    // ! The number of this interface
#else
#define KB_INTERFACE_NB        1    // ! The number of this interface
#endif
#else
#ifndef USB_CCID
#define KB_INTERFACE_NB        1    // ! The number of this interface
#else
#define KB_INTERFACE_NB        2    // ! The number of this interface
#endif
#endif


#define KB_ALTERNATE           0    // ! The alt setting nb of this interface
#define KB_NB_ENDPOINT         1    // ! The number of endpoints this interface has
#define KB_INTERFACE_CLASS     HID_CLASS    // ! HID Class
#define KB_INTERFACE_SUB_CLASS NO_SUBCLASS  // !
#define KB_INTERFACE_PROTOCOL  GENERIC_PROTOCOL    // !
#define KB_INTERFACE_INDEX     0

            // Keyboard HID descriptor
#define HID_VERSION_KEYBOARD         0x0110 // ! HID Class Specification release number
#define HID_COUNTRY_CODE_KEYBOARD      0x00 // ! Hardware target country
#define HID_NUM_DESCRIPTORS_KEYBOARD   0x01 // ! Number of HID class descriptors to follow

            // USB Endpoint 6 descriptor - KB IN
#define ENDPOINT_NB_6           (EP_KB_IN | MSK_EP_DIR)
#define EP_ATTRIBUTES_6         TYPE_INTERRUPT
#define EP_OUT_LENGTH_6_FS      64
#define EP_SIZE_6_FS            EP_OUT_LENGTH_6_FS
#define EP_OUT_LENGTH_6_HS      64  // 8
#define EP_SIZE_6_HS            EP_OUT_LENGTH_6_HS
#define EP_INTERVAL_6           5   // ! Interrupt polling interval from host

#endif // USB_KB




#define DEVICE_STATUS         BUS_POWERED
#define INTERFACE_STATUS      0x00  // TBD

#define LANG_ID               0x00


#define USB_MN_LENGTH         8 // 25
#define USB_MANUFACTURER_NAME \
{\
  Usb_unicode('N'),\
  Usb_unicode('i'),\
  Usb_unicode('t'),\
  Usb_unicode('r'),\
  Usb_unicode('o'),\
  Usb_unicode('k'),\
  Usb_unicode('e'),\
  Usb_unicode('y'),\
}

/*
   Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode('
   '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\ Usb_unicode(' '),\
   Usb_unicode(' '),\ Usb_unicode(' '),\ */
#define USB_PN_LENGTH         16    // 17
#define USB_PRODUCT_NAME \
{\
  Usb_unicode('N'),\
  Usb_unicode('i'),\
  Usb_unicode('t'),\
  Usb_unicode('r'),\
  Usb_unicode('o'),\
  Usb_unicode('k'),\
  Usb_unicode('e'),\
  Usb_unicode('y'),\
  Usb_unicode(' '),\
  Usb_unicode('S'),\
  Usb_unicode('t'),\
  Usb_unicode('o'),\
  Usb_unicode('r'),\
  Usb_unicode('a'),\
  Usb_unicode('g'),\
  Usb_unicode('e'),\
}

// Usb_unicode(' '),

/*
   Usb_unicode('v'),\ Usb_unicode('2'),\ Usb_unicode('.'),\ Usb_unicode('1'),\ */

#define USB_SN_LENGTH         13
#define USB_SERIAL_NUMBER \
{\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0'),\
  Usb_unicode('0') \
}

#define LANGUAGE_ID           0x0409


// ! USB Request
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bmRequestType;               // !< Characteristics of the request
U8 bRequest;                    // !< Specific request
U16 wValue;                     // !< Field that varies according to request
U16 wIndex;                     // !< Field that varies according to request
U16 wLength;                    // !< Number of bytes to transfer if Data
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_UsbRequest;


// ! USB Device Descriptor
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< DEVICE descriptor type
U16 bscUSB;                     // !< Binay Coded Decimal Spec. release
U8 bDeviceClass;                // !< Class code assigned by the USB
U8 bDeviceSubClass;             // !< Subclass code assigned by the USB
U8 bDeviceProtocol;             // !< Protocol code assigned by the USB
U8 bMaxPacketSize0;             // !< Max packet size for EP0
U16 idVendor;                   // !< Vendor ID. ATMEL = 0x03EB
U16 idProduct;                  // !< Product ID assigned by the manufacturer
U16 bcdDevice;                  // !< Device release number
U8 iManufacturer;               // !< Index of manu. string descriptor
U8 iProduct;                    // !< Index of prod. string descriptor
U8 iSerialNumber;               // !< Index of S.N.  string descriptor
U8 bNumConfigurations;          // !< Number of possible configurations
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_device_descriptor;


// ! USB Configuration Descriptor
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< CONFIGURATION descriptor type
U16 wTotalLength;               // !< Total length of data returned
U8 bNumInterfaces;              // !< Number of interfaces for this conf.
U8 bConfigurationValue;         // !< Value for SetConfiguration resquest
U8 iConfiguration;              // !< Index of string descriptor
U8 bmAttributes;                // !< Configuration characteristics
U8 MaxPower;                    // !< Maximum power consumption
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_configuration_descriptor;


// ! USB Interface Descriptor
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< INTERFACE descriptor type
U8 bInterfaceNumber;            // !< Number of interface
U8 bAlternateSetting;           // !< Value to select alternate setting
U8 bNumEndpoints;               // !< Number of EP except EP 0
U8 bInterfaceClass;             // !< Class code assigned by the USB
U8 bInterfaceSubClass;          // !< Subclass code assigned by the USB
U8 bInterfaceProtocol;          // !< Protocol code assigned by the USB
U8 iInterface;                  // !< Index of string descriptor
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_interface_descriptor;


// ! USB Endpoint Descriptor
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< ENDPOINT descriptor type
U8 bEndpointAddress;            // !< Address of the endpoint
U8 bmAttributes;                // !< Endpoint's attributes
U16 wMaxPacketSize;             // !< Maximum packet size for this EP
U8 bInterval;                   // !< Interval for polling EP in ms
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_endpoint_descriptor;


// ! USB Device Qualifier Descriptor
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< Device Qualifier descriptor type
U16 bscUSB;                     // !< Binay Coded Decimal Spec. release
U8 bDeviceClass;                // !< Class code assigned by the USB
U8 bDeviceSubClass;             // !< Subclass code assigned by the USB
U8 bDeviceProtocol;             // !< Protocol code assigned by the USB
U8 bMaxPacketSize0;             // !< Max packet size for EP0
U8 bNumConfigurations;          // !< Number of possible configurations
U8 bReserved;                   // !< Reserved for future use, must be zero
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_device_qualifier_descriptor;


// ! USB Language Descriptor
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< STRING descriptor type
U16 wlangid;                    // !< Language id
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_language_id;


// _____ U S B M A N U F A C T U R E R D E S C R I P T O R _______________

// ! struct usb_st_manufacturer
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< STRING descriptor type
U16 wstring[USB_MN_LENGTH];     // !< Unicode characters
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_manufacturer_string_descriptor;


// _____ U S B P R O D U C T D E S C R I P T O R _________________________

// ! struct usb_st_product
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< STRING descriptor type
U16 wstring[USB_PN_LENGTH];     // !< Unicode characters
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_product_string_descriptor;


// _____ U S B S E R I A L N U M B E R D E S C R I P T O R _____________

// ! struct usb_st_serial_number
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< STRING descriptor type
U16 wstring[USB_SN_LENGTH];     // !< Unicode characters
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_serial_number;


// _____ U S B D E V I C E M A S S - S T O R A G E D E S C R I P T O R _
typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
U8 bLength;                     // !< Size of this descriptor in bytes
U8 bDescriptorType;             // !< HID descriptor type
U16 bcdHID;                     // !< HID Class Specification release number
U8 bCountryCode;                // !< Hardware target country
U8 bNumDescriptors;             // !< Number of HID class descriptors to follow
U8 bRDescriptorType;            // !< Report descriptor type
U16 wItemLength;                // !< Total length of Report descriptor
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_hid_descriptor;


typedef
#if (defined __ICCAVR32__)
#pragma pack(1)
#endif
    struct
#if (defined __GNUC__)
    __attribute__ ((__packed__))
#endif
{
S_usb_configuration_descriptor cfg;
#ifdef USB_MSD
    // MSD
S_usb_interface_descriptor ifc;
S_usb_endpoint_descriptor ep1;
S_usb_endpoint_descriptor ep2;
#endif

    // CCID
#ifdef USB_CCID
S_usb_interface_descriptor ifc_CCID;
U8 CCID_Descriptor[0x36];
S_usb_endpoint_descriptor ep3;
S_usb_endpoint_descriptor ep4;
S_usb_endpoint_descriptor ep5;
#endif // USB_CCID

#ifdef USB_KB
S_usb_interface_descriptor ifc_kb;
S_usb_hid_descriptor hid_kb;
S_usb_endpoint_descriptor ep6;
#endif
}

#if (defined __ICCAVR32__)
#pragma pack()
#endif
S_usb_user_configuration_descriptor;


#endif // _USB_DESCRIPTORS_H_
