<?php

namespace App\Controllers;

use App\Core\Database;
use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;
use PDO;

/**
 * Call records. A single PATCH endpoint replaces the legacy design's eight
 * separate UpdateCallRecordXxx.php single-field-update endpoints
 * (TelNo/TalkTime/CallType/EndTime/ExtNum/RingTimes/TalkDuration/
 * FromExtNo/ToExtNo/VoiceRecFileUUID) - the Call State Engine can just PATCH
 * whichever fields changed as a call progresses through its state machine.
 */
class CallRecordController extends Controller
{
    private const UPDATABLE_COLUMNS = [
        'telNo' => 'tel_no',
        'extNum' => 'ext_num',
        'fromExtNum' => 'from_ext_num',
        'toExtNum' => 'to_ext_num',
        'ringTimes' => 'ring_times',
        'durationSec' => 'duration_sec',
        'secondDtmf' => 'second_dtmf',
        'voiceRecordUuid' => 'voice_record_uuid',
        'customerId' => 'customer_id',
        'employeeId' => 'employee_id',
        'callType' => 'call_type',
    ];

    /**
     * POST /call-records
     * { machineNo, callType, outVport?, outPhyPort?, telNo?, extNum?, fromExtNum?, toExtNum?,
     *   ringTimes?, startTime?, connectTime?, endTime?, durationSec?, secondDtmf?, voiceRecordUuid?,
     *   customerId?, employeeId? }
     */
    public function create(Request $request): void
    {
        $machineNo = (int) $request->requireInput('machineNo');
        $callType = (int) $request->requireInput('callType');
        if (!in_array($callType, [0, 1, 2], true)) {
            throw new ValidationException('callType 必須是 0（撥入）、1（撥出）或 2（內線互撥）。');
        }

        $stmt = $this->db()->prepare(
            'INSERT INTO dbo.call_records
                (machine_no, call_type, out_vport, out_phy_port, tel_no, ext_num, from_ext_num, to_ext_num,
                 ring_times, start_time, connect_time, end_time, duration_sec, second_dtmf, voice_record_uuid,
                 customer_id, employee_id)
             OUTPUT INSERTED.id
             VALUES
                (:machine_no, :call_type, :out_vport, :out_phy_port, :tel_no, :ext_num, :from_ext_num, :to_ext_num,
                 :ring_times, :start_time, :connect_time, :end_time, :duration_sec, :second_dtmf, :voice_record_uuid,
                 :customer_id, :employee_id)'
        );

        // 不能直接 execute($array)：PDO_ODBC 對陣列裡的 PHP null 值型別推斷
        // 不可靠，常常會送成空字串而不是 SQL NULL，塞進 INT/DATETIME2 欄位
        // 就會炸 "Invalid character value for cast specification"。用共用的
        // executeWithParams() 明確依型別綁定（null 一定綁 PARAM_NULL）。
        $this->executeWithParams($stmt, [
            'machine_no' => $machineNo,
            'call_type' => $callType,
            'out_vport' => $request->input('outVport'),
            'out_phy_port' => $request->input('outPhyPort'),
            'tel_no' => $request->input('telNo'),
            'ext_num' => $request->input('extNum'),
            'from_ext_num' => $request->input('fromExtNum'),
            'to_ext_num' => $request->input('toExtNum'),
            'ring_times' => $request->input('ringTimes'),
            'start_time' => $this->normalizeTimestamp($request->input('startTime')),
            'connect_time' => $this->normalizeTimestamp($request->input('connectTime')),
            'end_time' => $this->normalizeTimestamp($request->input('endTime')),
            'duration_sec' => $request->input('durationSec'),
            'second_dtmf' => $request->input('secondDtmf'),
            'voice_record_uuid' => $request->input('voiceRecordUuid'),
            'customer_id' => $request->input('customerId'),
            'employee_id' => $request->input('employeeId'),
        ]);

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
     * secondDtmf, voiceRecordUuid, customerId, employeeId, callType, startTime, connectTime, endTime.
     */
    public function update(Request $request): void
    {
        $id = (int) $request->params['id'];

        $fields = [];
        $params = ['id' => $id];

        foreach (self::UPDATABLE_COLUMNS as $inputKey => $column) {
            $value = $request->input($inputKey);
            if ($value !== null) {
                $fields[] = "$column = :$column";
                $params[$column] = $value;
            }
        }
        foreach (['startTime' => 'start_time', 'connectTime' => 'connect_time', 'endTime' => 'end_time'] as $inputKey => $column) {
            $value = $request->input($inputKey);
            if ($value !== null) {
                $fields[] = "$column = :$column";
                $params[$column] = $this->normalizeTimestamp($value);
            }
        }

        if (empty($fields)) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $fields[] = 'updated_at = SYSUTCDATETIME()';
        $sql = 'UPDATE dbo.call_records SET ' . implode(', ', $fields) . ' WHERE id = :id';
        $stmt = $this->db()->prepare($sql);
        $stmt->execute($params);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這筆通話紀錄。');
            return;
        }

        Response::ok(null, '通話紀錄已更新。');
    }

    /**
     * GET /call-records?machineNo=&telNo=&employeeId=&callType=&dateFrom=&dateTo=
     *                   &custName=&custAddr=&status=&limit=&offset=
     * Consolidates the legacy QueryInBoundNumbers / QueryInboundDataByDate /
     * QueryInboundDataByTel / SearchInboundData endpoints into one filterable query.
     *
     * custName/custAddr/status exist for OperatorWeb's 來電報表 (inbound report),
     * which the legacy SearchInboundData.php filtered by customer name/address -
     * those live on dbo.customers, not dbo.call_records, hence the LEFT JOIN.
     * status is a convenience filter derived from call_type/connect_time since
     * there's no dedicated status column:
     *   answered = call_type 0 (inbound) with a connect_time recorded
     *   missed   = call_type 0 (inbound) with no connect_time
     *   outbound = call_type 1
     */
    public function index(Request $request): void
    {
        [$limit, $offset] = $this->paginationArgs($request);

        $where = [];
        $params = [];

        if ($machineNo = $request->query['machineNo'] ?? null) {
            $where[] = 'cr.machine_no = :machine_no';
            $params['machine_no'] = (int) $machineNo;
        }
        if ($telNo = $request->query['telNo'] ?? null) {
            $where[] = 'cr.tel_no LIKE :tel_no';
            $params['tel_no'] = '%' . $telNo . '%';
        }
        if ($employeeId = $request->query['employeeId'] ?? null) {
            $where[] = 'cr.employee_id = :employee_id';
            $params['employee_id'] = (int) $employeeId;
        }
        if (isset($request->query['callType'])) {
            $where[] = 'cr.call_type = :call_type';
            $params['call_type'] = (int) $request->query['callType'];
        }
        if ($dateFrom = $request->query['dateFrom'] ?? null) {
            $where[] = 'cr.start_time >= :date_from';
            $params['date_from'] = $dateFrom;
        }
        if ($dateTo = $request->query['dateTo'] ?? null) {
            $where[] = 'cr.start_time <= :date_to';
            $params['date_to'] = $dateTo;
        }
        if ($custName = $request->query['custName'] ?? null) {
            $where[] = 'c.name LIKE :cust_name';
            $params['cust_name'] = '%' . $custName . '%';
        }
        if ($custAddr = $request->query['custAddr'] ?? null) {
            // pdo_odbc 不支援同一個具名參數在同一句查詢裡重複使用（底層 ODBC
            // 只認位置式 ?，重複用同一個名字會讓後面 :offset/:limit 的綁定
            // 對不上位置，炸 "FETCH 子句指定的資料列數目必須大於零"）。
            // 三個地方各自用不同名字綁同一個值。
            $where[] = '(c.address LIKE :cust_addr1 OR c.county LIKE :cust_addr2 OR c.township LIKE :cust_addr3)';
            $like = '%' . $custAddr . '%';
            $params['cust_addr1'] = $like;
            $params['cust_addr2'] = $like;
            $params['cust_addr3'] = $like;
        }
        $status = $request->query['status'] ?? null;
        if ($status === 'answered') {
            $where[] = 'cr.call_type = 0 AND cr.connect_time IS NOT NULL';
        } elseif ($status === 'missed') {
            $where[] = 'cr.call_type = 0 AND cr.connect_time IS NULL';
        } elseif ($status === 'outbound') {
            $where[] = 'cr.call_type = 1';
        }

        $sql = 'SELECT cr.id, cr.machine_no, cr.call_type, cr.out_vport, cr.out_phy_port, cr.tel_no, cr.ext_num,
                       cr.from_ext_num, cr.to_ext_num, cr.ring_times, cr.start_time, cr.connect_time, cr.end_time,
                       cr.duration_sec, cr.second_dtmf, cr.voice_record_uuid, cr.customer_id, cr.employee_id,
                       c.name AS customer_name, c.county AS customer_county, c.township AS customer_township,
                       c.address AS customer_address
                FROM dbo.call_records cr
                LEFT JOIN dbo.customers c ON c.id = cr.customer_id';
        if ($where) {
            $sql .= ' WHERE ' . implode(' AND ', $where);
        }
        $sql .= ' ORDER BY cr.start_time DESC OFFSET :offset ROWS FETCH NEXT :limit ROWS ONLY';

        // custName/custAddr 的 LIKE pattern 帶的是使用者輸入的中文字，
        // 一樣要走 Database::bindText() 做內碼轉換，不然在這台機器的
        // pdo_odbc 下比對永遠不會中（原因跟 machines.alias 那次一樣：
        // pdo_odbc 綁字串是用系統 ANSI 內碼，不是 UTF-8）。
        $textParams = ['cust_name', 'cust_addr1', 'cust_addr2', 'cust_addr3'];
        $stmt = $this->db()->prepare($sql);
        foreach ($params as $key => $value) {
            if (in_array($key, $textParams, true)) {
                Database::bindText($stmt, ":$key", $value);
            } else {
                $stmt->bindValue($key, $value);
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
            'SELECT id, machine_no, call_type, out_vport, out_phy_port, tel_no, ext_num, from_ext_num, to_ext_num,
                    ring_times, start_time, connect_time, end_time, duration_sec, second_dtmf, voice_record_uuid,
                    customer_id, employee_id
             FROM dbo.call_records WHERE id = :id'
        );
        $stmt->execute(['id' => $id]);
        return $stmt->fetch();
    }

    /**
     * Accepts a unix timestamp (int/numeric-string) or an ISO-8601 string;
     * returns a format this box's ODBC Driver 18 for SQL Server actually
     * accepts for an implicit string->DATETIME2 cast. It rejects the ISO
     * 8601 "T"/"Z" separators outright ("Invalid character value for cast
     * specification") - has to be plain "Y-m-d H:i:s".
     */
    private function normalizeTimestamp($value): ?string
    {
        if ($value === null || $value === '') {
            return null;
        }
        if (is_numeric($value)) {
            return gmdate('Y-m-d H:i:s', (int) $value);
        }
        $normalized = str_replace('T', ' ', (string) $value);
        $normalized = rtrim($normalized, 'Zz');
        return trim($normalized);
    }

    private function mapRow(array $row): array
    {
        return [
            'id' => (int) $row['id'],
            'machineNo' => (int) $row['machine_no'],
            'callType' => (int) $row['call_type'],
            'outVport' => $row['out_vport'] !== null ? (int) $row['out_vport'] : null,
            'outPhyPort' => $row['out_phy_port'] !== null ? (int) $row['out_phy_port'] : null,
            'telNo' => $row['tel_no'],
            'extNum' => $row['ext_num'] !== null ? (int) $row['ext_num'] : null,
            'fromExtNum' => $row['from_ext_num'] !== null ? (int) $row['from_ext_num'] : null,
            'toExtNum' => $row['to_ext_num'] !== null ? (int) $row['to_ext_num'] : null,
            'ringTimes' => $row['ring_times'] !== null ? (int) $row['ring_times'] : null,
            'startTime' => $row['start_time'],
            'connectTime' => $row['connect_time'],
            'endTime' => $row['end_time'],
            'durationSec' => $row['duration_sec'] !== null ? (int) $row['duration_sec'] : null,
            'secondDtmf' => $row['second_dtmf'],
            'voiceRecordUuid' => $row['voice_record_uuid'],
            'customerId' => $row['customer_id'] !== null ? (int) $row['customer_id'] : null,
            'employeeId' => $row['employee_id'] !== null ? (int) $row['employee_id'] : null,
            // 只有 index() 的查詢有 LEFT JOIN customers，show()/fetchById() 沒有，
            // 用 ?? 保護，這幾個欄位在那邊就單純是 null。
            'customerName' => isset($row['customer_name']) ? Database::decodeText($row['customer_name']) : null,
            'customerCounty' => isset($row['customer_county']) ? Database::decodeText($row['customer_county']) : null,
            'customerTownship' => isset($row['customer_township']) ? Database::decodeText($row['customer_township']) : null,
            'customerAddress' => isset($row['customer_address']) ? Database::decodeText($row['customer_address']) : null,
        ];
    }
}
