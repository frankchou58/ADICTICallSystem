/** 員工管理（僅 admin 看得到這個分頁）：列表、新增、編輯、座位機碼指派（machineMask，控制外線看板可見範圍）、虛擬內線指派。 */
const EmployeesTab = (() => {
  async function loadTable() {
    const tbody = document.querySelector('#empTable tbody');
    tbody.innerHTML = '<tr><td colspan="9">載入中...</td></tr>';
    try {
      const [employees, extPorts] = await Promise.all([
        Api.get('/employees'),
        Api.get('/extline-ports'),
      ]);
      const vports = Array.from(new Set(
        extPorts.map((p) => p.vport).filter((v) => v !== null && v !== undefined)
      )).sort((a, b) => a - b);

      tbody.innerHTML = '';
      employees.forEach((emp) => {
        const tr = document.createElement('tr');
        const machineMask = emp.machineMask || 0;
        const machineCheckboxes = Array.from({ length: 10 }, (_, i) => i + 1).map((machineNo) => {
          const checked = (machineMask & (1 << (machineNo - 1))) !== 0;
          return `<label style="margin-right:6px;"><input type="checkbox" data-machine-no="${machineNo}" ${checked ? 'checked' : ''} />${machineNo}</label>`;
        }).join('');
        const extLineCheckboxes = vports.length
          ? vports.map((vport) => {
              const checked = (emp.extVports || []).includes(vport);
              return `<label style="margin-right:6px;"><input type="checkbox" data-ext-vport="${vport}" ${checked ? 'checked' : ''} />${vport}</label>`;
            }).join('')
          : '(尚無虛擬內線資料)';
        tr.innerHTML = `
          <td>${emp.id}</td>
          <td>${emp.employeeNo}</td>
          <td><input type="text" value="${emp.name ?? ''}" data-field="name" /></td>
          <td>
            <select data-field="role">
              <option value="operator" ${emp.role === 'operator' ? 'selected' : ''}>operator</option>
              <option value="supervisor" ${emp.role === 'supervisor' ? 'selected' : ''}>supervisor</option>
              <option value="admin" ${emp.role === 'admin' ? 'selected' : ''}>admin</option>
            </select>
          </td>
          <td><input type="text" value="${emp.extNum ?? ''}" data-field="extNum" style="width:70px" /></td>
          <td class="machine-mask">${machineCheckboxes}</td>
          <td class="ext-line-mask">${extLineCheckboxes}</td>
          <td><input type="checkbox" data-field="isDisabled" ${emp.isDisabled ? 'checked' : ''} /></td>
          <td><button class="secondary" data-action="password">改密碼</button></td>
        `;
        tr.querySelectorAll('input[data-field], select[data-field]').forEach((input) => {
          const evt = input.type === 'checkbox' ? 'change' : 'change';
          input.addEventListener(evt, () => saveField(emp.id, input));
        });
        tr.querySelectorAll('input[data-machine-no]').forEach((cb) => {
          cb.addEventListener('change', () => toggleMachine(emp.id, cb));
        });
        tr.querySelectorAll('input[data-ext-vport]').forEach((cb) => {
          cb.addEventListener('change', () => toggleExtLine(emp.id, cb));
        });
        tr.querySelector('button[data-action="password"]').addEventListener('click', () => openPasswordModal(emp));
        tbody.appendChild(tr);
      });
    } catch (err) {
      Ui.handleError(err);
      tbody.innerHTML = '<tr><td colspan="9">載入失敗</td></tr>';
    }
  }

  async function saveField(id, input) {
    const field = input.dataset.field;
    let value = input.type === 'checkbox' ? input.checked : input.value;
    if (field === 'extNum') value = value === '' ? null : Number(value);
    try {
      await Api.patch(`/employees/${id}`, { [field]: value });
      Ui.toast('已更新');
    } catch (err) {
      Ui.handleError(err);
    }
  }

  async function toggleMachine(id, checkbox) {
    const machineNo = Number(checkbox.dataset.machineNo);
    try {
      if (checkbox.checked) {
        await Api.post(`/employees/${id}/machines/${machineNo}`);
      } else {
        await Api.del(`/employees/${id}/machines/${machineNo}`);
      }
      Ui.toast('已更新座位機碼指派');
    } catch (err) {
      Ui.handleError(err);
      checkbox.checked = !checkbox.checked;
    }
  }

  async function toggleExtLine(id, checkbox) {
    const vport = Number(checkbox.dataset.extVport);
    try {
      if (checkbox.checked) {
        await Api.post(`/employees/${id}/ext-lines/${vport}`);
      } else {
        await Api.del(`/employees/${id}/ext-lines/${vport}`);
      }
      Ui.toast('已更新虛擬內線指派');
    } catch (err) {
      Ui.handleError(err);
      checkbox.checked = !checkbox.checked;
    }
  }

  function openAddModal() {
    document.getElementById('empAddNo').value = '';
    document.getElementById('empAddPassword').value = '';
    document.getElementById('empAddName').value = '';
    document.getElementById('empAddRole').value = 'operator';
    document.getElementById('empAddMsg').textContent = '';
    document.getElementById('empAddBackdrop').classList.add('show');
  }

  function closeAddModal() {
    document.getElementById('empAddBackdrop').classList.remove('show');
  }

  async function saveNewEmployee() {
    const msgEl = document.getElementById('empAddMsg');
    msgEl.textContent = '';
    const employeeNo = document.getElementById('empAddNo').value.trim();
    const password = document.getElementById('empAddPassword').value;
    const name = document.getElementById('empAddName').value.trim();
    const role = document.getElementById('empAddRole').value;
    if (!employeeNo || !password) {
      msgEl.textContent = '請輸入員工編號與密碼';
      return;
    }
    try {
      await Api.post('/employees', { employeeNo, password, name, role });
      Ui.toast('已新增員工');
      closeAddModal();
      loadTable();
    } catch (err) {
      msgEl.textContent = err.message || '新增失敗';
    }
  }

  let passwordTargetId = null;

  function openPasswordModal(emp) {
    passwordTargetId = emp.id;
    const stored = JSON.parse(sessionStorage.getItem('adicti.operator.employee') || 'null');
    const isSelf = stored && stored.id === emp.id;
    document.getElementById('empPasswordTarget').textContent = `員工：${emp.name || emp.employeeNo}（${emp.employeeNo}）`;
    document.getElementById('empCurrentPasswordField').style.display = isSelf ? '' : 'none';
    document.getElementById('empCurrentPassword').value = '';
    document.getElementById('empNewPassword').value = '';
    document.getElementById('empNewPasswordConfirm').value = '';
    document.getElementById('empPasswordMsg').textContent = '';
    document.getElementById('empPasswordBackdrop').classList.add('show');
  }

  function closePasswordModal() {
    document.getElementById('empPasswordBackdrop').classList.remove('show');
    passwordTargetId = null;
  }

  async function savePassword() {
    const msgEl = document.getElementById('empPasswordMsg');
    msgEl.textContent = '';
    const isSelfField = document.getElementById('empCurrentPasswordField').style.display !== 'none';
    const currentPassword = document.getElementById('empCurrentPassword').value;
    const newPassword = document.getElementById('empNewPassword').value;
    const confirmPassword = document.getElementById('empNewPasswordConfirm').value;

    if (isSelfField && !currentPassword) {
      msgEl.textContent = '請輸入目前密碼';
      return;
    }
    if (!newPassword) {
      msgEl.textContent = '請輸入新密碼';
      return;
    }
    if (newPassword !== confirmPassword) {
      msgEl.textContent = '兩次輸入的新密碼不一致';
      return;
    }

    try {
      const body = isSelfField ? { currentPassword, newPassword } : { newPassword };
      await Api.patch(`/employees/${passwordTargetId}/password`, body);
      Ui.toast('密碼已變更');
      closePasswordModal();
    } catch (err) {
      msgEl.textContent = err.message || '變更失敗';
    }
  }

  function init() {
    document.getElementById('empAddBtn').addEventListener('click', openAddModal);
    document.getElementById('empAddCancelBtn').addEventListener('click', closeAddModal);
    document.getElementById('empAddSaveBtn').addEventListener('click', saveNewEmployee);
    document.getElementById('empRefreshBtn').addEventListener('click', loadTable);
    document.getElementById('empPasswordCancelBtn').addEventListener('click', closePasswordModal);
    document.getElementById('empPasswordSaveBtn').addEventListener('click', savePassword);
    loadTable();
  }

  return { init };
})();
