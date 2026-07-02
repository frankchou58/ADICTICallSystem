#pragma once


// CSettingDlg 對話方塊

class CSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CSettingDlg)

public:
	CSettingDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~CSettingDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CSettingDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	int m_ButtonFontHeight;
	int m_ButtonFontWidth;
	CFont* m_pButtonFont;
	CButton m_BtnSave;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CButton m_BtnCancel;
	CStatic m_TitleServerIPSetting;
	CEdit m_EditMachineServerIP;
	CStatic m_TitleMachineID;
	CStatic m_TitleOutPortNum;
//	CStatic m_ExtPortNum;
	CComboBox m_EditMachineID;
	CComboBox m_EditOutPortNum;
	CComboBox m_EditExtPortNum;
	CStatic m_TitleExtPortNum;
};
