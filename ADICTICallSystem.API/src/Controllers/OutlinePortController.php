<?php

namespace App\Controllers;

use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;

/**
 * One row per physical external-line port (machineType, machineNo, phyPort).
 * `vport` is the shared virtual line number - several rows across different
 * machine types can carry the same vport, meaning those devices are wired
 * to the same real phone line. Rows themselves are created/removed by
 * MachineController when a machine's outPortCount changes; this controller
 * only reads them and lets an admin reassign vport / in-use / call status.
 */
class OutlinePortController extends Controller
{
    /** GET /outline-ports?machineType=&machineNo=&phyPort=&vport= */
    public function index(Request $request): void
    {
        $where = [];
        $params = [];
        foreach (['machineType' => 'machine_type', 'machineNo' => 'machine_no', 'phyPort' => 'phy_port', 'vport' => 'vport'] as $q => $col) {
            if (isset($request->query[$q]) && $request->query[$q] !== '') {
                $where[] = "$col = :$col";
                $params[$col] = (int) $request->query[$q];
            }
        }

        $sql = 'SELECT id, machine_type, machine_no, phy_port, vport, in_use, call_status FROM dbo.outline_ports';
        if ($where) {
            $sql .= ' WHERE ' . implode(' AND ', $where);
        }
        $sql .= ' ORDER BY machine_type, machine_no, phy_port';

        $stmt = $this->db()->prepare($sql);
        $stmt->execute($params);

        Response::ok(array_map([$this, 'mapRow'], $stmt->fetchAll()));
    }

    /** GET /outline-ports/{id} */
    public function show(Request $request): void
    {
        $row = $this->fetchById((int) $request->params['id']);
        if (!$row) {
            Response::notFound('找不到這個外線埠。');
            return;
        }
        Response::ok($this->mapRow($row));
    }

    /** GET /outline-ports/by-vport/{vport} - every device currently sharing this virtual line. */
    public function showByVport(Request $request): void
    {
        $stmt = $this->db()->prepare(
            'SELECT id, machine_type, machine_no, phy_port, vport, in_use, call_status
             FROM dbo.outline_ports WHERE vport = :vport ORDER BY machine_type'
        );
        $stmt->execute(['vport' => (int) $request->params['vport']]);
        Response::ok(array_map([$this, 'mapRow'], $stmt->fetchAll()));
    }

    /** PATCH /outline-ports/{id}  { vport?, inUse?, callStatus? } */
    public function update(Request $request): void
    {
        if (!$this->requireRole($request, ['admin', 'supervisor'])) {
            return;
        }

        $id = (int) $request->params['id'];
        $fields = [];
        $params = ['id' => $id];

        if (($vport = $request->input('vport')) !== null) {
            $fields[] = 'vport = :vport';
            $params['vport'] = (int) $vport;
        }
        if (($inUse = $request->input('inUse')) !== null) {
            $fields[] = 'in_use = :in_use';
            $params['in_use'] = $inUse ? 1 : 0;
        }
        if (($callStatus = $request->input('callStatus')) !== null) {
            $fields[] = 'call_status = :call_status';
            $params['call_status'] = (int) $callStatus;
        }

        if (empty($fields)) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $sql = 'UPDATE dbo.outline_ports SET ' . implode(', ', $fields) . ' WHERE id = :id';
        $stmt = $this->db()->prepare($sql);
        $stmt->execute($params);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這個外線埠。');
            return;
        }

        Response::ok(null, '外線埠已更新。');
    }

    /** @return array|false */
    private function fetchById(int $id)
    {
        $stmt = $this->db()->prepare(
            'SELECT id, machine_type, machine_no, phy_port, vport, in_use, call_status FROM dbo.outline_ports WHERE id = :id'
        );
        $stmt->execute(['id' => $id]);
        return $stmt->fetch();
    }

    private function mapRow(array $row): array
    {
        return [
            'id' => (int) $row['id'],
            'machineType' => (int) $row['machine_type'],
            'machineNo' => (int) $row['machine_no'],
            'phyPort' => (int) $row['phy_port'],
            'vport' => $row['vport'] !== null ? (int) $row['vport'] : null,
            'inUse' => (bool) $row['in_use'],
            'callStatus' => (int) $row['call_status'],
        ];
    }
}
