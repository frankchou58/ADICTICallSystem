/**
 * 來電客戶資料：畫面彈出視窗（screen-pop）+ 一般客戶查詢共用同一套邏輯。
 * 找不到客戶時顯示「新客戶」，存檔會改成新增（POST）而不是更新（PATCH）。
 */
const CustomerPanel = (() => {
  let currentCustomer = null; // 目前彈出視窗對應的客戶物件；null 表示新客戶
  let currentTelNo = null;

  async function lookupByTel(telNo) {
    if (!telNo) return null;
    try {
      return await Api.get(`/customers/by-tel/${encodeURIComponent(telNo)}`);
    } catch (err) {
      if (err.status === 404) return null;
      throw err;
    }
  }

  function open(telNo, customer) {
    currentTelNo = telNo;
    currentCustomer = customer;
    document.getElementById('popTelNo').textContent = telNo || '(未知)';
    document.getElementById('popName').value = customer ? (customer.name || '') : '';
    document.getElementById('popCounty').value = customer ? (customer.county || '') : '';
    document.getElementById('popTownship').value = customer ? (customer.township || '') : '';
    document.getElementById('popAddress').value = customer ? (customer.address || '') : '';
    document.getElementById('popNewFlag').style.display = customer ? 'none' : 'inline-block';
    document.getElementById('popBlacklistFlag').style.display = customer && customer.isBlacklisted ? 'inline-block' : 'none';
    document.getElementById('popMsg').textContent = '';
    document.getElementById('popBackdrop').classList.add('show');
  }

  function close() {
    document.getElementById('popBackdrop').classList.remove('show');
  }

  async function showForCall(telNo) {
    try {
      const customer = await lookupByTel(telNo);
      open(telNo, customer);
    } catch (err) {
      Ui.handleError(err);
    }
  }

  async function save() {
    const msgEl = document.getElementById('popMsg');
    msgEl.textContent = '';
    const body = {
      telNo: currentTelNo,
      name: document.getElementById('popName').value.trim(),
      county: document.getElementById('popCounty').value.trim(),
      township: document.getElementById('popTownship').value.trim(),
      address: document.getElementById('popAddress').value.trim(),
    };
    try {
      if (currentCustomer) {
        await Api.patch(`/customers/${currentCustomer.id}`, body);
      } else {
        await Api.post('/customers', body);
      }
      Ui.toast('已儲存客戶資料');
      close();
    } catch (err) {
      msgEl.textContent = err.message || '儲存失敗';
    }
  }

  document.getElementById('popCloseBtn').addEventListener('click', close);
  document.getElementById('popSaveBtn').addEventListener('click', save);

  return { lookupByTel, showForCall, open, close };
})();
