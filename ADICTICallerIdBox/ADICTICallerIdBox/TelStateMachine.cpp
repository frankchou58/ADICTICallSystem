#include "pch.h"
#include "TelStateMachine.h"
#include "ADICTICallerIdBoxDlg.h"
#include "MachineClient.h"
#include "CallerIdBoxInterpreter.h"

int m_MachineID;
class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg;
class CViewLogDlg* pViewLogDlg;
class CSettingDlg* pCSettingDlg;

static HANDLE m_hStateMachineTimer;
static HANDLE m_hCallSimulation;
static int m_DTMFTimeout;

static CHAR EventName[7][100] = {
	"CallerID",
	"PickUp = 1",
	"PickUp = 0",
	"Ring Off",
	"Ring On",
	"DTMF",
	"DTMF TimeOut",
};

static void ThreadSimulation(LPVOID lpParam)
{
	PCallBlock_T gpCallBlock = (PCallBlock_T)lpParam;
	PCallBlock_T pCallBlock;
	int LineCounter = 0;
	int Timer = 0;

	CTime Today;
	time_t Timestamp;
	CHAR Message[1000];
	while (1)
	{
		pCallBlock = &gpCallBlock[LineCounter];
		pCallBlock->LineNo = 1;
		if (Timer == 300)
		{
			Today = CTime::GetCurrentTime();
			Timestamp = ConvertDateToUTC(&Today);
			pCallBlock->StartTalkingUTC = Timestamp;
			StateMachineSendEvent(pCallBlock, EVENT_PICKUP_IS_1);
		}
		else if (Timer == 350)
		{
			pCallBlock->DTMF[0] = 0x39;
			pCallBlock->DTMF[1] = 0x00;
			StateMachineSendEvent(pCallBlock, EVENT_DTMF);
		}
		else if (Timer == 400)
		{
			pCallBlock->DTMF[0] = 0x32;
			pCallBlock->DTMF[1] = 0x00;
			StateMachineSendEvent(pCallBlock, EVENT_DTMF);
		}
		else if (Timer == 450)
		{
			pCallBlock->DTMF[0] = 0x33;
			pCallBlock->DTMF[1] = 0x00;
			StateMachineSendEvent(pCallBlock, EVENT_DTMF);
		}
		else if (Timer == 800)
		{
			Today = CTime::GetCurrentTime();
			Timestamp = ConvertDateToUTC(&Today);
			pCallBlock->EndTalkingUTC = Timestamp;
			StateMachineSendEvent(pCallBlock, EVENT_PICKUP_IS_0);
		}
		else if (Timer == 900)
		{
			Today = CTime::GetCurrentTime();
			Timestamp = ConvertDateToUTC(&Today);
			pCallBlock->StartTalkingUTC = Timestamp;
			sprintf_s(pCallBlock->CallerID, 100, "0921452333");
			StateMachineSendEvent(pCallBlock, EVENT_CALLER_ID);
		}
		else if (Timer == 950)
		{
			StateMachineSendEvent(pCallBlock, EVENT_PICKUP_IS_1);
		}
		else if (Timer == 1050)
		{
			Today = CTime::GetCurrentTime();
			Timestamp = ConvertDateToUTC(&Today);
			pCallBlock->EndTalkingUTC = Timestamp;
			StateMachineSendEvent(pCallBlock, EVENT_PICKUP_IS_0);
			break;
		}

		Timer++;
		Sleep(1); /* 40 msec*/
	}
}

static void ThreadTimer(LPVOID lpParam)
{
	PCallBlock_T gpCallBlock = (PCallBlock_T)lpParam;
	PCallBlock_T pCallBlock;
	int LineCounter = 0;
	while (1)
	{
		pCallBlock = &gpCallBlock[LineCounter];
		if (pCallBlock->StartDTMFTimer == TRUE)
		{
			if (pCallBlock->DTMFTimeOut-- == 0)
			{
				StateMachineSendEvent(pCallBlock, EVENT_DTMF_TIMEOUT);
				pCallBlock->StartDTMFTimer = FALSE;
			}
		}
		if (++LineCounter == MAX_CALL_BLOCK_NUM)
			LineCounter = 0;
		Sleep(1); /* 5 msec*/
	}
}

static StateFunc* StateIdle(int Event, PCallBlock_T pCallBlock)
{
	int Ret;
	CHAR Message[1000];
	CallParams_T CallParams;
	switch (Event)
	{
	case EVENT_CALLER_ID:
		/* 撥入-來電碼 */
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.pCallID = pCallBlock->CallerID;
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN, &CallParams);
		sprintf_s(Message, 1000, "第%d線在Idle時收到(%s)來電碼%s,跳到WaitA狀態", pCallBlock->LineNo, EventName[Event], pCallBlock->CallerID);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		StateMachineGoto(pCallBlock, STATE_MACHINE_WAIT_A);
		break;
	case EVENT_PICKUP_IS_1:
		/* 撥出 */
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.pCallID = pCallBlock->CallerID;
		//CallParams.pCallID = 0;
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT, &CallParams);
		sprintf_s(Message, 1000, "第%d線在Idle時收到(%s)事件,跳到WaitDTMF狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		StateMachineGoto(pCallBlock, STATE_MACHINE_WAIT_DTMF);
		break;
	case EVENT_RING_ON:
		/* 撥入-響鈴 */
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.RingNums = pCallBlock->RingTimes;
		memset(&pCallBlock->CallerID, 0x00, sizeof(CallerIdBoxData_T));
		CallParams.pCallID = pCallBlock->CallerID;
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN, &CallParams);
		sprintf_s(Message, 1000, "第%d線在Idle時收到第%d聲(%s)事件,跳到WaitB狀態", pCallBlock->LineNo, pCallBlock->RingTimes, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		StateMachineGoto(pCallBlock, STATE_MACHINE_WAIT_B);
		break;
	case EVENT_PICKUP_IS_0:
	case EVENT_RING_OFF:
	case EVENT_DTMF:
		break;
	}

	return NULL;
}

static StateFunc* StateCallOut(int Event, PCallBlock_T pCallBlock)
{
	int Ret;
	CHAR Message[1000];
	CallParams_T CallParams;
	CTime Today = CTime::GetCurrentTime();
	time_t Timestamp = ConvertDateToUTC(&Today);
	switch (Event)
	{
	case EVENT_CALLER_ID:
		break;
	case EVENT_PICKUP_IS_1:
		break;
	case EVENT_RING_ON:
		break;
	case EVENT_PICKUP_IS_0:
		/* 結束通話 */
		pCallBlock->EndTalkingUTC = (UINT32)Timestamp;
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.Duration = pCallBlock->EndTalkingUTC - pCallBlock->StartTalkingUTC; /* second */
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP, &CallParams);
		sprintf_s(Message, 1000, "第%d線在CallOut時收到(%s)事件,跳到Idle狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		memset(pCallBlock->CalleeID, 0x00, 100);
		memset(pCallBlock->CallerID, 0x00, 100);
		StateMachineGoto(pCallBlock, STATE_MACHINE_IDLE);
		break;
	case EVENT_RING_OFF:
		break;
	case EVENT_DTMF:
		/* 二次撥碼 */
		sprintf_s(Message, 1000, "第%d線在CallOut時收到DTMF=%s的(%s)事件,收集二次撥號碼", pCallBlock->LineNo, pCallBlock->DTMF, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		strcat_s(pCallBlock->SecondDialNum, pCallBlock->DTMF);
		break;
	}

	return NULL;
}

static StateFunc* StateCallIn(int Event, PCallBlock_T pCallBlock)
{
	CHAR StateName[] = { "CallIn" };
	int Ret;
	CHAR Message[1000];
	CallParams_T CallParams;
	CTime Today = CTime::GetCurrentTime();
	time_t Timestamp = ConvertDateToUTC(&Today);
	switch (Event)
	{
	case EVENT_CALLER_ID:
		break;
	case EVENT_PICKUP_IS_1:
		break;
	case EVENT_RING_ON:
		sprintf_s(Message, 1000, "第%d線在%s時收到第%d聲(Ring On)事件", pCallBlock->LineNo, StateName, pCallBlock->RingTimes);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		break;
	case EVENT_PICKUP_IS_0:
		/* 結束通話 */
		pCallBlock->EndTalkingUTC = (UINT32)Timestamp;
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.Duration = pCallBlock->EndTalkingUTC - pCallBlock->StartTalkingUTC; /* second */
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP, &CallParams);
		sprintf_s(Message, 1000, "第%d線在%s時收到(%s)事件,跳到Idle狀態", pCallBlock->LineNo, StateName, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		memset(pCallBlock->CalleeID, 0x00, 100);
		memset(pCallBlock->CallerID, 0x00, 100);
		StateMachineGoto(pCallBlock, STATE_MACHINE_IDLE);
		break;
	case EVENT_RING_OFF:
		/* 結束通話 */
#if 0
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		//CallParams.RingNums = pCallBlock->RingTimes;
		CallParams.Duration = pCallBlock->EndTalkingUTC - pCallBlock->StartTalkingUTC; /* second */
		//Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER, &CallParams);
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP, &CallParams);
		sprintf_s(Message, 1000, "第%d線在%s時收到(%s)事件,跳到IDEL狀態", pCallBlock->LineNo, StateName, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, Message);
		memset(pCallBlock->CalleeID, 0x00, 100);
		memset(pCallBlock->CallerID, 0x00, 100);
		StateMachineGoto(pCallBlock, STATE_MACHINE_IDLE);
#endif
		break;
	case EVENT_DTMF:
		/* 收集二次撥號碼 */
		sprintf_s(Message, 1000, "第%d線在%s時收到DTMF=%s的(%s)事件,收集二次撥號碼", pCallBlock->LineNo, StateName, pCallBlock->DTMF, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		strcat_s(pCallBlock->SecondDialNum, pCallBlock->DTMF);
	}
	return NULL;
}

static StateFunc* StateWaitDTMF(int Event, PCallBlock_T pCallBlock)
{
	int Ret;
	CHAR Message[1000];
	CallParams_T CallParams;
	CTime Today = CTime::GetCurrentTime();
	time_t Timestamp = ConvertDateToUTC(&Today);
	switch (Event)
	{
	case EVENT_CALLER_ID:
		break;
	case EVENT_PICKUP_IS_1:
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.RingNums = pCallBlock->RingTimes;
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT, &CallParams);
		sprintf_s(Message, 1000, "第%d線在WaitA時收到(%s)事件,跳到CallOut狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		StateMachineGoto(pCallBlock, STATE_MACHINE_CALLOUT);
		break;
	case EVENT_RING_ON:
		break;
	case EVENT_PICKUP_IS_0:
		//sprintf_s(Message, 1000, "第%d線在WaitDTMF時收到(%s)事件,跳到Idle狀態", pCallBlock->LineNo, EventName[Event]);
		//pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, Message);
		//StateMachineGoto(pCallBlock, STATE_MACHINE_IDLE);
		//
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.Duration = pCallBlock->EndTalkingUTC - pCallBlock->StartTalkingUTC; /* second */
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP, &CallParams);
		sprintf_s(Message, 1000, "第%d線在WaitA時收到(%s)事件,跳到IDEL狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		memset(pCallBlock->CalleeID, 0x00, 100);
		memset(pCallBlock->CallerID, 0x00, 100);
		StateMachineGoto(pCallBlock, STATE_MACHINE_IDLE);
		break;
	case EVENT_RING_OFF:
		break;
	case EVENT_DTMF:
		/* 收集CalleeID */
		sprintf_s(Message, 1000, "第%d線在WaitDTMF時收到DTMF=%s的(%s)事件,將DTMF碼收集到CalleeID", pCallBlock->LineNo, pCallBlock->DTMF, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		strcat_s(pCallBlock->CalleeID, pCallBlock->DTMF);
		pCallBlock->StartDTMFTimer = TRUE;
		pCallBlock->DTMFTimeOut = m_DTMFTimeout;
		break;
	case EVENT_DTMF_TIMEOUT:
		/* 開始通話 */
		pCallBlock->StartTalkingUTC = (UINT32)Timestamp;
		pCallBlock->EndTalkingUTC = 0;
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.RingNums = pCallBlock->RingTimes;
		CallParams.pCallID = pCallBlock->CalleeID;
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER, &CallParams);
		sprintf_s(Message, 1000, "第%d線在WaitDTMF時收到(%s)事件,跳到CallOut狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		StateMachineGoto(pCallBlock, STATE_MACHINE_CALLOUT);
		break;
	}

	return NULL;
}

static StateFunc* StateWaitA(int Event, PCallBlock_T pCallBlock)
{
	int Ret;
	CHAR Message[1000];
	CallParams_T CallParams;
	CTime Today = CTime::GetCurrentTime();
	time_t Timestamp = ConvertDateToUTC(&Today);
	switch (Event)
	{
	case EVENT_CALLER_ID:
		break;
	case EVENT_PICKUP_IS_1:
		/* 開始通話 */
		pCallBlock->StartTalkingUTC = (UINT32)Timestamp;
		pCallBlock->EndTalkingUTC = 0;
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.RingNums = pCallBlock->RingTimes;
		CallParams.pCallID = pCallBlock->CallerID;
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER, &CallParams);
		sprintf_s(Message, 1000, "第%d線在WaitA時收到(%s)事件,跳到CallIn狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		StateMachineGoto(pCallBlock, STATE_MACHINE_CALLIN);
		break;
	case EVENT_RING_ON:
		break;
	case EVENT_PICKUP_IS_0:
		break;
	case EVENT_RING_OFF:
		/* 結束通話 */
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		//CallParams.RingNums = pCallBlock->RingTimes;
		CallParams.Duration = pCallBlock->EndTalkingUTC - pCallBlock->StartTalkingUTC; /* second */
		//Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER, &CallParams);
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP, &CallParams);
		sprintf_s(Message, 1000, "第%d線在WaitA時收到(%s)事件,跳到IDEL狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		memset(pCallBlock->CalleeID, 0x00, 100);
		memset(pCallBlock->CallerID, 0x00, 100);
		StateMachineGoto(pCallBlock, STATE_MACHINE_IDLE);
		break;
	case EVENT_DTMF:
		/* 結合遺漏的CallerID */
		sprintf_s(Message, 1000, "第%d線在WaitA時收到DTMF=%s的(%s)事件,將DTMF碼收集到CallerID", pCallBlock->LineNo, pCallBlock->DTMF, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		strcat_s(pCallBlock->CallerID, pCallBlock->DTMF);
		break;
	}

	return NULL;
}

static StateFunc* StateWaitB(int Event, PCallBlock_T pCallBlock)
{
	int Ret;
	CHAR Message[1000];
	CallParams_T CallParams;
	CTime Today = CTime::GetCurrentTime();
	time_t Timestamp = ConvertDateToUTC(&Today);
	switch (Event)
	{
	case EVENT_CALLER_ID:
		/* 撥入-來電碼 */
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.pCallID = pCallBlock->CallerID;
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN, &CallParams);
		sprintf_s(Message, 1000, "第%d線在WaitB時收到(%s)來電碼%s,跳到CallIn狀態", pCallBlock->LineNo, EventName[Event], pCallBlock->CallerID);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		StateMachineGoto(pCallBlock, STATE_MACHINE_CALLIN);
		break;
	case EVENT_PICKUP_IS_1:
		/* 開始通話 */
		pCallBlock->StartTalkingUTC = (UINT32)Timestamp;
		pCallBlock->EndTalkingUTC = 0;
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER, &CallParams);
		sprintf_s(Message, 1000, "第%d線在WaitB時收到(%s)事件,跳到CallIn狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		StateMachineGoto(pCallBlock, STATE_MACHINE_CALLIN);
		break;
	case EVENT_PICKUP_IS_0:
		break;
	case EVENT_RING_OFF:
		/* 結束通話 */
#if 0
		pCallBlock->EndTalkingUTC = (UINT32)Timestamp;
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_CALLERID_BOX;
		CallParams.MachineID = m_MachineID;
		CallParams.OutPortNo = pCallBlock->LineNo;
		CallParams.FromExtNo = 0;
		CallParams.Duration = pCallBlock->EndTalkingUTC - pCallBlock->StartTalkingUTC; /* second */
		Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP, &CallParams);
		sprintf_s(Message, 1000, "第%d線在WaitB時收到(%s)事件,跳到Idle狀態", pCallBlock->LineNo, EventName[Event]);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, Message);
		memset(pCallBlock->CalleeID, 0x00, 100);
		memset(pCallBlock->CallerID, 0x00, 100);
		StateMachineGoto(pCallBlock, STATE_MACHINE_IDLE);
#endif
		break;
	case EVENT_RING_ON:
		sprintf_s(Message, 1000, "第%d線在WaitB時收到第%d聲(Ring On)事件", pCallBlock->LineNo, pCallBlock->RingTimes);
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, Message);
		break;
	case EVENT_DTMF:
		break;
	}

	return NULL;
}

static StateFunc* StateFunction[6] = {
	(StateFunc*)&StateIdle,
	(StateFunc*)&StateWaitA,
	(StateFunc*)&StateWaitB,
	(StateFunc*)&StateWaitDTMF,
	(StateFunc*)&StateCallOut,
	(StateFunc*)&StateCallIn,
};

void InitStateMachine(PCallBlock_T gpCallBlock)
{
	m_DTMFTimeout = AfxGetApp()->GetProfileInt("SystemSetting", "DTMFTimeout", 100) / MAX_CALL_BLOCK_NUM;

	m_hStateMachineTimer = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadTimer, gpCallBlock, 0, NULL);
	//m_hCallSimulation = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadSimulation, gpCallBlock, 0, NULL);
	m_MachineID = AfxGetApp()->GetProfileInt("SystemSetting", "MachineID", 1);
	
	pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	pCSettingDlg = (CSettingDlg*)pCADICTICallerIdBoxDlg->GetSettingWnd();
}

void StateMachineGoto(PCallBlock_T pCallBlock, int StateId)
{
	pCallBlock->CurrentState = StateId;
}

void StateMachineSendEvent(PCallBlock_T pCallBlock, int Event)
{
	StateFunction[pCallBlock->CurrentState](Event, pCallBlock);
}

