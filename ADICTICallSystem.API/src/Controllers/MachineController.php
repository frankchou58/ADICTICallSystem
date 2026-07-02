<?php

namespace App\Controllers;

use App\Core\Database;
use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;

/**
 * Sub-machine slots: machineType 1 = PBX, 2 = CallerID Box, 3 = Voice Card;
 * machineNo is the 1-10 "合併機碼". Per clarified requirements, any
 * combination of machine types (including all three at once) is allowed
 * at the same machineNo, each independently configurable - there is no
 * cross-type port-count validation. The one remaining rule: only PBX
 * (type 1) may have extension ports, since only PBX has FXO/FXS internal
 * line capability.
 *
 * When out_port_count/ext_port_count changes, this controller keeps
 * dbo.outline_ports / dbo.extline_ports in sync: growing a count inserts
 * new physical-port rows (vport left unassigned), shrinking it removes
 * the phy ports beyond the new count. The vport "pool" is therefore
 * exactly as large as the sum of actual configured physical ports -
 * there's no fixed pre-allocated ceiling.
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
            $where[] = 'machine_no = :machine_no';
            $params['machine_no'] = (int) $no;
        }

        $sql = 'SELECT machine_type, machine_no, alias, out_port_count, ext_port_count, ip_address, sw_version, is_connected, last_seen_at
                FROM dbo.machines';
        if ($where) {
            $sql .= ' WHERE ' . implode(' AND ', $where);
        }
        $sql .= ' ORDER BY machine_type, machine_no';

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
        if (!$this->requireRole($request, ['admin', 'supervisor'])) {
            return;
        }

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
        $newOutPorts = null;
        $newExtPorts = null;

        if (($alias = $request->input('alias')) !== null) {
            $fields[] = 'alias = :alias';
            $params['alias'] = $alias;
        }

        if (($outPorts = $request->input('outPortCount')) !== null) {
            $newOutPorts = (int) $outPorts;
            if ($newOutPorts < 0) {
                throw new ValidationException('outPortCount 不能是負數。');
            }
            $fields[] = 'out_port_count = :out_port_count';
            $params['out_port_count'] = $newOutPorts;
        }

        if (($extPorts = $request->input('extPortCount')) !== null) {
            $newExtPorts = (int) $extPorts;
            if ($newExtPorts < 0) {
                throw new ValidationException('extPortCount 不能是負數。');
            }
            if ($machineType !== 1 && $newExtPorts > 0) {
                throw new ValidationException('只有 machineType 1（電話總機）支援內線埠。');
            }
            $fields[] = 'ext_port_count = :ext_port_count';
            $params['ext_port_count'] = $newExtPorts;
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

        $sql = 'UPDATE dbo.machines SET ' . implode(', ', $fields) . '
                WHERE machine_type = :machine_type AND machine_no = :machine_no';
        $stmt = $this->db()->prepare($sql);
        $this->executeWithParams($stmt, $params, ['alias']);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這個機器槽位。');
            return;
        }

        if ($newOutPorts !== null) {
            $this->syncPorts('outline_ports', $machineType, $machineNo, $newOutPorts);
        }
        if ($newExtPorts !== null) {
            $this->syncPorts('extline_ports', $machineType, $machineNo, $newExtPorts);
        }

        Response::ok(null, '機器資料已更新。');
    }

    /**
     * Grows or shrinks the physical-port rows for one (machineType, machineNo)
     * in the given ports table so they exactly match $newCount rows
     * (phy_port 1..$newCount). New rows are auto-assigned the next
     * available vport in that table; shrinking simply deletes the rows
     * for phy ports beyond the new count (including any that already had
     * a vport/ext assignment - matches the legacy MFC UI's behavior of
     * warning that shrinking a count changes the physical/virtual mapping
     * but proceeding anyway).
     */
    private function syncPorts(string $table, int $machineType, int $machineNo, int $newCount): void
    {
        $db = $this->db();

        $stmt = $db->prepare("SELECT MAX(phy_port) AS max_phy FROM dbo.$table WHERE machine_type = :t AND machine_no = :n");
        $stmt->execute(['t' => $machineType, 'n' => $machineNo]);
        $currentMax = (int) ($stmt->fetchColumn() ?: 0);

        if ($newCount > $currentMax) {
            $vportStmt = $db->prepare("SELECT MAX(vport) AS max_vport FROM dbo.$table");
            $vportStmt->execute();
            $nextVport = (int) ($vportStmt->fetchColumn() ?: 0) + 1;

            $insert = $db->prepare(
                "INSERT INTO dbo.$table (machine_type, machine_no, phy_port, vport) VALUES (:t, :n, :p, :v)"
            );
            for ($phyPort = $currentMax + 1; $phyPort <= $newCount; $phyPort++) {
                $insert->execute(['t' => $machineType, 'n' => $machineNo, 'p' => $phyPort, 'v' => $nextVport]);
                $nextVport++;
            }
        } elseif ($newCount < $currentMax) {
            $delete = $db->prepare("DELETE FROM dbo.$table WHERE machine_type = :t AND machine_no = :n AND phy_port > :max");
            $delete->execute(['t' => $machineType, 'n' => $machineNo, 'max' => $newCount]);
        }
    }

    /** @return array|false */
    private function fetchOne(int $machineType, int $machineNo)
    {
        $stmt = $this->db()->prepare(
            'SELECT machine_type, machine_no, alias, out_port_count, ext_port_count, ip_address, sw_version, is_connected, last_seen_at
             FROM dbo.machines WHERE machine_type = :machine_type AND machine_no = :machine_no'
        );
        $stmt->execute(['machine_type' => $machineType, 'machine_no' => $machineNo]);
        return $stmt->fetch();
    }

    private function mapRow(array $row): array
    {
        return [
            'machineType' => (int) $row['machine_type'],
            'machineNo' => (int) $row['machine_no'],
            'alias' => Database::decodeText($row['alias']),
            'outPortCount' => (int) $row['out_port_count'],
            'extPortCount' => (int) $row['ext_port_count'],
            'ipAddress' => $row['ip_address'],
            'swVersion' => $row['sw_version'],
            'isConnected' => (bool) $row['is_connected'],
            'lastSeenAt' => $row['last_seen_at'],
        ];
    }
}
