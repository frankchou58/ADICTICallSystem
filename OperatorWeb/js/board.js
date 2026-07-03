/**
 * 即時通話看板：座席登入後預設看到的畫面。
 *
 * 資料來源分兩塊：
 *   - 座席負責哪些虛擬外線：開頭用 REST API 查一次（依 machineMask 找出
 *     負責哪些機碼，再抓每個機碼底下的實體外線埠，依 vport 分組成一個個
 *     看板方塊）。machineMask 這個欄位 MFC 主機（WSServer.cpp）也會拿去
 *     判斷要把通話事件推播給哪個座席，不能亂改語意，所以虛擬外線看板
 *     繼續沿用它。
 *   - 座席負責哪些虛擬內線：跟虛擬外線是分開的獨立指派，來自員工管理
 *     頁面設定的 employee.extVports（對應 dbo.employee_ext_lines 表），
 *     不受 machineMask 限制。
 *   - 每個方塊當下的狀態：靠 WsClient 收 MFC 主機（ADICTICallCenter）推播
 *     的即時通話事件更新，不是用輪詢 REST API 做的。
 *
 * 通話事件協定細節見 js/ws-client.js 開頭的註解。
 */
const BoardTab = (() => {
  let initialized = false;
  let myEmployeeId = null;
  let myExtNum = null;
  const extNameCache = new Map(); // extNum -> employee name（含查不到的 null 快取）

  async function resolveMyProfile() {
    const stored = JSON.parse(sessionStorage.getItem('adicti.operator.employee') || 'null');
    if (!stored) throw new Error('找不到登入資料');
    // 重新查一次最新的分機/機碼設定，避免登入之後被管理員改過還沒反映。
    const fresh = await Api.get(`/employees/${stored.id}`);
    myEmployeeId = fresh.id;
    myExtNum = fresh.extNum;
    return fresh;
  }

  async function fetchOwnedOutlinePorts(machineMask) {
    const requests = [];
    for (let machineIndex = 0; machineIndex < 10; machineIndex++) {
      if ((machineMask & (1 << machineIndex)) !== 0) {
        requests.push(Api.get('/outline-ports', { machineNo: machineIndex + 1 }));
      }
    }
    const results = await Promise.all(requests);
    return results.flat();
  }

  async function fetchOwnedExtlinePorts(extVports) {
    const requests = (extVports || []).map((vport) => Api.get('/extline-ports', { vport }));
    const results = await Promise.all(requests);
    return results.flat();
  }

  function groupByVport(ports) {
    const byVport = new Map();
    ports.forEach((p) => {
      if (p.vport === null || p.vport === undefined) return;
      if (!byVport.has(p.vport)) byVport.set(p.vport, []);
      byVport.get(p.vport).push(p);
    });
    return byVport;
  }

  const MACHINE_TYPE_LABEL = { 1: 'PBX', 2: '來電盒', 3: '語音卡' };

  function renderGrid(byVport) {
    const grid = document.getElementById('boardGrid');
    grid.innerHTML = '';
    const vports = Array.from(byVport.keys()).sort((a, b) => a - b);
    vports.forEach((vport) => {
      const ports = byVport.get(vport);
      const subtitle = ports.map((p) => `${MACHINE_TYPE_LABEL[p.machineType] || p.machineType}${p.machineNo}-${p.phyPort}`).join(', ');
      const box = document.createElement('div');
      box.className = 'board-box';
      box.id = `box-${vport}`;
      box.innerHTML = `
        <div class="box-title">虛擬外線 ${String(vport).padStart(3, '0')}</div>
        <div class="box-status" id="box-status-${vport}">待機</div>
        <div class="box-callid" id="box-callid-${vport}"></div>
        <div class="box-meta">
          <span id="box-ext-${vport}"></span>
          <span id="box-operator-${vport}"></span>
        </div>
        <div class="box-address" id="box-address-${vport}">${subtitle}</div>
      `;
      grid.appendChild(box);
    });
    document.getElementById('boardHint').textContent = vports.length
      ? `共 ${vports.length} 條虛擬外線`
      : '目前沒有指派任何座位機碼，請聯絡管理員設定。';
  }

  function renderExtGrid(byVport) {
    const grid = document.getElementById('extBoardGrid');
    grid.innerHTML = '';
    const vports = Array.from(byVport.keys()).sort((a, b) => a - b);
    vports.forEach((vport) => {
      const ports = byVport.get(vport);
      const extNums = ports.map((p) => p.extNum).filter((n) => n !== null && n !== undefined);
      const subtitle = ports.map((p) => `${MACHINE_TYPE_LABEL[p.machineType] || p.machineType}${p.machineNo}-${p.phyPort}`).join(', ');
      const box = document.createElement('div');
      box.className = 'board-box';
      box.id = `extbox-${vport}`;
      box.innerHTML = `
        <div class="box-title">虛擬內線 ${String(vport).padStart(3, '0')}</div>
        <div class="box-status" id="extbox-status-${vport}">待機</div>
        <div class="box-callid" id="extbox-callid-${vport}"></div>
        <div class="box-meta">
          <span id="extbox-ext-${vport}">${extNums.length ? `分機 ${extNums.join(', ')}` : ''}</span>
          <span id="extbox-operator-${vport}"></span>
        </div>
        <div class="box-address" id="extbox-address-${vport}">${subtitle}</div>
      `;
      grid.appendChild(box);
    });
  }

  async function resolveExtName(extNum) {
    if (!extNum) return null;
    if (extNameCache.has(extNum)) return extNameCache.get(extNum);
    try {
      const emp = await Api.get(`/employees/by-ext/${extNum}`);
      extNameCache.set(extNum, emp.name || emp.employeeNo);
      return extNameCache.get(extNum);
    } catch (err) {
      extNameCache.set(extNum, null);
      return null;
    }
  }

  const CALL_STATUS_LABEL = { 1: '撥入', 2: '撥出', 3: '通話', 4: '待機' };

  // 虛擬內線看板：外線/內線通話都會回報 extVPort（見 ws-client.js 開頭
  // 註解），跟虛擬外線看板是各自獨立的兩組方塊，用不同的 id 前綴
  // （extbox- vs box-），同一通電話會同時更新兩塊看板。
  function updateExtBox(call) {
    if (!call.extVPort || call.extVPort <= 0) return; // -1 或 undefined：舊主機或這通電話沒有對應的虛擬內線
    const statusEl = document.getElementById(`extbox-status-${call.extVPort}`);
    if (!statusEl) return; // 不是這個座位負責的虛擬內線，忽略

    const callIdEl = document.getElementById(`extbox-callid-${call.extVPort}`);
    const extEl = document.getElementById(`extbox-ext-${call.extVPort}`);
    const operatorEl = document.getElementById(`extbox-operator-${call.extVPort}`);

    if (call.callStatus === 4) {
      statusEl.textContent = '待機';
      statusEl.style.background = '#bbbbbb';
      statusEl.style.color = '#000';
      callIdEl.textContent = '';
      callIdEl.style.background = '';
      return;
    }

    const bg = call.callStatus === 3 ? '#82FF82' : call.callStatus === 1 ? '#FFC9C9' : '#FFFFBC';
    statusEl.textContent = CALL_STATUS_LABEL[call.callStatus] || '';
    statusEl.style.background = bg;
    statusEl.style.color = '#000';
    callIdEl.style.background = bg;
    callIdEl.textContent = call.telNo || '';

    if (call.extNum) {
      extEl.textContent = `分機 ${call.extNum}`;
      resolveExtName(call.extNum).then((name) => {
        operatorEl.textContent = name ? `客服: ${name}` : '';
      });
    }
  }

  async function handleCall(call) {
    updateExtBox(call);

    const statusEl = document.getElementById(`box-status-${call.vport}`);
    if (!statusEl) return; // 不是這個座位負責的虛擬外線，忽略

    const callIdEl = document.getElementById(`box-callid-${call.vport}`);
    const extEl = document.getElementById(`box-ext-${call.vport}`);
    const operatorEl = document.getElementById(`box-operator-${call.vport}`);

    if (call.callStatus === 4) {
      // 待機：清空顯示內容
      statusEl.textContent = '待機';
      statusEl.style.background = '#bbbbbb';
      statusEl.style.color = '#000';
      callIdEl.textContent = '';
      callIdEl.style.background = '';
      extEl.textContent = '';
      operatorEl.textContent = '';
      return;
    }

    // 顏色邏輯：先依 CallType 給預設色，CallStatus 為撥入/通話中時蓋掉。
    // 2026-07-03：這台部署沒有 customers 資料表，拿掉了黑名單來電判斷
    // （原本會查詢客戶資料決定要不要標黑名單顏色）。
    let bg = call.callType === 0 ? '#1FD3AB' : call.callType === 1 ? '#FFFFBC' : '#E7B0E8';
    let color = '#000';
    if (call.callStatus === 1) bg = '#FFC9C9';
    else if (call.callStatus === 3) bg = '#82FF82';

    statusEl.textContent = CALL_STATUS_LABEL[call.callStatus] || '';
    statusEl.style.background = bg;
    statusEl.style.color = color;
    callIdEl.style.background = bg;
    callIdEl.style.color = color;
    callIdEl.textContent = call.telNo || '';
    document.getElementById(`box-address-${call.vport}`).textContent = '';

    if (call.extNum) {
      extEl.textContent = `分機 ${call.extNum}`;
      resolveExtName(call.extNum).then((name) => {
        operatorEl.textContent = name ? `客服: ${name}` : '';
      });
    }

    // 螢幕彈出：自己的分機正在通話（外線接通）或正在撥出時，自動彈出
    // 客戶資料，不用手動切分頁 - 跟舊版 OutPortsStatus.html 的邏輯一致。
    if (myExtNum && call.extNum === myExtNum) {
      const shouldPop = (call.callStatus === 3 && call.callType === 0) || (call.callStatus === 2 && call.callType === 1);
      if (shouldPop) {
        CustomerPanel.showForCall(call.telNo);
      }
    }
  }

  async function init() {
    if (initialized) return;
    initialized = true;
    try {
      const profile = await resolveMyProfile();
      const [ports, extPorts] = await Promise.all([
        fetchOwnedOutlinePorts(profile.machineMask || 0),
        fetchOwnedExtlinePorts(profile.extVports || []),
      ]);
      renderGrid(groupByVport(ports));
      renderExtGrid(groupByVport(extPorts));
      WsClient.onCall(handleCall);
    } catch (err) {
      Ui.handleError(err);
      document.getElementById('boardHint').textContent = '看板載入失敗：' + err.message;
    }
  }

  return { init };
})();
