// CSettingDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallerIdBox.h"
#include "CSettingDlg.h"
#include "afxdialogex.h"
#include "ADICTICallerIdBoxDlg.h"
#include "MachineClient.h"


// CSettingDlg 對話方塊

IMPLEMENT_DYNAMIC(CSettingDlg, CDialogEx)

CSettingDlg::CSettingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CSettingDlg, pParent)
{

}

CSettingDlg::~CSettingDlg()
{
}

void CSettingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDOK, m_BtnSave);
	DDX_Control(pDX, IDCANCEL, m_BtnCancel);
	DDX_Control(pDX, IDC_STATIC_MACHINE_ID, m_TitleMachineID);
	DDX_Control(pDX, IDC_COMBO_MACHINE_ID, m_EditMachineID);
	DDX_Control(pDX, IDC_STATIC_OUT_PORTS, m_TitleOutPortNum);
	DDX_Control(pDX, IDC_COMBO_OUT_PORTS, m_EditOutPortNum);
	DDX_Control(pDX, IDC_STATIC_EXT_PORTS, m_TitleExtPortNum);
	DDX_Control(pDX, IDC_COMBO_EXT_PORTS, m_EditExtPortNum);
	DDX_Control(pDX, IDC_STATIC_TITLE_SERVER_IP, m_TitleServerIPSetting);
	DDX_Control(pDX, IDC_EDIT_SERVER_IP, m_EditMachineServerIP);
	DDX_Control(pDX, IDC_SHOW_OUT_PORT, m_ShowOutPortNum);
}


BEGIN_MESSAGE_MAP(CSettingDlg, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &CSettingDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &CSettingDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CSettingDlg 訊息處理常式


BOOL CSettingDlg::OnInitDialog()
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

	CString Title = "子機伺服器網址： ";
	m_TitleServerIPSetting.SetWindowTextA(Title);
	RECT TitleServerIPSettingLRect;
	TitleServerIPSettingLRect.top = ClientRect.top + 15;
	TitleServerIPSettingLRect.left = ClientRect.left;
	TitleServerIPSettingLRect.right = TitleServerIPSettingLRect.left + m_ButtonFontWidth * Title.GetLength();
	TitleServerIPSettingLRect.bottom = TitleServerIPSettingLRect.top + m_ButtonFontHeight;
	m_TitleServerIPSetting.MoveWindow(&TitleServerIPSettingLRect);
	m_TitleServerIPSetting.SetFont(m_pButtonFont);

	CString DatabaseBackEndURL;
	DatabaseBackEndURL = AfxGetApp()->GetProfileString("SystemSetting", "MachineServerIP", "");
	m_EditMachineServerIP.SetWindowTextA(DatabaseBackEndURL);
	RECT EditServerIPRect;
	EditServerIPRect.left = TitleServerIPSettingLRect.right;
	EditServerIPRect.bottom = TitleServerIPSettingLRect.bottom;
	EditServerIPRect.top = TitleServerIPSettingLRect.top;
	EditServerIPRect.right = EditServerIPRect.left + 1500;
	m_EditMachineServerIP.MoveWindow(&EditServerIPRect);
	m_EditMachineServerIP.SetFont(m_pButtonFont);

	Title = "子機編號： ";
	m_TitleMachineID.SetWindowTextA(Title);
	RECT TitleMachineIDRect;
	TitleMachineIDRect.top = TitleServerIPSettingLRect.bottom + 15;
	TitleMachineIDRect.left = TitleServerIPSettingLRect.left;
	TitleMachineIDRect.right = TitleMachineIDRect.left + m_ButtonFontWidth * Title.GetLength();
	TitleMachineIDRect.bottom = TitleMachineIDRect.top + m_ButtonFontHeight;
	m_TitleMachineID.MoveWindow(&TitleMachineIDRect);
	m_TitleMachineID.SetFont(m_pButtonFont);

	int MachineID;
	MachineID = AfxGetApp()->GetProfileInt("SystemSetting", "MachineID", 1);
	m_EditMachineID.SetCurSel(MachineID - 1);
	RECT EditMachineIDRect;
	EditMachineIDRect.left = TitleMachineIDRect.right;
	EditMachineIDRect.top = TitleMachineIDRect.top;
	EditMachineIDRect.bottom = EditMachineIDRect.top + m_ButtonFontHeight * 12;
	EditMachineIDRect.right = EditMachineIDRect.left + 80;
	m_EditMachineID.MoveWindow(&EditMachineIDRect);
	m_EditMachineID.SetFont(m_pButtonFont);

	CHAR Value[10];
	for (int i = 1; i <= 240; i++)
	{
		sprintf_s(Value, 10, "%03d", i);
		m_EditOutPortNum.AddString(Value);
		m_EditExtPortNum.AddString(Value);
	}

	Title = "外線數量： ";
	m_TitleOutPortNum.SetWindowTextA(Title);
	RECT TitleOutPortNumRect;
	TitleOutPortNumRect.top = TitleMachineIDRect.bottom + 15;
	TitleOutPortNumRect.left = TitleMachineIDRect.left;
	TitleOutPortNumRect.right = TitleOutPortNumRect.left + m_ButtonFontWidth * Title.GetLength();
	TitleOutPortNumRect.bottom = TitleOutPortNumRect.top + m_ButtonFontHeight;
	m_TitleOutPortNum.MoveWindow(&TitleOutPortNumRect);
	m_TitleOutPortNum.SetFont(m_pButtonFont);

	//int OutPort;
	//OutPort = AfxGetApp()->GetProfileInt("SystemSetting", "OutPortNum", 16);
	RECT EditOutPortRect;
	EditOutPortRect.left = TitleOutPortNumRect.right;
	EditOutPortRect.top = TitleOutPortNumRect.top;
	EditOutPortRect.bottom = EditOutPortRect.top + m_ButtonFontHeight;
	EditOutPortRect.right = EditOutPortRect.left + 100;
	m_ShowOutPortNum.MoveWindow(&EditOutPortRect);
	m_ShowOutPortNum.SetFont(m_pButtonFont);
#if 0
	Title = "內線數量： ";
	m_TitleExtPortNum.SetWindowTextA(Title);
	RECT TitleExtPortNumRect;
	TitleExtPortNumRect.top = TitleOutPortNumRect.bottom + 15;
	TitleExtPortNumRect.left = TitleOutPortNumRect.left;
	TitleExtPortNumRect.right = TitleExtPortNumRect.left + m_ButtonFontWidth * Title.GetLength();
	TitleExtPortNumRect.bottom = TitleExtPortNumRect.top + m_ButtonFontHeight;
	m_TitleExtPortNum.MoveWindow(&TitleExtPortNumRect);
	m_TitleExtPortNum.SetFont(m_pButtonFont);

	int ExtPort;
	ExtPort = AfxGetApp()->GetProfileInt("SystemSetting", "ExtPortNum", 8);
	m_EditExtPortNum.SetCurSel(ExtPort - 1);
	RECT EditExtPortRect;
	EditExtPortRect.left = TitleExtPortNumRect.right;
	EditExtPortRect.top = TitleExtPortNumRect.top;
	EditExtPortRect.bottom = EditExtPortRect.top + m_ButtonFontHeight * 12;
	EditExtPortRect.right = EditExtPortRect.left + 100;
	m_EditExtPortNum.MoveWindow(&EditExtPortRect);
	m_EditExtPortNum.SetFont(m_pButtonFont);
#endif

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX 屬性頁應傳回 FALSE
}


void CSettingDlg::OnBnClickedCancel()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CString MachineServerIP;
	MachineServerIP = AfxGetApp()->GetProfileString("SystemSetting", "MachineServerIP", "");
	m_EditMachineServerIP.SetWindowTextA(MachineServerIP);

	int MachineID;
	MachineID = AfxGetApp()->GetProfileInt("SystemSetting", "MachineID", 1);
	m_EditMachineID.SetCurSel(MachineID - 1);

	//int OutPortNum;
	//OutPortNum = AfxGetApp()->GetProfileInt("SystemSetting", "OutPortNum", 16);
	//m_EditOutPortNum.SetCurSel(OutPortNum - 1);

	//int ExtPortNum;
	//ExtPortNum = AfxGetApp()->GetProfileInt("SystemSetting", "ExtPortNum", 8);
	//m_EditExtPortNum.SetCurSel(ExtPortNum - 1);

	return;
	CDialogEx::OnCancel();
}


void CSettingDlg::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	BOOL Changed = FALSE;

	CString MachineServerIPOld;
	MachineServerIPOld = AfxGetApp()->GetProfileString("SystemSetting", "MachineServerIP", "");
	CString MachineServerIP;
	m_EditMachineServerIP.GetWindowTextA(MachineServerIP);
	AfxGetApp()->WriteProfileString("SystemSetting", "MachineServerIP", MachineServerIP);
	if (MachineServerIPOld != MachineServerIP)
		Changed = TRUE;

	int MachineIDOld;
	MachineIDOld = AfxGetApp()->GetProfileInt("SystemSetting", "MachineID", 1);
	int MachineID;
	MachineID = m_EditMachineID.GetCurSel() + 1;
	if (MachineIDOld != MachineID)
	{
		AfxGetApp()->WriteProfileInt("SystemSetting", "MachineID", MachineID);
		Changed = TRUE;
	}

#if 0	
	int OutPortNumOld;
	OutPortNumOld = AfxGetApp()->GetProfileInt("SystemSetting", "OutPortNum", 16);
	int OutPortNum;
	OutPortNum = m_EditOutPortNum.GetCurSel() + 1;
	if (OutPortNumOld != OutPortNum)
	{
		AfxGetApp()->WriteProfileInt("SystemSetting", "OutPortNum", OutPortNum);
		Changed = TRUE;
	}

	int ExtPortNumOld;
	ExtPortNumOld = AfxGetApp()->GetProfileInt("SystemSetting", "ExtPortNum", 8);
	int ExtPortNum;
	ExtPortNum = m_EditExtPortNum.GetCurSel() + 1;
	if (ExtPortNumOld != ExtPortNum)
	{
		AfxGetApp()->WriteProfileInt("SystemSetting", "ExtPortNum", ExtPortNum);
		Changed = TRUE;
	}
#endif

	if (Changed)
	{
		// 原本這裡只叫使用者重啟程式，設定其實已經寫進登錄檔了，只是連線
		// 用的 StartMachineClient() 一直是開機時呼叫一次就不會再執行。
		// 改成存檔時直接呼叫 StartMachineClient()（內部已改成會自動先斷開
		// 舊連線）重新連線，不用再重啟整個程式。
		CADICTICallerIdBoxDlg* pCADICTICallerIdBoxDlg = (CADICTICallerIdBoxDlg*)AfxGetApp()->m_pMainWnd;
		pCADICTICallerIdBoxDlg->ShowMessage("設定已變更，正在重新連線伺服器...");
		StartMachineClient();
	}

	return;
	CDialogEx::OnOK();
}


//int CSettingDlg::ShowMessage(PCHAR pMessage)
//{
//	// TODO: 請在此新增您的實作程式碼.
//	return 0;
//}


int CSettingDlg::ShowOutPortNumber(int OutPorts)
{
	// TODO: 請在此新增您的實作程式碼.
	CHAR Buff[100];
	sprintf_s(Buff, 100, "%d", OutPorts);
	m_ShowOutPortNum.SetWindowTextA(Buff);
	AfxGetApp()->WriteProfileInt("SystemSetting", "OutPortNum", OutPorts);

	int Ret = StartMachineClient();
	return 0;
}
