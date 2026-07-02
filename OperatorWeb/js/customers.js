/** 客戶管理（僅 admin 看得到這個分頁）：列表、搜尋、新增、編輯。 */
const CustomersTab = (() => {
  async function loadTable(telNo) {
    const tbody = document.querySelector('#custTable tbody');
    tbody.innerHTML = '<tr><td colspan="7">載入中...</td></tr>';
    try {
      const customers = await Api.get('/customers', telNo ? { telNo } : {});
      tbody.innerHTML = '';
      if (customers.length === 0) {
        tbody.innerHTML = '<tr><td colspan="7">查無資料</td></tr>';
        return;
      }
      customers.forEach((c) => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
          <td>${c.id}</td>
          <td><input type="text" value="${c.name ?? ''}" data-field="name" /></td>
          <td><input type="text" value="${c.telNo ?? ''}" data-field="telNo" /></td>
          <td><input type="text" value="${c.county ?? ''}" data-field="county" style="width:70px" /></td>
          <td><input type="text" value="${c.township ?? ''}" data-field="township" style="width:70px" /></td>
          <td><input type="text" value="${c.address ?? ''}" data-field="address" /></td>
          <td><input type="checkbox" data-field="isBlacklisted" ${c.isBlacklisted ? 'checked' : ''} /></td>
        `;
        tr.querySelectorAll('input[data-field]').forEach((input) => {
          input.addEventListener('change', () => saveField(c.id, input));
        });
        tbody.appendChild(tr);
      });
    } catch (err) {
      Ui.handleError(err);
      tbody.innerHTML = '<tr><td colspan="7">載入失敗</td></tr>';
    }
  }

  async function saveField(id, input) {
    const field = input.dataset.field;
    const value = input.type === 'checkbox' ? input.checked : input.value;
    try {
      await Api.patch(`/customers/${id}`, { [field]: value });
      Ui.toast('已更新');
    } catch (err) {
      Ui.handleError(err);
    }
  }

  function openAddModal() {
    document.getElementById('custAddTel').value = '';
    document.getElementById('custAddName').value = '';
    document.getElementById('custAddCounty').value = '';
    document.getElementById('custAddTownship').value = '';
    document.getElementById('custAddAddress').value = '';
    document.getElementById('custAddMsg').textContent = '';
    document.getElementById('custAddBackdrop').classList.add('show');
  }

  function closeAddModal() {
    document.getElementById('custAddBackdrop').classList.remove('show');
  }

  async function saveNewCustomer() {
    const msgEl = document.getElementById('custAddMsg');
    msgEl.textContent = '';
    const body = {
      telNo: document.getElementById('custAddTel').value.trim(),
      name: document.getElementById('custAddName').value.trim(),
      county: document.getElementById('custAddCounty').value.trim(),
      township: document.getElementById('custAddTownship').value.trim(),
      address: document.getElementById('custAddAddress').value.trim(),
    };
    try {
      await Api.post('/customers', body);
      Ui.toast('已新增客戶');
      closeAddModal();
      loadTable();
    } catch (err) {
      msgEl.textContent = err.message || '新增失敗';
    }
  }

  function init() {
    document.getElementById('custSearchBtn').addEventListener('click', () => {
      loadTable(document.getElementById('custSearchTel').value.trim());
    });
    document.getElementById('custAddBtn').addEventListener('click', openAddModal);
    document.getElementById('custAddCancelBtn').addEventListener('click', closeAddModal);
    document.getElementById('custAddSaveBtn').addEventListener('click', saveNewCustomer);
    document.getElementById('custRefreshBtn').addEventListener('click', () => loadTable());
    loadTable();
  }

  return { init };
})();
