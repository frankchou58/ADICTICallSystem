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
function createMachinesTab(machineType, elIds) {
  const hasExtPorts = machineType === 1;

  async function loadMachineTable() {
    const tbody = document.querySelector(`#${elIds.table} tbody`);
    tbody.innerHTML = '';
    try {
      const machines = await Api.get('/machines', { machineType });
      machines.sort((a, b) => a.machineNo - b.machineNo);
      machines.forEach((m) => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
          <td>${m.machineNo}</td>
          <td><input type="text" value="${m.alias ?? ''}" data-field="alias" /></td>
          <td><input type="number" min="0" value="${m.outPortCount}" data-field="outPortCount" style="width:70px" /></td>
          <td>${hasExtPorts ? `<input type="number" min="0" value="${m.extPortCount}" data-field="extPortCount" style="width:70px" />` : '—'}</td>
          <td><span class="badge ${m.isConnected ? 'on' : 'off'}">${m.isConnected ? '已連線' : '未連線'}</span></td>
        `;
        tr.querySelectorAll('input').forEach((input) => {
          input.addEventListener('change', () => saveMachineField(m.machineNo, input.dataset.field, input));
        });
        tbody.appendChild(tr);
      });
    } catch (err) {
      Ui.handleError(err);
    }
  }

  async function saveMachineField(machineNo, field, input) {
    const value = field === 'alias' ? input.value : Number(input.value);
    try {
      await Api.patch(`/machines/${machineType}/${machineNo}`, { [field]: value });
      Ui.toast('已更新');
      if (field === 'outPortCount' || field === 'extPortCount') {
        loadPortList();
      }
    } catch (err) {
      Ui.handleError(err);
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

  function init() {
    loadMachineTable();
    loadPortList();
    document.getElementById(elIds.refreshBtn).addEventListener('click', () => {
      loadMachineTable();
      loadPortList();
    });
  }

  return { init };
}
