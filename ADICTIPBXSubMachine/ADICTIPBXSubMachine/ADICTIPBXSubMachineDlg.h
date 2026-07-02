
// ADICTIPBXSubMachineDlg.h: 標頭檔
//

#pragma once
#include "CSettingDlg.h"
#include "CUartSettingDlg.h"
#include "Serial.h"
#include "CViewLogDlg.h"

enum MyEnum
{
	CTRL_ID_MACHINE_SETTING_DLG = 0,
	CTRL_ID_UART_SETTING_DLG,
	CTRL_ID_VIEW_LOG_DLG,
};

typedef struct Date_Tag
{
	int Year;
	int Month;
	int Day;
	int Minutes;
	int Seconds;
} Date_T, *PDate_T;

// CADICTIPBXSubMachineDlg 對話方塊
class CADICTIPBXSubMachineDlg : public CDialogEx
{
// 建構
public:
	CADICTIPBXSubMachineDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ADICTIPBXSUBMACHINE_DIALOG };
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
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	CTabCtrl m_TabCtrl;
	int m_fontHeight;
	int m_fontWidth;
	CFont* m_font;
	CSettingDlg m_CSettingDlg;
	CViewLogDlg m_CViewLogDlg;
	CStatic m_Message;
	CFont* m_MessageFont;
	int m_MessageFontHeight;
	int m_MessageFontWidth;
	int ShowMessage(PCHAR pMessage);
	HANDLE m_hThreadMachineProtoco;
//	afx_msg void OnTimer(UINT_PTR nIDEvent);
//	BOOL OpenUARTPort(int Port);
//	void CloseUARTPort();
	CUartSettingDlg m_CUartSettingDlg;
	afx_msg void OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	CWnd* GetViewLogWnd();
};
