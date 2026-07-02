<?php

namespace App\Controllers;

use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;

/**
 * One row per physical internal-line port (machineType, machineNo, phyPort).
 * In practice machineType is always 1 (PBX) since only PBX has extension
 * ports, but the table doesn't hard-code that assumption. `vport` is the
 * shared virtual extension-line number; rows are created/removed by
 * MachineController when a machine's extPortCount changes.
 */
class ExtlinePortController extends Controller
{
    /** GET /extline-ports?machineType=&machineNo=&phyPort=&vport= */
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

        $sql = 'SELECT id, machine_type, machine_no, phy_port, vport, ext_num, status, current_employee_id FROM dbo.extline_ports';
        if ($where) {
            $sql .= ' WHERE ' . implode(' AND ', $where);
        }
        $sql .= ' ORDER BY machine_type, machine_no, phy_port';

        $stmt = $this->db()->prepare($sql);
        $stmt->execute($params);

        Response::ok(array_map([$this, 'mapRow'], $stmt->fetchAll()));
    }

    /** GET /extline-ports/{id} */
    public function show(Request $request): void
    {
        $row = $this->fetchById((int) $request->params['id']);
        if (!$row) {
            Response::notFound('找不到這個內線埠。');
            return;
        }
        Response::ok($this->mapRow($row));
    }

    /** GET /extline-ports/by-vport/{vport} */
    public function showByVport(Request $request): void
    {
        $stmt = $this->db()->prepare(
            'SELECT id, machine_type, machine_no, phy_port, vport, ext_num, status, current_employee_id
             FROM dbo.extline_ports WHERE vport = :vport ORDER BY machine_type'
        );
        $stmt->execute(['vport' => (int) $request->params['vport']]);
        Response::ok(array_map([$this, 'mapRow'], $stmt->fetchAll()));
    }

    /** GET /extline-ports/by-ext-num/{machineType}/{machineNo}/{extNum} */
    public function showByExtNum(Request $request): void
    {
        $stmt = $this->db()->prepare(
            'SELECT id, machine_type, machine_no, phy_port, vport, ext_num, status, current_employee_id
             FROM dbo.extline_ports WHERE machine_type = :machine_type AND machine_no = :machine_no AND ext_num = :ext_num'
        );
        $stmt->execute([
            'machine_type' => (int) $request->params['machineType'],
            'machine_no' => (int) $request->params['machineNo'],
            'ext_num' => (int) $request->params['extNum'],
        ]);
        $row = $stmt->fetch();

        if (!$row) {
            Response::notFound('找不到這個內線埠。');
            return;
        }
        Response::ok($this->mapRow($row));
    }

    /** PATCH /extline-ports/{id}  { vport?, extNum?, status? } */
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
        if (($extNum = $request->input('extNum')) !== null) {
            $fields[] = 'ext_num = :ext_num';
            $params['ext_num'] = (int) $extNum;
        }
        if (($status = $request->input('status')) !== null) {
            $fields[] = 'status = :status';
            $params['status'] = (int) $status;
        }

        if (empty($fields)) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $sql = 'UPDATE dbo.extline_ports SET ' . implode(', ', $fields) . ' WHERE id = :id';
        $stmt = $this->db()->prepare($sql);

        try {
            $stmt->execute($params);
        } catch (\PDOException $e) {
            if (in_array((int) ($e->errorInfo[1] ?? 0), [2601, 2627], true)) {
                Response::error('這個分機號碼已經被其他埠使用了。', 409);
                return;
            }
            throw $e;
        }

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這個內線埠。');
            return;
        }

        Response::ok(null, '內線埠已更新。');
    }

    /** @return array|false */
    private function fetchById(int $id)
    {
        $stmt = $this->db()->prepare(
            'SELECT id, machine_type, machine_no, phy_port, vport, ext_num, status, current_employee_id FROM dbo.extline_ports WHERE id = :id'
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
            'extNum' => $row['ext_num'] !== null ? (int) $row['ext_num'] : null,
            'status' => (int) $row['status'],
            'currentEmployeeId' => $row['current_employee_id'] !== null ? (int) $row['current_employee_id'] : null,
        ];
    }
}
