/*
* Author: Copyright (C) Rudolf Boeddeker  Date: 24.11.2010
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



#ifndef USB_CCID_H_
#define USB_CCID_H_

#define TRUE 						1
#define FALSE           0


#define CCID_MAX_XFER_LENGTH 	270


#define CCID_NO_ERROR 			       	0
#define CCID_ERROR_BAD_SLOT		      1
#define CCID_ERROR_BAD_LENTGH   	  2
#define CCID_ERROR_HW_ERROR     	  3
#define CCID_ERROR_CMD_ABORTED   	  4

#define CCID_OFFSET_MESSAGE_TYPE		0
#define CCID_OFFSET_LENGTH		  		1
#define CCID_OFFSET_SLOT		    	  5
#define CCID_OFFSET_SEQ   	   			6
#define CCID_OFFSET_STATUS 	   			7
#define CCID_OFFSET_ERROR  	   			8
#define CCID_OFFSET_CLOCK_STATUS		9
#define CCID_OFFSET_PROTOCOL_NUM 		9
#define CCID_OFFSET_CHAIN_PARAMETER 9

#define CCID_OFFSET_POWER_ON_POWER_SELECT   		7

#define CCID_OFFSET_XFR_BLOCK_BWI			   		    7
#define CCID_OFFSET_XFR_BLOCK_LEVEL_PARAMETER		8
#define CCID_OFFSET_XFR_BLOCK_DATA				     10

#define CCID_OFFSET_SET_PARAMS_DATA				     10

#define CCID_OFFSET_SLOT_STATUS_STATUS         7
#define CCID_OFFSET_SLOT_STATUS_CLOCK_STATUS   9


#define PC_TO_RDR_ICCPOWERON							                 0x62
#define PC_TO_RDR_ICCPOWEROFF							                 0x63
#define PC_TO_RDR_GETSLOTSTATUS								             0x65
#define PC_TO_RDR_XFRBLOCK   								               0x6F
#define PC_TO_RDR_GETPARAMETERS								             0x6C
#define PC_TO_RDR_RESETPARAMETERS							             0x6D
#define PC_TO_RDR_SETPARAMETERS								             0x61
#define PC_TO_RDR_ESCAPE									                 0x6B
#define PC_TO_RDR_ICCCLOCK								                 0x6E
#define PC_TO_RDR_T0APDU									                 0x6A
#define PC_TO_RDR_SECURE									                 0x69
#define PC_TO_RDR_MECHANICAL							                 0x71
#define PC_TO_RDR_ABORT										                 0x72
#define PC_TO_RDR_SET_DATA_RATE_AND_CLOCK_FREQUENCY		     0x73

#define RDR_TO_PC_DATA_BLOCK											         0x80
#define RDR_TO_PC_SLOT_STATUS											         0x81
#define RDR_TO_PC_PARAMETERS											         0x82
#define RDR_TO_PC_ESCAPE											             0x83
#define RDR_TO_PC_DATA_RATE_AND_CLOCK_FREQUNCY    				 0x84

#define CCID_CONTROL_ABORT                                 0x01
#define CCID_CONTROL_GET_CLOCK_FREQUENCIES                 0x02
#define CCID_CONTROL_GET_DATA_RATES                        0x03

#define CCID_SLOT_STATUS_PRESENT_ACTIVE       0
#define CCID_SLOT_STATUS_PRESENT_INACTIVE     1
#define CCID_SLOT_STATUS_NOT_PRESENT          2

#define CCID_SLOT_STATUS_CLOCK_RUNNING    		0
#define CCID_SLOT_STATUS_CLOCK_STOPPED_LOW		1
#define CCID_SLOT_STATUS_CLOCK_STOPPED_HIGH		2
#define CCID_SLOT_STATUS_CLOCK_UNKNOWN     		3

#define CCID_OFFSET_SET_PARAMS_DATA				   10


#define USB_CCID_MAX_LENGTH                     CCID_MAX_XFER_LENGTH // 65

typedef struct
{
	char	 USB_data[CCID_MAX_XFER_LENGTH];
	int 	 CCID_datalen;
	char	 CCID_CMD_aborted;
} t_USB_CCID_data_st;


void USB_CCID_SetLockCounter (u32 Value_u32);
void USB_CCID_DecLockCounter (void);
u32 USB_CCID_GetLockCounter (void);

void USB_to_CRD_DispatchUSBMessage_v (t_USB_CCID_data_st *USB_CCID_data_pst);
u8 CCID_GetSlotStatus_u8 (void);
u8 CCID_RestartSmartcard_u8 (void);
u8 CCID_InternalSmartcardOff_u8 (void);

#endif /* USB_CCID_H_ */
