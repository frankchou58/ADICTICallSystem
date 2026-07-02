/** 「設定」分頁：API 連線位址。 */
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

  function init() {
    initApiBaseUrl();
  }

  return { init };
})();
