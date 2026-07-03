// CPbxDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallCenter.h"
#include "CPbxDlg.h"
#include "afxdialogex.h"


#define IDC_PBX_EDIT 4500
// CPbxDlg 對話方塊
extern INT Numbers[LINES_NUM_ARRAY];
static RGB_T MachineColor[SUBPROGRAM_NUMBERS] = {
	{184, 250, 167},
	{233, 242, 179},
	{198, 208, 238},
	{225, 250, 167},
	{200, 192, 224},
	{210, 206, 175},
	{232, 255, 191},
	{245, 176, 171},
	{186, 231, 221},
	{243, 167, 250}
};

IMPLEMENT_DYNAMIC(CPbxDlg, CDialogEx)

CPbxDlg::CPbxDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CPbxDlg, pParent)
{

}

CPbxDlg::~CPbxDlg()
{
}

void CPbxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PORTS_SELECT, m_ComboListPortsSelect);
	DDX_Control(pDX, IDC_EDIT_ALIAS, m_EditMachineAlias);
}


BEGIN_MESSAGE_MAP(CPbxDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CPbxDlg::OnBnClickedOk)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(NM_CUSTOMDRAW, 1000, &CPbxDlg::OnColorSubProgramList)
	ON_NOTIFY(NM_DBLCLK, 1000, &CPbxDlg::OnEditSubProgramListItem)
	ON_NOTIFY(NM_CLICK, 1000, &CPbxDlg::OnClickSubProgramListItem)
	ON_CBN_SELCHANGE(IDC_COMBO_PORTS_SELECT, &CPbxDlg::OnCbnSelchangeComboPortsSelect)
END_MESSAGE_MAP()


// CPbxDlg 訊息處理常式


void CPbxDlg::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	return;
	CDialogEx::OnOK();
}


BOOL CPbxDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此加入額外的初始化
	memset(m_ExtPorts, 0x00, sizeof(ExtPort_T) * VIRTUAL_PORT_NUMS);
	m_MachineNums = SUBPROGRAM_NUMBERS;
	m_TitleFontHeight = 35;
	m_TitleFontWidth = m_TitleFontHeight * 0.5;
	m_pTitleFont = new CFont();
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_TitleFontHeight;
	lf.lfWidth = m_TitleFontWidth;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "微軟正黑體");
	m_pTitleFont->CreateFontIndirect(&lf);

	m_ListFontHeight = 16;
	m_ListFontWidth = m_ListFontHeight * 0.5;
	m_pListFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_ListFontHeight;
	lf.lfWidth = m_ListFontWidth;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "新細明體");
	//_tcscpy_s(lf.lfFaceName, "微軟正黑體");
	m_pListFont->CreateFontIndirect(&lf);

	m_FontHeight = 18;
	m_pFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_FontHeight;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "微軟正黑體");
	m_pFont->CreateFontIndirect(&lf);

	m_EditFontHeight = 14;
	m_EditFontWidth = m_EditFontHeight * 0.5;
	m_pEditFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_EditFontHeight;
	lf.lfWidth = m_EditFontWidth;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "Arial");
	m_pEditFont->CreateFontIndirect(&lf);
	CHAR PortNum[20];
	for (int i = 0; i <= VIRTUAL_PORT_NUMS; i++)
	{
		sprintf_s(PortNum, 20, "%03d", i);
		m_ComboListPortsSelect.AddString(PortNum);
	}
	m_ComboListPortsSelect.SetFont(m_pEditFont);

	m_SubProgramListTitle.Create("交換機類型的子機連線列表", WS_CHILD | WS_VISIBLE | SS_CENTER,
		CRect(0, 0, 0, 0), this, 2000);
	m_SubProgramListTitle.SetFont(m_pTitleFont);
	m_pParent = GetParent();

	RECT ClientRect;
	m_pParent->GetClientRect(&ClientRect);

	RECT SubProgramListTitleRect;
	SubProgramListTitleRect.top = ClientRect.top;
	SubProgramListTitleRect.right = ClientRect.right;
	SubProgramListTitleRect.bottom = SubProgramListTitleRect.top + m_TitleFontHeight;
	SubProgramListTitleRect.left = ClientRect.left;// +m_FontHeight * 4;
	m_SubProgramListTitle.MoveWindow(CRect(SubProgramListTitleRect));

	CHAR Buff[10];
	PExtPort_T pExtPort;
	PExtLineEdit_T pExtLineEdit;
	for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
	{
		pExtPort = (PExtPort_T)&m_ExtPorts[i];
		pExtLineEdit = (PExtLineEdit_T)&pExtPort->ExtLineBtn;
		if (pExtLineEdit->pExtLineNo == NULL)
		{
			pExtLineEdit->pExtLineNo = new CStatic();
			pExtLineEdit->pExtLineNo->Create(
				"", WS_CHILD | WS_VISIBLE | SS_CENTER,
				CRect(0, 0, 0, 0), this);
		}
		pExtLineEdit->DlgCtrlID = IDC_EDIT_EXTLINE1 + i;
		pExtLineEdit->pExtEditBtn = new CEdit();
		CEdit* pEdit = pExtLineEdit->pExtEditBtn;
		pEdit->Create(ES_CENTER | ES_AUTOHSCROLL | ES_NOHIDESEL | /*ES_READONLY | */ ES_NUMBER & ~WS_VISIBLE & ~WS_BORDER,
			CRect(0, 0, 0, 0), this, pExtLineEdit->DlgCtrlID);
		pEdit->ModifyStyle(i, WS_TABSTOP);
	}
	m_ListSubProgram.Create(WS_VISIBLE | WS_CHILD | WS_BORDER  /*| WS_VSCROLL */ | LVS_REPORT /* | LVS_EDITLABELS*/,
		CRect(0, 0, 0, 0), this, 1000);
	ListView_SetExtendedListViewStyle(m_ListSubProgram.m_hWnd, LVS_EX_FULLROWSELECT);
	m_ListSubProgram.InsertColumn(ID_LIST_SUB_PROGRAM_ID, "合併機碼", LVCFMT_CENTER);
	m_ListSubProgram.InsertColumn(ID_LIST_SUB_PROGRAM_ALIAS, "別名", LVCFMT_CENTER);
	m_ListSubProgram.InsertColumn(ID_LIST_SUB_PROGRAM_IP_ADDR, "IP位址", LVCFMT_CENTER);
	m_ListSubProgram.InsertColumn(ID_LIST_SUB_PROGRAM_OUT_PORTS, "外線數量", LVCFMT_CENTER);
	m_ListSubProgram.InsertColumn(ID_LIST_SUB_PROGRAM_EXT_PORTS, "內線數量", LVCFMT_CENTER);
	m_ListSubProgram.InsertColumn(ID_LIST_SUB_PROGRAM_FW_VER, "子機版號", LVCFMT_CENTER);
	m_ListSubProgram.InsertColumn(ID_LIST_SUB_PROGRAM_LINK_STATUS, "狀態", LVCFMT_CENTER);

	int ListWidth = SubProgramListTitleRect.right - SubProgramListTitleRect.left;
	int Width = (ListWidth) / (ID_LIST_SUB_PROGRAM_LINK_STATUS + 1);
	m_ListSubProgram.SetColumnWidth(ID_LIST_SUB_PROGRAM_ID, Width);
	m_ListSubProgram.SetColumnWidth(ID_LIST_SUB_PROGRAM_ALIAS, Width);
	m_ListSubProgram.SetColumnWidth(ID_LIST_SUB_PROGRAM_IP_ADDR, Width);
	m_ListSubProgram.SetColumnWidth(ID_LIST_SUB_PROGRAM_OUT_PORTS, Width);
	m_ListSubProgram.SetColumnWidth(ID_LIST_SUB_PROGRAM_EXT_PORTS, Width);
	m_ListSubProgram.SetColumnWidth(ID_LIST_SUB_PROGRAM_FW_VER, Width);
	m_ListSubProgram.SetColumnWidth(ID_LIST_SUB_PROGRAM_LINK_STATUS, Width);

	RECT SubProgramListRect;
	//m_ListHeight = (m_SubProgramNums + 1.8) * m_ListFontHeight;
	SubProgramListRect.top = SubProgramListTitleRect.top + m_TitleFontHeight;
	SubProgramListRect.right = ClientRect.right;
	SubProgramListRect.bottom = SubProgramListRect.top + m_ListFontHeight * 6;
	SubProgramListRect.left = ClientRect.left;
	m_ListSubProgram.MoveWindow(CRect(SubProgramListRect));

	/*顯示[編輯內線分機號碼]Title*/
	if (m_pEditExtLineTitle == NULL)
	{
		m_pEditExtLineTitle = new CStatic();
		m_pEditExtLineTitle->Create("編輯虛擬內線埠的分機號碼", WS_CHILD | WS_VISIBLE | SS_CENTER,
			CRect(0, 0, 0, 0), this, 2000);
	}
	m_ListSubProgram.GetWindowRect(&SubProgramListRect);
	SubProgramListRect.bottom -= m_TitleFontHeight;
	RECT EditExtLineTitleRect;
	EditExtLineTitleRect.top = SubProgramListRect.bottom;
	EditExtLineTitleRect.right = ClientRect.right;
	EditExtLineTitleRect.bottom = EditExtLineTitleRect.top + m_TitleFontHeight;
	EditExtLineTitleRect.left = ClientRect.left;
	m_pEditExtLineTitle->MoveWindow(CRect(EditExtLineTitleRect));
	m_pEditExtLineTitle->SetFont(m_pTitleFont);
	m_pEditExtLineTitle->EnableWindow(true);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX 屬性頁應傳回 FALSE
}



int CPbxDlg::ShowSubProgramList()
{
	// TODO: 請在此新增您的實作程式碼.
	m_ListSubProgram.DeleteAllItems();
	LVITEM	lvI;
	lvI.mask = LVIF_TEXT;
	CHAR Buff[100];
	CString Linked = "";
	for (int i = 0; i < m_MachineNums; i++)
	{
		lvI.iItem = i;
		lvI.iSubItem = 0;
		lvI.pszText = "1";
		lvI.cchTextMax = 60;
		PSubProgramGroup_T pSubProgramGroup = &m_PBXSubProgramGroup[i];
		m_ListSubProgram.InsertItem(&lvI);
		//m_ListSubProgram.SetFont(m_pListFont);
		sprintf_s(Buff, 100, "%d", i + 1);
		m_ListSubProgram.SetItemText(i, ID_LIST_SUB_PROGRAM_ID, Buff);
		sprintf_s(Buff, 100, "%s", pSubProgramGroup->Alias);
		m_ListSubProgram.SetItemText(i, ID_LIST_SUB_PROGRAM_ALIAS, Buff);
		if (pSubProgramGroup->LinkStatus == LINK_STATUS_LOGIN || pSubProgramGroup->LinkStatus == LINK_STATUS_LOGOUT)
			sprintf_s(Buff, pSubProgramGroup->IPAddr);
		else
			sprintf_s(Buff, "000.000.000.000");
		m_ListSubProgram.SetItemText(i, ID_LIST_SUB_PROGRAM_IP_ADDR, Buff);
		sprintf_s(Buff, 100, "%d", pSubProgramGroup->OutPortNum);
		m_ListSubProgram.SetItemText(i, ID_LIST_SUB_PROGRAM_OUT_PORTS, Buff);
		sprintf_s(Buff, 100, "%d", pSubProgramGroup->ExtPortNum);
		m_ListSubProgram.SetItemText(i, ID_LIST_SUB_PROGRAM_EXT_PORTS, Buff);
		sprintf_s(Buff, 100, "%d", pSubProgramGroup->FWVer);
		m_ListSubProgram.SetItemText(i, ID_LIST_SUB_PROGRAM_FW_VER, Buff);
		if (pSubProgramGroup->LinkStatus == LINK_STATUS_LOGIN)
			Linked = "已連線";
		else if (pSubProgramGroup->LinkStatus == LINK_STATUS_LOGOUT)
			Linked = "離線";
		else
			Linked = "未連線";
		m_ListSubProgram.SetItemText(i, ID_LIST_SUB_PROGRAM_LINK_STATUS, Linked);
	}

	m_ListSubProgram.SetFont(m_pListFont);

	return 0;
}


int CPbxDlg::SetExtLineFont()
{
	// TODO: 請在此新增您的實作程式碼.
	double Ratio;
	int Index = 0;
	int DownNumber;
	int UpNumber;
	for (int i = 0; i < LINES_NUM_ARRAY; i++)
	{
		DownNumber = Numbers[i];
		if (i == LINES_NUM_ARRAY - 1)
			UpNumber = VIRTUAL_PORT_NUMS;
		else
			UpNumber = Numbers[i + 1];
		if (m_ExtLineNum >= DownNumber && m_ExtLineNum <= UpNumber)
		{
			Index = i + 1;
			break;
		}
	}

	switch (Index)
	{
	case 0: /* 0 */
		Ratio = 1;
		m_ExtLineFontHeight = 0;
		m_EditFontHeight = 0;
		m_EditFontWidth = 0;
		m_EditListNums = 0;
		break;
	case 1: /* 8 */
		Ratio = 0.49;
		m_ExtLineFontHeightGap = 25;
		m_ExtLineFontHeight = 180;
		m_EditFontHeight = 180;
		m_EditFontWidth = 40;
		m_EditListNums = 2;
		break;
	case 2: /* 16 */
		Ratio = 0.98;
		m_ExtLineFontHeightGap = 10;
		m_ExtLineFontHeight = 90;
		m_EditFontHeight = 90;
		m_EditFontWidth = 40;
		m_EditListNums = 4;
		break;
	case 3: /* 24 */
		Ratio = 0.66;
		m_ExtLineFontHeightGap = 10;
		m_ExtLineFontHeight = 90;
		m_EditFontHeight = 90;
		m_EditFontWidth = 26;
		m_EditListNums = 5;
		break;
	case 4: /* 32 */
		Ratio = 0.492;
		m_ExtLineFontHeightGap = 8;
		m_ExtLineFontHeight = 92;
		m_EditFontHeight = 92;
		m_EditFontWidth = 20;
		m_EditListNums = 6;
		break;
	case 5: /* 64 */
		Ratio = 0.97;
		m_ExtLineFontHeightGap = 5;
		m_ExtLineFontHeight = 45;
		m_EditFontHeight = 45;
		m_EditFontWidth = 19;
		m_EditListNums = 12;
		break;
	case 6: /* 128 */
		Ratio = 0.58;
		m_ExtLineFontHeightGap = -30;
		m_ExtLineFontHeight = 40;
		m_EditFontHeight = 40;
		m_EditFontWidth = 10;
		m_EditListNums = 15;
		break;
	case 7:
		Ratio = 0.95;
		m_ExtLineFontHeightGap = 2;
		m_ExtLineFontHeight = 22;
		m_EditFontHeight = 22;
		m_EditFontWidth = 10;
		m_EditListNums = 18;
		break;
	}
	LOGFONT lf;
	m_ExtLineFontWidth = m_ExtLineFontHeight * Ratio;
	m_pOutStatustFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_ExtLineFontHeight;
	lf.lfWeight = 700;
	lf.lfWidth = m_ExtLineFontWidth;
	_tcscpy_s(lf.lfFaceName, "Arial");
	m_pOutStatustFont->CreateFontIndirect(&lf);

	m_pEditFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_EditFontHeight;
	lf.lfWidth = m_EditFontWidth;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "Arial");
	m_pEditFont->CreateFontIndirect(&lf);

	return 0;
}


int CPbxDlg::ShowExtLineIcon()
{
	// TODO: 請在此新增您的實作程式碼.
	int Index = 0;
	PExtPort_T pExtPort;
	PExtLineEdit_T pExtLineEdit;
	CEdit* pEdit = NULL;
	RECT ClientRect;
	m_pParent = GetParent();
	m_pParent->GetClientRect(&ClientRect);
	RECT EditExtLineTitleRECT;
	m_pEditExtLineTitle->GetWindowRect(&EditExtLineTitleRECT);
	EditExtLineTitleRECT.bottom -= m_TitleFontHeight;
	int DownNumber;
	int UpNumber;

	Index = 0;
	for (int i = 0; i < LINES_NUM_ARRAY; i++)
	{
		DownNumber = Numbers[i];
		if (i == LINES_NUM_ARRAY - 1)
			UpNumber = VIRTUAL_PORT_NUMS;
		else
			UpNumber = Numbers[i + 1];
		if (m_ExtLineNum >= DownNumber && m_ExtLineNum <= UpNumber)
		{
			Index = i + 1;
			break;
		}
	}
	int Width = (ClientRect.right - ClientRect.left);
	int Height = ClientRect.bottom - EditExtLineTitleRECT.bottom;
	int XGap;
	int YGap;
	CRect Rect;
	int XPos = 0;
	int	YPos = EditExtLineTitleRECT.bottom;

	int YIndex = 0;
	int XIndex = 0;
	int X;
	int Y;

	CHAR Buff[100];
	for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
	{
		pExtPort = (PExtPort_T)&m_ExtPorts[i];
		pExtLineEdit = &pExtPort->ExtLineBtn;
		if (i >= m_ExtLineNum && pExtPort->IsInUsed == FALSE)
		{
			pExtLineEdit->pExtLineNo->ShowWindow(FALSE);
			pEdit = (CEdit*)GetDlgItem(pExtLineEdit->DlgCtrlID);
			pEdit->ShowWindow(FALSE);
			continue;
		}
		if (i >= Numbers[Index])
		{
			pExtLineEdit->pExtLineNo->ShowWindow(false);
			pEdit = (CEdit*)GetDlgItem(pExtLineEdit->DlgCtrlID);
			pEdit->ShowWindow(false);
		}
		else
		{
			sprintf_s(Buff, 100, "%03d", i + 1);
			pExtLineEdit->pExtLineNo->SetWindowTextA(Buff);
			pExtLineEdit->pExtLineNo->SetFont(m_pOutStatustFont);
			//m_AssignOutPortList.SetFont(m_pEditFont);
			pEdit = (CEdit*)GetDlgItem(pExtLineEdit->DlgCtrlID);
			pEdit->SetFont(m_pEditFont);
			switch (Index)
			{
			case 0:
				X = 1; Y = 1;
				break;
			case 1:
				X = 4; Y = 2;
				break;
			case 2:
				X = 4; Y = 4;
				break;
			case 3:
				X = 6; Y = 4;
				break;
			case 4:
				X = 8; Y = 4;
				break;
			case 5:
				X = 8; Y = 8;
				break;
			case 6:
				X = 15; Y = 15;
				break;
			case 7:
				X = 16; Y = 15;
				break;
			}
			XGap = (Width / X);
			YGap = Height / Y - m_ExtLineFontHeightGap;
			YIndex = i / X;
			XIndex = i % X;
			XPos = XGap * XIndex;
			if (XIndex == 0)
			{
				XPos = 0;
				if (YIndex > 0)
					YPos += YGap;
			}
			Rect.top = YPos;
			Rect.bottom = Rect.top + m_ExtLineFontHeight;
			Rect.left = XPos;
			Rect.right = Rect.left + m_ExtLineFontWidth * 5;
			pExtLineEdit->pExtLineNo->MoveWindow(CRect(Rect));
			pExtLineEdit->pExtLineNo->ShowWindow(TRUE);

			Rect.top = Rect.bottom + 2;
			Rect.bottom = Rect.top + m_ExtLineFontHeight * 1;
			Rect.left = XPos;
			Rect.right = Rect.left + m_ExtLineFontWidth * 5;
			pExtPort->Rect = Rect;
			pEdit->MoveWindow(CRect(Rect));
			pEdit->ShowWindow(TRUE);
			//pEdit->EnableWindow(TRUE);
			if (pExtPort->ExtNo > 0)
			{
				sprintf_s(Buff, 100, "%d", pExtPort->ExtNo);
				pEdit->SetWindowTextA(Buff);
			}
		}
	}

	return 0;
}


void CPbxDlg::OnDblClkOutPortListItem()
{
	// TODO: 請在此新增您的實作程式碼.
}


void CPbxDlg::OnLbdOutPortListItem()
{
	// TODO: 請在此新增您的實作程式碼.
}


int CPbxDlg::LoadMachineData()
{
	// 原本對 3 種類型 x m_MachineNums 逐一呼叫 GetMachineInfo()，等於每次
	// 開啟這個分頁都要跑 30 次同步 HTTP 往返；改成一次 GET /machines 拿全部
	// 列回來自己分類，降成 1 次 HTTP 往返。
	m_DatabaseAccessURL.GetAllMachinesSubProgramInfo(m_PBXSubProgramGroup, m_CallerIDSubProgramGroup, m_VoiceCardSubProgramGroup);
	m_ExtLineNum = GetDBExtVPortNums();
	return 0;
}


void CPbxDlg::SetPortAssignList(int MachineID)
{
	// TODO: 請在此新增您的實作程式碼.
}


int CPbxDlg::CheckOutVPortSetting(int ThisMachineIDIndex)
{
	// TODO: 請在此新增您的實作程式碼.
	PSubProgramGroup_T pPBXSubProgramGroup;
	PSubProgramGroup_T pCallerIDBoxSubProgramGroup;
	PSubProgramGroup_T pVoiceCardSubProgramGroup;
	int PBXOutPorts = 0;
	int CallerIDBoxOutPorts = 0;
	int VoiceCardOutPorts = 0;
	BOOL bAllOutPortZero = FALSE;
	CHAR Buff[1000];
	int Len = 0;
	int Ret = 0;
	for (int i = 0; i < ThisMachineIDIndex; i++)
	{
		pPBXSubProgramGroup = &m_PBXSubProgramGroup[i];
		pCallerIDBoxSubProgramGroup = &m_CallerIDSubProgramGroup[i];
		pVoiceCardSubProgramGroup = &m_VoiceCardSubProgramGroup[i];
		PBXOutPorts = pPBXSubProgramGroup->OutPortNum;
		CallerIDBoxOutPorts = pCallerIDBoxSubProgramGroup->OutPortNum;
		VoiceCardOutPorts = pVoiceCardSubProgramGroup->OutPortNum;
		if (PBXOutPorts == 0 && CallerIDBoxOutPorts == 0 && VoiceCardOutPorts == 0)
		{
			Len += sprintf_s(Buff + Len, 1000 - Len, "\r\n合併機碼(%d)的交換機類型,來電盒類型及語音卡類型的外線設定都為0", i + 1);
			bAllOutPortZero = TRUE;
		}
	}
	if (bAllOutPortZero)
	{
		Len += sprintf_s(Buff + Len, 1000 - Len, "\r\n合併機碼的設定必須是連續的中間不能有外線是0的數量");
		MessageBoxEx(0, Buff, "警告", MB_ICONWARNING, 0);
		Ret = -1;
	}

	return 0;
}


int CPbxDlg::RedrawIcons()
{
	// TODO: 請在此新增您的實作程式碼.
	LoadMachineData();
	m_DatabaseAccessURL.GetExtLines(m_ExtPorts);
	SetExtLineFont();
	ShowExtLineIcon();
	m_ExtOutLineNum = m_ExtLineNum;

	return 0;
}


HBRUSH CPbxDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此變更 DC 的任何屬性
	int ThisCtrlID = pWnd->GetDlgCtrlID();
	PExtPort_T pExtPort;
	PExtLineEdit_T pExtLineEdit;

	int Index;

	if (ThisCtrlID == 2000)
	{
		pDC->SetTextColor(RGB(202, 245, 194));
		pDC->SetBkColor(RGB(19, 34, 168));
	}
	if (ThisCtrlID >= IDC_EDIT_EXTLINE1 && ThisCtrlID <= IDC_EDIT_EXTLINE1 + VIRTUAL_PORT_NUMS)
	{
		for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
		{
			pExtPort = &m_ExtPorts[i];
			pExtLineEdit = &pExtPort->ExtLineBtn;
			Index = pExtPort->MachineID - 1;
			if (pExtLineEdit->DlgCtrlID == ThisCtrlID)
			{
				pDC->SetTextColor(RGB(230, 15, 47));
				pDC->SetBkColor(RGB(MachineColor[Index].R, MachineColor[Index].G, MachineColor[Index].B));
			}
		}
	}

	// TODO:  如果預設值並非想要的，則傳回不同的筆刷
	return hbr;
}

void CPbxDlg::OnColorSubProgramList(NMHDR* pNMHDR, LRESULT* pResult)
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
		int Index = pLVCD->nmcd.dwItemSpec;
		crTextBrk = RGB(MachineColor[Index].R, MachineColor[Index].G, MachineColor[Index].B);
		pLVCD->clrTextBk = crTextBrk;
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT | CDDS_SUBITEM)
	{
		// This is the prepaint stage for an item. Here's where we set the
		// item's text color. Our return value will tell Windows to draw the
		// item itself, but it will use the new color we set here.
		// We'll cycle the colors through red, green, and light blue.
		COLORREF crText;
		PSubProgramGroup_T pSubProgramGroup;

		int SubProgramIdex = pLVCD->nmcd.dwItemSpec;
		pSubProgramGroup = &m_PBXSubProgramGroup[SubProgramIdex];
		crText = RGB(150, 150, 150);
		if (pSubProgramGroup->LinkStatus == LINK_STATUS_LOGIN)
			crText = RGB(0, 0, 0);

		pLVCD->clrText = crText;
		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;
	}
}

void CPbxDlg::OnEditSubProgramListItem(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pLVCD = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	RECT rect = { 0 };
	PCHAR pText = NULL;
	CHAR Buff[100];
	INT EditType = 0;
	INT Value = 1;
	m_Item = pLVCD->iItem;
	m_Subitem = pLVCD->iSubItem;

	m_ComboListPortsSelect.ShowWindow(FALSE);
	if (m_Subitem == ID_LIST_SUB_PROGRAM_OUT_PORTS || m_Subitem == ID_LIST_SUB_PROGRAM_EXT_PORTS)
	{
		// 防呆：合併機碼互斥規則——這個機碼如果已經被別的類型用掉了（外線或
		// 內線數量不是 0），這裡直接擋掉，不讓使用者打開下拉選單去改，
		// 跟後端 PATCH /machines 的驗證規則保持一致，避免選了半天才在儲存時被拒絕。
		int OccupyingType = 0;
		if (m_CallerIDSubProgramGroup[m_Item].OutPortNum > 0 || m_CallerIDSubProgramGroup[m_Item].ExtPortNum > 0)
			OccupyingType = MACHINE_TYPE_CALLER_ID_BOX;
		else if (m_VoiceCardSubProgramGroup[m_Item].OutPortNum > 0 || m_VoiceCardSubProgramGroup[m_Item].ExtPortNum > 0)
			OccupyingType = MACHINE_TYPE_VOICE_CARD;

		if (OccupyingType != 0)
		{
			CString TypeLabel = (OccupyingType == MACHINE_TYPE_CALLER_ID_BOX) ? _T("來電盒") : _T("語音卡");
			CString Msg;
			Msg.Format(_T("合併機碼 %d 目前已經被「%s類型」使用（外線或內線數量不是 0），\r\n請先把%s類型的外線、內線數量都改成 0，才能在這個機碼設定其他類型。"), m_Item + 1, TypeLabel, TypeLabel);
			MessageBox(Msg, _T("無法設定"), MB_OK | MB_ICONWARNING);
			return;
		}

		ListView_GetSubItemRect(m_ListSubProgram, m_Item, m_Subitem, LVIR_BOUNDS, &rect);
		int Hight = rect.bottom - rect.top;
		int Width = rect.right - rect.left;
		rect.top += Hight * 2;
		rect.bottom = rect.top + Hight;
		rect.left += Width / 2 - 15;
		rect.right = rect.left + 60;
		m_ComboListPortsSelect.MoveWindow(CRect(rect), 1);
		m_ComboListPortsSelect.ShowWindow(TRUE);
		m_ComboListPortsSelect.SetFocus();

		ListView_GetItemText(m_ListSubProgram, m_Item, m_Subitem, Buff, 100);
		int Index = atoi(Buff);
		m_ComboListPortsSelect.SetCurSel(Index);
		UINT flag = LVIS_SELECTED | LVIS_FOCUSED;
		m_ListSubProgram.SetItemState(m_Item, flag, flag);
	}
	else if (m_Subitem == ID_LIST_SUB_PROGRAM_ALIAS)
	{
		ListView_GetSubItemRect(m_ListSubProgram, m_Item, m_Subitem, LVIR_BOUNDS, &rect);
		int Hight = rect.bottom - rect.top;
		int Width = rect.right - rect.left;
		rect.top += Hight * 2;
		rect.bottom = rect.top + Hight;
		rect.left += Width / 2 - 100;
		rect.right = rect.left + 200;

		m_EditMachineAlias.SetWindowTextA(m_PBXSubProgramGroup[m_Item].Alias);
		m_EditMachineAlias.MoveWindow(CRect(rect), 1);
		m_EditMachineAlias.ShowWindow(TRUE);
		m_EditMachineAlias.SetFocus();
		UINT flag = LVIS_SELECTED | LVIS_FOCUSED;
		m_ListSubProgram.SetItemState(m_Item, flag, flag);
	}
}

void CPbxDlg::OnClickSubProgramListItem(NMHDR* pNMHDR, LRESULT* pResult)
{
	m_ComboListPortsSelect.ShowWindow(FALSE);
}


void CPbxDlg::OnCbnSelchangeComboPortsSelect()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	m_ComboListPortsSelect.ShowWindow(FALSE);
	int Ports;
	Ports = m_ComboListPortsSelect.GetCurSel();
	int SubProgramID = m_Item + 1;
	int Ret;
	int TypeBOutPorts;

	switch (m_Subitem)
	{
	case ID_LIST_SUB_PROGRAM_OUT_PORTS:
		TypeBOutPorts = m_CallerIDSubProgramGroup[m_Item].OutPortNum;
		if (TypeBOutPorts > Ports && Ports != 0)
		{
			CHAR Buff[200];
			int Len;
			Len = sprintf_s(Buff, 200, "來電盒類型的外線數量已設定為%d\r\n", TypeBOutPorts);
			Len += sprintf_s(Buff + Len, 200 - Len, "所以交換機類型的外線數量不能少於來電盒類型的外線數量\r\n");
			Len += sprintf_s(Buff + Len, 200 - Len, "請先調整來電盒類型的外線數量");

			MessageBoxEx(0, Buff, "錯誤", MB_ICONERROR, 0);
			m_ComboListPortsSelect.ShowWindow(FALSE);
		}
		else
		{
			Ret = 0;
			if (Ports > 0)
			{
				Ret = CheckOutVPortSetting(m_Item);
			}
			if (Ret == 0)
			{
				CHAR ErrMsg[200] = { 0 };
				int FailedPortCount = 0;
				Ret = m_DatabaseAccessURL.SetMachineOutPortsByMachineID(MACHINE_TYPE_PBX, SubProgramID, Ports, ErrMsg, &FailedPortCount);
				if (Ret == 0)
				{
					/*設定外線資料庫*/
					m_PBXSubProgramGroup[m_Item].OutPortNum = Ports;
					ShowSubProgramList();
					if (FailedPortCount > 0)
					{
						// 這台部署的資料庫只能搬動既有佈線的實體埠，沒辦法
						// 憑空新增——超過既有佈線涵蓋範圍的埠，指派會失敗但
						// 畫面不會有任何提示，這裡明確告知管理員。
						CString Msg;
						Msg.Format(_T("有 %d 個實體外線埠指派失敗：這幾個實體埠在資料庫裡沒有既有佈線資料，無法自動指派虛擬線路，請聯絡系統管理員確認資料庫佈線。"), FailedPortCount);
						MessageBox(Msg, _T("部分外線埠指派失敗"), MB_OK | MB_ICONWARNING);
					}
				}
				else
				{
					CString Msg;
					if (ErrMsg[0])
						Msg = ErrMsg;
					else
						Msg = _T("設定外線數量失敗，請確認資料庫連線或聯絡系統管理員。");
					MessageBox(Msg, _T("設定失敗"), MB_OK | MB_ICONWARNING);
					ShowSubProgramList();
				}
			}
		}
		break;
	case ID_LIST_SUB_PROGRAM_EXT_PORTS:
	{
		CHAR ExtErrMsg[200] = { 0 };
		int FailedExtPortCount = 0;
		Ret = m_DatabaseAccessURL.SetMachineExtPortsByMachineID(MACHINE_TYPE_PBX, SubProgramID, Ports, ExtErrMsg, &FailedExtPortCount);
		if (Ret == 0)
		{
			m_PBXSubProgramGroup[m_Item].ExtPortNum = Ports;
			LoadMachineData();
			m_DatabaseAccessURL.GetExtLines(m_ExtPorts);
			ShowSubProgramList();
			SetExtLineFont();
			ShowExtLineIcon();
			Invalidate(FALSE);
			if (FailedExtPortCount > 0)
			{
				CString Msg;
				Msg.Format(_T("有 %d 個實體內線埠指派失敗：這幾個實體埠在資料庫裡沒有既有佈線資料，無法自動指派虛擬線路，請聯絡系統管理員確認資料庫佈線。"), FailedExtPortCount);
				MessageBox(Msg, _T("部分內線埠指派失敗"), MB_OK | MB_ICONWARNING);
			}
		}
		else
		{
			CString Msg;
			if (ExtErrMsg[0])
				Msg = ExtErrMsg;
			else
				Msg = _T("設定內線數量失敗，請確認資料庫連線或聯絡系統管理員。");
			MessageBox(Msg, _T("設定失敗"), MB_OK | MB_ICONWARNING);
			ShowSubProgramList();
		}
		break;
	}
	}
}


BOOL CPbxDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
	CWnd* pThisWnd;
	CWnd* pEditWnd;
	CString Value;
	CHAR Buff[20];
	BOOL Changed = FALSE;
	int MachineID;
	int ExtVPort;

	if (pMsg->message == WM_KEYDOWN &&
		(pMsg->wParam == VK_ESCAPE)
		)
	{
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN &&
		(pMsg->wParam == VK_RETURN || pMsg->wParam == VK_TAB)
		)
	{
		// handle return pressed in edit control
		pThisWnd = GetFocus();
		pEditWnd = (CEdit*)GetDlgItem(IDC_EDIT_ALIAS);
		if (pThisWnd == pEditWnd)
		{
			int Len;
			CString Alias;
			CHAR ThisAlias[20];
			pEditWnd->GetWindowTextA(Alias);
			Len = Alias.GetLength();
			if (Len >= 20)
				Len = 19;
			strncpy_s(ThisAlias, 20, Alias.GetBuffer(), Len);
			PSubProgramGroup_T pSubProgramGroup = &m_PBXSubProgramGroup[m_Item];
			if (strcmp(pSubProgramGroup->Alias, ThisAlias) != 0)
			{
				memcpy(pSubProgramGroup->Alias, ThisAlias, 20);
				if (m_DatabaseAccessURL.SetMachineAliasByMachineID(MACHINE_TYPE_PBX, m_Item + 1, pSubProgramGroup->Alias, NULL) < 0)
				{
					pEditWnd->SetWindowTextA(pSubProgramGroup->Alias);
				}
				pEditWnd->ShowWindow(FALSE);
				ShowSubProgramList();
			}
		}
		else
		{
			for (int i = 0; i < m_ExtLineNum; i++)
			{
				PExtLineEdit_T pExtLineEdit;
				pExtLineEdit = &m_ExtPorts[i].ExtLineBtn;
				MachineID = m_ExtPorts[i].MachineID;
				pEditWnd = (CEdit*)GetDlgItem(pExtLineEdit->DlgCtrlID);
				if (pEditWnd == pThisWnd)
				{
					pEditWnd->GetWindowTextA(Value);
					sprintf_s(Buff, 20, Value.GetBuffer());
					int Extension = atoi(Buff);

					if (m_ExtPorts[i].ExtNo != Extension)
					{
						Changed = TRUE;
						ExtVPort = i + 1;
						if (m_DatabaseAccessURL.SetExtNumber(ExtVPort, MachineID, Extension, NULL) < 0)
						{
							if(m_ExtPorts[i].ExtNo == 0)
								sprintf_s(Buff, 20, "");
							else
								sprintf_s(Buff, 20, "%d", m_ExtPorts[i].ExtNo);
							pEditWnd->SetWindowTextA(Buff);
						}
						else
						{
							m_ExtPorts[i].ExtNo = Extension;
							SetExtLineFont();
							ShowExtLineIcon();
							break;
						}
					}
				}
			}
		}

		//return TRUE; // this doesn't need processing anymore
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CPbxDlg::MachineLogin(int MachineID, PCHAR pIPAddr, int SWVer)
{
	int Index = MachineID - 1;
	CHAR Ver[10];

	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_IP_ADDR, pIPAddr);
	sprintf_s(Ver, 10, "%d", SWVer);
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_FW_VER, Ver);
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_LINK_STATUS, "已連線");
	m_DatabaseAccessURL.SetMachineConnected(MACHINE_TYPE_PBX, MachineID, pIPAddr, TRUE, NULL);
}

void CPbxDlg::MachineLogout(int MachineID)
{
	int Index = MachineID - 1;
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_IP_ADDR, "000.000.000.000");
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_FW_VER, "0");
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_LINK_STATUS, "離線");
	m_DatabaseAccessURL.SetMachineConnected(MACHINE_TYPE_PBX, MachineID, NULL, FALSE, NULL);
}

int CPbxDlg::InitPorts()
{
	// TODO: 請在此新增您的實作程式碼.
	LoadMachineData();
	m_DatabaseAccessURL.GetExtLines(m_ExtPorts);
	ShowSubProgramList();
	SetExtLineFont();
	ShowExtLineIcon();
	return 0;
}
