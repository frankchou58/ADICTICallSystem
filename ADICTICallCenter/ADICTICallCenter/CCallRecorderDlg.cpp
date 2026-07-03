// CCallRecorderDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallCenter.h"
#include "CCallRecorderDlg.h"
#include "afxdialogex.h"

#define IDC_CALLERID_BOX_CARD 5000
// CCallRecorderDlg 對話方塊
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

IMPLEMENT_DYNAMIC(CCallRecorderDlg, CDialogEx)

CCallRecorderDlg::CCallRecorderDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CCallRecorderDlg, pParent)
{
}

CCallRecorderDlg::~CCallRecorderDlg()
{
}

void CCallRecorderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//  DDX_Text(pDX, IDC_EDIT2, m_test);
	//  D//  DV_MaxChars(p//  DX, m_test, 10);
	//DDX_Control(pDX, IDC_LIST1, m_ListSubprogram);
	//DDX_Control(pDX, IDC_LIST_SUBPROGRAM, m_ListSubProgram);
	DDX_Control(pDX, IDC_COMBO_PORTS_SELECT, m_ComboListPortsSelect);
	DDX_Control(pDX, IDC_EDIT_ALIAS, m_EditMachineAlias);
	DDX_Control(pDX, IDC_COMBO_POTPORTS_ASSIGN, m_AssignOutPortList);
}


BEGIN_MESSAGE_MAP(CCallRecorderDlg, CDialogEx)
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDOK, &CCallRecorderDlg::OnBnClickedOk)
	ON_NOTIFY(NM_CUSTOMDRAW, 1000, &CCallRecorderDlg::OnColorSubProgramList)
	ON_NOTIFY(NM_DBLCLK, 1000, &CCallRecorderDlg::OnEditSubProgramListItem)
	ON_NOTIFY(NM_CLICK, 1000, &CCallRecorderDlg::OnClickSubProgramListItem)
	ON_CBN_SELCHANGE(IDC_COMBO_PORTS_SELECT, &CCallRecorderDlg::OnCbnSelchangeComboPortsSelect)
	ON_CBN_SELCHANGE(IDC_COMBO_POTPORTS_ASSIGN, &CCallRecorderDlg::OnCbnSelchangeComboPotportsAssign)
END_MESSAGE_MAP()


// CCallRecorderDlg 訊息處理常式


BOOL CCallRecorderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此加入額外的初始化
	memset(m_OutPorts, 0x00, sizeof(OutPort_T) * VIRTUAL_PORT_NUMS);
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

	m_SubProgramListTitle.Create("來電盒類型的子機連線列表", WS_CHILD | WS_VISIBLE | SS_CENTER,
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
	POutPort_T pOutPort;
	POutLineEdit_T pOutLineEdit;
	for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
	{
		pOutPort = (POutPort_T)&m_OutPorts[i];
		pOutLineEdit = (POutLineEdit_T)&pOutPort->OutLineBtn;
		if (pOutLineEdit->pOutLineNo == NULL)
		{
			pOutLineEdit->pOutLineNo = new CStatic();
			pOutLineEdit->pOutLineNo->Create(
				"", WS_CHILD | WS_VISIBLE | SS_CENTER,
				CRect(0, 0, 0, 0), this);
		}
		pOutLineEdit->DlgCtrlID = IDC_CALLERID_BOX_CARD + i;
		pOutLineEdit->pOutEditBtn = new CEdit();
		CEdit* pEdit = pOutLineEdit->pOutEditBtn;
		pEdit->Create(ES_CENTER | ES_AUTOHSCROLL | ES_NOHIDESEL | ES_READONLY | ES_NUMBER & ~WS_VISIBLE & ~WS_BORDER,
			CRect(0, 0, 0, 0), this, pOutLineEdit->DlgCtrlID);
	}
	m_ListSubProgram.Create(WS_VISIBLE | WS_CHILD | WS_BORDER /* | WS_VSCROLL */ | LVS_REPORT /* | LVS_EDITLABELS*/,
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

	/*顯示[分配來電盒Port到PBX的外線Port]Title*/
	if (m_pEditOutLineTitle == NULL)
	{
		m_pEditOutLineTitle = new CStatic();
		m_pEditOutLineTitle->Create("分配來電盒的實體埠到虛擬外線埠", WS_CHILD | WS_VISIBLE | SS_CENTER,
			CRect(0, 0, 0, 0), this, 2000);
	}
	m_ListSubProgram.GetWindowRect(&SubProgramListRect);
	SubProgramListRect.bottom -= m_TitleFontHeight;
	RECT EditExtLineTitleRect;
	EditExtLineTitleRect.top = SubProgramListRect.bottom;
	EditExtLineTitleRect.right = ClientRect.right;
	EditExtLineTitleRect.bottom = EditExtLineTitleRect.top + m_TitleFontHeight;
	EditExtLineTitleRect.left = ClientRect.left;
	m_pEditOutLineTitle->MoveWindow(CRect(EditExtLineTitleRect));
	m_pEditOutLineTitle->SetFont(m_pTitleFont);
	m_pEditOutLineTitle->EnableWindow(true);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX 屬性頁應傳回 FALSE
}

int CCallRecorderDlg::ShowSubProgramList()
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
		PSubProgramGroup_T pSubProgramGroup = &m_CallerIDSubProgramGroup[i];
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


#if 0
void CCallRecorderDlg::OnEnChangeEdit2()
{
	// TODO:  如果這是 RICHEDIT 控制項，控制項將不會
	// 傳送此告知，除非您覆寫 CDialogEx::OnInitDialog()
	// 函式和呼叫 CRichEditCtrl().SetEventMask()
	// 讓具有 ENM_CHANGE 旗標 ORed 加入遮罩。

	// TODO:  在此加入控制項告知處理常式程式碼
	printf("dwedwedwe");
}
#endif

//BOOL CCallRecorderDlg::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
//{
//	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
//#if 0
//	CWnd* pWnd = GetFocus();
//	int CtrlID = pWnd->GetDlgCtrlID();
//	//if (message == WM_CTLCOLORDLG && (CtrlID < IDC_CALLERID_BOX_CARD || CtrlID > IDC_CALLERID_BOX_CARD + VIRTUAL_PORT_NUMS) && CtrlID != 1001)
//	if (message == WM_CTLCOLORDLG)
//	{
//		if (CtrlID == 0x01)
//		{
//			if (CtrlID < IDC_CALLERID_BOX_CARD || CtrlID > IDC_CALLERID_BOX_CARD + VIRTUAL_PORT_NUMS)
//			{
//				LoadMachineData();
//				if (m_OutLineNum != m_OldOutLineNum)
//				{
//					m_OldOutLineNum = m_OutLineNum;
//					m_DatabaseAccessURL.GetOutLines(m_OutPorts);
//					SetOutLineFont();
//					ShowOutLineIcon();
//					Invalidate(false);
//				}
//			}
//		}
//	}
//#endif
//	return CDialogEx::OnChildNotify(message, wParam, lParam, pLResult);
//}


void CCallRecorderDlg::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	return;
	CDialogEx::OnOK();
}

void CCallRecorderDlg::OnEditSubProgramListItem(NMHDR* pNMHDR, LRESULT* pResult)
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
	if (m_Subitem == ID_LIST_SUB_PROGRAM_OUT_PORTS)
	{
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
		m_IsInEditSelectPort = TRUE;
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

		m_EditMachineAlias.SetWindowTextA(m_CallerIDSubProgramGroup[m_Item].Alias);
		m_EditMachineAlias.MoveWindow(CRect(rect), 1);
		m_EditMachineAlias.ShowWindow(TRUE);
		m_EditMachineAlias.SetFocus();
		UINT flag = LVIS_SELECTED | LVIS_FOCUSED;
		m_ListSubProgram.SetItemState(m_Item, flag, flag);
	}
}

void CCallRecorderDlg::OnClickSubProgramListItem(NMHDR* pNMHDR, LRESULT* pResult)
{
	m_ComboListPortsSelect.ShowWindow(FALSE);
}


HBRUSH CCallRecorderDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此變更 DC 的任何屬性
	int ThisCtrlID = pWnd->GetDlgCtrlID();
	POutPort_T pOutPort;
	POutLineEdit_T pOutLineEdit;

	int Index;

	if (ThisCtrlID == 2000)
	{
		pDC->SetTextColor(RGB(202, 245, 194));
		pDC->SetBkColor(RGB(19, 34, 168));
	}
	if (ThisCtrlID >= IDC_CALLERID_BOX_CARD && ThisCtrlID <= IDC_CALLERID_BOX_CARD + VIRTUAL_PORT_NUMS)
	{
		for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
		{
			pOutPort = &m_OutPorts[i];
			pOutLineEdit = &pOutPort->OutLineBtn;
			Index = pOutPort->MachineID - 1;
			if (pOutLineEdit->DlgCtrlID == ThisCtrlID)
			{
				pDC->SetTextColor(RGB(230, 15, 47));
				pDC->SetBkColor(RGB(MachineColor[Index].R, MachineColor[Index].G, MachineColor[Index].B));
			}
		}
	}

	// TODO:  如果預設值並非想要的，則傳回不同的筆刷
	return hbr;
}

void CCallRecorderDlg::OnColorSubProgramList(NMHDR* pNMHDR, LRESULT* pResult)
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
		pSubProgramGroup = &m_CallerIDSubProgramGroup[SubProgramIdex];
		crText = RGB(150, 150, 150);
		if (pSubProgramGroup->LinkStatus == LINK_STATUS_LOGIN)
			crText = RGB(0, 0, 0);

		pLVCD->clrText = crText;
		// Tell Windows to paint the control itself.
		*pResult = CDRF_DODEFAULT;
	}
}



BOOL CCallRecorderDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 在此加入特定的程式碼和 (或) 呼叫基底類別
	CWnd* pThisWnd;
	CWnd* pEditWnd;
	CString Value;
	CHAR Buff[20];
	BOOL Changed = FALSE;
	int SubProgramID;
	int ExtVPort;

	if (pMsg->message == WM_KEYDOWN &&
		(pMsg->wParam == VK_ESCAPE)
		)
	{
		return TRUE;
	}

	if (pMsg->message == WM_LBUTTONDBLCLK)
	{
		/*雙擊左鍵*/
		OnDblClkOutPortListItem();
	}
	else if (pMsg->message == WM_LBUTTONDOWN)
	{
		/*單擊左鍵按*/
		OnLbdOutPortListItem();
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
			PSubProgramGroup_T pSubProgramGroup = &m_CallerIDSubProgramGroup[m_Item];
			if (strcmp(pSubProgramGroup->Alias, ThisAlias) != 0)
			{
				if (m_DatabaseAccessURL.SetMachineAliasByMachineID(MACHINE_TYPE_CALLER_ID_BOX, m_Item + 1, ThisAlias, NULL) < 0)
				{
					pEditWnd->SetWindowTextA(pSubProgramGroup->Alias);
				}
				else
					memcpy(pSubProgramGroup->Alias, ThisAlias, 20);

				pEditWnd->ShowWindow(FALSE);
				ShowSubProgramList();
			}
		}
		else
		{
		}
		//return TRUE; // this doesn't need processing anymore
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

int CCallRecorderDlg::SetOutLineFont()
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
		if (m_OutLineNum >= DownNumber && m_OutLineNum <= UpNumber)
		{
			Index = i + 1;
			break;
		}
	}

	switch (Index)
	{
	case 0: /* 0 */
		Ratio = 1;
		m_OutLineFontHeight = 0;
		m_EditFontHeight = 0;
		m_EditFontWidth = 0;
		m_EditListNums = 0;
		break;
	case 1: /* 8 */
		Ratio = 0.49;
		m_OutLineFontHeightGap = 25;
		m_OutLineFontHeight = 190;
		m_EditFontHeight = 150;
		m_EditFontWidth = 40;
		m_EditListNums = 2;
		break;
	case 2: /* 16 */
		Ratio = 0.98;
		m_OutLineFontHeightGap = 10;
		m_OutLineFontHeight = 95;
		m_EditFontHeight = 90;
		m_EditFontWidth = 40;
		m_EditListNums = 4;
		break;
	case 3: /* 24 */
		Ratio = 0.66;
		m_OutLineFontHeightGap = 10;
		m_OutLineFontHeight = 95;
		m_EditFontHeight = 90;
		m_EditFontWidth = 26;
		m_EditListNums = 5;
		break;
	case 4: /* 32 */
		Ratio = 0.492;
		m_OutLineFontHeightGap = 10;
		m_OutLineFontHeight = 95;
		m_EditFontHeight = 70;
		m_EditFontWidth = 20;
		m_EditListNums = 6;
		break;
	case 5: /* 64 */
		Ratio = 0.97;
		m_OutLineFontHeightGap = 5;
		m_OutLineFontHeight = 48;
		m_EditFontHeight = 40;
		m_EditFontWidth = 19;
		m_EditListNums = 12;
		break;
	case 6: /* 128 */
		Ratio = 0.58;
		m_OutLineFontHeightGap = -32;
		m_OutLineFontHeight = 42;
		m_EditFontHeight = 35;
		m_EditFontWidth = 10;
		m_EditListNums = 15;
		break;
	case 7:
		Ratio = 0.95;
		m_OutLineFontHeightGap = 2;
		m_OutLineFontHeight = 25;
		m_EditFontHeight = 20;
		m_EditFontWidth = 10;
		m_EditListNums = 18;
		break;
	}
	LOGFONT lf;
	m_OutLineFontWidth = m_OutLineFontHeight * Ratio;
	m_pOutStatustFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_OutLineFontHeight;
	lf.lfWeight = 700;
	lf.lfWidth = m_OutLineFontWidth;
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


int CCallRecorderDlg::ShowOutLineIcon()
{
	// TODO: 請在此新增您的實作程式碼.
	int Index = 0;
	POutPort_T pOutPort;
	POutLineEdit_T pOutLineEdit;
	CEdit* pEdit = NULL;
	RECT ClientRect;
	m_pParent = GetParent();
	m_pParent->GetClientRect(&ClientRect);
	RECT EditOutLineTitleRECT;
	m_pEditOutLineTitle->GetWindowRect(&EditOutLineTitleRECT);
	EditOutLineTitleRECT.bottom -= m_TitleFontHeight;
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
		if (m_OutLineNum >= DownNumber && m_OutLineNum <= UpNumber)
		{
			Index = i + 1;
			break;
		}
	}
	int Width = (ClientRect.right - ClientRect.left);
	int Height = ClientRect.bottom - EditOutLineTitleRECT.bottom;
	int XGap;
	int YGap;
	CRect Rect;
	int XPos = 0;
	int	YPos = EditOutLineTitleRECT.bottom;

	int YIndex = 0;
	int XIndex = 0;
	int X;
	int Y;

	CHAR Buff[100];
	for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
	{
		pOutPort = (POutPort_T)&m_OutPorts[i];
		pOutLineEdit = &pOutPort->OutLineBtn;
		if (i >= m_OutLineNum && pOutPort->IsInUsed == FALSE)
		{
			pOutLineEdit->pOutLineNo->ShowWindow(FALSE);
			pEdit = (CEdit*)GetDlgItem(pOutLineEdit->DlgCtrlID);
			pEdit->ShowWindow(FALSE);
			continue;
		}
		if (i >= Numbers[Index])
		{
			pOutLineEdit->pOutLineNo->ShowWindow(false);
			pEdit = (CEdit*)GetDlgItem(pOutLineEdit->DlgCtrlID);
			pEdit->ShowWindow(false);
		}
		else
		{
			sprintf_s(Buff, 100, "%03d", i + 1);
			pOutLineEdit->pOutLineNo->SetWindowTextA(Buff);
			pOutLineEdit->pOutLineNo->SetFont(m_pOutStatustFont);
			m_AssignOutPortList.SetFont(m_pEditFont);
			pEdit = (CEdit*)GetDlgItem(pOutLineEdit->DlgCtrlID);
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
			YGap = Height / Y - m_OutLineFontHeightGap;
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
			Rect.bottom = Rect.top + m_OutLineFontHeight;
			Rect.left = XPos;
			Rect.right = Rect.left + m_OutLineFontWidth * 5;
			pOutLineEdit->pOutLineNo->MoveWindow(CRect(Rect));
			pOutLineEdit->pOutLineNo->ShowWindow(TRUE);


			int MachineID = pOutPort->MachineID;
			int ThisOutPortNum = m_CallerIDSubProgramGroup[MachineID - 1].OutPortNum;
			Rect.top = Rect.bottom + 2;
			Rect.bottom = Rect.top + m_OutLineFontHeight * 1;
			Rect.left = XPos;
			Rect.right = Rect.left + m_OutLineFontWidth * 5;
			pOutPort->Rect = Rect;
			pEdit->MoveWindow(CRect(Rect));
			pEdit->ShowWindow(TRUE);
			//pEdit->EnableWindow(TRUE);
			if (ThisOutPortNum > 0)
			{
				if (pOutPort->MachineTypeBPhyPort > 0)
					sprintf_s(Buff, 100, "%03d", pOutPort->MachineTypeBPhyPort);
				else if (pOutPort->MachineTypeBPhyPort == 0)
					sprintf_s(Buff, 100, "");
				pEdit->SetWindowTextA(Buff);
			}
			else if(ThisOutPortNum == 0)
			{
				pEdit->SetWindowTextA("X");
			}
		}
	}

	return 0;
}

#if 0
int CCallRecorderDlg::CalOutPortNums()
{
	// TODO: 請在此新增您的實作程式碼.
	int PBXOutPortNum;
	int ThisOutPortNum;
	m_OutLineNum = 0;
	for (int i = 0; i < m_MachineNums; i++)
	{
		PBXOutPortNum = m_PBXSubProgramGroup[i].OutPortNum;
		m_OutLineNum += PBXOutPortNum;

		if (PBXOutPortNum == 0)
		{
			ThisOutPortNum = m_CallerIDSubProgramGroup[i].OutPortNum;
			m_OutLineNum += ThisOutPortNum;
		}
	}
	if (m_OutLineNum > VIRTUAL_PORT_NUMS)
		m_OutLineNum = VIRTUAL_PORT_NUMS;

	return 0;
}
#endif

void CCallRecorderDlg::OnCbnSelchangeComboOutLine1()
{
	// TODO: 在此加入控制項告知處理常式程式碼
}


void CCallRecorderDlg::OnDblClkOutPortListItem()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CWnd* pWnd = GetFocus();
	int CtrlID = pWnd->GetDlgCtrlID();

	if (CtrlID >= IDC_CALLERID_BOX_CARD && CtrlID <= IDC_CALLERID_BOX_CARD + VIRTUAL_PORT_NUMS)
	{
		m_Item = CtrlID - IDC_CALLERID_BOX_CARD;
		int MachineID = m_OutPorts[m_Item].MachineID;
		int PBXOutPorts = m_PBXSubProgramGroup[MachineID - 1].OutPortNum;
		int ThisOutPorts = m_CallerIDSubProgramGroup[MachineID - 1].OutPortNum;
		if (m_PrevCtrlID != CtrlID)
		{
			if (m_PrevCtrlID >= IDC_CALLERID_BOX_CARD && m_PrevCtrlID <= IDC_CALLERID_BOX_CARD + VIRTUAL_PORT_NUMS)
			{
				CEdit* pPrevEdit = (CEdit*)GetDlgItem(m_PrevCtrlID);
				pPrevEdit->ShowWindow(TRUE);
			}
//			if (PBXOutPorts != 0 && PBXOutPorts > ThisOutPorts)
			if(ThisOutPorts > 0)
			{
				/*交換機模式的外線不是0且*/
				CEdit* pThisEdit = (CEdit*)GetDlgItem(m_OutPorts[m_Item].OutLineBtn.DlgCtrlID);
				pThisEdit->ShowWindow(FALSE);
				RECT Rect;
				Rect = m_OutPorts[m_Item].Rect;
				Rect.bottom += m_EditFontHeight * m_EditListNums;
				SetPortAssignList(MachineID);
				m_AssignOutPortList.MoveWindow(CRect(Rect));
				m_AssignOutPortList.SetFocus();
				m_AssignOutPortList.ShowWindow(TRUE);
				m_IsInEdit = TRUE;
			}
			m_PrevCtrlID = CtrlID;
		}
		else if (m_PrevCtrlID == CtrlID)
		{
//			if (PBXOutPorts != 0 && PBXOutPorts > ThisOutPorts)
			if(ThisOutPorts > 0)
			{
				RECT Rect;
				CEdit* pThisEdit = (CEdit*)GetDlgItem(m_OutPorts[m_Item].OutLineBtn.DlgCtrlID);
				pThisEdit->ShowWindow(FALSE);
				Rect = m_OutPorts[m_Item].Rect;
				Rect.bottom += m_EditFontHeight * m_EditListNums;
				m_AssignOutPortList.MoveWindow(CRect(Rect));
				m_AssignOutPortList.SetFocus();
				m_AssignOutPortList.ShowWindow(TRUE);
				m_IsInEdit = TRUE;
			}
		}
	}
}

void CCallRecorderDlg::OnLbdOutPortListItem()
{
	CWnd* pWnd = GetFocus();
	int CtrlID = pWnd->GetDlgCtrlID();

	m_EditMachineAlias.ShowWindow(FALSE);
	if (m_IsInEdit == FALSE)
	{
		if (m_PrevCtrlID >= IDC_CALLERID_BOX_CARD && m_PrevCtrlID <= IDC_CALLERID_BOX_CARD + VIRTUAL_PORT_NUMS)
		{
			CEdit* pThisEdit = (CEdit*)GetDlgItem(m_PrevCtrlID);
			pThisEdit->ShowWindow(TRUE);
			if (m_PrevCtrlID != CtrlID)
				m_AssignOutPortList.ShowWindow(FALSE);
		}
		m_AssignOutPortList.ShowWindow(FALSE);
	}
	else if (CtrlID != m_PrevCtrlID)
	{
		if (m_PrevCtrlID >= IDC_CALLERID_BOX_CARD && m_PrevCtrlID <= IDC_CALLERID_BOX_CARD + VIRTUAL_PORT_NUMS)
		{
			CEdit* pThisEdit = (CEdit*)GetDlgItem(m_PrevCtrlID);
			pThisEdit->ShowWindow(TRUE);
			m_AssignOutPortList.ShowWindow(FALSE);
		}
		m_AssignOutPortList.ShowWindow(FALSE);
	}
}

//void CCallRecorderDlg::OnMouseMove(UINT nFlags, CPoint point)
//{
//	// TODO: 在此加入您的訊息處理常式程式碼和 (或) 呼叫預設值
//	//memcpy(&m_MousePos, &point, sizeof(CPoint));
//	m_MousePos = point;
//	CDialogEx::OnMouseMove(nFlags, point);
//}

void CCallRecorderDlg::OnCbnSelchangeComboPortsSelect()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	//printf("dwedwed");
	m_ComboListPortsSelect.ShowWindow(FALSE);
	int Ports;
	Ports = m_ComboListPortsSelect.GetCurSel();
	int SubProgramID = m_Item + 1;
	int Ret;
	CHAR Buff[200];
	int Len;

	switch (m_Subitem)
	{
	case ID_LIST_SUB_PROGRAM_OUT_PORTS:
		int PBXOutPortNum = m_PBXSubProgramGroup[m_Item].OutPortNum;
		int CallerIDBoxOutPortNum = m_CallerIDSubProgramGroup[m_Item].OutPortNum;
		int VoiceCardOutPortNum = m_VoiceCardSubProgramGroup[m_Item].OutPortNum;
		if (PBXOutPortNum < Ports && PBXOutPortNum > 0)
		{
			Len = sprintf_s(Buff, 200, "交換機類型的外線數量已設定為%d\r\n", PBXOutPortNum);
			Len += sprintf_s(Buff + Len, 200 - Len, "所以來電盒類型的外線數量不能超過交換機類型的外線數量\r\n");
			Len += sprintf_s(Buff + Len, 200 - Len, "請先調整交換機類型的外線數量");

			MessageBoxEx(0, Buff, "錯誤", MB_ICONERROR, 0);
		}
		else if (VoiceCardOutPortNum != 0)
		{
			Len = sprintf_s(Buff, 200, "語音卡類型的外線數量已設定為%d\r\n", VoiceCardOutPortNum);
			Len += sprintf_s(Buff + Len, 200 - Len, "如欲設定來電盒的外線數量,請先將語音卡類型的外線數量設成0");

			MessageBoxEx(0, Buff, "錯誤", MB_ICONERROR, 0);
		}
		else
		{
			Ret = 0;
			Len = 0;
			if (PBXOutPortNum == 0 && CallerIDBoxOutPortNum != Ports)
			{
				Len += sprintf_s(Buff + Len, 200 - Len, "確定更改成此數量[%d],請按下[確定]按鈕\r\n", Ports);
				Len += sprintf_s(Buff + Len, 200 - Len, "但更改後的實際外線與虛擬外線的對應將會變動\r\n");
				Ret = MessageBoxEx(0, Buff, "警告", MB_ICONWARNING | MB_OKCANCEL, NULL);
				if (Ret == IDOK)
					Ret = 0;
			}
			
			if (Ret == 0)
			{
				if (Ports > 0)
				{
					Ret = CheckOutVPortSetting(m_Item);
				}
				if (Ret == 0)
				{
					Ret = m_DatabaseAccessURL.SetMachineOutPortsByMachineID(MACHINE_TYPE_CALLER_ID_BOX, SubProgramID, Ports, NULL);
					if (Ret == 0)
					{
						m_CallerIDSubProgramGroup[m_Item].OutPortNum = Ports;
						ShowSubProgramList();
					}
				}
				m_PrevCtrlID = 0;
				int FailedPortCount = SetDBOutVPortSetting(SubProgramID);
				if (FailedPortCount >= 0)
				{
					LoadMachineData();
					if (m_DatabaseAccessURL.GetOutLines(m_OutPorts) < 0)
						m_OutLineNum = 0;
					SetOutLineFont();
					ShowOutLineIcon();
					Invalidate(false);
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
			}
		}
		break;
	}
}



void CCallRecorderDlg::OnCbnSelchangeComboPotportsAssign()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CEdit* pThisEdit = (CEdit*)GetDlgItem(m_OutPorts[m_Item].OutLineBtn.DlgCtrlID);
	int Ports;
	int ListIndex = m_AssignOutPortList.GetCurSel();
	CHAR Buff[10];
	CHAR StrNum[10];
	m_AssignOutPortList.GetLBText(ListIndex, StrNum);
	Ports = atoi(StrNum);
	memset(Buff, 0x00, 10);
	if (Ports > 0)
		sprintf_s(Buff, 10, "%03d", Ports);
	pThisEdit->SetWindowTextA(Buff);
	pThisEdit->ShowWindow(TRUE);
	m_AssignOutPortList.ShowWindow(FALSE);
	m_OutPorts[m_Item].OutLineBtn.pOutLineNo->ShowWindow(TRUE);
	int MachineID = m_OutPorts[m_Item].MachineID;
	int Ret;

	Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(m_Item + 1, MACHINE_TYPE_CALLER_ID_BOX, MachineID, Ports, NULL);
	if(Ret == 0)
	{
		if (m_DatabaseAccessURL.GetOutLines(m_OutPorts) < 0)
			m_OutLineNum = 0;
	}
	else
	{
		// 指派失敗：這台部署的資料庫只能搬動既有佈線的實體埠，選到一個
		// 沒有既有佈線資料的埠號就會失敗。上面已經先把選到的埠號寫進
		// 編輯框（畫面看起來像是成功了），這裡明確告知失敗，並用
		// GetOutLines 重新讀回資料庫實際內容蓋掉那個沒有真的存進去的
		// 顯示值，避免畫面跟資料庫不一致。
		CString Msg;
		Msg.Format(_T("指派實體埠 %d 失敗：這個埠在資料庫裡沒有既有佈線資料，無法指派。"), Ports);
		MessageBox(Msg, _T("指派失敗"), MB_OK | MB_ICONWARNING);
		if (m_DatabaseAccessURL.GetOutLines(m_OutPorts) < 0)
			m_OutLineNum = 0;
		SetOutLineFont();
		ShowOutLineIcon();
		Invalidate(FALSE);
	}
}


int CCallRecorderDlg::LoadMachineData()
{
	// 原本對 3 種類型 x m_MachineNums 逐一呼叫 GetMachineInfo()，等於每次
	// 開啟這個分頁都要跑 30 次同步 HTTP 往返；改成一次 GET /machines 拿全部
	// 列回來自己分類，降成 1 次 HTTP 往返。
	memset(m_VoiceCardSubProgramGroup, 0x00, sizeof(SubProgramGroup_T) * m_MachineNums);
	m_DatabaseAccessURL.GetAllMachinesSubProgramInfo(m_PBXSubProgramGroup, m_CallerIDSubProgramGroup, m_VoiceCardSubProgramGroup);
	m_OutLineNum = GetDBOutVPortNums();

	return 0;
}


void CCallRecorderDlg::SetPortAssignList(int MachineID)
{
	CHAR Buff[200];
	int OutPortNum;
	int AssignedOutPort;
	int PortsArray[VIRTUAL_PORT_NUMS];
	int PortIndex = 0;
	OutPortNum = m_CallerIDSubProgramGroup[MachineID - 1].OutPortNum;
	AssignedOutPort = m_OutPorts[m_Item].MachineTypeBPhyPort;

	for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
	{
		if (
			m_OutPorts[i].MachineTypeBPhyPort > 0 && 
			m_OutPorts[i].IsInUsed &&
			m_OutPorts[i].MachineID == MachineID
			)
		{
			PortsArray[PortIndex] = m_OutPorts[i].MachineTypeBPhyPort;
			PortIndex++;
		}
	}

	for (int i = m_AssignOutPortList.GetCount() - 1; i >= 0; i--)
	{
		m_AssignOutPortList.DeleteString(i);
	}
	BOOL NotAssigned = FALSE;

	for (int i = 0; i <= OutPortNum; i++)
	{
		NotAssigned = FALSE;
		for (int j = 0; j < PortIndex; j++)
		{
			if (PortsArray[j] == i)
			{
				NotAssigned = TRUE;
				break;
			}
		}
		if (NotAssigned == FALSE)
		{
			sprintf_s(Buff, 100, "%03d", i);
			m_AssignOutPortList.AddString(Buff);
		}
		if (AssignedOutPort == i)
			m_AssignOutPortList.SetCurSel(i);
	}

}

//int CCallRecorderDlg::OutVPortDatabaseSetting(int ThisMachineIndex, int Ports)
//{
//	PSubProgramGroup_T pPBXSubProgramGroup;
//	PSubProgramGroup_T pThisSubProgramGroup;
//	PSubProgramGroup_T pVoiceCardSubProgramGroup;
//	int PBXOutPorts = 0;
//	int ThisOutPorts = 0;
//	int VoiceCardOutPorts = 0;
//	int Ret;
//	int PortIndex = 0;
//	POutPort_T pOutPorts;
//	BOOL bIsSetting = FALSE;
//	int OutVPort;
//
//	for (int i = 0; i < m_MachineNums; i++)
//	{
//		pPBXSubProgramGroup = &m_PBXSubProgramGroup[i];
//		pThisSubProgramGroup = &m_CallerIDSubProgramGroup[i];
//		PBXOutPorts = pPBXSubProgramGroup->OutPortNum;
//		ThisOutPorts = pThisSubProgramGroup->OutPortNum;
//		if (PBXOutPorts > 0 && ThisOutPorts > 0)
//		{
//			/*
//			來電盒有並接到交換機
//			PortIndex是根據PBX的外線數量
//			*/
//			PortIndex += PBXOutPorts;
//		}
//		else
//		{
//			/*來電盒沒有並接到交換機*/
//			if (i == ThisMachineIndex)
//			{
//				for (int i = 0; i < Ports; i++)
//				{
//					OutVPort = PortIndex + i;
//					pOutPorts = &m_OutPorts[OutVPort];
//					pOutPorts->IsInUsed = TRUE;
//					pOutPorts->MachineID = ThisMachineIndex;
//					pOutPorts->MachineTypeBPhyPort = i + 1;
//				}
//			}
//			PortIndex += ThisOutPorts;
//		}
//	}
//
//	return 0;
//}

int CCallRecorderDlg::CheckOutVPortSetting(int ThisMachineIDIndex)
{
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

	return Ret;
}



int CCallRecorderDlg::RedrawIcons()
{
	// TODO: 請在此新增您的實作程式碼.
	LoadMachineData();
	if (m_DatabaseAccessURL.GetOutLines(m_OutPorts) < 0)
		m_OutLineNum = 0;
	SetOutLineFont();
	ShowOutLineIcon();
	m_OldOutLineNum = m_OutLineNum;

	return 0;
}

void CCallRecorderDlg::MachineLogin(int MachineID, PCHAR pIPAddr, int SWVer)
{
	int Index = MachineID - 1;
	CHAR Ver[10];

	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_IP_ADDR, pIPAddr);
	sprintf_s(Ver, 10, "%d", SWVer);
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_FW_VER, Ver);
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_LINK_STATUS, "已連線");
	m_DatabaseAccessURL.SetMachineConnected(MACHINE_TYPE_CALLER_ID_BOX, MachineID, pIPAddr, TRUE, NULL);
}

void CCallRecorderDlg::MachineLogout(int MachineID)
{
	int Index = MachineID - 1;
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_IP_ADDR, "000.000.000.000");
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_FW_VER, "0");
	m_ListSubProgram.SetItemText(Index, ID_LIST_SUB_PROGRAM_LINK_STATUS, "離線");
	m_DatabaseAccessURL.SetMachineConnected(MACHINE_TYPE_CALLER_ID_BOX, MachineID, NULL, FALSE, NULL);
}

int CCallRecorderDlg::InitPorts()
{
	// TODO: 請在此新增您的實作程式碼.
	LoadMachineData();
	if (m_DatabaseAccessURL.GetOutLines(m_OutPorts) < 0)
		m_OutLineNum = 0;
	ShowSubProgramList();
	SetOutLineFont();
	ShowOutLineIcon();

	return 0;
}
