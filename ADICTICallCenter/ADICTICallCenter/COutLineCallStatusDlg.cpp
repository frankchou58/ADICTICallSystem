// COutLineCallStatusDlg.cpp: 實作檔案
//

#include "pch.h"
#include "ADICTICallCenter.h"
#include "COutLineCallStatusDlg.h"
#include "afxdialogex.h"


// COutLineCallStatusDlg 對話方塊

IMPLEMENT_DYNAMIC(COutLineCallStatusDlg, CDialogEx)

COutLineCallStatusDlg::COutLineCallStatusDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_COutLineCallStatusDlg, pParent)
{

}

COutLineCallStatusDlg::~COutLineCallStatusDlg()
{
}

void COutLineCallStatusDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(COutLineCallStatusDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &COutLineCallStatusDlg::OnBnClickedOk)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// COutLineCallStatusDlg 訊息處理常式


BOOL COutLineCallStatusDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	// TODO:  在此加入額外的初始化
	memset(m_OutPorts, 0x00, sizeof(OutPort_T) * VIRTUAL_PORT_NUMS);
	int DBOutPorts = 0;
	POutPort_T pOutPort;
	if (m_DatabaseAccessURL.GetOutLines(m_OutPorts) >= 0)
	{
		for (int i = 0; i < VIRTUAL_PORT_NUMS; i++)
		{
			pOutPort = &m_OutPorts[i];
			if (pOutPort->IsInUsed == TRUE)
				DBOutPorts++;
		}
	}
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

	m_OutLineCallStatusTitle.Create("虛擬外線通話狀態", WS_CHILD | WS_VISIBLE | SS_CENTER,
		CRect(0, 0, 0, 0), this, 2000);
	m_OutLineCallStatusTitle.SetFont(m_pTitleFont);

	m_pParent = GetParent();

	RECT ClientRect;
	m_pParent->GetClientRect(&ClientRect);

	RECT m_OutLineCallStatusTitleRect;
	m_OutLineCallStatusTitleRect.top = ClientRect.top;
	m_OutLineCallStatusTitleRect.right = ClientRect.right;
	m_OutLineCallStatusTitleRect.bottom = m_OutLineCallStatusTitleRect.top + m_TitleFontHeight;
	m_OutLineCallStatusTitleRect.left = ClientRect.left;
	m_OutLineCallStatusTitle.MoveWindow(CRect(m_OutLineCallStatusTitleRect));

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX 屬性頁應傳回 FALSE
}


void COutLineCallStatusDlg::OnBnClickedOk()
{
	// TODO: 在此加入控制項告知處理常式程式碼
	CDialogEx::OnOK();
}




HBRUSH COutLineCallStatusDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// TODO:  在此變更 DC 的任何屬性
	int ThisCtrlID = pWnd->GetDlgCtrlID();

	if (ThisCtrlID == 2000)
	{
		pDC->SetTextColor(RGB(202, 245, 194));
		pDC->SetBkColor(RGB(19, 34, 168));
	}

	// TODO:  如果預設值並非想要的，則傳回不同的筆刷
	return hbr;
}
