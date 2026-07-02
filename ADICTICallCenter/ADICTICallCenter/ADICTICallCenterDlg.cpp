
// ADICTICallCenterDlg.cpp: 實作檔案
//

#include "pch.h"
#include "framework.h"
#include "ADICTICallCenter.h"
#include "ADICTICallCenterDlg.h"
#include "afxdialogex.h"
#include "CallStateRecordEngine.h"
#include "CSettingsDlg.h"
#include "WSServer.h"

#define IDC_BTN_SET_STATUS 1000
extern INT Numbers[7];
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
public:
//	virtual BOOL Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
//	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
//	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


// CADICTICallCenterDlg 對話方塊



CADICTICallCenterDlg::CADICTICallCenterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ADICTICALLCENTER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CADICTICallCenterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//DDX_Control(pDX, IDC_LIST_SUB_PROGRAM, m_SubProgramList);
	DDX_Control(pDX, IDOK, m_CloseAPPBtn);
	//  DDX_Control(pDX, IDC_STATIC_TEST_BMP, m_TestBMP);
	DDX_Control(pDX, IDC_BUTTON_SHOW_EXT_LIST, m_ShowExtPortNumCorrespond);
	DDX_Control(pDX, IDC_TAB1, m_TabCtrl);
	//	DDX_Control(pDX, IDC_BTN_SET_STATUS, m_BtnStatusSetting);
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_Message);
}

BEGIN_MESSAGE_MAP(CADICTICallCenterDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
//	ON_WM_QUERYDRAGICON()
//	ON_WM_SIZE()
//ON_WM_SIZE()
//ON_WM_SIZING()
ON_WM_GETMINMAXINFO()
ON_WM_TIMER()
ON_BN_CLICKED(IDC_BUTTON_SHOW_EXT_LIST, &CADICTICallCenterDlg::OnBnClickedButtonShowExtList)
ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, & CADICTICallCenterDlg::OnTcnSelchangeTab1)
//ON_WM_CHILDACTIVATE()
//ON_WM_DRAWITEM()
ON_BN_CLICKED(IDOK, &CADICTICallCenterDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CADICTICallCenterDlg 訊息處理常式

BOOL CADICTICallCenterDlg::OnInitDialog()
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
	/*Launch Main Code*/
	InitCallStateRecordEngine();

	m_SubPrgramGroupIndex = 0;
	CRect rt;
	GetWindowRect(rt);
	int ListWidth = rt.Width();
	m_fontHeight = 13;
	m_font = new CFont();
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_fontHeight;
	//lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "新細明體");
	m_font->CreateFontIndirect(&lf);
	
	m_ListFontHeight = 20;
	m_ListFontWidth = m_ListFontHeight * 0.5;
	m_ListFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_ListFontHeight;
	lf.lfWeight = 700;
	lf.lfWidth = m_ListFontWidth;
	_tcscpy_s(lf.lfFaceName, "新細明體");
	m_ListFont->CreateFontIndirect(&lf);

	m_MessageFontHeight = 25;
	m_MessageFontWidth = m_MessageFontHeight * 0.5;
	m_MessageFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_MessageFontHeight;
	lf.lfWeight = 700;
	lf.lfWidth = m_MessageFontWidth;
	_tcscpy_s(lf.lfFaceName, "標楷體");
	m_MessageFont->CreateFontIndirect(&lf);

	m_DisplayRectLeft = 10;
	m_DisplayRectRight = ListWidth - 30;
	m_GroupRectLeft = m_DisplayRectLeft + 1;
	m_GroupRectRight = m_DisplayRectRight - 1;
	m_SubProgramListTitle.Create("子機連線列表", WS_CHILD | WS_VISIBLE | SS_CENTER,
		CRect(0, 0, 0, 0), this);
	m_SubProgramListTitle.SetFont(m_ListFont);
	//m_SubProgramListTitle.MoveWindow(CRect(m_DisplayRectLeft, 26, m_DisplayRectRight, 26 + m_ListFontHeight));

	m_SubProgramList.Create(WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LVS_REPORT | LVS_EDITLABELS,
		CRect(0, 0, 0, 0), this, 100);
	m_SubProgramList.SetFont(m_ListFont);
	ListView_SetExtendedListViewStyle(m_SubProgramList.m_hWnd, LVS_EX_FULLROWSELECT);
	m_SubProgramList.InsertColumn(ID_LIST_SUB_PROGRAM_ID, "辨識碼", LVCFMT_CENTER);	
	m_SubProgramList.InsertColumn(ID_LIST_SUB_PROGRAM_IP_ADDR, "IP位址", LVCFMT_CENTER);
	m_SubProgramList.InsertColumn(ID_LIST_SUB_PROGRAM_OUT_PORTS, "外線數量", LVCFMT_CENTER);
	m_SubProgramList.InsertColumn(ID_LIST_SUB_PROGRAM_EXT_PORTS, "內線數量", LVCFMT_CENTER);
	m_SubProgramList.InsertColumn(ID_LIST_SUB_PROGRAM_FW_VER, "子機版號", LVCFMT_CENTER);
	m_SubProgramList.InsertColumn(ID_LIST_SUB_PROGRAM_LINK_STATUS, "狀態", LVCFMT_CENTER);

	int Width = (ListWidth - 40) / 7;
	m_SubProgramList.SetColumnWidth(ID_LIST_SUB_PROGRAM_ID, Width);
	m_SubProgramList.SetColumnWidth(ID_LIST_SUB_PROGRAM_IP_ADDR, Width);
	m_SubProgramList.SetColumnWidth(ID_LIST_SUB_PROGRAM_OUT_PORTS, Width);
	m_SubProgramList.SetColumnWidth(ID_LIST_SUB_PROGRAM_EXT_PORTS, Width);
	m_SubProgramList.SetColumnWidth(ID_LIST_SUB_PROGRAM_FW_VER, Width);
	m_SubProgramList.SetColumnWidth(ID_LIST_SUB_PROGRAM_LINK_STATUS, Width);
	SetCloseBtnPos();
	SetExtCorrespondListBtnPos();
	//ShowSubProgramList();
	//ShowSubProgramPorts();
	//m_TabCtrl.Create(TCS_TABS | TCS_FIXEDWIDTH | WS_CHILD | WS_VISIBLE,
		//CRect(10, 30, 1895, 900), this, 2000);
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
	tcItem.pszText = _T("虛擬內外線狀態");
	m_TabCtrl.InsertItem(CTRL_ID_SHOW_PORT_STATUS, &tcItem);
	//tcItem.pszText = _T("虛擬外線通話狀態");
	//m_TabCtrl.InsertItem(CTRL_ID_OUTLINE_CALL_STATUS, &tcItem);
	tcItem.pszText = _T("交換機類型");
	m_TabCtrl.InsertItem(CTRL_ID_PBX_TYPE_UI, &tcItem);
	tcItem.pszText = _T("來電盒類型");
	m_TabCtrl.InsertItem(CTRL_ID_CALL_RECORDER_TYPE_UI, &tcItem);
	tcItem.pszText = _T("語音卡類型");
	m_TabCtrl.InsertItem(CTRL_ID_VOICE_RECORDER_TYPE_UI, &tcItem);
	tcItem.pszText = _T("網頁版控制台");
	m_TabCtrl.InsertItem(CTRL_ID_WEB_CONSOLE_UI, &tcItem);
	tcItem.pszText = _T("設定");
	m_TabCtrl.InsertItem(CTRL_ID_VOICE_SETTING_UI, &tcItem);

	m_BackEndConnect = m_DatabaseAccessURL.CheckBackEndURL();
	if (m_BackEndConnect == -2)
	{
		ShowMessage("請在[資料庫後端網址]中設定正確的網址!!!!!!");
	}
	else if (m_BackEndConnect == -1)
	{
		ShowMessage("無法連上資料庫後端伺服器，請在[資料庫後端網址]中設定正確的網址!!!!!!");
	}

	m_DataBase = m_DatabaseAccessURL.CheckDBTables();
	if (m_DataBase == -4)
	{
		ShowMessage("請檢查資料庫連線設定或新增一個ADICTICallCenter資料庫!!!!!!");
	}
	else if (m_DataBase == -3)
	{
		BOOL bCreateTable = TRUE;

#if 0
		if (bCreateTable)
		{
			if (m_DatabaseAccessURL.CreateDBOperatorTable() != 0)
			{
				ShowMessage("新增operator資料表失敗!!!!!!");
				bCreateTable = FALSE;
			}
		}
#endif
		if (bCreateTable)
		{
			if (m_DatabaseAccessURL.CreateDBMachineTable() != 0)
			{
				ShowMessage("新增machine資料表失敗!!!!!!");
				bCreateTable = FALSE;
			}
		}

		if (bCreateTable)
		{
			if (m_DatabaseAccessURL.CreateDBOutLineTable() != 0)
			{
				ShowMessage("新增outline資料表失敗!!!!!!");
				bCreateTable = FALSE;
			}
		}

		if (bCreateTable)
		{
			if (m_DatabaseAccessURL.CreateDBExtLineTable() != 0)
			{
				ShowMessage("新增extline資料表失敗!!!!!!");
				bCreateTable = FALSE;
			}
		}
#if 0
		if (bCreateTable)
		{
			if (m_DatabaseAccessURL.CreateDBRecordTable() != 0)
			{
				ShowMessage("新增record資料表失敗!!!!!!");
				bCreateTable = FALSE;
			}
		}
		if (bCreateTable)
		{
			if (m_DatabaseAccessURL.CreateDBAdminTable() != 0)
			{
				ShowMessage("新增admin資料表失敗!!!!!!");
				bCreateTable = FALSE;
			}
		}
#endif


		if (bCreateTable)
		{
			ShowMessage("新增所有資料表成功!!!!!!");
			m_DataBase = 0;
		}
	}

	/* 虛擬內外線顯示介面 */
	// 分頁列上不顯示這 4 個分頁（上面的 InsertItem 仍然註解），但底下的
	// Create()/m_pWndModleDlg 註冊/InitPorts() 一定要留著：子機連線列表的
	// 資料是存在這幾個分頁物件裡的，MachineServer.cpp/WSServer.cpp 在子機
	// 登入、更新虛擬線路狀態時都要透過 m_pWndModleDlg[...] 拿到這些物件的
	// 指標。拿掉這幾行會讓該指標一直是垃圾值，子機一連線呼叫 MachineLogin()
	// 就會在 CListCtrl::SetItemText 炸掉（存取違規）。
	m_ShowLinesStatusDlg.Create(IDD_CShowLinesStatusDlg, &m_TabCtrl);
	m_pWndModleDlg[CTRL_ID_SHOW_PORT_STATUS] = &m_ShowLinesStatusDlg;
	if(m_BackEndConnect == 0 && m_DataBase == 0)
		m_ShowLinesStatusDlg.InitPorts();
	/* 虛擬外線通話狀態顯示介面 */
	//m_OutLineCallStatusDlg.Create(IDD_COutLineCallStatusDlg, &m_TabCtrl);
	//m_pWndModleDlg[CTRL_ID_OUTLINE_CALL_STATUS] = &m_OutLineCallStatusDlg;
	/* 通話紀錄類型操作介面 */
	m_PBXTypeUIDlg.Create(IDD_CPbxDlg, &m_TabCtrl);
	m_pWndModleDlg[CTRL_ID_PBX_TYPE_UI] = &m_PBXTypeUIDlg;
	if (m_BackEndConnect == 0 && m_DataBase == 0)
		m_PBXTypeUIDlg.InitPorts();
	/* 通話紀錄類型操作介面 */
	m_CallRecorderTypeUIDlg.Create(IDD_CCallRecorderDlg, &m_TabCtrl);
	m_pWndModleDlg[CTRL_ID_CALL_RECORDER_TYPE_UI] = &m_CallRecorderTypeUIDlg;
	if (m_BackEndConnect == 0 && m_DataBase == 0)
		m_CallRecorderTypeUIDlg.InitPorts();
	/* 通話錄音類型操作介面 */
	m_VoiceRecorderTypeUIDlg.Create(IDD_CVoiceRecorderDlg, &m_TabCtrl);
	m_pWndModleDlg[CTRL_ID_VOICE_RECORDER_TYPE_UI] = &m_VoiceRecorderTypeUIDlg;
	if (m_BackEndConnect == 0 && m_DataBase == 0)
		m_VoiceRecorderTypeUIDlg.InitPorts();
	/* 設定操作介面 */
	m_SettingsUIDlg.Create(IDD_CSettingsDlg, &m_TabCtrl);
	m_pWndModleDlg[CTRL_ID_VOICE_SETTING_UI] = &m_SettingsUIDlg;
	/* 網頁版控制台（內嵌 ADICTICallCenter.Web，額外多一個分頁，不取代任何既有功能） */
	m_WebConsoleUIDlg.Create(IDD_CWebConsoleDlg, &m_TabCtrl);
	m_pWndModleDlg[CTRL_ID_WEB_CONSOLE_UI] = &m_WebConsoleUIDlg;

	CRect rs;
	m_TabCtrl.GetClientRect(&rs);
	//調整子對話方塊在父視窗中的位置 
	rs.top += 25;
	rs.bottom -= 10;
	rs.left += 1;
	rs.right -= 2;
	m_ShowLinesStatusDlg.MoveWindow(&rs);
	m_ShowLinesStatusDlg.ShowWindow(true);
	//m_OutLineCallStatusDlg.MoveWindow(&rs);
	//m_OutLineCallStatusDlg.ShowWindow(false);
	m_PBXTypeUIDlg.MoveWindow(&rs);
	m_PBXTypeUIDlg.ShowWindow(false);
	m_CallRecorderTypeUIDlg.MoveWindow(&rs);
	m_CallRecorderTypeUIDlg.ShowWindow(false);
	m_VoiceRecorderTypeUIDlg.MoveWindow(&rs);
	m_VoiceRecorderTypeUIDlg.ShowWindow(false);
	m_WebConsoleUIDlg.MoveWindow(&rs);
	m_WebConsoleUIDlg.ShowWindow(false);
	m_SettingsUIDlg.MoveWindow(&rs);
	m_SettingsUIDlg.ShowWindow(false);
	//SetTimer(1, 500, NULL);
	//SetTimer(2, 3500, NULL);
	//SetTimer(3, 500, NULL);

	SetMainWndPtr();
	if (m_BackEndConnect == 0 && m_DataBase == 0)
	{
		Start_Winsock();
		OpenMachineServer();
		OpenWebSocketServer();
	}

	return TRUE;  // 傳回 TRUE，除非您對控制項設定焦點
}

void CADICTICallCenterDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CADICTICallCenterDlg::OnPaint()
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
	//ShowSubProgramList();
}

// 當使用者拖曳最小化視窗時，
// 系統呼叫這個功能取得游標顯示。
//HCURSOR CADICTICallCenterDlg::OnQueryDragIcon()
//{
//	return static_cast<HCURSOR>(m_hIcon);
//}



//int CADICTICallCenterDlg::ShowSubProgramList()
//{
//	// TODO: 請在此新增您的實作程式碼.
//	return 0;
//}


//void CADICTICallCenterDlg::OnSize(UINT nType, int cx, int cy)
//{
//	CDialogEx::OnSize(nType, cx, cy);
//
//	// TODO: 在此加入您的訊息處理常式程式碼
//}


//void CADICTICallCenterDlg::OnSizing(UINT fwSide, LPRECT pRect)
//{
//	CDialogEx::OnSizing(fwSide, pRect);
//
//	// TODO: 在此加入您的訊息處理常式程式碼
//	SetCloseBtnPos();
//	SetSubProgramListPos();
//	ShowSubProgramList();
//}


void CADICTICallCenterDlg::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值

	lpMMI->ptMinTrackSize.x = 1925;
	lpMMI->ptMinTrackSize.y = 1050;
	lpMMI->ptMaxTrackSize.x = 1925;
	lpMMI->ptMaxTrackSize.y = 1050;

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}


int CADICTICallCenterDlg::SetCloseBtnPos()
{
	// TODO: 請在此新增您的實作程式碼.
	RECT ButtonNewPos;
	GetClientRect(&ButtonNewPos);
	ButtonNewPos.top = ButtonNewPos.bottom - 31;
	ButtonNewPos.left = ButtonNewPos.right - 100;
	ButtonNewPos.bottom = ButtonNewPos.top + 26;
	ButtonNewPos.right = ButtonNewPos.left + 90;
	m_CloseAPPBtn.MoveWindow(&ButtonNewPos, 1);

	return 0;
}

int CADICTICallCenterDlg::SetExtCorrespondListBtnPos()
{
	// TODO: 請在此新增您的實作程式碼.
	RECT ButtonNewPos;
	GetClientRect(&ButtonNewPos);
	ButtonNewPos.top = ButtonNewPos.bottom - 31;
	ButtonNewPos.left = ButtonNewPos.left + 10;
	ButtonNewPos.bottom = ButtonNewPos.top + 26;
	ButtonNewPos.right = ButtonNewPos.left + 120;
	m_ShowExtPortNumCorrespond.MoveWindow(&ButtonNewPos, 1);

	return 0;
}

int CADICTICallCenterDlg::ShowOutPortNo(int SubProgramIndex)
{
	// TODO: 請在此新增您的實作程式碼.
#if 0
	SubPrgramGroup_T* pSubPrgramGroup = &m_SubPrgramGroup[SubProgramIndex];
	m_GroupRectTop = m_GroupRectBottom;
	int LineY = m_GroupRectTop + m_fontHeight;
	int Index;
	int t, b, l, r;
	int t2 = 0, b2 = 0;
	int yEndNumber = LINE_NUMBERS / 32;
	int Less = LINE_NUMBERS % 32;
	if (Less)
		yEndNumber++;
	l = m_DisplayRectLeft + 5;

	/* 顯示外線狀態 */
	pSubPrgramGroup->pOutPortString->SetFont(m_font);
	pSubPrgramGroup->pOutPortString->MoveWindow(CRect(m_DisplayRectLeft + 2, LineY, m_DisplayRectRight - 5, LineY + m_fontHeight));
	t = m_fontHeight + LineY;
	for (int y = 0; y < yEndNumber; y++)
	{
		l = m_DisplayRectLeft + 5;
		if (y > 0)
			t += m_fontHeight * 2;
		b = t + m_fontHeight;
		for (int x = 0; x < 32; x++)
		{
			Index = y * 32 + x;
			//if (Index >= pSubPrgramGroup->OutPortNum)
			if (Index >= LINE_NUMBERS)
				break;
			if(x > 0)
				l += 58;
			r = l + 58;
			POutPort_T pOutPort = (POutPort_T)&pSubPrgramGroup->OutPorts[Index];
			if (pOutPort->IsInUsed)
				pOutPort->CallState = STATUS_CALL_OFF;
			CStatic* pOutPortText = pOutPort->pOutPortText;
			if (pOutPortText != NULL)
			{
				pOutPortText->SetFont(m_font);
				pOutPortText->MoveWindow(CRect(l, t, r, b));
			}
			t2 = t + m_fontHeight;
			b2 = b + m_fontHeight;
			CStatic* pOutPortIcon = pOutPort->pOutPortIcon;
			if (pOutPortIcon != NULL)
			{
				pOutPortIcon->SetFont(m_font);
				pOutPortIcon->SetWindowTextA("000");
				pOutPortIcon->MoveWindow(CRect(l, t2, r, b2));
			}
		}
	}

	if(b2 == 0)
		m_OutPortRectBottom = LineY + m_fontHeight * 2;
	else
		m_OutPortRectBottom = b2;
#endif
	return 0;
}


int CADICTICallCenterDlg::ShowExtPortNo(int SubProgramIndex)
{
	// TODO: 請在此新增您的實作程式碼.
#if 0
	SubPrgramGroup_T* pSubPrgramGroup = &m_SubPrgramGroup[SubProgramIndex];
	int LineY = m_OutPortRectBottom;
	int Index;
	int t, b, l, r;
	int t2 = 0, b2 = 0;
	int yEndNumber = EXT_NUMBERS / 32;
	int Less = EXT_NUMBERS % 32;
	if (Less)
		yEndNumber++;
	l = m_DisplayRectLeft + 5;
	/* 顯示內線狀態 */
	pSubPrgramGroup->pExtPortString->SetFont(m_font);
	pSubPrgramGroup->pExtPortString->MoveWindow(CRect(m_DisplayRectLeft + 2, LineY, m_DisplayRectRight - 5, LineY + m_fontHeight));
	t = m_fontHeight + LineY;
	for (int y = 0; y < yEndNumber; y++)
	{
		l = m_DisplayRectLeft + 5;
		if (y > 0)
			t += m_fontHeight * 2;
		b = t + m_fontHeight;
		for (int x = 0; x < 32; x++)
		{
			Index = y * 32 + x;
			//if (Index >= pSubPrgramGroup->ExtPortNum)
			if (Index >= EXT_NUMBERS)
				break;
			if (x > 0)
				l += 58;
			r = l + 58;
			b = t + m_fontHeight;
			PExtPort_T pExtPort = (PExtPort_T)&pSubPrgramGroup->ExtPorts[Index];
			if (pExtPort->IsInUsed)
				pExtPort->CallState = STATUS_CALL_OFF;
			CStatic* pExtPortText = pExtPort->pExtPortText;
			if (pExtPortText != NULL)
			{
				pExtPortText->SetFont(m_font);
				pExtPortText->MoveWindow(CRect(l, t, r, b));
			}
			t2 = t + m_fontHeight;
			b2 = b + m_fontHeight;
			CStatic* pExtPortIcon = pExtPort->pExtPortIcon;
			if (pExtPortIcon != NULL)
			{
				pExtPortIcon->SetFont(m_font);
				pExtPortIcon->SetWindowTextA("000");
				pExtPortIcon->MoveWindow(CRect(l, t2, r, b2));
			}
		}
	}

	if (b2 == 0)
	{
		m_ExtPortRectBottom = LineY + m_fontHeight;
		m_GroupRectBottom = m_ExtPortRectBottom + 5 + m_fontHeight;
	}
	else
	{
		m_ExtPortRectBottom = b2;
		m_GroupRectBottom = m_ExtPortRectBottom + 5;
	}

	int LessHeight = 1000 - m_GroupRectBottom;
#endif
	return 0;
}


#if 0
int CADICTICallCenterDlg::SetLineCallStatusLed(int SubProgramID, int LineNo, int ExtNo, int Status)
{
	// TODO: 請在此新增您的實作程式碼.
	int LineIndex = LineNo - 1;

	for (int i = 0; i < 4; i++)
	{
		PSubPrgramGroup_T pSubPrgramGroup = (PSubPrgramGroup_T)&m_SubPrgramGroup[i];
		if (pSubPrgramGroup->IsInUsed == TRUE && 
			pSubPrgramGroup->SubProgramID == SubProgramID)
		{
			if (LineNo < pSubPrgramGroup->OutPortNum)
			{
				POutPort_T pOutPort = (POutPort_T)&pSubPrgramGroup->OutPorts[LineIndex];
				pOutPort->CallState = Status;
				pOutPort->ExtNo = ExtNo;
				CHAR ExtString[100];
				sprintf_s(ExtString, 100, "%03d", ExtNo);
				pOutPort->pOutPortIcon->SetWindowTextA(ExtString);
			}
			if (ExtNo < pSubPrgramGroup->ExtPortNum)
			{
				PExtPort_T pExtPort = (PExtPort_T)&pSubPrgramGroup->ExtPorts[LineIndex];
				pExtPort->CallState = Status;
				pExtPort->ExtNo = ExtNo;				
				CHAR ExtString[100];
				sprintf_s(ExtString, 100, "%03d", LineNo);
				pExtPort->pExtPortIcon->SetWindowTextA(ExtString);
			}
		}
	}

	return 0;
}
#endif

int CADICTICallCenterDlg::SetExtCallStatusLed(int ExtNo, int LineNo, int Status)
{
	// TODO: 請在此新增您的實作程式碼.
#if 0
	int ExtIndex = ExtNo - 1;
	for (int i = 0; i < EXT_NUMBERS; i++)
	{
		if (m_ExtLines[i].ExtNo == ExtIndex)
		{
			m_ExtLines[ExtIndex].CallState = Status;
			CHAR ExtString[100];
			sprintf_s(ExtString, 100, "%03d", LineNo);
			m_ExtLines[ExtIndex].ExtIcon.SetWindowTextA(ExtString);
		}
	}
#endif
	return 0;
}

void CADICTICallCenterDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值
	switch (nIDEvent)
	{
	case 1:
		break;
	case 2:
		break;
	case 3:
		break;
	}
	CDialogEx::OnTimer(nIDEvent);
}


void CADICTICallCenterDlg::OnBnClickedButtonShowExtList()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CHAR Message[1000];
	int Len = 0;

	memset(Message, 0x00, 1000);
	for (int i = 0; i < m_ExtLineNumbers; i++)
	{
		//Len += sprintf_s(Message + Len, 1000 - Len, "%d <=> %d\n", m_ExtLines[i].PortNo, m_ExtLines[i].ExtNo);
	}
	MessageBox(Message, "實體內線與分機對應表", 0);
}


BOOL CADICTICallCenterDlg::CheckSubProgramIDExist(int SubProgramID)
{
	BOOL Ret = FALSE;
	for (int i = 0; i < SUBPROGRAM_NUMBERS; i++)
	{
		SubProgramGroup_T* pSubProgramGroup = &m_SubProgramGroup[i];
		if (pSubProgramGroup->SubProgramID == SubProgramID)
			Ret = TRUE;
	}
	return Ret;
}

int CADICTICallCenterDlg::InsertSubProgramGroup(PIPCEventHeader_T pConnectHeader, PIPCEventConnectBody_T pConnectBody)
{
#if 0
	if (m_SubPrgramGroupIndex == 4)
		return 0;
	int OutPortNum;
	int ExtPortNum;
	UINT OutPortMasks[5];
	UINT ExtPortMasks[5];
	int OutPortMaskIndex;
	int ExtPortMaskIndex;
	UINT Masks;
	int MaskBit;
	int Index;
	int MaskValue;
	TCHAR Buff[100];

	BOOL IsSubProgramIDExtsied = CheckSubProgramIDExist(pConnectHeader->SubProgramID);

	memcpy(OutPortMasks, pConnectBody->OutPortMask, sizeof(UINT) * 5);
	memcpy(ExtPortMasks, pConnectBody->ExtPortMask, sizeof(UINT) * 5);
	if(pConnectBody->OutPortNumbers > LINE_NUMBERS)
		OutPortNum = LINE_NUMBERS;
	else
		OutPortNum = pConnectBody->OutPortNumbers;
	if (pConnectBody->ExtPortNumbers > EXT_NUMBERS)
		ExtPortNum = EXT_NUMBERS;
	else
		ExtPortNum = pConnectBody->ExtPortNumbers;
	m_SubPrgramGroup[m_SubPrgramGroupIndex].OutPortNum = OutPortNum;
	m_SubPrgramGroup[m_SubPrgramGroupIndex].ExtPortNum = ExtPortNum;
#if 0
	OutPortMaskIndex = OutPortNum / 32;
	if (OutPortNum % 32 != 0)
		OutPortMaskIndex++;
	ExtPortMaskIndex = ExtPortNum / 32;
	if (ExtPortNum % 32 != 0)
		ExtPortMaskIndex++;
#endif
	OutPortMaskIndex = LINE_NUMBERS / 32;
	if (LINE_NUMBERS % 32 != 0)
		OutPortMaskIndex++;
	ExtPortMaskIndex = EXT_NUMBERS / 32;
	if (EXT_NUMBERS % 32 != 0)
		ExtPortMaskIndex++;
	SubPrgramGroup_T* pSubPrgramGroup = &m_SubPrgramGroup[m_SubPrgramGroupIndex];
	pSubPrgramGroup->Type = pConnectHeader->SubProgramType;
	pSubPrgramGroup->SubProgramID = pConnectHeader->SubProgramID;
	pSubPrgramGroup->IsInUsed = TRUE;
	pSubPrgramGroup->FWVer = pConnectBody->FWVer;
	pSubPrgramGroup->IsSubProgramIDExtsied = IsSubProgramIDExtsied;
	memcpy(pSubPrgramGroup->IPAddr, pConnectHeader->IPAddr, sizeof(CHAR) * 16);
	/* 建立GroupBox */
	CButton* pGroupBox = new CButton();
	sprintf_s(Buff, 100, "子機(辨識碼=%d)的內外線", pSubPrgramGroup->SubProgramID);
	pSubPrgramGroup->pGroupBox = pGroupBox;
	pGroupBox->Create(Buff, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
		CRect(0, 0, 0, 0), this, 100);
	/* 建立內外線文字及圖示 */
	pSubPrgramGroup->pOutPortString = new CStatic();
	pSubPrgramGroup->pOutPortString->Create(_T("外線狀態"), WS_CHILD | WS_VISIBLE | SS_CENTER,
		CRect(0, 0, 0, 0), this);
	pSubPrgramGroup->pExtPortString = new CStatic();
	pSubPrgramGroup->pExtPortString->Create(_T("內線狀態"), WS_CHILD | WS_VISIBLE | SS_CENTER,
		CRect(0, 0, 0, 0), this);
	for (int i = 0; i < SUBPROGRAM_NUMBERS; i++)
	{
		for (int j = 0; j < OutPortMaskIndex; j++)
		{
			Masks = OutPortMasks[j];
			int BitMask;
			for (int bit = 0; bit < 32; bit++)
			{
				Index = j * 32 + bit;
				POutPort_T pOutPort = (POutPort_T)&pSubPrgramGroup->OutPorts[Index];
				pOutPort->pOutPortText = new CStatic();
				CStatic* pOutPortText = pOutPort->pOutPortText;
				wsprintf(Buff, "%03d", Index + 1);
				pOutPortText->Create(Buff, WS_CHILD | WS_VISIBLE | SS_CENTER,
					CRect(0, 0, 0, 0), this);
				pOutPort->pOutPortIcon = new CStatic();
				pOutPort->CtrlID = 101 * (m_SubPrgramGroupIndex + 1) + Index;
				CStatic* pOutPortIcon = pOutPort->pOutPortIcon;
				pOutPortIcon->Create("", WS_CHILD | WS_VISIBLE | SS_CENTER,
					CRect(0, 0, 0, 0), this, pOutPort->CtrlID);
				BitMask = 1 << bit;
				MaskValue = Masks & BitMask;
				if (MaskValue != 0)
				{
					pOutPort->IsInUsed = TRUE;
				}
			}
		}
		for (int j = 0; j < ExtPortMaskIndex; j++)
		{
			Masks = ExtPortMasks[j];
			int BitMask;
			for (int bit = 0; bit < 32; bit++)
			{
				Index = j * 32 + bit;
				PExtPort_T pExtPort = (PExtPort_T)&pSubPrgramGroup->ExtPorts[Index];
				pExtPort->pExtPortText = new CStatic();
				CStatic* pExtPortText = pExtPort->pExtPortText;
				wsprintf(Buff, "%03d", Index + 1);
				pExtPortText->Create(Buff, WS_CHILD | WS_VISIBLE | SS_CENTER,
					CRect(0, 0, 0, 0), this);
				pExtPort->pExtPortIcon = new CStatic();
				pExtPort->CtrlID = 500 * (m_SubPrgramGroupIndex + 1) + Index;
				CStatic* pExtPortIcon = pExtPort->pExtPortIcon;
				pExtPortIcon->Create("", WS_CHILD | WS_VISIBLE | SS_CENTER,
					CRect(0, 0, 0, 0), this, pExtPort->CtrlID);
				BitMask = 1 << bit;
				MaskValue = Masks & BitMask;
				if (MaskValue != 0)
				{
					pExtPort->IsInUsed = TRUE;
				}
			}
		}
	}
	m_SubPrgramGroupIndex++;
#endif
	return 0;
}

int CADICTICallCenterDlg::ShowSubProgramGroup(int SubProgramIndex)
{
	/* Create Subprogram group Title */
	CHAR Buff[100];
	PSubProgramGroup_T pSubProgramGroup = (PSubProgramGroup_T)&m_SubProgramGroup[SubProgramIndex];
	//if (pSubProgramGroup->IsInUsed == TRUE)
	{
		pSubProgramGroup->pGroupBox->SetFont(m_font);
		pSubProgramGroup->pGroupBox->MoveWindow(CRect(m_GroupRectLeft, m_GroupRectTop, m_GroupRectRight, m_GroupRectBottom));
	}

	return 0;
}

//int CADICTICallCenterDlg::ShowSubProgramPorts()
//{
//#if 0
//	/* 計算目前有幾行 */
//	if (m_SubPrgramGroupIndex == 1)
//		m_fontHeight = 36;
//	else if (m_SubPrgramGroupIndex == 2)
//		m_fontHeight = 34;
//	else if (m_SubPrgramGroupIndex == 3)
//		m_fontHeight = 22;
//	else if (m_SubPrgramGroupIndex == 4)
//		m_fontHeight = 16;
//	int Lines = 0;
//	for (int i = 0; i < 4; i++)
//	{
//		PSubPrgramGroup_T pSubPrgramGroup = &m_SubPrgramGroup[i];
//		Lines += pSubPrgramGroup->OutPortNum / 32;
//		if (pSubPrgramGroup->OutPortNum % 32 != 0)
//			Lines++;
//		Lines += pSubPrgramGroup->ExtPortNum / 32;
//		if (pSubPrgramGroup->ExtPortNum % 32 != 0)
//			Lines++;
//	}
//	if (m_SubPrgramGroupIndex == 1)
//		Lines += 1;
//	else if (m_SubPrgramGroupIndex == 2)
//		Lines += 6;
//	else if (m_SubPrgramGroupIndex == 3)
//		Lines += 9;
//	else if (m_SubPrgramGroupIndex == 4)
//		Lines += 12;
//
//	if (Lines >= 16)
//		m_fontHeight = 17;
//	else if (Lines <= 10 && Lines > 8)
//		m_fontHeight = 20;
//	else if(Lines <= 8 && Lines > 6)
//		m_fontHeight = 22;
//	else if (Lines <= 6 && Lines >= 0)
//		m_fontHeight = 36;
//	m_font = new CFont();
//	LOGFONT lf;
//	memset(&lf, 0, sizeof(LOGFONT));
//	lf.lfHeight = m_fontHeight;
//	lf.lfWeight = 700;
//	_tcscpy_s(lf.lfFaceName, "新細明體");
//	m_font->CreateFontIndirect(&lf);
//
//	for (int i = 0; i < SUBPROGRAM_NUMBERS; i++)
//	{
//		PSubPrgramGroup_T pSubProgramGroup = (PSubPrgramGroup_T)&m_SubPrgramGroup[i];
//		if (pSubProgramGroup->IsInUsed == TRUE)
//		{
//			ShowOutPortNo(i);
//			ShowExtPortNo(i);
//			ShowSubProgramGroup(i);
//		}
//	}
//#endif
//
//	return 0;
//}



void CADICTICallCenterDlg::OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CShowLinesStatusDlg* pShowLinesStatusDlg = (CShowLinesStatusDlg*)m_pWndModleDlg[CTRL_ID_SHOW_PORT_STATUS];
	CCallRecorderDlg* pCallRecorderDlg = (CCallRecorderDlg*)m_pWndModleDlg[CTRL_ID_CALL_RECORDER_TYPE_UI];
	CPbxDlg* pPBXTypeDlg = (CPbxDlg*)m_pWndModleDlg[CTRL_ID_PBX_TYPE_UI];
	CVoiceRecorderDlg* pVoiceRecorderDlg = (CVoiceRecorderDlg*)m_pWndModleDlg[CTRL_ID_VOICE_RECORDER_TYPE_UI];

	int CurSel = m_TabCtrl.GetCurSel();
	switch (CurSel)
	{
#if 1
	case CTRL_ID_SHOW_PORT_STATUS:
		if (m_BackEndConnect == 0 && m_DataBase == 0)
			pShowLinesStatusDlg->RedrawIcons();
		m_ShowLinesStatusDlg.ShowWindow(true);
		//m_OutLineCallStatusDlg.ShowWindow(false);
		m_PBXTypeUIDlg.ShowWindow(false);
		m_CallRecorderTypeUIDlg.ShowWindow(false);
		m_VoiceRecorderTypeUIDlg.ShowWindow(false);
		m_SettingsUIDlg.ShowWindow(false);
		m_WebConsoleUIDlg.ShowWindow(false);
		break;
#if 0
	case CTRL_ID_OUTLINE_CALL_STATUS:
		m_ShowLinesStatusDlg.ShowWindow(false);
		m_OutLineCallStatusDlg.ShowWindow(true);
		m_PBXTypeUIDlg.ShowWindow(false);
		m_CallRecorderTypeUIDlg.ShowWindow(false);
		m_VoiceRecorderTypeUIDlg.ShowWindow(false);
		m_SettingsUIDlg.ShowWindow(false);
		break;
#endif
	case CTRL_ID_PBX_TYPE_UI:
		if (m_BackEndConnect == 0 && m_DataBase == 0)
			pPBXTypeDlg->RedrawIcons();
		m_ShowLinesStatusDlg.ShowWindow(false);
		//m_OutLineCallStatusDlg.ShowWindow(false);
		m_PBXTypeUIDlg.ShowWindow(true);
		m_CallRecorderTypeUIDlg.ShowWindow(false);
		m_VoiceRecorderTypeUIDlg.ShowWindow(false);
		m_SettingsUIDlg.ShowWindow(false);
		m_WebConsoleUIDlg.ShowWindow(false);
		break;
	case CTRL_ID_CALL_RECORDER_TYPE_UI:
		if (m_BackEndConnect == 0)
			pCallRecorderDlg->RedrawIcons();
		m_ShowLinesStatusDlg.ShowWindow(false);
		//m_OutLineCallStatusDlg.ShowWindow(false);
		m_PBXTypeUIDlg.ShowWindow(false);
		m_CallRecorderTypeUIDlg.ShowWindow(true);
		m_VoiceRecorderTypeUIDlg.ShowWindow(false);
		m_SettingsUIDlg.ShowWindow(false);
		m_WebConsoleUIDlg.ShowWindow(false);
		break;
	case CTRL_ID_VOICE_RECORDER_TYPE_UI:
		if (m_BackEndConnect == 0 && m_DataBase == 0)
			pVoiceRecorderDlg->RedrawIcons();
		m_ShowLinesStatusDlg.ShowWindow(false);
		//m_OutLineCallStatusDlg.ShowWindow(false);
		m_PBXTypeUIDlg.ShowWindow(false);
		m_CallRecorderTypeUIDlg.ShowWindow(false);
		m_VoiceRecorderTypeUIDlg.ShowWindow(true);
		m_SettingsUIDlg.ShowWindow(false);
		m_WebConsoleUIDlg.ShowWindow(false);
		break;
#endif
	case CTRL_ID_VOICE_SETTING_UI:
		m_ShowLinesStatusDlg.ShowWindow(false);
		//m_OutLineCallStatusDlg.ShowWindow(false);
		m_PBXTypeUIDlg.ShowWindow(false);
		m_CallRecorderTypeUIDlg.ShowWindow(false);
		m_VoiceRecorderTypeUIDlg.ShowWindow(false);
		m_SettingsUIDlg.ShowWindow(true);
		m_WebConsoleUIDlg.ShowWindow(false);
		break;
	case CTRL_ID_WEB_CONSOLE_UI:
		m_ShowLinesStatusDlg.ShowWindow(false);
		//m_OutLineCallStatusDlg.ShowWindow(false);
		m_PBXTypeUIDlg.ShowWindow(false);
		m_CallRecorderTypeUIDlg.ShowWindow(false);
		m_VoiceRecorderTypeUIDlg.ShowWindow(false);
		m_SettingsUIDlg.ShowWindow(false);
		m_WebConsoleUIDlg.ShowWindow(true);
		break;
	default:
		break;
	}

	*pResult = 0;
}



//BOOL CAboutDlg::Create(LPCTSTR lpszTemplateName, CWnd* pParentWnd)
//{
//	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
//
//	return CDialogEx::Create(lpszTemplateName, pParentWnd);
//}


void CADICTICallCenterDlg::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//CDialogEx::OnOK();
}


//void CAboutDlg::OnMouseMove(UINT nFlags, CPoint point)
//{
//	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值
//
//	CDialogEx::OnMouseMove(nFlags, point);
//}


void CADICTICallCenterDlg::ShowMessage(PCHAR pMessage)
{
	// TODO: 請在此新增您的實作程式碼.
	m_Message.SetWindowTextA("");
	m_Message.SetWindowTextA(pMessage);
}
