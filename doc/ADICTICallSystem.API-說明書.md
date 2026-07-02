# 電話中心系統 API 說明書（重寫版）

版本：初版
對應專案：`ADICTICallSystem.API`
本文件僅說明 API 的呼叫方式，架構與資料庫設計請參考 `doc/README.md`。

---

## 2026-07-01 更新：修正中文文字欄位寫入/讀取失敗（PDO_ODBC 內碼問題）

**問題**：透過 Web 版設定交換機別名（`PATCH /machines/{type}/{no}`，帶中文字）會失敗，錯誤是 `SQLSTATE[22026]: String data, length mismatch` 或 `SQLSTATE[22001]: String data, right truncated`，有時甚至沒回錯誤但存進去的值變成亂碼或別的欄位的值。

**根因**：這台機器沒有 `pdo_sqlsrv`，用 `pdo_odbc` 連 SQL Server（見 `config/config.php` 的 `db.driver=odbc`）。`pdo_odbc` 綁定字串參數時是用**作業系統的 ANSI 內碼**（這台是繁體中文 Windows，內碼 950/Big5），不是 UTF-8——PHP 字串本身是 UTF-8，直接綁定給 ODBC 驅動程式，驅動程式會誤判成 Big5 位元組去跟目標的 `NVARCHAR` 欄位做轉換，導致長度算不合、寫入失敗，或者算出一個「看起來合法但實際上是亂碼」的字串存進去。讀出來的時候也有對稱的問題：`pdo_odbc` fetch 回來的 `NVARCHAR` 文字也是用同一套 ANSI 內碼路徑，不是 UTF-8，直接拿去 `json_encode()` 常常會整包回應變成空的（`json_encode` 遇到不合法 UTF-8 位元組會直接失敗，不會拋例外，只會回傳空字串）。

**修法**：
- `src/Core/Database.php` 新增兩個靜態方法：
  - `Database::bindText($stmt, $param, $value)`：寫入前，如果是 `odbc` 驅動而且值有非 ASCII 位元組，先用 `iconv('UTF-8', 'CP950//TRANSLIT', $value)` 轉成系統內碼再綁定，讓 ODBC 驅動程式自己那套（原本就正確的）內碼轉換去處理。
  - `Database::decodeText($value)`：讀出來後，如果值不是合法 UTF-8（用 PCRE `/u` 修飾字驗證，因為這台機器沒裝 `mbstring`），假設它是系統內碼，轉回 UTF-8。
  - 系統內碼预設 `CP950`，可用 `config.php` 的 `db.odbc_ansi_codepage`（或環境變數 `DB_ODBC_ANSI_CODEPAGE`）覆寫，換到非繁體中文 Windows 主機時要記得改。
- `src/Controllers/Controller.php` 新增 `executeWithParams($stmt, $params, $textFields = [])` 共用方法：把 `$textFields` 裡列出的欄位用 `Database::bindText()` 綁，其餘欄位一般 `bindValue()` 綁，最後呼叫 `execute()`（不帶參數）。**這裡有個踩到的坑**：`pdo_odbc` 不支援「先手動 `bindValue()` 綁一個欄位，其餘欄位再用 `execute($array)` 一次綁」這種混合寫法——它內部是把具名參數位置化的，混用會讓位置對不上，實測會讓 `alias` 欄位變成存進 `machine_type` 的值！所以只要有任何一個欄位需要 `bindText()`，其他欄位也必須全部用 `bindValue()` 顯式綁，不能再靠 `execute($array)`。
- 已套用到目前資料庫裡實際會存中文自由文字的欄位：`machines.alias`（`MachineController`）、`employees.name`（`EmployeeController`）、`customers.name`/`county`/`township`/`address`（`CustomerController`）。`call_records.tel_no`/`second_dtmf` 沒有套用，因為這兩欄位性質是電話號碼/DTMF 數字序列，不會有中文，也沒有用到會踩雷的混合綁定寫法。

---

## 一、使用須知

### 1. 基本網址（Base URL）

本機部署範例（實際環境請依部署位置調整）：
```
http://localhost:8841/ADICTICallSystem.API/index.php
```

### 2. 路徑呼叫方式

因本機 IIS 未安裝 URL Rewrite 模組，所有 API 路徑一律透過 `route` 查詢參數呼叫，格式為：
```
{Base URL}?route=<路徑>&<其他參數>
```
例如呼叫 `GET /machines`：
```
http://localhost:8841/ADICTICallSystem.API/index.php?route=/machines
```
若日後部署到有支援 URL Rewrite 的主機，可直接使用乾淨路徑（`/machines`），不需要 `route` 參數。

### 3. 身分驗證

除了「登入」本身，**所有 API 都必須在 HTTP Header 帶上登入取得的 Token**：
```
Authorization: Bearer <token>
```
Token 效期預設 8 小時，過期或被登出撤銷後需要重新登入。

### 4. 回應格式

所有 API 一律回傳以下格式：
```json
{
  "success": true,
  "data": { ... },
  "message": "說明文字或 null",
  "meta": {}
}
```
- `success`：`true` 表示成功、`false` 表示失敗。
- `data`：成功時的資料內容，型別依 API 而定（物件、陣列或 `null`）。
- `message`：附加說明訊息，可能為 `null`。
- 失敗時 HTTP 狀態碼會是對應的 4xx/5xx（例如 401 未登入、403 權限不足、404 找不到資料、422 參數錯誤、409 資料衝突、500 伺服器錯誤），`success` 為 `false`，`message` 說明錯誤原因。錯誤訊息刻意不包含原始資料庫錯誤內容，避免資安資訊外洩。

### 5. 角色權限

員工帳號有三種角色：
- `operator`：一般客服員
- `supervisor`：主管，可監看多個機碼
- `admin`：系統管理員，可管理員工帳號與系統設定

下方每支 API 皆會標示所需權限；標示「任何人」代表任何已登入（帶有效 Token）的員工皆可呼叫，不分角色。

---

## 二、身分驗證 API

### 2.1 登入

- **方法與路徑**：`POST /auth/login`
- **說明**：以員工編號與密碼登入，取得 Session Token。
- **權限**：不需登入
- **參數**（JSON Body）：

| 參數 | 型別 | 必填 | 說明 |
|---|---|---|---|
| employeeNo | string | 是 | 員工編號 |
| password | string | 是 | 密碼 |

- **回傳** `data`：

| 欄位 | 型別 | 說明 |
|---|---|---|
| token | string | 之後 API 呼叫要帶的 Bearer Token |
| expiresInSeconds | int | Token 效期（秒） |
| employee.id | int | 員工內部索引 |
| employee.employeeNo | string | 員工編號 |
| employee.name | string\|null | 姓名 |
| employee.role | string | 角色：operator/supervisor/admin |
| employee.machineMask | int | 綁定機碼的位元遮罩 |
| employee.extNum | int\|null | 分機號碼 |

- **範例**：
```bash
curl -X POST "http://localhost:8841/ADICTICallSystem.API/index.php?route=/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"employeeNo":"admin","password":"AdminStr0ngPassw0rd!"}'
```
```json
{
  "success": true,
  "data": {
    "token": "ae48ac96cd30c2ab42a05872cf2b13e718b7ef3234c9b247ca3f283f7ba28a7f",
    "expiresInSeconds": 28800,
    "employee": { "id": 1, "employeeNo": "admin", "name": "System Administrator", "role": "admin", "machineMask": 0, "extNum": null }
  },
  "message": "Login successful.",
  "meta": {}
}
```
- **失敗情境**：員工編號或密碼錯誤、帳號被停用 → 401。

### 2.2 登出

- **方法與路徑**：`POST /auth/logout`
- **說明**：撤銷目前使用中的 Token，該 Token 之後即失效。
- **權限**：任何人（需帶 Token）
- **參數**：無
- **範例**：
```bash
curl -X POST "http://localhost:8841/ADICTICallSystem.API/index.php?route=/auth/logout" \
  -H "Authorization: Bearer <token>"
```

### 2.3 目前登入者資訊

- **方法與路徑**：`GET /auth/me`
- **說明**：查詢目前 Token 對應的員工資料。
- **權限**：任何人
- **回傳** `data`：同 2.1 的 `employee` 欄位結構。

---

## 三、員工管理 API

### 3.1 新增員工

- **方法與路徑**：`POST /employees`
- **權限**：admin
- **參數**（JSON Body）：

| 參數 | 型別 | 必填 | 說明 |
|---|---|---|---|
| employeeNo | string | 是 | 員工編號，須唯一 |
| password | string | 是 | 密碼，至少 8 碼 |
| name | string | 否 | 姓名 |
| role | string | 否 | operator（預設）/ supervisor / admin |

- **回傳** `data`：`{ "id": 新員工索引 }`
- **失敗情境**：`employeeNo` 已存在 → 409；密碼不足 8 碼或角色值不合法 → 422。

### 3.2 列出員工

- **方法與路徑**：`GET /employees`
- **權限**：admin、supervisor
- **參數**（Query String，皆為選填）：

| 參數 | 型別 | 說明 |
|---|---|---|
| role | string | 依角色篩選 |
| limit | int | 每頁筆數，預設 50，上限 200 |
| offset | int | 跳過筆數，預設 0 |

- **回傳** `data`：員工物件陣列，每筆包含 `id, employeeNo, name, role, machineMask, extNum, loginAt, logoutAt, isDisabled`。

### 3.3 員工詳細資料

- **方法與路徑**：`GET /employees/{id}`
- **權限**：本人、admin、supervisor
- **回傳** `data`：單一員工物件（欄位同上）。
- **失敗情境**：查無此員工 → 404；非本人且非 admin/supervisor → 403。

### 3.4 更新員工

- **方法與路徑**：`PATCH /employees/{id}`
- **權限**：本人僅能改 `name`；改 `role` 或 `isDisabled` 需要 admin。
- **參數**（JSON Body，皆為選填，至少帶一項）：

| 參數 | 型別 | 說明 |
|---|---|---|
| name | string | 姓名 |
| role | string | operator / supervisor / admin（僅 admin 可用） |
| extNum | int | 分機號碼 |
| isDisabled | bool | 是否停用帳號（僅 admin 可用） |

### 3.5 變更密碼

- **方法與路徑**：`PATCH /employees/{id}/password`
- **權限**：本人或 admin
- **參數**（JSON Body）：

| 參數 | 型別 | 必填 | 說明 |
|---|---|---|---|
| currentPassword | string | 本人操作時必填 | 目前密碼（admin 代改時不需要） |
| newPassword | string | 是 | 新密碼，至少 8 碼 |

- **失敗情境**：本人操作但 `currentPassword` 錯誤 → 401。

### 3.6 綁定機碼

- **方法與路徑**：`POST /employees/{id}/machines/{machineNo}`
- **說明**：讓員工可以看到/操作指定機碼（1~10）的通話狀態，主管可綁定多個機碼。
- **權限**：admin

### 3.7 解除綁定機碼

- **方法與路徑**：`DELETE /employees/{id}/machines/{machineNo}`
- **權限**：admin

---

## 四、客戶管理 API

### 4.1 新增客戶

- **方法與路徑**：`POST /customers`
- **權限**：任何人
- **參數**（JSON Body，皆為選填）：`customerNo, name, birthday(YYYY-MM-DD), gender(M/F), telNo, email, county, township, address`
- **回傳** `data`：`{ "id": 新客戶索引 }`

### 4.2 搜尋客戶

- **方法與路徑**：`GET /customers`
- **權限**：任何人
- **參數**（Query String，皆為選填）：

| 參數 | 型別 | 說明 |
|---|---|---|
| telNo | string | 電話號碼模糊搜尋 |
| blacklisted | bool | 是否僅列出黑名單客戶 |
| limit / offset | int | 分頁 |

### 4.3 客戶詳細資料

- **方法與路徑**：`GET /customers/{id}`
- **權限**：任何人

### 4.4 依電話號碼查客戶

- **方法與路徑**：`GET /customers/by-tel/{telNo}`
- **說明**：常用於來電時彈出客戶資料（依電話號碼取最新一筆）。
- **權限**：任何人

### 4.5 更新客戶

- **方法與路徑**：`PATCH /customers/{id}`
- **權限**：任何人
- **參數**：同 4.1 欄位，另可帶 `isBlacklisted`（bool），皆為選填、至少帶一項。

---

## 五、子機設定 API

子機類型（`machineType`）：`1` = 交換機(PBX)、`2` = 來電盒(CallerID Box)、`3` = 語音卡(Voice Card)。機碼（`machineNo`）範圍 1~10。

### 5.1 列出子機槽位

- **方法與路徑**：`GET /machines`
- **權限**：任何人
- **參數**（Query String，皆為選填）：`machineType, machineNo`
- **回傳** `data`：陣列，每筆含 `machineType, machineNo, alias, outPortCount, extPortCount, ipAddress, swVersion, isConnected, lastSeenAt`。

### 5.2 子機槽位詳細資料

- **方法與路徑**：`GET /machines/{machineType}/{machineNo}`
- **權限**：任何人

### 5.3 更新子機設定

- **方法與路徑**：`PATCH /machines/{machineType}/{machineNo}`
- **權限**：admin、supervisor
- **參數**（JSON Body，皆為選填）：`alias(string), outPortCount(int), extPortCount(int)`
- **業務規則**：
  - 只有 `machineType=1`（交換機）能設定 `extPortCount`；其餘類型設定內線數會被拒絕（422）。
  - 除此之外**沒有其他跨類型限制**——同一個 `machineNo` 底下，PBX、來電盒、語音卡可以任意組合、任意數量同時存在，彼此獨立設定外線數，不會互相卡住。
  - 修改 `outPortCount`/`extPortCount` 時，系統會自動在 `outline_ports`/`extline_ports`（見第六、七節）新增或刪除對應的實體埠列，讓埠位列表跟這裡設定的數量隨時保持一致。

---

## 六、虛擬外線 API（outline_ports）

**設計說明**：虛擬外線編號(`vport`)不是固定 1~240 的池子，而是每一個「實體外線埠」列上的一個欄位。一列代表一個 `(machineType, machineNo, phyPort)` 組合，也就是某個子機的某一個實體外線孔；`vport` 可以是 `null`（代表這個實體埠還沒被指派虛擬編號），也可以跟其他列共用同一個數字——共用代表這幾個實體埠監看的是同一條真實電話線。列的數量會隨著 `machines` 表的 `outPortCount` 自動增減（見 5.3），不需要另外新增/刪除實體埠列的 API。

### 6.1 列出實體外線埠

- **方法與路徑**：`GET /outline-ports`
- **權限**：任何人
- **參數**（Query String，皆為選填）：`machineType(int), machineNo(int), phyPort(int), vport(int)`
- **回傳** `data`：陣列，每筆含 `id, machineType, machineNo, phyPort, vport, inUse, callStatus`。
  - `callStatus`：`0`=閒置、`1`=撥入、`2`=撥出、`3`=通話中、`4`=掛機
  - `vport` 可能是 `null`（尚未指派）

### 6.2 實體外線埠詳細資料

- **方法與路徑**：`GET /outline-ports/{id}`
- **權限**：任何人

### 6.3 查詢共用同一虛擬編號的所有實體外線埠

- **方法與路徑**：`GET /outline-ports/by-vport/{vport}`
- **說明**：回傳目前哪些子機類型/機碼的實體外線埠正在共用這個虛擬編號（代表同一條實體線路）。
- **權限**：任何人

### 6.4 更新實體外線埠

- **方法與路徑**：`PATCH /outline-ports/{id}`
- **權限**：admin、supervisor
- **參數**（JSON Body，皆為選填）：`vport(int), inUse(bool), callStatus(int)`
- **說明**：這是指派/變更虛擬編號的地方——把不同類型、不同機碼的實體外線埠 PATCH 成同一個 `vport` 值，就代表它們是同一條實體線路。

---

## 七、虛擬內線 API（extline_ports）

設計原則同第六節，差別是內線只有交換機(PBX)類型會用到，並且多了分機號碼欄位。

### 7.1 列出實體內線埠

- **方法與路徑**：`GET /extline-ports`
- **權限**：任何人
- **參數**（Query String，皆為選填）：`machineType(int), machineNo(int), phyPort(int), vport(int)`
- **回傳** `data`：陣列，每筆含 `id, machineType, machineNo, phyPort, vport, extNum, status, currentEmployeeId`。

### 7.2 實體內線埠詳細資料

- **方法與路徑**：`GET /extline-ports/{id}`
- **權限**：任何人

### 7.3 查詢共用同一虛擬編號的所有實體內線埠

- **方法與路徑**：`GET /extline-ports/by-vport/{vport}`
- **權限**：任何人

### 7.4 依分機號碼查詢

- **方法與路徑**：`GET /extline-ports/by-ext-num/{machineType}/{machineNo}/{extNum}`
- **權限**：任何人

### 7.5 更新實體內線埠

- **方法與路徑**：`PATCH /extline-ports/{id}`
- **權限**：admin、supervisor
- **參數**（JSON Body，皆為選填）：`vport(int), extNum(int), status(int)`
- **失敗情境**：`extNum` 與其他實體內線埠重複 → 409。

---

## 八、通聯記錄 API

`callType`：`0` = 外線撥入、`1` = 外線撥出、`2` = 內線互打。時間欄位（`startTime`/`connectTime`/`endTime`）可傳 ISO 8601 字串（如 `2026-07-01T10:00:00Z`）或 Unix 時間戳（整數秒）。

### 8.1 新增通聯記錄

- **方法與路徑**：`POST /call-records`
- **權限**：任何人
- **參數**（JSON Body）：

| 參數 | 型別 | 必填 | 說明 |
|---|---|---|---|
| machineNo | int | 是 | 機碼 |
| callType | int | 是 | 0/1/2 |
| outVport | int | 否 | 虛擬外線編號 |
| outPhyPort | int | 否 | 實體外線編號 |
| telNo | string | 否 | 電話號碼 |
| extNum | int | 否 | 外線通話時的接聽分機 |
| fromExtNum | int | 否 | 內線互打時的撥出分機 |
| toExtNum | int | 否 | 內線互打時的撥入分機 |
| ringTimes | int | 否 | 響鈴次數 |
| startTime / connectTime / endTime | string/int | 否 | 開始/接通/結束時間 |
| durationSec | int | 否 | 通話秒數 |
| secondDtmf | string | 否 | 二次撥號內容 |
| voiceRecordUuid | string | 否 | 錄音檔名(UUID) |
| customerId | int | 否 | 對應客戶索引 |
| employeeId | int | 否 | 對應員工索引 |

- **回傳** `data`：`{ "id": 新通聯記錄索引 }`

### 8.2 搜尋通聯記錄

- **方法與路徑**：`GET /call-records`
- **權限**：任何人
- **參數**（Query String，皆為選填）：`machineNo, telNo, employeeId, callType, dateFrom, dateTo, limit, offset`（`dateFrom`/`dateTo` 依 `startTime` 篩選）
- **說明**：取代舊版 `QueryInBoundNumbers` / `QueryInboundDataByDate` / `QueryInboundDataByTel` / `SearchInboundData` 四支端點，統一用查詢參數組合篩選條件。

### 8.3 通聯記錄詳細資料

- **方法與路徑**：`GET /call-records/{id}`
- **權限**：任何人

### 8.4 更新通聯記錄

- **方法與路徑**：`PATCH /call-records/{id}`
- **權限**：任何人
- **說明**：取代舊版 8 支個別欄位更新端點（`UpdateCallRecordTelNo.php`、`UpdateCallRecordTalkTime.php` 等），可一次更新任意組合的欄位，欄位同 8.1（`machineNo`、`callType` 除外皆可更新，`callType` 亦可更新）。

---

## 九、常見錯誤代碼對照

| HTTP 狀態碼 | 情境 |
|---|---|
| 400 | 一般錯誤（少見，多半會是下列更明確的代碼） |
| 401 | 未登入、Token 過期/無效、密碼錯誤 |
| 403 | 已登入但權限不足 |
| 404 | 查無資料或路徑不存在 |
| 409 | 資料衝突（例如員工編號、分機號碼重複） |
| 422 | 參數缺漏或不合法 |
| 500 | 伺服器內部錯誤（詳細原因僅記錄於伺服器 log，不會回傳給前端） |

---

## 十、Postman 測試集

已提供可直接匯入的 Postman collection，涵蓋以上所有 API 並內建登入後自動帶入 Token 的機制，位置：
```
ADICTICallSystem.API/postman/ADICTICallSystem.API.postman_collection.json
```
