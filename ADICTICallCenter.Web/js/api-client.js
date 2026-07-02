/**
 * ADICTICallSystem.API 用戶端包裝。
 *
 * 因為目前部署環境（IIS，無 URL Rewrite 模組）沒辦法用乾淨路徑呼叫，
 * 所有請求一律組成 <baseUrl>?route=<path>&<其他查詢參數> 的形式，
 * 詳見 doc/README.md「沒有 URL Rewrite 時的路由方式」。
 *
 * baseUrl 預設可在 settings 頁面覆寫，存在 localStorage。
 *
 * 2026-07-03：這個網站不再要求登入（見 doc/ADICTICallCenter.Web-說明.md），
 * 呼叫的 /machines、/outline-ports、/extline-ports 這幾組端點後端也已經
 * 拿掉 Bearer Token 驗證，所以這裡不再處理 token/登入相關邏輯。
 */
const Api = (() => {
  // 用目前網頁本身的主機名稱組出預設 API 位址，而不是寫死 localhost：
  // 如果從別台電腦用區網 IP（例如 http://192.168.201.239:8842/）開這個
  // 網頁，寫死 localhost 會讓瀏覽器打 fetch 時連到自己身上，不是真正
  // 跑 API 的那台主機，直接 "Failed to fetch"。API 站台預期跟這個網頁
  // 部署在同一台主機上，只差 port。
  const DEFAULT_BASE_URL = `http://${window.location.hostname}:8841/ADICTICallSystem.API/index.php`;
  const BASE_URL_KEY = 'adicti.baseUrl';

  function getBaseUrl() {
    return localStorage.getItem(BASE_URL_KEY) || DEFAULT_BASE_URL;
  }

  function setBaseUrl(url) {
    localStorage.setItem(BASE_URL_KEY, url);
  }

  /**
   * @param {string} method GET/POST/PATCH/DELETE
   * @param {string} route 例如 "/machines/1/2"
   * @param {object} [options]
   * @param {object} [options.query] 額外查詢參數物件，例如 { machineType: 1 }
   * @param {object} [options.body] JSON body（POST/PATCH 用）
   */
  async function request(method, route, options = {}) {
    const params = new URLSearchParams();
    params.set('route', route);
    if (options.query) {
      Object.entries(options.query).forEach(([key, value]) => {
        if (value !== undefined && value !== null && value !== '') {
          params.set(key, value);
        }
      });
    }

    const response = await fetch(`${getBaseUrl()}?${params.toString()}`, {
      method,
      headers: { 'Content-Type': 'application/json' },
      body: options.body ? JSON.stringify(options.body) : undefined,
    });

    let json;
    try {
      json = await response.json();
    } catch (e) {
      throw new ApiError('伺服器回應不是有效的 JSON', response.status, null);
    }

    if (!json.success) {
      throw new ApiError(json.message || '發生錯誤', response.status, json);
    }

    return json.data;
  }

  class ApiError extends Error {
    constructor(message, status, envelope) {
      super(message);
      this.status = status;
      this.envelope = envelope;
    }
  }

  return {
    getBaseUrl,
    setBaseUrl,
    get: (route, query) => request('GET', route, { query }),
    post: (route, body) => request('POST', route, { body }),
    patch: (route, body) => request('PATCH', route, { body }),
    del: (route) => request('DELETE', route),
    ApiError,
  };
})();
