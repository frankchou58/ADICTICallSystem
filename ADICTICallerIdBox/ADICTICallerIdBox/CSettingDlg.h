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
	CButton m_BtnCancel;
	CStatic m_TitleMachineID;
	CComboBox m_EditMachineID;
	CStatic m_TitleOutPortNum;
	CComboBox m_EditOutPortNum;
	CStatic m_TitleExtPortNum;
	CComboBox m_EditExtPortNum;
	CStatic m_TitleServerIPSetting;
	CEdit m_EditMachineServerIP;
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
//	int ShowMessage(PCHAR pMessage);
	CEdit m_ShowOutPortNum;
	int ShowOutPortNumber(int OutPorts);
};
