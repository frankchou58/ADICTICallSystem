
// ADICTICallerIdBoxDlg.h: 標頭檔
//

#pragma once
#include "CSettingDlg.h"
#include "CUartSettingDlg.h"
#include "CViewLogDlg.h"

enum MyEnum
{
	CTRL_ID_MACHINE_SETTING_DLG = 0,
	CTRL_ID_UART_SETTING_DLG,
	CTRL_ID_VIEW_LOG_DLG,
};


// CADICTICallerIdBoxDlg 對話方塊
class CADICTICallerIdBoxDlg : public CDialogEx
{
// 建構
public:
	CADICTICallerIdBoxDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ADICTICALLERIDBOX_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支援


// 程式碼實作
protected:
	HICON m_hIcon;

	// 產生的訊息對應函式
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	bool m_DeviceStarted;
	int m_DeviceCounter;
	CTabCtrl m_TabCtrl;
	CFont* m_MessageFont;
	int m_MessageFontHeight;
	int m_MessageFontWidth;
	int m_fontHeight;
	int m_fontWidth;
	CFont* m_font;
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	CStatic m_Message;
	CSettingDlg m_CSettingDlg;
	CUartSettingDlg m_CUartSettingDlg;
	afx_msg void OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	CViewLogDlg m_CViewLogDlg;
	int ShowMessage(PCHAR pMessage);
	CWnd* GetViewLogWnd();
	CWnd* GetSettingWnd();
	CWnd* GetUartSettingWnd();
//	int m_OutPorts;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL DestroyWindow();
	afx_msg void OnClose();
};

