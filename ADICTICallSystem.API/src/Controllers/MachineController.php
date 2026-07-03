<?php

namespace App\Controllers;

use App\Core\Database;
use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;

/**
 * 2026-07-03: rewritten against the customer's pre-existing ADICTICallCenter
 * database's dbo.machine table (machine_type 1=PBX/2=CallerID Box/3=Voice
 * Card, machine_id is the 1-10 slot number - all 30 combinations are
 * pre-seeded rows, confirmed against the legacy PHP source in D:\ADICTICallSystem\API).
 *
 * Unlike this API's original design (where out_port_count/ext_port_count
 * changes drove creating/deleting rows in dbo.outline_ports/extline_ports),
 * this customer's dbo.outline/dbo.extline are a FIXED pool of 240 rows each
 * that already exist independently of any machine's port counts - the
 * counts here are purely descriptive (matches the legacy app's own
 * behaviour: nothing in the ~65 old endpoint files ever kept them in sync
 * with actual outline/extline row assignments either). So there is no
 * port-syncing side effect here anymore - admins assign which physical
 * outline/extline row belongs to which machine slot directly via
 * OutlinePortController/ExtlinePortController.
 */
class MachineController extends Controller
{
    private const TYPES = [1, 2, 3];

    /** GET /machines?machineType=&machineNo= */
    public function index(Request $request): void
    {
        $where = [];
        $params = [];
        if ($type = $request->query['machineType'] ?? null) {
            $where[] = 'machine_type = :machine_type';
            $params['machine_type'] = (int) $type;
        }
        if ($no = $request->query['machineNo'] ?? null) {
            $where[] = 'machine_id = :machine_no';
            $params['machine_no'] = (int) $no;
        }

        // RTRIM(alias) is load-bearing, not cosmetic: PDO_ODBC silently
        // returns NULL (not an error) when fetching this NCHAR(50) column
        // directly if it holds non-ASCII content padded out to its full
        // fixed width - confirmed by comparing against a raw DATALENGTH()
        // check on the same row, which still showed real bytes stored.
        // Plain-ASCII content in the same column fetches fine either way;
        // only wide/space-padded values trigger it. RTRIM shortens what
        // crosses the driver boundary and avoids the bug entirely.
        $sql = 'SELECT machine_type, machine_id, RTRIM(alias) AS alias, out_port_num, ext_port_num, ip_address, sw_version, is_connected, last_seen_at
                FROM dbo.machine';
        if ($where) {
            $sql .= ' WHERE ' . implode(' AND ', $where);
        }
        $sql .= ' ORDER BY machine_type, machine_id';

        $stmt = $this->db()->prepare($sql);
        $stmt->execute($params);

        Response::ok(array_map([$this, 'mapRow'], $stmt->fetchAll()));
    }

    /** GET /machines/{machineType}/{machineNo} */
    public function show(Request $request): void
    {
        $row = $this->fetchOne((int) $request->params['machineType'], (int) $request->params['machineNo']);
        if (!$row) {
            Response::notFound('找不到這個機器槽位。');
            return;
        }
        Response::ok($this->mapRow($row));
    }

    /** PATCH /machines/{machineType}/{machineNo}  { alias?, outPortCount?, extPortCount?, isConnected?, ipAddress? } */
    public function update(Request $request): void
    {
        $machineType = (int) $request->params['machineType'];
        $machineNo = (int) $request->params['machineNo'];

        if (!in_array($machineType, self::TYPES, true)) {
            throw new ValidationException('machineType 必須是 1（電話總機）、2（來電顯示盒）或 3（錄音卡）。');
        }
        if ($machineNo < 1 || $machineNo > 10) {
            throw new ValidationException('machineNo 必須介於 1 到 10 之間。');
        }

        $fields = [];
        $params = ['machine_type' => $machineType, 'machine_no' => $machineNo];

        if (($alias = $request->input('alias')) !== null) {
            $fields[] = 'alias = :alias';
            $params['alias'] = $alias;
        }

        if (($outPorts = $request->input('outPortCount')) !== null) {
            $newOutPorts = (int) $outPorts;
            if ($newOutPorts < 0) {
                throw new ValidationException('outPortCount 不能是負數。');
            }
            $fields[] = 'out_port_num = :out_port_num';
            $params['out_port_num'] = $newOutPorts;
        }

        if (($extPorts = $request->input('extPortCount')) !== null) {
            $newExtPorts = (int) $extPorts;
            if ($newExtPorts < 0) {
                throw new ValidationException('extPortCount 不能是負數。');
            }
            if ($machineType !== 1 && $newExtPorts > 0) {
                throw new ValidationException('只有 machineType 1（電話總機）支援內線埠。');
            }
            $fields[] = 'ext_port_num = :ext_port_num';
            $params['ext_port_num'] = $newExtPorts;
        }

        // Sent by the MFC Main Core's MachineLogin()/MachineLogout() handlers
        // when a sub-machine connects/disconnects over its own TCP protocol -
        // this endpoint is the only place is_connected/ip_address ever gets
        // written, there's no separate "connection event" endpoint.
        if (($isConnected = $request->input('isConnected')) !== null) {
            $fields[] = 'is_connected = :is_connected';
            $params['is_connected'] = $isConnected ? 1 : 0;
            $fields[] = 'last_seen_at = SYSUTCDATETIME()';
        }

        if (($ipAddress = $request->input('ipAddress')) !== null) {
            $fields[] = 'ip_address = :ip_address';
            $params['ip_address'] = $ipAddress;
        }

        if (empty($fields)) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $sql = 'UPDATE dbo.machine SET ' . implode(', ', $fields) . '
                WHERE machine_type = :machine_type AND machine_id = :machine_no';
        $stmt = $this->db()->prepare($sql);
        $this->executeWithParams($stmt, $params, ['alias']);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這個機器槽位。');
            return;
        }

        Response::ok(null, '機器資料已更新。');
    }

    /** @return array|false */
    private function fetchOne(int $machineType, int $machineNo)
    {
        $stmt = $this->db()->prepare(
            'SELECT machine_type, machine_id, RTRIM(alias) AS alias, out_port_num, ext_port_num, ip_address, sw_version, is_connected, last_seen_at
             FROM dbo.machine WHERE machine_type = :machine_type AND machine_id = :machine_no'
        );
        $stmt->execute(['machine_type' => $machineType, 'machine_no' => $machineNo]);
        return $stmt->fetch();
    }

    private function mapRow(array $row): array
    {
        return [
            'machineType' => (int) $row['machine_type'],
            'machineNo' => (int) $row['machine_id'],
            // Already RTRIM()'d in SQL (see index()/fetchOne()'s query) -
            // that's load-bearing there, not just cosmetic padding removal.
            'alias' => $row['alias'] !== null ? Database::decodeText($row['alias']) : null,
            'outPortCount' => (int) $row['out_port_num'],
            'extPortCount' => (int) $row['ext_port_num'],
            'ipAddress' => $row['ip_address'],
            'swVersion' => $row['sw_version'],
            'isConnected' => (bool) $row['is_connected'],
            'lastSeenAt' => $row['last_seen_at'],
        ];
    }
}
