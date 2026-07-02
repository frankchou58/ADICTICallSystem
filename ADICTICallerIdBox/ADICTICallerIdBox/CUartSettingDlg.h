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
	CButton m_BtnCancel;
	CStatic m_TitleCOMPortList;
	CComboBox m_EditCOMPortSelect;
	CStatic m_TitleCOMPortBaudRate;
	CComboBox m_EditCOMBaudRateList;
	afx_msg void OnColorCallerIdBoxList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CListCtrl m_ListCallerIdBox;
	CStatic m_TitleListCallerIdBox;
	afx_msg void OnBnClickedButtonReflash();
	CButton m_BtnReflashUart;
	virtual BOOL DestroyWindow();
	CStatic m_ShowTitleDTMFTimeout;
	CEdit m_EditDTMFTimeout;
};

enum
{
	ID_LIST_BOX_OUT_LINE_NO = 0,
	ID_LIST_BOX_INFO_IN_STORAGE,
	ID_LIST_BOX_SERIAL_NO_IN_STORAGE,
	ID_LIST_BOX_OUT_PORTS_IN_STORAGE,
	ID_LIST_BOX_UART_PORT_IN_STORAGE,

	ID_LIST_BOX_INFO_IN_CURRENT,
	ID_LIST_BOX_SERIAL_NO_IN_CURRENT,
	ID_LIST_BOX_OUT_PORTS_IN_CURRENT,
	ID_LIST_BOX_UART_PORT_IN_CURRENT,
	ID_LIST_BOX_SW_VER_IN_CURRENT,
};
