<?php

namespace App\Controllers;

use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;

/**
 * 2026-07-03: rewritten against the customer's pre-existing ADICTICallCenter
 * database's dbo.extline table - unlike dbo.outline, this one already is a
 * fixed pool of 240 rows shaped one-row-per-physical-port (port_no is the
 * vport, phy_port is the physical port, sub_program_id is the machine slot),
 * so the mapping onto this API's original "one row per physical port"
 * contract is direct - no id-encoding tricks needed. machineType is always
 * hard-coded to 1 (PBX) since only PBX has extension ports (matches the
 * legacy code, which never varies it either).
 *
 * `currentEmployeeId` is always returned as null: the legacy column that
 * would map to it, operator_uuid (a CHAR(33) uuid, not an int id), was
 * never populated by any endpoint in the old system (see
 * doc/ADICTICallSystem.API-說明書.md's legacy-schema research notes) and
 * isn't the right data type to hold an operators.ID value anyway.
 */
class ExtlinePortController extends Controller
{
    private const MACHINE_TYPE_PBX = 1;

    /** GET /extline-ports?machineType=&machineNo=&phyPort=&vport= */
    public function index(Request $request): void
    {
        if (isset($request->query['machineType']) && $request->query['machineType'] !== '' && (int) $request->query['machineType'] !== self::MACHINE_TYPE_PBX) {
            Response::ok([]);
            return;
        }

        // sub_program_id IS NOT NULL 一定要加：dbo.extline 是固定 240 筆
        // 的池子，沒有真正指派給任何機碼的列永遠都在，不像這支 API 原本
        // 設計假設的「沒配置的實體埠根本不會是一筆列」。不濾掉的話，前端
        // (ADICTICallCenter.Web 的機器設定頁)會看到一堆機碼=0 的幽靈列，
        // 即使 outPortCount/extPortCount 已經改成 0 也還在。這裡刻意用
        // sub_program_id 而非 phy_port 判斷是否「真的佈線」——實測發現
        // phy_port 這台資料庫從一開始建立就被批次塞成等於 port_no（240
        // 筆幾乎全部都有值），只有 sub_program_id 才是真正代表「這個
        // 實體埠現在有指派給某個機碼」的欄位。
        $where = ['sub_program_id IS NOT NULL'];
        $params = [];
        foreach (['machineNo' => 'sub_program_id', 'phyPort' => 'phy_port', 'vport' => 'port_no'] as $q => $col) {
            if (isset($request->query[$q]) && $request->query[$q] !== '') {
                $where[] = "$col = :$col";
                $params[$col] = (int) $request->query[$q];
            }
        }

        $sql = 'SELECT ID AS id, sub_program_id, phy_port, port_no, ext_no, ext_status FROM dbo.extline
                WHERE ' . implode(' AND ', $where) . '
                ORDER BY sub_program_id, phy_port';

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
            'SELECT ID AS id, sub_program_id, phy_port, port_no, ext_no, ext_status
             FROM dbo.extline WHERE port_no = :port_no AND sub_program_id IS NOT NULL ORDER BY ID'
        );
        $stmt->execute(['port_no' => (int) $request->params['vport']]);
        Response::ok(array_map([$this, 'mapRow'], $stmt->fetchAll()));
    }

    /** GET /extline-ports/by-ext-num/{machineType}/{machineNo}/{extNum} */
    public function showByExtNum(Request $request): void
    {
        if ((int) $request->params['machineType'] !== self::MACHINE_TYPE_PBX) {
            Response::notFound('找不到這個內線埠。');
            return;
        }

        $stmt = $this->db()->prepare(
            'SELECT ID AS id, sub_program_id, phy_port, port_no, ext_no, ext_status
             FROM dbo.extline WHERE sub_program_id = :sub_program_id AND ext_no = :ext_no'
        );
        $stmt->execute([
            'sub_program_id' => (int) $request->params['machineNo'],
            'ext_no' => (int) $request->params['extNum'],
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
        $id = (int) $request->params['id'];
        $fields = [];
        $params = ['id' => $id];

        if (($vport = $request->input('vport')) !== null) {
            $fields[] = 'port_no = :port_no';
            $params['port_no'] = (int) $vport;
        }
        if (($extNum = $request->input('extNum')) !== null) {
            $fields[] = 'ext_no = :ext_no';
            $params['ext_no'] = (int) $extNum;
        }
        if (($status = $request->input('status')) !== null) {
            $fields[] = 'ext_status = :ext_status';
            $params['ext_status'] = (int) $status;
        }

        if (empty($fields)) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $sql = 'UPDATE dbo.extline SET ' . implode(', ', $fields) . ' WHERE ID = :id';
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

    /**
     * DELETE /extline-ports/{id} - unassign this physical port from whichever
     * machine it currently belongs to (clears sub_program_id/phy_port).
     * Added 2026-07-03 for the same reason as OutlinePortController::destroy()
     * - reducing extPortCount used to delete the row outright in this API's
     * original design; against this customer's fixed 240-row dbo.extline
     * pool there's nothing to delete, so this clears the machine association
     * instead. port_no (vport)/ext_no are left as-is - only the physical
     * wiring to a machine is cleared.
     */
    public function destroy(Request $request): void
    {
        $id = (int) $request->params['id'];

        $stmt = $this->db()->prepare('UPDATE dbo.extline SET sub_program_id = NULL, phy_port = NULL WHERE ID = :id');
        $stmt->execute(['id' => $id]);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這個內線埠。');
            return;
        }

        Response::ok(null, '內線埠已取消指派。');
    }

    /** @return array|false */
    private function fetchById(int $id)
    {
        $stmt = $this->db()->prepare(
            'SELECT ID AS id, sub_program_id, phy_port, port_no, ext_no, ext_status FROM dbo.extline WHERE ID = :id'
        );
        $stmt->execute(['id' => $id]);
        $row = $stmt->fetch();

        // 沒有真正指派給任何機碼(sub_program_id 為 NULL)就當作這個實體
        // 埠不存在，跟 index()/showByVport() 的過濾邏輯一致。
        if (!$row || $row['sub_program_id'] === null) {
            return false;
        }
        return $row;
    }

    private function mapRow(array $row): array
    {
        return [
            'id' => (int) $row['id'],
            'machineType' => self::MACHINE_TYPE_PBX,
            'machineNo' => $row['sub_program_id'] !== null ? (int) $row['sub_program_id'] : 0,
            'phyPort' => (int) $row['phy_port'],
            'vport' => $row['port_no'] !== null ? (int) $row['port_no'] : null,
            'extNum' => $row['ext_no'] !== null ? (int) $row['ext_no'] : null,
            'status' => (int) $row['ext_status'],
            'currentEmployeeId' => null,
        ];
    }
}
