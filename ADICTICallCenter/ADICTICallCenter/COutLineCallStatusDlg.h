#pragma once
#include "CDatabaseAccessURL.h"


// COutLineCallStatusDlg 對話方塊

class COutLineCallStatusDlg : public CDialogEx
{
	DECLARE_DYNAMIC(COutLineCallStatusDlg)

public:
	COutLineCallStatusDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~COutLineCallStatusDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_COutLineCallStatusDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	OutPort_T m_OutPorts[VIRTUAL_PORT_NUMS];
	CDatabaseAccessURL m_DatabaseAccessURL;
	CStatic m_OutLineCallStatusTitle;
	int m_TitleFontHeight;
	int m_TitleFontWidth;
	CFont* m_pTitleFont;
	CWnd* m_pParent;

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
