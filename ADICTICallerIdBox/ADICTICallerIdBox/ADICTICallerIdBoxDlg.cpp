
// ADICTICallerIdBoxDlg.cpp: 實作檔案
//

#include "pch.h"
#include "framework.h"
#include "ADICTICallerIdBox.h"
#include "ADICTICallerIdBoxDlg.h"
#include "afxdialogex.h"
//#include "adicticidu.h"
#include "MachineClient.h"
#include "CallerIdBoxInterpreter.h"
#include "TelStateMachine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 對 App About 使用 CAboutDlg 對話方塊

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

// 程式碼實作
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CADICTICallerIdBoxDlg 對話方塊



CADICTICallerIdBoxDlg::CADICTICallerIdBoxDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ADICTICALLERIDBOX_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_DeviceStarted = FALSE;
	m_DeviceCounter = 0;
	//  m_OutPorts = 0;
}

void CADICTICallerIdBoxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB1, m_TabCtrl);
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_Message);
}

BEGIN_MESSAGE_MAP(CADICTICallerIdBoxDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_GETMINMAXINFO()
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CADICTICallerIdBoxDlg::OnTcnSelchangeTab1)
	ON_WM_TIMER()
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CADICTICallerIdBoxDlg 訊息處理常式

BOOL CADICTICallerIdBoxDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 將 [關於...] 功能表加入系統功能表。

	// IDM_ABOUTBOX 必須在系統命令範圍之中。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 設定此對話方塊的圖示。當應用程式的主視窗不是對話方塊時，
	// 框架會自動從事此作業
	SetIcon(m_hIcon, TRUE);			// 設定大圖示
	SetIcon(m_hIcon, FALSE);		// 設定小圖示

	// TODO: 在此加入額外的初始設定
	CRect rt;
	GetWindowRect(rt);

	LOGFONT lf;

	m_MessageFontHeight = 25;
	m_MessageFontWidth = m_MessageFontHeight * 0.5;
	m_MessageFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_MessageFontHeight;
	lf.lfWeight = 700;
	lf.lfWidth = m_MessageFontWidth;
	_tcscpy_s(lf.lfFaceName, "標楷體");
	m_MessageFont->CreateFontIndirect(&lf);

	m_fontHeight = 22;
	m_fontWidth = m_fontHeight * 0.6;
	m_font = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_fontHeight;
	lf.lfWidth = m_fontWidth;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "微軟正黑體");
	m_font->CreateFontIndirect(&lf);

	m_TabCtrl.MoveWindow(CRect(10, 5, 1900, 980));
	m_TabCtrl.SetFont(m_font);

	RECT MessageRect;
	MessageRect.top = 980;
	MessageRect.left = 10;
	MessageRect.right = 1900;
	MessageRect.bottom = rt.bottom;
	m_Message.MoveWindow(&MessageRect);
	m_Message.SetFont(m_MessageFont);
	m_Message.SetWindowTextA("");

	TCITEM tcItem;
	tcItem.mask = TCIF_TEXT;
	tcItem.pszText = _T("子機設定");
	m_TabCtrl.InsertItem(CTRL_ID_MACHINE_SETTING_DLG, &tcItem);
	tcItem.pszText = _T("來電盒設定");
	m_TabCtrl.InsertItem(CTRL_ID_UART_SETTING_DLG, &tcItem);
	tcItem.pszText = _T("紀錄");
	m_TabCtrl.InsertItem(CTRL_ID_VIEW_LOG_DLG, &tcItem);

	CRect rs;
	m_TabCtrl.GetClientRect(&rs);
	//調整子對話方塊在父視窗中的位置 
	rs.top += 25;
	rs.bottom -= 10;
	rs.left += 1;
	rs.right -= 2;
	m_CSettingDlg.Create(IDD_CSettingDlg, &m_TabCtrl);
	m_CSettingDlg.MoveWindow(&rs);
	m_CUartSettingDlg.Create(IDD_CUartSettingDlg, &m_TabCtrl);
	m_CUartSettingDlg.MoveWindow(&rs);
	m_CViewLogDlg.Create(IDD_CViewLogDlg, &m_TabCtrl);
	m_CViewLogDlg.MoveWindow(&rs);

	m_CSettingDlg.ShowWindow(true);
	m_CUartSettingDlg.ShowWindow(false);
	m_CViewLogDlg.ShowWindow(false);

	CallerIdBoxInterpreterOpen();

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

void CADICTICallerIdBoxDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果將最小化按鈕加入您的對話方塊，您需要下列的程式碼，
// 以便繪製圖示。對於使用文件/檢視模式的 MFC 應用程式，
// 框架會自動完成此作業。

void CADICTICallerIdBoxDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 繪製的裝置內容

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 將圖示置中於用戶端矩形
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 描繪圖示
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 當使用者拖曳最小化視窗時，
// 系統呼叫這個功能取得游標顯示。
HCURSOR CADICTICallerIdBoxDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CADICTICallerIdBoxDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值

	lpMMI->ptMinTrackSize.x = 1925;
	lpMMI->ptMinTrackSize.y = 1050;
	lpMMI->ptMaxTrackSize.x = 1925;
	lpMMI->ptMaxTrackSize.y = 1050;


	CDialogEx::OnGetMinMaxInfo(lpMMI);
}


void CADICTICallerIdBoxDlg::OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此加入控制項告知處理常式程式碼
	int CurSel = m_TabCtrl.GetCurSel();
	switch (CurSel)
	{
	case CTRL_ID_MACHINE_SETTING_DLG:
		m_CSettingDlg.ShowWindow(true);
		m_CUartSettingDlg.ShowWindow(false);
		m_CViewLogDlg.ShowWindow(false);
		break;
	case CTRL_ID_UART_SETTING_DLG:
		m_CSettingDlg.ShowWindow(false);
		m_CUartSettingDlg.ShowWindow(true);
		m_CViewLogDlg.ShowWindow(false);
		break;
	case CTRL_ID_VIEW_LOG_DLG:
		m_CSettingDlg.ShowWindow(false);
		m_CUartSettingDlg.ShowWindow(false);
		m_CViewLogDlg.ShowWindow(true);
		break;
	}
	*pResult = 0;
}


int CADICTICallerIdBoxDlg::ShowMessage(PCHAR pMessage)
{
	// TODO: 請在此新增您的實作程式碼.
	m_Message.SetWindowTextA("");
	m_Message.SetWindowTextA(pMessage);

	return 0;
}


CWnd* CADICTICallerIdBoxDlg::GetViewLogWnd()
{
	// TODO: 請在此新增您的實作程式碼.
	return  (CWnd*)&m_CViewLogDlg;
}

CWnd* CADICTICallerIdBoxDlg::GetSettingWnd()
{
	// TODO: 請在此新增您的實作程式碼.
	return  (CWnd*)&m_CSettingDlg;
}

CWnd* CADICTICallerIdBoxDlg::GetUartSettingWnd()
{
	// TODO: 請在此新增您的實作程式碼.
	return  (CWnd*)&m_CUartSettingDlg;
}



void CADICTICallerIdBoxDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值

	CDialogEx::OnTimer(nIDEvent);
}


BOOL CADICTICallerIdBoxDlg::DestroyWindow()
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
	CloseAllHandle();

	return CDialogEx::DestroyWindow();
}


void CADICTICallerIdBoxDlg::OnClose()
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值

	CDialogEx::OnClose();
}
