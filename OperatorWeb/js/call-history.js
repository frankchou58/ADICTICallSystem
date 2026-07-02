/** 通話紀錄查詢：日期區間 + 電話號碼篩選，呼叫 GET /call-records。 */
const CallHistoryTab = (() => {
  const CALL_TYPE_LABEL = { 0: '外線撥入', 1: '外線撥出', 2: '內線互撥' };

  function pad(n) { return String(n).padStart(2, '0'); }
  function toDateStr(d) { return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())}`; }

  function computeRange(preset) {
    const now = new Date();
    let start = new Date(now);
    if (preset === 'today') {
      // start = 今天 00:00
    } else if (preset === 'week') {
      start.setDate(start.getDate() - 6);
    } else if (preset === 'thisWeek') {
      const day = (start.getDay() + 6) % 7; // 週一為 0
      start.setDate(start.getDate() - day);
    } else if (preset === 'month') {
      start = new Date(now.getFullYear(), now.getMonth(), 1);
    } else if (preset === 'year') {
      start = new Date(now.getFullYear(), 0, 1);
    }
    return { dateFrom: toDateStr(start), dateTo: toDateStr(now) };
  }

  function formatTime(iso) {
    if (!iso) return '';
    return String(iso).replace('T', ' ').substring(0, 19);
  }

  async function search() {
    const preset = document.getElementById('chRangePreset').value;
    let dateFrom, dateTo;
    if (preset === 'custom') {
      dateFrom = document.getElementById('chStartDate').value;
      dateTo = document.getElementById('chEndDate').value;
    } else {
      const range = computeRange(preset);
      dateFrom = range.dateFrom;
      dateTo = range.dateTo;
    }
    const telNo = document.getElementById('chTelFilter').value.trim();

    const tbody = document.querySelector('#chTable tbody');
    tbody.innerHTML = '<tr><td colspan="12">查詢中...</td></tr>';
    try {
      const records = await Api.get('/call-records', {
        dateFrom: dateFrom ? `${dateFrom}T00:00:00` : undefined,
        dateTo: dateTo ? `${dateTo}T23:59:59` : undefined,
        telNo: telNo || undefined,
        limit: 200,
      });
      tbody.innerHTML = '';
      if (records.length === 0) {
        tbody.innerHTML = '<tr><td colspan="12">查無資料</td></tr>';
        return;
      }
      records.forEach((r) => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
          <td>${r.id}</td>
          <td>${CALL_TYPE_LABEL[r.callType] || r.callType}</td>
          <td>${r.machineNo}</td>
          <td>${r.outVport ?? ''}${r.outPhyPort !== null ? '/' + r.outPhyPort : ''}</td>
          <td>${r.telNo ?? ''}</td>
          <td>${r.extNum ?? r.fromExtNum ?? ''}${r.toExtNum ? ' → ' + r.toExtNum : ''}</td>
          <td>${formatTime(r.startTime)}</td>
          <td>${formatTime(r.connectTime)}</td>
          <td>${formatTime(r.endTime)}</td>
          <td>${r.durationSec ?? ''}</td>
          <td>${r.ringTimes ?? ''}</td>
          <td>${r.voiceRecordUuid ?? ''}</td>
        `;
        tbody.appendChild(tr);
      });
    } catch (err) {
      Ui.handleError(err);
      tbody.innerHTML = '<tr><td colspan="12">查詢失敗</td></tr>';
    }
  }

  function onPresetChange() {
    const isCustom = document.getElementById('chRangePreset').value === 'custom';
    document.getElementById('chStartDate').style.display = isCustom ? '' : 'none';
    document.getElementById('chEndDate').style.display = isCustom ? '' : 'none';
  }

  function init() {
    document.getElementById('chRangePreset').addEventListener('change', onPresetChange);
    document.getElementById('chSearchBtn').addEventListener('click', search);
    search();
  }

  return { init };
})();
