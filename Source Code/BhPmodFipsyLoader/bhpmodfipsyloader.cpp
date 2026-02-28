/*  BHPMODFIPSYLOADER.C                                                   
 
    This code file forms a library for loading the Fipsy module using 
    the BackHauler PMOD.  
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
// Headers for this API
#include "..\bhpmodicd.h"
#include "..\BhPmodApi\bhpmodapi.h"
#include "machxo2.h"
#include "bhpmodfipsyloader.h"

/*--------------------------------------------------------------------------*/
/* DLL Private Functions And Data Declarations                              */
/*--------------------------------------------------------------------------*/

/* JEDEC file parsing support subroutine declarations */
char JEDEC_SeekNextNonWhitespace(void);
char JEDEC_SeekNextKeyChar(void);
BYTE JEDEC_ReadFuseByte(BYTE *FuseByte);

/* General purpose subroutine declarations */
DWORD ErrorMessage(char *ErrorDescription, char *ErrorType);
// Predefined error messages
#define ErrorFileNotFound()         ErrorMessage("Unable to open the specified file", "File Error")
#define ErrorFileFormat()           ErrorMessage("File format is not valid for JEDEC", "File Error")
#define ErrorBadSetting()           ErrorMessage("JEDEC file has SPI slave port disabled - programming aborted", "File Error")
#define ErrorBadValue()             ErrorMessage("Specified value is out of range", "Parameter Error")
#define ErrorBadLength()            ErrorMessage("Requested length is not valid", "Parameter Error")
#define ErrorBadAddress()           ErrorMessage("Bad offset or length given", "Parameter Error")
#define ErrorNullPointer()          ErrorMessage("Data pointer provided is NULL", "Parameter Error")
#define ErrorNotErased()            ErrorMessage("The FPGA must be erased to program", "Operation Order Error")
#define ErrorNotOpen()              ErrorMessage("The SPI connection has not been initialized", "Operation Order Error")
#define ErrorTimeout()              ErrorMessage("Timed out waiting for FPGA busy", "Timeout Error")

/* Local data definitions */

// General purpose message buffer
char UserMsg[1000];

// General purpose buffer used to transact data on SPI
// This is bigger than most routines need, but reduces repeated declarations
// and is bigger than the actual SPI transaction can be, meaning there is 
// always enough room
BYTE SPIBuf[100];

// Count of bytes in the SPI buffer
// This is filled based on the count to send in a transaction
// On return from a transaction, this will specify the number of bytes returned
// Bytes returned is the entire SPI transaction, not just the data, so the value
// should not change unless something went wrong.
BYTE MachXO2_Count = 0;
 
// Macro to set the most frequently used dummy values for the SPI buffer
BYTE SPIBUF_DEFAULT[20] = { 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
#define SPIBUFINIT                  { MachXO2_Count = 0; memcpy(SPIBuf, SPIBUF_DEFAULT, 20); }

// Defines for key elements of SPI transactions
#define MachXO2_Command             SPIBuf[0]
#define pMachXO2_Operand            (&SPIBuf[1])
#define pMachXO2_Data               (&SPIBuf[4])

// Macro to complete an SPI transaction of the specified count with the global buffer
// The global buffer count is set with the parameter, saving coding steps 
#define MachXO2_SPITrans(c)         { MachXO2_Count = c; BHPMOD_SPI_Transaction(&MachXO2_Count, SPIBuf); }	 

// Flag indicating hardware has been opened and the port is ready
BYTE HWIsOpen = 0;

// Flag indicating that a caller has erased the FPGA
BYTE FPGAIsErased = 0;

// File stream pointer for JEDEC file 
// See comments with the parsing support functions
FILE *JFile = NULL;

// Predefined advanced error message macro including closing the file on a format error
#define EXIT_FILEFORMATERROR		{ fclose(JFile); return(ErrorFileFormat()); }
#define EXIT_FILEBADSETTINGERROR	{ fclose(JFile); return(ErrorBadSetting()); }

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

/* FIPSY_OPEN will establish a connection to the FPGA through the chosen
   SPI port, which in this case is the BackHauler PMOD.  This is typically 
   used to open hardware, which it effectively does by accessing the 
   BackHauler library, if the hardware is not already open, but also 
   sets the port for operation as required from here.
   
   More checking of return values is done here than in other routines, 
   with the idea that if things are working here they should continue to 
   work well in other places if the hardware remains connected and the 
   coding is correct.  This is executed first, so a failure here would
   mean that something is wrong with the hardware.
   
   A flag is set to indicate that the hardware has been opened.  Other
   implementations may have an actual hardware handle here, but that 
   aspect is handled separately in this case. 
*/

DCAPI Fipsy_Open(void)
 {  
  // MessageBox(NULL, "Fipsy_Open", "", MB_TASKMODAL);

  // Setup the hardware for SPI 
  // This will open the BackHauler PMOD if it is not already open
  if(!BHPMOD_SetConfiguration(PMOD_CONFIGURATION_SPI)) return(0); 
  if(!BHPMOD_SPI_SetClockPhase(SPI_CLOCK_PHASE_0)) return(0); 
  
  // Send a NOP to wakeup the device
  SPIBUFINIT;                  
  MachXO2_Command = MACHXO2_CMD_NOP;
  pMachXO2_Operand[0] = 0xFF;         
  pMachXO2_Operand[1] = 0xFF;         
  pMachXO2_Operand[2] = 0xFF;    
  MachXO2_SPITrans(4);
 	 
  // Set the open flag
  HWIsOpen = 1;
     
  // Return success
  return(1);
 }
 
/*--------------------------------------------------------------------------*/

/* FIPSY_CLOSE does nothing in this library except operate the flag 
   described above.  However, it is here for completeness since such a thing 
   may need to be implemented for ports of this code to other hardware.  
*/

DCAPI Fipsy_Close(void)
 {  
  // MessageBox(NULL, "Fipsy_Close", "", MB_TASKMODAL);

  // Clear the open flag
  HWIsOpen = 0;
 
  // Return success
  return(1);
 }
 
/*--------------------------------------------------------------------------*/

/* FIPSY_READDEVICEID retrieves the device identification number from the
   FPGA connected to the SPI port.  This number can be used to verify that
   the SPI is working and we are talking to the right device.  To improve
   future flexibility, this routine does not decide if this is actually
   the right device, but just returns the four bytes it got.
*/

DCAPI Fipsy_ReadDeviceID(BYTE *DeviceID)
 {
  // All exported library functions get this check of hardware and arguments    
  // MessageBox(NULL, "Fipsy_ReadDeviceID", "", MB_TASKMODAL);
  if(!HWIsOpen) return(ErrorNotOpen());
  if(DeviceID == NULL) return(ErrorNullPointer());
    
  // Construct the command  
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_READ_DEVICEID;
  MachXO2_SPITrans(8);
  
  // Get the data 
  memcpy(DeviceID, pMachXO2_Data, 4);
      
  // Return success
  return(1);
 }

/*--------------------------------------------------------------------------*/

/* FIPSY_READUNIQUEID retrieves the 8-byte unique identification code 
   from the FPGA connected to the SPI port.  This number can be used to 
   identify a specific chip, and therefore a specific assembly too.
   There is no specific use for this information in this context, but it 
   is is sometimes used alone or connection with the user id (if implemented)
   to form a type of serial number.  It can take the place of other chips
   and resources designed to do just this, so it might be a useful features
   to expose here.  The bytes are returned in a simple array.
*/

DCAPI Fipsy_ReadUniqueID(BYTE *UniqueID)
 {
  // All exported library functions get this check of hardware and arguments    
  // MessageBox(NULL, "Fipsy_ReadUniqueID", "", MB_TASKMODAL);
  if(!HWIsOpen) return(ErrorNotOpen());
  if(UniqueID == NULL) return(ErrorNullPointer());
    
  // Construct the command  
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_READ_UIDCODE;
  MachXO2_SPITrans(12);
  
  // Copy the data to the argument
  memcpy(UniqueID, pMachXO2_Data, 8);
      
  // Return success
  return(1);
 }

/*--------------------------------------------------------------------------*/

/* FIPSY_ERASEALL clears the configuration from all portions of the FPGA.  
   By this choice, the FPGA will return to its erased state function here.
   
   For this library, this is the first step to programming.  This is the
   function that enters the programming mode, so it must be completed 
   before the programming operation.  A global flag is set here to be sure
   that happens.  This removes the need to blank check elsewhere.
   
   It is possible that in the future we will want to erase just part of 
   the FPGA configuration or do some other more elaborate things, but this
   is not contemplated here other than in the name of this function.
*/   

DCAPI Fipsy_EraseAll(void)
 {
  BYTE busy = 0x80;
  DWORD timeout = 0;

  // All exported library functions get this check of hardware and arguments    
  // MessageBox(NULL, "Fipsy_EraseAll", "", MB_TASKMODAL);
  if(!HWIsOpen) return(ErrorNotOpen());
 
  // Since we are now messing with the FPGA, indicate its status is not erased
  FPGAIsErased = 0;
 
  // Send command to enter programming mode
  // We use only offline mode
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_ENABLE_OFFLINE;
  pMachXO2_Operand[0] = 0x08;         
  MachXO2_SPITrans(4);
  
  // Apparently this mode change takes a brief moment, ok to just wait it out here
  Sleep(1);
  
  // Send command to erase everything
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_ERASE;
  pMachXO2_Operand[0] = 0x0F;         
  MachXO2_SPITrans(4);
  
  // Look at busy status every so often until it is clear or until too much time goes by
  // The busy bit is in the MSB, but still means we can test nonzero
  do
   { 
    // Do a wait between polls of the busy bit
    Sleep(100);
    timeout += 100;
    if(timeout > 20000) return(ErrorTimeout());

    // Go read the busy bit
    SPIBUFINIT;
    MachXO2_Command = MACHXO2_CMD_CHECK_BUSY;
    MachXO2_SPITrans(5); 
    busy = pMachXO2_Data[0]; 
   }   
  while(busy);

  // Set flag indicating we has successfully erased the FPGA
  FPGAIsErased = 1;
  
  // Return success
  return(1);
 }
 
/*--------------------------------------------------------------------------*/

/* FIPSY_LOADCONFIGURATION loads the active configuration from flash as
   it would on power up.  This can be completed at any time, but is only 
   useful if there is a good configuration in flash.  It is usually called
   after a configuration has been written so that the new configuration can
   take effect.  This is separate from the configuration programming itself
   so the user can decide when to do that.
*/

DCAPI Fipsy_LoadConfiguration(void)
 {
  // All exported library functions get this check of hardware and arguments    
  // MessageBox(NULL, "Fipsy_LoadConfiguration", "", MB_TASKMODAL);
  if(!HWIsOpen) return(ErrorNotOpen());

  // Send command to load the configuration
  // Remember there are fewer operands for unknown reasons
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_REFRESH;
  MachXO2_SPITrans(3);

  // Wait for what is suppose to be a very fast configuration
  Sleep(100);

  // Return success
  return(1);
 }
 
/*--------------------------------------------------------------------------*/

/* FIPSY_WRITEFEATURES writes 'feature row' and 'feabits' values to the
   flash as prescribed in the arguments.  This is typically used in a call 
   from the programming routine, but it is possible that we would want to 
   do that separately.  The caveat is that this does not check for erased 
   condition, so overwriting an existing set is a potential user error.
   
   Also, this routine is for the Fipsy module, so the SPI must remain
   available, and therefore this routine forces that condition and won't
   let the caller change that setting. However, this appears to be a 
   necessary but not sufficient condition, as the diamond software will 
   also drop support for the pins in that case.

   Note that most of the action here seems to be in the feabits.  What is 
   being called the feature row here, as far as we can tell, contains other
   information that may or may not apply to our chip or configuration, 
   including various addresses and programmable codes.  So for it will 
   usually be all 0.  The feabits contain the hardware control settings 
   that concern us most.
 */

DCAPI Fipsy_WriteFeatures(BYTE *FeatureRow, BYTE *Feabits)
 {
  // All exported library functions get this check of hardware and arguments    
  // MessageBox(NULL, "Fipsy_WriteFeatureRow", "", MB_TASKMODAL);
  if(!HWIsOpen) return(ErrorNotOpen());
  if(FeatureRow == NULL) return(ErrorNullPointer());
  if(Feabits == NULL) return(ErrorNullPointer()); 

  // Command to write feature row    
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_PROG_FEATURE;
  memcpy(pMachXO2_Data, FeatureRow, 8);
  MachXO2_SPITrans(12); 
 
  // Wait more than the time suggested as required to complete
  Sleep(2);

  // Command to write feabits
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_PROG_FEABITS;
  memcpy(pMachXO2_Data, Feabits, 2);
  // Prevent the SPI from being disabled - not sufficient to prevent a bricked device
  pMachXO2_Data[1] &= 0xBF;
  // Send the command
  MachXO2_SPITrans(6); 

  // Wait more than the time suggested as required to complete
  Sleep(2);
     
  // Return success
  return(1);
 }

/*--------------------------------------------------------------------------*/

/* FIPSY_WRITECONFIGURATION writes a JEDEC file to the configuration 
   flash and the feature switches.  This does not erase the chip or load 
   the configuration.  The chip must be erased before entry using the above 
   routine, and therefore will also be in programming mode. 

   This routine accepts a filename as a full path string.  It is assumed that
   the file is a JEDEC file and is structured as required for the target chip.
   There is very little explicit error checking, although there is some 
   error checking is undertaking by looking for specific features.  There is
   still no checking of checksums and other such features, mostly to keep this
   code as simple as possible. We also do make some effort to make this as 
   ANSI-C portable as possible.
   
   Importantly, a JEDEC file has the '*' character as the field delimiter, 
   so white space should not be used in code as a delimiter (CR, LF, etc), 
   even if most files we are working with are structured in a way that seems
   to suggest that might work, and even might work in a specific version.  
   We want to parse this strictly according to JEDEC, though we use the 
   presentation of this information from the Lattice manual.  We do assume
   that the JEDEC file is not encrypted, we ignore any security fuse
   or OTP fuse settings, and we do not program the user code.  Finally, 
   we assume the default fuse state is the erased state and do not program
   unspecified fuses to that nor use it to validate the checksum. 

   This version now fully parses the JEDEC file to be sure the SPI slave port
   is not going to be disabled before allowing the programming to begin.
   We could look at header matter to determine the same thing in various ways,
   but there is not guaranteed that the header matter won't change in other
   versions of diamond.  So we look at the feature row directly.  That does
   mean we could still be spoofed, but only by user intent.
*/

DCAPI Fipsy_WriteConfiguration(char *JEDECFileName)
 {
  BYTE featurerow[10];
  BYTE feabits[4];
  int i, addr_digits, key, keyq;
  BYTE status;

  // All exported library functions get this check of hardware and arguments    
  // MessageBox(NULL, "Fipsy_WriteConfiguration", "", MB_TASKMODAL);
  if(!HWIsOpen) return(ErrorNotOpen());
  if(JEDECFileName == NULL) return(ErrorNullPointer());
  if(JEDECFileName[0] == 0) return(ErrorNullPointer());
  
  // If the FPGA has not been erased, indicate bad order and quit
  if(!FPGAIsErased) return(ErrorNotErased());                        

  // If we are even about to configure the part, let's clear this flag here and 
  // so indicate that we tried to program the part and should erase it again before
  // trying to program the part again.  
  FPGAIsErased = 0;

  // Attempt to open the JEDEC file
  // A JEDEC file is a text file - we will only need to read text, and the end of line can be useful to us
  JFile = fopen(JEDECFileName, "r");
  if(JFile == NULL) return(ErrorFileNotFound());

  // Read the file characters until we find the starting STX (CTRL-B, 0x02)
  do { key = fgetc(JFile); } while ((key != 0x02) && (key != EOF));
  if(key == EOF) EXIT_FILEFORMATERROR;

  // Look for key characters until we find Q and the qualifier 'F' for fuse count
  // We assume this will come before the fuse table as it is needed to read the address
  do 
   { 
    // Look for Q
    do { if((key = JEDEC_SeekNextKeyChar()) == 0) EXIT_FILEFORMATERROR; } while(key != 'Q');
    // Get qualifier
    keyq = fgetc(JFile);
    if(keyq == EOF) return(0);
   }
  while (keyq != 'F');  
 
  // We really don't care about the fuse count, unless we want to use it the help verify this is the right device
  // But we need to know how may characters it has so we can read addresses in the fuse table
  // Go get that count of characters
  addr_digits = 0;
  do
   {
    key = fgetc(JFile);
    if(key == EOF) EXIT_FILEFORMATERROR;
    if(key != '*') addr_digits += 1;   
   }
  while(key != '*');

  // We just read a delimiter, so we are at the start of the next field
  // Look for the key character - this step is probably overkill here, but for completeness
  if((key = JEDEC_SeekNextNonWhitespace()) == 0) EXIT_FILEFORMATERROR;  
  // Look for the key character 'E' for the feature row 
  // Doing this early allows us to make sure the features do not include a disabled SPI port
  // We will do it again later to program these bits
  while(key != 'E') { if((key = JEDEC_SeekNextKeyChar()) == 0) EXIT_FILEFORMATERROR; };
  // We are now at the feature row bits, pointed at the fuse data
  // Read the data into the local arrays 
  for(i=0;i<8;i++) if(JEDEC_ReadFuseByte(&featurerow[i]) != 1) EXIT_FILEFORMATERROR; 
  for(i=0;i<2;i++) if(JEDEC_ReadFuseByte(&feabits[i]) != 1) EXIT_FILEFORMATERROR;
  // If the SPI port is disabled, warn the user and exit
  if(feabits[1] & 0x40) EXIT_FILEBADSETTINGERROR;
  // Restart the file to the beginning
  rewind(JFile); 
  // Read the file characters until we find the starting STX (CTRL-B, 0x02)
  do { key = fgetc(JFile); } while ((key != 0x02) && (key != EOF));
  if(key == EOF) EXIT_FILEFORMATERROR;

  // Next look for the fuse table specifically
  while(key != 'L') { if((key = JEDEC_SeekNextKeyChar()) == 0) EXIT_FILEFORMATERROR; };
  
  // We are now at the fuse table and pointing to the starting address.
  // This address contains the same number of digits as the fuse count as previously found.
  // The documentation says it is followed by white space, but does not really guarantee what kind 
  // of white space, so we must read off the address characters one at a time.  
  // We do assume from the documentation that the first L key character found will contain the address zero,  
  // and fuse data will start from the beginning of flash.  It is more work to translate and use the address,  
  // so we don't want to bother if that is not the way it is suppose to be.  However, we can verify this address  
  // is zero as expected and flag an error if it is something else.  
  
  // Read the address and check that it is all zero
  // Note that any EOF would also drop out here
  for(i=0;i<addr_digits;i++) 
   if(fgetc(JFile) != '0') EXIT_FILEFORMATERROR;

  // The address is good and is zero, so clear the address in the device
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_INIT_ADDRESS;
  MachXO2_SPITrans(4); 
 
  // We are now pointed at the fuse data (link table, whatever you want to call it)  
  // Proceed to write flash locations per page until the delimiter is reached.  
  // The table should have an even set of pages in it, so if we get the delimiter or any other failure 
  // in the middle of a page, that is a format error.  Unfortunately, exiting here will have the chip 
  // in a bizarre state of partially programmed, but that should be an exceptional case. 
  do 
   {
    // Setup the write and increment command
    SPIBUFINIT;
    MachXO2_Command = MACHXO2_CMD_PROG_INCR_NV;
    pMachXO2_Operand[2] = 0x01; 
    // Get the first byte and check that we are not at the delimiter
    status = JEDEC_ReadFuseByte(pMachXO2_Data);   
    if(status == 0) EXIT_FILEFORMATERROR;
    // If we did not get the delimiter, this should be a valid row 
    if(status != '*')
     {
      // Attempt to collect the rest of the page
      for(i=1;i<16;i++) 
       {
        status = JEDEC_ReadFuseByte(&pMachXO2_Data[i]);    
        if(status != 1) EXIT_FILEFORMATERROR;
       };   
      // We can now send the command
      MachXO2_SPITrans(20); 
     };
    // The page is suppose to program in 200us, which is generally faster than we can send another command,
    // but for completeness and for the benefit of porting this code, we setup a wait here.
    // With a small chip this is not much of a burden, while other systems with a faster connection
    // and a larger chip my want to check busy instead.  
    Sleep(1);
    // We are done when the status here is the delimiter, otherwise go get the next row
   }
  while(status != '*');
 
  // Note that for our chip the JEDEC file seems to contain two blocks of data without easily located explanation. 
  // The first is the configuration for the present design, and the second seems to be the remainder of the 
  // configuration memory, as the address for the second block changes for each design.  There is no user flash 
  // memory in this chip.  This remainder is always in a benign state, with another end flag of sorts at the
  // end (all 1's), which is never reached.  Considering all this, we ignore this second block of data.
   
  // Go find the key for our next thing of interest, the feature row fuses
  // We just read a delimiter, so we are at the start of the next field
  if((key = JEDEC_SeekNextNonWhitespace()) == 0) EXIT_FILEFORMATERROR;
  // Look for the key character
  while(key != 'E') { if((key = JEDEC_SeekNextKeyChar()) == 0) EXIT_FILEFORMATERROR; };
  
  // We are now at the feature row bits, pointed at the fuse data
  // Read the data into the local arrays 
  for(i=0;i<8;i++) if(JEDEC_ReadFuseByte(&featurerow[i]) != 1) EXIT_FILEFORMATERROR; 
  for(i=0;i<2;i++) if(JEDEC_ReadFuseByte(&feabits[i]) != 1) EXIT_FILEFORMATERROR;
  // Call our routine to program these values
  // Note that this routine may alter some bits (see comments with routine)
  Fipsy_WriteFeatures(featurerow, feabits);

  // Program the DONE bit (internal)
  // This effectively tells the SDM (self download mode) that it is allowed to run
  // and allows the device to enter user mode when loading is complete (ie done)
  SPIBUFINIT;
  MachXO2_Command = MACHXO2_CMD_PROGRAM_DONE;
  MachXO2_SPITrans(4);
  Sleep(1);

  // Security and OTP bits would be programmed here, but we do not support them
  // They seem to operate similar to DONE, enabling or disabling certain features
  // DONE is all we need - enables user mode
  // Advanced user may also user an external 'DONE' pin to control entry into user mode,
  // but that will require additional study of feature settings and coding

  // Now that everything is programmed, reload the configuration from flash
  Fipsy_LoadConfiguration();

  // Close file
  fclose(JFile); 

  // If we got here, all went ok, return success
  return(1);
 }

/*--------------------------------------------------------------------------*/
/* JEDEC File Parsing Support Subroutines                                   */
/*--------------------------------------------------------------------------*/

/* The following private functions all operate on a JEDEC file using a 
   file stream pointer for an open text file established globally by 
   the caller.  Coding this way removes the need to pass the file pointer 
   to each function and among them.  Because these are private functions 
   and used in a manner controlled in this module, we do not do additional 
   checking of the pointers and other variables in use. 
 */

/*--------------------------------------------------------------------------*/

/* JEDEC_SEEKNEXTNONWHITESPACE parses the file until it finds a character 
   that is not white space as defined for a JEDEC file.  That character is 
   returned if found.  If there is an error, a 0 is returned.
   
   Note that the delimiter ('*') is also whitespace in this context.  This is
   like a line terminator in a sense, so if we have not already read it, we 
   don't want to now read it and return it.  This could happen if it is 
   the first character in a file, or if a field ended with "**" or more.
   These technically null fields should be ignored in this search.
   
   So define white space as ' ', CR, LF, NULL, and '*' at least.  But in
   reality we equally ignore any character less than space, which includes
   most control characters, including the JEDEC file start and end STX/EOT.
*/

char JEDEC_SeekNextNonWhitespace(void)
 {
  int ic;

  // Read until we find something other than white space
  do
   {
    ic = fgetc(JFile);
    if(ic == EOF) return(0);     
   }
  while((ic <= ' ') || (ic == '*'));
  
  // Return what we found
  return(LOBYTE(ic));
 }

/*--------------------------------------------------------------------------*/

/* JEDEC_SEEKNEXTKEYCHAR reads the specified file stream until the next key 
   character has been read. The key character (ie key word) is the first 
   character of a field (ie after the previous field's delimiter) after any 
   white space.  Thus, in order to do this search, this function will also 
   search for the start of the next field.  If we know we are at the start 
   of a field already, then we should not use this routine but instead just 
   look for the character.  This routine will find the next key character.
   
   The key character found is returned by value.  If the end of the file 
   is reached, or some other error occurs, a 0 is returned.
*/

char JEDEC_SeekNextKeyChar(void)
 {
  int ic;
  char key;
  
  // Look for end of field, point to start of next field
  do
   {
    ic = fgetc(JFile);
    if(ic == EOF) return(0);     
   }
  while (ic != '*');
  
  // Pull white space until an actual character is reached
  // This will be our key character
  key = JEDEC_SeekNextNonWhitespace();
 
  // Return the value that got us here
  return(key);  
 }
 
/*--------------------------------------------------------------------------*/
  
/* JEDEC_READFUSEBYTE reads from the specified file stream until it has 
   collected eight binary characters and converts those characters to a byte
   to return by reference.  The value returned is 1 if this happened correctly,
   0 if an error was encountered, and '*' if the field has ended ('*' found).
   Generally, unless something is wrong with the file, the full byte will be
   collected or '*' will be returned.  Other white space is automatically removed.
   A return value of 0 can be interpreted as a format error.  A return value 
   of '*' means the caller should assume the file pointer now points to the 
   start of the next field.
   
   Note that anything other than 1,0,* characters can be considered white space.
   If a character is out of place or replaced, and it disrupts the count of 1s
   and 0s, then an error will eventually be found.  If the bad character is there
   without affecting the result, then it has no impact this way.  In a perfect
   file, the characters so removed are truly white space. 
*/

BYTE JEDEC_ReadFuseByte(BYTE *FuseByte)
 {
  char bstr[10];
  int ic;
  BYTE cnt = 0;
  
  // Default byte value
  *FuseByte = 0;
  
  // Read characters  
  do
   {
    // Get a character
    ic = fgetc(JFile);
    if(ic == EOF) return(0);
    // Record valid characters
    if(ic == '0') bstr[cnt++] = '0';
    if(ic == '1') bstr[cnt++] = '1';    
    // If delimiter found, return it
    if(ic == '*') return('*');   
   }
  while(cnt < 8);
  
  // Convert the characters to binary
  // We could make this a loop, but it would just take more variables and time
  // This also allows us to easily adjust the bit order if it ever needs to be different
  if(bstr[0] == '1') *FuseByte += 128;
  if(bstr[1] == '1') *FuseByte += 64;
  if(bstr[2] == '1') *FuseByte += 32;
  if(bstr[3] == '1') *FuseByte += 16;
  if(bstr[4] == '1') *FuseByte += 8;
  if(bstr[5] == '1') *FuseByte += 4;
  if(bstr[6] == '1') *FuseByte += 2;
  if(bstr[7] == '1') *FuseByte += 1;
    
  // Return normal success
  return(1);
 } 

/*--------------------------------------------------------------------------*/
/* General Purpose And Helper Subroutines                                   */
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

