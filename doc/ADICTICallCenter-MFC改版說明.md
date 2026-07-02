# ADICTICallCenter（MFC 主核心）改用新版 API 說明

## 2026-07-01 更新：啟動效能修正 + 內嵌 ADICTICallCenter.Web 分頁

### 啟動很慢的根因與修正

1. **系統 Proxy**：`CDatabaseAccessURL`（繼承 `CInternetSession`）建構子預設用 `PRE_CONFIG_INTERNET_ACCESS`，會讓 WinINet 套用系統/IE 的 Proxy 設定連 API。如果這台機器有設定 Proxy 又沒把 localhost 排除在外，每次 API 呼叫都要等 Proxy 逾時才會失敗，非常慢。已改成 `INTERNET_OPEN_TYPE_DIRECT` 強制直連。
2. **N+1 次 HTTP 呼叫**：`OnInitDialog()` 建立 4 個分頁時，`VirtualOutPortDBAccess.cpp` 的 `LoadMachinePorts()`、以及 `CPbxDlg`/`CCallRecorderDlg`/`CVoiceRecorderDlg` 各自的 `LoadMachineData()`，都是對 3 種類型 x 10 個機碼逐一呼叫 `GetMachineInfo()`（等於 30 次同步 HTTP 往返 x 4 個地方 ≈ 90~120 次）。新增 `CDatabaseAccessURL::GetAllMachinesPortCounts()` 與 `GetAllMachinesSubProgramInfo()` 兩支方法，各自用一次 `GET /machines`（不帶篩選條件回傳全部列）取代整組迴圈，啟動流程壓到十幾次 HTTP 呼叫。

### 新增「網頁版控制台」分頁（內嵌 ADICTICallCenter.Web）

在原本 5 個分頁之後多加一個「網頁版控制台」分頁，用 WebView2 內嵌顯示 `ADICTICallCenter.Web`（預設 `http://localhost:8842`，可用登錄檔 `SystemSetting\WebConsoleURL` 覆寫），純粹多一個入口，不取代任何既有分頁/功能。新增檔案：`CWebConsoleDlg.h`/`.cpp`，`third_party/WebView2/`（手動下載解壓的 WebView2 SDK nupkg，未走 NuGet 還原，因為這個舊式 vcxproj 沒有配置 NuGet restore）。

**2026-07-02 更新**：`third_party/WebView2/build/native/{include,x64,x86}`（`WebView2.h` 跟這個專案實際會連結的 `WebView2LoaderStatic.lib`，約 25MB）現在直接 commit 進 repo，`git clone` 下來就能建置，不用再手動補檔案。WebView2 SDK 的授權（`LICENSE.txt`，類 BSD）允許以二進位形式重新散布，單一檔案大小（最大約 10.7MB）也遠低於 GitHub 的限制，所以評估後決定直接 vendor 進版控，換取「clone 下來就能建置」而不用每個人（或 CI）多一道下載步驟。

只保留這個 native C++ MFC 專案實際用得到的子集：`.vcxproj` 只宣告了 `Win32`（x86）跟 `x64` 平台，所以不含 `arm64`；又因為是純 native 專案，不含 nupkg 裡給 .NET/WinRT/WPF 用的 managed 投影版本（`lib/`、`lib_manual/`、`runtimes/`、`tools/`、`build/native/include-winrt/`）——這些仍然在 `.gitignore` 裡排除，避免以後有人不小心整包 nupkg 解壓進來又整份 commit 上去。

**只有在升級 WebView2 SDK 版本時才需要**用到 `ADICTICallCenter/ADICTICallCenter/third_party/Fetch-WebView2Sdk.ps1`：先更新 `third_party/WebView2/Microsoft.Web.WebView2.nuspec` 裡的版本號，在 `third_party/` 資料夾底下執行 `.\Fetch-WebView2Sdk.ps1 -Version 1.0.xxxx.xx -Force`，腳本會下載新版 nupkg、只取出 `build/native/{include,x64,x86}` 覆蓋現有檔案，跑完後 `git diff` 確認變動再 commit。這台機器需要能連上 `api.nuget.org`；環境沒有對外網路的話，改成從另一台已經跑過的機器直接複製 `third_party/WebView2/build/native/` 過去即可。

⚠️ **關鍵坑：這個 App 從來沒有初始化過 COM**。`AfxEnableControlContainer()` 不會幫你呼叫 `CoInitialize`/`OleInitialize`，WebView2 的 `CreateCoreWebView2EnvironmentWithOptions` 需要呼叫端執行緒已經是 COM STA，沒有這行的話，環境建立會直接失敗，而且是**完全沒有錯誤訊息的靜默失敗**——分頁看起來就是一片空白，跟正常運作時的視覺效果沒有明顯差異，很難第一時間判斷是「還沒載入完」還是「根本沒在動」。已在 `ADICTICallCenter.cpp` 的 `InitInstance()` 最前面加上 `CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);` 解決。`CWebConsoleDlg` 另外加了一個狀態文字（載入中/實際 HRESULT/`WebErrorStatus`），之後再遇到類似問題至少畫面上看得出卡在哪一步，不會又是一片空白。

## 2026-07-01 更新：虛擬線路 API 改版（outline_ports/extline_ports）

API 端的虛擬外線/內線資料模型第二次改版（詳見 `doc/README.md`「虛擬線路的第二版設計」），`CDatabaseAccessURL.cpp` 已經跟著同步更新，公開方法簽章一樣沒有變。這次改版對 MFC 端的影響：

- `GetOutLines()` 現在要把新 API 回傳的「一列一個實體埠」資料依 `vport` 分組，重建回舊版「一個虛擬外線一列、三種類型各自的實體埠欄位」的形狀，內部多了一段分組邏輯，但輸出給呼叫端的 `POutPort_T` 陣列格式不變。
- `AssignPhyOutPortInfo()`/`AssignPhyExtPortInfo()` 現在要先用 `(machineType, machineNo, phyPort)` 查出實體埠的內部 id 才能送出更新，多了一次額外的 API 呼叫（先 GET 查 id，再 PATCH）。
- `SetOutPortInUsed()`/`SetOutVPortCallStatusInfo()` 現在會先查出共用這個虛擬編號的所有實體埠，然後全部一起更新（因為一個虛擬編號可能同時對應到 PBX、來電盒、語音卡三種類型的實體埠）。
- 這幾個函式因此都比改版前多了 1~2 次 HTTP 往返，正常情況下差異不大，但如果狀態更新頻率很高、對延遲敏感，實機測試時要留意看看有沒有感覺卡頓。

## 改了什麼

只改了 3 個檔案：
- `ADICTICallCenter/CDatabaseAccessURL.h`
- `ADICTICallCenter/CDatabaseAccessURL.cpp`
- `ADICTICallCenter/ErrorCode.h`（新增 3 個錯誤代碼）

其餘 19 個會呼叫 `CDatabaseAccessURL` 的檔案（`ADICTICallCenterDlg.cpp`、`CallStateRecordEngine.cpp`、`WSServer.cpp`、`MachineServer.cpp`、`VirtualOutPortDBAccess.cpp`、`CPbxDlg.cpp` 等）**完全不用改**，因為所有對外的方法名稱、參數、輸出結構都保持原樣，只有類別內部改成呼叫新版 `ADICTICallSystem.API`。

## ⚠️ 重要：部署前務必實機測試

這台機器最初沒有裝 Visual Studio / MSBuild，這次的 C++ 修改是在沒有編譯驗證的情況下寫出來的（後來已經在使用者的 Visual Studio 上重建成功，見下方「除錯記錄」）。這是控制真實電話交換機/來電盒/語音卡的核心程式，正式上線前請務必：

1. 先在測試環境（不是正式的電話線路）跑過一輪完整流程：登入、來電、去電、內線互打、掛斷、錄音檔名寫入，並到資料庫確認 `call_records`/`outlines`/`extlines` 資料有正確寫入。
2. 確認沒問題後才部署到正式環境。

### 除錯記錄：兩個實際踩到的坑

1. **這台機器其實有兩份獨立的 `ADICTICallCenter` 專案**（`D:\ADICTI\ADICTICallCenter\` 與 `D:\ADICTI\ADICTICallSystem\ADICTICallCenter\`），且兩者當時 `CDatabaseAccessURL.cpp` 的內部實作寫法不同（一份用 jsoncpp、一份是更早期手刻 `malloc`+`BCMParserJson` 解析）。所幸對外的 44 個公開方法簽章完全一致，重寫內容不用跟著改，只是要注意改對檔案路徑。
2. **編碼問題（UTF-8 BOM）**：專案裡其他檔案開頭都有 UTF-8 BOM（`EF BB BF`），這是告訴編譯器「這份檔案是 UTF-8」的標記；工具寫出來的新檔案一開始沒有帶 BOM，導致編譯器改用系統的 Big5(950) 碼頁去解讀中文註解的 UTF-8 位元組，位元組被誤判後，程式碼在看似隨機的位置出現一堆「遺漏函式標頭」「識別項未宣告」之類的錯誤，而且錯誤的行號跟實際內容對不上（因為誤判會讓行號計算跟著跑掉）。判斷依據是編譯輸出裡的 `warning C4819`。修法是幫檔案補上 UTF-8 BOM。**之後如果這個專案還有其他檔案要重寫且含有中文字，記得檢查新檔案是否也帶 BOM。**
3. `dist\jsoncpp.cpp` 原本沒有被加進 `.vcxproj` 的編譯清單，導致 `Json::Value`/`Json::Reader`/`Json::FastWriter` 有標頭宣告但沒有實作可以連結，已經加入並設定 `PrecompiledHeader=NotUsing`（因為它沒有 `#include "pch.h"`）。

## 部署前必須先做的事：建立服務帳號

新版 API 每一支端點都需要身分驗證，MFC 主核心程式現在會在第一次呼叫 API 時自動用一組「服務帳號」登入、快取 Token，之後每次呼叫都自動帶上，Token 過期也會自動重新登入一次。

**步驟 1：建立服務帳號**

用 Postman（或 curl）先登入一個既有的 admin 帳號，呼叫 `POST /employees` 建立一個給主核心程式專用的服務帳號，例如：
```json
{
  "employeeNo": "core-service",
  "password": "一組你自己設定的強密碼",
  "name": "主核心服務帳號",
  "role": "admin"
}
```
> 建議給 `admin` 角色，因為主核心需要呼叫 `PATCH /machines/*`、`PATCH /outlines/*` 等需要 admin/supervisor 權限的端點。

**步驟 2：把帳密寫進登錄檔**

程式是透過 `AfxGetApp()->GetProfileString("SystemSetting", "ApiEmployeeNo"/"ApiPassword", ...)` 讀取，跟既有的 `DatabaseBackEndURL` 用同一套機制（`SetRegistryKey` 已經把設定導向登錄檔，不是 .ini 檔）。因為這次沒有新增設定畫面（怕手動改 .rc 資源檔沒編譯器驗證會出錯），需要手動用 `regedit` 或指令加入這兩個值：

1. 打開 `regedit`，先找到既有的 `DatabaseBackEndURL` 在哪裡（在 `HKEY_CURRENT_USER\Software\亨模工作室\ADICTICallCenter\SystemSetting` 底下，實際的 App 子機碼名稱請以您機器上實際看到的為準）。
2. 在同一個 `SystemSetting` 機碼底下，新增兩個字串值：
   - `ApiEmployeeNo` = `core-service`
   - `ApiPassword` = 步驟 1 設定的密碼

**步驟 3：確認 Base URL 設定**

沿用既有的 `DatabaseBackEndURL`（主機名稱）、`DatabaseBackEndURLPort`（連接埠）設定，不用改；但因為新版 API 的路徑進入點改了，新增了一個可選的登錄檔值：
- `DatabaseBackEndBasePath`，預設是 `/ADICTICallSystem.API/index.php`（對應本機部署時建立的轉接檔位置）。如果您的部署路徑不同，記得在登錄檔加這個值覆蓋預設。

## 這次順便修掉的舊 bug

- `SetMachineAliasByMachineID`：舊版程式碼組 URL 字串時漏了把別名參數代入 `sprintf_s`，導致別名永遠送出空字串，設定畫面上改別名其實從來沒真的生效過。新版用 JSON Body 正確帶入，這個功能現在才是真的能用的狀態。

## 已知的能力落差（新 API 沒有對應欄位，做了合理的最佳對應）

- `GetCustomerInfo` 的 `UUID` 欄位：新的客戶資料表沒有 `customer_uuid` 這個欄位了，回傳一律是空字串。
- `GetOperatorInfo` 的 `LoginTimestamp`/`LogoutTimestamp`：新版 `/auth/me` 沒有回傳登入/登出時間，一律回傳 0。`Level` 欄位原始設計文件從未定義數值意義，這裡用角色字串換算（operator=0、supervisor=1、admin=2），僅供既有畫面顯示參考，如果原本的 UI 邏輯依賴特定數值判斷，需要另外確認。
- `SetOutVPortCallStatusInfo`：新的虛擬外線資料表沒有 `TelNo`/`ExtNum`/`CallType`/`PhyPortNo` 欄位（這些現在只存在 `call_records`），只更新了 `callStatus`（通話狀態燈號），其餘欄位這次沒有對應的地方可以寫入。
- `AddOperator`：新版 API 要求密碼至少 8 碼，如果既有呼叫端傳入更短的密碼會被拒絕（回傳非 0），這是新 API 的密碼強度規則，跟舊版行為不同。

## 身分驗證的重新設計

舊版：`GetOperatorInfo(OperatorUUID)` 用一組永久有效、跟密碼綁死的 UUID 當識別碼去查資料庫。
新版：沒有永久 UUID 的概念了，改成登入後拿到一個會過期的 Token。`GetOperatorInfo` 的行為被重新解讀為「把傳入的 `OperatorUUID` 當作該名員工自己的 Token，呼叫 `GET /auth/me` 取得他自己的資料」——语意上等同（因為舊版的 UUID 本來就是「代表這個人身分」的憑證），但如果呼叫端某處把 `OperatorUUID` 當成「可以查詢任何人」的萬用鑰匙在用（而不是查詢當事人自己），行為會不一樣，請在測試時特別留意這個函式的呼叫情境。
