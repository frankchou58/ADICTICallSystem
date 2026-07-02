
// ADICTICallCenterDlg.h: 標頭檔
//

#pragma once
#include "CDatabaseAccessURL.h"
#include "CShowLinesStatusDlg.h"
#include "CPbxDlg.h"
#include "CCallRecorderDlg.h"
#include "CVoiceRecorderDlg.h"
#include "COutLineCallStatusDlg.h"
#include "CSettingsDlg.h"
#include "CWebConsoleDlg.h"
#include "MachineServer.h"

// CADICTICallCenterDlg 對話方塊
class CADICTICallCenterDlg : public CDialogEx
{
// 建構
public:
	CADICTICallCenterDlg(CWnd* pParent = nullptr);	// 標準建構函式

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ADICTICALLCENTER_DIALOG };
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
//	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_SubProgramList;
//	int ShowSubProgramList();
//	void OnSize(UINT nType, int cx, int cy);
//	afx_msg void OnSize(UINT nType, int cx, int cy);
//	afx_msg void OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	CButton m_CloseAPPBtn;
	RECT m_CloseButtonRect;
	int SetCloseBtnPos();
	CStatic m_SubProgramListTitle;
	CStatic m_LineTitle;
	CStatic m_ExtTitle;
	int m_OutPortRectBottom;
	int m_ExtPortRectBottom;
	int m_ListHeight;
	int m_fontHeight;
	CFont* m_font;
	CFont* m_ListFont;
	int ShowOutPortNo(int SubProgramIndex);
//	CStatic m_TestBMP;
	int ShowExtPortNo(int SubProgramIndex);
	//int SetLineCallStatusLed(int SubProgramID, int LineNo, int ExtNo, int Status);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	CDatabaseAccessURL m_DatabaseAccessURL;
	SubProgramGroup_T m_SubProgramGroup[SUBPROGRAM_NUMBERS];
	int m_InUsedOutLineNumbers;
	int m_ExtLineNumbers;
	CButton m_ShowExtPortNumCorrespond;
	int SetExtCorrespondListBtnPos();
	afx_msg void OnBnClickedButtonShowExtList();
//	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
//	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	int SetExtCallStatusLed(int ExtNo, int LineNo, int Status);
	int m_SubPrgramGroupIndex;
	int InsertSubProgramGroup(PIPCEventHeader_T pConnectHeader, PIPCEventConnectBody_T pConnectBody);
	int ShowSubProgramGroup(int SubProgramIndex);
	BOOL CheckSubProgramIDExist(int SubProgramID);
	int m_ListFontHeight;
	int m_ListFontWidth;
	int m_OutStatustHeight;
	int m_OutStatustWidth;
	CFont* m_pOutStatustFont;
	// 大小要跟著 ADICTICallCenter.h 的 MyEnum(CTRL_ID_*) 走，該 enum 目前有
	// 7 個值（0~6）；加新分頁記得同時檢查這裡夠不夠大，不然會寫到陣列外面。
	CWnd* m_pWndModleDlg[7];
	int m_StatustTitleHeight;
	int m_StatustTitleWidth;
	CFont* m_pStatustTitleFont;
//	int ShowSubProgramPorts();
	int m_DisplayRectLeft;
	int m_DisplayRectRight;
	int m_GroupRectLeft;
	int m_GroupRectRight;
	int m_GroupRectTop;
	int m_GroupRectBottom;
	//CTabCtrl m_TabCtrl;
	CTabCtrl m_TabCtrl;
	afx_msg void OnTcnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	CStatic m_OutLineString;
	int m_OutLineBottomPos;
	CStatic m_ExtLineString;
	int m_ExtLineBottomPos;
	int m_OutLineNum;
	int m_ExtLineNum;
//	VPortStatus_T m_VPortStatus[VIRTUAL_PORT_NUMS];
//	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
//	afx_msg void OnChildActivate();
	CShowLinesStatusDlg m_ShowLinesStatusDlg;
	CPbxDlg m_PBXTypeUIDlg;
	CCallRecorderDlg m_CallRecorderTypeUIDlg;
	CVoiceRecorderDlg m_VoiceRecorderTypeUIDlg;
	COutLineCallStatusDlg m_OutLineCallStatusDlg;
	CSettingsDlg m_SettingsUIDlg;
	CWebConsoleDlg m_WebConsoleUIDlg;
	afx_msg void OnBnClickedOk();
	int m_BackEndConnect;
	int m_DataBase;
	CStatic m_Message;
	void ShowMessage(PCHAR pMessage);
	CFont* m_MessageFont;
	int m_MessageFontHeight;
	int m_MessageFontWidth;
};
