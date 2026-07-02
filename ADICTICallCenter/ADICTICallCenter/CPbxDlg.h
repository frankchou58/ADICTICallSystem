#pragma once
#include "CDatabaseAccessURL.h"
#include "VirtualOutPortDBAccess.h"


// CPbxDlg 對話方塊

class CPbxDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CPbxDlg)

public:
	CPbxDlg(CWnd* pParent = nullptr);   // 標準建構函式
	virtual ~CPbxDlg();

// 對話方塊資料
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CPbxDlg };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支援

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	virtual BOOL OnInitDialog();
	afx_msg void OnColorSubProgramList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEditSubProgramListItem(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClickSubProgramListItem(NMHDR* pNMHDR, LRESULT* pResult);

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
	int m_ExtLineFontHeight;
	int m_ExtLineFontWidth;
	int m_ExtLineFontHeightGap;
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
	ExtPort_T m_ExtPorts[VIRTUAL_PORT_NUMS];
	CEdit m_EditMachineAlias;
	CDatabaseAccessURL m_DatabaseAccessURL;
	//int m_OutLineNum;
	int m_ExtLineNum;
	int m_ExtOutLineNum;
	//CStatic* m_pEditOutLineTitle;
	CStatic* m_pEditExtLineTitle;
	CFont* m_pOutStatustFont;
	int m_PrevCtrlID;
	CComboBox m_AssignOutPortList;
	BOOL m_IsInEdit;
	BOOL m_IsInEditSelectPort;
	int SetExtLineFont();
	int ShowExtLineIcon();
	void OnDblClkOutPortListItem();
	void OnLbdOutPortListItem();
	int LoadMachineData();
	void SetPortAssignList(int MachineID);
	int CheckOutVPortSetting(int ThisMachineIDIndex);
	int RedrawIcons();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnCbnSelchangeComboPortsSelect();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void MachineLogin(int MachineID, PCHAR pIPAddr, int SWVer);
	void MachineLogout(int MachineID);
	int InitPorts();
};
