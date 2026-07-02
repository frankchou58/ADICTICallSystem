# ADICTICallSystem.API

這是舊版 `ADICTICallSystem/API`（PHP）的重寫版，目標環境是本機 MSSQL。這是**全新設計**，不是逐行搬移的相容版本——套用到既有前端之前，請先看下面的「與舊版的不相容之處」。

## 為什麼要重寫

針對舊版 `API/` 資料夾（`tools.php` + 約 60 支「一動作一檔案」端點）做設計審查後，發現以下問題，這個重寫版都已修正：

| 舊版問題 | 這裡的修法 |
|---|---|
| SQL 全部用字串拼接組成 | 100% 改用 PDO 參數化查詢(Prepared Statement) |
| 密碼從不落地儲存；用 MD5(帳號:密碼) 產生的值當成永久 Bearer Token，明碼帶在 URL 查詢參數上 | 改用 bcrypt 雜湊密碼 + 隨機產生的 Session Token（落地只存 SHA-256 雜湊值），走 `Authorization: Bearer` Header，依 `SESSION_TTL_SECONDS` 設定到期 |
| 直接把 `odbc_error()` 的原始錯誤訊息回傳給呼叫端 | 一律回覆通用錯誤訊息給客戶端；詳細內容只寫進伺服器端 log |
| `CREATE TABLE extline`/`log` 的欄位定義結尾多一個逗號 → 語法錯誤 | 已在 `sql/schema.sql` 修正 |
| `AddSupervisor()` 寫入的欄位名（`admin_uuid`）跟 `admin` 表實際欄位名（`supervisor_uuid`）對不上 | 移除 `admin` 表，改併入 `employees.role` |
| `Login.php` 查詢了 `operators` 表根本沒有的 `group_id` 欄位 | 已移除，新的 `employees` schema 沒有這個欄位 |
| `LinkExtToOperator()` 呼叫了早已從 PHP 移除的 `mysql_*` 系列函式 | 已移除，改用正常的外鍵欄位 `extlines.current_employee_id` |
| 完全沒有索引、沒有外鍵約束 | `sql/schema.sql` 已全面補上 |
| 每支 API 都回 `Access-Control-Allow-Origin: *`，GET 就能觸發寫入動作，也沒有 CSRF 防護 | CORS 改為白名單機制（`config.php` → `CORS_ALLOWED_ORIGINS`）；異動動作一律要求 `POST`/`PATCH`/`DELETE` 加上合法的 Bearer Token |
| 資料庫帳密寫死在 `tools.php` 原始碼裡 | 改由環境變數 / `config/config.local.php`（不進版本控制）讀取 |
| 沒有驗證機制的 `Create*Table.php` 端點，任何人都能觸發重建/清空整個 schema | 建置流程改成 CLI 腳本（`bin/setup.php`），不透過 HTTP 對外開放 |

以下是**實際部署過程中**額外發現並修掉的問題（不是舊版的問題，但值得記錄）：

| 部署時發現的問題 | 修法 |
|---|---|
| 程式碼原本用了 PHP 8.0+ 才有的聯合型別（`array\|false`）與 `mixed` 型別，在這台機器實際跑的 PHP 7.4 上直接語法錯誤 | 全部移除聯合型別/`mixed` 型別宣告，改用 PHPDoc 註解說明型別 |
| `config/config.php` 被三個不同檔案各自 `require`，PHP 在同一次請求中第二次 `require` 時會因為重複宣告 `env()` 函式而 Fatal Error | 集中改用 `App\Core\Config::all()`，內部只 `require` 一次並快取結果 |
| ……原本想改成 `require_once` 修正上面的 Fatal Error，結果變成另一個問題：`require_once` 第二次以後呼叫回傳的是 `true`，不是檔案原本的回傳值，導致設定值悄悄變成空的 | 用跟上面一樣的 `Config::all()` 解法，整個程式只有一個地方會真的執行 `require` |
| `bin/setup.php` 建立資料庫時用 `$pdo->quote()` 組 `IF DB_ID(...)` 判斷式，這台機器的 `PDO_ODBC` 驅動的 `quote()` 實作不可靠，靜默組出無效的 SQL——沒有丟出任何例外，但資料庫也從未真的被建立 | 拿掉 `quote()`，改成手動處理跳脫字元（因為資料庫名稱是來自設定檔而非外部輸入，安全無虞），並加上建立後的存在性檢查，失敗時會明確報錯而不是悄悄放過 |

## 環境需求

- PHP 7.4 以上（程式碼刻意寫成 7.4 和 8.1+ 都能執行——沒有用到聯合型別/`mixed` 等新語法）
- PHP 要能連 SQL Server，以下擇一：
  - **pdo_sqlsrv**（微軟官方的 PDO SQL Server 驅動），或
  - **pdo_odbc** + 主機上已有的任何 SQL Server ODBC 驅動（不管是舊版內建的「SQL Server」驅動，或是「ODBC Driver 17/18 for SQL Server」）。要走這條路的話，在設定檔裡把 `db.driver` 設成 `'odbc'`，並設定 `db.odbc_driver_name`——參考 `config/config.local.php.example`。
- 一個已經**開啟混合驗證模式**的 SQL Server 執行個體，以及一組具備 `dbcreator` 伺服器角色的 SQL 登入帳號（部署腳本會自己建立資料庫，所以需要這個權限）。
- 一台指向 `public/` 資料夾的網站伺服器（IIS + `web.config`，或 Apache + `.htaccess`，本機測試也可以直接用 `php -S localhost:8080 -t public`）。如果主機沒有 URL Rewrite / 萬用字元對應可用，請看下面「沒有 URL Rewrite 時的路由方式」。

## 本機部署步驟

1. 複製 `config/config.local.php.example` 成 `config/config.local.php`，填入本機 SQL Server 的連線資訊；或是設定對應的環境變數（`DB_DRIVER`、`DB_ODBC_DRIVER_NAME`、`DB_HOST`、`DB_NAME`、`DB_USER`、`DB_PASSWORD`）。
2. 執行一次部署腳本：
   ```
   php bin/setup.php --admin-no=admin --admin-password=YourInitialPassword123!
   ```
   這支腳本會建立資料庫（如果還不存在）、套用 `sql/schema.sql` 建表、寫入 `sql/seed.sql` 的種子資料（240 個虛擬外線、240 個虛擬內線、30 個子機槽位），並在 `employees` 表裡沒有這個員工編號時建立一個 `admin` 角色的帳號。如果沒有帶 `--admin-password`，腳本會自動產生一組隨機密碼並印出來一次——請立刻記下來，之後無法再次查詢，只能重設。
3. 把網站伺服器的文件根目錄指向 `public/`；或本機快速測試：
   ```
   php -S localhost:8080 -t public
   ```
4. 登入測試：
   ```
   curl -X POST http://localhost:8080/auth/login \
        -H "Content-Type: application/json" \
        -d '{"employeeNo":"admin","password":"YourInitialPassword123!"}'
   ```
   之後的請求都要在 Header 帶上回傳的 `token`：`Authorization: Bearer <token>`。

## 沒有 URL Rewrite 時的路由方式

`public/index.php` 會依序嘗試以下來源來決定要分派到哪個路由：`?route=` 查詢參數 → `PATH_INFO` → `REQUEST_URI`。如果主機沒有裝 IIS 的 URL Rewrite 模組，也沒有設定 PHP 的萬用字元指令碼對應，像 `/employees/5` 這種乾淨網址根本不會被送進 PHP——這時改用 `index.php?route=/employees/5` 的方式呼叫即可（該端點自己需要的查詢參數可以跟 `route` 一起放，例如 `index.php?route=/machines&machineType=1`）。有裝 URL Rewrite 的主機（參考 `public/.htaccess` / `public/web.config`）可以忽略這段，直接用乾淨網址。

## 在既有 IIS 站台底下部署、又不想更動站台實體路徑

如果不想去動既有 IIS 站台的實體路徑（例如該站台底下已經有其他子資料夾在跑別的應用程式），可以在站台底下新增一個資料夾，裡面**只放**一支轉接用的 `index.php`：

```php
<?php
require 'D:/ADICTI/ADICTICallSystem/ADICTICallSystem.API/public/index.php';
```

PHP 的 `__DIR__` 在被 `require` 進來的檔案裡，仍然會解析成該檔案自己實際所在的路徑，所以 App 內部所有用相對路徑寫的 `require` 都不受影響、照常運作。這樣可以讓 `config/`（含資料庫密碼）、`src/`、`sql/`、`bin/` 完全不會出現在 IIS 對外開放的資料夾底下——例如直接用網址存取 `/ADICTICallSystem.API/config/config.local.php` 會得到 404，因為該站台實體路徑底下根本沒有這個檔案。

## API 規格

所有回應都共用同一種格式：
```json
{ "success": true, "data": { ... }, "message": "...", "meta": {} }
```
發生錯誤時一樣用這個格式，只是 `success` 為 `false`、HTTP 狀態碼是非 2xx、`message` 會說明錯誤原因。除了 `POST /auth/login` 之外，其餘所有端點都需要帶 `Authorization: Bearer <token>`。

| 方法 | 路徑 | 說明 | 權限 |
|---|---|---|---|
| POST | `/auth/login` | `{ employeeNo, password }` → 取得 Session Token | 任何人 |
| POST | `/auth/logout` | 撤銷目前的 Token | 任何人 |
| GET | `/auth/me` | 查詢目前登入者的個人資料 | 任何人 |
| POST | `/employees` | 新增一位客服員/主管/管理員 | admin |
| GET | `/employees?role=&limit=&offset=` | 列出員工清單 | admin、supervisor |
| GET | `/employees/{id}` | 員工詳細資料 | 本人、admin、supervisor |
| PATCH | `/employees/{id}` | 更新姓名/角色/分機/停用狀態 | 本人（僅能改姓名）、admin（可改全部欄位） |
| PATCH | `/employees/{id}/password` | `{ currentPassword?, newPassword }` | 本人或 admin |
| POST | `/employees/{id}/machines/{machineNo}` | 綁定一個機碼（1~10） | admin |
| DELETE | `/employees/{id}/machines/{machineNo}` | 解除綁定機碼 | admin |
| POST | `/customers` | 新增客戶 | 任何人 |
| GET | `/customers?telNo=&blacklisted=&limit=&offset=` | 搜尋客戶 | 任何人 |
| GET | `/customers/{id}` | 客戶詳細資料 | 任何人 |
| GET | `/customers/by-tel/{telNo}` | 依電話號碼查最新一筆客戶 | 任何人 |
| PATCH | `/customers/{id}` | 更新客戶資料 | 任何人 |
| GET | `/machines?machineType=&machineNo=` | 列出子機槽位 | 任何人 |
| GET | `/machines/{machineType}/{machineNo}` | 子機槽位詳細資料 | 任何人 |
| PATCH | `/machines/{machineType}/{machineNo}` | `{ alias?, outPortCount?, extPortCount? }` | admin、supervisor |
| GET | `/outline-ports?machineType=&machineNo=&phyPort=&vport=` | 列出實體外線埠（一列一個實體埠，見下方「虛擬線路的第二版設計」） | 任何人 |
| GET | `/outline-ports/{id}` | 實體外線埠詳細資料 | 任何人 |
| GET | `/outline-ports/by-vport/{vport}` | 查詢共用這個虛擬編號的所有實體外線埠 | 任何人 |
| PATCH | `/outline-ports/{id}` | `{ vport?, inUse?, callStatus? }` | admin、supervisor |
| GET | `/extline-ports?machineType=&machineNo=&phyPort=&vport=` | 列出實體內線埠 | 任何人 |
| GET | `/extline-ports/{id}` | 實體內線埠詳細資料 | 任何人 |
| GET | `/extline-ports/by-vport/{vport}` | 查詢共用這個虛擬編號的所有實體內線埠 | 任何人 |
| GET | `/extline-ports/by-ext-num/{machineType}/{machineNo}/{extNum}` | 依分機號碼查詢 | 任何人 |
| PATCH | `/extline-ports/{id}` | `{ vport?, extNum?, status? }` | admin、supervisor |
| POST | `/call-records` | 新增一筆通聯記錄 | 任何人 |
| GET | `/call-records?machineNo=&telNo=&employeeId=&callType=&dateFrom=&dateTo=&limit=&offset=` | 搜尋通聯記錄 | 任何人 |
| GET | `/call-records/{id}` | 通聯記錄詳細資料 | 任何人 |
| PATCH | `/call-records/{id}` | 更新任意欄位（隨通話狀態演進逐步更新） | 任何人 |

`machineType`：`1` = 交換機(PBX)、`2` = 來電盒(CallerID Box)、`3` = 語音卡(Voice Card)。`callType`：`0` = 外線撥入、`1` = 外線撥出、`2` = 內線互打。

「任何人」指的是任何已登入（帶有效 Token）的員工，不分角色。

## 與舊版的不相容之處

這次是刻意的全新設計，不是保留相容性的搬移，所以既有 `Web/` 資料夾裡的 HTML/JS 前端的 AJAX 呼叫都需要跟著調整：

- **端點形式**：從「一動作一檔案」改成 RESTful 資源路徑 + HTTP 動詞（例如 `SetMachineAlias.php`＋`SetMachineOutPorts.php`＋`SetMachineExtPorts.php` 三支，合併成一支 `PATCH /machines/{type}/{no}`）。
- **身分驗證**：登入後拿到的是短效期的 `token`，透過 `Authorization: Bearer <token>` 使用，不再是永久有效、放在 URL 查詢參數裡的 UUID。Token 依 `SESSION_TTL_SECONDS`（預設 8 小時）到期，也可以呼叫 `/auth/logout` 主動撤銷。
- **回應格式**：改成 `{ success, data, message, meta }`，不是舊版的 `{ status, esptime, fields, data }`。
- **通聯記錄更新**：舊版 8 支各自更新單一欄位的 `UpdateCallRecordXxx.php`，全部合併成一支 `PATCH /call-records/{id}`，可以一次更新任意組合的欄位。
- **管理員 vs 客服員**：不再有獨立的管理員登入/資料表，管理員就只是 `role` 為 `"admin"` 的一位員工。
- **建表方式**：`Create*Table.php` 系列端點已移除，改成執行 `php bin/setup.php`（每個環境跑一次即可）。

## 虛擬線路的第二版設計（outline_ports / extline_ports）

這是上線後才發現需要調整、已經套用的第二次設計改版，取代了最早那版「固定 240 筆虛擬外線/內線」的做法。

**背景**：ADICTICallCenter 可以接 3 種類型的子機（1=PBX 交換機、2=來電盒、3=語音卡），每個子機碼（1~10，代表一個實體地點）底下這三種類型可以任意組合、任意數量共存（含三種同時存在），彼此互相獨立設定內外線數量，唯一的限制是只有 PBX 有內線(分機)功能。因為同一條實體電話線可能同時被 PBX、來電盒、語音卡三種不同硬體各自用自己的實體埠編號監看，所以需要一個「虛擬編號」把這幾種各自獨立的實體埠編號統一起來，畫面上才能用同一個號碼代表同一條實體線路。

**第一版做法（已捨棄）**：`outlines`/`extlines` 兩張表各自預先塞好固定 240 筆虛擬編號（1~240），每一列有三個欄位（`pbx_phy_port`/`callerid_box_phy_port`/`voice_card_phy_port`）分別放三種類型各自對應的實體埠。問題：240 是寫死的天花板；不管實際設定多少實體埠，永遠佔好 240 筆空間；虛擬編號本身變成一個要手動維護的獨立資源，跟子機實際的埠數設定要手動保持同步。

**第二版做法（目前採用）**：改成「一個實體埠一列」——`outline_ports`/`extline_ports` 每一列代表一個 `(machine_type, machine_no, phy_port)` 的實體埠，虛擬編號(`vport`)只是這一列上的一個**普通欄位**（可為 NULL，代表還沒指派），不是一個獨立的資源池：

```
outline_ports:  id, machine_type, machine_no, phy_port, vport, in_use, call_status
extline_ports:  id, machine_type, machine_no, phy_port, vport, ext_num, status, current_employee_id
```

好處：
- 虛擬線路的總數就是「目前所有子機實際設定的實體埠數量總和」，沒有 240 這種寫死的天花板，也沒有用不到的空位。
- 同一個 `vport` 值可以同時出現在 PBX、來電盒、語音卡三種類型各自的列上——這正是「虛擬」真正要表達的意思：這幾種設備正在監看同一條實體線路。用 `GET /outline-ports/by-vport/{vport}` 可以一次查出目前有哪些設備共用這個編號。
- 子機在「交換機/來電盒/語音卡類型」頁面把外線或內線數量從 8 改成 12，`MachineController` 會自動幫這個子機補 4 筆新的 `outline_ports`/`extline_ports` 列（`vport` 先是 NULL）；改小數量則會刪除多出來的實體埠列（`MachineController::syncPorts()`）。虛擬編號的指派（含跨類型共用同一編號）是後續手動或前端操作的步驟，新增列時系統只會先給一個依序遞增、不重複的預設編號。
- 因為任何類型組合都合法，`MachineController` 不再檢查「來電盒跟語音卡不能同時存在」或「外線數不能超過 PBX」這類跨類型規則——這兩條規則是第一次重寫時看著舊版 MFC UI 的行為描述誤加上去的，跟原始規劃書其實不一致，這次順便拿掉。

## 資料庫 Schema

詳見 `sql/schema.sql`（資料表定義）與 `sql/seed.sql`（僅 30 筆機碼種子資料，`outline_ports`/`extline_ports` 不預先塞資料，由 `MachineController` 依實際設定動態建立）。與舊版 `tools.php` schema 的主要差異已整理在上方表格中；另外要注意時間戳記一律存成 `DATETIME2`，而不是原始的 32 位元 Unix 時間戳，避免在資料庫層再疊加一次通訊協定原本就有的 2038 年溢位問題。
