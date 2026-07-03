<?php

namespace App\Controllers;

use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;
use PDO;

/**
 * 2026-07-03: rewritten against the customer's pre-existing ADICTICallCenter
 * database's dbo.record table. A single PATCH endpoint still replaces the
 * legacy design's eight separate UpdateCallRecordXxx.php single-field-update
 * endpoints - the Call State Engine can just PATCH whichever fields changed
 * as a call progresses through its state machine.
 *
 * Columns not carried over from this API's original design:
 *   - customerId / customer name+address search+join: dbo.customers doesn't
 *     exist in this database and can't be created (no new tables allowed),
 *     so customer lookup/matching for call records is dropped entirely.
 *     record.customer_uuid exists but is a CHAR(33) uuid the legacy app
 *     never actually wrote either - not usable as a foreign key to anything.
 *   - employeeId: record.operator_uuid is likewise a CHAR(33) uuid (not an
 *     int id compatible with dbo.operators.ID) that no legacy endpoint ever
 *     wrote - dropped rather than repurposed.
 * record.call_second_dtmf exists but per the legacy-schema research notes
 * (doc/ADICTICallSystem.API-說明書.md) no code ever read or wrote it either;
 * kept here as a pass-through free-text field since the column is real.
 */
class CallRecordController extends Controller
{
    private const UPDATABLE_COLUMNS = [
        'telNo' => 'tel_no',
        'extNum' => 'ext_num',
        'fromExtNum' => 'internal_call_from_ext_no',
        'toExtNum' => 'internal_call_to_ext_no',
        'ringTimes' => 'ring_times',
        'durationSec' => 'call_duration_time',
        'secondDtmf' => 'call_second_dtmf',
        'voiceRecordUuid' => 'voice_record',
        'callType' => 'call_type',
    ];

    /**
     * POST /call-records
     * { machineNo, callType, outVport?, outPhyPort?, telNo?, extNum?, fromExtNum?, toExtNum?,
     *   ringTimes?, startTime?, connectTime?, endTime?, durationSec?, secondDtmf?, voiceRecordUuid? }
     */
    public function create(Request $request): void
    {
        $machineNo = (int) $request->requireInput('machineNo');
        $callType = (int) $request->requireInput('callType');
        if (!in_array($callType, [0, 1, 2], true)) {
            throw new ValidationException('callType 必須是 0（撥入）、1（撥出）或 2（內線互撥）。');
        }

        $stmt = $this->db()->prepare(
            'INSERT INTO dbo.record
                (machine_id, call_type, out_vport_no, out_phyport_no, tel_no, ext_num,
                 internal_call_from_ext_no, internal_call_to_ext_no,
                 ring_times, call_start_time, call_connect_time, call_end_time,
                 call_duration_time, call_second_dtmf, voice_record)
             OUTPUT INSERTED.ID
             VALUES
                (:machine_id, :call_type, :out_vport_no, :out_phyport_no, :tel_no, :ext_num,
                 :from_ext_no, :to_ext_no,
                 :ring_times, :start_time, :connect_time, :end_time,
                 :duration_sec, :second_dtmf, :voice_record)'
        );

        // 不能直接 execute($array)：PDO_ODBC 對陣列裡的 PHP null 值型別推斷
        // 不可靠，常常會送成空字串而不是 SQL NULL，塞進 INT 欄位就會炸
        // "Invalid character value for cast specification"。用共用的
        // executeWithParams() 明確依型別綁定（null 一定綁 PARAM_NULL）。
        $this->executeWithParams($stmt, [
            'machine_id' => $machineNo,
            'call_type' => $callType,
            'out_vport_no' => $request->input('outVport'),
            'out_phyport_no' => $request->input('outPhyPort'),
            'tel_no' => $request->input('telNo'),
            'ext_num' => $request->input('extNum'),
            'from_ext_no' => $request->input('fromExtNum'),
            'to_ext_no' => $request->input('toExtNum'),
            'ring_times' => $request->input('ringTimes'),
            'start_time' => $this->normalizeTimestamp($request->input('startTime')),
            'connect_time' => $this->normalizeTimestamp($request->input('connectTime')),
            'end_time' => $this->normalizeTimestamp($request->input('endTime')),
            'duration_sec' => $request->input('durationSec'),
            'second_dtmf' => $request->input('secondDtmf'),
            'voice_record' => $request->input('voiceRecordUuid'),
        ], ['tel_no', 'second_dtmf', 'voice_record']);

        Response::created(['id' => (int) $stmt->fetchColumn()], '通話紀錄已建立。');
    }

    /** GET /call-records/{id} */
    public function show(Request $request): void
    {
        $row = $this->fetchById((int) $request->params['id']);
        if (!$row) {
            Response::notFound('找不到這筆通話紀錄。');
            return;
        }
        Response::ok($this->mapRow($row));
    }

    /**
     * PATCH /call-records/{id}
     * Accepts any subset of: telNo, extNum, fromExtNum, toExtNum, ringTimes, durationSec,
     * secondDtmf, voiceRecordUuid, callType, startTime, connectTime, endTime.
     */
    public function update(Request $request): void
    {
        $id = (int) $request->params['id'];

        $fields = [];
        $params = ['id' => $id];
        $textFields = [];

        foreach (self::UPDATABLE_COLUMNS as $inputKey => $column) {
            $value = $request->input($inputKey);
            if ($value !== null) {
                $fields[] = "$column = :$column";
                $params[$column] = $value;
                if (in_array($column, ['tel_no', 'call_second_dtmf', 'voice_record'], true)) {
                    $textFields[] = $column;
                }
            }
        }
        foreach (['startTime' => 'call_start_time', 'connectTime' => 'call_connect_time', 'endTime' => 'call_end_time'] as $inputKey => $column) {
            $value = $request->input($inputKey);
            if ($value !== null) {
                $fields[] = "$column = :$column";
                $params[$column] = $this->normalizeTimestamp($value);
            }
        }

        if (empty($fields)) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $sql = 'UPDATE dbo.record SET ' . implode(', ', $fields) . ' WHERE ID = :id';
        $stmt = $this->db()->prepare($sql);
        $this->executeWithParams($stmt, $params, $textFields);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這筆通話紀錄。');
            return;
        }

        Response::ok(null, '通話紀錄已更新。');
    }

    /**
     * GET /call-records?machineNo=&telNo=&callType=&dateFrom=&dateTo=&limit=&offset=
     * Consolidates the legacy QueryInBoundNumbers / QueryInboundDataByDate /
     * QueryInboundDataByTel / SearchInboundData endpoints into one filterable query.
     *
     * Dropped vs. this API's original design: custName/custAddr/status
     * filters and the customer-name/address columns, all of which relied on
     * a dbo.customers table that doesn't exist here and can't be created
     * (see class docblock) - OperatorWeb's 來電報表 falls back to showing
     * no customer name/address for this deployment.
     */
    public function index(Request $request): void
    {
        [$limit, $offset] = $this->paginationArgs($request);

        $where = [];
        $params = [];
        $textParams = [];

        if ($machineNo = $request->query['machineNo'] ?? null) {
            $where[] = 'machine_id = :machine_id';
            $params['machine_id'] = (int) $machineNo;
        }
        if ($telNo = $request->query['telNo'] ?? null) {
            $where[] = 'tel_no LIKE :tel_no';
            $params['tel_no'] = '%' . $telNo . '%';
            $textParams[] = 'tel_no';
        }
        if (isset($request->query['callType'])) {
            $where[] = 'call_type = :call_type';
            $params['call_type'] = (int) $request->query['callType'];
        }
        if ($dateFrom = $request->query['dateFrom'] ?? null) {
            $where[] = 'call_start_time >= :date_from';
            $params['date_from'] = $this->normalizeTimestamp($dateFrom);
        }
        if ($dateTo = $request->query['dateTo'] ?? null) {
            $where[] = 'call_start_time <= :date_to';
            $params['date_to'] = $this->normalizeTimestamp($dateTo);
        }
        $status = $request->query['status'] ?? null;
        if ($status === 'answered') {
            $where[] = 'call_type = 0 AND call_connect_time IS NOT NULL';
        } elseif ($status === 'missed') {
            $where[] = 'call_type = 0 AND call_connect_time IS NULL';
        } elseif ($status === 'outbound') {
            $where[] = 'call_type = 1';
        }

        $sql = 'SELECT ID AS id, machine_id, call_type, out_vport_no, out_phyport_no, tel_no, ext_num,
                       internal_call_from_ext_no, internal_call_to_ext_no, ring_times,
                       call_start_time, call_connect_time, call_end_time,
                       call_duration_time, call_second_dtmf, voice_record
                FROM dbo.record';
        if ($where) {
            $sql .= ' WHERE ' . implode(' AND ', $where);
        }
        $sql .= ' ORDER BY call_start_time DESC OFFSET :offset ROWS FETCH NEXT :limit ROWS ONLY';

        $stmt = $this->db()->prepare($sql);
        foreach ($params as $key => $value) {
            if (in_array($key, $textParams, true)) {
                \App\Core\Database::bindText($stmt, ":$key", $value);
            } else {
                $stmt->bindValue($key, $value, is_int($value) ? PDO::PARAM_INT : PDO::PARAM_STR);
            }
        }
        $stmt->bindValue('offset', $offset, PDO::PARAM_INT);
        $stmt->bindValue('limit', $limit, PDO::PARAM_INT);
        $stmt->execute();

        Response::ok(array_map([$this, 'mapRow'], $stmt->fetchAll()));
    }

    /** @return array|false */
    private function fetchById(int $id)
    {
        $stmt = $this->db()->prepare(
            'SELECT ID AS id, machine_id, call_type, out_vport_no, out_phyport_no, tel_no, ext_num,
                    internal_call_from_ext_no, internal_call_to_ext_no, ring_times,
                    call_start_time, call_connect_time, call_end_time,
                    call_duration_time, call_second_dtmf, voice_record
             FROM dbo.record WHERE ID = :id'
        );
        $stmt->execute(['id' => $id]);
        return $stmt->fetch();
    }

    /**
     * Accepts a unix timestamp (int/numeric-string) or an ISO-8601 string;
     * returns a plain int Unix-epoch seconds value, matching this legacy
     * table's call_start_time/call_connect_time/call_end_time column type
     * (INT, not DATETIME2 - see doc/ADICTICallSystem.API-說明書.md's
     * legacy-schema research notes).
     */
    private function normalizeTimestamp($value): ?int
    {
        if ($value === null || $value === '') {
            return null;
        }
        if (is_numeric($value)) {
            return (int) $value;
        }
        $timestamp = strtotime((string) $value);
        return $timestamp !== false ? $timestamp : null;
    }

    private function formatTimestamp(?int $epochSeconds): ?string
    {
        return $epochSeconds !== null ? gmdate('Y-m-d\TH:i:s\Z', $epochSeconds) : null;
    }

    private function mapRow(array $row): array
    {
        return [
            'id' => (int) $row['id'],
            'machineNo' => $row['machine_id'] !== null ? (int) $row['machine_id'] : null,
            'callType' => (int) $row['call_type'],
            'outVport' => $row['out_vport_no'] !== null ? (int) $row['out_vport_no'] : null,
            'outPhyPort' => $row['out_phyport_no'] !== null ? (int) $row['out_phyport_no'] : null,
            // tel_no/call_second_dtmf/voice_record are CHAR (fixed-width) columns.
            'telNo' => $row['tel_no'] !== null ? rtrim($row['tel_no']) : null,
            'extNum' => $row['ext_num'] !== null ? (int) $row['ext_num'] : null,
            'fromExtNum' => $row['internal_call_from_ext_no'] !== null ? (int) $row['internal_call_from_ext_no'] : null,
            'toExtNum' => $row['internal_call_to_ext_no'] !== null ? (int) $row['internal_call_to_ext_no'] : null,
            'ringTimes' => $row['ring_times'] !== null ? (int) $row['ring_times'] : null,
            // call_start_time/connect_time/end_time are legacy int Unix-epoch
            // columns; formatted back to an ISO-ish string here so
            // OperatorWeb's formatTime() (which does a plain string
            // replace('T',' ').substring(0,19)) keeps working unmodified.
            'startTime' => $this->formatTimestamp($row['call_start_time']),
            'connectTime' => $this->formatTimestamp($row['call_connect_time']),
            'endTime' => $this->formatTimestamp($row['call_end_time']),
            'durationSec' => $row['call_duration_time'] !== null ? (int) $row['call_duration_time'] : null,
            'secondDtmf' => $row['call_second_dtmf'] !== null ? rtrim($row['call_second_dtmf']) : null,
            'voiceRecordUuid' => $row['voice_record'] !== null ? rtrim($row['voice_record']) : null,
        ];
    }
}
