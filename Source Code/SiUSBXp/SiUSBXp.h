#ifndef _SIUSBXP_H_
#define _SIUSBXP_H_

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SI_USB_XP_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SI_USB_XP_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef SI_USB_XP_EXPORTS
#define SI_USB_XP_API __declspec(dllexport)
#else
#define SI_USB_XP_API __declspec(dllimport)
#pragma comment (lib, "SiUSBXp.lib")
#endif

#if ! defined(_PREFAST_)
#if ! defined(_Check_return_)
#define	_Check_return_
#endif // ! defined(_Check_return_)
#if ! defined(_Ret_range_)
#define _Ret_range_(lb,ub)
#endif // ! defined(_Ret_range_)
#if ! defined(_Success_)
#define	_Success_(expr)
#endif // ! defined(_Success_)
#if ! defined(_In_)
#define	_In_
#endif // ! defined(_In_)
#if ! defined(_In_opt_)
#define	_In_opt_
#endif // ! defined(_In_opt_)
#if ! defined(_Out_)
#define	_Out_
#endif // ! defined(_Out_)
#if ! defined(_In_range_)
#define _In_range_(lb,ub)
#endif // ! defined(_In_range_)
#if ! defined(_Out_range_)
#define _Out_range_(lb,ub)
#endif // ! defined(_Out_range_)
#if ! defined(_In_reads_bytes_)
#define	_In_reads_bytes_(n)
#endif // ! defined(_In_reads_bytes_)
#if ! defined(_Out_writes_bytes_)
#define	_Out_writes_bytes_(n)
#endif // ! defined(_Out_writes_bytes_)
#if ! defined(_Out_writes_bytes_opt_)
#define	_Out_writes_bytes_opt_(n)
#endif // ! defined(_Out_writes_bytes_opt_)
#endif // ! defined(_PREFAST_)
 
// Return codes
#define		SI_SUCCESS					0x00
#define		SI_DEVICE_NOT_FOUND			0xFF
#define		SI_INVALID_HANDLE			0x01
#define		SI_READ_ERROR				0x02
#define		SI_RX_QUEUE_NOT_READY		0x03
#define		SI_WRITE_ERROR				0x04
#define		SI_RESET_ERROR				0x05
#define		SI_INVALID_PARAMETER		0x06
#define		SI_INVALID_REQUEST_LENGTH	0x07
#define		SI_DEVICE_IO_FAILED			0x08
#define		SI_INVALID_BAUDRATE			0x09
#define		SI_FUNCTION_NOT_SUPPORTED	0x0A
#define		SI_GLOBAL_DATA_ERROR		0x0B
#define		SI_SYSTEM_ERROR_CODE		0x0C
#define		SI_READ_TIMED_OUT			0x0D
#define		SI_WRITE_TIMED_OUT			0x0E
#define		SI_IO_PENDING				0x0F
#define     SI_NOTHING_TO_CANCEL        0xA0

// GetProductString() function flags
#define		SI_RETURN_SERIAL_NUMBER		0x00
#define		SI_RETURN_DESCRIPTION		0x01
#define		SI_RETURN_LINK_NAME			0x02
#define		SI_RETURN_VID				0x03
#define		SI_RETURN_PID				0x04

// RX Queue status flags
#define		SI_RX_NO_OVERRUN			0x00
#define		SI_RX_EMPTY					0x00
#define		SI_RX_OVERRUN				0x01
#define		SI_RX_READY					0x02

// Buffer size limits
#define		SI_MAX_DEVICE_STRLEN		256
#define		SI_MAX_READ_SIZE			4096*16
#define		SI_MAX_WRITE_SIZE			4096

// Type definitions
typedef		int		SI_STATUS;	// _range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
typedef		char	SI_DEVICE_STRING[SI_MAX_DEVICE_STRLEN];

// Input and Output pin Characteristics
#define		SI_HELD_INACTIVE			0x00
#define		SI_HELD_ACTIVE				0x01
#define		SI_FIRMWARE_CONTROLLED		0x02		
#define		SI_RECEIVE_FLOW_CONTROL		0x02
#define		SI_TRANSMIT_ACTIVE_SIGNAL	0x03
#define		SI_STATUS_INPUT				0x00
#define		SI_HANDSHAKE_LINE			0x01

// Mask and Latch value bit definitions
#define		SI_GPIO_0					0x0001
#define		SI_GPIO_1					0x0002
#define		SI_GPIO_2					0x0004
#define		SI_GPIO_3					0x0008
#define		SI_GPIO_4					0x0010
#define		SI_GPIO_5					0x0020
#define		SI_GPIO_6					0x0040
#define		SI_GPIO_7					0x0080
#define		SI_GPIO_8					0x0100
#define		SI_GPIO_9					0x0200
#define		SI_GPIO_10					0x0400
#define		SI_GPIO_11					0x0800
#define		SI_GPIO_12					0x1000
#define		SI_GPIO_13					0x2000
#define		SI_GPIO_14					0x4000
#define		SI_GPIO_15					0x8000

// SI_GetPartNumber() return codes
#define     SI_USBXPRESS_F3xx           0x80
#define		SI_CP2101_VERSION			0x01
#define		SI_CP2102_VERSION			0x02
#define		SI_CP2103_VERSION			0x03
#define		SI_CP2104_VERSION			0x04
#define		SI_CP2105_VERSION			0x05
#define		SI_CP2108_VERSION			0x08
#define		SI_CP2109_VERSION			0x09


#ifdef __cplusplus
extern "C" {
#endif

_Check_return_
_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API
SI_STATUS
WINAPI
SI_GetNumDevices(
	_Out_ LPDWORD lpdwNumDevices
	);

_Check_return_
_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API
SI_STATUS WINAPI SI_GetProductString(
	_In_ const DWORD dwDeviceNum,
	LPVOID lpvDeviceString,
	_In_ const DWORD dwFlags
	);

_Check_return_
_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API
SI_STATUS WINAPI SI_Open(
	_In_ const DWORD dwDevice,
	_Out_ HANDLE* cyHandle
	); 

_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API
SI_STATUS WINAPI SI_Close(
	HANDLE cyHandle
	);

_Check_return_
_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API
SI_STATUS WINAPI SI_Read(
	_In_ const HANDLE cyHandle,
	_Out_writes_bytes_(dwBytesToRead) LPVOID lpBuffer,
	_In_range_(0, SI_MAX_READ_SIZE) const DWORD dwBytesToRead, 
	_Out_range_(0,dwBytesToRead) LPDWORD lpdwBytesReturned,
	_In_opt_ OVERLAPPED* o = NULL
	);

_Check_return_
_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API
SI_STATUS WINAPI SI_Write(
	_In_ const HANDLE cyHandle,
	_In_reads_bytes_(dwBytesToWrite) LPVOID lpBuffer,
	_In_range_(0, SI_MAX_WRITE_SIZE) const DWORD dwBytesToWrite,
	_Out_range_(0, dwBytesToWrite) LPDWORD lpdwBytesWritten,
	_In_opt_ OVERLAPPED* o = NULL
	);

SI_USB_XP_API 
SI_STATUS WINAPI SI_DeviceIOControl(
	HANDLE cyHandle,
	DWORD dwIoControlCode,
	LPVOID lpInBuffer,
	DWORD dwBytesToRead,
	LPVOID lpOutBuffer,
	DWORD dwBytesToWrite,
	LPDWORD lpdwBytesSucceeded
	);

_Check_return_
_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API
SI_STATUS WINAPI SI_FlushBuffers(
	_In_ const HANDLE cyHandle,
	_In_ const BYTE FlushTransmit,
	_In_ const BYTE FlushReceive
	);

SI_USB_XP_API 
SI_STATUS WINAPI SI_SetTimeouts(
	DWORD dwReadTimeout,
	DWORD dwWriteTimeout
	);

SI_USB_XP_API 
SI_STATUS WINAPI SI_GetTimeouts(
	LPDWORD lpdwReadTimeout,
	LPDWORD lpdwWriteTimeout
	);

_Check_return_
_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API 
SI_STATUS WINAPI SI_CheckRXQueue(
	_In_ const HANDLE cyHandle,
	LPDWORD lpdwNumBytesInQueue,
	LPDWORD lpdwQueueStatus
	);

SI_USB_XP_API
SI_STATUS	WINAPI SI_SetBaudRate(
	_In_ const HANDLE cyHandle,
	DWORD dwBaudRate
	);

SI_USB_XP_API
SI_STATUS	WINAPI SI_SetBaudDivisor(
	_In_ const HANDLE cyHandle,
	WORD wBaudDivisor
	);

SI_USB_XP_API
SI_STATUS	WINAPI SI_SetLineControl(
	HANDLE cyHandle, 
	WORD wLineControl
	);

SI_USB_XP_API
SI_STATUS	WINAPI SI_SetFlowControl(
	HANDLE cyHandle, 
	BYTE bCTS_MaskCode, 
	BYTE bRTS_MaskCode, 
	BYTE bDTR_MaskCode, 
	BYTE bDSR_MaskCode, 
	BYTE bDCD_MaskCode, 
	BOOL bFlowXonXoff
	);

SI_USB_XP_API
SI_STATUS WINAPI SI_GetModemStatus(
	HANDLE cyHandle, 
	PBYTE ModemStatus
	);

SI_USB_XP_API
SI_STATUS WINAPI SI_SetBreak(
	HANDLE cyHandle, 
	WORD wBreakState
	);

SI_USB_XP_API 
SI_STATUS WINAPI SI_ReadLatch(
	HANDLE cyHandle,
	LPWORD	lpwLatch
	);

SI_USB_XP_API 
SI_STATUS WINAPI SI_WriteLatch(
	HANDLE cyHandle,
	WORD	wMask,
	WORD	wLatch
	);


SI_USB_XP_API 
SI_STATUS WINAPI SI_GetPartNumber(
	HANDLE cyHandle,
	LPBYTE	lpbPartNum
	);

SI_USB_XP_API 
SI_STATUS WINAPI SI_GetInterfaceNumber(
	HANDLE cyHandle,
	LPBYTE	lpbInterfaceNum
	);

SI_USB_XP_API
SI_STATUS WINAPI SI_GetDeviceProductString(	
	HANDLE	cyHandle,
	LPVOID	lpProduct,
	LPBYTE	lpbLength,
	BOOL	bConvertToASCII = TRUE
	);

SI_USB_XP_API 
SI_STATUS WINAPI SI_GetDLLVersion(
	_Out_writes_bytes_opt_(sizeof(DWORD)) DWORD* HighVersion,
	_Out_writes_bytes_opt_(sizeof(DWORD)) DWORD* LowVersion
	);

__declspec(deprecated)
_Ret_range_(SI_SUCCESS, SI_DEVICE_NOT_FOUND)
_Success_(SI_SUCCESS)
SI_USB_XP_API
SI_STATUS WINAPI SI_GetDriverVersion(
	DWORD* HighVersion,
	DWORD* LowVersion
	);

SI_USB_XP_API
SI_STATUS WINAPI SI_CancelIo(
    HANDLE cyHandle
    );

SI_USB_XP_API
SI_STATUS WINAPI SI_CancelIoEx(
    HANDLE cyHandle,
    LPOVERLAPPED lpOverlapped
    );

#ifdef __cplusplus
}
#endif

#endif //_SIUSBXP_H_