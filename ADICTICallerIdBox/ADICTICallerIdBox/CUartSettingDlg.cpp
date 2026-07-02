// CUartSettingDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallerIdBox.h"
#include "CUartSettingDlg.h"
#include "afxdialogex.h"
#include "CallerIdBoxInterpreter.h"
#include "CallerIdBoxInterpreter.h"
#include "CViewLogDlg.h"
#include "ADICTICallerIdBoxDlg.h"

// CUartSettingDlg 對話方塊
IMPLEMENT_DYNAMIC(CUartSettingDlg, CDialogEx)

CUartSettingDlg::CUartSettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CUartSettingDlg, pParent)
{

}

CUartSettingDlg::~CUartSettingDlg()
{
}

void CUartSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_BtnSave);
	DDX_Control(pDX, IDCANCEL, m_BtnCancel);
	DDX_Control(pDX, IDC_BUTTON_REFLASH, m_BtnReflashUart);
	DDX_Control(pDX, IDC_STATIC_COM_PORT, m_TitleCOMPortList);
	DDX_Control(pDX, IDC_COMBO_LIST_COM_PORTS, m_EditCOMPortSelect);
	DDX_Control(pDX, IDC_STATIC_COM_BAUD_RATE, m_TitleCOMPortBaudRate);
	DDX_Control(pDX, IDC_COMBO_COM_BAUD_RATE_LIST, m_EditCOMBaudRateList);
	DDX_Control(pDX, IDC_LIST_CALLERID_BOX, m_ListCallerIdBox);
	DDX_Control(pDX, IDC_STATIC_TITLE_LIST_CALLERID_BOX, m_TitleListCallerIdBox);
	DDX_Control(pDX, IDC_STATIC_TITLE_DTMF_TIMEOUT, m_ShowTitleDTMFTimeout);
	DDX_Control(pDX, IDC_EDIT_DTMG_TIMEOUT, m_EditDTMFTimeout);
}


BEGIN_MESSAGE_MAP(CUartSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CUartSettingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CUartSettingDlg::OnBnClickedCancel)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_CALLERID_BOX, &CUartSettingDlg::OnColorCallerIdBoxList)
	ON_BN_CLICKED(IDC_BUTTON_REFLASH, &CUartSettingDlg::OnBnClickedButtonReflash)
END_MESSAGE_MAP()


// CUartSettingDlg 訊息處理常式


BOOL CUartSettingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此加入額外的初始化
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
	ButtonPos.left = ClientRect.left + 20;
	ButtonPos.right = ButtonPos.left + 200;
	ButtonPos.top = ClientRect.bottom - 90;
	ButtonPos.bottom = ButtonPos.top + 50;
	m_BtnReflashUart.MoveWindow(CRect(ButtonPos));
	m_BtnReflashUart.SetFont(m_pButtonFont);
	
	CString Title;
	Title = "DTMF逾時設定(ms): ";
	m_ShowTitleDTMFTimeout.SetWindowTextA(Title);
	RECT TitleDTMFTimeoutRect;
	TitleDTMFTimeoutRect.top = ClientRect.top + 15;
	TitleDTMFTimeoutRect.left = ClientRect.left;
	TitleDTMFTimeoutRect.right = TitleDTMFTimeoutRect.left + Title.GetLength() * 20;
	TitleDTMFTimeoutRect.bottom = TitleDTMFTimeoutRect.top + m_ButtonFontHeight;
	m_ShowTitleDTMFTimeout.MoveWindow(&TitleDTMFTimeoutRect);
	m_ShowTitleDTMFTimeout.SetFont(m_pButtonFont);

	int DTMFTimeout;
	DTMFTimeout = AfxGetApp()->GetProfileInt("SystemSetting", "DTMFTimeout", 240);
	CHAR Buff[100];
	sprintf_s(Buff, 100, "%d", DTMFTimeout);
	m_EditDTMFTimeout.SetWindowTextA(Buff);
	RECT DTMFTimeoutRect;
	DTMFTimeoutRect.top = ClientRect.top + 15;
	DTMFTimeoutRect.left = TitleDTMFTimeoutRect.right;
	DTMFTimeoutRect.right = DTMFTimeoutRect.left + 100;
	DTMFTimeoutRect.bottom = DTMFTimeoutRect.top + m_ButtonFontHeight;
	m_EditDTMFTimeout.MoveWindow(&DTMFTimeoutRect);
	m_EditDTMFTimeout.SetFont(m_pButtonFont);

	Title = "來電盒列表 (綠色表示設定的來電盒被取代請按下[儲存]鈕,紅色表示設定的來電盒不存在!!!!)";
	m_TitleListCallerIdBox.SetWindowTextA(Title);
	RECT TitleCallerIdBoxRect;
	TitleCallerIdBoxRect.top = TitleDTMFTimeoutRect.bottom + 15;
	TitleCallerIdBoxRect.left = ClientRect.left;
	TitleCallerIdBoxRect.right = ClientRect.right;
	TitleCallerIdBoxRect.bottom = TitleCallerIdBoxRect.top + m_ButtonFontHeight;
	m_TitleListCallerIdBox.MoveWindow(&TitleCallerIdBoxRect);
	m_TitleListCallerIdBox.SetFont(m_pButtonFont);

	RECT ListCallerIdBoxRect;
	ListCallerIdBoxRect.left = ClientRect.left + 10;
	ListCallerIdBoxRect.top = TitleCallerIdBoxRect.bottom + 15;
	ListCallerIdBoxRect.bottom = ClientRect.bottom - 100;
	ListCallerIdBoxRect.right = ClientRect.right - 10;
	m_ListCallerIdBox.MoveWindow(&ListCallerIdBoxRect);
	//m_ListCallerIdBox.SetFont(m_pButtonFont);

	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_OUT_LINE_NO, "虛擬外線編號", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_INFO_IN_STORAGE, "訊息(設定)", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_SERIAL_NO_IN_STORAGE, "序號(設定)", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_OUT_PORTS_IN_STORAGE, "外線數(設定)", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_UART_PORT_IN_STORAGE, "串列埠(設定)", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_INFO_IN_CURRENT, "訊息(目前)", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_SERIAL_NO_IN_CURRENT, "序號(目前)", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_OUT_PORTS_IN_CURRENT, "外線數(目前)", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_UART_PORT_IN_CURRENT, "串列埠(目前)", LVCFMT_CENTER);
	m_ListCallerIdBox.InsertColumn(ID_LIST_BOX_SW_VER_IN_CURRENT, "韌體版本(目前)", LVCFMT_CENTER);

	int ListWidth = ListCallerIdBoxRect.right - ListCallerIdBoxRect.left;
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_OUT_LINE_NO, ListWidth * 6 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_INFO_IN_STORAGE, ListWidth * 10 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_SERIAL_NO_IN_STORAGE, ListWidth * 9 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_OUT_PORTS_IN_STORAGE, ListWidth * 9 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_UART_PORT_IN_STORAGE, ListWidth * 9 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_INFO_IN_CURRENT, ListWidth * 10 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_SERIAL_NO_IN_CURRENT, ListWidth * 9 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_OUT_PORTS_IN_CURRENT, ListWidth * 9 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_UART_PORT_IN_CURRENT, ListWidth * 9 / 100);
	m_ListCallerIdBox.SetColumnWidth(ID_LIST_BOX_SW_VER_IN_CURRENT, ListWidth * 9 / 100);
#if 0
	int COMPort;
	COMPort = AfxGetApp()->GetProfileInt("SystemSetting", "COMPort", 1);
	int i, Selected = 0;
	TCHAR	buff[50];
	for (i = 1; i < 128; i++)
	{
		if (OpenUARTPort(i, 38400))
		{
			CloseUARTPort();
			sprintf_s(buff, 50, "COM%d", i);
			m_EditCOMPortSelect.AddString(buff);
			if (COMPort == i)
				m_EditCOMPortSelect.SetCurSel(Selected);
			Selected++;
		}
	}

	CString Title;
	Title = "選擇ＣＯＭ埠： ";
	m_TitleCOMPortList.SetWindowTextA(Title);
	RECT TitleCOMPortListRect;
	TitleCOMPortListRect.top = ClientRect.top + 15;
	TitleCOMPortListRect.left = ClientRect.left;
	TitleCOMPortListRect.right = TitleCOMPortListRect.left + m_ButtonFontWidth * Title.GetLength();
	TitleCOMPortListRect.bottom = TitleCOMPortListRect.top + m_ButtonFontHeight;
	m_TitleCOMPortList.MoveWindow(&TitleCOMPortListRect);
	m_TitleCOMPortList.SetFont(m_pButtonFont);

	RECT EditCOMPortSelectRect;
	EditCOMPortSelectRect.left = TitleCOMPortListRect.right;
	EditCOMPortSelectRect.top = TitleCOMPortListRect.top;
	EditCOMPortSelectRect.bottom = EditCOMPortSelectRect.top + m_ButtonFontHeight * 12;
	EditCOMPortSelectRect.right = EditCOMPortSelectRect.left + 150;
	m_EditCOMPortSelect.MoveWindow(&EditCOMPortSelectRect);
	m_EditCOMPortSelect.SetFont(m_pButtonFont);

	Title = "選擇鮑率： ";
	m_TitleCOMPortBaudRate.SetWindowTextA(Title);
	RECT TitleCOMPortBaudRateRect;
	TitleCOMPortBaudRateRect.top = TitleCOMPortListRect.bottom + 15;
	TitleCOMPortBaudRateRect.left = TitleCOMPortListRect.left;
	TitleCOMPortBaudRateRect.right = TitleCOMPortBaudRateRect.left + m_ButtonFontWidth * Title.GetLength();
	TitleCOMPortBaudRateRect.bottom = TitleCOMPortBaudRateRect.top + m_ButtonFontHeight;
	m_TitleCOMPortBaudRate.MoveWindow(&TitleCOMPortBaudRateRect);
	m_TitleCOMPortBaudRate.SetFont(m_pButtonFont);

	UINT ListBaudRate[] = { 110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200 };
	int COMPortBaudRate;
	CString StrBaudRate;
	COMPortBaudRate = AfxGetApp()->GetProfileInt("SystemSetting", "COMPortBaudRate", 57600);
	for (int i = 0; i < 12; i++)
	{
		int BaudRate = ListBaudRate[i];
		sprintf_s(buff, 50, "%d", BaudRate);
		m_EditCOMBaudRateList.AddString(buff);
		if (BaudRate == COMPortBaudRate)
			m_EditCOMBaudRateList.SetCurSel(i);
	}

	RECT EditCOMBaudRateListRect;
	EditCOMBaudRateListRect.left = TitleCOMPortBaudRateRect.right;
	EditCOMBaudRateListRect.top = TitleCOMPortBaudRateRect.top;
	EditCOMBaudRateListRect.bottom = EditCOMBaudRateListRect.top + m_ButtonFontHeight * 12;
	EditCOMBaudRateListRect.right = EditCOMBaudRateListRect.left + 160;
	m_EditCOMBaudRateList.MoveWindow(&EditCOMBaudRateListRect);
	m_EditCOMBaudRateList.SetFont(m_pButtonFont);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX 屬性頁應傳回 FALSE
}


void CUartSettingDlg::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int COMPort;
	CString StrCOMPort;
	PCHAR Ptr;

	m_EditCOMPortSelect.GetWindowTextA(StrCOMPort);
	Ptr = StrCOMPort.GetBuffer();
	Ptr += 3;
	COMPort = atoi(Ptr);
	AfxGetApp()->WriteProfileInt("SystemSetting", "COMPort", COMPort);

	int BaudRate;
	CString StrBaudRate;
	m_EditCOMBaudRateList.GetWindowTextA(StrBaudRate);
	BaudRate = atoi(StrBaudRate.GetBuffer());
	AfxGetApp()->WriteProfileInt("SystemSetting", "COMPortBaudRate", BaudRate);

	int DTMFTimeout;
	CString StrDTMFTimeout;
	m_EditDTMFTimeout.GetWindowTextA(StrDTMFTimeout);
	DTMFTimeout = atoi(StrDTMFTimeout.GetBuffer());
	AfxGetApp()->WriteProfileInt("SystemSetting", "DTMFTimeout", DTMFTimeout);

	CallerIdBoxInfoSave();

	return;
	CDialogEx::OnOK();
}


void CUartSettingDlg::OnBnClickedCancel()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int COMPort;
	COMPort = AfxGetApp()->GetProfileInt("SystemSetting", "COMPort", 1);
	int i, Selected = 0;
	TCHAR	buff[50];
	int Counter = m_EditCOMPortSelect.GetCount();
	for (int i = 0; i < Counter; i++)
		m_EditCOMPortSelect.DeleteString(0);
	for (i = 1; i < 128; i++)
	{
		if (OpenUARTPort(i, 9600))
		{
			CloseUARTPort();
			sprintf_s(buff, 50, "COM%d", i);
			m_EditCOMPortSelect.AddString(buff);
			if (COMPort == i)
				m_EditCOMPortSelect.SetCurSel(Selected);
			Selected++;
		}
	}

	UINT ListBaudRate[] = { 110, 300, 600, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 115200 };
	int COMPortBaudRate;
	CString StrBaudRate;
	COMPortBaudRate = AfxGetApp()->GetProfileInt("SystemSetting", "COMPortBaudRate", 57600);
	for (int i = 0; i < 12; i++)
	{
		int BaudRate = ListBaudRate[i];
		if (BaudRate == COMPortBaudRate)
			m_EditCOMBaudRateList.SetCurSel(i);
	}

	return;
	CDialogEx::OnCancel();
}

void CUartSettingDlg::OnColorCallerIdBoxList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	// Take the default processing unless we set this to something else below.
	*pResult = 0;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.
	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		COLORREF crTextBrk;
		crTextBrk = RGB(255, 255, 255);
		if (pLVCD->nmcd.dwItemSpec % 2)
			crTextBrk = RGB(220, 220, 220);
		pLVCD->clrTextBk = crTextBrk;

		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT | CDDS_SUBITEM)
	{
		COLORREF crText;
		PVirtualLineInfo_T pVirtualLineInfo = (PVirtualLineInfo_T)pLVCD->nmcd.lItemlParam;
		if (pVirtualLineInfo->Matched == TRUE && pVirtualLineInfo->NotSame == FALSE)
			crText = RGB(0, 0, 255);
		else if (pVirtualLineInfo->Matched == TRUE && pVirtualLineInfo->NotSame == TRUE)
			crText = RGB(15, 151, 15);
		else
			crText = RGB(255, 0, 0);

		pLVCD->clrText = crText;

		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;
	}
}

void CUartSettingDlg::OnBnClickedButtonReflash()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	BeginWaitCursor();
	CloseUARTPort();
	m_ListCallerIdBox.DeleteAllItems();
	CallerIdBoxInterpreterOpen();
	EndWaitCursor();
}


BOOL CUartSettingDlg::DestroyWindow()
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別

	return CDialogEx::DestroyWindow();
}
