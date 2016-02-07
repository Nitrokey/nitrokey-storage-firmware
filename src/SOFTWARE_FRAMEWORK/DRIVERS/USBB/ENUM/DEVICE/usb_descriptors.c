/* This source file is part of the ATMEL AVR-UC3-SoftwareFramework-1.7.0 Release */

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


// _____ I N C L U D E S ____________________________________________________

#include "conf_usb.h"


#if USB_DEVICE_FEATURE == ENABLED

#include "usb_drv.h"
#include "usb_descriptors.h"
#include "usb_standard_request.h"
#include "usb_specific_request.h"


// _____ M A C R O S ________________________________________________________


// _____ D E F I N I T I O N S ______________________________________________

// usb_hid_report_descriptor_mouse

#define KEYBOARD_FEATURE_COUNT                64

const U8 usb_hid_report_descriptor_keyboard[USB_HID_REPORT_DESC_KEYBOARD] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x06, // USAGE (Keyboard)
    0xa1, 0x01, // COLLECTION (Application)
    // 0x85, 0x01, // REPORT_ID (1)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0xe0, // USAGE_MINIMUM (Keyboard LeftControl)
    0x29, 0xe7, // USAGE_MAXIMUM (Keyboard Right GUI)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x08, // REPORT_COUNT (8)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x08, // REPORT_SIZE (8)
    0x81, 0x03, // INPUT (Cnst,Var,Abs)
    0x95, 0x05, // REPORT_COUNT (5)
    0x75, 0x01, // REPORT_SIZE (1)
    0x05, 0x08, // USAGE_PAGE (LEDs)
    0x19, 0x01, // USAGE_MINIMUM (Num Lock)
    0x29, 0x05, // USAGE_MAXIMUM (Kana)
    0x91, 0x02, // OUTPUT (Data,Var,Abs)
    0x95, 0x01, // REPORT_COUNT (1)
    0x75, 0x03, // REPORT_SIZE (3)
    0x91, 0x03, // OUTPUT (Cnst,Var,Abs)
    0x95, 0x06, // REPORT_COUNT (6)
    0x75, 0x08, // REPORT_SIZE (8)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x65, // LOGICAL_MAXIMUM (101)
    0x05, 0x07, // USAGE_PAGE (Keyboard)
    0x19, 0x00, // USAGE_MINIMUM (Reserved (no event indicated))
    0x29, 0x65, // USAGE_MAXIMUM (Keyboard Application)
    0x81, 0x00, // INPUT (Data,Ary,Abs)
    0x09, 0x03, // USAGE (Keyboard ErrorUndefine)
    0x75, 0x08, // REPORT_SIZE (8)
    0x95, KEYBOARD_FEATURE_COUNT,   // REPORT_COUNT (64)
    0xB1, 0x02, // FEATURE (Data,Var,Abs)

    0xc0,   // END_COLLECTION
};

// usb_user_device_descriptor
const S_usb_device_descriptor usb_dev_desc = {
    sizeof (S_usb_device_descriptor),
    DEVICE_DESCRIPTOR,
    Usb_format_mcu_to_usb_data (16, USB_SPECIFICATION),
    DEVICE_CLASS,
    DEVICE_SUB_CLASS,
    DEVICE_PROTOCOL,
    EP_CONTROL_LENGTH,
    Usb_format_mcu_to_usb_data (16, VENDOR_ID),
    Usb_format_mcu_to_usb_data (16, PRODUCT_ID),
    Usb_format_mcu_to_usb_data (16, RELEASE_NUMBER),
    MAN_INDEX,
    PROD_INDEX,
    SN_INDEX,
    NB_CONFIGURATION
};


// usb_user_configuration_descriptor FS
const S_usb_user_configuration_descriptor usb_conf_desc_fs = {
    {
     sizeof (S_usb_configuration_descriptor),
     CONFIGURATION_DESCRIPTOR,
     Usb_format_mcu_to_usb_data (16, sizeof (S_usb_user_configuration_descriptor)),
     NB_INTERFACE,
     CONF_NB,
     CONF_INDEX,
     CONF_ATTRIBUTES,
     MAX_POWER}
#ifdef USB_MSD
    , {
       sizeof (S_usb_interface_descriptor),
       INTERFACE_DESCRIPTOR,
       INTERFACE_NB,
       ALTERNATE,
       NB_ENDPOINT,
       INTERFACE_CLASS,
       INTERFACE_SUB_CLASS,
       INTERFACE_PROTOCOL,
       INTERFACE_INDEX}
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_1,
     EP_ATTRIBUTES_1,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_1_FS),
     EP_INTERVAL_1}
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_2,
     EP_ATTRIBUTES_2,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_2_FS),
     EP_INTERVAL_2}
#endif
#ifdef USB_CCID
    , {
       sizeof (S_usb_interface_descriptor),
       INTERFACE_DESCRIPTOR,
       CCID_INTERFACE_NB,
       CCID_ALTERNATE,
       CCID_NB_ENDPOINT,
       CCID_INTERFACE_CLASS,
       CCID_INTERFACE_SUB_CLASS,
       CCID_INTERFACE_PROTOCOL,
       CCID_INTERFACE_INDEX}
    ,

    // 0x36 Bytes CCID Descriptor
    {
     0x36,  // bLength: CCID Descriptor size
     0x21,  // bDescriptorType: HID To be updated with CCID specific number
     0x10,  // bcdHID(LSB): CCID Class Spec release number (1.10)
     0x01,  // bcdHID(MSB)

     0x00,  // bMaxSlotIndex
     0x02,  // bVoltageSupport: 3v

     0x02, 0x00, 0x00, 0x00,    // dwProtocols: supports T=1
     0x10, 0x0E, 0x00, 0x00,    // dwDefaultClock: 3,6 Mhz (0x00000E10)
     0x10, 0x0E, 0x00, 0x00,    // dwMaximumClock: 3,6 Mhz (0x00000E10)
     0x00,  // bNumClockSupported => only default clock

     0xCD, 0x25, 0x00, 0x00,    // dwDataRate: 9677 bps (0x000025CD)
     0xA1, 0xC5, 0x01, 0x00,    // dwMaxDataRate: 116129 bps (0x0001C5A1)
     0x00,  // bNumDataRatesSupported => no manual setting

     0xFE, 0x00, 0x00, 0x00,    /* dwMaxIFSD: 254 */
     0x00, 0x00, 0x00, 0x00,    /* dwSynchProtocols */
     0x00, 0x00, 0x00, 0x00,    /* dwMechanical: no special characteristics */

     // 0x3E,0x00,0x02,0x00, // instand pc reset on XP :-)
     0xBA, 0x04, 0x01, 0x00,    // 000104BAh
     // 00000002h Automatic parameter configuration based on ATR data
     // 00000008h Automatic ICC voltage selection
     // 00000010h Automatic ICC clock frequency change
     // 00000020h Automatic baud rate change
     // 00000080h Automatic PPS
     // 00000400h Automatic IFSD exchange
     // 00010000h TPDU level exchanges with CCID

     // 0x24,0x00,0x00,0x00, /* dwMaxCCIDMessageLength : Maximun block size + header*/
     0x0F, 0x01, 0x00, 0x00,    /* dwMaxCCIDMessageLength : Maximun block size + header */
     /* 261 + 10 */

     0x00,  /* bClassGetResponse */
     0x00,  /* bClassEnvelope */
     0x00, 0x00,    /* wLcdLayout */
     0x00,  /* bPINSupport : no PIN verif and modif */
     0x01   /* bMaxCCIDBusySlots */
     }
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_3,
     EP_ATTRIBUTES_3,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_3_FS),
     EP_INTERVAL_3}
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_4,
     EP_ATTRIBUTES_4,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_4_FS),
     EP_INTERVAL_4}
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_5,
     EP_ATTRIBUTES_5,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_5_FS),
     EP_INTERVAL_5}

#endif // USB_CCID

#ifdef USB_KB
    , {
       sizeof (S_usb_interface_descriptor),
       INTERFACE_DESCRIPTOR,
       KB_INTERFACE_NB,
       KB_ALTERNATE,
       KB_NB_ENDPOINT,
       KB_INTERFACE_CLASS,
       KB_INTERFACE_SUB_CLASS,
       KB_INTERFACE_PROTOCOL,
       KB_INTERFACE_INDEX}
    ,
    {
     sizeof (S_usb_hid_descriptor),
     HID_DESCRIPTOR,
     Usb_format_mcu_to_usb_data (16, HID_VERSION_KEYBOARD),
     HID_COUNTRY_CODE_KEYBOARD,
     HID_NUM_DESCRIPTORS_KEYBOARD,
     HID_REPORT_DESCRIPTOR,
     Usb_format_mcu_to_usb_data (16, sizeof (usb_hid_report_descriptor_keyboard))}
    ,
    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_6,
     EP_ATTRIBUTES_6,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_6_FS),
     EP_INTERVAL_6}

#endif // USB_KB

};

#if (USB_HIGH_SPEED_SUPPORT==ENABLED)

// usb_user_configuration_descriptor HS
const S_usb_user_configuration_descriptor usb_conf_desc_hs = {
    {
     sizeof (S_usb_configuration_descriptor),
     CONFIGURATION_DESCRIPTOR,
     Usb_format_mcu_to_usb_data (16, sizeof (S_usb_user_configuration_descriptor)),
     NB_INTERFACE,
     CONF_NB,
     CONF_INDEX,
     CONF_ATTRIBUTES,
     MAX_POWER}
#ifdef USB_MSD
    , {
       sizeof (S_usb_interface_descriptor),
       INTERFACE_DESCRIPTOR,
       INTERFACE_NB,
       ALTERNATE,
       NB_ENDPOINT,
       INTERFACE_CLASS,
       INTERFACE_SUB_CLASS,
       INTERFACE_PROTOCOL,
       INTERFACE_INDEX}
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_1,
     EP_ATTRIBUTES_1,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_1_HS),
     EP_INTERVAL_1}
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_2,
     EP_ATTRIBUTES_2,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_2_HS),
     EP_INTERVAL_2}
#endif
#ifdef USB_CCID
    , {
       sizeof (S_usb_interface_descriptor),
       INTERFACE_DESCRIPTOR,
       CCID_INTERFACE_NB,
       CCID_ALTERNATE,
       CCID_NB_ENDPOINT,
       CCID_INTERFACE_CLASS,
       CCID_INTERFACE_SUB_CLASS,
       CCID_INTERFACE_PROTOCOL,
       CCID_INTERFACE_INDEX}
    ,

    // 0x36 Bytes CCID Descriptor
    {
     0x36,  // bLength: CCID Descriptor size
     0x21,  // bDescriptorType: HID To be updated with CCID specific number
     0x10,  // bcdHID(LSB): CCID Class Spec release number (1.10)
     0x01,  // bcdHID(MSB)

     0x00,  // bMaxSlotIndex
     0x02,  // bVoltageSupport: 3v

     0x02, 0x00, 0x00, 0x00,    // dwProtocols: supports T=1
     0x10, 0x0E, 0x00, 0x00,    // dwDefaultClock: 3,6 Mhz (0x00000E10)
     0x10, 0x0E, 0x00, 0x00,    // dwMaximumClock: 3,6 Mhz (0x00000E10)
     0x00,  // bNumClockSupported => only default clock

     0xCD, 0x25, 0x00, 0x00,    // dwDataRate: 9677 bps (0x000025CD)
     0xA1, 0xC5, 0x01, 0x00,    // dwMaxDataRate: 116129 bps (0x0001C5A1)
     0x00,  // bNumDataRatesSupported => no manual setting

     0x05, 0x01, 0x00, 0x00,    /* dwMaxIFSD: 261 */
     0x00, 0x00, 0x00, 0x00,    /* dwSynchProtocols */
     0x00, 0x00, 0x00, 0x00,    /* dwMechanical: no special characteristics */

     // 0x3E,0x00,0x02,0x00, // instand pc reset on XP :-)
     0xBA, 0x04, 0x01, 0x00,    // 000104BAh
     // 00000002h Automatic parameter configuration based on ATR data
     // 00000008h Automatic ICC voltage selection
     // 00000010h Automatic ICC clock frequency change
     // 00000020h Automatic baud rate change
     // 00000080h Automatic PPS
     // 00000400h Automatic IFSD exchange
     // 00010000h TPDU level exchanges with CCID

     // 0x24,0x00,0x00,0x00, /* dwMaxCCIDMessageLength : Maximun block size + header*/
//     0x30,0x00,0x00,0x00, /* dwMaxCCIDMessageLength : Maximun block size + header*/
     0x0F, 0x01, 0x00, 0x00,    /* dwMaxCCIDMessageLength : 271 Maximun block size + header */
     /* 261 + 10 */

     0x00,  /* bClassGetResponse */
     0x00,  /* bClassEnvelope */
     0x00, 0x00,    /* wLcdLayout */
     0x00,  /* bPINSupport : no PIN verif and modif */
     0x01   /* bMaxCCIDBusySlots */
     }
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_3,
     EP_ATTRIBUTES_3,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_3_HS),
     EP_INTERVAL_3}
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_4,
     EP_ATTRIBUTES_4,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_4_HS),
     EP_INTERVAL_4}
    ,

    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_5,
     EP_ATTRIBUTES_5,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_5_HS),
     EP_INTERVAL_5}

#endif // USB_CCID



#ifdef USB_KB
    , {
       sizeof (S_usb_interface_descriptor),
       INTERFACE_DESCRIPTOR,
       KB_INTERFACE_NB,
       KB_ALTERNATE,
       KB_NB_ENDPOINT,
       KB_INTERFACE_CLASS,
       KB_INTERFACE_SUB_CLASS,
       KB_INTERFACE_PROTOCOL,
       KB_INTERFACE_INDEX}
    ,
#ifdef NOT_USED
      /******************** Descriptor of Keyboard HID ********************/
    0x09,   /* bLength: HID Descriptor size */
    HID_DESCRIPTOR_TYPE,    /* bDescriptorType: HID */
    0x10,   /* bcdHID: HID Class Spec release number */
    0x01,
    0x00,   /* bCountryCode: Hardware target country */
    0x01,   /* bNumDescriptors: Number of HID class descriptors to follow */
    0x22,   /* bDescriptorType */
    KEYBOARD_SIZ_REPORT_DESC,   /* wItemLength: Total length of Report descriptor */
    0x00,
#endif // NOT_USED
    {
     sizeof (S_usb_hid_descriptor),
     HID_DESCRIPTOR,
     Usb_format_mcu_to_usb_data (16, HID_VERSION_KEYBOARD),
     HID_COUNTRY_CODE_KEYBOARD,
     HID_NUM_DESCRIPTORS_KEYBOARD,
     HID_REPORT_DESCRIPTOR,
     Usb_format_mcu_to_usb_data (16, sizeof (usb_hid_report_descriptor_keyboard))}
    ,
    {
     sizeof (S_usb_endpoint_descriptor),
     ENDPOINT_DESCRIPTOR,
     ENDPOINT_NB_6,
     EP_ATTRIBUTES_6,
     Usb_format_mcu_to_usb_data (16, EP_SIZE_6_HS),
     EP_INTERVAL_6}

#endif // USB_KB


};


// usb_qualifier_desc FS
const S_usb_device_qualifier_descriptor usb_qualifier_desc = {
    sizeof (S_usb_device_qualifier_descriptor),
    DEVICE_QUALIFIER_DESCRIPTOR,
    Usb_format_mcu_to_usb_data (16, USB_SPECIFICATION),
    DEVICE_CLASS,
    DEVICE_SUB_CLASS,
    DEVICE_PROTOCOL,
    EP_CONTROL_LENGTH,
    NB_CONFIGURATION,
    0
};
#endif


// usb_user_language_id
const S_usb_language_id usb_user_language_id = {
    sizeof (S_usb_language_id),
    STRING_DESCRIPTOR,
    Usb_format_mcu_to_usb_data (16, LANGUAGE_ID)
};


// usb_user_manufacturer_string_descriptor
const S_usb_manufacturer_string_descriptor usb_user_manufacturer_string_descriptor = {
    sizeof (S_usb_manufacturer_string_descriptor),
    STRING_DESCRIPTOR,
    USB_MANUFACTURER_NAME
};


// usb_user_product_string_descriptor
const S_usb_product_string_descriptor usb_user_product_string_descriptor = {
    sizeof (S_usb_product_string_descriptor),
    STRING_DESCRIPTOR,
    USB_PRODUCT_NAME
};


// usb_user_serial_number
const S_usb_serial_number usb_user_serial_number = {
    sizeof (S_usb_serial_number),
    STRING_DESCRIPTOR,
    USB_SERIAL_NUMBER
};


#endif // USB_DEVICE_FEATURE == ENABLED
