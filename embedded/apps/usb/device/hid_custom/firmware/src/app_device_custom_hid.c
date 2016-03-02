/********************************************************************
 Software License Agreement:

 The software supplied herewith by Microchip Technology Incorporated
 (the "Company") for its PIC(R) Microcontroller is intended and
 supplied to you, the Company's customer, for use solely and
 exclusively on Microchip PIC Microcontroller products. The
 software is owned by the Company and/or its supplier, and is
 protected under applicable copyright laws. All rights are reserved.
 Any use in violation of the foregoing restrictions may subject the
 user to criminal sanctions under applicable laws, as well as to
 civil liability for the breach of the terms and conditions of this
 license.

 THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,
 WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *******************************************************************/

/** INCLUDES *******************************************************/
#include "usb.h"
#include "usb_device_hid.h"

#include <string.h>

#include "system.h"

#define	MAJOR_VERSION	3
#define	MINOR_VERSION	2
#define COMPILE_DAY		05
#define	COMPILE_MONTH	7
#define COMPILE_YEAR	13

/** VARIABLES ******************************************************/
/* Some processors have a limited range of RAM addresses where the USB module
 * is able to access.  The following section is for those devices.  This section
 * assigns the buffers that need to be used by the USB module into those
 * specific areas.
 */
#if defined(FIXED_ADDRESS_MEMORY)
    #if defined(COMPILER_MPLAB_C18)
        #pragma udata HID_CUSTOM_OUT_DATA_BUFFER = HID_CUSTOM_OUT_DATA_BUFFER_ADDRESS
        unsigned char ReceivedDataBuffer[64];
        #pragma udata HID_CUSTOM_IN_DATA_BUFFER = HID_CUSTOM_IN_DATA_BUFFER_ADDRESS
        unsigned char ToSendDataBuffer[64];
        #pragma udata

    #else defined(__XC8)
        unsigned char ReceivedDataBuffer[64] @ HID_CUSTOM_OUT_DATA_BUFFER_ADDRESS;
        unsigned char ToSendDataBuffer[64] @ HID_CUSTOM_IN_DATA_BUFFER_ADDRESS;
    #endif
#else
    unsigned char ReceivedDataBuffer[64];
    unsigned char ToSendDataBuffer[64];
#endif

volatile USB_HANDLE USBOutHandle;    
volatile USB_HANDLE USBInHandle;

/** DEFINITIONS ****************************************************/
typedef enum
{
    COMMAND_TOGGLE_LED = 0x80,
    COMMAND_GET_BUTTON_STATUS = 0x81,
    COMMAND_READ_POTENTIOMETER = 0x37
} CUSTOM_HID_DEMO_COMMANDS;

/** FUNCTIONS ******************************************************/

/*********************************************************************
* Function: void APP_DeviceCustomHIDInitialize(void);
*
* Overview: Initializes the Custom HID demo code
*
* PreCondition: None
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_DeviceCustomHIDInitialize()
{
    //initialize the variable holding the handle for the last
    // transmission
    USBInHandle = 0;

    //enable the HID endpoint
    USBEnableEndpoint(CUSTOM_DEVICE_HID_EP, USB_IN_ENABLED|USB_OUT_ENABLED|USB_HANDSHAKE_ENABLED|USB_DISALLOW_SETUP);

    //Re-arm the OUT endpoint for the next packet
    USBOutHandle = (volatile USB_HANDLE)HIDRxPacket(CUSTOM_DEVICE_HID_EP,(uint8_t*)&ReceivedDataBuffer,64);
}

/*********************************************************************
* Function: void APP_DeviceCustomHIDTasks(void);
*
* Overview: Keeps the Custom HID demo running.
*
* PreCondition: The demo should have been initialized and started via
*   the APP_DeviceCustomHIDInitialize() and APP_DeviceCustomHIDStart() demos
*   respectively.
*
* Input: None
*
* Output: None
*
********************************************************************/
void APP_DeviceCustomHIDTasks()
{   
    //Check if we have received an OUT data packet from the host
    if(HIDRxHandleBusy(USBOutHandle) == false)
    {   
        //We just received a packet of data from the USB host.
        //Check the first uint8_t of the packet to see what command the host
        //application software wants us to fulfill.
        switch(ReceivedDataBuffer[0])				//Look at the data the host sent, to see what kind of application specific command it sent.
        {
            case 0x90:	// Control relays
        {
                if( 0 != ReceivedDataBuffer[1] )
                {
                        LATCbits.LATC0 = 1;		// Zoom in
                }
                else
                {
                        LATCbits.LATC0 = 0;
                }
                if( 0 != ReceivedDataBuffer[2] )
                {
                        LATCbits.LATC2 = 1;		// Zoom out
                }
                else
                {
                        LATCbits.LATC2 = 0;
                }
                break;
            }

            case 0xFB:	// Write capabilities
            {
                //EECON1bits.WREN = 1;			// Enable EEPROM writes
                //EEPROM_Write( 16, ReceivedDataBuffer[1] );	// PanTilt version
                //EEPROM_Write( 17, ReceivedDataBuffer[2] );	// Relay version
                //EEPROM_Write( 18, ReceivedDataBuffer[3] );	// LANC version
                //EEPROM_Write( 19, ReceivedDataBuffer[4] );	// Reserved
                //EEPROM_Write( 20, ReceivedDataBuffer[5] );	// Reserved
                //EEPROM_Write( 21, ReceivedDataBuffer[6] );	// Reserved
                //EEPROM_Write( 22, ReceivedDataBuffer[7] );	// Reserved
                //EEPROM_Write( 23, ReceivedDataBuffer[8] );	// Reserved
                //EECON1bits.WREN = 0;			// Prevent further writes
                break;
            }

            case 0xFC:	// Request capabilities.  Capabilities are stored in EEPROM because all PTS have the
                        // same software but hardware functionality varies
            {
                ToSendDataBuffer[0] = 0xFC;
                ToSendDataBuffer[1] = 0x01;
                ToSendDataBuffer[2] = 1;	// PanTilt version
                ToSendDataBuffer[3] = 1;	// Relay version
                ToSendDataBuffer[4] = 0;	// LANC version
                ToSendDataBuffer[5] = 0;	// Reserved
                ToSendDataBuffer[6] = 0;	// Reserved
                ToSendDataBuffer[7] = 0;	// Reserved
                ToSendDataBuffer[8] = 0;	// Reserved
                ToSendDataBuffer[9] = 0;	// Reserved
                if(!HIDTxHandleBusy(USBInHandle))
                {
                    USBInHandle = USBTransferOnePacket(CUSTOM_DEVICE_HID_EP,IN_TO_HOST,(uint8_t*)&ToSendDataBuffer[0],10);
                }
                break;
            }

            case 0xFD:	// Write serial number
            {
                //EECON1bits.WREN = 1;			// Enable EEPROM writes
                //EEPROM_Write( 0, ReceivedDataBuffer[1] );
                //EEPROM_Write( 1, ReceivedDataBuffer[2] );
                //EEPROM_Write( 2, ReceivedDataBuffer[3] );
                //EEPROM_Write( 3, ReceivedDataBuffer[4] );
                //EEPROM_Write( 4, ReceivedDataBuffer[5] );
                //EEPROM_Write( 5, ReceivedDataBuffer[6] );
                //EEPROM_Write( 6, ReceivedDataBuffer[7] );
                //EEPROM_Write( 7, ReceivedDataBuffer[8] );
                //EECON1bits.WREN = 0;			// Prevent further writes
                break;
            }

            case 0xFE:	// Request software version
            {
                ToSendDataBuffer[0] = 0xFE;
                ToSendDataBuffer[1] = 0x01;
                ToSendDataBuffer[2] = MAJOR_VERSION;				// Major part of version
                ToSendDataBuffer[3] = MINOR_VERSION;				// Minor part of version
                ToSendDataBuffer[4] = COMPILE_YEAR;					// Compile date year
                ToSendDataBuffer[5] = COMPILE_MONTH;				// Compile date month
                ToSendDataBuffer[6] = COMPILE_DAY;					// Compile date day
                ToSendDataBuffer[7] = 0;
                ToSendDataBuffer[8] = 0;
                ToSendDataBuffer[9] = 0;
                if(!HIDTxHandleBusy(USBInHandle))
                {
                    USBInHandle = USBTransferOnePacket(CUSTOM_DEVICE_HID_EP,IN_TO_HOST,(uint8_t*)&ToSendDataBuffer[0],10);
                }
                break;
            }

            case 0xFF:	// Request serial number
            {
                ToSendDataBuffer[0] = 0xFF;
                ToSendDataBuffer[1] = 0x01;
                ToSendDataBuffer[2] = 'A';
                ToSendDataBuffer[3] = 'B';
                ToSendDataBuffer[4] = 'C';
                ToSendDataBuffer[5] = 'D';
                ToSendDataBuffer[6] = 'E';
                ToSendDataBuffer[7] = 'F';
                ToSendDataBuffer[8] = 'G';
                ToSendDataBuffer[9] = 'H';
                if(!HIDTxHandleBusy(USBInHandle))
                {
                    USBInHandle = USBTransferOnePacket(CUSTOM_DEVICE_HID_EP,IN_TO_HOST,(uint8_t*)&ToSendDataBuffer[0],10);
                }
                break;
            }

        }
        //Re-arm the OUT endpoint, so we can receive the next OUT data packet 
        //that the host may try to send us.
        USBOutHandle = HIDRxPacket(CUSTOM_DEVICE_HID_EP, (uint8_t*)&ReceivedDataBuffer, 64);
    }
}