#pragma once
#include "ADICTICallCenter.h"

enum
{
	STATE_ID_IDLE = 0,
	STATE_ID_WAIT_LINE_TALKING,
	STATE_ID_WAIT_EXT_TALKING,
	STATE_ID_RING_TALKING,
};

enum
{
	EVENT_ID_OUTLINE_IN = 0,
	EVENT_ID_OUTLINE_OUT,
	EVENT_ID_OUTLINE_DISCONNECT,
	EVENT_ID_OUTLINE_TALKING,
	EVENT_ID_EXTLINE_MAKE,
	EVENT_ID_EXTLINE_DISCONNECT,
	EVENT_ID_EXTLINE_TALKING,
	EVENT_ID_VOICE_REC_FILE_UUID,
	EVENT_ID_CALL_RELEASE,
};

typedef struct ExtData_Tag
{
	int ID;
	int ExtNo;
	int ExtVPort;
}ExtData_T, *PExtData_T;

typedef struct CallBlock_Tag
{
	int DBIndexID;
	bool isInUsed;
	CallType_T CallType;
	int OutVPort;
	int PhyOutPort;
	int ToExtNo;
	int FromExtNo;
	int ToExtVPort;
	int FromExtVPort;
	int CallStatus;
	int MachineType;
	int MachineID;
	int CallRecID;
	UINT32 CallStartTime;
	UINT32 CallConnectionTime;
	UINT32 CallEndTime;
	UINT32 CallDuration;
	char CallerId[50];
	//char CalleeId[50];
	char SecondDTMF[200];
	int CurrentState;
	int PrevState;
	char CustomerUUID[32];
	char OperatorUUID[32];
	char VoiceRecFileUUID[33];
	PVPortStatus_T pFromLineStatus;
	PVPortStatus_T pToLineStatus;
	int RingTimes;
	BOOL SaveToDB;
	CHAR County[50];
	CHAR Townships[50];
	CHAR Address[200];
} CallBlock_T, * PCallBlock_T;

typedef struct EventParamsTag
{
	UINT MachineType;
	UINT MachineID;
	int PhyOutPort;
	int FromExtNum;
	int ToExtNum;
	CHAR TelNo[50];
	UINT32 SecDigitNo;
	int RingTimes;
	int TalkingDuration;
	UINT32 Timestamp;
	CHAR VoiceRecFileName[33];
	int OutVPort;
	ExtData_T FromExt;
	ExtData_T ToExt;
} EventParamsT, * PEventParamsT;

int InitCallStateRecordEngine();
int SendEvent(PEventParamsT pEventParams, UINT EventID);
PCallBlock_T GetGlobelCallBlockPtr();
void SetMainWndPtr();
void ReleaseCallStateBlock(UINT MachineType, UINT MachineID);

