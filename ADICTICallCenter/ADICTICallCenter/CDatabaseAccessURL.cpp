// CDatabaseAccessURL.cpp: 實作檔案
//
// 重寫說明（改用 ADICTICallSystem.API 新版 REST API）：
//   - 對外公開介面完全不變，19 個呼叫端檔案不需要修改。
//   - 內部改成呼叫新版 API：JSON Body、Bearer Token 驗證、
//     ?route= 查詢字串（因部署環境沒有 IIS URL Rewrite 模組）。
//   - 拿掉舊版手刻 malloc + BCMParserJson 的 JSON 解析方式，改用
//     專案已經內附的 jsoncpp（Json::Reader / Json::FastWriter）。
//   - Token 用 static 成員快取，因為呼叫端習慣在函式內建立區域變數
//     （不是單一長駐物件），若不快取每次呼叫都要重新登入。
//   - 服務帳號設定放在登錄檔 SystemSetting\ApiEmployeeNo /
//     ApiPassword（跟 DatabaseBackEndURL 用同一套 GetProfileString
//     機制），沒有對應的設定畫面，需要用 reg add 或其他方式寫入。
//   - Create*Table 系列端點在新 API 已移除（改成部署時執行一次的
//     bin/setup.php），這些方法保留但直接回傳成功，不再送出任何請求。
//
// 重要：此檔案未經編譯驗證（環境沒有 MSBuild/Visual Studio），部署前
// 務必在 Visual Studio 完整建置並實機測試過一輪，才能上線取代舊版。

#include "pch.h"
#include "CDatabaseAccessURL.h"
#include <map>

// CDatabaseAccessURL
typedef struct Fields_Tag
{
	CHAR Field[500];
} Fields_T, * PFields_T;

CString CDatabaseAccessURL::s_ApiToken;

int AsciiToInt(UCHAR Char)
{
	int Value;

	if (Char >= '0' && Char <= '9')
		Value = Char - '0';
	else
		Value = Char - 'a' + 10;

	return Value;
}

int AnsiToUTF8(PCHAR tchStr, char* chRtn)
{
	int erg = 0;
	int size = MultiByteToWideChar(CP_ACP, 0, tchStr, -1, NULL, 0);
	wchar_t* pBuffer = (wchar_t*)malloc(size * sizeof(wchar_t));
	erg = MultiByteToWideChar(CP_ACP, 0, tchStr, -1, pBuffer, size * sizeof(wchar_t)); // ANSI to UNICODE
	int wLen = WideCharToMultiByte(CP_UTF8, 0, pBuffer, -1, NULL, 0, NULL, NULL);
	erg = WideCharToMultiByte(CP_UTF8, 0, pBuffer, -1, chRtn, wLen, NULL, NULL);     // UNICODE to UTF-8
	free(pBuffer);

	return erg;
}

void UnicodeToAnsi(wchar_t* pStockName, CHAR* resp)
{
	int wLen = 2;
	memset(resp, 0x00, 50);
	WideCharToMultiByte(CP_ACP, 0, pStockName, -1, resp, wLen, NULL, NULL);     // UNICODE to ANSI
}

int ParserCharacters(PCHAR StartPtr, void* pResult)
{
	INT RightBracketNum = 0;
	INT Ret = 0;
	INT SegmentIndex = 0;
	BOOL HaveMatrix = FALSE;
	INT ParamIndex = 0;
	CHAR Segment[500];
	PFields_T pField;
	INT Len;
	CHAR LastChar;
	INT DoubleQuotationNum = 0;

	ParamIndex = 0;
	pField = (PFields_T)pResult;
	Len = strlen(StartPtr);
	LastChar = StartPtr[Len - 1];
	for (int i = 0; i < Len; i++)
	{
		if (StartPtr[i] == '[')
			RightBracketNum++;
		if ((RightBracketNum == 1 && HaveMatrix == FALSE))
		{
			if (StartPtr[i] != '"' && StartPtr[i] != ',' && StartPtr[i] != '[' && StartPtr[i] != ']')
				Segment[SegmentIndex++] = StartPtr[i];
			else if (StartPtr[i] == ',' || (LastChar == ']' && i == Len - 1))
			{
				if (pResult)
				{
					memset(pField, 0, 500);
					memcpy(pField, Segment, SegmentIndex);
					pField++;
				}
				SegmentIndex = 0;
				ParamIndex++;
			}
		}
		else if (RightBracketNum == 0)
		{
			if (StartPtr[i] == '"')
				DoubleQuotationNum++;
			if (DoubleQuotationNum == 1)
			{
				if (StartPtr[i] != '"' && StartPtr[i] != ']' && StartPtr[i] != '[' && StartPtr[i] != '}')
					Segment[SegmentIndex++] = StartPtr[i];
			}
			else if (DoubleQuotationNum == 2 || DoubleQuotationNum == 0)
			{
				if (pResult)
				{
					memset(pField, 0, 500);
					memcpy(pField, Segment, SegmentIndex);
				}
				SegmentIndex = 0;
				ParamIndex++;
				break;
			}
		}
		else
		{
			Segment[SegmentIndex++] = StartPtr[i];
			HaveMatrix = TRUE;
			if (StartPtr[i] == ']' && RightBracketNum == 2)
			{
				HaveMatrix = FALSE;
			}
		}
		if (StartPtr[i] == ']')
			RightBracketNum--;
	}
	return ParamIndex;
}

BOOL HaveBracketSign(PCHAR pField)
{
	BOOL HaveBracket = FALSE;

	if (strstr(pField, "["))
		HaveBracket = TRUE;

	return HaveBracket;
}

#define BCM_PARSER_TYPE_INT		1
#define BCM_PARSER_TYPE_CHAR	2
#define BCM_PARSER_TYPE_BOOLEAN	3

CDatabaseAccessURL::CDatabaseAccessURL(LPCTSTR pstrAgent /*= nullptr*/,
		DWORD dwContext /*= 1*/,
		DWORD dwAccessType /*= INTERNET_OPEN_TYPE_DIRECT*/,
		LPCTSTR pstrProxyName /*= nullptr*/,
		LPCTSTR pstrProxyBypass /*= nullptr*/,
		DWORD dwFlags /*= 0*/)
		: CInternetSession (pstrAgent, dwContext, dwAccessType, pstrProxyName,
							pstrProxyBypass, dwFlags)
{
}

CDatabaseAccessURL::~CDatabaseAccessURL()
{
}

CString CDatabaseAccessURL::fixjsonchinese(std::string inputstringbuf)
{
	size_t len = strlen(inputstringbuf.c_str()) + 1;
	char outch[MAX_PATH];
	WCHAR* wChar = new WCHAR[len];
	wChar[0] = 0;
	MultiByteToWideChar(CP_UTF8, 0, inputstringbuf.c_str(), len, wChar, len);
	WideCharToMultiByte(CP_ACP, 0, wChar, len, outch, len, 0, 0);
	delete[] wChar;
	char* pchar = (char*)outch;
	len = strlen(pchar) + 1;
	WCHAR outName[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, pchar, len, outName, len);

	return (CString)outName;
}

int CDatabaseAccessURL::BCMParserJson(PCHAR pJason, PCHAR pStatement, void* pResult, int Type)
{
	PCHAR StartPtr;
	PCHAR EndPtr;
	int Len = 0;
	CHAR Param[500];
	CHAR Statement[50];
	int Value;

	sprintf_s(Statement, "\"%s\":", pStatement);
	StartPtr = strstr(pJason, Statement);
	if (StartPtr)
	{
		StartPtr += strlen(Statement);
		switch (Type)
		{
		case BCM_PARSER_TYPE_INT:
			StartPtr++;
			EndPtr = strstr(StartPtr, ",");
			if (EndPtr)
			{
				Len = EndPtr - StartPtr - 1;
				strncpy_s(Param, 500, StartPtr, Len);
				Value = atoi(Param);
				memcpy(pResult, &Value, sizeof(int));
			}
			break;
		case BCM_PARSER_TYPE_BOOLEAN:
			EndPtr = strstr(StartPtr, ",");
			if (EndPtr)
			{
				Len = EndPtr - StartPtr;
				strncpy_s(Param, 500, StartPtr, Len);
				if (!strncmp(Param, "true", 4))
					memcpy(pResult, Param, 4);
				else if (!strncmp(Param, "false", 5))
					memcpy(pResult, Param, 5);
				else
					memcpy(pResult, "NULL", 4);
			}
			break;
		case BCM_PARSER_TYPE_CHAR:
			Len = ParserCharacters(StartPtr, pResult);
			break;
		}
	}

	return Len;
}

// ============================================================
// 新版 HTTP 層
// ============================================================

std::string CDatabaseAccessURL::JsonEscape(LPCSTR pValue)
{
	// 保留給呼叫端需要手動組字串時使用；主要都是直接建立 Json::Value
	// 再用 Json::FastWriter 序列化，交給 jsoncpp 處理跳脫字元。
	Json::Value v(pValue ? pValue : "");
	Json::FastWriter writer;
	std::string s = writer.write(v);
	return s;
}

bool CDatabaseAccessURL::ParseEnvelope(const CString& Response, Json::Value& outData)
{
	Json::Reader reader;
	Json::Value root;
	std::string ResponseData = CT2A(Response.GetString());
	if (!reader.parse(ResponseData, root))
		return false;

	bool success = root.get("success", false).asBool();
	outData = root["data"];
	return success;
}

// 從錯誤回應（{success:false, message:"..."}）裡取出訊息文字，寫進
// pMessage（呼叫端自備的緩衝區，大小 BufferSize）。解析失敗或本來就是
// 空的就寫入空字串，呼叫端可以用「pMessage 是否為空字串」判斷有沒有
// 抓到訊息。
void CDatabaseAccessURL::ExtractErrorMessage(const CString& Response, PCHAR pMessage, int BufferSize)
{
	if (!pMessage || BufferSize <= 0)
		return;
	pMessage[0] = '\0';

	if (Response.IsEmpty())
		return;

	Json::Reader reader;
	Json::Value root;
	std::string ResponseData = CT2A(Response.GetString());
	if (!reader.parse(ResponseData, root))
		return;

	std::string Message = root.get("message", "").asString();
	if (Message.empty())
		return;

	CString FixedMessage = fixjsonchinese(Message);
	strcpy_s(pMessage, BufferSize, CT2A(FixedMessage.GetString()));
}

int CDatabaseAccessURL::Login()
{
	CString EmployeeNo = AfxGetApp()->GetProfileString("SystemSetting", "ApiEmployeeNo", "");
	CString Password = AfxGetApp()->GetProfileString("SystemSetting", "ApiPassword", "");
	if (EmployeeNo.IsEmpty() || Password.IsEmpty())
		return ERROR_CODE_NO_API_CREDENTIAL;

	Json::Value body;
	body["employeeNo"] = (LPCSTR)CT2A(EmployeeNo.GetString());
	body["password"] = (LPCSTR)CT2A(Password.GetString());
	std::string JsonBody = Json::FastWriter().write(body);

	CString Response;
	int Ret = MakeHttpConnectionRaw("POST", "/auth/login", "", JsonBody.c_str(), &Response, FALSE);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_UNAUTHORIZED;

	s_ApiToken = data["token"].asCString();
	return ERROR_CODE_SUCCESS;
}

// pRoute 只放乾淨路徑（例如 "/machines/1/2"），額外的查詢參數(篩選條件)
// 透過 pExtraQuery 帶入，格式需自行加開頭的 '&'，例如 "&machineType=1"。
// 這樣才能讓 route 本身維持乾淨路徑，篩選參數則是各自獨立的查詢參數，
// 對應新版 API 的 Router 只把 ?route= 的值當路徑、其餘 query 照常解析。
int CDatabaseAccessURL::MakeHttpConnectionRaw(LPCSTR pVerb, LPCSTR pRoute, LPCSTR pExtraQuery, LPCSTR pJsonBody, CString* pRet, BOOL bAttachAuth)
{
	int Ret = ERROR_CODE_SUCCESS;
	CHttpConnection* pHttp = NULL;
	CHttpFile* file = NULL;

	CString DatabaseBackEndURL = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndURL", "localhost");
	CString DatabaseBackEndURLPort = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndURLPort", "8841");
	// 新版 API 的網站根目錄與轉接進入點，預設對應本機部署時建立的
	// D:\ADICTI\Server\ADICTICallSystem.API\index.php 轉接檔。
	CString DatabaseBackEndBasePath = AfxGetApp()->GetProfileString("SystemSetting", "DatabaseBackEndBasePath", "/ADICTICallSystem.API/index.php");

	if (DatabaseBackEndURL.IsEmpty() || DatabaseBackEndURLPort.IsEmpty())
		return ERROR_CODE_NO_BACK_END_URI;

	CString RequestPath;
	RequestPath.Format("%s?route=%s%s", (LPCTSTR)DatabaseBackEndBasePath, pRoute, pExtraQuery ? pExtraQuery : "");

	CString Headers = "Content-Type: application/json\r\n";
	if (bAttachAuth && !s_ApiToken.IsEmpty())
	{
		CString AuthHeader;
		AuthHeader.Format("Authorization: Bearer %s\r\n", (LPCTSTR)s_ApiToken);
		Headers += AuthHeader;
	}

	try
	{
		pHttp = GetHttpConnection(DatabaseBackEndURL.GetBuffer(), (INTERNET_PORT)atoi(DatabaseBackEndURLPort.GetBuffer()));
		DWORD HttpRequestFlags = INTERNET_FLAG_EXISTING_CONNECT | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE;

		file = pHttp->OpenRequest(pVerb, RequestPath, NULL, 1, NULL, (LPCTSTR)"1.1", HttpRequestFlags);

		if (pJsonBody && strlen(pJsonBody) > 0)
		{
			file->SendRequest(Headers, Headers.GetLength(), (LPVOID)pJsonBody, (DWORD)strlen(pJsonBody));
		}
		else
		{
			file->SendRequest(Headers, Headers.GetLength());
		}

		DWORD statusCode = 0;
		file->QueryInfoStatusCode(statusCode);

		// 不管狀態碼是不是 2xx 都把回應內容讀出來——這支 API 的錯誤回應
		// 一律是 {success:false, message:"..."} 這種 JSON，之前只有成功
		// 才讀，導致 422/409 這類「請求本身有問題」的錯誤（例如 2026-07-03
		// 新增的「同一個機碼同時間只能有一種類型」驗證）完全沒有訊息可以
		// 顯示給使用者，只留下一個不知道為什麼失敗的錯誤碼。
		if (pRet)
			file->ReadString(*pRet);

		if (statusCode == HTTP_STATUS_OK || statusCode == 201 /* Created */)
		{
			Ret = ERROR_CODE_SUCCESS;
		}
		else if (statusCode == HTTP_STATUS_DENIED /* 401 */)
		{
			Ret = ERROR_CODE_UNAUTHORIZED;
		}
		else if (statusCode == HTTP_STATUS_NOT_FOUND)
		{
			Ret = ERROR_CODE_HTTP_NOT_FOUND;
		}
		else
		{
			Ret = ERROR_CODE_HTTP_ERROR;
		}
		file->Close();
	}
	catch (CInternetException* ex)
	{
		TCHAR error[1024];
		ex->GetErrorMessage(error, 1024);
		ex->Delete();
		Ret = ERROR_CODE_HTTP_ERROR;
	}

	if (file)
		delete file;
	if (pHttp)
		delete pHttp;

	return Ret;
}

// 對外實際使用的入口：自動處理登入與 401 後的重新登入重試一次。
int CDatabaseAccessURL::MakeHttpConnection(LPCSTR pVerb, LPCSTR pRoute, LPCSTR pExtraQuery, LPCSTR pJsonBody, CString* pRet)
{
	if (s_ApiToken.IsEmpty())
	{
		int LoginRet = Login();
		if (LoginRet != ERROR_CODE_SUCCESS)
			return LoginRet;
	}

	int Ret = MakeHttpConnectionRaw(pVerb, pRoute, pExtraQuery, pJsonBody, pRet, TRUE);
	if (Ret == ERROR_CODE_UNAUTHORIZED)
	{
		// Token 可能過期或被撤銷，清掉快取重新登入一次再試一次。
		s_ApiToken.Empty();
		int LoginRet = Login();
		if (LoginRet != ERROR_CODE_SUCCESS)
			return LoginRet;
		Ret = MakeHttpConnectionRaw(pVerb, pRoute, pExtraQuery, pJsonBody, pRet, TRUE);
	}

	return Ret;
}

// ============================================================
// 建表系列：新 API 已移除這些端點（改成部署時的 CLI 腳本
// bin/setup.php），保留方法簽章但不再送出任何 HTTP 請求。
// ============================================================
int CDatabaseAccessURL::CheckDBTables()
{
	return ERROR_CODE_SUCCESS;
}
int CDatabaseAccessURL::CreateDBTable(PCHAR pTableName)
{
	return ERROR_CODE_SUCCESS;
}
int CDatabaseAccessURL::CreateDBOperatorTable()
{
	return ERROR_CODE_SUCCESS;
}
int CDatabaseAccessURL::CreateDBMachineTable()
{
	return ERROR_CODE_SUCCESS;
}
int CDatabaseAccessURL::CreateDBOutLineTable()
{
	return ERROR_CODE_SUCCESS;
}
int CDatabaseAccessURL::CreateDBExtLineTable()
{
	return ERROR_CODE_SUCCESS;
}
int CDatabaseAccessURL::CreateDBRecordTable()
{
	return ERROR_CODE_SUCCESS;
}
int CDatabaseAccessURL::CreateDBAdminTable()
{
	return ERROR_CODE_SUCCESS;
}

// 240 個虛擬外/內線埠的初始種子資料，新版由部署時的 sql/seed.sql 建立，
// 不再是執行期可呼叫的端點。
int CDatabaseAccessURL::AddExtPort(int ExtPort, PCHAR pMessage)
{
	return ERROR_CODE_SUCCESS;
}
int CDatabaseAccessURL::AddOutPort(int OutPort, PCHAR pMessage)
{
	return ERROR_CODE_SUCCESS;
}

int CDatabaseAccessURL::CheckBackEndURL()
{
	// 新 API 沒有真正「不需要驗證」的端點，這裡呼叫 /auth/me：
	// 回 200（剛好有快取的合法 Token）或 401（伺服器有回應、只是未驗證）
	// 都代表後端可連線；只有連線層級的例外才視為真的連不到。
	CString Response;
	int Ret = MakeHttpConnectionRaw("GET", "/auth/me", "", NULL, &Response, TRUE);
	if (Ret == ERROR_CODE_SUCCESS || Ret == ERROR_CODE_UNAUTHORIZED)
		return 0;
	return -1;
}

// 用 (machineType, machineNo, phyPort) 自然鍵找到 outline_ports 那一列的 id。
// 新版每個實體埠是獨立一列，PATCH 需要用 id，不能再像舊版直接用 vport 當路徑。
int CDatabaseAccessURL::FindOutlinePortId(int MachineType, int MachineID, int PhyPort, int* pId)
{
	CString ExtraQuery;
	ExtraQuery.Format("&machineNo=%d&phyPort=%d", MachineID, PhyPort);

	CString Response;
	int Ret = MakeHttpConnection("GET", "/outline-ports", ExtraQuery, NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	for (unsigned int i = 0; i < data.size(); ++i)
	{
		if (data[i].get("machineType", 0).asInt() == MachineType)
		{
			if (pId)
				*pId = data[i].get("id", 0).asInt();
			return ERROR_CODE_SUCCESS;
		}
	}
	return ERROR_CODE_HTTP_NOT_FOUND;
}

int CDatabaseAccessURL::FindExtlinePortId(int MachineType, int MachineID, int PhyPort, int* pId)
{
	CString ExtraQuery;
	ExtraQuery.Format("&machineType=%d&machineNo=%d&phyPort=%d", MachineType, MachineID, PhyPort);

	CString Response;
	int Ret = MakeHttpConnection("GET", "/extline-ports", ExtraQuery, NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (data.size() == 0)
		return ERROR_CODE_HTTP_NOT_FOUND;

	if (pId)
		*pId = data[0u].get("id", 0).asInt();
	return ERROR_CODE_SUCCESS;
}

// 一個虛擬外線編號(vport)現在可能同時被 1~3 種子機類型的實體埠共用
// （代表這幾種設備正在監看同一條實體線路），所以設定通話中/狀態燈號
// 時要把所有共用這個 vport 的列都更新，不能只改一列。
int CDatabaseAccessURL::PatchAllOutlinePortsByVport(int VPort, LPCSTR pJsonBody)
{
	CString Route;
	Route.Format("/outline-ports/by-vport/%d", VPort);

	CString Response;
	int Ret = MakeHttpConnection("GET", Route, "", NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	for (unsigned int i = 0; i < data.size(); ++i)
	{
		int id = data[i].get("id", 0).asInt();
		CString PatchRoute;
		PatchRoute.Format("/outline-ports/%d", id);
		CString PatchResponse;
		int PatchRet = MakeHttpConnection("PATCH", PatchRoute, "", pJsonBody, &PatchResponse);
		if (PatchRet != ERROR_CODE_SUCCESS)
			Ret = PatchRet; // 盡量全部都更新，但保留最後一個失敗的錯誤碼
	}
	return Ret;
}

int CDatabaseAccessURL::GetOutLines(POutPort_T pOutLine)
{
	// 新版 API 回傳的是「一個實體埠一列」，這裡依 vport 分組，重建回舊版
	// 「一個虛擬外線一列、三種類型各自的實體埠當欄位」的形狀，讓呼叫端
	// (ADICTICallCenterDlg 等) 不用跟著改。沒有指派 vport 的實體埠先略過
	// （這是新設計允許的狀態：實體埠已建立但還沒被指派虛擬編號）。
	CString Response;
	int Ret = MakeHttpConnection("GET", "/outline-ports", "", NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (!pOutLine)
		return Ret;

	// vport -> 該 vport 目前指標所在的輸出列位置
	std::map<int, POutPort_T> vportToRow;
	POutPort_T pCursor = pOutLine;
	int RowCount = 0;

	for (unsigned int i = 0; i < data.size(); ++i)
	{
		Json::Value row = data[i];
		if (row["vport"].isNull())
			continue;
		int vport = row["vport"].asInt();

		POutPort_T pRow;
		std::map<int, POutPort_T>::iterator it = vportToRow.find(vport);
		if (it == vportToRow.end())
		{
			// 注意：不能對整個 *pRow 做 memset -- 這個結構裡還有呼叫端已經
			// 設定好、指向既有 UI 控制項的 CStatic*/CEdit* 等指標欄位
			// （OutLineBtn、pOutLineNo 等），全部清零會把那些控制項的
			// 連結打斷。這裡只初始化這個函式實際會填的欄位。
			pRow = pCursor++;
			RowCount++;
			pRow->ID = vport;
			pRow->VPortNo = vport;
			pRow->IsInUsed = FALSE;
			pRow->MachineID = 0;
			pRow->MachineTypeAPhyPort = 0;
			pRow->MachineTypeBPhyPort = 0;
			pRow->MachineTypeCPhyPort = 0;
			vportToRow[vport] = pRow;
		}
		else
		{
			pRow = it->second;
		}

		int machineType = row.get("machineType", 0).asInt();
		int phyPort = row.get("phyPort", 0).asInt();
		pRow->MachineID = row.get("machineNo", 0).asInt();
		if (row.get("inUse", false).asBool())
			pRow->IsInUsed = TRUE;
		switch (machineType)
		{
		case MACHINE_TYPE_PBX:
			pRow->MachineTypeAPhyPort = phyPort;
			break;
		case MACHINE_TYPE_CALLER_ID_BOX:
			pRow->MachineTypeBPhyPort = phyPort;
			break;
		case MACHINE_TYPE_VOICE_CARD:
			pRow->MachineTypeCPhyPort = phyPort;
			break;
		}
	}
	return RowCount;
}

int CDatabaseAccessURL::GetExtLines(PExtPort_T pExtLine)
{
	// 只有 PBX 有內線功能，extline_ports 目前只會有 machineType=1 的資料，
	// 所以這裡不需要像 GetOutLines 那樣依 vport 分組多種類型。
	CString Response;
	int Ret = MakeHttpConnection("GET", "/extline-ports", "&machineType=1", NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (pExtLine)
	{
		for (unsigned int i = 0; i < data.size(); ++i)
		{
			Json::Value row = data[i];
			if (row["vport"].isNull())
				continue; // 尚未指派虛擬編號的實體內線埠先略過
			pExtLine->ID = row.get("id", 0).asInt();
			pExtLine->PortNo = row.get("vport", 0).asInt();
			pExtLine->ExtNo = row["extNum"].isNull() ? 0 : row["extNum"].asInt();
			pExtLine->MachineID = row.get("machineNo", 0).asInt();
			pExtLine++;
		}
	}
	return Ret;
}

int CDatabaseAccessURL::SetExtNumber(int ExtVPort, int MachineID, int ExtensionNo, PCHAR pMessage)
{
	// ExtVPort 在舊介面裡是「虛擬內線編號」；新版要先用 machineNo+vport
	// 找到對應的實體內線埠 id 才能 PATCH。找不到代表這個機碼在這個虛擬
	// 編號底下還沒有指派任何實體內線埠。
	CString ExtraQuery;
	ExtraQuery.Format("&machineType=1&machineNo=%d&vport=%d", MachineID, ExtVPort);

	CString Response;
	int Ret = MakeHttpConnection("GET", "/extline-ports", ExtraQuery, NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data) || data.size() == 0)
		return ERROR_CODE_HTTP_NOT_FOUND;

	int id = data[0u].get("id", 0).asInt();

	Json::Value body;
	body["extNum"] = ExtensionNo;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/extline-ports/%d", id);

	CString PatchResponse;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &PatchResponse);
}

int CDatabaseAccessURL::GetExtVPortByExtNum(int MachineID, int ExtNum, PExtData_T pExtData, PCHAR pMessage)
{
	CString Route;
	Route.Format("/extline-ports/by-ext-num/1/%d/%d", MachineID, ExtNum);

	CString Response;
	int Ret = MakeHttpConnection("GET", Route, "", NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (pExtData)
	{
		pExtData->ID = data.get("id", 0).asInt();
		pExtData->ExtNo = data["extNum"].isNull() ? 0 : data["extNum"].asInt();
		pExtData->ExtVPort = data["vport"].isNull() ? 0 : data["vport"].asInt();
	}
	return Ret;
}

int CDatabaseAccessURL::SetOutPortInUsed(int OutPort, BOOL bInUsed, PCHAR pMessage)
{
	// OutPort 是虛擬外線編號；可能同時有 1~3 種類型的實體埠共用這個編號，
	// 全部都要一起標記使用中/閒置，才能反映「這條實體線路正在通話」。
	Json::Value body;
	body["inUse"] = (bool)(bInUsed == TRUE);
	std::string JsonBody = Json::FastWriter().write(body);

	return PatchAllOutlinePortsByVport(OutPort, JsonBody.c_str());
}

int CDatabaseAccessURL::GetMachineInfo(int MachineType, int MachineID, PCHAR pAlias, int* pOutPort, int* pExtPort, PCHAR pMessage)
{
	CString Route;
	Route.Format("/machines/%d/%d", MachineType, MachineID);

	CString Response;
	int Ret = MakeHttpConnection("GET", Route, "", NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (pAlias)
	{
		CString UniValue = fixjsonchinese(data.get("alias", "").asString());
		strcpy_s(pAlias, 500, UniValue.GetBuffer());
	}
	if (pOutPort)
		*pOutPort = data.get("outPortCount", 0).asInt();
	if (pExtPort)
		*pExtPort = data.get("extPortCount", 0).asInt();

	return Ret;
}

int CDatabaseAccessURL::GetAllMachinesPortCounts(int* pPBXOutPort, int* pPBXExtPort,
	int* pCallerIDBoxOutPort, int* pCallerIDBoxExtPort,
	int* pVoiceCardOutPort, int* pVoiceCardExtPort)
{
	for (int i = 0; i < SUBPROGRAM_NUMBERS; i++)
	{
		pPBXOutPort[i] = 0;
		pPBXExtPort[i] = 0;
		pCallerIDBoxOutPort[i] = 0;
		pCallerIDBoxExtPort[i] = 0;
		pVoiceCardOutPort[i] = 0;
		pVoiceCardExtPort[i] = 0;
	}

	CString Response;
	int Ret = MakeHttpConnection("GET", "/machines", "", NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	for (unsigned int i = 0; i < data.size(); ++i)
	{
		Json::Value row = data[i];
		int MachineType = row.get("machineType", 0).asInt();
		int MachineID = row.get("machineNo", 0).asInt();
		if (MachineID < 1 || MachineID > SUBPROGRAM_NUMBERS)
			continue;
		int OutPort = row.get("outPortCount", 0).asInt();
		int ExtPort = row.get("extPortCount", 0).asInt();

		switch (MachineType)
		{
		case MACHINE_TYPE_PBX:
			pPBXOutPort[MachineID - 1] = OutPort;
			pPBXExtPort[MachineID - 1] = ExtPort;
			break;
		case MACHINE_TYPE_CALLER_ID_BOX:
			pCallerIDBoxOutPort[MachineID - 1] = OutPort;
			pCallerIDBoxExtPort[MachineID - 1] = ExtPort;
			break;
		case MACHINE_TYPE_VOICE_CARD:
			pVoiceCardOutPort[MachineID - 1] = OutPort;
			pVoiceCardExtPort[MachineID - 1] = ExtPort;
			break;
		default:
			break;
		}
	}

	return ERROR_CODE_SUCCESS;
}

int CDatabaseAccessURL::GetAllMachinesSubProgramInfo(PSubProgramGroup_T pPBXGroup,
	PSubProgramGroup_T pCallerIDBoxGroup,
	PSubProgramGroup_T pVoiceCardGroup)
{
	CString Response;
	int Ret = MakeHttpConnection("GET", "/machines", "", NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	for (unsigned int i = 0; i < data.size(); ++i)
	{
		Json::Value row = data[i];
		int MachineType = row.get("machineType", 0).asInt();
		int MachineID = row.get("machineNo", 0).asInt();
		if (MachineID < 1 || MachineID > SUBPROGRAM_NUMBERS)
			continue;

		PSubProgramGroup_T pGroup = NULL;
		switch (MachineType)
		{
		case MACHINE_TYPE_PBX:
			pGroup = pPBXGroup;
			break;
		case MACHINE_TYPE_CALLER_ID_BOX:
			pGroup = pCallerIDBoxGroup;
			break;
		case MACHINE_TYPE_VOICE_CARD:
			pGroup = pVoiceCardGroup;
			break;
		default:
			continue;
		}

		CString Alias = fixjsonchinese(row.get("alias", "").asString());
		pGroup[MachineID - 1].OutPortNum = row.get("outPortCount", 0).asInt();
		pGroup[MachineID - 1].ExtPortNum = row.get("extPortCount", 0).asInt();
		strcpy_s(pGroup[MachineID - 1].Alias, 500, Alias.GetBuffer());
	}

	return ERROR_CODE_SUCCESS;
}

int CDatabaseAccessURL::SetMachineOutPortsByMachineID(int MachineType, int MachineID, int OutPort, PCHAR pMessage, int* pFailedPortCount)
{
	if (pFailedPortCount)
		*pFailedPortCount = 0;

	Json::Value body;
	body["outPortCount"] = OutPort;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/machines/%d/%d", MachineType, MachineID);

	CString Response;
	int Ret = MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
	// 2026-07-03 新增：同一個合併機碼同時間只能有一種類型的外/內線數量
	// 不是 0，違反時後端會回傳明確訊息（例如「合併機碼已被交換機類型
	// 使用...」），這裡取出來讓呼叫端可以顯示給管理員看，不要只是一個
	// 看不出原因的錯誤碼。
	if (Ret != ERROR_CODE_SUCCESS)
	{
		if (pMessage)
			ExtractErrorMessage(Response, pMessage, 200);
		return Ret;
	}

	if (pFailedPortCount)
	{
		Json::Value data;
		if (ParseEnvelope(Response, data))
			*pFailedPortCount = data.get("outPortFailedCount", 0).asInt();
	}
	return Ret;
}

int CDatabaseAccessURL::GetMachineOutPortsByMachineID(int MachineType, int MachineID, int* pOutPort, PCHAR pMessage)
{
	// 舊版呼叫 GetOutLinesByMachine.php 後在迴圈裡覆寫 *pOutPort，
	// 實際上永遠只拿到最後一筆的值，語意上已經等同「這個機碼設定的外
	// 線總數」。新版直接讀子機設定的 outPortCount，語意更正確也更簡單。
	return GetMachineInfo(MachineType, MachineID, NULL, pOutPort, NULL, pMessage);
}

int CDatabaseAccessURL::SetMachineExtPortsByMachineID(int MachineType, int MachineID, int ExtPort, PCHAR pMessage, int* pFailedPortCount)
{
	if (pFailedPortCount)
		*pFailedPortCount = 0;

	Json::Value body;
	body["extPortCount"] = ExtPort;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/machines/%d/%d", MachineType, MachineID);

	CString Response;
	int Ret = MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
	// 理由同 SetMachineOutPortsByMachineID()。
	if (Ret != ERROR_CODE_SUCCESS)
	{
		if (pMessage)
			ExtractErrorMessage(Response, pMessage, 200);
		return Ret;
	}

	if (pFailedPortCount)
	{
		Json::Value data;
		if (ParseEnvelope(Response, data))
			*pFailedPortCount = data.get("extPortFailedCount", 0).asInt();
	}
	return Ret;
}

int CDatabaseAccessURL::SetMachineAliasByMachineID(int MachineType, int MachineID, PCHAR pAlias, PCHAR pMessage)
{
	// 注意：舊版這裡有一個既有 bug -- 組 URL 字串時從未把 pAlias 實際
	// 代入參數，導致舊版永遠送出空字串別名。新版用 JSON Body 正確帶入
	// pAlias，等於順便修掉這個問題。
	Json::Value body;
	body["alias"] = pAlias ? std::string(pAlias) : std::string("");
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/machines/%d/%d", MachineType, MachineID);

	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::SetMachineConnected(int MachineType, int MachineID, PCHAR pIPAddr, BOOL bConnected, PCHAR pMessage)
{
	// 子機連線/斷線時呼叫，把 machines.is_connected/ip_address 寫回資料庫，
	// 這樣網頁版控制台的「狀態」欄位才會反映真正的連線狀態（原本這條路徑
	// 完全沒有實作，MachineLogin/MachineLogout 只更新過 MFC 自己的
	// CListCtrl，資料庫的 is_connected 永遠是預設值 0）。斷線時不覆寫
	// ipAddress，保留最後一次已知的位址。
	Json::Value body;
	body["isConnected"] = (bool)(bConnected == TRUE);
	if (bConnected && pIPAddr)
	{
		body["ipAddress"] = std::string(pIPAddr);
	}
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/machines/%d/%d", MachineType, MachineID);

	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::AssignPhyOutPortInfo(int VPort, int MachineType, int MachineID, int PhyPort, PCHAR pMessage)
{
	// 新版每個實體埠是獨立一列，要先用自然鍵找到那一列的 id 才能 PATCH
	// 它的 vport 欄位。找不到代表這個子機在這個實體埠編號還沒有被建立
	// （通常是因為 outPortCount 還沒設定到涵蓋這個 PhyPort）。
	int id = 0;
	int Ret = FindOutlinePortId(MachineType, MachineID, PhyPort, &id);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value body;
	body["vport"] = VPort;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/outline-ports/%d", id);

	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::AssignPhyExtPortInfo(int VPort, int MachineID, int PhyPort, PCHAR pMessage)
{
	int id = 0;
	int Ret = FindExtlinePortId(MACHINE_TYPE_PBX, MachineID, PhyPort, &id);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value body;
	body["vport"] = VPort;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/extline-ports/%d", id);

	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UnassignPhyOutPortInfo(int MachineType, int MachineID, int PhyPort, PCHAR pMessage)
{
	int id = 0;
	int Ret = FindOutlinePortId(MachineType, MachineID, PhyPort, &id);
	if (Ret == ERROR_CODE_HTTP_NOT_FOUND)
		return ERROR_CODE_SUCCESS; // 本來就沒指派過，當作成功。
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	CString Route;
	Route.Format("/outline-ports/%d", id);

	CString Response;
	return MakeHttpConnection("DELETE", Route, "", NULL, &Response);
}

int CDatabaseAccessURL::UnassignPhyExtPortInfo(int MachineID, int PhyPort, PCHAR pMessage)
{
	int id = 0;
	int Ret = FindExtlinePortId(MACHINE_TYPE_PBX, MachineID, PhyPort, &id);
	if (Ret == ERROR_CODE_HTTP_NOT_FOUND)
		return ERROR_CODE_SUCCESS; // 本來就沒指派過，當作成功。
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	CString Route;
	Route.Format("/extline-ports/%d", id);

	CString Response;
	return MakeHttpConnection("DELETE", Route, "", NULL, &Response);
}

int CDatabaseAccessURL::GetWiredOutPhyPorts(int MachineType, int MachineID, int* pPhyPorts, int MaxCount, int* pActualCount)
{
	CString ExtraQuery;
	ExtraQuery.Format("&machineNo=%d", MachineID);

	CString Response;
	int Ret = MakeHttpConnection("GET", "/outline-ports", ExtraQuery, NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	int Count = 0;
	for (unsigned int i = 0; i < data.size() && Count < MaxCount; ++i)
	{
		if (data[i].get("machineType", 0).asInt() == MachineType)
		{
			pPhyPorts[Count++] = data[i].get("phyPort", 0).asInt();
		}
	}
	if (pActualCount)
		*pActualCount = Count;
	return ERROR_CODE_SUCCESS;
}

int CDatabaseAccessURL::GetWiredExtPhyPorts(int MachineID, int* pPhyPorts, int MaxCount, int* pActualCount)
{
	CString ExtraQuery;
	ExtraQuery.Format("&machineNo=%d", MachineID);

	CString Response;
	int Ret = MakeHttpConnection("GET", "/extline-ports", ExtraQuery, NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	int Count = 0;
	for (unsigned int i = 0; i < data.size() && Count < MaxCount; ++i)
	{
		pPhyPorts[Count++] = data[i].get("phyPort", 0).asInt();
	}
	if (pActualCount)
		*pActualCount = Count;
	return ERROR_CODE_SUCCESS;
}

int CDatabaseAccessURL::GetOutVPort(int MachineType, int MachineID, int PhyPort, int* pOutVPort, PCHAR pMessage)
{
	// 新版可以直接用 machineType+machineNo+phyPort 當查詢參數濾出剛好一列，
	// 不用像舊版那樣抓整批資料再自己比對欄位。
	CString ExtraQuery;
	ExtraQuery.Format("&machineNo=%d&phyPort=%d", MachineID, PhyPort);

	CString Response;
	int Ret = MakeHttpConnection("GET", "/outline-ports", ExtraQuery, NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	for (unsigned int i = 0; i < data.size(); ++i)
	{
		if (data[i].get("machineType", 0).asInt() == MachineType)
		{
			if (data[i]["vport"].isNull())
				return ERROR_CODE_HTTP_NOT_FOUND;
			if (pOutVPort)
				*pOutVPort = data[i]["vport"].asInt();
			return ERROR_CODE_SUCCESS;
		}
	}

	return ERROR_CODE_HTTP_NOT_FOUND;
}

int CDatabaseAccessURL::AddInternalCallRecord(int MachineID, int StartTime, int FromExtNum, int ToExtNum, int* pDBIndexID, PCHAR pMessage)
{
	Json::Value body;
	body["machineNo"] = MachineID;
	body["callType"] = 2; // 內線互打
	body["fromExtNum"] = FromExtNum;
	body["toExtNum"] = ToExtNum;
	body["startTime"] = StartTime;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Response;
	int Ret = MakeHttpConnection("POST", "/call-records", "", JsonBody.c_str(), &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (pDBIndexID)
		*pDBIndexID = data.get("id", 0).asInt();
	return Ret;
}

int CDatabaseAccessURL::AddInternalCallRecordOneTime(PCallBlock_T pCallBlock, PCHAR pMessage)
{
	Json::Value body;
	body["machineNo"] = pCallBlock->MachineID;
	body["callType"] = (int)pCallBlock->CallType;
	body["fromExtNum"] = pCallBlock->FromExtNo;
	body["toExtNum"] = pCallBlock->ToExtNo;
	body["startTime"] = (Json::UInt)pCallBlock->CallStartTime;
	body["endTime"] = (Json::UInt)pCallBlock->CallEndTime;
	body["connectTime"] = (Json::UInt)pCallBlock->CallConnectionTime;
	body["durationSec"] = (Json::UInt)pCallBlock->CallDuration;
	body["ringTimes"] = pCallBlock->RingTimes;
	if (strlen(pCallBlock->VoiceRecFileUUID) > 0)
		body["voiceRecordUuid"] = std::string(pCallBlock->VoiceRecFileUUID);
	std::string JsonBody = Json::FastWriter().write(body);

	CString Response;
	int Ret = MakeHttpConnection("POST", "/call-records", "", JsonBody.c_str(), &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (ParseEnvelope(Response, data))
		pCallBlock->CallRecID = data.get("id", 0).asInt();

	return Ret;
}

int CDatabaseAccessURL::AddExternalCallRecord(int MachineID, int StartTime, int OutVPort, int PhyOutPort, int* pDBIndexID, PCHAR pMessage)
{
	Json::Value body;
	body["machineNo"] = MachineID;
	// 舊版 AddExternalCallRecord.php 沒有帶 CallType 參數；新版 schema
	// 該欄位為必填，這裡先預設 0（外線撥入），後續狀態機若判斷是撥出，
	// 呼叫 UpdateCallRecordCallType 修正即可，跟舊版流程一致。
	body["callType"] = 0;
	body["outVport"] = OutVPort;
	body["outPhyPort"] = PhyOutPort;
	body["startTime"] = StartTime;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Response;
	int Ret = MakeHttpConnection("POST", "/call-records", "", JsonBody.c_str(), &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (pDBIndexID)
		*pDBIndexID = data.get("id", 0).asInt();
	return Ret;
}

int CDatabaseAccessURL::AddExternalCallRecordOneTime(PCallBlock_T pCallBlock, PCHAR pMessage)
{
	Json::Value body;
	body["machineNo"] = pCallBlock->MachineID;
	body["callType"] = (int)pCallBlock->CallType;
	body["outVport"] = pCallBlock->OutVPort;
	body["outPhyPort"] = pCallBlock->PhyOutPort;
	body["telNo"] = std::string(pCallBlock->CallerId);
	body["extNum"] = pCallBlock->ToExtNo;
	body["startTime"] = (Json::UInt)pCallBlock->CallStartTime;
	body["endTime"] = (Json::UInt)pCallBlock->CallEndTime;
	body["connectTime"] = (Json::UInt)pCallBlock->CallConnectionTime;
	body["durationSec"] = (Json::UInt)pCallBlock->CallDuration;
	body["ringTimes"] = pCallBlock->RingTimes;
	if (strlen(pCallBlock->VoiceRecFileUUID) > 0)
		body["voiceRecordUuid"] = std::string(pCallBlock->VoiceRecFileUUID);
	std::string JsonBody = Json::FastWriter().write(body);

	CString Response;
	int Ret = MakeHttpConnection("POST", "/call-records", "", JsonBody.c_str(), &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (ParseEnvelope(Response, data))
		pCallBlock->CallRecID = data.get("id", 0).asInt();

	return Ret;
}

int CDatabaseAccessURL::UpdateCallRecordTel(int DBIndexID, PCHAR pTelNo, PCHAR pMessage)
{
	Json::Value body;
	body["telNo"] = std::string(pTelNo ? pTelNo : "");
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordTalkTime(int DBIndexID, UINT32 Timestamp, PCHAR pMessage)
{
	// 對應原始設計文件裡「開始通話時間」的說法（=接通時間），對到新
	// schema 是 connect_time，不是 call_start_time。
	Json::Value body;
	body["connectTime"] = (Json::UInt)Timestamp;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordRingTimes(int DBIndexID, UINT32 RingTimes, PCHAR pMessage)
{
	Json::Value body;
	body["ringTimes"] = (int)RingTimes;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordEndTime(int DBIndexID, UINT32 Timestamp, PCHAR pMessage)
{
	Json::Value body;
	body["endTime"] = (Json::UInt)Timestamp;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordTalkDuration(int DBIndexID, UINT32 Timestamp, PCHAR pMessage)
{
	Json::Value body;
	body["durationSec"] = (Json::UInt)Timestamp;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordExtNum(int DBIndexID, UINT32 ExtNum, PCHAR pMessage)
{
	Json::Value body;
	body["extNum"] = (int)ExtNum;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordFromExtNum(int DBIndexID, UINT32 FromExtNum, PCHAR pMessage)
{
	Json::Value body;
	body["fromExtNum"] = (int)FromExtNum;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordToExtNum(int DBIndexID, UINT32 ToExtNum, PCHAR pMessage)
{
	Json::Value body;
	body["toExtNum"] = (int)ToExtNum;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordCallType(int DBIndexID, UINT32 CallType, PCHAR pMessage)
{
	Json::Value body;
	body["callType"] = (int)CallType;
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::UpdateCallRecordVoiceRecFileUUID(int DBIndexID, PCHAR pVoiceRecFileUUID, PCHAR pMessage)
{
	Json::Value body;
	body["voiceRecordUuid"] = std::string(pVoiceRecFileUUID ? pVoiceRecFileUUID : "");
	std::string JsonBody = Json::FastWriter().write(body);

	CString Route;
	Route.Format("/call-records/%d", DBIndexID);
	CString Response;
	return MakeHttpConnection("PATCH", Route, "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::AddOperator(int EmployeeId, PCHAR pPassword, PCHAR pMessage)
{
	Json::Value body;
	CHAR EmployeeNoStr[20];
	_itoa_s(EmployeeId, EmployeeNoStr, 10);
	body["employeeNo"] = std::string(EmployeeNoStr);
	body["password"] = std::string(pPassword ? pPassword : "");
	body["role"] = std::string("operator");
	std::string JsonBody = Json::FastWriter().write(body);

	CString Response;
	return MakeHttpConnection("POST", "/employees", "", JsonBody.c_str(), &Response);
}

int CDatabaseAccessURL::GetOperatorInfo(PCHAR OperatorUUID, POperatorInfo_T pOperatorInfo)
{
	// 新版 API 拿掉了「永久 UUID」的身分驗證方式，改成登入才會拿到、
	// 會過期的 Bearer Token。這裡把舊參數 OperatorUUID 直接當成該名
	// 員工自己的 Token 使用（呼叫 GET /auth/me 取得該員工本人資料），
	// 不套用服務帳號的快取 Token。
	if (!OperatorUUID || strlen(OperatorUUID) == 0)
		return ERROR_CODE_UNAUTHORIZED;

	CString SavedToken = s_ApiToken;
	s_ApiToken = OperatorUUID;

	CString Response;
	int Ret = MakeHttpConnectionRaw("GET", "/auth/me", "", NULL, &Response, TRUE);

	s_ApiToken = SavedToken; // 還原服務帳號自己的 Token，不要互相污染

	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (pOperatorInfo)
	{
		pOperatorInfo->ID = data.get("id", 0).asInt();
		pOperatorInfo->EmployeeID = atoi(data.get("employeeNo", "0").asCString());
		CString Name = fixjsonchinese(data.get("name", "").asString());
		strcpy_s(pOperatorInfo->Name, 50, Name.GetBuffer());
		pOperatorInfo->MachineID = data.get("machineMask", 0).asInt();
		// 新 API 的 /auth/me 沒有回傳登入/登出時間戳，這裡先歸零；
		// Level 欄位原始文件沒有明確定義數值意義，這裡用角色字串換算：
		// operator=0、supervisor=1、admin=2，供既有畫面顯示參考用。
		pOperatorInfo->LoginTimestamp = 0;
		pOperatorInfo->LogoutTimestamp = 0;
		std::string Role = data.get("role", "operator").asString();
		if (Role == "admin")
			pOperatorInfo->Level = 2;
		else if (Role == "supervisor")
			pOperatorInfo->Level = 1;
		else
			pOperatorInfo->Level = 0;
		pOperatorInfo->ExtNum = data["extNum"].isNull() ? 0 : data["extNum"].asInt();
	}
	return Ret;
}

int CDatabaseAccessURL::GetCustomerInfo(PCHAR pTelNo, PCustomerInfo_T pCustomerInfo)
{
	CString Route;
	Route.Format("/customers/by-tel/%s", pTelNo ? pTelNo : "");

	CString Response;
	int Ret = MakeHttpConnection("GET", Route, "", NULL, &Response);
	if (Ret != ERROR_CODE_SUCCESS)
		return Ret;

	Json::Value data;
	if (!ParseEnvelope(Response, data))
		return ERROR_CODE_INVALID_RESPONSE;

	if (pCustomerInfo)
	{
		pCustomerInfo->ID = data.get("id", 0).asInt();
		pCustomerInfo->CustomerID = atoi(data.get("customerNo", "0").asCString());
		// 新 schema 沒有 customer_uuid 欄位了，這裡沒有值可以填。
		pCustomerInfo->UUID[0] = '\0';
		CString Name = fixjsonchinese(data.get("name", "").asString());
		strcpy_s(pCustomerInfo->Name, 50, Name.GetBuffer());
		std::string Gender = data.get("gender", "").asString();
		// 沿用舊版既有寫法（atoi 一個 'M'/'F' 字元一律得到 0）；
		// 這是舊版就存在的問題，不在這次重寫的範圍內處理。
		pCustomerInfo->Gender = Gender.empty() ? 0 : atoi(Gender.c_str());
		CString TelNo = fixjsonchinese(data.get("telNo", "").asString());
		strcpy_s(pCustomerInfo->TelNo, 50, TelNo.GetBuffer());
		CString County = fixjsonchinese(data.get("county", "").asString());
		strcpy_s(pCustomerInfo->County, 50, County.GetBuffer());
		CString Township = fixjsonchinese(data.get("township", "").asString());
		strcpy_s(pCustomerInfo->Townships, 50, Township.GetBuffer());
		CString Address = fixjsonchinese(data.get("address", "").asString());
		strcpy_s(pCustomerInfo->Address, 200, Address.GetBuffer());
		std::string Birthday = data.get("birthday", "").asString();
		strcpy_s(pCustomerInfo->Birthday, 11, Birthday.c_str());
		pCustomerInfo->InBlackList = data.get("isBlacklisted", false).asBool() ? 1 : 0;
	}
	return Ret;
}

int CDatabaseAccessURL::SetOutVPortCallStatusInfo(INT OutVPortNo, POutVPortCallStatus_T pOutVPortCallStatus)
{
	// 新 schema 的 outline_ports 只保留 callStatus 與實體埠對應欄位，沒有
	// TelNo/ExtNum/CallType/PhyPortNo 這幾個欄位（這些屬於 call_records
	// 的內容，由 Add/UpdateCallRecord 系列另外處理）。這裡只做部分對應，
	// 更新即時通話狀態燈號用的 callStatus，而且這個 vport 可能同時被
	// 多種類型的實體埠共用，要一次全部更新。
	if (!pOutVPortCallStatus)
		return ERROR_CODE_SUCCESS;

	Json::Value body;
	body["callStatus"] = pOutVPortCallStatus->Status;
	std::string JsonBody = Json::FastWriter().write(body);

	return PatchAllOutlinePortsByVport(OutVPortNo, JsonBody.c_str());
}
