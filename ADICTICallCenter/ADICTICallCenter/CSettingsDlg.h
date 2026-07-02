#pragma once


// CSettingsDlg 對話方塊

class CSettingsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingsDlg)

public:
	CSettingsDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~CSettingsDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CSettingsDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CButton m_OkButton;
	CButton m_CancelButton;
	CWnd* m_pParent;
	int m_ButtonFontHeight;
	int m_ButtonFontWidth;
	CFont* m_pButtonFont;
	CComboBox m_SelectNetworkDevice;
	CStatic m_ShowMessageBox;
	CEdit m_DatabaseBackEndURL;
	CEdit m_TitleDatabaseBackEndURL;
//	CEdit m_TelNoPrioritySelect;
	CEdit m_TelNoPriorityTitle;
	CComboBox m_TelNoPrioritySelect;
	CStatic m_TitleCallStatusToDB;
//	CComboBox m_EditCallStatusToDB;
//	afx_msg void OnCbnSelchangeComboCallStatusToDb();
	CComboBox m_EditCallStatusToDB;
	CStatic m_TitleDatabaseBackEndURLPort;
	CEdit m_DatabaseBackEndURLPort;
};
