/*  BHPMODAPI.C                                                   
 
    This header file forms an API library for the BackHauler PMOD as interfaced on USB over
    the SiLabs USBExpress libraries.  
*/

#define STRICT
#define _CRT_SECURE_NO_WARNINGS

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <memory.h>
#include <windows.h>
#include <winbase.h>
#pragma hdrstop
#include "SiUSBXp.h"
// Headers for this API
#include "..\bhpmodicd.h"
#include "bhpmodapi.h"

/*--------------------------------------------------------------------------*/
/* DLL Private Functions And Data Declarations                              */
/*--------------------------------------------------------------------------*/

/* Primitive hardware functions */
DWORD HW_Open(void);
DWORD HW_Close(void);
DWORD HW_SendDeviceCommand(BYTE Token, BYTE Count, void *DataMessage);
DWORD HW_GetDeviceResponse(BYTE *Token, BYTE *Count, void *DataMessage);

/* General purpose subroutine declarations */
DWORD GetStatusResponse(BYTE *Status);
DWORD ErrorMessage(char *ErrorDescription, char *ErrorType);
#define ErrorNoDevice()             ErrorMessage("Device not properly identified", "API Parameter Error")
#define ErrorFileNotFound()         ErrorMessage("Unable to open the specified file", "API Parameter Error")
#define ErrorBadValue()             ErrorMessage("Specified value is out of range", "API Parameter Error")
#define ErrorBadLength()            ErrorMessage("Requested length is not valid", "API Parameter Error")
#define ErrorBadAddress()           ErrorMessage("Bad offset or length given", "API Parameter Error")
#define ErrorNullPointer()          ErrorMessage("Data pointer provided is NULL", "API Parameter Error")
#define ErrorBadLengthReturned()    ErrorMessage("A data read operation did not recieve the expected number of bytes")
#define ErrorInternal()             ErrorMessage("The USB driver has reported an error", "Device Communication Error")

/* Local data definitions */

// General purpose message buffer
char UserMsg[1000];

// Driver handle to the device
HANDLE hBHPMOD = INVALID_HANDLE_VALUE;                               

/*--------------------------------------------------------------------------*/
/* System Level Functions                                                   */
/*--------------------------------------------------------------------------*/

/* DLLMAIN is the main program for a DLL.  It is executed when the DLL is 
   first loaded for each process and again when it is unloaded.  This routine
   must return a TRUE/FALSE success/fail result.  The routine can be used to 
   clear global variables for each new process.  Each process will have its 
   own data memory.  

   Note - Newer windows environments are less kind about FALSE responses 
   from this routine.  This seems to be because a failed load previously 
   threw a common error where as it now causes some kind of windows exception.
   Therefore, do not return false simply due to failure to open or
   similar level issues, but be sure that users of the relevant data points
   (such as an open handle) do check for valid.  An application can still 
   exit based on failed response associated with such a call.
*/

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
 {
  // MessageBox(NULL, "DllMain", "", MB_TASKMODAL);

  switch(fdwReason)
   {
    /* The DLL is attaching to a process due to process initialization or a call to LoadLibrary */
    case DLL_PROCESS_ATTACH:
     DisableThreadLibraryCalls(hinstDLL); // No need for thread attach/detach notifications.
     // MessageBox(NULL, "DllMain DLL_PROCESS_ATTACH", "", MB_TASKMODAL);
     // Attempt to open the device
     HW_Open();
     break;

    /* The attached process creates a new thread */
    case DLL_THREAD_ATTACH:
     // MessageBox(NULL, "DllMain DLL_THREAD_ATTACH", "", MB_TASKMODAL);
     break;

    /* The thread of the attached process terminates */
    case DLL_THREAD_DETACH:
     // MessageBox(NULL, "DllMain DLL_THREAD_DETACH", "", MB_TASKMODAL);
     break;

    /* The DLL is detaching from a process due to process termination or a call to FreeLibrary */
    case DLL_PROCESS_DETACH:
     // MessageBox(NULL, "DllMain DLL_PROCESS_DETACH", "", MB_TASKMODAL);
     HW_Close();
     break;

    default:
     break;
   };

  // If we get here, return success
  return(TRUE);
 } 

/*--------------------------------------------------------------------------*/
/* Exported Functions                                                       */
/*--------------------------------------------------------------------------*/

/* BHPMOD_GETSTATUS requests and returns the status of the device as reported
   in a status message.  The status is returned by value as interpreted by
   the ICD.  A 1/0 pass/fail indication is returned by value.
*/

DCAPI BHPMOD_GetStatus(BYTE *Status)
 {
  // Send the command
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_GET_STATUS, 0, NULL)) return(0);

  // Get the response and return the status
  return(GetStatusResponse(Status));
 } 

/*--------------------------------------------------------------------------*/

/* BHPMOD_SETCONFIGURATION allows the caller to establish the connectivity
   of the PMOD connector.  This will generally affect the usability of 
   various pins, which the caller should consider separately.
*/

DCAPI BHPMOD_SetConfiguration(BYTE Configuration)
 {
  BYTE status;
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_PMOD_SET_CONFIGURATION, 1, &Configuration)) return(0);
  GetStatusResponse(&status);
  return((status & STATUS_BIT_ERROR) ? 0 : 1);
 }

/*--------------------------------------------------------------------------*/

/* BHPMOD_SETPINDRIVE allows the drive configuration for as specific pin 
   to be established as push-pull or not push-pull (1/0).  If not push-pull,
   the pin can be used as an input, passively pulled high, or driven low. 
   The call will tend to be a success, but the actual functionality may depend 
   on the present configuration of the PMOD interface.  

   The software stack accepts values other than 0 as being 1, so the caller 
   can for example usually cast a boolean to the argument and get the same
   end result.
*/

DCAPI BHPMOD_SetPinDrive(BYTE PinNumber, BYTE PushPull)
 {
  BYTE buf[4];
  BYTE status;
  buf[0] = PinNumber;
  buf[1] = PushPull;
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_PMOD_SET_PIN_DRIVE, 2, buf)) return(0);
  GetStatusResponse(&status);
  return((status & STATUS_BIT_ERROR) ? 0 : 1);
 }

/*--------------------------------------------------------------------------*/

/* BHPMOD_SETPINSTATE allows an individual pin to be written.  The call will
   tend to be a success, but the actual functionality may depend on the 
   present configuration of the PMOD interface.

   The software stack accepts values other than 0 as being 1, so the caller 
   can for example usually cast a boolean to the argument and get the same
   end result.
*/

DCAPI BHPMOD_SetPinState(BYTE PinNumber, BYTE State)
 {
  BYTE buf[4];
  BYTE status;
  buf[0] = PinNumber;
  buf[1] = State;
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_PMOD_WRITE_PIN, 2, buf)) return(0);
  GetStatusResponse(&status);
  return((status & STATUS_BIT_ERROR) ? 0 : 1);
 }

/*--------------------------------------------------------------------------*/

/* BHPMOD_GETPINSTATE retrieves the present state of a given pin.  

*/

DCAPI BHPMOD_GetPinState(BYTE PinNumber, BYTE *State)
 {
  BYTE token, cnt, val;

  // Check arguments not checked internally
  if(State == NULL) return(0);

  // Send command
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_PMOD_READ_PIN, 1, &PinNumber)) return(0);

  // Get the response, which should be the associated response
  cnt = 1;
  if(!HW_GetDeviceResponse(&token, &cnt, &val)) return(ErrorInternal());
  // If the token is something other than expected, that is a failure
  // By the ICD, a normal command failure would return status with the error bit set
  // Any other token would be an even more severe failure
  // All are handled here
  if(token != TOKEN_RESPONSE_PMOD_READ_PIN) return(0);
  if(cnt != 1) return(0);

  // Return the response we got
  *State = val;

  // Return success
  return(1); 
 }

/*--------------------------------------------------------------------------*/

/* BHPMOD_SPI_SETCLOCKPHASE sets the clock to data phase relationshpi for any
   transactions on the SPI bus.  This routine can be executed anytime, but
   it is only useful when using SPI.
*/

DCAPI BHPMOD_SPI_SetClockPhase(BYTE ClockPhase)
 {
  BYTE status;
 
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_SET_SPI_CLOCK_PHASE, 1, &ClockPhase)) return(0);
  GetStatusResponse(&status);
  return((status & STATUS_BIT_ERROR) ? 0 : 1);
 }

/*--------------------------------------------------------------------------*/

/* BHPMOD_SPI_TRANSACTION completes a standard transaction on the local
   PMOD SPI bus using the provided data and returns the data received 
   in the same buffer. This routine allows at most 62 bytes.  The count
   is returned by reference and will differ from the count sent only if
   there was a problem.  This routine will not function properly if 
   the PMOD is not configured for SPI, but will indicate success just 
   with a zero data count.

   Remember that the content buffer must be large enough to handle the 
   count specified or this routine will tend to fail badly. This could
   be improved in a number of ways using more advanced Windows coding.
   The count is limited to 62, but because the count is returned, a 
   larger count is allowed and simply changed, reducing the need to 
   defined this limit somewhere.
*/

DCAPI BHPMOD_SPI_Transaction(BYTE *Count, BYTE *Buffer)
 {
  BYTE token, cnt;
  BYTE buf[70];

  // Check arguments
  if(Buffer == NULL) return(0);
  if(Count == NULL) return(0);
  if(*Count > 62) *Count = 62;

  // Send command
  if(!HW_SendDeviceCommand(TOKEN_COMMMAND_SPI_TRANSACTION, *Count, Buffer)) { *Count = 0; return(0); };

  // Get the response, which should be SPI data
  // Expected count is the same number of bytes we sent - SPI has a ring shift register nature
  // Yet other counts might be expected if there is an error, so we want to know that and act accordingly
  cnt = *Count;
  if(!HW_GetDeviceResponse(&token, &cnt, buf)) { *Count = 0; return(ErrorInternal()); };
 
  // If the token is something other than SPI data, that is a failure
  // By the ICD, a normal command failure would return status with the error bit set
  // Any other token would be an even more severe failure
  // All failures mean no data and failure response
  if(token != TOKEN_RESPONSE_SPI_TRANSACTION) { *Count = 0; return(0); };
 
  // We got the read response
  *Count = cnt;
  if(cnt > 0) memcpy(Buffer, &buf, cnt);

  // Return success
  return(1); 
 }

/*--------------------------------------------------------------------------*/

/* BHPMOD_I2C_WRITE translates directly to the same function on the embedded
   level of the BhPmod device.  The only limitation is that the subaddress if
   any is limited to two bytes, endian order change supported here for x86.  
   However, use of the subaddress is only required if a restart is required 
   by the device.  The return value is the count of bytes actually written 
   successfully, with 0 indicating some kind of error and other numbers up 
   to caller interpretation. No more than 57 bytes can be written at a time, 
   which is presumed to be not typical. SubAddr can be NULL if not used. 
   Content cannot be NULL.

   The function indicates success or failure in general with a 1/0 returned
   by value.  The Count of data actually transacted is returned by reference
   in the argument.  This routine will fail if the PMOD connector is not 
   configured for I2C, but it will show success and just return no data.
  
   The embedded code will return all data that have been written for consistent
   performance with other similar transactions.  But unlike the others, the 
   the content written is not updated on return from this write only routine.  
   Just the associated count is updated to indicate what was actually done.
*/

DCAPI BHPMOD_I2C_Write(BYTE Address, BYTE SubAddrSize, void *SubAddr, BYTE *Count, void *Content)
 {
  BYTE token, cnt;
  BYTE buf[70];
  BYTE *subaddress = (BYTE *)SubAddr;

  // Check arguments
  if(SubAddrSize > 2) return(0);
  if((SubAddrSize > 0) && (SubAddr == NULL)) return(0); 
  if(Count == NULL) return(0);
  if(*Count > 57) return(0);
  if(Content == NULL) return(0);

  // Send command
  buf[0] = Address;
  buf[1] = SubAddrSize;
  buf[2] = (SubAddrSize == 1) ? subaddress[0] : 0;
  if(SubAddrSize == 2) buf[2] = subaddress[1]; 
  buf[3] = (SubAddrSize == 2) ? subaddress[0] : 0;
  buf[4] = *Count;
  memcpy(&buf[5], Content, *Count);
  cnt = *Count + 5;
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_I2C_WRITE, cnt, buf)) { *Count = 0; return(0); };

  // Get the response, which will be I2C data as previously written
  if(!HW_GetDeviceResponse(&token, &cnt, &buf)) { *Count = 0; return(ErrorInternal()); };

  // If the token is something other than I2C data, that is a failure
  // By the ICD, a normal command failure would return status with the error bit set
  // Any other token would be an even more severe failure
  // All failures mean no data and failure response
  if(token != TOKEN_RESPONSE_I2C_WRITE) { *Count = 0; return(0); };

  // Update the count actually written based on the response
  *Count = buf[4];

  // Return success
  return(1);
 }

/*--------------------------------------------------------------------------*/

/* BHPMOD_I2C_READ translates directly to the same function on the embedded
   level of the BhPmod device.  Comments are the same as for write in the dual.
   However, in this case the content read is returned.
*/

DCAPI BHPMOD_I2C_Read(BYTE Address, BYTE SubAddrSize, void *SubAddr, BYTE *Count, void *Content)
 {
  BYTE token, cnt;
  BYTE buf[70];
  BYTE *subaddress = (BYTE *)SubAddr;

  // Check arguments
  if(SubAddrSize > 2) return(0);
  if((SubAddrSize > 0) && (SubAddr == NULL)) return(0); 
  if(Count == NULL) return(0);
  if(*Count > 57) return(0);
  if(Content == NULL) return(0);
 
  // Send command
  buf[0] = Address;
  buf[1] = SubAddrSize;
  buf[2] = (SubAddrSize == 1) ? subaddress[0] : 0;
  if(SubAddrSize == 2) buf[2] = subaddress[1]; 
  buf[3] = (SubAddrSize == 2) ? subaddress[0] : 0;
  buf[4] = *Count;
  memcpy(&buf[5], Content, *Count);
  cnt = *Count + 5;
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_I2C_READ, cnt, buf)) { *Count = 0; return(0); };

  // Get the response, which will be SPI data
  if(!HW_GetDeviceResponse(&token, &cnt, &buf)) { *Count = 0; return(ErrorInternal()); };

  // If the token is something other than I2C data, that is a failure
  // By the ICD, a normal command failure would return status with the error bit set
  // Any other token would be an even more severe failure
  // All failures mean no data and failure response
  if(token != TOKEN_RESPONSE_I2C_READ) { *Count = 0; return(0); };

  // We got the read response
  // Update the count to indicate results
  *Count = buf[4];
  // If there is data, go return it
  if(*Count > 0) memcpy(Content, &buf[5], *Count);

  // Return success
  return(1);
 }

/*--------------------------------------------------------------------------*/

/* PHMOD_SERIAL_PRINT is an abstraction of the more generalized serial
   write function that allows the caller to simply provide a standard
   null terminated string to send to the serial port.  The string can
   be any length and this routine will send it in groups as required 
   by the write routine.  Locating the ending null character will 
   complete the write.  This routine does not send the null, and it 
   does not alter or filter any characters.  Considerations such as 
   effects on the receiver of CR versus CR/LF must be addressed in 
   some other way.

   Note that this approach contains aspects that are not considered 
   good form these days.  We like that it is simple and clear, which is
   what you need for an instrument that you fully control.  But consider
   things like what happens if the string is not properly terminated or
   contains other errors.  This is the kind of thing that hackers look for 
   to attack software.  Use with care.
*/

DCAPI BHPMOD_SERIAL_Print(char *Strz)
 {
  BYTE cnt, strcnt;
  BYTE buf[70];

  // Check argument
  if(Strz == NULL) return(0);
  if(Strz[0] == 0) return(0);
  
  // Construct and send packetized version
  // Return failure if we run into trouble
  // Yes, having an absolute number for the count limit is poor form, but
  // the only thing that cares about this is the next routine defined here only
  strcnt = 0;
  do
   {
    cnt = 0;
    while((Strz[strcnt] != 0) && (cnt < 62)) buf[cnt++] = Strz[strcnt++];
    if(!BHPMOD_SERIAL_Write(&cnt, buf)) return(0);
    if(cnt == 0) return(0);
   }
  while(Strz[strcnt] != 0);

  // Return success
  return(1);
 }

/*--------------------------------------------------------------------------*/

/* PHPMOD_SERIAL_WRITE allows a counted number of bytes to be written
   to the serial port.  This works if the PMOD is configured for serial.
   The Count returned by reference will be the number of bytes actually 
   written, which will depend on the buffer state and other factors.

   The device will return the full contents of data written per the
   requirements of the ICD, but because users of this routine may not 
   expect the content to be returned, the contents are not returned 
   to the argument.  The count is updated as prescribed above, and it 
   can be assumed that bytes actually written from the content are the 
   first in the argument.

   Remember that the content buffer must be large enough to handle the 
   count specified or this routine will tend to fail badly. This could
   be improved in a number of ways using more advanced Windows coding.
   The count is limited to 62, but because the count is returned, a 
   larger count is allowed and simply changed, reducing the need to 
   defined this limit somewhere.
*/

DCAPI BHPMOD_SERIAL_Write(BYTE *Count, BYTE *Content)
 {
  BYTE token, cnt;
  BYTE buf[70];

  // Check arguments
  if(Count == NULL) return(0);
  if(Content == NULL) return(0);
  if(*Count > 62) *Count = 62;

  // Send command
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_SERIAL_WRITE, *Count, Content)) { *Count = 0; return(0); };

  // Get the response, which should be serial data
  if(!HW_GetDeviceResponse(&token, &cnt, buf))  { *Count = 0; return(ErrorInternal()); };

  // If the token is something other that serial write data, that is a failure
  // By the ICD, a normal command failure would return status with the error bit set
  // Any other token would be an even more severe failure
  // All failures mean no data and failure response
  if(token != TOKEN_RESPONSE_SERIAL_WRITE) { *Count = 0; return(0); };

  // Return the resulting count
  *Count = cnt;

  // Return success
  return(1); 
 }

/*--------------------------------------------------------------------------*/

/* PHPMOD_SERIAL_READ allows a counted number of bytes to be read from
   the serial port.  This works if the PMOD is configured for serial.
   The Count returned by reference will be the number of bytes actually 
   read, which will depend on the buffer state and other factors.

   Remember that the content buffer must be large enough to handle the 
   count specified or this routine will tend to fail badly. This could
   be improved in a number of ways using more advanced Windows coding.
   The count is limited to 62, but because the count is returned, a 
   larger count is allowed and simply changed, reducing the need to 
   defined this limit somewhere.
*/

DCAPI BHPMOD_SERIAL_Read(BYTE *Count, BYTE *Content)
 {
  BYTE token, cnt;
  BYTE buf[70];

  // Check arguments
  if(Count == NULL) return(0);
  if(Content == NULL) return(0);
  if(*Count > 62) *Count = 62;
 
  // Send command
  if(!HW_SendDeviceCommand(TOKEN_COMMAND_SERIAL_READ, *Count, Content))  { *Count = 0; return(0); };

  // Get the response, which should be serial data
  if(!HW_GetDeviceResponse(&token, &cnt, buf)) { *Count = 0; return(ErrorInternal()); };

  // If the token is something other that serial write data, that is a failure
  // By the ICD, a normal command failure would return status with the error bit set
  // Any other token would be an even more severe failure
  // All failures mean no data and failure response
  if(token != TOKEN_RESPONSE_SERIAL_READ)  { *Count = 0; return(0); };

  // Return the resulting data
  *Count = cnt;
  memcpy(Content, buf, cnt);

  // Return success
  return(1); 
 }

/*--------------------------------------------------------------------------*/
/* Exported Test Functions                                                  */
/*--------------------------------------------------------------------------*/

/* BHPMOD_TESTCODE is an undefined routine where code can be written to test 
   various aspeBHPMOD of hardware or software.  A test number identifies one 
   of several tests if several are defined.  Arguments are strings representing
   unsigned long hex values so they must be at least 8 characters.  If a
   string is not needed, it may be NULL and its value will be 0.  If a string 
   is present, a full 8 characters will be written to it on return representing
   its ending value.
*/

DCAPI BHPMOD_TestCode(BYTE TestNumber, char *ArgStr1, char *ArgStr2)
 {
  DWORD Arg1, Arg2;
  BYTE test_packet[100];
  BYTE token = 0;
  BYTE count = 0;

  // MessageBox(NULL, "BHPMOD_TestCode", "", MB_TASKMODAL);

  // Get values in arguments
  if(ArgStr1 == NULL) Arg1 = 0; else sscanf(ArgStr1, "%08X", &Arg1);
  if(ArgStr2 == NULL) Arg2 = 0; else sscanf(ArgStr2, "%08X", &Arg2);

  // Load the test number in the first data byte - embedded test code would use this
  test_packet[0] = TestNumber;
  
  // Do the prescribed test
  switch(TestNumber)
   {
    case 0: 
     break;

    case 1:
     break;

    case 2:
     break;

    case 90:
     // Poke xdata
     test_packet[1] = HIBYTE(LOWORD(Arg1));
     test_packet[2] = LOBYTE(LOWORD(Arg1));
     test_packet[3] = LOBYTE(LOWORD(Arg2));
     HW_SendDeviceCommand(TOKEN_COMMAND_TEST, 4, test_packet);
     Arg2 = test_packet[3];
     break;

    case 91:
     // Peek xdata
     test_packet[1] = HIBYTE(LOWORD(Arg1));
     test_packet[2] = LOBYTE(LOWORD(Arg1));
     HW_SendDeviceCommand(TOKEN_COMMAND_TEST, 3, test_packet);
     count = 1;
     HW_GetDeviceResponse(&token, &count, test_packet);
     Arg2 = test_packet[0];
     break;

    case 99:
     // Change test mode
     test_packet[1] = LOBYTE(LOWORD(Arg1));
     HW_SendDeviceCommand(TOKEN_COMMAND_TEST, 2, test_packet);
     break;

    default:
     Arg1 = 0xBAD00001;
     Arg2 = 0xBAD00002;
     break;
   };

  // Write back given arguments
  if(ArgStr1 != NULL) sprintf(ArgStr1, "%08X", Arg1);
  if(ArgStr2 != NULL) sprintf(ArgStr2, "%08X", Arg2);

  // Return success
  return(1);
 } 

/*--------------------------------------------------------------------------*/
/* Hardware Support Functions                                               */
/*--------------------------------------------------------------------------*/

/* HW_OPEN identifies an attached BHPMOD and opens a handle to it.

*/

DWORD HW_Open(void)
 {
  DWORD NumDevices, devindex;
  WORD deviceid;
  BYTE device_found;
  char devidstr[100];
  SI_STATUS status;
  
  // MessageBox(NULL, "HW_Open", "", MB_TASKMODAL);

  // Set the driver default timeouts
  // Note that some operations may take a certain amount of time to
  // respond by design
  SI_SetTimeouts(2000, 2000);

  // Get the number of devices on the bus which use this driver
  status = SI_GetNumDevices(&NumDevices);
  if(status != SI_SUCCESS) return(ErrorMessage("Unable to access device driver", "Device Not Found"));

  // Look for the BHPMOD in the group
  devindex=0;
  do
   {
    device_found = 1;
    // Get and match the VID of a device
    SI_GetProductString(devindex, devidstr, SI_RETURN_VID);
    sscanf(devidstr, "%hX", &deviceid);
    if(deviceid != BHPMOD_VENDOR_ID) device_found = 0;
    // Get and match the PID of a device
    SI_GetProductString(devindex, devidstr, SI_RETURN_PID);
    sscanf(devidstr, "%hX", &deviceid);
    if(deviceid != BHPMOD_PRODUCT_ID) device_found = 0;

    // If this device is what we are looking for, try to open it
    if(device_found)
     {
      status = SI_Open(devindex, &hBHPMOD);
      if(status != SI_SUCCESS) return(ErrorMessage("Unable to open driver", "Device Not Found"));
      return(1);
     };

    // If not, then look at the next one
    devindex+=1;
   }
  while(devindex<NumDevices);

  // If we have exhausted the devices, then our device is not found, so return failure  
  return(ErrorMessage("BackHauler PMOD was not found", "Device Not Found"));
 } 

/*--------------------------------------------------------------------------*/

/* HW_CLOSE closes the device to which the handle was opened.

*/

DWORD HW_Close(void)
 {
  // MessageBox(NULL, "HW_Close", "", MB_TASKMODAL);

  if(hBHPMOD == INVALID_HANDLE_VALUE) return(1);
  SI_Close(hBHPMOD);
  hBHPMOD = INVALID_HANDLE_VALUE;
  return(1);
 } 

/*--------------------------------------------------------------------------*/

/* HW_SENDDEVICECOMMAND sends the specified command and data to the device 
   over the USB driver infrastructure.  A 1/0 pass/fail response is returned.
*/

DWORD HW_SendDeviceCommand(BYTE Token, BYTE Count, void *DataMessage)
 {
  SI_STATUS status;
  BYTE packet[64];
  DWORD WrCnt;

  // MessageBox(NULL, "HW_SendDeviceCommand", "", MB_TASKMODAL);

  // Check device
  if(hBHPMOD == INVALID_HANDLE_VALUE) return(0);  

  // Check arguments
  if(Count > 62) return(0);
  if((Count > 0) && (DataMessage == NULL)) return(0);

  // Construct packet
  packet[0] = Token;
  packet[1] = Count;
  memcpy(&packet[2], DataMessage, Count);

  // Send packet
  status = SI_Write(hBHPMOD, packet, Count+2, &WrCnt); 
  if(status != SI_SUCCESS) return(0);

  // Return success
  return(1);
 } 

/*--------------------------------------------------------------------------*/

/* HW_GETDEVICERESPONSE attempts to receive an ordinary command response.  
   The token and count will be read, followed by the message contents.  
   If the read was successful within the default timeout, then a 1 is 
   returned, otherwise a zero is returned.  The data are returned by 
   reference.  Up to 62 bytes can be returned in the data message; but 
   for convenience in passing variables of arbitrary type, the routine
   will not return more data than the count in the argument on entry.
   However, the count will be updated on exit to reflect the number of 
   bytes actually provided in the response packet.  This can help the
   caller interpret an unexpected response.  Less data can be returned
   than in the count on entry.  The full response is always cleared from 
   the USB pipe by this routine.
*/ 

DWORD HW_GetDeviceResponse(BYTE *Token, BYTE *Count, void *DataMessage)
 {
  SI_STATUS status;
  BYTE packet[64];
  DWORD RdCnt;
  BYTE limit_count = *Count;

  // MessageBox(NULL, "HW_GetDeviceResponse", "", MB_TASKMODAL);

  // Check device
  if(hBHPMOD == INVALID_HANDLE_VALUE) return(0);  

  // Check arguments
  if(Token == NULL) return(0);
  if(Count == NULL) return(0);
  if((*Count > 0) && (DataMessage == NULL)) return(0);

  // Attempt to collect an input packet token and count
  status = SI_Read(hBHPMOD, packet, 2, &RdCnt);
  if(status != SI_SUCCESS) return(0);
  *Token = packet[0];
  *Count = packet[1];
  // Attempt to collect the input data portion
  if(*Count > 0) 
   {
    status = SI_Read(hBHPMOD, packet, *Count, &RdCnt);
    if(status != SI_SUCCESS) return(0);
    if(*Count < limit_count) limit_count = *Count;
    memcpy(DataMessage, packet, limit_count);
   };

  // Return success
  return(1);
 } 

/*--------------------------------------------------------------------------*/
/* General Purpose And Helper Subroutines                                   */
/*--------------------------------------------------------------------------*/

/* GETSTATUSRESPONSE collects a status response from the device.  This will 
   generally wait up to the default timeout for a response to return, thus
   allowing a brief wait for completion.  If anything other than a status 
   is returned, a message is displayed.  The status byte is returned
   by reference.
*/

DWORD GetStatusResponse(BYTE *Status)
 {
  BYTE token, count, val;

  // Get the response 
  count = 1;
  if(!HW_GetDeviceResponse(&token, &count, &val)) return(ErrorInternal());

  // Make sure the response is a status
  if(token != TOKEN_RESPONSE_STATUS) 
   {
    MessageBox(NULL, "Unexpected device response in place of status", "Device Communication Error", MB_ICONSTOP | MB_TASKMODAL);
    return(0);
   };

  // Return the status
  *Status = val;
  
  // Return success 
  return(1);
 } 

/*--------------------------------------------------------------------------*/

/* ERRORMESSAGE notifies the user of an error specified by a text string.  
   A zero is returned for convenience.  
*/

DWORD ErrorMessage(char *ErrorDescription, char *ErrorType) 
 {
  MessageBox(NULL, ErrorDescription, ErrorType, MB_ICONSTOP | MB_TASKMODAL);
  return(0);
 } 

/*--------------------------------------------------------------------------*/

/* END OF MODULE */

