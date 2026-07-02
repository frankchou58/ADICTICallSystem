# ADICTICallCenter.Web

以純 HTML + CSS + JavaScript（無框架、無建置流程）重現 MFC 版 `ADICTICallCenter` 監控台的**設定/監控介面**，對接 `ADICTICallSystem.API`。不包含 MFC 版的背景核心服務（Machine Server、WebSocket Server、通話狀態引擎）——那些是處理真實電話硬體事件的常駐服務，沒辦法用網頁重現，仍然由 MFC 版 `ADICTICallCenter.exe` 負責。

> **2026-07-01 更新**：API 端的虛擬外線/內線資料模型改版（`outlines`/`extlines` 固定 240 筆 → `outline-ports`/`extline-ports` 一個實體埠一列，詳見 `doc/README.md`「虛擬線路的第二版設計」），`machines.js`/`console.html` 已經跟著同步更新。子機頁面下方原本「格狀圖點格子編輯」的介面改成表格列表，每一列是一個實體埠，虛擬編號欄位可以直接輸入數字。
>
> 「虛擬內外線狀態」分頁（原本對應 `dashboard.js`）已移除，不再顯示於 Web 版；`js/dashboard.js` 檔案還留著但 `console.html` 不再引用。目前預設分頁改成「交換機類型」。MFC 版另外已用 WebView2 內嵌這個 Web 版，多一個「網頁版控制台」分頁（詳見 `ADICTICallCenter-MFC改版說明.md`）。

## 目錄結構

```
ADICTICallCenter.Web/
├── index.html       進入點，依登入狀態導向 login.html 或 console.html
├── login.html        登入頁
├── console.html      主控台（分頁：交換機類型｜來電盒類型｜語音卡類型｜設定）
├── css/style.css
└── js/
    ├── api-client.js  串接 ADICTICallSystem.API 的共用邏輯
    ├── ui.js          Toast 提示等共用小工具
    ├── dashboard.js   （未使用）「虛擬內外線狀態」分頁移除後不再被 console.html 引用，檔案保留
    ├── machines.js    「交換機/來電盒/語音卡類型」三個分頁共用（用 machineType 參數區分）
    └── settings.js    「設定」分頁
```

## 對應 MFC 分頁的實作範圍

| MFC 分頁 | Web 版對應 | 說明 |
|---|---|---|
| 虛擬內外線狀態 | 無（已移除） | Web 版原本有對應分頁，因為資料只是快照、不是即時通話燈號，容易跟 MFC 版的即時顯示搞混，已拿掉。 |
| 交換機類型 | 「交換機類型」分頁 | 上方表格可編輯別名/外線數量/內線數量；下方格狀圖可編輯虛擬內線的分機號碼。 |
| 來電盒類型 | 「來電盒類型」分頁 | 上方表格可編輯別名/外線數量；下方格狀圖可編輯該來電盒的實體外線對應到哪個虛擬外線編號。 |
| 語音卡類型 | 「語音卡類型」分頁 | 同來電盒類型，換成語音卡的實體外線對應。 |
| 設定 | 「設定」分頁 | 只有「API 連線位址」這一項是這個 Web 版能管理的設定。 |

## 已知落差（跟 MFC 版行為不同的地方）

- **沒有即時推播**：MFC 版透過 WebSocket 即時收到通話狀態變化；這個 Web 版只在載入分頁或按「重新整理」時，用 REST API 抓一次當下的快照。如果需要真正即時的通話監看，仍要用 MFC 主控台。
- **「電話號碼決定優先」設定沒有對應的 API**：這個設定目前只存在於 MFC 主核心程式的登錄檔（`SystemSetting`），新版 `ADICTICallSystem.API` 沒有提供讀寫這個設定的端點，所以 Web 版的「設定」分頁沒有這個選項。
- **虛擬外線/內線的「清除指派」沒有做**：格狀圖裡的輸入框目前只支援「指派」一個實體埠或分機號碼；清空輸入框不會真的把伺服器上的值清掉（因為新版 API 目前的 `PATCH` 語意是「有帶欄位才更新」，沒有「明確設成空值」的方式）。如果需要解除指派，暫時還是要透過 Postman 直接呼叫 API，或之後再擴充。
- **子機版本號、IP 位址欄位沒有顯示**：新版 API 的 `machines` 資料裡雖然有 `swVersion`/`ipAddress` 欄位，但目前都是空值（要等子機端程式改連新協定並回報才會有資料），畫面上先省略。

## 部署方式

因為是純靜態檔案，不需要建置流程，直接把整個 `ADICTICallCenter.Web` 資料夾複製到 IIS 網站底下（或另建一個新站台）指向這個資料夾即可，IIS 預設就會處理 `index.html` 當首頁。

⚠️ **務必確認 CORS 設定**：如果這個 Web 版跟 `ADICTICallSystem.API` 不是同一個「來源」（scheme+host+port 三者都要完全一樣才算同源），瀏覽器會擋掉 API 請求。需要到 `ADICTICallSystem.API/config/config.local.php`（或環境變數 `CORS_ALLOWED_ORIGINS`）把這個 Web 版實際的網址加進允許清單，例如：
```php
'cors' => [
    'allowed_origins' => ['http://localhost:8841', 'http://your-web-host:port'],
],
```

部署完成後，記得到「設定」分頁確認/調整 `js/api-client.js` 裡的 `DEFAULT_BASE_URL`（或直接在畫面上的「設定」分頁修改，存在瀏覽器 localStorage）指向正確的 API 位址。

## 這份程式碼沒有在真實瀏覽器裡測試過

已用 Node.js 對所有 `.js` 檔做過語法檢查（`node --check`），確認沒有語法錯誤，但**沒有實際在瀏覽器裡打開來操作測試過**（例如沒有驗證 CORS 實際放行、沒有驗證表單互動的實際使用體感）。建議部署後先用一個測試帳號完整跑過一輪：登入 → 切換四個分頁 → 編輯別名/外線數量 → 編輯分機號碼/實體埠指派 → 登出，確認沒有問題再正式使用。
