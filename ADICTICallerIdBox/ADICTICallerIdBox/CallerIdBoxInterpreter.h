#pragma once
#include "Serial.h"

#define MAX_CALL_BLOCK_NUM	40
#define MAX_CALLER_ID_BOX_NUM 10
#define MAX_VIRTUAL_LINE_PORT_NUM	MAX_CALL_BLOCK_NUM

typedef struct CallerIdBoxData_Tag
{
	int	LineNo;
	int	StatusCode;
	PCHAR	pData;
}CallerIdBoxData_T, * PCallerIdBoxData_T;


typedef struct CallerIdBoxInfo_Tag
{
	int BoxIndex;
	class CSerial CallerIdBoxUart;
	int ComPortNo;
	CHAR Info[100];
	BOOL Found;
	int OutPortNumbers;
	int SerialNo;
	BOOL Assigned;
	int VirtualLineFirst;
	CHAR RecvBuff[500];
	int RecvIndex;
	HANDLE hThreadPBXUart;
	CallerIdBoxData_T CallerIdBoxData;
	int VirtualLineNo;
	BOOL StartRecv;
	CHAR Message[1000];
	BOOL FirstS;
	BOOL FoundStartCodes;
	UINT Timer;
	CHAR Data[100];
	CHAR FWVersion[100];
} CallerIdBoxInfo_T, * PCallerIdBoxInfo_T;

typedef struct VirtualLineInfo_Tag
{
	BOOL Matched;
	class CSerial* pCallerIdBoxUart;
	BOOL NotSame;
	CHAR BoxInfoInStorage[100];
	int SerialNoInStorage;
	int OurPortNumInStorage;
	int ComPortNoInStorage;
	CHAR BoxInfoInCurrent[100];
	int SerialNoInCurrent;
	int OurPortNumInCurrent;
	int ComPortNoInCurrent;
	CHAR BoxFWVersion[100];
}VirtualLineInfo_T, * PVirtualLineInfo_T;


BOOL CallerIdBoxInterpreterOpen();
BOOL OpenUARTPort(int Port, int BaudRate);
void CloseUARTPort();
void CallerIdBoxInfoSave();
void CloseAllHandle();
