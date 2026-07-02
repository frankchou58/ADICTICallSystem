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
	CStatic m_LogBox;
	virtual BOOL OnInitDialog();
	CButton m_BtnSave;
	CButton m_BtnCancel;
	int m_ButtonFontHeight;
	int m_ButtonFontWidth;
	CFont* m_pButtonFont;
	CStatic m_TitleViewMode;
	CComboBox m_EditViewMode;
	afx_msg void OnCbnSelchangeComboListViewMode();
	int WriteLog(int Level, int Type, PCHAR pMessage);
	int m_SqLiteResult;
	CStatic m_TitleViewDate;
	CDateTimeCtrl m_EditViewDate;
	CStatic m_TitleViewLevel;
	CComboBox m_EditViewLevel;
	CComboBox m_EditViewRange;
	CListCtrl m_ListLog;
	int m_ListFontHeight;
	int m_ListFontWidth;
	CFont* m_pListFont;
	afx_msg void OnCbnSelchangeComboViewRange();
	int m_ViewLogRangeIndex;
	afx_msg void OnDtnDatetimechangeDatetimepickerViewDate(NMHDR* pNMHDR, LRESULT* pResult);
	void GetTimeRange(int* pStartTimestamp, int* pEndTimestamp);
	LogParam_T m_LogParam;
	PLog_T m_pLogData;
	afx_msg void OnCbnSelchangeComboViewLevel();
	void ShowLogList();
	int m_Numbers;
	CButton m_BtnClearLog;
	afx_msg void OnBnClickedButtonClearLog();
	int m_ListIndex;
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnDeleteLog();
	int m_uSelectedCount;
	int m_nItemIdx;
};
