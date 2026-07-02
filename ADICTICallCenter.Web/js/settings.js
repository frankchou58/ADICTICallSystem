/** 「設定」分頁：API 連線位址、變更目前登入者的密碼。 */
const SettingsTab = (() => {
  function initApiBaseUrl() {
    const input = document.getElementById('apiBaseUrlInput');
    input.value = Api.getBaseUrl();

    document.getElementById('saveSettingsBtn').addEventListener('click', () => {
      const url = input.value.trim();
      if (!url) {
        Ui.toast('請輸入 API 位址', true);
        return;
      }
      Api.setBaseUrl(url);
      Ui.toast('設定已儲存，重新整理頁面後生效');
    });
  }

  function initChangePassword() {
    const currentInput = document.getElementById('currentPasswordInput');
    const newInput = document.getElementById('newPasswordInput');
    const confirmInput = document.getElementById('confirmPasswordInput');
    const msgEl = document.getElementById('changePasswordMsg');

    document.getElementById('changePasswordBtn').addEventListener('click', async () => {
      msgEl.textContent = '';

      const employee = JSON.parse(sessionStorage.getItem('adicti.employee') || 'null');
      if (!employee) {
        msgEl.textContent = '找不到目前登入者資訊，請重新登入';
        return;
      }

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
        await Api.patch(`/employees/${employee.id}/password`, { currentPassword, newPassword });
        Ui.toast('密碼已變更');
        currentInput.value = '';
        newInput.value = '';
        confirmInput.value = '';
      } catch (err) {
        msgEl.textContent = err.message || '變更密碼失敗';
      }
    });
  }

  function init() {
    initApiBaseUrl();
    initChangePassword();
  }

  return { init };
})();
