/**
 * 來電報表：舊版 InBoundList.html 對應的功能，當年打的是硬編 IP、
 * 表格渲染還被拿掉（半成品）。新版直接用 GET /call-records 的
 * custName/custAddr/status 篩選（後端有 LEFT JOIN customers）。
 */
const InboundReportTab = (() => {
  const STATUS_LABEL = { answered: '來電接聽', missed: '未接', outbound: '撥出' };

  function formatTime(iso) {
    if (!iso) return '';
    return String(iso).replace('T', ' ').substring(0, 19);
  }

  function rowStatusLabel(r) {
    if (r.callType === 1) return STATUS_LABEL.outbound;
    if (r.callType === 0) return r.connectTime ? STATUS_LABEL.answered : STATUS_LABEL.missed;
    return '內線';
  }

  async function search() {
    const dateFrom = document.getElementById('ibStartDate').value;
    const dateTo = document.getElementById('ibEndDate').value;
    const tel = document.getElementById('ibTel').value.trim();
    const name = document.getElementById('ibName').value.trim();
    const addr = document.getElementById('ibAddr').value.trim();
    const status = document.getElementById('ibStatus').value;

    const tbody = document.querySelector('#ibTable tbody');
    tbody.innerHTML = '<tr><td colspan="8">查詢中...</td></tr>';
    try {
      const records = await Api.get('/call-records', {
        dateFrom: dateFrom ? `${dateFrom}T00:00:00` : undefined,
        dateTo: dateTo ? `${dateTo}T23:59:59` : undefined,
        telNo: tel || undefined,
        custName: name || undefined,
        custAddr: addr || undefined,
        status: status || undefined,
        limit: 200,
      });
      tbody.innerHTML = '';
      if (records.length === 0) {
        tbody.innerHTML = '<tr><td colspan="8">查無資料</td></tr>';
        return;
      }
      records.forEach((r) => {
        const tr = document.createElement('tr');
        const addrText = [r.customerCounty, r.customerTownship, r.customerAddress].filter(Boolean).join('');
        tr.innerHTML = `
          <td>${formatTime(r.startTime)}</td>
          <td>${r.telNo ?? ''}</td>
          <td>${r.customerName ?? ''}</td>
          <td>${addrText}</td>
          <td>${rowStatusLabel(r)}</td>
          <td>${r.extNum ?? ''}</td>
          <td>${r.durationSec ?? ''}</td>
          <td>${r.voiceRecordUuid ?? ''}</td>
        `;
        tbody.appendChild(tr);
      });
    } catch (err) {
      Ui.handleError(err);
      tbody.innerHTML = '<tr><td colspan="8">查詢失敗</td></tr>';
    }
  }

  function init() {
    document.getElementById('ibSearchBtn').addEventListener('click', search);
    // 預設帶今天日期，跟舊版一樣一進來就有資料可看。
    const today = new Date().toISOString().substring(0, 10);
    document.getElementById('ibStartDate').value = today;
    document.getElementById('ibEndDate').value = today;
    search();
  }

  return { init };
})();
