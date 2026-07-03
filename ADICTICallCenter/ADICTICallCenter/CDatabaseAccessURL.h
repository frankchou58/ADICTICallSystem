#pragma once

// CDatabaseAccessURL 命令目標
//
// 重寫說明：對外的公開介面（方法名稱、參數、輸出結構）完全不變，
// 呼叫端（ADICTICallCenterDlg / CallStateRecordEngine / WSServer /
// MachineServer / VirtualOutPortDBAccess 等 19 個檔案）不需要修改。
// 內部改為呼叫新版 ADICTICallSystem.API：
//   - 走 JSON Body（POST/PATCH），不再是 GET/POST query string 或
//     手刻 malloc+BCMParserJson 解析
//   - 需要先登入取得 Bearer Token，Token 存成 static 成員供全部
//     CDatabaseAccessURL 實例共用（呼叫端每次都是在函式內建立區域
//     變數，不是單一長駐物件，所以用 static 快取才能避免每次都要
//     重新登入）
//   - 因為部署環境沒有裝 IIS URL Rewrite，所有路徑一律透過
//     ?route= 查詢參數呼叫，詳見 doc/README.md「沒有 URL Rewrite
//     時的路由方式」
//   - Create*Table 系列方法對應的端點在新 API 已經移除（改成部署時
//     執行一次的 CLI 腳本 bin/setup.php），這些方法保留簽章但直接
//     回傳成功，不再發出任何 HTTP 請求
//
// 服務帳號設定：需要在登錄檔設定 SystemSetting\ApiEmployeeNo /
// ApiPassword（沒有 GUI 可以編輯，请參考 doc 內的部署說明用 reg add
// 設定，或呼叫新版 API 的 POST /employees 先建立一個帳號）。

#include "ADICTICallCenter.h"
#include "CallStateRecordEngine.h"
#include "ErrorCode.h"
#include "include/json/json.h"

class CDatabaseAccessURL : public CInternetSession
{
private:
	// 目前所有 CDatabaseAccessURL 執行個體共用的 Bearer Token 快取。
	// 呼叫端習慣在函式內建立區域變數（見 VirtualOutPortDBAccess.cpp
	// 等），不是單一長駐物件，所以登入狀態必須放在 static 成員，
	// 否則每次呼叫都要重新登入一次。
	static CString s_ApiToken;

	int Login();
	int DoUpdateDBValue(PCHAR pURLCommand, PCHAR pMessage);
	// pVerb: "GET"/"POST"/"PATCH"/"DELETE"
	// pRoute: 乾淨路徑，例如 "/machines" 或 "/outlines/5"（不含查詢字串）
	// pExtraQuery: 要附加在 route 後面的查詢字串，需自行帶開頭的 '&'，例如
	//              "&machineType=1&limit=50"；沒有就傳 ""
	// pJsonBody: POST/PATCH 要送出的 JSON 字串；GET/DELETE 傳 nullptr
	int MakeHttpConnection(LPCSTR pVerb, LPCSTR pRoute, LPCSTR pExtraQuery, LPCSTR pJsonBody, CString* pRet);
	int MakeHttpConnectionRaw(LPCSTR pVerb, LPCSTR pRoute, LPCSTR pExtraQuery, LPCSTR pJsonBody, CString* pRet, BOOL bAttachAuth);
	bool ParseEnvelope(const CString& Response, Json::Value& outData);
	// 從 {success:false, message:"..."} 這種錯誤回應裡取出訊息文字，寫進
	// pMessage（呼叫端自備的緩衝區）。2026-07-03 新增，搭配 MakeHttpConnectionRaw
	// 現在不管成功失敗都會讀回應內容，讓 SetMachineOutPortsByMachineID 等
	// 方法可以把後端驗證錯誤的具體原因（例如「合併機碼已被別的類型使用」）
	// 回傳給呼叫端顯示，不再只有一個看不出原因的錯誤碼。
	void ExtractErrorMessage(const CString& Response, PCHAR pMessage, int BufferSize);
	CString fixjsonchinese(std::string inputstringbuf);
	static std::string JsonEscape(LPCSTR pValue);
	// 新版 outline_ports/extline_ports 是「一個實體埠一列」，vport 只是
	// 該列上的一個欄位，不再是固定 1~240 的池子。這幾個輔助函式處理
	// 「用(machineType,machineNo,phyPort)自然鍵找到那一列的 id」，以及
	// 「一個 vport 可能同時被 1~3 種類型共用，設定通話狀態時要一次全部
	// 更新」這兩種新舊介面轉換情境。
	int FindOutlinePortId(int MachineType, int MachineID, int PhyPort, int* pId);
	int FindExtlinePortId(int MachineType, int MachineID, int PhyPort, int* pId);
	int PatchAllOutlinePortsByVport(int VPort, LPCSTR pJsonBody);
public:
	// 強制直連（不吃系統 Proxy 設定）：後端一律是固定的內部/本機伺服器，
	// 用 PRE_CONFIG_INTERNET_ACCESS 會讓 WinINet 套用 IE 的 Proxy 設定，
	// 如果那組 Proxy 連不到 localhost/內網，會又慢（等 Proxy 逾時）又失敗。
	CDatabaseAccessURL(LPCTSTR pstrAgent = nullptr,
		DWORD dwContext = 1,
		DWORD dwAccessType = INTERNET_OPEN_TYPE_DIRECT,
		LPCTSTR pstrProxyName = nullptr,
		LPCTSTR pstrProxyBypass = nullptr,
		DWORD dwFlags = 0);
	virtual ~CDatabaseAccessURL();
	int BCMParserJson(PCHAR pJason, PCHAR pStatement, void* pResult, int Type);
	int CheckBackEndURL();
	int CheckDBTables();
	int CreateDBTable(PCHAR pTableName);
	int CreateDBOperatorTable();
	int CreateDBMachineTable();
	int CreateDBOutLineTable();
	int CreateDBExtLineTable();
	int CreateDBRecordTable();
	int CreateDBAdminTable();
	int GetOutLines(POutPort_T pOutLine);
	int GetExtLines(PExtPort_T pExtLine);
	int SetExtNumber(int ExtPort, int MachineID, int ExtensionNo, PCHAR pMessage);
	int AddExtPort(int ExtPort, PCHAR pMessage);
	int AddOutPort(int OutPort, PCHAR pMessage);
	int GetExtVPortByExtNum(int MachineID, int ExtNum, PExtData_T pExtData, PCHAR pMessage);
	int SetOutPortInUsed(int OutPort, BOOL bInUsed, PCHAR pMessage);
	int GetMachineInfo(int MachineType, int MachineID, PCHAR pAlias, int* pOutPort, int* pExtPort, PCHAR pMessage);
	// 一次拿到全部 3 種類型 x SUBPROGRAM_NUMBERS 個機碼的外線/內線埠數量
	// （GET /machines 不帶篩選條件就是回傳全部列），取代逐一呼叫
	// GetMachineInfo() 30 次的寫法。每個陣列大小都要是 SUBPROGRAM_NUMBERS，
	// 索引 i 對應機碼 i+1。
	int GetAllMachinesPortCounts(int* pPBXOutPort, int* pPBXExtPort,
		int* pCallerIDBoxOutPort, int* pCallerIDBoxExtPort,
		int* pVoiceCardOutPort, int* pVoiceCardExtPort);
	// 同上，但輸出對象是 CPbxDlg/CCallRecorderDlg/CVoiceRecorderDlg 各自維護的
	// SubProgramGroup_T 陣列（多了 Alias），取代這三個檔案的 LoadMachineData()
	// 原本各自逐一呼叫 GetMachineInfo() 30 次的寫法。每個陣列大小都要是
	// SUBPROGRAM_NUMBERS。只會寫入 Alias/OutPortNum/ExtPortNum 這三個欄位，
	// 不動陣列裡其他呼叫端已經設定好的欄位（例如 pGroupBox 之類的 UI 指標）。
	int GetAllMachinesSubProgramInfo(PSubProgramGroup_T pPBXGroup,
		PSubProgramGroup_T pCallerIDBoxGroup,
		PSubProgramGroup_T pVoiceCardGroup);
	// pFailedPortCount（選填）：新數量涵蓋範圍裡有幾個實體埠因為 dbo.outline/
	// dbo.extline 池子裡已經沒有真正閒置的列可以認領而指派失敗——這個數字
	// 現在由後端在同一次 PATCH 裡直接算好回傳，呼叫端不用再自己另外發一輪
	// HTTP 請求去重新核對全部 240 個虛擬埠（那樣做過，一次設定要等好幾百次
	// 同步 API 呼叫跑完，非常慢，2026-07 拿掉了）。
	int SetMachineOutPortsByMachineID(int MachineType, int MachineID, int OutPort, PCHAR pMessage, int* pFailedPortCount = NULL);
	int GetMachineOutPortsByMachineID(int MachineType, int MachineID, int* pOutPort, PCHAR pMessage);
	int SetMachineExtPortsByMachineID(int MachineType, int MachineID, int ExtPort, PCHAR pMessage, int* pFailedPortCount = NULL);
	int SetMachineAliasByMachineID(int MachineType, int MachineID, PCHAR pAlias, PCHAR pMessage);
	int SetMachineConnected(int MachineType, int MachineID, PCHAR pIPAddr, BOOL bConnected, PCHAR pMessage);
	int AssignPhyOutPortInfo(int VPort, int MachineType, int MachineID, int PhyPort, PCHAR pMessage);
	int AssignPhyExtPortInfo(int VPort, int MachineID, int PhyPort, PCHAR pMessage);
	// 取消指派：跟 AssignPhy*PortInfo 相反，把這個實體埠跟機碼的關聯清掉
	// （不影響虛擬線路編號本身）。2026-07-03 新增：客戶資料庫的
	// outline/extline 是固定 240 筆的池子，縮減外線/內線數量時沒辦法像
	// 舊設計那樣直接刪列，只能明確呼叫這個來取消指派，否則畫面會一直
	// 顯示已經不該存在的舊埠號/虛擬編號。找不到這個實體埠（本來就沒
	// 指派過）時視為成功，語意跟 AssignPhy*PortInfo 一樣是 idempotent。
	int UnassignPhyOutPortInfo(int MachineType, int MachineID, int PhyPort, PCHAR pMessage);
	int UnassignPhyExtPortInfo(int MachineID, int PhyPort, PCHAR pMessage);
	// 查詢資料庫裡目前實際指派給這個機碼(該類型)的所有實體埠編號——縮減
	// 外線/內線數量時，舊的埠數只存在資料庫裡，程式這邊的靜態陣列拿不到，
	// 只能實際查一次才知道哪些埠已經超出新數量、需要呼叫 UnassignPhy*PortInfo。
	// pPhyPorts 陣列大小至少要 VIRTUAL_PORT_NUMS。
	int GetWiredOutPhyPorts(int MachineType, int MachineID, int* pPhyPorts, int MaxCount, int* pActualCount);
	int GetWiredExtPhyPorts(int MachineID, int* pPhyPorts, int MaxCount, int* pActualCount);
	int GetOutVPort(int MachineType, int MachineID, int PhyPort, int* pOutVPort, PCHAR pMessage);
	int AddInternalCallRecord(int MachineID, int StartTime, int FromExtNum, int ToExtNum, int* pDBIndexID, PCHAR pMessage);
	int AddInternalCallRecordOneTime(PCallBlock_T pCallBlock, PCHAR pMessage);
	int AddExternalCallRecord(int MachineID, int StartTime, int OutVPort, int PhyOutPort, int* pDBIndexID, PCHAR pMessage);
	int AddExternalCallRecordOneTime(PCallBlock_T pCallBlock, PCHAR pMessage);
	int UpdateCallRecordTel(int DBIndexID, PCHAR pTelNo, PCHAR pMessage);
	int UpdateCallRecordTalkTime(int DBIndexID, UINT32 Timestamp, PCHAR pMessage);
	int UpdateCallRecordRingTimes(int DBIndexID, UINT32 RingTimes, PCHAR pMessage);
	int UpdateCallRecordEndTime(int DBIndexID, UINT32 Timestamp, PCHAR pMessage);
	int UpdateCallRecordTalkDuration(int DBIndexID, UINT32 Timestamp, PCHAR pMessage);
	int UpdateCallRecordExtNum(int DBIndexID, UINT32 ExtNum, PCHAR pMessage);
	int UpdateCallRecordFromExtNum(int DBIndexID, UINT32 FromExtNum, PCHAR pMessage);
	int UpdateCallRecordToExtNum(int DBIndexID, UINT32 ToExtNum, PCHAR pMessage);
	int UpdateCallRecordCallType(int DBIndexID, UINT32 CallType, PCHAR pMessage);
	int UpdateCallRecordVoiceRecFileUUID(int DBIndexID, PCHAR pVoiceRecFileUUID, PCHAR pMessage);
	int AddOperator(int EmployeeId, PCHAR pPassword, PCHAR pMessage);
	int GetOperatorInfo(PCHAR OperatorUUID, POperatorInfo_T pOperatorInfo);
	int GetCustomerInfo(PCHAR pTelNo, PCustomerInfo_T pOperatorInfo);
	int SetOutVPortCallStatusInfo(INT OutVPortNo, POutVPortCallStatus_T pOutVPortCallStatus);
};

