// CViewLogDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallerIdBox.h"
#include "CViewLogDlg.h"
#include "afxdialogex.h"


// CViewLogDlg 對話方塊

IMPLEMENT_DYNAMIC(CViewLogDlg, CDialogEx)

CViewLogDlg::CViewLogDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CViewLogDlg, pParent)
{

}

CViewLogDlg::~CViewLogDlg()
{
}

void CViewLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_BtnSave);
	DDX_Control(pDX, IDCANCEL, m_BtnCancel);
	DDX_Control(pDX, IDC_STATIC_VIEW_MODE, m_TitleViewMode);
	DDX_Control(pDX, IDC_COMBO_LIST_VIEW_MODE, m_EditViewMode);
	DDX_Control(pDX, IDC_STATIC_VIEW_LEVEL, m_TitleViewLevel);
	DDX_Control(pDX, IDC_COMBO_VIEW_LEVEL, m_EditViewLevel);
	DDX_Control(pDX, IDC_STATIC_VIEW_DURATION, m_TitleViewDate);
	DDX_Control(pDX, IDC_DATETIMEPICKER_VIEW_DURATION, m_EditViewDate);
	DDX_Control(pDX, IDC_COMBO_VIEW_RANGE, m_EditViewRange);
	DDX_Control(pDX, IDC_BUTTON_CLEAR_LOG, m_BtnClearLog);
	DDX_Control(pDX, IDC_LIST_LOG, m_ListLog);
	DDX_Control(pDX, IDC_BUTTON_REFLASH_RECORD, m_BtnReflashLog);
	DDX_Control(pDX, IDC_STATIC_SELECT_VLINE, m_TitleSelectVLine);
	DDX_Control(pDX, IDC_COMBO_VLINE, m_SelectVLine);
}


BEGIN_MESSAGE_MAP(CViewLogDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR_LOG, &CViewLogDlg::OnBnClickedButtonClearLog)
	ON_CBN_SELCHANGE(IDC_COMBO_LIST_VIEW_MODE, &CViewLogDlg::OnCbnSelchangeComboListViewMode)
	ON_CBN_SELCHANGE(IDC_COMBO_VIEW_LEVEL, &CViewLogDlg::OnCbnSelchangeComboViewLevel)
	ON_CBN_SELCHANGE(IDC_COMBO_VIEW_RANGE, &CViewLogDlg::OnCbnSelchangeComboViewRange)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DATETIMEPICKER_VIEW_DURATION, &CViewLogDlg::OnDtnDatetimechangeDatetimepickerViewDuration)
	ON_BN_CLICKED(IDC_BUTTON_REFLASH_RECORD, &CViewLogDlg::OnBnClickedButtonReflashRecord)
	ON_CBN_SELCHANGE(IDC_COMBO_VLINE, &CViewLogDlg::OnCbnSelchangeComboVline)
END_MESSAGE_MAP()


// CViewLogDlg 訊息處理常式


BOOL CViewLogDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此加入額外的初始化
	CHAR Error[200];
	m_SqLiteResult = CheckLogTableInDB(Error);
	m_ButtonFontHeight = 35;
	m_ButtonFontWidth = m_ButtonFontHeight * 0.5;
	m_pButtonFont = new CFont();
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_ButtonFontHeight;
	lf.lfWidth = m_ButtonFontWidth;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "微軟正黑體");
	m_pButtonFont->CreateFontIndirect(&lf);

	CWnd* pParent;
	pParent = GetParent();
	RECT ClientRect;
	pParent->GetClientRect(&ClientRect);
	RECT ButtonPos;
	ButtonPos.right = ClientRect.right - 230;
	ButtonPos.top = ClientRect.bottom - 90;
	ButtonPos.bottom = ButtonPos.top + 50;
	ButtonPos.left = ButtonPos.right - 200;
	m_BtnSave.MoveWindow(CRect(ButtonPos));
	m_BtnSave.SetFont(m_pButtonFont);
	ButtonPos.right = ClientRect.right - 20;
	ButtonPos.top = ClientRect.bottom - 90;
	ButtonPos.bottom = ButtonPos.top + 50;
	ButtonPos.left = ButtonPos.right - 200;
	m_BtnCancel.MoveWindow(CRect(ButtonPos));
	m_BtnCancel.SetFont(m_pButtonFont);

	CString Title;
	Title = "選擇顯示紀錄類型：";
	m_TitleViewMode.SetWindowTextA(Title);
	RECT TitleViewModePos;
	TitleViewModePos.top = ClientRect.top + 15;
	TitleViewModePos.left = ClientRect.left;
	TitleViewModePos.right = TitleViewModePos.left + m_ButtonFontWidth * (Title.GetLength() + 2);
	TitleViewModePos.bottom = TitleViewModePos.top + m_ButtonFontHeight;
	m_TitleViewMode.MoveWindow(&TitleViewModePos);
	m_TitleViewMode.SetFont(m_pButtonFont);

	m_LogParam.Type = AfxGetApp()->GetProfileInt("SystemSetting", "ViewModeIndex", 0);
	m_EditViewMode.AddString("打開所有");
	m_EditViewMode.AddString("打開子機狀態");
	m_EditViewMode.AddString("打開來電盒UART");
	m_EditViewMode.AddString("關閉所有");
	m_EditViewMode.SetCurSel(m_LogParam.Type);
	RECT EditLVewModeRect;
	EditLVewModeRect.left = TitleViewModePos.right;
	EditLVewModeRect.top = TitleViewModePos.top;
	EditLVewModeRect.bottom = EditLVewModeRect.top + m_ButtonFontHeight * 12;
	EditLVewModeRect.right = EditLVewModeRect.left + m_ButtonFontWidth * 9 * 2;
	m_EditViewMode.MoveWindow(&EditLVewModeRect);
	m_EditViewMode.SetFont(m_pButtonFont);

	Title = "選擇顯示紀錄層級：";
	m_TitleViewLevel.SetWindowTextA(Title);
	RECT TitleViewLevelPos;
	TitleViewLevelPos.top = TitleViewModePos.top;
	TitleViewLevelPos.left = EditLVewModeRect.right + 20;
	TitleViewLevelPos.right = TitleViewLevelPos.left + m_ButtonFontWidth * (Title.GetLength() + 2);
	TitleViewLevelPos.bottom = TitleViewLevelPos.top + m_ButtonFontHeight;
	m_TitleViewLevel.MoveWindow(&TitleViewLevelPos);
	m_TitleViewLevel.SetFont(m_pButtonFont);

	m_LogParam.Level = AfxGetApp()->GetProfileInt("SystemSetting", "ViewLevel", 0);
	m_EditViewLevel.AddString("所有");
	m_EditViewLevel.AddString("訊息");
	m_EditViewLevel.AddString("錯誤");
	m_EditViewLevel.AddString("嚴重錯誤");
	m_EditViewLevel.SetCurSel(m_LogParam.Level);
	RECT EditViewLevelRect;
	EditViewLevelRect.left = TitleViewLevelPos.right;
	EditViewLevelRect.top = TitleViewLevelPos.top;
	EditViewLevelRect.bottom = EditViewLevelRect.top + m_ButtonFontHeight * 12;
	EditViewLevelRect.right = EditViewLevelRect.left + m_ButtonFontWidth * 5 * 2;
	m_EditViewLevel.MoveWindow(&EditViewLevelRect);
	m_EditViewLevel.SetFont(m_pButtonFont);

	Title = "選擇顯示虛擬外線：";
	m_TitleSelectVLine.SetWindowTextA(Title);
	RECT TitleSelectVLinePos;
	TitleSelectVLinePos.top = TitleViewLevelPos.bottom + 15;
	TitleSelectVLinePos.left = TitleViewModePos.left;
	TitleSelectVLinePos.right = TitleSelectVLinePos.left + m_ButtonFontWidth * (Title.GetLength() + 2);
	TitleSelectVLinePos.bottom = TitleSelectVLinePos.top + m_ButtonFontHeight;
	m_TitleSelectVLine.MoveWindow(&TitleSelectVLinePos);
	m_TitleSelectVLine.SetFont(m_pButtonFont);

	RECT EditViewVLineRect;
	EditViewVLineRect.left = TitleSelectVLinePos.right;
	EditViewVLineRect.top = TitleSelectVLinePos.top;
	EditViewVLineRect.bottom = EditViewVLineRect.top + m_ButtonFontHeight * 12;
	EditViewVLineRect.right = EditViewVLineRect.left + m_ButtonFontWidth * 10 * 2;
	m_SelectVLine.MoveWindow(&EditViewVLineRect);
	m_SelectVLine.SetFont(m_pButtonFont);

	Title = "選擇顯示日期：";
	m_TitleViewDate.SetWindowTextA(Title);
	RECT TitleViewDatePos;
	TitleViewDatePos.top = TitleSelectVLinePos.bottom + 15;
	TitleViewDatePos.left = TitleViewModePos.left;
	TitleViewDatePos.right = TitleViewDatePos.left + m_ButtonFontWidth * (Title.GetLength() + 2);
	TitleViewDatePos.bottom = TitleViewDatePos.top + m_ButtonFontHeight;
	m_TitleViewDate.MoveWindow(&TitleViewDatePos);
	m_TitleViewDate.SetFont(m_pButtonFont);

	int ViewDate;
	//ViewDate = AfxGetApp()->GetProfileInt("SystemSetting", "ViewLevel", 0);
	RECT EditViewDateRect;
	EditViewDateRect.left = TitleViewDatePos.right;
	EditViewDateRect.top = TitleViewDatePos.top;
	EditViewDateRect.bottom = EditViewDateRect.top + m_ButtonFontHeight * 1.2;
	EditViewDateRect.right = EditViewDateRect.left + 250;
	m_EditViewDate.MoveWindow(&EditViewDateRect);
	m_EditViewDate.SetFont(m_pButtonFont);

	m_EditViewRange.AddString("本日");
	m_EditViewRange.AddString("本周");
	m_EditViewRange.AddString("本月");
	m_EditViewRange.SetCurSel(m_ViewLogRangeIndex);
	RECT EditViewRangeRect;
	EditViewRangeRect.left = EditViewDateRect.right + 5;
	EditViewRangeRect.top = EditViewDateRect.top;
	EditViewRangeRect.bottom = EditViewRangeRect.top + m_ButtonFontHeight * (Title.GetLength() + 2);
	EditViewRangeRect.right = EditViewRangeRect.left + m_ButtonFontWidth * 3 * 2;
	m_EditViewRange.MoveWindow(&EditViewRangeRect);
	m_EditViewRange.SetFont(m_pButtonFont);

	CString BtnReflashLogText;
	m_BtnReflashLog.GetWindowTextA(BtnReflashLogText);
	RECT BtnReflashRect;
	BtnReflashRect.right = ClientRect.right - 280;
	BtnReflashRect.top = EditViewRangeRect.top;
	BtnReflashRect.bottom = EditViewRangeRect.top + m_ButtonFontHeight * 1.2;
	BtnReflashRect.left = BtnReflashRect.right - m_ButtonFontWidth * BtnReflashLogText.GetLength() * 1.2;
	m_BtnReflashLog.MoveWindow(&BtnReflashRect);
	m_BtnReflashLog.SetFont(m_pButtonFont);

	CString BtnClearLogText;
	m_BtnClearLog.GetWindowTextA(BtnClearLogText);
	RECT BtnClearLogRect;
	BtnClearLogRect.right = ClientRect.right - 10;
	BtnClearLogRect.top = EditViewRangeRect.top;
	BtnClearLogRect.bottom = BtnClearLogRect.top + m_ButtonFontHeight * 1.2;
	BtnClearLogRect.left = BtnClearLogRect.right - m_ButtonFontWidth * BtnClearLogText.GetLength() * 1.2;
	m_BtnClearLog.MoveWindow(&BtnClearLogRect);
	m_BtnClearLog.SetFont(m_pButtonFont);

	RECT LogBoxPos;
	LogBoxPos.right = ClientRect.right - 10;
	LogBoxPos.left = ClientRect.left + 10;
	LogBoxPos.top = EditViewDateRect.top + m_ButtonFontHeight * 2;
	LogBoxPos.bottom = ClientRect.bottom - m_ButtonFontHeight * 2;
	ListView_SetExtendedListViewStyle(m_ListLog.m_hWnd, LVS_EX_FULLROWSELECT);
	
	m_ListLog.MoveWindow(CRect(LogBoxPos));

	m_ListFontHeight = 20;
	m_ListFontWidth = m_ListFontHeight * 0.5;
	m_pListFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_ListFontHeight;
	lf.lfWidth = m_ListFontWidth;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "新細明體");
	m_pListFont->CreateFontIndirect(&lf);

	m_ListLog.SetFont(m_pListFont);

	m_ListLog.InsertColumn(ID_LIST_TYPE, "類型", LVCFMT_CENTER);
	m_ListLog.InsertColumn(ID_LIST_LEVEL, "層級", LVCFMT_CENTER);
	m_ListLog.InsertColumn(ID_LIST_VLINE, "虛擬外線編號", LVCFMT_CENTER);
	m_ListLog.InsertColumn(ID_LIST_DATE, "時間", LVCFMT_CENTER);
	m_ListLog.InsertColumn(ID_LIST_MESSAGE, "訊息內容", LVCFMT_LEFT);

	int ListWidth = LogBoxPos.right - LogBoxPos.left;
	m_ListLog.SetColumnWidth(ID_LIST_TYPE, ListWidth * 9 / 100);
	m_ListLog.SetColumnWidth(ID_LIST_LEVEL, ListWidth * 6 / 100);
	m_ListLog.SetColumnWidth(ID_LIST_VLINE, ListWidth * 9 / 100);
	m_ListLog.SetColumnWidth(ID_LIST_DATE, ListWidth * 20 / 100);
	m_ListLog.SetColumnWidth(ID_LIST_MESSAGE, ListWidth * 60 / 100);


	m_LogParam.VLineNo = AfxGetApp()->GetProfileInt("SystemSetting", "ViewVLine", 0);
	LoadAndShowLog();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX 屬性頁應傳回 FALSE
}



int CViewLogDlg::GetTimeRange(int* pStartTimestamp, int* pEndTimestamp)
{
	// TODO: 請在此新增您的實作程式碼.
	m_ViewLogRangeIndex = m_EditViewRange.GetCurSel();
	CTime ThisTime;
	m_EditViewDate.GetTime(ThisTime);
	time_t Timestamp = 0;
	struct tm timeinfo;
	int ThisWeekDay;
	ThisWeekDay = ThisTime.GetDayOfWeek();
	timeinfo.tm_year = ThisTime.GetYear() - 1900;
	timeinfo.tm_mon = ThisTime.GetMonth() - 1;    //months since January - [0,11]
	timeinfo.tm_mday = ThisTime.GetDay();          //day of the month - [1,31] 
	timeinfo.tm_hour = 00;         //hours since midnight - [0,23]
	timeinfo.tm_min = 00;          //minutes after the hour - [0,59]
	timeinfo.tm_sec = 00;          //seconds after the minute - [0,59]
	Timestamp = mktime(&timeinfo);

	if (m_ViewLogRangeIndex == 0)
	{
		*pStartTimestamp = Timestamp;
		*pEndTimestamp = Timestamp + 24 * 3600 - 1;
	}
	else if (m_ViewLogRangeIndex == 1)
	{
		*pStartTimestamp = Timestamp - ((ThisWeekDay - 1) * 24 * 3600);
		*pEndTimestamp = Timestamp + ((8 - ThisWeekDay) * 24 * 3600) - 1;
	}
	else if (m_ViewLogRangeIndex == 2)
	{
		timeinfo.tm_year = ThisTime.GetYear() - 1900;
		timeinfo.tm_mon = ThisTime.GetMonth() - 1;    //months since January - [0,11]
		timeinfo.tm_mday = 1;          //day of the month - [1,31] 
		timeinfo.tm_hour = 00;         //hours since midnight - [0,23]
		timeinfo.tm_min = 00;          //minutes after the hour - [0,59]
		timeinfo.tm_sec = 00;          //seconds after the minute - [0,59]
		*pStartTimestamp = mktime(&timeinfo);
		timeinfo.tm_year = ThisTime.GetYear() - 1900;
		timeinfo.tm_mon = ThisTime.GetMonth();    //months since January - [0,11]
		timeinfo.tm_mday = 1;          //day of the month - [1,31] 
		*pEndTimestamp = mktime(&timeinfo) - 1;
	}
	return 0;
}

int CViewLogDlg::WriteLog(int Level, int Type, int vLine, PCHAR pMessage)
{
	// TODO: 請在此新增您的實作程式碼.
	int Ret;
	CTime Today = CTime::GetCurrentTime();
	time_t Timestamp = 0;
	struct tm timeinfo;

	timeinfo.tm_year = Today.GetYear() - 1900;
	timeinfo.tm_mon = Today.GetMonth() - 1;    //months since January - [0,11]
	timeinfo.tm_mday = Today.GetDay();          //day of the month - [1,31] 
	timeinfo.tm_hour = Today.GetHour();         //hours since midnight - [0,23]
	timeinfo.tm_min = Today.GetMinute();          //minutes after the hour - [0,59]
	timeinfo.tm_sec = Today.GetSecond();          //seconds after the minute - [0,59]
	Timestamp = mktime(&timeinfo);
	if (m_SqLiteResult == 0)
	{
		Ret = InsertLogDB((int)Timestamp, Level, Type, vLine, pMessage);
		if (Ret == 0)
		{
			Log_T Log;
			Log.Level = Level;
			Log.Timestamp = Timestamp;
			Log.Type = Type;
			Log.ID = m_ListIndex;
			Log.vLineNo = vLine;

			InsertLogInList(&Log, pMessage);
		}
	}

	return 0;
}




void CViewLogDlg::OnBnClickedButtonClearLog()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	ClearLogDB();
	LoadAndShowLog();
}


void CViewLogDlg::OnCbnSelchangeComboListViewMode()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	m_LogParam.Type = m_EditViewMode.GetCurSel();
	AfxGetApp()->WriteProfileInt("SystemSetting", "ViewModeIndex", m_LogParam.Type);
	LoadAndShowLog();
}


void CViewLogDlg::OnCbnSelchangeComboViewLevel()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	m_LogParam.Level = m_EditViewLevel.GetCurSel();
	AfxGetApp()->WriteProfileInt("SystemSetting", "ViewLevel", m_LogParam.Level);
	LoadAndShowLog();
}


void CViewLogDlg::OnCbnSelchangeComboViewRange()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	LoadAndShowLog();
}


void CViewLogDlg::OnDtnDatetimechangeDatetimepickerViewDuration(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMDATETIMECHANGE pDTChange = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
	// TODO: 在此加入控制項告知處理常式程式碼
	LoadAndShowLog();

	*pResult = 0;
}


void CViewLogDlg::OnBnClickedButtonReflashRecord()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	LoadAndShowLog();
}


int CViewLogDlg::LoadAndShowLog()
{
	// TODO: 請在此新增您的實作程式碼.
	if (!m_ListLog)
		return -1;
	GetTimeRange(&m_LogParam.StartTimestamp, &m_LogParam.EndTimestamp);
	m_Numbers = GetLogNumberDB(&m_LogParam);
	if (m_Numbers > 0)
	{
		m_pLogData = (PLog_T)malloc(m_Numbers * sizeof(Log_T));
		QueryLogDB(&m_LogParam, m_pLogData);
	}
	LVITEM	lvI;
	lvI.mask = LVIF_TEXT;
	CHAR StrBuff[100];
	struct tm timeinfo;
	time_t ThisTimestamp;
	m_ListIndex = 0;
	m_ListLog.DeleteAllItems();
	PLog_T pLog;
	for (int i = 0; i < m_Numbers; i++)
	{
		pLog = &m_pLogData[i];
		lvI.iItem = m_ListIndex;
		lvI.iSubItem = 0;
		lvI.pszText = "1";
		lvI.cchTextMax = 60;
		m_ListLog.InsertItem(&lvI);
		if (pLog->Type == LOG_TYPE_SUB_MACHINE)
			sprintf_s(StrBuff, 100, "子機");
		else if (pLog->Type == LOG_TYPE_CALLERID_BOX_UART)
			sprintf_s(StrBuff, 100, "來電盒UART");
		m_ListLog.SetItemText(m_ListIndex, ID_LIST_TYPE, StrBuff);
		if (pLog->Level == LOG_LEVEL_MESSAGE)
			sprintf_s(StrBuff, 100, "訊息");
		else if (pLog->Level == LOG_LEVEL_ERROR)
			sprintf_s(StrBuff, 100, "錯誤");
		else if (pLog->Level == LOG_LEVEL_FATAL_ERROR)
			sprintf_s(StrBuff, 100, "嚴重錯誤");
		m_ListLog.SetItemText(m_ListIndex, ID_LIST_LEVEL, StrBuff);
		sprintf_s(StrBuff, 100, "%d", pLog->vLineNo);
		m_ListLog.SetItemText(m_ListIndex, ID_LIST_VLINE, StrBuff);
		ThisTimestamp = pLog->Timestamp;
		localtime_s(&timeinfo, (time_t*)&ThisTimestamp);
		sprintf_s(StrBuff, 100, "%04d/%02d/%02d %02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
		m_ListLog.SetItemText(m_ListIndex, ID_LIST_DATE, StrBuff);
		m_ListLog.SetItemText(m_ListIndex, ID_LIST_MESSAGE, pLog->Message);
		m_ListLog.SetItemState(m_ListIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_ListLog.EnsureVisible(m_ListIndex, FALSE);
		m_ListIndex++;
	}
	return 0;
}


int CViewLogDlg::InsertLogInList(PLog_T pLog, PCHAR pMessage)
{
	// TODO: 請在此新增您的實作程式碼.
	if (m_LogParam.Type != 0)
	{
		if (m_LogParam.Type == 3)
			return 0;
		else
		{
			if (pLog->Type != m_LogParam.Type)
				return 0;
		}
	}
	if (m_LogParam.Level != 0)
	{
		if (pLog->Level != m_LogParam.Level)
			return 0;
	}
	if (m_LogParam.VLineNo != 0)
	{
		if (pLog->vLineNo != m_LogParam.VLineNo)
			return 0;
	}

	struct tm timeinfo;
	time_t ThisTimestamp;
	CHAR StrBuff[100];
	LVITEM	lvI;
	lvI.mask = LVIF_TEXT;
	lvI.iItem = m_ListIndex;
	lvI.iSubItem = 0;
	lvI.pszText = "1";
	lvI.cchTextMax = 60;
	m_ListLog.InsertItem(&lvI);
	if (pLog->Type == LOG_TYPE_SUB_MACHINE)
		sprintf_s(StrBuff, 100, "子機");
	else if (pLog->Type == LOG_TYPE_CALLERID_BOX_UART)
		sprintf_s(StrBuff, 100, "來電盒UART");
	m_ListLog.SetItemText(m_ListIndex, ID_LIST_TYPE, StrBuff);
	if (pLog->Level == LOG_LEVEL_MESSAGE)
		sprintf_s(StrBuff, 100, "訊息");
	else if (pLog->Level == LOG_LEVEL_ERROR)
		sprintf_s(StrBuff, 100, "錯誤");
	else if (pLog->Level == LOG_LEVEL_FATAL_ERROR)
		sprintf_s(StrBuff, 100, "嚴重錯誤");
	m_ListLog.SetItemText(m_ListIndex, ID_LIST_LEVEL, StrBuff);
	sprintf_s(StrBuff, 100, "%d", pLog->vLineNo);
	m_ListLog.SetItemText(m_ListIndex, ID_LIST_VLINE, StrBuff);
	ThisTimestamp = pLog->Timestamp;
	localtime_s(&timeinfo, (time_t*)&ThisTimestamp);
	sprintf_s(StrBuff, 100, "%04d/%02d/%02d %02d:%02d:%02d", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
	m_ListLog.SetItemText(m_ListIndex, ID_LIST_DATE, StrBuff);
	m_ListLog.SetItemText(m_ListIndex, ID_LIST_MESSAGE, pMessage);
	m_ListLog.SetItemState(m_ListIndex, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_ListLog.EnsureVisible(m_ListIndex, FALSE);

	m_ListIndex++;
	return 0;
}


int CViewLogDlg::SetOutPortNumber(int OutPorts)
{
	// TODO: 請在此新增您的實作程式碼.
	m_LogParam.VLineNo = AfxGetApp()->GetProfileInt("SystemSetting", "ViewVLine", 0);
	m_SelectVLine.AddString("所有虛擬外線");
	CHAR Buff[100];
	for (int i = 1; i <= OutPorts; i++)
	{
		sprintf_s(Buff, 100, "虛擬外線%d", i);
		m_SelectVLine.AddString(Buff);
	}
	if(m_LogParam.VLineNo <= OutPorts)
		m_SelectVLine.SetCurSel(m_LogParam.VLineNo);
	else
	{
		/* 在選擇顯示虛擬外線的設定跑掉時,恢復成"所有虛擬外線" */
		m_LogParam.VLineNo = 0;
		m_SelectVLine.SetCurSel(m_LogParam.VLineNo);
		AfxGetApp()->WriteProfileInt("SystemSetting", "ViewVLine", m_LogParam.VLineNo);
	}

	return 0;
}


void CViewLogDlg::OnCbnSelchangeComboVline()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	m_LogParam.VLineNo = m_SelectVLine.GetCurSel();
	AfxGetApp()->WriteProfileInt("SystemSetting", "ViewVLine", m_LogParam.VLineNo);
	LoadAndShowLog();
}
