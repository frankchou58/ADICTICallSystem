
// ADICTICallCenter.h: PROJECT_NAME 應用程式的主要標頭檔
//

#pragma once

#ifndef __AFXWIN_H__
	#error "對 PCH 在包含此檔案前先包含 'pch.h'"
#endif

#include "resource.h"		// 主要符號
// CADICTICallCenterApp:
// 查看 ADICTICallCenter.cpp 以了解此類別的實作
//

class CADICTICallCenterApp : public CWinApp
{
public:
	CADICTICallCenterApp();

// 覆寫
public:
	virtual BOOL InitInstance();

// 程式碼實作

	DECLARE_MESSAGE_MAP()
};

extern CADICTICallCenterApp theApp;
#define LINES_NUM_ARRAY	8
#define SUBPROGRAM_NUMBERS	10
#define VIRTUAL_PORT_NUMS	240
#define LINE_NUMBERS	64
#define EXT_NUMBERS		64

enum
{
	LINK_STATUS_NONE = 0,
	LINK_STATUS_LOGIN,
	LINK_STATUS_LOGOUT,
};

typedef struct RGB_Tag
{
	UCHAR R;
	UCHAR G;
	UCHAR B;
}RGB_T, * PRGB_T;

enum
{
	MACHINE_TYPE_PBX = 1,
	MACHINE_TYPE_CALLER_ID_BOX,
	MACHINE_TYPE_VOICE_CARD,
};

enum MyEnum
{
	CTRL_ID_SHOW_PORT_STATUS = 0,
	//CTRL_ID_OUTLINE_CALL_STATUS,
	CTRL_ID_PBX_TYPE_UI,
	CTRL_ID_CALL_RECORDER_TYPE_UI,
	CTRL_ID_VOICE_RECORDER_TYPE_UI,
	CTRL_ID_WEB_CONSOLE_UI,
	CTRL_ID_VOICE_SETTING_UI,
};

enum ListSubProgramFields
{
	//ID_LIST_SUB_PROGRAM_TYPE = 0,
	ID_LIST_SUB_PROGRAM_ID = 0,
	ID_LIST_SUB_PROGRAM_ALIAS,
	ID_LIST_SUB_PROGRAM_OUT_PORTS,
	ID_LIST_SUB_PROGRAM_EXT_PORTS,
	ID_LIST_SUB_PROGRAM_FW_VER,
	ID_LIST_SUB_PROGRAM_IP_ADDR,
	ID_LIST_SUB_PROGRAM_LINK_STATUS,
};

enum 
{
	STATUS_NO_USE = 0,
	STATUS_CALL_IN,
	STATUS_CALL_OUT,
	STATUS_CALL_TALKING,
	STATUS_CALL_OFF,
};

typedef enum CallType_Tag
{
	TYPE_CALL_IN = 0,
	TYPE_CALL_OUT,
	TYPE_INTERNAL_CALL,
} CallType_T;

typedef struct ExtLineEdit_Tag
{
	CStatic* pExtLineNo;
	CEdit* pExtEditBtn;
	int DlgCtrlID;
	//CString ExtLineValue;
}ExtLineEdit_T, * PExtLineEdit_T;

typedef struct OutLineEdit_Tag
{
	CStatic* pOutLineNo;
	CEdit* pOutEditBtn;
	//CComboBox* pOutEditBtn;
	int DlgCtrlID;
	//CString ExtLineValue;
}OutLineEdit_T, * POutLineEdit_T;

typedef struct OutPort_Tag
{
	BOOL IsInUsed;
	UINT ID;
	//UINT OutPort;
	UINT VPortNo;
	CHAR OperatorUUID[33];
	CEdit* pEditExt;
	CStatic* pEditOutPortText;
	OutLineEdit_T OutLineBtn;
	//int SubProgramID;
	RECT Rect;
	int MachineID;
	int MachineTypeAPhyPort;
	int MachineTypeBPhyPort;
	int MachineTypeCPhyPort;
} OutPort_T, * POutPort_T;

typedef struct ExtPort_Tag
{
	BOOL IsInUsed;
	UINT ID;
	UINT ExtNo; 
	UINT PortNo;
	CHAR OperatorUUID[33];
	CEdit* pEditExt;
	CStatic* pEditExtPortText;
	ExtLineEdit_T ExtLineBtn;
	int MachineID;
	RECT Rect;
} ExtPort_T, * PExtPort_T;

typedef struct SubProgramGroup_Tag
{
	//BOOL IsInUsed;
	CHAR Alias[500];
	int LinkStatus;
	CHAR Type;
	int FWVer;
	CHAR IPAddr[16];
	int SubProgramID;
	CButton* pGroupBox;
	int OutPortNum;
	//OutPort_T OutPorts[64];
	int ExtPortNum;
	//ExtPort_T ExtPorts[64];
	//BOOL IsSubProgramIDExtsied;
	//CStatic* pOutPortString;
	//CStatic* pExtPortString;
}SubProgramGroup_T, *PSubProgramGroup_T;

typedef struct VPortStatus_Tag
{
	CHAR VPortNoStr[10];
	CStatic* pVPortNoString;
	CStatic* pVPortIcon;
	int IconCtlID;
	int CallState;
}VPortStatus_T, *PVPortStatus_T;

typedef struct OperatorInfo_Tag
{
	int ID;
	CHAR UUID[33];
	int EmployeeID;
	CHAR Name[50];
	int MachineID;
	UINT32 LoginTimestamp;
	UINT32 LogoutTimestamp;
	UINT16 Level;
	int ExtNum;
} OperatorInfo_T, *POperatorInfo_T;

typedef struct CustomerInfo_Tag
{
	int ID;
	int CustomerID;
	CHAR Name[50];
	CHAR UUID[33];
	int Gender;
	CHAR Birthday[11];
	CHAR TelNo[50];
	CHAR eMail[100];
	CHAR County[50];
	CHAR Townships[50];
	CHAR Address[200];
	int InBlackList;
} CustomerInfo_T, * PCustomerInfo_T;

typedef struct OutVPortCallStatus_Tag
{
	INT Status;
	INT Type;
	PCHAR pTelNo;
	INT ExtNum;
	INT PhyPortNo;
}OutVPortCallStatus_T, *POutVPortCallStatus_T;