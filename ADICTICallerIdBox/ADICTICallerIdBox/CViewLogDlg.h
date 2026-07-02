#pragma once
#include "database.h"


// CViewLogDlg 對話方塊

class CViewLogDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CViewLogDlg)

public:
	CViewLogDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~CViewLogDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CViewLogDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	int m_ButtonFontHeight;
	int m_ButtonFontWidth;
	CFont* m_pButtonFont;
	int m_SqLiteResult;
	CButton m_BtnSave;
	CButton m_BtnCancel;
	CStatic m_TitleViewMode;
	CComboBox m_EditViewMode;
	CStatic m_TitleViewLevel;
	CComboBox m_EditViewLevel;
	CStatic m_TitleViewDate;
	CDateTimeCtrl m_EditViewDate;
	CComboBox m_EditViewRange;
	CButton m_BtnClearLog;
	int m_ViewLogRangeIndex;
	LogParam_T m_LogParam;
	int m_ListFontHeight;
	int m_ListFontWidth;
	CFont* m_pListFont;
	CListCtrl m_ListLog;
	int GetTimeRange(int* pStartTimestamp, int* pEndTimestamp);
	int m_Numbers;
	PLog_T m_pLogData;
	int m_ListIndex;
	int WriteLog(int Level, int Type, int vLine, PCHAR pMessage);
	afx_msg void OnBnClickedButtonClearLog();
	afx_msg void OnCbnSelchangeComboListViewMode();
	afx_msg void OnCbnSelchangeComboViewLevel();
	afx_msg void OnCbnSelchangeComboViewRange();
	afx_msg void OnDtnDatetimechangeDatetimepickerViewDuration(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedButtonReflashRecord();
	CButton m_BtnReflashLog;
	int LoadAndShowLog();
	int InsertLogInList(PLog_T pLog, PCHAR pMessage);
	int m_ID;
	CStatic m_TitleSelectVLine;
	CComboBox m_SelectVLine;
	int SetOutPortNumber(int OutPorts);
	afx_msg void OnCbnSelchangeComboVline();
};
