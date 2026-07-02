#include "pch.h"
#include "CallerIdBoxInterpreter.h"
#include "Serial.h"
#include "ADICTICallerIdBoxDlg.h"
#include "TelStateMachine.h"
#include "MachineClient.h"

/* test */
#define CALLERID_BOX_STATUS_CODE_CALLERID	'1'
#define CALLERID_BOX_STATUS_CODE_RINGON		'2'
#define CALLERID_BOX_STATUS_CODE_RINGOFF	'3'
#define CALLERID_BOX_STATUS_CODE_HOOK		'4'
#define CALLERID_BOX_STATUS_CODE_DTMF		'5'
#define CALLERID_BOX_STATUS_CODE_HW_VER		'F'
#define QUEUE_NUMBERS	20

typedef struct EventQueue_Tag
{
	BOOL InUsed;
	UINT EventId;
	PCallerIdBoxInfo_T pCallerIdBoxUart;
	PCallBlock_T pCallBlock;
}EventQueue_T, *PEventQueue_T;

extern int m_MachineID;
static CallBlock_T	m_CallBlock[MAX_CALL_BLOCK_NUM];
//class CSerial m_CallerIdBoxUart[30];
static CallerIdBoxInfo_T m_CallerIdBoxInfoInCurrent[MAX_CALLER_ID_BOX_NUM];	/* 目前PC上的來電盒資訊 */
static CallerIdBoxInfo_T m_CallerIdBoxInfoInStorage[MAX_CALLER_ID_BOX_NUM];	/* 儲存在Registry的來電盒資訊 */
static int m_CallerIdBoxNum = 0;
static BOOL m_UartOpened = FALSE;
//static HANDLE m_hThreadPBXUart;
static int m_VirtualLineNumbers;
static int m_VirtualLineNumbersInStorage;
static BOOL	FirstTime = TRUE;
static HANDLE m_hThreadEventQueue;
static VirtualLineInfo_T m_VirtualLineInfo[MAX_VIRTUAL_LINE_PORT_NUM];				/* 來自來電盒的虛擬外線資訊 */
static EventQueue_T m_EventQueueBuff[QUEUE_NUMBERS];
static UINT m_TopQueueIndex = 0;
static UINT m_BottomQueueIndex = 0;
static BOOL m_QueueSemaphore = FALSE;
static BOOL m_DoQueueSemaphore = FALSE;
static BOOL m_Semaphore = FALSE;

static void InterpretCallerIdBoxInfo(PCallerIdBoxInfo_T pCallerIdBoxInfo)
{
	PCallerIdBoxInfo_T pOneCallerIdBox;
	for (int i = 0; i < MAX_CALLER_ID_BOX_NUM; i++)
	{
		pOneCallerIdBox = &pCallerIdBoxInfo[i];
		if (pOneCallerIdBox->Found == TRUE)
		{
			/* 根據訊息解析外線數量 */
			PCHAR pHeader = strstr(pOneCallerIdBox->Info, "FCID");
			if (pHeader)
			{
				CHAR Lines[10] = { 0 };
				memcpy(&Lines[0], &pHeader[4], 2);
				pOneCallerIdBox->OutPortNumbers = atoi(Lines);
				/* 根據訊息解析來電盒序號 */
				PCHAR pSn = strstr(pHeader + 4, "F");
				CHAR SerialNo[10] = { 0 };
				memcpy(&SerialNo[0], &pSn[1], 4);
				pOneCallerIdBox->SerialNo = atoi(SerialNo);
			}
		}
	}
}

static void	AutoArrangeLines()
{
	PCallerIdBoxInfo_T pThisCallerIdBoxInfo = NULL;
	PCallerIdBoxInfo_T pPrevCallerIdBoxInfo = NULL;
	PCallerIdBoxInfo_T pCallerIdBoxInfoInCurrent = NULL;
	PVirtualLineInfo_T pVirtualLineInfo = NULL;

	m_VirtualLineNumbers = 0;
	m_VirtualLineNumbersInStorage = 0;

	/* 根據Storage的來電盒資訊建立虛擬外線資訊 */
	for (int i = 0; i < MAX_CALLER_ID_BOX_NUM; i++)
	{
		pCallerIdBoxInfoInCurrent = &m_CallerIdBoxInfoInCurrent[i];
		pThisCallerIdBoxInfo = &m_CallerIdBoxInfoInStorage[i];
		if (pThisCallerIdBoxInfo->Found == TRUE)
		{
			if (i == 0)
			{
				pThisCallerIdBoxInfo->VirtualLineFirst = 1;
			}
			else
			{
				pPrevCallerIdBoxInfo = &m_CallerIdBoxInfoInStorage[i - 1];
			}
			if (i > 0)
				pThisCallerIdBoxInfo->VirtualLineFirst = pPrevCallerIdBoxInfo->VirtualLineFirst + pPrevCallerIdBoxInfo->OutPortNumbers;
			for (int j = 0; j < pThisCallerIdBoxInfo->OutPortNumbers; j++)
			{
				pVirtualLineInfo = &m_VirtualLineInfo[m_VirtualLineNumbers];
				memcpy(pVirtualLineInfo->BoxInfoInStorage, pThisCallerIdBoxInfo->Info, 100);
				pVirtualLineInfo->SerialNoInStorage = pThisCallerIdBoxInfo->SerialNo;
				pVirtualLineInfo->ComPortNoInStorage = pThisCallerIdBoxInfo->ComPortNo;
				pVirtualLineInfo->OurPortNumInStorage = pThisCallerIdBoxInfo->OutPortNumbers;
				pVirtualLineInfo->pCallerIdBoxUart = &pThisCallerIdBoxInfo->CallerIdBoxUart;
				memcpy(pVirtualLineInfo->BoxFWVersion, pCallerIdBoxInfoInCurrent->FWVersion, 100);
				m_VirtualLineNumbersInStorage++;
				m_VirtualLineNumbers++;
			}
		}
	}

	/* 根據Storage來電盒資訊在Current來電盒資訊中找到是否由符合的SerialNo */
	for (int i = 0; i < MAX_CALLER_ID_BOX_NUM; i++)
	{
		pThisCallerIdBoxInfo = &m_CallerIdBoxInfoInStorage[i];
		for (int j = 0; j < MAX_CALLER_ID_BOX_NUM; j++)
		{
			pCallerIdBoxInfoInCurrent = &m_CallerIdBoxInfoInCurrent[j];
			if (
				pThisCallerIdBoxInfo->Found == TRUE &&
				pCallerIdBoxInfoInCurrent->Found == TRUE &&
				pThisCallerIdBoxInfo->SerialNo == pCallerIdBoxInfoInCurrent->SerialNo)
			{
				pThisCallerIdBoxInfo->Assigned = TRUE;
				pCallerIdBoxInfoInCurrent->Assigned = TRUE;
				/*
					Storage與Current來電盒資訊有符合的SerialNo.
					將Current來電盒訊息存放到虛擬外線資訊中.
				*/
				for (int x = 0; x < m_VirtualLineNumbers; x++)
				{
					pVirtualLineInfo = &m_VirtualLineInfo[x];
					if (pVirtualLineInfo->SerialNoInStorage == pThisCallerIdBoxInfo->SerialNo)
					{
						memcpy(pVirtualLineInfo->BoxInfoInCurrent, pCallerIdBoxInfoInCurrent->Info, 100);
						pVirtualLineInfo->SerialNoInCurrent = pCallerIdBoxInfoInCurrent->SerialNo;
						pVirtualLineInfo->ComPortNoInCurrent = pCallerIdBoxInfoInCurrent->ComPortNo;
						pVirtualLineInfo->OurPortNumInCurrent = pCallerIdBoxInfoInCurrent->OutPortNumbers;
						pVirtualLineInfo->Matched = TRUE;
						/* Serial Port的硬體指標要使用目前的硬體指標因為有可能會USB Port */
						pVirtualLineInfo->pCallerIdBoxUart = &pCallerIdBoxInfoInCurrent->CallerIdBoxUart;
					}
				}
			}
		}
	}
	BOOL SuccessFill = TRUE;
	for (int x = 0; x < m_VirtualLineNumbers; x++)
	{
		pVirtualLineInfo = &m_VirtualLineInfo[x];
		if (pVirtualLineInfo->Matched == FALSE)
		{
			SuccessFill = FALSE;
			break;
		}
	}

	int LineNumInStorage;
	int LineNumInCurrent;
	if (SuccessFill == FALSE)
	{
		for (int i = 0; i < MAX_CALLER_ID_BOX_NUM; i++)
		{
			/* 檢查Storage來電盒資訊是否有沒被assigned的 */
			pThisCallerIdBoxInfo = &m_CallerIdBoxInfoInStorage[i];
			if (pThisCallerIdBoxInfo->Assigned == FALSE)
			{
				LineNumInStorage = pThisCallerIdBoxInfo->OutPortNumbers;
				if (LineNumInStorage > 0)
				{
					/* 檢查Current來電盒資訊是否有沒被assigned的 */
					for (int j = 0; j < MAX_CALLER_ID_BOX_NUM; j++)
					{
						pCallerIdBoxInfoInCurrent = &m_CallerIdBoxInfoInCurrent[j];
						if (pCallerIdBoxInfoInCurrent->Assigned == FALSE)
						{
							LineNumInCurrent = pCallerIdBoxInfoInCurrent->OutPortNumbers;
							/* 在Storage來電盒未assigned的外線數量等於在Current來電盒未assigned的外線數量 */
							if (LineNumInStorage == LineNumInCurrent)
							{
								/* 將Current來電盒assigned給這個Storage來電盒*/
								pThisCallerIdBoxInfo->Assigned = TRUE;
								pCallerIdBoxInfoInCurrent->Assigned = TRUE;
								for (int x = 0; x < m_VirtualLineNumbers; x++)
								{
									pVirtualLineInfo = &m_VirtualLineInfo[x];
									if (pVirtualLineInfo->Matched == FALSE)
									{
										pVirtualLineInfo->NotSame = TRUE;
										memcpy(pVirtualLineInfo->BoxInfoInCurrent, pCallerIdBoxInfoInCurrent->Info, 100);
										pVirtualLineInfo->SerialNoInCurrent = pCallerIdBoxInfoInCurrent->SerialNo;
										pVirtualLineInfo->ComPortNoInCurrent = pCallerIdBoxInfoInCurrent->ComPortNo;
										pVirtualLineInfo->OurPortNumInCurrent = pCallerIdBoxInfoInCurrent->OutPortNumbers;
										pVirtualLineInfo->Matched = TRUE;
										/* Serial Port的硬體指標要使用目前的硬體指標因為有可能會USB Port */
										pVirtualLineInfo->pCallerIdBoxUart = &pCallerIdBoxInfoInCurrent->CallerIdBoxUart;
									}
								}
							}
						}
					}
				}
			}
		}
	}

#if 1
	/* 根據Current來電盒資訊是否存在於Storage */
	for (int j = 0; j < MAX_CALLER_ID_BOX_NUM; j++)
	{
		pCallerIdBoxInfoInCurrent = &m_CallerIdBoxInfoInCurrent[j];
		if (pCallerIdBoxInfoInCurrent->Found == TRUE && pCallerIdBoxInfoInCurrent->Assigned == FALSE)
		{
			for (int x = 0; x < pCallerIdBoxInfoInCurrent->OutPortNumbers; x++)
			{
				pVirtualLineInfo = &m_VirtualLineInfo[m_VirtualLineNumbers];
				memcpy(pVirtualLineInfo->BoxInfoInCurrent, pCallerIdBoxInfoInCurrent->Info, 100);
				pVirtualLineInfo->SerialNoInCurrent = pCallerIdBoxInfoInCurrent->SerialNo;
				pVirtualLineInfo->ComPortNoInCurrent = pCallerIdBoxInfoInCurrent->ComPortNo;
				pVirtualLineInfo->OurPortNumInCurrent = pCallerIdBoxInfoInCurrent->OutPortNumbers;
				/* Serial Port的硬體指標要使用目前的硬體指標因為有可能會USB Port */
				pVirtualLineInfo->pCallerIdBoxUart = &pCallerIdBoxInfoInCurrent->CallerIdBoxUart;
				m_VirtualLineNumbers++;
			}
		}
	}
#endif
}

static void ShowVirtualLineList()
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	class CUartSettingDlg* pCUartSettingDlg = (CUartSettingDlg*)pCADICTICallerIdBoxDlg->GetUartSettingWnd();
	class CSettingDlg* pCSettingDlg = (CSettingDlg*)pCADICTICallerIdBoxDlg->GetSettingWnd();
	LVITEM	lvI;
	lvI.mask = LVIF_TEXT;
	PVirtualLineInfo_T pVirtualLineInfo;
	CHAR StrBuff[100];

	for (int i = 0; i < m_VirtualLineNumbers; i++)
	{
		pVirtualLineInfo = &m_VirtualLineInfo[i];
		lvI.iItem = i;
		lvI.iSubItem = 0;
		lvI.pszText = "1";
		lvI.cchTextMax = 60;
		pCUartSettingDlg->m_ListCallerIdBox.InsertItem(&lvI);

		sprintf_s(StrBuff, 100, "%d", i + 1);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_OUT_LINE_NO, StrBuff);
		sprintf_s(StrBuff, 100, "%s", pVirtualLineInfo->BoxInfoInStorage);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_INFO_IN_STORAGE, StrBuff);
		sprintf_s(StrBuff, 100, "%04d", pVirtualLineInfo->SerialNoInStorage);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_SERIAL_NO_IN_STORAGE, StrBuff);
		sprintf_s(StrBuff, 100, "%d", pVirtualLineInfo->OurPortNumInStorage);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_OUT_PORTS_IN_STORAGE, StrBuff);
		sprintf_s(StrBuff, 100, "%d", pVirtualLineInfo->ComPortNoInStorage);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_UART_PORT_IN_STORAGE, StrBuff);
		sprintf_s(StrBuff, 100, "%s", pVirtualLineInfo->BoxInfoInCurrent);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_INFO_IN_CURRENT, StrBuff);
		sprintf_s(StrBuff, 100, "%04d", pVirtualLineInfo->SerialNoInCurrent);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_SERIAL_NO_IN_CURRENT, StrBuff);
		sprintf_s(StrBuff, 100, "%d", pVirtualLineInfo->OurPortNumInCurrent);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_OUT_PORTS_IN_CURRENT, StrBuff);
		sprintf_s(StrBuff, 100, "%d", pVirtualLineInfo->ComPortNoInCurrent);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_UART_PORT_IN_CURRENT, StrBuff);
		sprintf_s(StrBuff, 100, "%s", &pVirtualLineInfo->BoxFWVersion[2]);
		pCUartSettingDlg->m_ListCallerIdBox.SetItemText(i, ID_LIST_BOX_SW_VER_IN_CURRENT, StrBuff);

		pCUartSettingDlg->m_ListCallerIdBox.SetItemData(i, (DWORD_PTR)pVirtualLineInfo);
	}

}


static void WaitQueueSemaphore()
{
	while (m_QueueSemaphore == TRUE) { Sleep(1); };
}

static void StartQueueSemaphore()
{
	m_QueueSemaphore = TRUE;
}

static void StopQueueSemaphore()
{
	m_QueueSemaphore = FALSE;
}

static void AddEventQueue(UINT EventId, PCallerIdBoxInfo_T pCallerIdBoxUart, PCallBlock_T pCallBlock)
{
	WaitQueueSemaphore();
	StartQueueSemaphore();
	PEventQueue_T pEventQueue;
	pEventQueue = &m_EventQueueBuff[m_TopQueueIndex];
	pEventQueue->InUsed = TRUE;
	pEventQueue->EventId = EventId;
	pEventQueue->pCallerIdBoxUart = pCallerIdBoxUart;
	pEventQueue->pCallBlock = pCallBlock;
	if (++m_TopQueueIndex >= QUEUE_NUMBERS)
		m_TopQueueIndex = 0;
	StopQueueSemaphore();
}

static void DoEventQueue()
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	while (m_DoQueueSemaphore) {};
	m_DoQueueSemaphore = TRUE;
	PEventQueue_T pEventQueue = &m_EventQueueBuff[m_BottomQueueIndex];

	if (pEventQueue->InUsed == TRUE)
	{
		PCallerIdBoxInfo_T pCallerIdBoxUart = pEventQueue->pCallerIdBoxUart;
		PCallBlock_T pCallBlock = pEventQueue->pCallBlock;
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_CALLERID_BOX_UART, pCallBlock->LineNo, pCallerIdBoxUart->Message);
		if (pEventQueue->EventId != -1)
			StateMachineSendEvent(pCallBlock, pEventQueue->EventId);
		pEventQueue->InUsed = FALSE;
		if (++m_BottomQueueIndex >= QUEUE_NUMBERS)
			m_BottomQueueIndex = 0;
	}
	m_DoQueueSemaphore = FALSE;
}

static void ThreadEventQueue(LPVOID lpParam)
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	CHAR Message[100];

	while (1)
	{
		if (m_TopQueueIndex != m_BottomQueueIndex)
		{
			DoEventQueue();
		}
		Sleep(1);
	}
}

static void ThreadCallerIdBoxUart(LPVOID lpParam)
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	class CSettingDlg* pCSettingDlg = (CSettingDlg*)pCADICTICallerIdBoxDlg->GetSettingWnd();
	CHAR	OneByte = 0;
	int len = 0;
	int LineNo;
	CHAR StatusCode;
	int RingCounter;

	PCallerIdBoxInfo_T pCallerIdBoxUart = (PCallerIdBoxInfo_T)lpParam;
	while (1)
	{
		while (m_Semaphore) {};
		m_Semaphore = TRUE;
		len = pCallerIdBoxUart->CallerIdBoxUart.ReadData(&OneByte, 1);
		if (len == 1)
		{
			if (pCallerIdBoxUart->FoundStartCodes == TRUE)
			{
				pCallerIdBoxUart->RecvBuff[pCallerIdBoxUart->RecvIndex++] = OneByte;
			}
			if (OneByte == 'S')
			{
				if (pCallerIdBoxUart->FirstS == FALSE)
					pCallerIdBoxUart->FirstS = TRUE;
				else
				{
					/* 找到起始碼SS */
					pCallerIdBoxUart->FoundStartCodes = TRUE;
					memset(pCallerIdBoxUart->RecvBuff, 0x00, 500);
					pCallerIdBoxUart->RecvIndex = 0;
				}
			}
			else if (OneByte == 'X' && pCallerIdBoxUart->FoundStartCodes)
			{
				/* 分析收到來電盒的資料 */
				pCallerIdBoxUart->RecvBuff[pCallerIdBoxUart->RecvIndex - 1] = 0;
				memset(&pCallerIdBoxUart->CallerIdBoxData, 0x00, sizeof(CallerIdBoxData_T));
				LineNo = pCallerIdBoxUart->CallerIdBoxData.LineNo = pCallerIdBoxUart->RecvBuff[0] - 0x30;
				LineNo -= 1;
				pCallerIdBoxUart->VirtualLineNo = pCallerIdBoxUart->VirtualLineFirst + LineNo;
				StatusCode = pCallerIdBoxUart->CallerIdBoxData.StatusCode = pCallerIdBoxUart->RecvBuff[1];
				if (StatusCode != CALLERID_BOX_STATUS_CODE_HW_VER)
				{
					pCallerIdBoxUart->CallerIdBoxData.pData = (PCHAR)&pCallerIdBoxUart->RecvBuff[2];
					PCallBlock_T pCallBlock;
					pCallBlock = &m_CallBlock[pCallerIdBoxUart->VirtualLineNo - 1];
					pCallBlock->LineNo = pCallerIdBoxUart->VirtualLineNo;
					switch (StatusCode)
					{
					case CALLERID_BOX_STATUS_CODE_CALLERID:
						sprintf_s(pCallerIdBoxUart->Message, "收到來電盒來電碼資料: 虛擬外線第%d線 來電碼: %s", pCallerIdBoxUart->VirtualLineNo, pCallerIdBoxUart->CallerIdBoxData.pData);
						strcpy_s(pCallBlock->CallerID, 100, pCallerIdBoxUart->CallerIdBoxData.pData);
						AddEventQueue(EVENT_CALLER_ID, pCallerIdBoxUart, pCallBlock);
						break;
					case CALLERID_BOX_STATUS_CODE_RINGON:
						RingCounter = atoi(pCallerIdBoxUart->CallerIdBoxData.pData);
						sprintf_s(pCallerIdBoxUart->Message, "收到來電盒響鈴資料: 虛擬外線第%d線 第幾次響鈴: %d", pCallerIdBoxUart->VirtualLineNo, RingCounter);
						pCallBlock->RingTimes = RingCounter;
						AddEventQueue(EVENT_RING_ON, pCallerIdBoxUart, pCallBlock);
						break;
					case CALLERID_BOX_STATUS_CODE_RINGOFF:
						sprintf_s(pCallerIdBoxUart->Message, "收到來電盒響鈴停止資料: 虛擬外線第%d線", pCallerIdBoxUart->VirtualLineNo);
						AddEventQueue(EVENT_RING_OFF, pCallerIdBoxUart, pCallBlock);
						break;
					case CALLERID_BOX_STATUS_CODE_HOOK:
						if (pCallerIdBoxUart->CallerIdBoxData.pData[0] == '1')
						{
							sprintf_s(pCallerIdBoxUart->Message, "收到來電盒聽筒資料: 虛擬外線第%d線 舉機", pCallerIdBoxUart->VirtualLineNo);
							AddEventQueue(EVENT_PICKUP_IS_1, pCallerIdBoxUart, pCallBlock);
						}
						else if (pCallerIdBoxUart->CallerIdBoxData.pData[0] == '0')
						{
							sprintf_s(pCallerIdBoxUart->Message, "收到來電盒聽筒資料: 虛擬外線第%d線 掛機", pCallerIdBoxUart->VirtualLineNo);
							AddEventQueue(EVENT_PICKUP_IS_0, pCallerIdBoxUart, pCallBlock);
						}
						else
						{
							sprintf_s(pCallerIdBoxUart->Message, "收到來電盒聽筒資料: 虛擬外線第%d線 不明狀態: %c", pCallerIdBoxUart->VirtualLineNo, pCallerIdBoxUart->CallerIdBoxData.pData[0]);
							AddEventQueue(-1, pCallerIdBoxUart, pCallBlock);
						}
						break;
					case CALLERID_BOX_STATUS_CODE_DTMF:
						sprintf_s(pCallerIdBoxUart->Message, "收到來電盒按鍵號碼資料: 虛擬外線第%d線 DTMF: %c", pCallerIdBoxUart->VirtualLineNo, pCallerIdBoxUart->CallerIdBoxData.pData[0]);
						pCallBlock->DTMF[0] = pCallerIdBoxUart->CallerIdBoxData.pData[0];
						pCallBlock->DTMF[1] = 0x00;
						AddEventQueue(EVENT_DTMF, pCallerIdBoxUart, pCallBlock);
						break;
					}
					pCallerIdBoxUart->RecvIndex = 0;
					pCallerIdBoxUart->FirstS = FALSE;
					pCallerIdBoxUart->FoundStartCodes = FALSE;
				}
			}
		}
#if 0 /* 模擬 */
		UINT32 Index;
		if (pCallerIdBoxUart->Timer == 300)
		{
			pCallerIdBoxUart->VirtualLineNo = pCallerIdBoxUart->VirtualLineFirst;
			Index = pCallerIdBoxUart->VirtualLineNo - 1;
			PCallBlock_T pCallBlock = &m_CallBlock[Index];
			sprintf_s(pCallerIdBoxUart->Data, 100, "1243345");
			pCallerIdBoxUart->CallerIdBoxData.pData = pCallerIdBoxUart->Data;
			//sprintf_s(pCallerIdBoxUart->Message, "收到來電盒來電碼資料: 虛擬外線第%d線 來電碼: %s", pCallerIdBoxUart->VirtualLineNo, pCallerIdBoxUart->CallerIdBoxData.pData);
			sprintf_s(pCallerIdBoxUart->Message, "Virtual Line %d CallerId: %s", pCallerIdBoxUart->VirtualLineNo, pCallerIdBoxUart->CallerIdBoxData.pData);
			pCallBlock->LineNo = pCallerIdBoxUart->VirtualLineNo;
			strcpy_s(pCallBlock->CallerID, 100, pCallerIdBoxUart->CallerIdBoxData.pData);
			AddEventQueue(EVENT_CALLER_ID, pCallerIdBoxUart, pCallBlock);
		}
		else if (pCallerIdBoxUart->Timer == 500)
		{
			pCallerIdBoxUart->VirtualLineNo = pCallerIdBoxUart->VirtualLineFirst;
			Index = pCallerIdBoxUart->VirtualLineNo - 1;
			PCallBlock_T pCallBlock = &m_CallBlock[Index];
			pCallBlock->LineNo = pCallerIdBoxUart->VirtualLineNo;
			//sprintf_s(pCallerIdBoxUart->Message, "收到來電盒聽筒資料: 虛擬外線第%d線 舉機", pCallerIdBoxUart->VirtualLineNo);
			sprintf_s(pCallerIdBoxUart->Message, "Virtual Line %d PickUp", pCallerIdBoxUart->VirtualLineNo);
			AddEventQueue(EVENT_PICKUP_IS_1, pCallerIdBoxUart, pCallBlock);
		}
		else if (pCallerIdBoxUart->Timer == 501)
		{
#if 0
			pCallerIdBoxUart->VirtualLineNo = pCallerIdBoxUart->VirtualLineFirst + 1;
			Index = pCallerIdBoxUart->VirtualLineNo - 1;
			PCallBlock_T pCallBlock = &m_CallBlock[Index];
			pCallBlock->LineNo = pCallerIdBoxUart->VirtualLineNo;
			sprintf_s(pCallerIdBoxUart->Message, "收到來電盒聽筒資料: 虛擬外線第%d線 舉機", pCallerIdBoxUart->VirtualLineNo);
			AddEventQueue(EVENT_PICKUP_IS_1, pCallerIdBoxUart, pCallBlock);
#endif
		}
		else if (pCallerIdBoxUart->Timer == 820)
		{
			pCallerIdBoxUart->VirtualLineNo = pCallerIdBoxUart->VirtualLineFirst;
			Index = pCallerIdBoxUart->VirtualLineNo - 1;
			PCallBlock_T pCallBlock = &m_CallBlock[Index];
			pCallBlock->LineNo = pCallerIdBoxUart->VirtualLineNo;
			//sprintf_s(pCallerIdBoxUart->Message, "收到來電盒聽筒資料: 虛擬外線第%d線 掛機", pCallerIdBoxUart->VirtualLineNo);
			sprintf_s(pCallerIdBoxUart->Message, "Virtual Line %d PickDown", pCallerIdBoxUart->VirtualLineNo);
			AddEventQueue(EVENT_PICKUP_IS_0, pCallerIdBoxUart, pCallBlock);
		}
		else if (pCallerIdBoxUart->Timer == 822)
		{
#if 0
			pCallerIdBoxUart->VirtualLineNo = pCallerIdBoxUart->VirtualLineFirst + 1;
			Index = pCallerIdBoxUart->VirtualLineNo - 1;
			PCallBlock_T pCallBlock = &m_CallBlock[Index];
			pCallBlock->LineNo = pCallerIdBoxUart->VirtualLineNo;
			sprintf_s(pCallerIdBoxUart->Message, "收到來電盒聽筒資料: 虛擬外線第%d線 掛機", pCallerIdBoxUart->VirtualLineNo);
			AddEventQueue(EVENT_PICKUP_IS_0, pCallerIdBoxUart, pCallBlock);
#endif
		}
		if (++pCallerIdBoxUart->Timer >= 1000)
		{
			pCallerIdBoxUart->VirtualLineNo = pCallerIdBoxUart->VirtualLineFirst;
			Index = pCallerIdBoxUart->VirtualLineNo - 1;
			PCallBlock_T pCallBlock = &m_CallBlock[Index];
			pCallBlock->LineNo = pCallerIdBoxUart->VirtualLineNo;
			sprintf_s(pCallerIdBoxUart->Message, "################################## Virtual Line %d Start Count", pCallerIdBoxUart->VirtualLineNo);
			AddEventQueue(-1, pCallerIdBoxUart, pCallBlock);
			pCallerIdBoxUart->Timer = 0;
		}
#endif
		m_Semaphore = FALSE;
		Sleep(1);
	}
}


BOOL CallerIdBoxInterpreterOpen()
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	class CUartSettingDlg* pCUartSettingDlg = (CUartSettingDlg*)pCADICTICallerIdBoxDlg->GetUartSettingWnd();
	class CSettingDlg* pCSettingDlg = (CSettingDlg*)pCADICTICallerIdBoxDlg->GetSettingWnd();
	CHAR OneByte;
	int Len;
	BOOL Found;
	PCallerIdBoxInfo_T pCallerIdBoxInfo;

	memset(&m_CallerIdBoxInfoInCurrent, 0x00, sizeof(CallerIdBoxInfo_T) * MAX_CALLER_ID_BOX_NUM);
	memset(&m_CallerIdBoxInfoInStorage, 0x00, sizeof(CallerIdBoxInfo_T) * MAX_CALLER_ID_BOX_NUM);
	memset(&m_VirtualLineInfo, 0x00, sizeof(VirtualLineInfo_T) * MAX_VIRTUAL_LINE_PORT_NUM);
	m_CallerIdBoxNum = 0;
	m_VirtualLineNumbers = 0;
	m_VirtualLineNumbersInStorage = 0;

	for (int i = 1; i < 128; i++)
	{
		pCallerIdBoxInfo = &m_CallerIdBoxInfoInCurrent[m_CallerIdBoxNum];
		Found = pCallerIdBoxInfo->CallerIdBoxUart.Open(i, 9600, true);
		if (Found == TRUE)
		{
			pCallerIdBoxInfo->CallerIdBoxUart.SendData("TT00X", 5);
			Sleep(100);
			Len = pCallerIdBoxInfo->CallerIdBoxUart.ReadData(&OneByte, 1);
			if (Len == 1)
			{
				pCallerIdBoxInfo->ComPortNo = i;
				pCallerIdBoxInfo->Found = Found;
				m_CallerIdBoxNum++;
			}
			//else
				pCallerIdBoxInfo->CallerIdBoxUart.Close();
		}
	}

	/* 將目前來電盒資訊放入m_CallerIdBoxInfoInCurrent結構中 */
	Sleep(100);
	CHAR RecvBuff[100];
	int RecvIndex = 0;

	for (int i = 0; i < m_CallerIdBoxNum; i++)
	{
		BOOL FirstSCode = FALSE;
		BOOL SecondSCode = FALSE;
		pCallerIdBoxInfo = &m_CallerIdBoxInfoInCurrent[i];
		pCallerIdBoxInfo->CallerIdBoxUart.Open(pCallerIdBoxInfo->ComPortNo, 9600, true);
		pCallerIdBoxInfo->CallerIdBoxUart.SendData("TT00X", 5);
		Sleep(100);
		OneByte = 0;
		memset(RecvBuff, 0x00, 100);
		RecvIndex = 0;
		while (1)
		{
			Len = pCallerIdBoxInfo->CallerIdBoxUart.ReadData(&OneByte, 1);
			if (Len == 1)
			{
				if (OneByte == 'X')
					break;
				if (FirstSCode == TRUE && SecondSCode == TRUE)
					pCallerIdBoxInfo->Info[RecvIndex++] = OneByte;
				if (OneByte == 'S' && FirstSCode == FALSE)
					FirstSCode = TRUE;
				else if (OneByte == 'S' && FirstSCode == TRUE)
					SecondSCode = TRUE;
				else if (FirstSCode == TRUE && SecondSCode == FALSE)
				{
					FirstSCode = FALSE;
					SecondSCode = FALSE;
				}
			}
			else
				break;
		}
		pCallerIdBoxInfo->CallerIdBoxUart.SendData("TT01X", 5);
		Sleep(100);
		OneByte = 0;
		memset(RecvBuff, 0x00, 100);
		RecvIndex = 0;
		FirstSCode = FALSE;
		SecondSCode = FALSE;
		while (1)
		{
			Len = pCallerIdBoxInfo->CallerIdBoxUart.ReadData(&OneByte, 1);
			if (Len == 1)
			{
				if (OneByte == 'X')
					break;
				if (FirstSCode == TRUE && SecondSCode == TRUE)
					pCallerIdBoxInfo->FWVersion[RecvIndex++] = OneByte;
				if (OneByte == 'S' && FirstSCode == FALSE)
					FirstSCode = TRUE;
				else if (OneByte == 'S' && FirstSCode == TRUE)
					SecondSCode = TRUE;
				else if (FirstSCode == TRUE && SecondSCode == FALSE)
				{
					FirstSCode = FALSE;
					SecondSCode = FALSE;
				}
			}
			else
				break;
		}
	}
	/* 分析目前來電盒訊息 */
	InterpretCallerIdBoxInfo(m_CallerIdBoxInfoInCurrent);

	/* 有儲存來電盒序號,取出來電盒資料 */
	CString CallerIdBoxSerialNo;
	CallerIdBoxSerialNo = AfxGetApp()->GetProfileString("SystemSetting", "CallerIdBoxInfo", "");
	CString CallerIdBoxComPort;
	CallerIdBoxComPort = AfxGetApp()->GetProfileString("SystemSetting", "CallerIdBoxComPort", "");
	Len = CallerIdBoxSerialNo.GetLength();
	if(Len <= 1000)
	{
		int CallerIdBoxSerialNoLen = CallerIdBoxSerialNo.GetLength() + 1;
		PCHAR pBuffSerialNo = (PCHAR)malloc(sizeof(CHAR) * CallerIdBoxSerialNoLen);
		memset(pBuffSerialNo, 0x00, CallerIdBoxSerialNoLen);
		int CallerIdBoxComPortLen = CallerIdBoxComPort.GetLength() + 1;
		PCHAR pBuffComport = (PCHAR)malloc(sizeof(CHAR) * CallerIdBoxComPortLen);
		memset(pBuffComport, 0x00, CallerIdBoxComPortLen);
		strcpy_s(pBuffSerialNo, CallerIdBoxSerialNoLen, CallerIdBoxSerialNo.GetBuffer());
		strcpy_s(pBuffComport, CallerIdBoxComPortLen, CallerIdBoxComPort.GetBuffer());
		PCHAR token1, token2;
		PCHAR next_token1 = NULL;
		PCHAR next_token2 = NULL;
		token1 = strtok_s(pBuffSerialNo, ";", &next_token1);
		token2 = strtok_s(pBuffComport, ";", &next_token2);
		int Index = 0;
		while (token1 && token2)
		{
			pCallerIdBoxInfo = &m_CallerIdBoxInfoInStorage[Index++];
			pCallerIdBoxInfo->Found = TRUE;
			sprintf_s(pCallerIdBoxInfo->Info, 100, token1);
			pCallerIdBoxInfo->ComPortNo = atoi(token2);
			token1 = strtok_s(NULL, ";", &next_token1);
			token2 = strtok_s(NULL, ";", &next_token2);
		}
		free(pBuffSerialNo);
		free(pBuffComport);
		InterpretCallerIdBoxInfo(m_CallerIdBoxInfoInStorage);
	}

	/* 根據在Current與Storage來電盒的資訊顯示在來電盒列表的視窗上 */
	AutoArrangeLines();
	ShowVirtualLineList();
	BOOL MatchedEveryOne = TRUE;
	PVirtualLineInfo_T pVirtualLineInfo;
	for (int i = 0; i < m_VirtualLineNumbers; i++)
	{
		pVirtualLineInfo = &m_VirtualLineInfo[i];
		if (pVirtualLineInfo->Matched == FALSE)
			MatchedEveryOne = FALSE;
	}

	if (MatchedEveryOne == TRUE)
	{
		int BoxIndex = 0;
		for (int i = 0; i < MAX_CALLER_ID_BOX_NUM; i++)
		{
			PCallerIdBoxInfo_T pCallerIdBoxInfoInCurrent = &m_CallerIdBoxInfoInCurrent[i];
			PCallerIdBoxInfo_T pCallerIdBoxInfoInStorage = &m_CallerIdBoxInfoInStorage[i];
			if (pCallerIdBoxInfoInCurrent->Found == TRUE)
			{
				memcpy(&pCallerIdBoxInfoInStorage->CallerIdBoxUart
					, &pCallerIdBoxInfoInCurrent->CallerIdBoxUart
					, sizeof(CSerial));
				pCallerIdBoxInfoInCurrent->BoxIndex = BoxIndex++;
				pCallerIdBoxInfoInStorage->BoxIndex = pCallerIdBoxInfoInCurrent->BoxIndex;
			}
		}

		if (FirstTime == TRUE)
		{
			FirstTime = FALSE;
			for (int i = 0; i < MAX_CALLER_ID_BOX_NUM; i++)
			{
				PCallerIdBoxInfo_T pCallerIdBoxInfoInStorage = &m_CallerIdBoxInfoInStorage[i];
				if (pCallerIdBoxInfoInStorage->Found == TRUE)
				{
					pCallerIdBoxInfoInStorage->hThreadPBXUart = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadCallerIdBoxUart, pCallerIdBoxInfoInStorage, 0, NULL);
				}
			}
			memset(m_EventQueueBuff, 0x00, sizeof(EventQueue_T) * QUEUE_NUMBERS);
			m_hThreadEventQueue = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadEventQueue, NULL, 0, NULL);
			memset(m_CallBlock, 0x00, sizeof(CallBlock_T) * MAX_CALL_BLOCK_NUM);
			InitStateMachine(m_CallBlock);
			pCSettingDlg->ShowOutPortNumber(m_VirtualLineNumbersInStorage);
			pViewLogDlg->SetOutPortNumber(m_VirtualLineNumbersInStorage);
		}
	}
	else
	{
		CHAR Message[1000];
		sprintf_s(Message, "儲存的來電盒訊息與實際的來電盒訊息不符!!!");
		pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_CALLERID_BOX_UART, 0, Message);
		pCADICTICallerIdBoxDlg->ShowMessage(Message);
	}

	return m_UartOpened;
}

BOOL OpenUARTPort(int Port, int BaudRate)
{
	BOOL result = FALSE;

//	result = m_Uart.Open(Port, BaudRate, true);

	return result;
}

void CloseUARTPort()
{
//	result = m_Uart.Close(Port, BaudRate, true);
}

void CallerIdBoxInfoSave()
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	class CUartSettingDlg* pCUartSettingDlg = (CUartSettingDlg*)pCADICTICallerIdBoxDlg->GetUartSettingWnd();
	class CSettingDlg* pCSettingDlg = (CSettingDlg*)pCADICTICallerIdBoxDlg->GetSettingWnd();
	CHAR InfoBuff[1000];
	CHAR ComPortBuff[1000];
	PCallerIdBoxInfo_T pCallerIdBoxInfo;
	PVirtualLineInfo_T pVirtualLineInfo;
	int InfoLen = 0;
	int ComPortLen = 0;
	int CallerIdBoxIndex = 0;
	int CurrentSerialNo = 0;

	/* 根據m_VirtualLineInfo內容重新配置m_CallerIdBoxInfoInCurrent */
	for (int i = 0; i < MAX_VIRTUAL_LINE_PORT_NUM; i++)
	{
		pVirtualLineInfo = &m_VirtualLineInfo[i];
		if (pVirtualLineInfo->OurPortNumInCurrent)
		{
			if (CurrentSerialNo != pVirtualLineInfo->SerialNoInCurrent)
			{
				CurrentSerialNo = pVirtualLineInfo->SerialNoInCurrent;
				InfoLen += sprintf_s(InfoBuff + InfoLen, 1000 - InfoLen, "%s;", pVirtualLineInfo->BoxInfoInCurrent);
				ComPortLen += sprintf_s(ComPortBuff + ComPortLen, 1000 - ComPortLen, "%d;", pVirtualLineInfo->ComPortNoInCurrent);
			}
		}
	}
	if (InfoLen > 0)
	{
		InfoBuff[InfoLen - 1] = 0x00;
		AfxGetApp()->WriteProfileString("SystemSetting", "CallerIdBoxInfo", InfoBuff);
	}

	if (ComPortLen > 0)
	{
		ComPortBuff[ComPortLen - 1] = 0x00;
		AfxGetApp()->WriteProfileString("SystemSetting", "CallerIdBoxComPort", ComPortBuff);
	}

	pCUartSettingDlg->OnBnClickedButtonReflash();

	return;
}

void CloseAllHandle()
{
	for (int i = 0; i < MAX_CALLER_ID_BOX_NUM; i++)
	{
		PCallerIdBoxInfo_T pCallerIdBoxInfoInCurrent = &m_CallerIdBoxInfoInCurrent[i];
		PCallerIdBoxInfo_T pCallerIdBoxInfoInStorage = &m_CallerIdBoxInfoInStorage[i];
		if (pCallerIdBoxInfoInCurrent->Found == TRUE)
		{
			pCallerIdBoxInfoInStorage->CallerIdBoxUart.Close();
			CloseHandle(pCallerIdBoxInfoInCurrent->hThreadPBXUart);
		}
	}
}