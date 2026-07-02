/** 共用的小工具：Toast 訊息、格狀圖產生器。 */
const Ui = (() => {
  let toastTimer = null;

  function toast(message, isError = false) {
    let el = document.getElementById('__toast');
    if (!el) {
      el = document.createElement('div');
      el.id = '__toast';
      el.className = 'toast';
      document.body.appendChild(el);
    }
    el.textContent = message;
    el.classList.toggle('error', isError);
    el.classList.add('show');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => el.classList.remove('show'), 2500);
  }

  function handleError(err) {
    console.error(err);
    toast(err.message || '發生未知錯誤', true);
  }

  return { toast, handleError };
})();
