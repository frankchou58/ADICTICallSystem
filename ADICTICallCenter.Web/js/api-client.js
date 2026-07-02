/**
 * ADICTICallSystem.API 用戶端包裝。
 *
 * 因為目前部署環境（IIS，無 URL Rewrite 模組）沒辦法用乾淨路徑呼叫，
 * 所有請求一律組成 <baseUrl>?route=<path>&<其他查詢參數> 的形式，
 * 詳見 doc/README.md「沒有 URL Rewrite 時的路由方式」。
 *
 * baseUrl 預設可在 settings 頁面覆寫，存在 localStorage。
 */
const Api = (() => {
  // 用目前網頁本身的主機名稱組出預設 API 位址，而不是寫死 localhost：
  // 如果從別台電腦用區網 IP（例如 http://192.168.201.239:8842/）開這個
  // 網頁，寫死 localhost 會讓瀏覽器打 fetch 時連到自己身上，不是真正
  // 跑 API 的那台主機，直接 "Failed to fetch"。API 站台預期跟這個網頁
  // 部署在同一台主機上，只差 port。
  const DEFAULT_BASE_URL = `http://${window.location.hostname}:8841/ADICTICallSystem.API/index.php`;
  const TOKEN_KEY = 'adicti.token';
  const BASE_URL_KEY = 'adicti.baseUrl';

  function getBaseUrl() {
    return localStorage.getItem(BASE_URL_KEY) || DEFAULT_BASE_URL;
  }

  function setBaseUrl(url) {
    localStorage.setItem(BASE_URL_KEY, url);
  }

  function getToken() {
    return localStorage.getItem(TOKEN_KEY);
  }

  function setToken(token) {
    if (token) localStorage.setItem(TOKEN_KEY, token);
    else localStorage.removeItem(TOKEN_KEY);
  }

  function isLoggedIn() {
    return !!getToken();
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

    const headers = { 'Content-Type': 'application/json' };
    const token = getToken();
    if (token) headers['Authorization'] = `Bearer ${token}`;

    const response = await fetch(`${getBaseUrl()}?${params.toString()}`, {
      method,
      headers,
      body: options.body ? JSON.stringify(options.body) : undefined,
    });

    let json;
    try {
      json = await response.json();
    } catch (e) {
      throw new ApiError('伺服器回應不是有效的 JSON', response.status, null);
    }

    if (response.status === 401) {
      setToken(null);
      if (!route.startsWith('/auth/login')) {
        window.location.href = 'login.html';
      }
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
    getToken,
    setToken,
    isLoggedIn,
    get: (route, query) => request('GET', route, { query }),
    post: (route, body) => request('POST', route, { body }),
    patch: (route, body) => request('PATCH', route, { body }),
    del: (route) => request('DELETE', route),
    ApiError,
  };
})();
