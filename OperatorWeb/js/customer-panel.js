/**
 * 來電螢幕彈出視窗：只顯示來電號碼。
 *
 * 2026-07-03：這台部署沒有 customers 資料表（見 doc/ADICTICallSystem.API-說明書.md），
 * 原本這裡會查詢/顯示/編輯客戶姓名地址，現在拿掉，只剩單純的「有來電」通知。
 */
const CustomerPanel = (() => {
  function showForCall(telNo) {
    document.getElementById('popTelNo').textContent = telNo || '(未知)';
    document.getElementById('popBackdrop').classList.add('show');
  }

  function close() {
    document.getElementById('popBackdrop').classList.remove('show');
  }

  document.getElementById('popCloseBtn').addEventListener('click', close);

  return { showForCall, close };
})();
