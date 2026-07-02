#include "pch.h"
#include "MachineClient.h"
#include "Socket.h"
#include "ADICTICallerIdBoxDlg.h"

static HANDLE m_hThreadMachineProtoco = NULL;
static BOOL m_LoginSuccess = FALSE;
static MachineResponsePackage_T m_MachineResponsePackage;
static BOOL m_WaitResponse = FALSE;
static BOOL m_IntentionalDisconnect = FALSE;

static void ThreadMachineConnect(LPVOID lParam)
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	PMachineResponsePackage_T pMachineResponsePackage;
	int Ret;

	while (1)
	{
		int Ret = RecvResp((PCHAR)&m_MachineResponsePackage);
		if (Ret < 0)
		{
			if (!m_IntentionalDisconnect)
			{
				pCADICTICallerIdBoxDlg->ShowMessage("!!!!離線!!!!-請關閉程式，並檢查伺服器及網路正常後再重啟程式。謝謝");
			}
			break;
		}
		m_WaitResponse = FALSE;
	}
}

static void WaitResponse()
{
	m_WaitResponse = TRUE;
	while (m_WaitResponse) { Sleep(1); };
}

static BOOL CheckMachineParam(int MachineType, int MachineID)
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	BOOL Ret = TRUE;
	
	if (MachineType != PACKAGE_MACHINE_TYPE_CALLERID_BOX)
	{
		pCADICTICallerIdBoxDlg->ShowMessage("錯誤：子機類型錯誤，請聯絡奕誠科技。");
		pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "子機類型錯誤");
		Ret = FALSE;
	}
	int SettingMachineID;
	SettingMachineID = AfxGetApp()->GetProfileInt("SystemSetting", "MachineID", 1);
	if (SettingMachineID != MachineID)
	{
		pCADICTICallerIdBoxDlg->ShowMessage("錯誤：子機編號錯誤，請聯絡奕誠科技。");
		pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "子機編號錯誤");
		Ret = FALSE;
	}

	return Ret;
}

static BOOL CheckOutPort(int OutPortNo)
{
	int OutPortNum;
	OutPortNum = AfxGetApp()->GetProfileInt("SystemSetting", "OutPortNum", 16);
	if (OutPortNum < OutPortNo)
	{
		return FALSE;
	}

	return TRUE;
}

static int LoginMachineServer(int MachineType, int MachineID, int OutPortNums, int ExtPortNums)
{
	int Ret = 0;

	Ret = SendLogin(MachineType, MachineID, OutPortNums, ExtPortNums);
	if (Ret > 0)
	{
		WaitResponse();
	}
	else
		Stop_Winsock();

	return Ret;
}

time_t ConvertDateToUTC(CTime* pToday)
{
	struct tm timeinfo;
	time_t UTC;

	timeinfo.tm_year = pToday->GetYear() - 1900;
	timeinfo.tm_mon = pToday->GetMonth() - 1;    //months since January - [0,11]
	timeinfo.tm_mday = pToday->GetDay();          //day of the month - [1,31] 
	timeinfo.tm_hour = pToday->GetHour();         //hours since midnight - [0,23]
	timeinfo.tm_min = pToday->GetMinute();          //minutes after the hour - [0,59]
	timeinfo.tm_sec = pToday->GetSecond();          //seconds after the minute - [0,59]
	UTC = mktime(&timeinfo);

	return UTC;
}


// 子機設定（伺服器網址/子機編號）改了之後會呼叫這裡先斷線，確保舊的
// socket/背景執行緒先關乾淨，不會留著孤兒 handle，也不會讓新舊兩個
// 執行緒搶同一條 socket 收資料。
int StopMachineClient()
{
	m_IntentionalDisconnect = TRUE;
	m_LoginSuccess = FALSE;

	Close_TCP_Sock();

	if (m_hThreadMachineProtoco != NULL)
	{
		WaitForSingleObject(m_hThreadMachineProtoco, 2000);
		CloseHandle(m_hThreadMachineProtoco);
		m_hThreadMachineProtoco = NULL;
	}

	Stop_Winsock();
	m_IntentionalDisconnect = FALSE;

	return 0;
}

int StartMachineClient()
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	CHAR ServerIP[100];
	memset(ServerIP, 0x00, 100);
	int Ret = 0;
	CString MachineServerIP;

	// 可能是子機設定變更後重新呼叫，先確保舊連線關乾淨再重連。
	if (m_LoginSuccess || m_hThreadMachineProtoco != NULL)
	{
		StopMachineClient();
	}

	pCADICTICallerIdBoxDlg->ShowMessage("與伺服器連線中，請稍後!!!!!");

	MachineServerIP = AfxGetApp()->GetProfileString("SystemSetting", "MachineServerIP", "");
	Ret = InitMachineClient(MachineServerIP.GetBuffer());
	if (Ret == -1)
	{
		pCADICTICallerIdBoxDlg->ShowMessage("錯誤：子機網路初始錯誤。");
		pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "子機網路初始錯誤");
	}
	else if (Ret == -2)
	{
		pCADICTICallerIdBoxDlg->ShowMessage("錯誤：子機連線失敗，請確定伺服器是否開啟或[子機伺服器網址]設定是否正確。");
		pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "子機連線失敗");
	}

	if (Ret == 0)
	{
		m_hThreadMachineProtoco = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadMachineConnect, NULL, 0, NULL);
		if (m_hThreadMachineProtoco != NULL)
		{
			int Ret;
			int MachineID;
			MachineID = AfxGetApp()->GetProfileInt("SystemSetting", "MachineID", 1);
			int OutPortNum;
			OutPortNum = AfxGetApp()->GetProfileInt("SystemSetting", "OutPortNum", 16);
			int ExtPortNum;
			ExtPortNum = AfxGetApp()->GetProfileInt("SystemSetting", "ExtPortNum", 8);
			Ret = LoginMachineServer(2, MachineID, OutPortNum, ExtPortNum);
			CHAR Message[100];
			memset(Message, 0, 100);
			if (Ret > 0)
			{
				if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_LOGIN_OK)
				{
					sprintf_s(Message, 100, "與伺服器連線成功。");
					pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_SUB_MACHINE, 0, "與伺服器連線成功");
					m_LoginSuccess = TRUE;
				}
				else if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_LOGIN_ALREADY)
				{
					sprintf_s(Message, 100, "錯誤：機碼%d已登入。", MachineID);
					pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "機碼已登入");
				}
				else if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_MACHINE_NOT_SETTING)
				{
					sprintf_s(Message, 100, "錯誤：在伺服器的機碼%d尚未設定Port數。", MachineID);
					pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "尚未設定外線數");
				}
				else if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_MACHINE_OUTPORTS_NOTMATCH)
				{
					sprintf_s(Message, 100, "錯誤：與在伺服器==>分頁[交換機類型]==>合併機碼[%d]==>外線數量[%d]不符。", MachineID, m_MachineResponsePackage.OutPortNum);
					pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "外線數量不符");
				}
				else if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_MACHINE_EXTPORTS_NOTMATCH)
				{
					sprintf_s(Message, 100, "錯誤：與在伺服器==>分頁[交換機類型]==>合併機碼[%d]==>內線數量[%d]不符。", MachineID, m_MachineResponsePackage.ExtPortNum);
					pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "內線數量不符");
				}
				else if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_OVER_MACHINE_CALLBLOCK_NUMS)
				{
					sprintf_s(Message, 100, "錯誤：超過子機數。");
					pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "超過子機數");
				}
			}
			else
			{
				sprintf_s(Message, 100, "登入失敗");
				pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "登入失敗");
			}
			pCADICTICallerIdBoxDlg->ShowMessage(Message);
		}
	}

	if (m_LoginSuccess == TRUE)
		return 0;
	
	return -1;
}

int InitMachineClient(PCHAR pServerIPAddr)
{
	if (Start_Winsock() != 0)
    {
        return -1;
    }
    int Ret = 0;
    Ret = Create_TCP_Sock(3015, pServerIPAddr);
    if (Ret < 0)
    {
        return -2;
    }

	return 0;
}

int ClientSendCallStatus(int CallType, PCallParams_T pCallParams)
{
	class CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
	class CViewLogDlg* pViewLogDlg = (CViewLogDlg*)pCADICTICallerIdBoxDlg->GetViewLogWnd();
	CHAR Message[500];
	int Len = 0;
	if (m_LoginSuccess == FALSE)
		return -1;

	if (CheckMachineParam(pCallParams->MachineType, pCallParams->MachineID) == FALSE)
	{
		return -1;
	}

	if (CheckOutPort(pCallParams->OutPortNo) == FALSE && pCallParams->OutPortNo > 0)
	{
		pCADICTICallerIdBoxDlg->ShowMessage("錯誤：外線編號 > 該子機的外線數量設定，請檢查參數設定。");
		pViewLogDlg->WriteLog(LOG_LEVEL_ERROR, LOG_TYPE_SUB_MACHINE, 0, "外線編號編號超過設定");
		return -1;
	}

	switch (CallType)
	{
	case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN:
		Len = sprintf_s(Message + Len, 200 - Len, "外線號碼%s從外線編號%d撥入", pCallParams->pCallID, (int)pCallParams->OutPortNo);
		break;
	case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT:
		Len = sprintf_s(Message + Len, 200 - Len, "分機號碼%d從外線編號%d撥出外線號碼%s", pCallParams->FromExtNo, pCallParams->OutPortNo, pCallParams->pCallID);
		break;
	case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER:
		Len = sprintf_s(Message + Len, 200 - Len, "外線號碼%s與分機號碼%d通話,響鈴次數%d", pCallParams->pCallID, pCallParams->FromExtNo, pCallParams->RingNums);
		break;
	case PACKAGE_EVENT_TYPE_EXTERNAL_SECOND_DIAL:
		Len = sprintf_s(Message + Len, 200 - Len, "外線號碼%s撥入後的二次撥號碼%s", pCallParams->pCallID, pCallParams->SecondDialNo);
		break;
	case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP:
		Len = sprintf_s(Message + Len, 200 - Len, "外線號碼%s與分機號碼%d結束通話,通話時間%d", pCallParams->pCallID, pCallParams->FromExtNo, pCallParams->Duration);
		break;
	case PACKAGE_EVENT_TYPE_INTERNAL_CALL_MAKE:
		Len = sprintf_s(Message + Len, 200 - Len, "內線撥打");
		break;
	case PACKAGE_EVENT_TYPE_INTERNAL_CALL_ANSWER:
		Len = sprintf_s(Message + Len, 200 - Len, "內線通話");
		break;
	case PACKAGE_EVENT_TYPE_INTERNAL_SECOND_DIAL:
		Len = sprintf_s(Message + Len, 200 - Len, "內線二次撥號");
		break;
	case PACKAGE_EVENT_TYPE_INTERNAL_CALL_HANGUP:
		Len = sprintf_s(Message + Len, 200 - Len, "內線掛斷");
		break;
	case PACKAGE_EVENT_TYPE_VOICE_REC_FILENAME:
		Len = sprintf_s(Message + Len, 200 - Len, "外線號碼%s與分機號碼%d的錄音檔名%s", pCallParams->pCallID, pCallParams->FromExtNo, pCallParams->pVoiceRecFileName);
		break;
	}

	pViewLogDlg->WriteLog(LOG_LEVEL_MESSAGE, LOG_TYPE_SUB_MACHINE, pCallParams->OutPortNo, Message);

	int Ret;
	PMachineResponsePackage_T pMachineResponsePackage = NULL;
	CTime Today = CTime::GetCurrentTime();
	time_t Timestamp;
	CHAR Date[100];
	memset(Message, 0, 500);
	sprintf_s(Date, 100, "%02d:%02d:%02d", Today.GetHour(), Today.GetMinute(), Today.GetSecond());
	Timestamp = ConvertDateToUTC(&Today);
	switch (CallType)
	{
	case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN:
		sprintf_s(Message, 100, "外線撥入：時間=%s 外線號碼=%d 來電碼=%s", Date, pCallParams->OutPortNo, pCallParams->pCallID);
		Ret = SendExternalCallIn(pCallParams->MachineType, pCallParams->MachineID, Timestamp, pCallParams->OutPortNo, pCallParams->pCallID);
		break;
	case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT:
		sprintf_s(Message, 100, "外線撥出：時間=%s 外線號碼=%d 來電碼=%s 分機號碼=%d", Date, pCallParams->OutPortNo, pCallParams->pCallID, pCallParams->FromExtNo);
		Ret = SendExternalCallOut(pCallParams->MachineType, pCallParams->MachineID, Timestamp, pCallParams->OutPortNo, pCallParams->FromExtNo, pCallParams->pCallID);
		break;
	case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER:
		sprintf_s(Message, 100, "通話：時間=%s 外線號碼=%d與分機號碼=%d,振鈴次數=%d次,去來電碼=%s[通話中]", Date, pCallParams->OutPortNo, pCallParams->FromExtNo, pCallParams->RingNums, pCallParams->pCallID);
		Ret = SendExternalCallAnswer(pCallParams->MachineType, pCallParams->MachineID, Timestamp, pCallParams->OutPortNo, pCallParams->FromExtNo, pCallParams->RingNums, pCallParams->pCallID);
		break;
	case PACKAGE_EVENT_TYPE_EXTERNAL_SECOND_DIAL:
		break;
	case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP:
		sprintf_s(Message, 100, "掛斷：時間=%s 外線號碼=%d與分機號碼=%d,通話時間=%d秒[通話結束]", Date, pCallParams->OutPortNo, pCallParams->OutPortNo, pCallParams->Duration);
		Ret = SendExternalCallHangup(pCallParams->MachineType, pCallParams->MachineID, Timestamp, pCallParams->OutPortNo, pCallParams->Duration);
		break;
	case PACKAGE_EVENT_TYPE_INTERNAL_CALL_MAKE:
		Ret = SendInternalCallMake(pCallParams->MachineType, pCallParams->MachineID, Timestamp, pCallParams->FromExtNo, pCallParams->ToExtNo);
		break;
	case PACKAGE_EVENT_TYPE_INTERNAL_CALL_ANSWER:
		Ret = SendInternalCallAnswer(pCallParams->MachineType, pCallParams->MachineID, Timestamp, pCallParams->FromExtNo, pCallParams->ToExtNo, pCallParams->RingNums);
		break;
	case PACKAGE_EVENT_TYPE_INTERNAL_SECOND_DIAL:
		break;
	case PACKAGE_EVENT_TYPE_INTERNAL_CALL_HANGUP:
		Ret = SendInternalCallHangup(pCallParams->MachineType, pCallParams->MachineID, Timestamp, pCallParams->FromExtNo, pCallParams->ToExtNo, pCallParams->Duration);
		break;
	case PACKAGE_EVENT_TYPE_VOICE_REC_FILENAME:
		Ret = SendVoiceFileName(pCallParams->MachineType, pCallParams->MachineID, Timestamp, pCallParams->OutPortNo, pCallParams->FromExtNo, pCallParams->pVoiceRecFileName);
		break;
	}

	if (Ret > 0)
	{
		WaitResponse();
		if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_MACHINE_TYPE_NOTMATCH)
		{
			sprintf_s(Message, 100, "錯誤：發送事件的子機類型與註冊時不相同。");
		}
		else if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST)
		{
			sprintf_s(Message, 100, "錯誤：[%d]分機號碼不存在，請在主機的交換機類型頁簽中的合併機碼[%d]設定正確的分機號碼。", pCallParams->FromExtNo, pCallParams->MachineID);
		}
		else if (m_MachineResponsePackage.ResponseID == PACKAGE_RESPONSE_OUTPORT_OVER)
		{
			sprintf_s(Message, 100, "錯誤：外線號碼超過。");
		}
		pCADICTICallerIdBoxDlg->ShowMessage(Message);
	}
	else
		Stop_Winsock();
	
	return Ret;
}
