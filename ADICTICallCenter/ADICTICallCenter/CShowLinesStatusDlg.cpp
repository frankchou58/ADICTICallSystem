// CShowLinesStatusDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallCenter.h"
#include "CShowLinesStatusDlg.h"
#include "afxdialogex.h"
#include "CSettingsDlg.h"
#include "ADICTICallCenter.h"
#include "WSServer.h"

// CShowLinesStatusDlg 對話方塊
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

IMPLEMENT_DYNAMIC(CShowLinesStatusDlg, CDialogEx)

CShowLinesStatusDlg::CShowLinesStatusDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CShowLinesStatusDlg, pParent)
{
}

CShowLinesStatusDlg::~CShowLinesStatusDlg()
{
}

void CShowLinesStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CShowLinesStatusDlg, CDialogEx)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CShowLinesStatusDlg 訊息處理常式


BOOL CShowLinesStatusDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此加入額外的初始化
	m_IconColorInit = TRUE;
	m_pGlobalCallBlock = GetGlobelCallBlockPtr();


	m_FontHeight = 18;
	m_pFont = new CFont();
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_FontHeight;
	lf.lfWeight = 700;
	_tcscpy_s(lf.lfFaceName, "微軟正黑體");
	m_pFont->CreateFontIndirect(&lf);

	m_StartOutLineIconCtrlID = 2001;
	m_StartExtLineIconCtrlID = m_StartOutLineIconCtrlID + VIRTUAL_PORT_NUMS;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX 屬性頁應傳回 FALSE
}

int CShowLinesStatusDlg::CreateLineStatusCompents()
{
	// TODO: 請在此新增您的實作程式碼.
	m_StatustTitleHeight = 35;
	m_StatustTitleWidth = m_StatustTitleHeight * 0.5;
	m_pStatustTitleFont = new CFont();
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_StatustTitleHeight;
	lf.lfWeight = 700;
	lf.lfWidth = m_StatustTitleWidth;
	_tcscpy_s(lf.lfFaceName, "微軟正黑體");
	//_tcscpy_s(lf.lfFaceName, "Calibri");
	m_pStatustTitleFont->CreateFontIndirect(&lf);


	m_OutLineString.Create("虛擬外線狀態", WS_CHILD | WS_VISIBLE | SS_CENTER,
		CRect(0, 0, 0, 0), this, 2000);

	m_ExtLineString.Create("虛擬內線狀態", WS_CHILD | WS_VISIBLE | SS_CENTER,
		CRect(0, 0, 0, 0), this, 2000);

	PVPortStatus_T pVPortStatus;
	for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
	{
		/*建立外線圖示*/
		pVPortStatus = (PVPortStatus_T)&m_OutLineStatus[i];
		if (pVPortStatus->pVPortNoString == NULL)
		{
			pVPortStatus->pVPortNoString = new CStatic();
			pVPortStatus->pVPortNoString->Create(
				"", WS_CHILD | WS_VISIBLE | SS_CENTER,
				CRect(0, 0, 0, 0), this);
		}
		if (pVPortStatus->pVPortIcon == NULL)
		{
			sprintf_s(pVPortStatus->VPortNoStr, 10, "000");
			pVPortStatus->IconCtlID = m_StartOutLineIconCtrlID + i;
			pVPortStatus->pVPortIcon = new CStatic();
			pVPortStatus->pVPortIcon->Create(
				"", WS_CHILD | WS_VISIBLE | SS_CENTER,
				CRect(0, 0, 0, 0), this, pVPortStatus->IconCtlID);
		}

		/*建立內線圖示*/
		pVPortStatus = (PVPortStatus_T)&m_ExtLineStatus[i];
		if (pVPortStatus->pVPortNoString == NULL)
		{
			pVPortStatus->pVPortNoString = new CStatic();
			pVPortStatus->pVPortNoString->Create(
				"", WS_CHILD | WS_VISIBLE | SS_CENTER,
				CRect(0, 0, 0, 0), this);
		}
		if (pVPortStatus->pVPortIcon == NULL)
		{
			sprintf_s(pVPortStatus->VPortNoStr, 10, "000");
			pVPortStatus->IconCtlID = m_StartExtLineIconCtrlID + i;
			pVPortStatus->pVPortIcon = new CStatic();
			pVPortStatus->pVPortIcon->Create(
				"", WS_CHILD | WS_VISIBLE | SS_CENTER,
				CRect(0, 0, 0, 0), this, pVPortStatus->IconCtlID);
		}

	}

	return 0;
}

int CShowLinesStatusDlg::ShowLineStatus()
{
	PCallBlock_T pCallBlock;

	RECT ClientRect;
	m_pParent->GetClientRect(&ClientRect);
	RECT NewRect;

	NewRect.top = ClientRect.top;
	NewRect.right = ClientRect.right;
	NewRect.bottom = NewRect.top + m_StatustTitleHeight;
	NewRect.left = ClientRect.left;
	m_OutLineString.MoveWindow(CRect(NewRect));
	m_OutLineString.SetFont(m_pStatustTitleFont);
	m_OutLineBottomPos = NewRect.bottom;

	NewRect.top = (ClientRect.bottom - ClientRect.top) / 2.3 + m_StatustTitleHeight;
	NewRect.right = ClientRect.right;
	NewRect.bottom = NewRect.top + m_StatustTitleHeight;
	NewRect.left = ClientRect.left;
	m_ExtLineString.MoveWindow(CRect(NewRect));
	m_ExtLineString.SetFont(m_pStatustTitleFont);
	m_ExtLineBottomPos = NewRect.bottom;

	SetOutLineStatusFont();
	ShowOutLineStatus();
	SetExtLineStatusFont();
	ShowExtLineStatus();

	return 0;
}

int CShowLinesStatusDlg::ShowOutLineStatus()
{
	RECT ClientRect;
	m_pParent->GetClientRect(&ClientRect);
	RECT ListRect;
	m_OutLineString.GetClientRect(&ListRect);
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

	int Width = (ClientRect.right - ClientRect.left);
	int Height = (ClientRect.bottom - ClientRect.top) / 2.3;
	int XGap;
	int YGap;
	CRect Rect;
	int XPos = 0;
	int	YPos = ListRect.bottom + m_StatustTitleHeight / 20;
	int YIndex = 0;
	int XIndex = 0;
	int X;
	int Y;

	CHAR Buff[100];
	PVPortStatus_T pVPortStatus;
	for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
	{
		pVPortStatus = &m_OutLineStatus[i];
		pVPortStatus->pVPortNoString->ShowWindow(false);
		pVPortStatus->pVPortIcon->ShowWindow(false);
	}
	for (int i = 0; i < m_OutLineNum; i++)
	{
		pVPortStatus = &m_OutLineStatus[i];
		sprintf_s(Buff, 100, "%03d", i + 1);
		pVPortStatus->pVPortNoString->SetWindowTextA(Buff);
		pVPortStatus->pVPortNoString->SetFont(m_pOutStatustFont);
		pVPortStatus->pVPortIcon->SetWindowTextA(pVPortStatus->VPortNoStr);
		pVPortStatus->pVPortIcon->SetFont(m_pOutStatustFont);

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
			X = 16; Y = 8;
			break;
		case 7:
			X = 30; Y = 8;
			break;
		}
		XGap = (Width / X);
		YGap = (Height / Y) - m_OutLineStatustHeightGap;
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
		Rect.bottom = Rect.top + m_OutStatustHeight;
		Rect.left = XPos;
		Rect.right = Rect.left + m_OutStatustWidth * 4;
		pVPortStatus->pVPortNoString->MoveWindow(CRect(Rect));
		pVPortStatus->pVPortNoString->ShowWindow(TRUE);

		Rect.top = YPos + m_OutStatustHeight + 0;
		Rect.bottom = Rect.top + m_OutStatustHeight + 0;
		Rect.left = XPos;
		Rect.right = XPos + m_OutStatustWidth * 4;
		pVPortStatus->pVPortIcon->MoveWindow(CRect(Rect));
		pVPortStatus->pVPortIcon->ShowWindow(TRUE);
	}


	return 0;
}

int CShowLinesStatusDlg::ShowExtLineStatus()
{
	RECT ClientRect;
	m_pParent->GetClientRect(&ClientRect);
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
	int Width = (ClientRect.right - ClientRect.left);
	int Height = (ClientRect.bottom - ClientRect.top) / 2.3;
	int XGap;
	int YGap;
	CRect Rect;
	int XPos = 0;
	int	YPos = (ClientRect.bottom - ClientRect.top) / 2.3 + m_StatustTitleHeight / 0.5;

	int YIndex = 0;
	int XIndex = 0;
	int X;
	int Y;

	CHAR Buff[100];
	PVPortStatus_T pVPortStatus;
	for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
	{
		pVPortStatus = &m_ExtLineStatus[i];
		pVPortStatus->pVPortNoString->ShowWindow(false);
		pVPortStatus->pVPortIcon->ShowWindow(false);
	}
	for (int i = 0; i < m_ExtLineNum; i++)
	{
		pVPortStatus = &m_ExtLineStatus[i];
		sprintf_s(Buff, 100, "%03d", i + 1);
		pVPortStatus->pVPortNoString->SetWindowTextA(Buff);
		pVPortStatus->pVPortNoString->SetFont(m_pExtStatustFont);
		pVPortStatus->pVPortIcon->SetWindowTextA(pVPortStatus->VPortNoStr);
		pVPortStatus->pVPortIcon->SetFont(m_pExtStatustFont);
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
			X = 16; Y = 8;
			break;
		case 7:
			X = 30; Y = 8;
			break;
		}
		XGap = (Width / X);
		YGap = (Height / Y) - m_ExtLineStatustHeightGap;
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
		Rect.bottom = Rect.top + m_ExtStatustHeight;
		Rect.left = XPos;
		Rect.right = XPos + m_ExtStatustWidth * 4;
		pVPortStatus->pVPortNoString->MoveWindow(CRect(Rect));
		pVPortStatus->pVPortNoString->ShowWindow(TRUE);

		Rect.top = YPos + m_ExtStatustHeight + 1;
		Rect.bottom = Rect.top + m_ExtStatustHeight;
		Rect.left = XPos;
		Rect.right = XPos + m_ExtStatustWidth * 4;
		pVPortStatus->pVPortIcon->MoveWindow(CRect(Rect));
		pVPortStatus->pVPortIcon->ShowWindow(TRUE);
	}
	return 0;
}


HBRUSH CShowLinesStatusDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此變更 DC 的任何屬性
	//return hbr;
	int ThisCtrlID = pWnd->GetDlgCtrlID();

	if (ThisCtrlID == 2000)
	{
		pDC->SetTextColor(RGB(202, 245, 194));
		pDC->SetBkColor(RGB(19, 34, 168));
	}
	else if (ThisCtrlID >= m_StartOutLineIconCtrlID && ThisCtrlID <= (m_StartExtLineIconCtrlID + VIRTUAL_PORT_NUMS) && m_IconColorInit == TRUE)
	{
		pDC->SetTextColor(RGB(172, 172, 215));
		pDC->SetBkColor(RGB(172, 172, 215));
		if (ThisCtrlID == (m_StartExtLineIconCtrlID + VIRTUAL_PORT_NUMS))
			m_IconColorInit = FALSE;
	}

	PVPortStatus_T pVPortStatus;
	CStatic* pOutLineIcon;
	int CallStatus;
	PCallBlock_T pCallBlock;
	for (int Index = 0; Index < VIRTUAL_PORT_NUMS; Index++)
	{
		pCallBlock = (PCallBlock_T)&m_pGlobalCallBlock[Index];
		//for (int i = 0; i < m_OutLineNum; i++)
		{
			pVPortStatus = (PVPortStatus_T)pCallBlock->pFromLineStatus;
			if (pVPortStatus)
			{
				if (pVPortStatus->IconCtlID == ThisCtrlID)
				{
					CallStatus = pCallBlock->CallStatus;
					switch (CallStatus)
					{
					case STATUS_NO_USE:
						//pDC->SetTextColor(RGB(150, 150, 150));
						//pDC->SetBkColor(RGB(150, 150, 150));
						break;
					case STATUS_CALL_IN:
						if (pCallBlock->CallType == TYPE_INTERNAL_CALL)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(231, 176, 232));
						}
						else if (pCallBlock->CallType == TYPE_CALL_IN || pCallBlock->CallType == TYPE_CALL_OUT)
						{
							pDC->SetTextColor(RGB(31, 211, 171));
							pDC->SetBkColor(RGB(31, 211, 171));
						}
						break;
					case STATUS_CALL_OUT:
						if (pCallBlock->CallType == TYPE_INTERNAL_CALL)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(231, 176, 232));
						}
						else if (pCallBlock->CallType == TYPE_CALL_IN || pCallBlock->CallType == TYPE_CALL_OUT)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(255, 255, 140));
						}

						break;
					case STATUS_CALL_TALKING:
						if (pCallBlock->CallType == TYPE_INTERNAL_CALL)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(238, 68, 187));
						}
						else if (pCallBlock->CallType == TYPE_CALL_IN || pCallBlock->CallType == TYPE_CALL_OUT)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(164, 255, 164));
						}
						break;
					case STATUS_CALL_OFF:
						//pDC->SetTextColor(RGB(172, 172, 215));
						//pDC->SetBkColor(RGB(172, 172, 215));
						break;
					}
				}
			}
		}
		//for (int i = 0; i < m_ExtLineNum; i++)
		{
			pVPortStatus = (PVPortStatus_T)pCallBlock->pToLineStatus;
			if (pVPortStatus)
			{
				if (pVPortStatus->IconCtlID == ThisCtrlID)
				{
					CallStatus = pCallBlock->CallStatus;
					switch (CallStatus)
					{
					case STATUS_NO_USE:
						//pDC->SetTextColor(RGB(150, 150, 150));
						//pDC->SetBkColor(RGB(150, 150, 150));
						break;
					case STATUS_CALL_IN:
						if (pCallBlock->CallType == TYPE_INTERNAL_CALL)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(231, 176, 232));
						}
						else if (pCallBlock->CallType == TYPE_CALL_IN || pCallBlock->CallType == TYPE_CALL_OUT)
						{
							pDC->SetTextColor(RGB(31, 211, 171));
							pDC->SetBkColor(RGB(31, 211, 171));
						}
						break;
					case STATUS_CALL_OUT:
						if (pCallBlock->CallType == TYPE_INTERNAL_CALL)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(231, 176, 232));
						}
						else if (pCallBlock->CallType == TYPE_CALL_IN || pCallBlock->CallType == TYPE_CALL_OUT)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(255, 255, 140));
						}
						break;
					case STATUS_CALL_TALKING:
						if (pCallBlock->CallType == TYPE_INTERNAL_CALL)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(238, 68, 187));
						}
						else if (pCallBlock->CallType == TYPE_CALL_IN || pCallBlock->CallType == TYPE_CALL_OUT)
						{
							pDC->SetTextColor(RGB(0, 0, 0));
							pDC->SetBkColor(RGB(164, 255, 164));
						}
						break;
					case STATUS_CALL_OFF:
						//pDC->SetTextColor(RGB(172, 172, 215));
						//pDC->SetBkColor(RGB(172, 172, 215));
						break;
					}
				}
			}
		}
	}

	// TODO:  如果預設值並非想要的，則傳回不同的筆刷
	return hbr;
}


//void CShowLinesStatusDlg::OnBnClickedButtonLinesNumSetting()
//{
//	// TODO: 在此加入控制項告知處理常式程式碼
//	CSettingsDlg SettingsDlg;
//	SettingsDlg.DoModal();
//	m_OutLineNum = AfxGetApp()->GetProfileInt("SystemSetting", "OutLineNum", 16);
//	m_ExtLineNum = AfxGetApp()->GetProfileInt("SystemSetting", "ExtLineNum", 16);
//	ShowLineStatus();
//}

void CShowLinesStatusDlg::SetOutLineStatusFont()
{
	double Ratio =0;
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
	case 0:
		m_OutStatustHeight = 0;
		break;
	case 1:
		Ratio = 1.13;
		m_OutLineStatustHeightGap = 12;
		m_OutStatustHeight = 104;
		break;
	case 2:
		Ratio = 2.2;
		m_OutLineStatustHeightGap = 0;
		m_OutStatustHeight = 52;
		break;
	case 3:
		Ratio = 1.65;
		m_OutLineStatustHeightGap = 0;
		m_OutStatustHeight = 48;
		break;
	case 4:
		Ratio = 1.3;
		m_OutLineStatustHeightGap = 0;
		m_OutStatustHeight = 46;
		break;
	case 5:
		Ratio = 2.3;
		m_OutLineStatustHeightGap = 0;
		m_OutStatustHeight = 26;
		break;
	case 6:
		Ratio = 1.1;
		m_OutLineStatustHeightGap = 0;
		m_OutStatustHeight = 27;
		break;
	case 7:
		Ratio = 0.58;
		m_OutLineStatustHeightGap = 0;
		m_OutStatustHeight = 27;
		break;
	}
	LOGFONT lf;
	m_OutStatustWidth = m_OutStatustHeight * Ratio;
	m_pOutStatustFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_OutStatustHeight;
	lf.lfWeight = 700;
	lf.lfWidth = m_OutStatustWidth;
	_tcscpy_s(lf.lfFaceName, "Arial");
	m_pOutStatustFont->CreateFontIndirect(&lf);
}

void CShowLinesStatusDlg::SetExtLineStatusFont()
{
	double Ratio = 0;
#if 0
	int Index = 0;
	for (int i = 0; i < LINES_NUM_ARRAY; i++)
	{
		if (Numbers[i] == m_ExtLineNum)
		{
			Index = i;
			break;
		}
	}
#endif
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
	case 0:
		m_ExtStatustHeight = 0;
		break;
	case 1:
		Ratio = 1.13;
		m_ExtLineStatustHeightGap = 12;
		m_ExtStatustHeight = 104;
		break;
	case 2:
		Ratio = 2.2;
		m_ExtLineStatustHeightGap = 0;
		m_ExtStatustHeight = 52;
		break;
	case 3:
		Ratio = 1.65;
		m_ExtLineStatustHeightGap = 0;
		m_ExtStatustHeight = 48;
		break;
	case 4:
		Ratio = 1.3;
		m_ExtLineStatustHeightGap = 0;
		m_ExtStatustHeight = 46;
		break;
	case 5:
		Ratio = 2.3;
		m_ExtLineStatustHeightGap = 0;
		m_ExtStatustHeight = 26;
		break;
	case 6:
		Ratio = 1.1;
		m_ExtLineStatustHeightGap = 0;
		m_ExtStatustHeight = 27;
		break;
	case 7:
		Ratio = 0.58;
		m_ExtLineStatustHeightGap = 0;
		m_ExtStatustHeight = 27;
		break;
	}
	LOGFONT lf;
	m_ExtStatustWidth = m_ExtStatustHeight * Ratio;
	m_pExtStatustFont = new CFont();
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfHeight = m_ExtStatustHeight;
	lf.lfWeight = 700;
	lf.lfWidth = m_ExtStatustWidth;
	_tcscpy_s(lf.lfFaceName, "Arial");
	m_pExtStatustFont->CreateFontIndirect(&lf);
}



int CShowLinesStatusDlg::SetLineCallStatusLed(PCallBlock_T pCallBlock, int Status)
{
	// TODO: 請在此新增您的實作程式碼.
	if (pCallBlock == NULL)
		return -1;

	pCallBlock->CallStatus = Status;
	CallType_T CallType = pCallBlock->CallType;
	PVPortStatus_T pVPortStatus = NULL;
	UINT OutLineIndex;
	UINT FromExtLineIndex;
	UINT ToExtLineIndex;
	UINT ExtLineIndex;
	switch (CallType)
	{
	case TYPE_CALL_IN:
		/*外線打內線*/
		if (pCallBlock->OutVPort > 0)
		{
			/*根據OutNo找出外線圖示的指標並將該指標指定給pFromLineStatus*/
			OutLineIndex = pCallBlock->OutVPort - 1;
			pVPortStatus = &m_OutLineStatus[OutLineIndex];
			sprintf_s(pVPortStatus->VPortNoStr, 10, "%03d", pCallBlock->ToExtVPort);
			pCallBlock->pFromLineStatus = pVPortStatus;
		}
		else
			pCallBlock->pFromLineStatus = NULL;

		if (pCallBlock->ToExtVPort > 0)
		{
			/*根據ToExtVPort找出內線圖示的指標並將該指標指定給pToLineStatus*/
			ExtLineIndex = pCallBlock->ToExtVPort - 1;
			pVPortStatus = &m_ExtLineStatus[ExtLineIndex];
			sprintf_s(pVPortStatus->VPortNoStr, 10, "%03d", pCallBlock->OutVPort);
			pCallBlock->pToLineStatus = pVPortStatus;
		}
		else
			pCallBlock->pToLineStatus = NULL;

		if (pCallBlock->FromExtVPort > 0)
		{
			/*外線打內線決不會有內線From的虛擬Port*/
			printf("wedwedwe");
		}
		break;
	case TYPE_CALL_OUT:
		/*內線打外線*/
		if (pCallBlock->OutVPort > 0)
		{
			/*根據OutNo找出外線圖示的指標並將該指標指定給pToLineStatus*/
			OutLineIndex = pCallBlock->OutVPort - 1;
			pVPortStatus = &m_OutLineStatus[OutLineIndex];
			sprintf_s(pVPortStatus->VPortNoStr, 10, "%03d", pCallBlock->FromExtVPort);
			pCallBlock->pToLineStatus = pVPortStatus;
		}
		else
			pCallBlock->pToLineStatus = NULL;

		if (pCallBlock->FromExtVPort > 0)
		{
			/*根據FromExtVPort找出內線圖示的指標並將該指標指定給pFromLineStatus*/
			ExtLineIndex = pCallBlock->FromExtVPort - 1;
			pVPortStatus = &m_ExtLineStatus[ExtLineIndex];
			sprintf_s(pVPortStatus->VPortNoStr, 10, "%03d", pCallBlock->OutVPort);
			pCallBlock->pFromLineStatus = pVPortStatus;
		}
		else
			pCallBlock->pFromLineStatus = NULL;

		if (pCallBlock->ToExtVPort > 0)
		{
			/*內線打外線決不會有內線To的虛擬Port*/
			printf("wedwedwe");
		}

		break;
	case TYPE_INTERNAL_CALL:
		/*內線打內線*/
		if (pCallBlock->OutVPort > 0)
		{
			/*內線打內線決不會有外線的虛擬Port*/
			printf("wedwedwe");
		}

		if (pCallBlock->FromExtVPort != 0)
		{
			/*根據FromExtVPort找出內線圖示的指標並將該指標指定給pFromLineStatus*/
			FromExtLineIndex = pCallBlock->FromExtVPort - 1;
			pVPortStatus = &m_ExtLineStatus[FromExtLineIndex];
			sprintf_s(pVPortStatus->VPortNoStr, 10, "%03d", pCallBlock->ToExtVPort);
			pCallBlock->pFromLineStatus = pVPortStatus;
		}
		else
			pCallBlock->pFromLineStatus = NULL;

		if (pCallBlock->ToExtVPort != 0)
		{
			/*根據ToExtVPort找出內線圖示的指標並將該指標指定給pToLineStatus*/
			ToExtLineIndex = pCallBlock->ToExtVPort - 1;
			pVPortStatus = &m_ExtLineStatus[ToExtLineIndex];
			sprintf_s(pVPortStatus->VPortNoStr, 10, "%03d", pCallBlock->FromExtVPort);
			pCallBlock->pToLineStatus = pVPortStatus;
		}
		else
			pCallBlock->pToLineStatus = NULL;

		break;
	}

	if (pCallBlock->pFromLineStatus)
	{
		pCallBlock->pFromLineStatus->pVPortIcon->SetWindowTextA(pCallBlock->pFromLineStatus->VPortNoStr);
	}
	if (pCallBlock->pToLineStatus)
	{
		pCallBlock->pToLineStatus->pVPortIcon->SetWindowTextA(pCallBlock->pToLineStatus->VPortNoStr);
	}

	/*發送目前狀態到Web Scoket機制*/
	WSSendOutPortStatus(pCallBlock);

	return 0;
}

int CShowLinesStatusDlg::RedrawIcons()
{
	// TODO: 請在此新增您的實作程式碼.
	//LoadMachineData();
	//m_DatabaseAccessURL.GetExtLines(m_ExtPorts);
	m_ExtLineNum = GetDBExtVPortNums();
	m_OutLineNum = GetDBOutVPortNums();
	ShowLineStatus();

	return 0;
}


int CShowLinesStatusDlg::InitPorts()
{
	// TODO: 請在此新增您的實作程式碼.
	m_pParent = GetParent();
	m_ExtLineNum = GetDBExtVPortNums();
	m_OutLineNum = GetDBOutVPortNums();

	CreateLineStatusCompents();
	ShowLineStatus();
	return 0;
}
