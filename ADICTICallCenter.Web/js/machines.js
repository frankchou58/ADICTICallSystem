/**
 * 「交換機類型」「來電盒類型」「語音卡類型」三個分頁共用的邏輯，
 * 差異只在 machineType（1=PBX、2=CallerID Box、3=Voice Card）跟
 * 是否顯示內線埠位設定（只有 PBX 有）。
 *
 * 埠位列表改成「一個實體埠一列」，虛擬編號(vport)是這一列上的一個可
 * 編輯欄位，不是固定 1~240 的格狀圖——列表裡的列數就等於這個類型目前
 * 實際設定的實體埠總數，新增/刪除實體埠列是伺服器在 outPortCount/
 * extPortCount 變動時自動處理，這裡只負責顯示與編輯 vport/分機號碼。
 */
const MACHINE_TYPE_LABEL = { 1: '交換機', 2: '來電盒', 3: '語音卡' };
const REJECT_FLASH_MS = 650; // 要跟 css/style.css 的 .field-rejected 動畫時間一致

function createMachinesTab(machineType, elIds) {
  const hasExtPorts = machineType === 1;

  async function loadMachineTable() {
    const tbody = document.querySelector(`#${elIds.table} tbody`);
    tbody.innerHTML = '';
    try {
      // 抓「全部類型」而不是只抓自己這個 machineType，才能知道同一個
      // 合併機碼有沒有已經被別的類型占用——後端規則是同一個機碼同時間
      // 只能有一種類型的外線或內線數量不是 0，這裡先在畫面上做防呆
      // （鎖住輸入框、顯示提示），不要等使用者按下去才收到後端錯誤。
      const allMachines = await Api.get('/machines');
      const byNo = new Map();
      allMachines.forEach((m) => {
        if (!byNo.has(m.machineNo)) byNo.set(m.machineNo, []);
        byNo.get(m.machineNo).push(m);
      });

      const machines = allMachines.filter((m) => m.machineType === machineType);
      machines.sort((a, b) => a.machineNo - b.machineNo);
      machines.forEach((m) => {
        const occupiedBy = (byNo.get(m.machineNo) || []).find(
          (other) => other.machineType !== machineType && (other.outPortCount > 0 || other.extPortCount > 0)
        );
        const locked = !!occupiedBy;
        const lockHint = locked
          ? `<div class="hint" style="color:#c0392b;">已被${MACHINE_TYPE_LABEL[occupiedBy.machineType]}類型使用，請先將該類型數量歸零</div>`
          : '';

        const tr = document.createElement('tr');
        tr.innerHTML = `
          <td>${m.machineNo}</td>
          <td><input type="text" value="${m.alias ?? ''}" data-field="alias" /></td>
          <td>
            <input type="number" min="0" value="${m.outPortCount}" data-field="outPortCount" style="width:70px" ${locked ? 'disabled' : ''} />
            ${lockHint}
          </td>
          <td>${hasExtPorts ? `<input type="number" min="0" value="${m.extPortCount}" data-field="extPortCount" style="width:70px" ${locked ? 'disabled' : ''} />` : '—'}</td>
          <td><span class="badge ${m.isConnected ? 'on' : 'off'}">${m.isConnected ? '已連線' : '未連線'}</span></td>
        `;
        tr.querySelectorAll('input').forEach((input) => {
          // 存下目前這個值，PATCH 被後端拒絕時（例如互斥規則）要把畫面
          // 復原成這個值，而不是留著使用者剛剛打的、其實沒有生效的數字。
          input.dataset.original = input.value;
        });
        tr.querySelectorAll('input:not([disabled])').forEach((input) => {
          input.addEventListener('change', () => saveMachineField(m.machineNo, input.dataset.field, input));
        });
        tbody.appendChild(tr);
      });
    } catch (err) {
      Ui.handleError(err);
    }
  }

  function flashRejected(input) {
    input.classList.remove('field-rejected');
    // 強制重排一次讓動畫可以重新觸發（同一個元素連續兩次失敗也要有效果）
    void input.offsetWidth;
    input.classList.add('field-rejected');
    input.addEventListener('animationend', () => input.classList.remove('field-rejected'), { once: true });
  }

  async function saveMachineField(machineNo, field, input) {
    const value = field === 'alias' ? input.value : Number(input.value);
    const previousValue = input.dataset.original ?? '';
    try {
      await Api.patch(`/machines/${machineType}/${machineNo}`, { [field]: value });
      Ui.toast('已更新');
      input.dataset.original = input.value;
      if (field === 'outPortCount' || field === 'extPortCount') {
        loadPortList();
        // 數量變動可能讓別的類型被鎖住/解鎖，重新整理讓鎖定提示跟資料庫同步。
        loadMachineTable();
      }
    } catch (err) {
      Ui.handleError(err);
      // 後端拒絕了這次修改（例如合併機碼互斥規則）：把輸入框復原成修改前
      // 的值並閃一下紅色，讓使用者清楚知道剛剛的輸入沒有生效——不然只看
      // 右下角一閃即逝的 toast，很容易誤以為數字已經改成功了。
      input.value = previousValue;
      flashRejected(input);
      if (field === 'outPortCount' || field === 'extPortCount') {
        // 等紅色閃爍動畫播完再整個重畫表格，不然畫面會被重畫打斷、看不到效果。
        setTimeout(() => loadMachineTable(), REJECT_FLASH_MS);
      }
    }
  }

  async function loadPortList() {
    // PBX 同時有外線跟內線兩種實體埠，兩個表格都要顯示；
    // 來電盒/語音卡只有外線，只顯示 outGrid 那一個。
    const outContainer = document.getElementById(elIds.outGrid);
    outContainer.innerHTML = '<span class="hint">載入中...</span>';
    try {
      const outPorts = await Api.get('/outline-ports', { machineType });
      renderOutPortTable(outContainer, outPorts);
    } catch (err) {
      Ui.handleError(err);
    }

    if (!hasExtPorts) {
      return;
    }
    const extContainer = document.getElementById(elIds.extGrid);
    extContainer.innerHTML = '<span class="hint">載入中...</span>';
    try {
      const extPorts = await Api.get('/extline-ports', { machineType });
      renderExtPortTable(extContainer, extPorts);
    } catch (err) {
      Ui.handleError(err);
    }
  }

  function portTableShell() {
    const table = document.createElement('table');
    table.className = 'data-table';
    return table;
  }

  function renderOutPortTable(container, ports) {
    container.innerHTML = '';
    if (ports.length === 0) {
      container.innerHTML = '<span class="hint">這個類型目前沒有設定任何實體外線（先在上方表格設定外線數量）</span>';
      return;
    }
    ports.sort((a, b) => a.machineNo - b.machineNo || a.phyPort - b.phyPort);
    const table = portTableShell();
    table.innerHTML = '<thead><tr><th>機碼</th><th>實體外線埠</th><th>虛擬外線編號</th><th>狀態</th></tr></thead><tbody></tbody>';
    const tbody = table.querySelector('tbody');
    ports.forEach((p) => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td>${p.machineNo}</td>
        <td>${p.phyPort}</td>
        <td><input type="number" min="1" value="${p.vport ?? ''}" style="width:90px" placeholder="未指派" /></td>
        <td>${p.inUse ? '<span class="badge on">通話中</span>' : '<span class="badge off">閒置</span>'}</td>
      `;
      const vportInput = tr.querySelector('input');
      vportInput.addEventListener('change', async () => {
        const value = vportInput.value.trim();
        if (value === '') return;
        try {
          await Api.patch(`/outline-ports/${p.id}`, { vport: Number(value) });
          Ui.toast(`已將 ${p.machineNo} 號機第 ${p.phyPort} 實體外線指派為虛擬外線 ${value}`);
        } catch (err) {
          Ui.handleError(err);
        }
      });
      tbody.appendChild(tr);
    });
    container.appendChild(table);
  }

  function renderExtPortTable(container, ports) {
    container.innerHTML = '';
    if (ports.length === 0) {
      container.innerHTML = '<span class="hint">這個類型目前沒有設定任何實體內線（先在上方表格設定內線數量）</span>';
      return;
    }
    ports.sort((a, b) => a.machineNo - b.machineNo || a.phyPort - b.phyPort);
    const table = portTableShell();
    table.innerHTML = '<thead><tr><th>機碼</th><th>實體內線埠</th><th>虛擬內線編號</th><th>分機號碼</th></tr></thead><tbody></tbody>';
    const tbody = table.querySelector('tbody');
    ports.forEach((p) => {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td>${p.machineNo}</td>
        <td>${p.phyPort}</td>
        <td><input type="number" min="1" value="${p.vport ?? ''}" data-field="vport" style="width:90px" placeholder="未指派" /></td>
        <td><input type="text" value="${p.extNum ?? ''}" data-field="extNum" style="width:100px" placeholder="分機號碼" /></td>
      `;
      tr.querySelectorAll('input').forEach((input) => {
        input.addEventListener('change', async () => {
          const value = input.value.trim();
          if (value === '') return;
          const field = input.dataset.field;
          try {
            await Api.patch(`/extline-ports/${p.id}`, { [field]: Number(value) });
            Ui.toast(`已更新 ${p.machineNo} 號機第 ${p.phyPort} 實體內線`);
          } catch (err) {
            Ui.handleError(err);
          }
        });
      });
      tbody.appendChild(tr);
    });
    container.appendChild(table);
  }

  function refresh() {
    loadMachineTable();
    loadPortList();
  }

  function init() {
    refresh();
    document.getElementById(elIds.refreshBtn).addEventListener('click', refresh);
  }

  return { init, refresh };
}
