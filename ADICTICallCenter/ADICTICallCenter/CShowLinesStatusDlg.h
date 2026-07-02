#pragma once
#include "CallStateRecordEngine.h"
#include "CDatabaseAccessURL.h"
#include "VirtualOutPortDBAccess.h"

// CShowLinesStatusDlg 對話方塊

class CShowLinesStatusDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CShowLinesStatusDlg)

public:
	CShowLinesStatusDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~CShowLinesStatusDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CShowLinesStatusDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	int CreateLineStatusCompents();
	int m_StatustTitleHeight;
	int m_StatustTitleWidth;
	CFont* m_pStatustTitleFont;
	CStatic m_OutLineString;
	CStatic m_ExtLineString;
	int ShowLineStatus();
	CWnd* m_pParent;
	int m_OutLineBottomPos;
	int m_ExtLineBottomPos;
	VPortStatus_T m_OutLineStatus[VIRTUAL_PORT_NUMS];
	VPortStatus_T m_ExtLineStatus[VIRTUAL_PORT_NUMS];
	int m_OutLineNum;
	int m_ExtLineNum;
	int m_OutStatustWidth;
	int m_OutStatustHeight;
	int m_ExtStatustWidth;
	int m_ExtStatustHeight;
	CFont* m_pOutStatustFont;
	CFont* m_pExtStatustFont;
	int ShowOutLineStatus();
	int ShowExtLineStatus();
	int m_StartOutLineIconCtrlID;
	int m_StartExtLineIconCtrlID;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
//	CButton m_BtnLinesNumSetting;
	CFont* m_pFont;
	int m_FontHeight;
//	afx_msg void OnBnClickedButtonLinesNumSetting();
	void SetOutLineStatusFont();
	void SetExtLineStatusFont();
	int SetLineCallStatusLed(PCallBlock_T pCallBlock, int Status);
	PCallBlock_T m_pGlobalCallBlock;
	BOOL m_IconColorInit;
	int m_OutLineStatustHeightGap;
	int m_ExtLineStatustHeightGap;
	int RedrawIcons();
	int InitPorts();
};
