#pragma once
#include <Windows.h>

enum
{
	PACKAGE_MACHINE_TYPE_PBX = 1,
	PACKAGE_MACHINE_TYPE_CALLERID_BOX,
	PACKAGE_MACHINE_TYPE_VOICE_CARD,
};

enum
{
	PACKAGE_EVENT_TYPE_LOGIN = 1,
	PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN,
	PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT,
	PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER,
	PACKAGE_EVENT_TYPE_EXTERNAL_SECOND_DIAL,
	PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP,
	PACKAGE_EVENT_TYPE_INTERNAL_CALL_MAKE,
	PACKAGE_EVENT_TYPE_INTERNAL_CALL_ANSWER,
	PACKAGE_EVENT_TYPE_INTERNAL_SECOND_DIAL,
	PACKAGE_EVENT_TYPE_INTERNAL_CALL_HANGUP,
	PACKAGE_EVENT_TYPE_VOICE_REC_FILENAME,
};

#define PACKAGE_RESPONSE_LOGIN_OK 1
#define PACKAGE_RESPONSE_EVENT_OK 2
#define PACKAGE_RESPONSE_LOGIN_ALREADY -1
#define PACKAGE_RESPONSE_MACHINE_NOT_SETTING -2
#define PACKAGE_RESPONSE_MACHINE_OUTPORTS_NOTMATCH -3
#define PACKAGE_RESPONSE_MACHINE_EXTPORTS_NOTMATCH -4
#define PACKAGE_RESPONSE_OVER_MACHINE_CALLBLOCK_NUMS -5
#define PACKAGE_RESPONSE_MACHINE_TYPE_NOTMATCH -6
#define PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST -7
#define PACKAGE_RESPONSE_OUTPORT_OVER -8

typedef struct MachineResponsePackage_Tag
{
	BYTE EventID;
	INT OutPortNum;
	INT ExtPortNum;
	INT8 ResponseID;
} MachineResponsePackage_T, * PMachineResponsePackage_T;

typedef struct MachineHeaderPackage_Tag
{
	BYTE EventID;
	BYTE MachineType;
	BYTE MachineID;
} MachineHeaderPackage_T, * PMachineHeaderPackage_T;

typedef struct LoginPackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE SWVer;
	BYTE OutPortNums;
	BYTE ExtPortNums;
	BYTE Reverse1;
	BYTE Reverse2;
} LoginPackage_T, * PLoginPackage_T;

typedef struct ExternalCallInPackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE OutPortNo;
	UINT32 Timestamp;
	UINT32 Length;
}ExternalCallInPackage_T, * PExternalCallInPackage_T;

typedef struct ExternalCallOutPackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE OutPortNo;
	UINT32 FromExtNum;
	UINT32 Timestamp;
	UINT32 Length;
}ExternalCallOutPackage_T, * PExternalCallOutPackage_T;

typedef struct ExternalCallAnswerPackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE OutPortNo;
	BYTE RingTimes;
	BYTE Reverse1;
	BYTE Reverse2;
	BYTE Reverse3;
	UINT32 ExtNum;
	UINT32 Timestamp;
	UINT32 Length;
}ExternalCallAnswerPackage_T, * PExternalCallAnswerPackage_T;

typedef struct ExternalCallHangupPackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE OutPortNo;
	UINT16 TalkingDuration;
	BYTE Reverse2;
	BYTE Reverse3;
	UINT32 Timestamp;
}ExternalCallHangupPackage_T, * PExternalCallHangupPackage_T;

typedef struct InternalCallMakePackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE Reverse1;
	UINT32 ToExtNum;
	UINT32 FromExtNum;
	UINT32 Timestamp;
}InternalCallMakePackage_T, * PInternalCallMakePackage_T;

typedef struct InternalCallAnswerPackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE RingTimes;
	UINT32 ToExtNum;
	UINT32 FromExtNum;
	UINT32 Timestamp;
}InternalCallAnswerPackage_T, * PInternalCallAnswerPackage_T;

typedef struct InternalCallHangupPackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE Reserve1;
	UINT16 Duration;
	BYTE Reserve2;
	BYTE Reserve3;
	UINT32 ToExtNum;
	UINT32 FromExtNum;
	UINT32 Timestamp;
}InternalCallHangupPackage_T, * PInternalCallHangupPackage_T;

typedef struct CallVoiceRecordPackage_Tag
{
	MachineHeaderPackage_T Header;
	BYTE OutPortNo;
	UINT32 ToExtNumber;
	UINT32 FromExtNumber;
	UINT32 Timestamp;
	UINT32 Length;
}CallVoiceRecordPackage_T, * PCallVoiceRecordPackage_T;

typedef struct CallParams_Tag
{
	int MachineType;
	int MachineID;
	int OutPortNo;
	int FromExtNo;
	int ToExtNo;
	PCHAR pCallID;
	int RingNums;
	int Duration;
	PCHAR pVoiceRecFileName;
	CHAR SecondDialNo[100];
}CallParams_T, * PCallParams_T;

int StartMachineClient();
int StopMachineClient();
int InitMachineClient(PCHAR pServerIPAddr);
int ClientSendCallStatus(int CallType, PCallParams_T pCallParams);
time_t ConvertDateToUTC(CTime* pToday);
