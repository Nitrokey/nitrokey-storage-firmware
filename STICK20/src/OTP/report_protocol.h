/*
* Author: Copyright (C) Andrzej Surowiec 2012
*												
*
* This file is part of GPF Crypto Stick.
*
* GPF Crypto Stick is free software: you can redistribute it and/or modify
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
* This file contains modifications done by Rudolf Boeddeker
* For the modifications applies:
*
* Author: Copyright (C) Rudolf Boeddeker  Date: 2013-08-16
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
#define FIRMWARE_VERSION 1

#define CMD_GET_STATUS                    0x00
#define CMD_WRITE_TO_SLOT                 0x01
#define CMD_READ_SLOT_NAME                0x02
#define CMD_READ_SLOT                     0x03
#define CMD_GET_CODE                      0x04
#define CMD_WRITE_CONFIG                  0x05
#define CMD_ERASE_SLOT                    0x06
#define CMD_FIRST_AUTHENTICATE            0x07
#define CMD_AUTHORIZE                     0x08
#define CMD_GET_PASSWORD_RETRY_COUNT      0x09
#define CMD_CLEAR_WARNING                 0x0A

#define CMD_SET_TIME                      0x0B
#define CMD_TEST_COUNTER                  0x0C
#define CMD_TEST_TIME                     0x0D
#define CMD_USER_AUTHENTICATE             0x0E
#define CMD_GET_USER_PASSWORD_RETRY_COUNT 0x0F
#define CMD_USER_AUTHORIZE                0x10
#define CMD_UNLOCK_USER_PASSOWRD          0x11
#define CMD_LOCK_DEVICE                   0x12
#define CMD_FACTORY_RESET                 0x13


#define CMD_GET_PW_SAFE_SLOT_STATUS       0x60
#define CMD_GET_PW_SAFE_SLOT_NAME         0x61
#define CMD_GET_PW_SAFE_SLOT_PASSWORD     0x62
#define CMD_GET_PW_SAFE_SLOT_LOGINNAME    0x63
#define CMD_SET_PW_SAFE_SLOT_DATA_1       0x64
#define CMD_SET_PW_SAFE_SLOT_DATA_2       0x65
#define CMD_PW_SAFE_ERASE_SLOT            0x66
#define CMD_PW_SAFE_ENABLE                0x67
#define CMD_PW_SAFE_INIT_KEY              0x68
#define CMD_PW_SAFE_SEND_DATA             0x69
#define CMD_SD_CARD_HIGH_WATERMARK        0x70

#define STICK20_CMD_START_VALUE                         0x20
#define STICK20_CMD_END_VALUE                           0x60
#define STICK20_CMD_ENABLE_CRYPTED_PARI                 (STICK20_CMD_START_VALUE +  0)
#define STICK20_CMD_DISABLE_CRYPTED_PARI                (STICK20_CMD_START_VALUE +  1)
#define STICK20_CMD_ENABLE_HIDDEN_CRYPTED_PARI          (STICK20_CMD_START_VALUE +  2)
#define STICK20_CMD_DISABLE_HIDDEN_CRYPTED_PARI         (STICK20_CMD_START_VALUE +  3)    // Not used
#define STICK20_CMD_ENABLE_FIRMWARE_UPDATE              (STICK20_CMD_START_VALUE +  4)
#define STICK20_CMD_EXPORT_FIRMWARE_TO_FILE             (STICK20_CMD_START_VALUE +  5)
#define STICK20_CMD_GENERATE_NEW_KEYS                   (STICK20_CMD_START_VALUE +  6)
#define STICK20_CMD_FILL_SD_CARD_WITH_RANDOM_CHARS      (STICK20_CMD_START_VALUE +  7)

#define STICK20_CMD_WRITE_STATUS_DATA                   (STICK20_CMD_START_VALUE +  8)
#define STICK20_CMD_ENABLE_READONLY_UNCRYPTED_LUN       (STICK20_CMD_START_VALUE +  9)
#define STICK20_CMD_ENABLE_READWRITE_UNCRYPTED_LUN      (STICK20_CMD_START_VALUE + 10)

#define STICK20_CMD_SEND_PASSWORD_MATRIX                (STICK20_CMD_START_VALUE + 11)
#define STICK20_CMD_SEND_PASSWORD_MATRIX_PINDATA        (STICK20_CMD_START_VALUE + 12)
#define STICK20_CMD_SEND_PASSWORD_MATRIX_SETUP          (STICK20_CMD_START_VALUE + 13)

#define STICK20_CMD_GET_DEVICE_STATUS                   (STICK20_CMD_START_VALUE + 14)
#define STICK20_CMD_SEND_DEVICE_STATUS                  (STICK20_CMD_START_VALUE + 15)

#define STICK20_CMD_SEND_HIDDEN_VOLUME_PASSWORD         (STICK20_CMD_START_VALUE + 16)
#define STICK20_CMD_SEND_HIDDEN_VOLUME_SETUP            (STICK20_CMD_START_VALUE + 17)

#define STICK20_CMD_SEND_PASSWORD                       (STICK20_CMD_START_VALUE + 18)
#define STICK20_CMD_SEND_NEW_PASSWORD                   (STICK20_CMD_START_VALUE + 19)

#define STICK20_CMD_CLEAR_NEW_SD_CARD_FOUND             (STICK20_CMD_START_VALUE + 20)
#define STICK20_CMD_SEND_STARTUP                        (STICK20_CMD_START_VALUE + 21)

#define STICK20_CMD_SEND_CLEAR_STICK_KEYS_NOT_INITIATED (STICK20_CMD_START_VALUE + 22)
#define STICK20_CMD_SEND_LOCK_STICK_HARDWARE            (STICK20_CMD_START_VALUE + 23)

#define STICK20_CMD_PRODUCTION_TEST                     (STICK20_CMD_START_VALUE + 24)
#define STICK20_CMD_SEND_DEBUG_DATA                     (STICK20_CMD_START_VALUE + 25)

/*
#define STICK20_CMD_SEND_PASSWORD_RETRY_COUNT           (STICK20_CMD_START_VALUE + 22)

#define STICK20_CMD_INIT_HIDDEN_VOLUME_SLOT             (STICK20_CMD_START_VALUE + 23)
#define STICK20_CMD_SAVE_HIDDEN_VOLUME_SLOT             (STICK20_CMD_START_VALUE + 24)
#define STICK20_CMD_READ_HIDDEN_VOLUME_SLOT             (STICK20_CMD_START_VALUE + 25)
*/


#define STICK20_SEND_STATUS_IDLE     0
#define STICK20_SEND_STATUS_PIN      1
#define STICK20_SEND_STARTUP         2
#define STICK20_SEND_PRODUCTION_TEST 3
#define STICK20_SEND_DEBUG_DATA      4


#define STATUS_READY                      0x00
#define STATUS_BUSY	                      0x01
#define STATUS_ERROR                      0x02
#define STATUS_RECEIVED_REPORT            0x03

#define CMD_STATUS_OK                     0
#define CMD_STATUS_WRONG_CRC              1
#define CMD_STATUS_WRONG_SLOT             2
#define CMD_STATUS_SLOT_NOT_PROGRAMMED    3
#define CMD_STATUS_WRONG_PASSWORD         4
#define CMD_STATUS_NOT_AUTHORIZED         5
#define CMD_STATUS_TIMESTAMP_WARNING      6
#define CMD_STATUS_NO_NAME_ERROR          7

/*
Output report
size	offset	description
1		0		device status
1		1		last command's type
4		2		last command's CRC
1		6		last command's status
53	7		last command's output
4		60		this report's CRC (with device status equal 0)
*/

#define OUTPUT_DEVICE_STATUS_OFFSET              0

// Quick hack for testing
/*
#define OUTPUT_LAST_CMD_TYPE_OFFSET              0
#define OUTPUT_CMD_CRC_OFFSET                    1
#define OUTPUT_CMD_STATUS_OFFSET                 5
#define OUTPUT_CMD_RESULT_OFFSET                 6
*/

#define OUTPUT_LAST_CMD_TYPE_OFFSET              1
#define OUTPUT_CMD_CRC_OFFSET                    2
#define OUTPUT_CMD_STATUS_OFFSET                 6
#define OUTPUT_CMD_RESULT_OFFSET                 7

#define OUTPUT_CMD_RESULT_STICK20_STATUS_START  20
#define OUTPUT_CMD_RESULT_STICK20_DATA_START    25
#define OUTPUT_CRC_OFFSET                       60


#define CMD_TYPE_OFFSET            0


#define STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ALL_VOL              0
#define STICK20_FILL_SD_CARD_WITH_RANDOM_CHARS_ENCRYPTED_VOL        1


#define OUTPUT_CMD_STICK20_STATUS_IDLE                      0
#define OUTPUT_CMD_STICK20_STATUS_OK                        1
#define OUTPUT_CMD_STICK20_STATUS_BUSY                      2
#define OUTPUT_CMD_STICK20_STATUS_WRONG_PASSWORD            3
#define OUTPUT_CMD_STICK20_STATUS_BUSY_PROGRESSBAR          4
#define OUTPUT_CMD_STICK20_STATUS_PASSWORD_MATRIX_READY     5
#define OUTPUT_CMD_STICK20_STATUS_NO_USER_PASSWORD_UNLOCK   6
#define OUTPUT_CMD_STICK20_STATUS_SMARTCARD_ERROR           7
#define OUTPUT_CMD_STICK20_STATUS_SECURITY_BIT_ACTIVE       8


typedef struct {
  u8  CommandCounter_u8;
  u8  LastCommand_u8;
  u8  Status_u8;
  u8  ProgressBarValue_u8;
} HID_Stick20Status_est;

void SetStick20Interface (u8 Command_u8);
void UpdateStick20Command (u8 Status_u8,u8 ProgressBarValue_u8);


#define OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS  20

typedef struct  {
  u8  PasswordLen_u8;
  u8  PasswordRowData_u8[OUTPUT_CMD_STICK20_MAX_MATRIX_ROWS];
} HID_Stick20SetupMatrix_est;


#define OUTPUT_CMD_STICK20_SEND_DATA_SIZE            25

#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_NONE        0
#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_DEBUG       1
#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_PW_DATA     2
#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_STATUS      3
#define OUTPUT_CMD_STICK20_SEND_DATA_TYPE_PROD_INFO   4


u8 Stick20HIDInitSendConfiguration (u8 state_u8);
u8 Stick20HIDSendAccessStatusData (u8 *output);
extern u8 Stick20HIDSendConfigurationState_u8;

typedef struct {
  unsigned char  SendCounter_u8;
  unsigned char  SendDataType_u8;
  unsigned char  FollowBytesFlag_u8;
  unsigned char  SendSize_u8;
  unsigned char  SendData_u8[OUTPUT_CMD_STICK20_SEND_DATA_SIZE];
} HID_Stick20SendData_est;

void Stick20HIDSendDebugData (u8 *output);

/* For Stick 20 - Matrix transfer*/
/*
#define OUTPUT_CMD_STICK20_MATRIX_DATA_SIZE            25

typedef struct {
  unsigned char  Counter_u8;
  unsigned char  BlockNo_u8;
  unsigned char  SendSize_u8;
  unsigned char  MatrixData_u8[OUTPUT_CMD_STICK20_SEND_DATA_SIZE];
} HID_Stick20Matrix_est;
*/


/*
CMD_WRITE_TO_SLOT

1b command type
1b slot number
15b slot name
20b	secret
1b configuration flags
12b token id
1b keyboard layout
8b counter
*/

#define CMD_WTS_SLOT_NUMBER_OFFSET    1
#define CMD_WTS_SLOT_NAME_OFFSET      2
#define CMD_WTS_SECRET_OFFSET        17
#define CMD_WTS_CONFIG_OFFSET        37
#define CMD_WTS_TOKEN_ID_OFFSET      38
#define CMD_WTS_COUNTER_OFFSET       51


/*
CMD_READ_SLOT

1b command type
1b slot number

*/

#define CMD_RS_SLOT_NUMBER_OFFSET     1

#define	CMD_RS_OUTPUT_COUNTER_OFFSET 34

/*
CMD_GET_CODE

report:
1b command type
1b slot number
8b challenge (for TOTP slot only)

output:
4b generated OTP
1b config flags
13b tokenID
	
*/

#define CMD_GC_SLOT_NUMBER_OFFSET      1
#define CMD_GC_CHALLENGE_OFFSET        2
#define CMD_GC_TIMESTAMP_OFFSET       10
#define CMD_GC_INTERVAL_OFFSET        18

/*
CMD_WRITE_CONFIG	

report:
1b command type
1b Numlock slot
1b Capslock slot
1b Scrolllock slot

output:

	
*/
	
	
/*
CMD_FIRST_AUTHENTICATE

report:
1b command type
25b card password
25b new temporary password

*/


/*
CMD_AUTHORIZE

report:
1b command type
4b authorized crc
25b temporary password


*/

void StartStick20Command (u8 Command_u8);

extern u8 OTP_device_status;
extern  u8 temp_password[25];
extern  u8 tmp_password_set;
extern  u32 authorized_crc;

u8 parse_report(u8 *report,u8 *output);
u8 cmd_get_status(u8 *report,u8 *output);
u8 cmd_write_to_slot(u8 *report,u8 *output);
u8 cmd_read_slot_name(u8 *report,u8 *output);
u8 cmd_read_slot(u8 *report,u8 *output);
u8 cmd_get_code(u8 *report,u8 *output);
u8 cmd_write_config(u8 *report,u8 *output);
u8 cmd_erase_slot(u8 *report,u8 *output);
u8 cmd_first_authenticate(u8 *report,u8 *output);
u8 cmd_authorize(u8 *report,u8 *output);
u8 cmd_get_password_retry_count(u8 *report,u8 *output);
u8 cmd_get_user_password_retry_count(u8 *report,u8 *output);
u8 cmd_user_authenticate(u8 *report,u8 *output);
u8 cmd_user_authorize(u8 *report,u8 *output);
u8 cmd_set_time(u8 *report,u8 *output);
u8 cmd_test_counter(u8 *report,u8 *output);
u8 cmd_test_time(u8 *report,u8 *output);
u8 cmd_getFactoryReset (u8 *report,u8 *output);

u8 cmd_getPasswordCount (u8 *report,u8 *output);
u8 cmd_getPasswordSafeStatus (u8 *report,u8 *output);
u8 cmd_getPasswordSafeSlotName (u8 *report,u8 *output);
u8 cmd_getPasswordSafeSlotPassword (u8 *report,u8 *output);
u8 cmd_getPasswordSafeSlotLoginName (u8 *report,u8 *output);
u8 cmd_getPasswordSafeSetSlotData_1 (u8 *report,u8 *output);
u8 cmd_getPasswordSafeSetSlotData_2 (u8 *report,u8 *output);
u8 cmd_getPasswordSafeEraseSlot (u8 *report,u8 *output);
u8 cmd_getPasswordSafeEnable (u8 *report,u8 *output);
u8 cmd_getPasswordSafeInitKey (u8 *report,u8 *output);
u8 cmd_getPasswordSafeSendData (u8 *report,u8 *output);
u8 cmd_unlock_userpassword (u8 *report,u8 *output);
u8 cmd_lock_device (u8 *report,u8 *output);
u8 cmd_getSdCardHighWaterMark (u8 *report,u8 *output);

void OTP_main (void);

#define CMD_DATA_OFFSET 0x01

#define KEYBOARD_FEATURE_COUNT                64

extern u8 HID_SetReport_Value[KEYBOARD_FEATURE_COUNT];
extern u8 HID_GetReport_Value[KEYBOARD_FEATURE_COUNT];

extern u8 HID_SetReport_Value_tmp[KEYBOARD_FEATURE_COUNT];
extern u8 HID_GetReport_Value_tmp[KEYBOARD_FEATURE_COUNT];


u8 Stick20HIDInitSendMatrixData (u8 *PasswordMatrixData_au8);

u8 CheckSystemtime (u32 Newtimestamp_u32);

