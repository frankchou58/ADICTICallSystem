/**
 * 分機自助設定：舊版是靠掃座位上的 QR Code、輸入工號來綁定分機（因為那個
 * 頁面本身不需要登入）。新版每個座位都已經是登入狀態，直接用自己的
 * employee id 呼叫 PATCH /employees/{id} 更新 ext_num 即可，不用另外設計
 * 一套「輸入工號認領分機」的流程。
 */
const ExtensionTab = (() => {
  async function load() {
    const stored = JSON.parse(sessionStorage.getItem('adicti.operator.employee') || 'null');
    if (!stored) return;
    try {
      const fresh = await Api.get(`/employees/${stored.id}`);
      document.getElementById('extCurrentNum').textContent = fresh.extNum || '(尚未設定)';
      document.getElementById('extNewNum').value = fresh.extNum || '';
    } catch (err) {
      Ui.handleError(err);
    }
  }

  async function save() {
    const msgEl = document.getElementById('extMsg');
    msgEl.textContent = '';
    const stored = JSON.parse(sessionStorage.getItem('adicti.operator.employee') || 'null');
    const newNum = document.getElementById('extNewNum').value.trim();
    if (!newNum || !/^\d+$/.test(newNum)) {
      msgEl.textContent = '請輸入數字分機號碼';
      return;
    }
    try {
      await Api.patch(`/employees/${stored.id}`, { extNum: Number(newNum) });
      Ui.toast('分機號碼已更新');
      load();
    } catch (err) {
      msgEl.textContent = err.message || '更新失敗';
    }
  }

  async function changePassword() {
    const msgEl = document.getElementById('pwMsg');
    msgEl.textContent = '';
    const stored = JSON.parse(sessionStorage.getItem('adicti.operator.employee') || 'null');
    if (!stored) {
      msgEl.textContent = '找不到目前登入者資訊，請重新登入';
      return;
    }

    const currentInput = document.getElementById('pwCurrentPassword');
    const newInput = document.getElementById('pwNewPassword');
    const confirmInput = document.getElementById('pwConfirmPassword');
    const currentPassword = currentInput.value;
    const newPassword = newInput.value;
    const confirmPassword = confirmInput.value;

    if (!currentPassword || !newPassword || !confirmPassword) {
      msgEl.textContent = '請完整填寫三個欄位';
      return;
    }
    if (newPassword !== confirmPassword) {
      msgEl.textContent = '兩次輸入的新密碼不一致';
      return;
    }

    try {
      await Api.patch(`/employees/${stored.id}/password`, { currentPassword, newPassword });
      Ui.toast('密碼已變更');
      currentInput.value = '';
      newInput.value = '';
      confirmInput.value = '';
    } catch (err) {
      msgEl.textContent = err.message || '變更密碼失敗';
    }
  }

  function init() {
    document.getElementById('extSaveBtn').addEventListener('click', save);
    document.getElementById('pwChangeBtn').addEventListener('click', changePassword);
    load();
  }

  return { init };
})();
