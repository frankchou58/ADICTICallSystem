<?php

namespace App\Controllers;

use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;

/**
 * 2026-07-03: rewritten against the customer's pre-existing ADICTICallCenter
 * database's dbo.outline table - a fixed pool of exactly 240 rows
 * (line_no 1-240), each row being one VIRTUAL outbound line with up to
 * three physical-port columns (phy_port_typeA/B/C, one per machineType
 * 1/2/3) plus a single machine_id shared by whichever type is set on that
 * row. This is the opposite shape from this API's original "one row per
 * physical port" design (dbo.outline_ports), which needed a new table this
 * customer's DB can't have - so this controller synthesizes the same
 * one-physical-port-per-item API contract on top of the fixed 240-row/
 * 3-column table:
 *   - synthetic id = line_no * 10 + typeIndex (typeIndex 1=PBX/2=CallerID/3=VoiceCard)
 *   - vport is always exactly line_no (there's no separate vport pool here)
 *   - machineNo is outline.machine_id - since that single column is shared
 *     across all three type columns on a row, a virtual line whose three
 *     device types are actually on three DIFFERENT machine slots can't be
 *     fully represented (known legacy limitation, not something this
 *     rewrite can fix without schema changes the customer can't allow).
 * Rows themselves always exist (all 240 are pre-seeded); "not configured"
 * for a given type is represented by that type's phy_port_type* column
 * being NULL, not by the row's absence.
 */
class OutlinePortController extends Controller
{
    private const TYPE_COLUMNS = [1 => 'phy_port_typeA', 2 => 'phy_port_typeB', 3 => 'phy_port_typeC'];

    /** GET /outline-ports?machineType=&machineNo=&phyPort=&vport= */
    public function index(Request $request): void
    {
        $rows = $this->fetchAllExpanded();

        $filters = [
            'machineType' => isset($request->query['machineType']) && $request->query['machineType'] !== '' ? (int) $request->query['machineType'] : null,
            'machineNo' => isset($request->query['machineNo']) && $request->query['machineNo'] !== '' ? (int) $request->query['machineNo'] : null,
            'phyPort' => isset($request->query['phyPort']) && $request->query['phyPort'] !== '' ? (int) $request->query['phyPort'] : null,
            'vport' => isset($request->query['vport']) && $request->query['vport'] !== '' ? (int) $request->query['vport'] : null,
        ];

        $rows = array_values(array_filter($rows, static function (array $row) use ($filters): bool {
            foreach ($filters as $key => $value) {
                if ($value !== null && $row[$key] !== $value) {
                    return false;
                }
            }
            return true;
        }));

        usort($rows, static fn ($a, $b) => [$a['machineType'], $a['machineNo'], $a['phyPort']] <=> [$b['machineType'], $b['machineNo'], $b['phyPort']]);

        Response::ok($rows);
    }

    /** GET /outline-ports/{id} */
    public function show(Request $request): void
    {
        $row = $this->fetchOne((int) $request->params['id']);
        if (!$row) {
            Response::notFound('找不到這個外線埠。');
            return;
        }
        Response::ok($row);
    }

    /** GET /outline-ports/by-vport/{vport} - every device currently sharing this virtual line. */
    public function showByVport(Request $request): void
    {
        $vport = (int) $request->params['vport'];
        $rows = array_values(array_filter($this->fetchAllExpanded(), static fn ($row) => $row['vport'] === $vport));
        usort($rows, static fn ($a, $b) => $a['machineType'] <=> $b['machineType']);
        Response::ok($rows);
    }

    /** PATCH /outline-ports/{id}  { vport?, inUse?, callStatus? } */
    public function update(Request $request): void
    {
        [$lineNo, $typeIndex, $column] = $this->decodeId((int) $request->params['id']);

        $stmt = $this->db()->prepare("SELECT machine_id, $column AS phy_port FROM dbo.outline WHERE line_no = :line_no");
        $stmt->execute(['line_no' => $lineNo]);
        $current = $stmt->fetch();

        if (!$current || $current['phy_port'] === null) {
            Response::notFound('找不到這個外線埠。');
            return;
        }

        $vport = $request->input('vport');
        $inUse = $request->input('inUse');
        $callStatus = $request->input('callStatus');

        if ($vport === null && $inUse === null && $callStatus === null) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $targetLineNo = $lineNo;

        if ($vport !== null && (int) $vport !== $lineNo) {
            $targetLineNo = (int) $vport;
            if ($targetLineNo < 1 || $targetLineNo > 240) {
                throw new ValidationException('vport 必須介於 1 到 240 之間。');
            }

            $db = $this->db();
            $db->beginTransaction();
            try {
                // 把這個實體埠（machine_id + 這一個 type 欄位）搬到新的 line_no（虛擬線路）上。
                $move = $db->prepare(
                    "UPDATE dbo.outline SET $column = :phy_port, machine_id = :machine_id WHERE line_no = :target"
                );
                $move->execute([
                    'phy_port' => $current['phy_port'],
                    'machine_id' => $current['machine_id'],
                    'target' => $targetLineNo,
                ]);

                $clear = $db->prepare("UPDATE dbo.outline SET $column = NULL WHERE line_no = :line_no");
                $clear->execute(['line_no' => $lineNo]);

                $db->commit();
            } catch (\Throwable $e) {
                $db->rollBack();
                throw $e;
            }
        }

        if ($inUse !== null || $callStatus !== null) {
            $fields = [];
            $params = ['line_no' => $targetLineNo];
            if ($inUse !== null) {
                $fields[] = 'line_in_used = :line_in_used';
                $params['line_in_used'] = $inUse ? 1 : 0;
            }
            if ($callStatus !== null) {
                $fields[] = 'line_status = :line_status';
                $params['line_status'] = (int) $callStatus;
            }
            $stmt = $this->db()->prepare('UPDATE dbo.outline SET ' . implode(', ', $fields) . ' WHERE line_no = :line_no');
            $stmt->execute($params);
        }

        Response::ok(null, '外線埠已更新。');
    }

    /** @return array{0:int,1:int,2:string} [lineNo, typeIndex, columnName] */
    private function decodeId(int $id): array
    {
        $typeIndex = $id % 10;
        $lineNo = intdiv($id, 10);
        if (!isset(self::TYPE_COLUMNS[$typeIndex]) || $lineNo < 1 || $lineNo > 240) {
            throw new ValidationException('找不到這個外線埠。');
        }
        return [$lineNo, $typeIndex, self::TYPE_COLUMNS[$typeIndex]];
    }

    /** @return array|null */
    private function fetchOne(int $id)
    {
        [$lineNo, $typeIndex, $column] = $this->decodeId($id);

        $stmt = $this->db()->prepare(
            "SELECT line_no, machine_id, line_in_used, line_status, $column AS phy_port FROM dbo.outline WHERE line_no = :line_no"
        );
        $stmt->execute(['line_no' => $lineNo]);
        $row = $stmt->fetch();

        if (!$row || $row['phy_port'] === null) {
            return null;
        }

        return $this->mapRow($row, $typeIndex);
    }

    private function fetchAllExpanded(): array
    {
        $stmt = $this->db()->prepare(
            'SELECT line_no, machine_id, line_in_used, line_status, phy_port_typeA, phy_port_typeB, phy_port_typeC FROM dbo.outline'
        );
        $stmt->execute();

        $result = [];
        foreach ($stmt->fetchAll() as $row) {
            foreach (self::TYPE_COLUMNS as $typeIndex => $column) {
                if ($row[$column] === null) {
                    continue;
                }
                $result[] = $this->mapRow([
                    'line_no' => $row['line_no'],
                    'machine_id' => $row['machine_id'],
                    'line_in_used' => $row['line_in_used'],
                    'line_status' => $row['line_status'],
                    'phy_port' => $row[$column],
                ], $typeIndex);
            }
        }
        return $result;
    }

    private function mapRow(array $row, int $typeIndex): array
    {
        return [
            'id' => ((int) $row['line_no']) * 10 + $typeIndex,
            'machineType' => $typeIndex,
            'machineNo' => $row['machine_id'] !== null ? (int) $row['machine_id'] : 0,
            'phyPort' => (int) $row['phy_port'],
            'vport' => (int) $row['line_no'],
            'inUse' => (bool) $row['line_in_used'],
            'callStatus' => (int) $row['line_status'],
        ];
    }
}
