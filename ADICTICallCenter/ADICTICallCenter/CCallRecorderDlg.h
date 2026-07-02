#pragma once
#include "CDatabaseAccessURL.h"
#include "VirtualOutPortDBAccess.h"

// CCallRecorderDlg 對話方塊

class CCallRecorderDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCallRecorderDlg)

public:
	CCallRecorderDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~CCallRecorderDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CCallRecorderDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	//afx_msg void OnEnChangeEdit2();
	afx_msg void OnBnClickedOk();
	afx_msg void OnEditSubProgramListItem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnColorSubProgramList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickSubProgramListItem(NMHDR* pNMHDR, LRESULT* pResult);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbnSelchangeComboOutLine1();
	afx_msg void OnCbnSelchangeComboPortsSelect();
	afx_msg void OnCbnSelchangeComboPotportsAssign();

	CStatic m_SubProgramListTitle;
	int m_ListTitleFontHeight;
	int m_ListTitleFontWidth;
	int m_TitleFontHeight;
	int m_TitleFontWidth;
	CFont* m_pTitleFont;
	CFont* m_pFont;
	int m_FontHeight;
	CFont* m_pListTitleFont;
	CWnd* m_pParent;
	int m_ListFontHeight;
	int m_ListFontWidth;
	CFont* m_pListFont;
	int m_EditFontHeight;
	int m_EditFontWidth;
	int m_EditListNums;
	CFont* m_pEditFont;
	int ShowSubProgramList();
	SubProgramGroup_T m_PBXSubProgramGroup[SUBPROGRAM_NUMBERS];
	SubProgramGroup_T m_CallerIDSubProgramGroup[SUBPROGRAM_NUMBERS];
	SubProgramGroup_T m_VoiceCardSubProgramGroup[SUBPROGRAM_NUMBERS];
	int m_MachineNums;
	CListCtrl m_ListSubProgram;
	CComboBox m_ComboListPortsSelect;
	int m_Item;
	int m_Subitem;
	OutPort_T m_OutPorts[VIRTUAL_PORT_NUMS];
	CEdit m_EditMachineAlias;
	CDatabaseAccessURL m_DatabaseAccessURL;
	int m_OutLineNum;
	int m_OldOutLineNum;
	CStatic* m_pEditOutLineTitle;
	int SetOutLineFont();
	int ShowOutLineIcon();
	CFont* m_pOutStatustFont;
	int m_OutLineFontHeight;
	int m_OutLineFontWidth;
	int m_OutLineFontHeightGap;
	void OnDblClkOutPortListItem();
	void OnLbdOutPortListItem();
	int m_PrevCtrlID;
	CComboBox m_AssignOutPortList;
	BOOL m_IsInEdit;
	BOOL m_IsInEditSelectPort;
	int LoadMachineData();
	void SetPortAssignList(int MachineID);
	int CheckOutVPortSetting(int ThisMachineIndex);
	int RedrawIcons();
	void MachineLogin(int MachineID, PCHAR pIPAddr, int SWVer);
	void MachineLogout(int MachineID);
	int InitPorts();
};
