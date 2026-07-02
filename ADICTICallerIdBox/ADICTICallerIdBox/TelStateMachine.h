#pragma once
#define DTMF_TIMEOUT_MSEC	6 /* 240 msec */

enum
{
	STATE_MACHINE_IDLE = 0,
	STATE_MACHINE_WAIT_A,
	STATE_MACHINE_WAIT_B,
	STATE_MACHINE_WAIT_DTMF,
	STATE_MACHINE_CALLOUT,
	STATE_MACHINE_CALLIN,
};

enum
{
	EVENT_CALLER_ID = 0,
	EVENT_PICKUP_IS_1,
	EVENT_PICKUP_IS_0,
	EVENT_RING_OFF,
	EVENT_RING_ON,
	EVENT_DTMF,
	EVENT_DTMF_TIMEOUT,
};

typedef struct CallBlock_Tag
{
	int LineNo;
	int	CurrentState;
	int RingTimes;
	CHAR DTMF[2];
	CHAR CallerID[100];
	CHAR CalleeID[100];
	CHAR SecondDialNum[100];
	int DTMFTimeOut;
	BOOL StartDTMFTimer;
	UINT32 StartTalkingUTC;
	UINT32 EndTalkingUTC;
}CallBlock_T, *PCallBlock_T;

typedef void* StateFunc(int Event, PCallBlock_T pCallBlock);

void InitStateMachine(PCallBlock_T gpCallBlock);
void StateMachineGoto(PCallBlock_T pCallBlock, int StateId);
void StateMachineSendEvent(PCallBlock_T pCallBlock, int Event);
