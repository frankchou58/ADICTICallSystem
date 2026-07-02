/**
 * 連上 ADICTICallCenter（MFC 主機）的即時通話事件 WebSocket（預設 port 3000）。
 *
 * 協定（跟 WSServer.cpp 的格式一致，不是 ADICTICallSystem.API 的一部分）：
 *   - 連上後第一則訊息要送目前登入的 Bearer Token（純文字，不是 JSON），
 *     主機端拿它去查 GET /auth/me 確認這個 WebSocket session 屬於哪個員工。
 *   - 之後主機端會推播 `{"data":["MachineID","CallType","CallStatus",
 *     "PhyOutPort","VPort","CallerId","ExtNum","ExtVPort"]}` 這種訊息：
 *       CallType:   0=外線撥入 1=外線撥出 2=內線互撥
 *       CallStatus: 1=撥入 2=撥出 3=通話 4=待機
 *       VPort:      虛擬外線編號（對應 dbo.outline_ports.vport）；內線互撥
 *                   時這欄固定是 -1，因為內線沒有外線埠可言。
 *       ExtVPort:   虛擬內線編號（對應 dbo.extline_ports.vport），外線/內線
 *                   通話都會有值，用來同步更新「虛擬內線」看板。
 *
 * 這是舊版 OperatorWeb 沿用下來的協定，跟新版 REST API 是分開的兩條路，
 * 新版 API 目前沒有對應的即時事件端點。
 */
const WsClient = (() => {
  const HOST_KEY = 'adicti.operator.wsHost';
  // 跟 api-client.js 一樣，用網頁本身的主機名稱組預設值，不要寫死
  // localhost，否則座席電腦用區網 IP 開網頁時會連到自己身上。
  const DEFAULT_HOST = `${window.location.hostname}:3000`;

  let socket = null;
  let reconnectTimer = null;
  let manuallyClosed = false;
  const listeners = new Set();
  const statusListeners = new Set();
  let lastStatus = 'down';

  function getHost() {
    return localStorage.getItem(HOST_KEY) || DEFAULT_HOST;
  }

  function setHost(host) {
    localStorage.setItem(HOST_KEY, host);
  }

  function setStatus(status) {
    lastStatus = status;
    statusListeners.forEach((fn) => fn(status));
  }

  function connect() {
    manuallyClosed = false;
    clearTimeout(reconnectTimer);
    setStatus('connecting');

    try {
      socket = new WebSocket(`ws://${getHost()}`);
    } catch (e) {
      setStatus('down');
      scheduleReconnect();
      return;
    }

    socket.onopen = () => {
      const token = Api.getToken();
      if (token) socket.send(token);
      setStatus('up');
    };

    socket.onmessage = (event) => {
      let parsed;
      try {
        parsed = JSON.parse(event.data);
      } catch (e) {
        return;
      }
      if (!parsed || !Array.isArray(parsed.data)) return;
      const [machineId, callType, callStatus, phyPort, vport, telNo, extNum, extVPort] = parsed.data;
      const call = {
        machineId: Number(machineId),
        callType: Number(callType),
        callStatus: Number(callStatus),
        phyPort: Number(phyPort),
        vport: Number(vport),
        telNo: telNo,
        extNum: Number(extNum),
        // 舊版主機（尚未更新 WSServer.cpp）不會送這個欄位，這裡用
        // undefined 而非 NaN，方便 board.js 判斷「這台主機支不支援」。
        extVPort: extVPort !== undefined ? Number(extVPort) : undefined,
      };
      listeners.forEach((fn) => fn(call));
    };

    socket.onclose = () => {
      setStatus('down');
      if (!manuallyClosed) scheduleReconnect();
    };

    socket.onerror = () => {
      setStatus('down');
    };
  }

  function scheduleReconnect() {
    clearTimeout(reconnectTimer);
    reconnectTimer = setTimeout(connect, 3000);
  }

  function disconnect() {
    manuallyClosed = true;
    clearTimeout(reconnectTimer);
    if (socket) socket.close();
  }

  function onCall(fn) {
    listeners.add(fn);
    return () => listeners.delete(fn);
  }

  function onStatus(fn) {
    statusListeners.add(fn);
    fn(lastStatus);
    return () => statusListeners.delete(fn);
  }

  return { getHost, setHost, connect, disconnect, onCall, onStatus };
})();
