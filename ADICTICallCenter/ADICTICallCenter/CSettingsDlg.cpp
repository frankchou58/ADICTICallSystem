// CSettingsDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallCenter.h"
#include "CSettingsDlg.h"
#include "afxdialogex.h"
#include "MachineServer.h"
#include "ADICTICallCenterDlg.h"

// CSettingsDlg 對話方塊
INT Numbers[LINES_NUM_ARRAY] = { 0, 8, 16, 24, 32, 64, 128, 240 };

IMPLEMENT_DYNAMIC(CSettingsDlg, CDialogEx)

CSettingsDlg::CSettingsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CSettingsDlg, pParent)
{

}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_OkButton);
	DDX_Control(pDX, IDCANCEL, m_CancelButton);
	DDX_Control(pDX, IDC_LIST_NETWORK_CARD, m_SelectNetworkDevice);
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_ShowMessageBox);
	DDX_Control(pDX, IDC_EDIT_URL_BACKEND_DATABASE, m_DatabaseBackEndURL);
	DDX_Control(pDX, IDC_EDIT2, m_TitleDatabaseBackEndURL);
	//  DDX_Control(pDX, IDC_EDIT3, m_TelNoPrioritySelect);
	DDX_Control(pDX, IDC_EDIT3, m_TelNoPriorityTitle);
	DDX_Control(pDX, IDC_COMBO1, m_TelNoPrioritySelect);
	DDX_Control(pDX, IDC_STATIC_CALL_STATUS_TO_DB, m_TitleCallStatusToDB);
	//  DDX_Control(pDX, IDC_COMBO_CALL_STATUS_TO_DB, m_EditCallStatusToDB);
	DDX_Control(pDX, IDC_COMBO_CALL_STATUS_TO_DB, m_EditCallStatusToDB);
	DDX_Control(pDX, IDC_STATIC_TITLE_BACKEND_URL_PORT, m_TitleDatabaseBackEndURLPort);
	DDX_Control(pDX, IDC_EDIT_URL_BACKEND_DATABASE_PORT, m_DatabaseBackEndURLPort);
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CSettingsDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CSettingsDlg::OnBnClickedCancel)
	//ON_CBN_SELCHANGE(IDC_COMBO_CALL_STATUS_TO_DB, &CSettingsDlg::OnCbnSelchangeComboCallStatusToDb)
END_MESSAGE_MAP()


// CSettingsDlg 訊息處理常式


BOOL CSettingsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// TODO:  在此加入額外的初始化
	//Start_Winsock();

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

	m_pParent = GetParent();

	RECT ClientRect;
	m_pParent->GetClientRect(&ClientRect);
	RECT ButtonPos;
	ButtonPos.right = ClientRect.right - 230;
	ButtonPos.top = ClientRect.bottom - 90;
	ButtonPos.bottom = ButtonPos.top + 50;
	ButtonPos.left = ButtonPos.right - 200;
	m_OkButton.MoveWindow(CRect(ButtonPos));
	m_OkButton.SetFont(m_pButtonFont);
	ButtonPos.right = ClientRect.right - 20;
	ButtonPos.top = ClientRect.bottom - 90;
	ButtonPos.bottom = ButtonPos.top + 50;
	ButtonPos.left = ButtonPos.right - 200;
	m_CancelButton.MoveWindow(CRect(ButtonPos));
	m_CancelButton.SetFont(m_pButtonFont);

	CString Title = "資料庫後端網址： ";
	m_TitleDatabaseBackEndURL.SetWindowTextA(Title);
	RECT TitleDatabaseBackEndURLRect;
	TitleDatabaseBackEndURLRect.top = ClientRect.top;
	TitleDatabaseBackEndURLRect.left = ClientRect.left;
	TitleDatabaseBackEndURLRect.right = TitleDatabaseBackEndURLRect.left + m_ButtonFontWidth * Title.GetLength();
	TitleDatabaseBackEndURLRect.bottom = TitleDatabaseBackEndURLRect.top + m_ButtonFontHeight;
	m_TitleDatabaseBackEndURL.MoveWindow(&TitleDatabaseBackEndURLRect);
	m_TitleDatabaseBackEndURL.SetFont(m_pButtonFont);

	CString DatabaseBackEndURL;
	DatabaseBackEndURL = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndURL", "");
	m_DatabaseBackEndURL.SetWindowTextA(DatabaseBackEndURL);
	RECT EditDatabaseBackEndURLRect;
	EditDatabaseBackEndURLRect.left = TitleDatabaseBackEndURLRect.right;
	EditDatabaseBackEndURLRect.bottom = TitleDatabaseBackEndURLRect.bottom;
	EditDatabaseBackEndURLRect.top = TitleDatabaseBackEndURLRect.top;
	EditDatabaseBackEndURLRect.right = EditDatabaseBackEndURLRect.left + 1000;
	m_DatabaseBackEndURL.MoveWindow(&EditDatabaseBackEndURLRect);
	m_DatabaseBackEndURL.SetFont(m_pButtonFont);

	Title = "埠號： ";
	m_TitleDatabaseBackEndURLPort.SetWindowTextA(Title);
	RECT TitleDatabaseBackEndURLPortRect;
	TitleDatabaseBackEndURLPortRect.top = EditDatabaseBackEndURLRect.top;
	TitleDatabaseBackEndURLPortRect.left = EditDatabaseBackEndURLRect.right + 50;
	TitleDatabaseBackEndURLPortRect.right = TitleDatabaseBackEndURLPortRect.left + m_ButtonFontWidth * Title.GetLength();
	TitleDatabaseBackEndURLPortRect.bottom = TitleDatabaseBackEndURLPortRect.top + m_ButtonFontHeight;
	m_TitleDatabaseBackEndURLPort.MoveWindow(&TitleDatabaseBackEndURLPortRect);
	m_TitleDatabaseBackEndURLPort.SetFont(m_pButtonFont);

	CString DatabaseBackEndURLPort;
	DatabaseBackEndURLPort = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndURLPort", "8830");
	m_DatabaseBackEndURLPort.SetWindowTextA(DatabaseBackEndURLPort);
	RECT EditDatabaseBackEndURLPortRect;
	EditDatabaseBackEndURLPortRect.left = TitleDatabaseBackEndURLPortRect.right;
	EditDatabaseBackEndURLPortRect.bottom = TitleDatabaseBackEndURLPortRect.bottom;
	EditDatabaseBackEndURLPortRect.top = TitleDatabaseBackEndURLPortRect.top;
	EditDatabaseBackEndURLPortRect.right = TitleDatabaseBackEndURLPortRect.left + 300;
	m_DatabaseBackEndURLPort.MoveWindow(&EditDatabaseBackEndURLPortRect);
	m_DatabaseBackEndURLPort.SetFont(m_pButtonFont);

	Title = "電話號碼決定優先： ";
	m_TelNoPriorityTitle.SetWindowTextA(Title);
	RECT TelNoPriorityTitle;
	TelNoPriorityTitle.top = TitleDatabaseBackEndURLRect.bottom + m_ButtonFontHeight;
	TelNoPriorityTitle.left = TitleDatabaseBackEndURLRect.left;
	TelNoPriorityTitle.right = TelNoPriorityTitle.left + m_ButtonFontWidth * Title.GetLength();
	TelNoPriorityTitle.bottom = TelNoPriorityTitle.top + m_ButtonFontHeight;
	m_TelNoPriorityTitle.MoveWindow(&TelNoPriorityTitle);
	m_TelNoPriorityTitle.SetFont(m_pButtonFont);

	m_TelNoPrioritySelect.AddString("以交換機優先");
	m_TelNoPrioritySelect.AddString("以來電盒優先");
	m_TelNoPrioritySelect.AddString("以語音卡優先");
	int TelNoPrioritySelectIndex;
	TelNoPrioritySelectIndex = AfxGetApp()->GetProfileInt("SystemSetting", "TelNoPrioritySelect", 0);
	m_TelNoPrioritySelect.SetCurSel(TelNoPrioritySelectIndex);
	RECT TelNoPrioritySelect;
	TelNoPrioritySelect.top = TelNoPriorityTitle.top - 3;
	TelNoPrioritySelect.left = TelNoPriorityTitle.right;
	TelNoPrioritySelect.right = TelNoPrioritySelect.left + m_ButtonFontWidth * 15;
	TelNoPrioritySelect.bottom = TelNoPriorityTitle.top + m_ButtonFontHeight * 5;
	m_TelNoPrioritySelect.MoveWindow(&TelNoPrioritySelect);
	m_TelNoPrioritySelect.SetFont(m_pButtonFont);

	Title = "外線狀態儲存到資料庫： ";
	m_TitleCallStatusToDB.SetWindowTextA(Title);
	RECT CallStatusToDBTitle;
	CallStatusToDBTitle.top = TelNoPriorityTitle.bottom + m_ButtonFontHeight;
	CallStatusToDBTitle.left = TelNoPriorityTitle.left;
	CallStatusToDBTitle.right = CallStatusToDBTitle.left + m_ButtonFontWidth * (Title.GetLength() + 0.3);
	CallStatusToDBTitle.bottom = CallStatusToDBTitle.top + m_ButtonFontHeight;
	m_TitleCallStatusToDB.MoveWindow(&CallStatusToDBTitle);
	m_TitleCallStatusToDB.SetFont(m_pButtonFont);

	m_EditCallStatusToDB.AddString("否");
	m_EditCallStatusToDB.AddString("是");
	int EditCallStatusToDBIndex;
	EditCallStatusToDBIndex = AfxGetApp()->GetProfileInt("SystemSetting", "EditCallStatusToDBIndex", 0);
	m_EditCallStatusToDB.SetCurSel(EditCallStatusToDBIndex);
	RECT EditCallStatusToDB;
	EditCallStatusToDB.top = CallStatusToDBTitle.top - 3;
	EditCallStatusToDB.left = CallStatusToDBTitle.right;
	EditCallStatusToDB.right = EditCallStatusToDB.left + m_ButtonFontWidth * 4;
	EditCallStatusToDB.bottom = EditCallStatusToDB.top + m_ButtonFontHeight * 5;
	m_EditCallStatusToDB.MoveWindow(&EditCallStatusToDB);
	m_EditCallStatusToDB.SetFont(m_pButtonFont);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX 屬性頁應傳回 FALSE
}


void CSettingsDlg::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//int Index;
	//int LineNumbers;
	//AfxGetApp()->WriteProfileInt("SystemSetting", "OutLineNum", LineNumbers);
	//AfxGetApp()->WriteProfileInt("SystemSetting", "ExtLineNum", LineNumbers);
	CString DatabaseBackEndURLOld;
	DatabaseBackEndURLOld = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndURL", "");

	CString DatabaseBackEndURL;
	m_DatabaseBackEndURL.GetWindowTextA(DatabaseBackEndURL);
	AfxGetApp()->WriteProfileString("SystemSetting", "DatabaseBackEndURL", DatabaseBackEndURL);

	CString DatabaseBackEndURLPort;
	m_DatabaseBackEndURLPort.GetWindowTextA(DatabaseBackEndURLPort);
	AfxGetApp()->WriteProfileString("SystemSetting", "DatabaseBackEndURLPort", DatabaseBackEndURLPort);

	int TelNoPrioritySelectIndex;
	TelNoPrioritySelectIndex = m_TelNoPrioritySelect.GetCurSel();
	AfxGetApp()->WriteProfileInt("SystemSetting", "TelNoPrioritySelect", TelNoPrioritySelectIndex);

	int EditCallStatusToDBIndex;
	EditCallStatusToDBIndex = m_EditCallStatusToDB.GetCurSel();
	AfxGetApp()->WriteProfileInt("SystemSetting", "EditCallStatusToDBIndex", EditCallStatusToDBIndex);

	if (DatabaseBackEndURLOld != DatabaseBackEndURL)
	{
		CADICTICallCenterDlg* pCADICTICallCenterDlg = (CADICTICallCenterDlg*)AfxGetApp()->m_pMainWnd;
		pCADICTICallCenterDlg->ShowMessage("資料庫後端網址設定已變更，請重啟伺服器。");
	}

	return;
	CDialogEx::OnOK();
}


void CSettingsDlg::OnBnClickedCancel()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CString DatabaseBackEndURL;
	DatabaseBackEndURL = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndURL", "");
	m_DatabaseBackEndURL.SetWindowTextA(DatabaseBackEndURL);

	CString DatabaseBackEndURLPort;
	DatabaseBackEndURLPort = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndURLPort", "");
	m_DatabaseBackEndURLPort.SetWindowTextA(DatabaseBackEndURLPort);

	return;
	CDialogEx::OnCancel();
}


//void CSettingsDlg::OnCbnSelchangeComboCallStatusToDb()
//{
//	// TODO: 在此加入控制項告知處理常式程式碼
//}
