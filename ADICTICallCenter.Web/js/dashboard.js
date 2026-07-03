/**
 * 「虛擬內外線狀態」分頁：新版資料模型下，虛擬編號(vport)是每個實體埠自己
 * 的一個欄位，不是固定 1~240 的池子，一個 vport 可能同時對應到 1~3 種
 * 子機類型的實體埠（代表這幾種設備正在看同一條實體線路）。所以畫面改成
 * 「依 vport 分組」呈現：每個方塊代表一個虛擬編號，裡面列出目前有哪些
 * 子機類型/機碼在共用這個編號。
 */
const DashboardTab = (() => {
  const TYPE_LABEL = { 1: 'PBX', 2: '來電盒', 3: '語音卡' };

  function statusClass(status) {
    switch (status) {
      case 1: return 'status-line-in';
      case 2: return 'status-line-out';
      case 3: return 'status-line-talking';
      default: return 'status-idle';
    }
  }

  function groupByVport(rows) {
    const groups = new Map();
    rows.forEach((row) => {
      if (row.vport === null) return; // 尚未指派虛擬編號的實體埠先不顯示
      if (!groups.has(row.vport)) groups.set(row.vport, []);
      groups.get(row.vport).push(row);
    });
    return [...groups.entries()].sort((a, b) => a[0] - b[0]);
  }

  function renderOutlineGrid(container, rows) {
    container.innerHTML = '';
    const groups = groupByVport(rows);
    if (groups.length === 0) {
      container.innerHTML = '<span class="hint">目前沒有已指派虛擬編號的外線</span>';
      return;
    }
    groups.forEach(([vport, members]) => {
      const worstStatus = Math.max(...members.map((m) => m.callStatus));
      const cell = document.createElement('div');
      cell.className = `port-cell ${statusClass(worstStatus)}`;
      const sub = members.map((m) => `${TYPE_LABEL[m.machineType]}${m.machineNo}`).join('/');
      cell.innerHTML = `<div class="no">${String(vport).padStart(3, '0')}</div><div class="sub">${sub}</div>`;
      container.appendChild(cell);
    });
  }

  function renderExtlineGrid(container, rows) {
    container.innerHTML = '';
    const groups = groupByVport(rows);
    if (groups.length === 0) {
      container.innerHTML = '<span class="hint">目前沒有已指派虛擬編號的內線</span>';
      return;
    }
    groups.forEach(([vport, members]) => {
      const cell = document.createElement('div');
      cell.className = 'port-cell status-idle';
      const extNums = members.map((m) => m.extNum ?? '-').join('/');
      cell.innerHTML = `<div class="no">${String(vport).padStart(3, '0')}</div><div class="sub">${extNums}</div>`;
      container.appendChild(cell);
    });
  }

  async function load() {
    const outlineGrid = document.getElementById('outlineGrid');
    const extlineGrid = document.getElementById('extlineGrid');
    try {
      const [outlinePorts, extlinePorts] = await Promise.all([
        Api.get('/outline-ports'),
        Api.get('/extline-ports'),
      ]);
      renderOutlineGrid(outlineGrid, outlinePorts);
      renderExtlineGrid(extlineGrid, extlinePorts);
    } catch (err) {
      Ui.handleError(err);
    }
  }

  function init() {
    load();
    document.getElementById('dashboardRefreshBtn').addEventListener('click', load);
  }

  return { init, load, refresh: load };
})();
