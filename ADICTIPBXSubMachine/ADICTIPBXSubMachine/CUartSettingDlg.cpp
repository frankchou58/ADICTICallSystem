// CUartSettingDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTIPBXSubMachine.h"
#include "CUartSettingDlg.h"
#include "afxdialogex.h"
#include "PBXInterpreter.h"

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
	DDX_Control(pDX, IDC_STATIC_COM_PORT, m_TitleCOMPortList);
	DDX_Control(pDX, IDC_COMBO_LIST_COM_PORTS, m_EditCOMPortSelect);
	DDX_Control(pDX, IDC_STATIC_COM_BAUD_RATE, m_TitleCOMPortBaudRate);
	DDX_Control(pDX, IDC_COMBO_COM_BAUD_RATE_LIST, m_EditCOMBaudRateList);
}


BEGIN_MESSAGE_MAP(CUartSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CUartSettingDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CUartSettingDlg::OnBnClickedCancel)
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

	int COMPort;
	COMPort = AfxGetApp()->GetProfileInt("SystemSetting", "COMPort", 1);
	int i, Selected = 0;
	TCHAR	buff[50];
	for (i = 0; i < 128; i++)
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
	for (i = 0; i < 128; i++)
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
