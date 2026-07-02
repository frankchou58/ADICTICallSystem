#include "pch.h"
#include "PBXInterpreter.h"
#include "Serial.h"
#include "ADICTIPBXSubMachineDlg.h"
#include "MachineClient.h"

enum
{
	INTERFACE_CO_SIDE = 0,	//局端介面
	INTERFACE_PBX1_SIZE,	//PBX1介面
	INTERFACE_ADMIN_SIZE,	//經理密碼撥出介面
	INTERFACE_PBX2_SIZE,	//PBX2介面
};

class CSerial m_Uart;
PBXFormate_T m_PBXFormate;
static BOOL m_UartOpened = FALSE;
static HANDLE m_hThreadPBXUart;

static UINT32 ConvertToTimestamp(PCHAR pDate, PCHAR pTime)
{
	time_t Timestamp;
	int Year;
	int Month;
	int Day;
	int Hours;
	int Minutes;
	int Seconds;
	CHAR Buff[3];

	CTime Today = CTime::GetCurrentTime();
	int ThisYear = Today.GetYear();
	ThisYear /= 100;
	ThisYear *= 100;

	memcpy(Buff, &pDate[0], 2);
	Year = atoi(Buff);
	memcpy(Buff, &pDate[3], 2);
	Month = atoi(Buff);
	memcpy(Buff, &pDate[6], 2);
	Day = atoi(Buff);

	memcpy(Buff, &pTime[0], 2);
	Hours = atoi(Buff);
	memcpy(Buff, &pTime[3], 2);
	Minutes = atoi(Buff);
	memcpy(Buff, &pTime[6], 2);
	Seconds = atoi(Buff);

	struct tm timeinfo;
	Year += ThisYear;
	timeinfo.tm_year = Year - 1900;
	timeinfo.tm_mon = Month - 1;				 //months since January - [0,11]
	timeinfo.tm_mday = Day;          //day of the month - [1,31] 
	timeinfo.tm_hour = Hours;         //hours since midnight - [0,23]
	timeinfo.tm_min = Minutes;          //minutes after the hour - [0,59]
	timeinfo.tm_sec = Seconds;          //seconds after the minute - [0,59]
	Timestamp = mktime(&timeinfo);

	return (UINT32)Timestamp;
}

static void DataInterpret(PPBXFormate_T pPBXFormate)
{
	class CADICTIPBXSubMachineDlg* pCADICTIPBXSubMachineDlg = (CADICTIPBXSubMachineDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTIPBXSubMachineDlg->GetViewLogWnd();
	CHAR HeaderByte;
	CHAR InterfaceByte;
	int Interface;
	CHAR DirectByte;
	int CallDirect;
	PCHAR pDate;
	PCHAR pTime;
	int Timestamp;
	int OutPortNo;
	CHAR Message[200];
	int Len = 0;
	CHAR CallID[26];
	PCHAR StartPtr;
	PCHAR SpacePtr;
	int Ret;

	memcpy(&HeaderByte, pPBXFormate->Header, sizeof(CHAR));
	if (HeaderByte == '$')
	{
		memcpy(&InterfaceByte, pPBXFormate->Interface, sizeof(CHAR));
		memcpy(&DirectByte, pPBXFormate->Direct, sizeof(CHAR));
		pDate = pPBXFormate->ChargeStartDate;
		pTime = pPBXFormate->ChargeStartTime;
		StartPtr = pPBXFormate->CallID;
		SpacePtr = strstr(pPBXFormate->CallID, " ");
		Len = SpacePtr - StartPtr;
		memset(CallID, 0, 26);
		memcpy(CallID, pPBXFormate->CallID, Len);
		OutPortNo = atoi(pPBXFormate->OutPortNo);
		Timestamp = ConvertToTimestamp(pDate, pTime);

		if (InterfaceByte == 'T')
		{
			Len = sprintf_s(Message, 200, "局端:");
			Interface = INTERFACE_CO_SIDE;
		}
		else if (InterfaceByte == 'P')
		{
			Len = sprintf_s(Message, 200, "PBX1:");
			Interface = INTERFACE_PBX1_SIZE;
		}
		else if (InterfaceByte == 'M')
		{
			Len = sprintf_s(Message, 200, "ADMIN:");
			Interface = INTERFACE_ADMIN_SIZE;
		}
		else if (InterfaceByte == 'B')
		{
			Len = sprintf_s(Message, 200, "PBX2:");
			Interface = INTERFACE_PBX2_SIZE;
		}

		Len += sprintf_s(Message + Len, 200 - Len, "在時間%d", Timestamp);
		Len += sprintf_s(Message + Len, 200 - Len, "透過外線%d", OutPortNo);
		if (DirectByte == 'O')
		{
			Len += sprintf_s(Message + Len, 200 - Len, "撥出到%s", CallID);
			CallDirect = PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT;
		}
		else if (DirectByte == 'I')
		{
			Len += sprintf_s(Message + Len, 200 - Len, "從%s撥入", CallID);
			CallDirect = PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN;
		}

		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_PBX_UART, Message);
		int MachineID;
		MachineID = AfxGetApp()->GetProfileInt("SystemSetting", "MachineID", 1);
		CallParams_T CallParams;
		memset(&CallParams, 0x00, sizeof(CallParams_T));
		CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
		CallParams.MachineID = MachineID;
		CallParams.OutPortNo = OutPortNo;
		CallParams.FromExtNo = 612;
		CallParams.pCallID = CallID;
		Ret = ClientSendCallStatus(CallDirect, &CallParams);
	}
}

static void ThreadPBXUart()
{
	class CADICTIPBXSubMachineDlg* pCADICTIPBXSubMachineDlg = (CADICTIPBXSubMachineDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTIPBXSubMachineDlg->GetViewLogWnd();
	CHAR	OneByte;
	int len;
	BOOL StartRecv = FALSE;
	PBXFormate_T InBuff;
	int Size = sizeof(PBXFormate_T);
	CHAR Message[200];

	while (1)
	{
		len = m_Uart.ReadData(&InBuff, Size);
		if (len > 0)
		{
			sprintf_s(Message, 200, "收到UART資料長度=%d", len);
			pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_PBX_UART, Message);
			if (len == Size)
			{
				sprintf_s(Message, 200, "收到UART資料=%s", (PCHAR)&InBuff);
				pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_PBX_UART, Message);
				DataInterpret(&InBuff);
			}
		}
		Sleep(1);
	}
}

BOOL PBXInterpreterOpen()
{
#if 0
	PBXFormate_T InBuff;
	memset(&InBuff, 0x20, sizeof(PBXFormate_T));
	memcpy(InBuff.Header, "$", 1);
	memcpy(InBuff.Interface, "T", 1);
	memcpy(InBuff.Direct, "O", 1);
	memcpy(InBuff.CallID, "0910505890", 10);
	memcpy(InBuff.OutPortNo, "05", 2);
	memcpy(InBuff.ChargeStartDate, "22/05/11", 8);
	memcpy(InBuff.ChargeStartTime, "10:24:45", 8);

	DataInterpret(&InBuff);
#endif
	class CADICTIPBXSubMachineDlg* pCADICTIPBXSubMachineDlg = (CADICTIPBXSubMachineDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTIPBXSubMachineDlg->GetViewLogWnd();
	int COMPort;
	COMPort = AfxGetApp()->GetProfileInt("SystemSetting", "COMPort", 1);

	int COMPortBaudRate;
	COMPortBaudRate = AfxGetApp()->GetProfileInt("SystemSetting", "COMPortBaudRate", 57600);

	CHAR Message[200];
	memset(Message, 0, 100);

	m_UartOpened = OpenUARTPort(COMPort, COMPortBaudRate);
	if (m_UartOpened == FALSE)
	{
		sprintf_s(Message, 200, "錯誤：開啟COM(%d)錯誤，請確定在[交換機設定]分頁中的COM設定是否正確。", COMPort);
		pCADICTIPBXSubMachineDlg->ShowMessage(Message);
		pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_PBX_UART, "開啟UART失敗");
	}
	else
	{
		pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_PBX_UART, "開啟UART成功");
		m_hThreadPBXUart = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadPBXUart, NULL, 0, NULL);
	}

	return m_UartOpened;
}

BOOL OpenUARTPort(int Port, int BaudRate)
{
	BOOL result;

	result = m_Uart.Open(Port, BaudRate, true);

	return result;
}

void CloseUARTPort()
{
	m_Uart.Close();
}
