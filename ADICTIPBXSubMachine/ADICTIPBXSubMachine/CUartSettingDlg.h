#pragma once


// CUartSettingDlg 對話方塊

class CUartSettingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CUartSettingDlg)

public:
	CUartSettingDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~CUartSettingDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CUartSettingDlg };
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
	CButton m_BtnCancel;
	afx_msg void OnBnClickedCancel();
	CStatic m_TitleCOMPortList;
	CComboBox m_EditCOMPortSelect;
	CStatic m_TitleCOMPortBaudRate;
	CComboBox m_EditCOMBaudRateList;
};
