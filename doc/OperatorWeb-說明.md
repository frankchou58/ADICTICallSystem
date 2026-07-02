# OperatorWeb（席位網頁）

2026-07-02 全面重寫。以純 HTML + CSS + JavaScript（無框架、無建置流程，跟 `ADICTICallCenter.Web` 同一套風格）取代舊版 frameset + jQuery + 舊版 PHP API 的席位（座席客服人員）網頁。

## 目錄結構

```
OperatorWeb/
├── index.html       進入點，依登入狀態導向 login.html 或 console.html
├── login.html        登入頁（統一用員工帳號登入，不再分「管理者/使用者」兩種表單）
├── console.html      主控台（單頁應用，分頁式切換，WebSocket 連線整個 App 生命週期只建立一次）
├── ADICTI.jpg         舊版留下的 logo 圖檔（新版目前沒有用到，先保留）
├── css/style.css
├── js/
│   ├── api-client.js    串接 ADICTICallSystem.API 的共用邏輯（跟 ADICTICallCenter.Web 同一套）
│   ├── ui.js             Toast 提示等共用小工具
│   ├── ws-client.js      連上 MFC 主機（ADICTICallCenter）即時通話事件 WebSocket
│   ├── board.js          「即時通話看板」分頁（座席核心畫面）
│   ├── customer-panel.js 來電螢幕彈出視窗 + 客戶查詢共用邏輯
│   ├── call-history.js   「通話紀錄」分頁
│   ├── inbound-report.js 「來電報表」分頁
│   ├── extension.js      「分機設定」分頁（自助綁定分機號碼）
│   ├── employees.js      「員工管理」分頁（僅 admin）
│   └── customers.js      「客戶管理」分頁（僅 admin）
└── 20220103/             舊版留下的通話錄音檔（執行期產生的資料，不是原始碼，維持不動）
```

## 跟舊版的關鍵差異

- **登入方式**：舊版靠使用者自己勾選「管理者」或「使用者」單選鈕決定要顯示哪種介面，沒有真正驗證身分。新版統一用 `POST /auth/login`，畫面上哪些分頁看得到完全依伺服器回傳的 `employee.role`（operator/supervisor/admin）決定。
- **API**：全部改打新版 `ADICTICallSystem.API`（Bearer Token、`{success,data,message,meta}` 回應格式），不再打會被淘汰的舊版 `D:\ADICTI\ADICTICallSystem\API\`（SQL 字串拼接、明文永久 Token、Big5 編碼）。
- **即時通話看板**：資料來源改成一次性用 REST API 抓「這個座位負責哪些虛擬外線」（依 `machineMask` 找機碼、`GET /outline-ports?machineNo=` 抓實體埠、依 `vport` 分組），不再是固定 1~240 的格狀圖。**通話事件本身還是走 WebSocket**（MFC 主機的 `WSServer.cpp`，預設 port 3000），這條路沒有被新版 REST API 取代，是分開的兩套機制，協定細節見 `js/ws-client.js` 開頭註解。
- **來電螢幕彈出**：舊版是整頁導頁（`top.location.href="index.html"` 換頁），新版改成彈出視窗蓋在任何分頁上面，操作員不會因為接到電話就跳離原本在看的畫面。
- **分機自助設定**：舊版是給沒登入的人掃 QR Code、輸入工號認領分機。新版每個座位本來就是登入狀態，直接用自己的 `employee.id` 打 `PATCH /employees/{id}` 更新 `extNum`，不用另外設計一套認領流程。
- **來電報表（InBoundList）**：舊版是半成品（打死 IP、表格渲染邏輯被拿掉）。新版直接擴充 `GET /call-records` 支援 `custName`/`custAddr`/`status` 篩選（`status` 是依 `call_type`/`connect_time` 推導出的「來電接聽/未接/撥出」分類，不是資料庫裡有一個獨立的狀態欄位）。
- **拿掉的死代碼**：舊版 `index.html` 的路由邏輯裡有一整套「Mode 4」健康監測儀表板（心率/血氧/體溫等圖表），對應的 HTML 檔案根本不存在，是別的產品線留下的殘骸，新版直接不處理。

## 重寫過程中一併修好的問題（不只是網頁本身）

這次重寫牽連到 MFC 主機（`ADICTICallCenter`）跟 API 好幾個之前沒被踩到的既有問題：

1. **MFC WebSocket 的身分識別會截斷新版 Token（`WSServer.cpp`）**：舊版身分識別是 32 字元的永久 UUID，`WSServer.cpp` 手刻的 WebSocket frame 解析器裡有兩個地方都假設身分字串固定 32 bytes（緩衝區大小、複製長度都寫死 32）。新版 API 的登入 Token 是 `bin2hex(random_bytes(32))` = 64 個字元，會被硬生生截斷成前 32 碼，導致 MFC 拿截斷後的假 token 去查 `/auth/me` 一定查不到人，**即時看板永遠收不到任何通話事件**。已修正：`WSCallBlock_T::OperatorUUID` 緩衝區從 33 bytes 放大到 72 bytes，複製長度改成照實際收到的長度來、並確實補上結尾的 0，順便把旁邊一個解碼緩衝區（`CHAR Buffer[100]`）也放大到 128（WS 短格式 payload 長度上限是 125，原本 100 也不夠）。已重新編譯確認可以build成功，**還沒有實機測試**，麻煩找一台子機重新登入測試看板功能。
2. **`CallRecordController::create()` 沒辦法正確處理空值欄位**：`$stmt->execute($array)` 這種寫法在這台機器的 `pdo_odbc` 底下，陣列裡的 PHP `null` 常常會被送成空字串而不是 SQL NULL，塞進 `INT`/`DATETIME2` 欄位會直接報 `Invalid character value for cast specification`。已改成用共用的 `Controller::executeWithParams()`（跟先前 `MachineController`/`CustomerController` 中文字修正時建立的同一套機制）明確依型別綁定。
3. **時間戳記格式**：`normalizeTimestamp()` 原本會產生 ISO 8601 的 `2026-07-01T10:00:00Z` 格式，這台機器的 ODBC Driver 18 不接受 `T`/`Z` 分隔符號做隱含轉型，一樣會報 cast 錯誤。已改成一律轉成 `Y-m-d H:i:s`（空白分隔，去掉 `Z`）。
4. **`GET /call-records` 新增的 `custAddr` 篩選一開始把 `LIMIT`/`OFFSET` 也弄壞了**：因為同一個具名參數（`:cust_addr`）在同一句 SQL 裡用了三次（`address`/`county`/`township` 各比對一次），`pdo_odbc` 不支援具名參數重複使用（底層 ODBC 只認位置式 `?`），造成後面 `:offset`/`:limit` 的綁定對不上位置，報 `FETCH 子句指定的資料列數目必須大於零`。已改成三個獨立參數名稱（`cust_addr1/2/3`）綁同一個值。
5. **`custName`/`custAddr` 的中文查詢字串比對不到資料**：這兩個新加的 `LIKE` 參數一開始用一般的 `bindValue()`，中文字一樣會踩到本 session 稍早在 `machines.alias`/`employees.name`/`customers.*` 上發現的同一個 `pdo_odbc` 內碼問題（綁字串用系統 ANSI 內碼、不是 UTF-8）。已改成透過 `Database::bindText()` 綁定。

以上第 2~5 點都已經用 curl 直接測試驗證過（新增通話紀錄、依中文姓名/地址查詢來電報表都正確）。第 1 點（MFC 端）目前只確認編譯成功，語意上應該正確，但沒有實機用真的子機連線驗證過，請務必實測。

## 部署方式

跟 `ADICTICallCenter.Web` 一樣是純靜態檔案，不需要建置流程。這次已經把 `D:\ADICTI\ADICTICallSystem\OperatorWeb` 的內容複製到 `D:\ADICTI\Server\OperatorWeb`（保留原本就在那邊的 `20220103/` 錄音資料夾沒有動）。

**還需要手動用系統管理員權限在 IIS 裡新增一個站台**（跟先前設定 `ADICTICallCenterWeb`/8842 站台的步驟一樣），指到 `D:\ADICTI\Server\OperatorWeb`，port 設成 **8844**：

1. IIS 管理員 → 新增網站
2. 站台名稱：`OperatorWeb`（或您習慣的命名）
3. 實體路徑：`D:\ADICTI\Server\OperatorWeb`
4. 繫結：連接埠 `8844`

## CORS 已設定

`ADICTICallSystem.API/config/config.local.php` 的 `cors.allowed_origins` 已加入 `http://localhost:8844`，跟 8841(API)/8842(ADICTICallCenter.Web) 並列。

## 已知落差 / 尚未測試

- **沒有實際在瀏覽器裡開起來操作測試過**：所有 `.js` 都用 `node --check` 驗證過語法沒問題，PHP 後端的擴充部分也用 curl 直接測過（包含中文篩選），但整個網頁的實際互動體感（分頁切換、彈出視窗、即時看板收到真的通話事件時的畫面）都還沒有在真實瀏覽器裡驗證。
- **即時看板依賴的 MFC WebSocket 修正還沒實機測試**（見上方第 1 點）。
- **測試資料殘留**：過程中為了驗證來電報表的中文篩選，在資料庫裡建了一筆測試客戶（電話 `0955000111`，姓名「測試客戶A」）跟三筆通話紀錄，目前沒有 DELETE 端點可以清掉，方便的話麻煩直接用 SQL 清除。
- **員工管理分頁的座位機碼指派（`machineMask`）與虛擬內線指派（`extVports`）**都用小勾選框顯示/切換，沒有做成更精緻的介面（跟 `ADICTICallCenter.Web` 的風格一致，走堪用但陽春的路線）。這是兩個獨立的欄位/機制：機碼指派決定外線看板可見範圍（也是 MFC 主機推播通話事件依據），虛擬內線指派決定內線看板可見範圍，兩者互不影響（詳見 `js/board.js` 開頭註解）。2026-07-02 補上機碼指派的勾選框 UI 之前，畫面上只有虛擬內線那一欄，機碼指派只能靠直接呼叫 API 設定。
- **通話紀錄/來電報表的「錄音檔」欄位**目前只顯示 `voiceRecordUuid` 字串本身，沒有做成可以直接播放/下載的連結（舊版是把 `D:\CPF24-REC\` 這種本機路徑字串替換成瀏覽器看得到的路徑，這次沒有實作，因為不確定新的錄音檔實際存放路徑/對外服務方式）。
