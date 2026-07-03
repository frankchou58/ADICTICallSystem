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

        // 業務規則（2026-07-03 確認）：同一個合併機碼(machineNo)同一時間
        // 只能有一種類型（交換機/來電盒/語音卡）的外線或內線數量不是
        // 0——要換成別的類型之前，必須先把目前那個類型的外線、內線數量
        // 都改回 0。只有「這次更新完之後，這個類型的外線或內線數量會變
        // 成大於 0」才需要檢查；把數量改回 0（釋放這個機碼）永遠允許。
        if (isset($newOutPorts) || isset($newExtPorts)) {
            $currentRow = $this->fetchOne($machineType, $machineNo);
            $resultingOut = $newOutPorts ?? (int) ($currentRow['out_port_num'] ?? 0);
            $resultingExt = $newExtPorts ?? (int) ($currentRow['ext_port_num'] ?? 0);

            if ($resultingOut > 0 || $resultingExt > 0) {
                $conflict = $this->db()->prepare(
                    'SELECT TOP 1 machine_type FROM dbo.machine
                     WHERE machine_id = :machine_no AND machine_type <> :machine_type
                       AND (out_port_num > 0 OR ext_port_num > 0)'
                );
                $conflict->execute(['machine_no' => $machineNo, 'machine_type' => $machineType]);
                $conflictType = $conflict->fetchColumn();
                if ($conflictType !== false) {
                    $typeLabel = [1 => '交換機', 2 => '來電盒', 3 => '語音卡'][(int) $conflictType];
                    throw new ValidationException(
                        "合併機碼 $machineNo 目前已經被「{$typeLabel}類型」使用（外線或內線數量不是 0），".
                        "請先把{$typeLabel}類型的外線、內線數量都改成 0，才能在這個機碼設定其他類型。"
                    );
                }
            }
        }

        $sql = 'UPDATE dbo.machine SET ' . implode(', ', $fields) . '
                WHERE machine_type = :machine_type AND machine_id = :machine_no';
        $stmt = $this->db()->prepare($sql);
        $this->executeWithParams($stmt, $params, ['alias']);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這個機器槽位。');
            return;
        }

        // 子機斷線時，把它名下佈線的外線/內線通話中狀態一起清掉——實體
        // 裝置都斷線了，不可能還有真正在進行中的通話，殘留的 line_in_used/
        // line_status/ext_status 只是斷線前最後一刻的快照，不清掉的話畫面
        // 會出現「未連線」卻同時顯示「通話中」這種自相矛盾的狀態（使用者
        // 回報過這個現象）。只在明確收到 isConnected=false 時才清，收到
        // true 或沒帶這個欄位都不動。
        if ($isConnected === false) {
            $resetOut = $this->db()->prepare(
                'UPDATE dbo.outline SET line_in_used = 0, line_status = 0 WHERE machine_id = :machine_no'
            );
            $resetOut->execute(['machine_no' => $machineNo]);

            if ($machineType === 1) {
                $resetExt = $this->db()->prepare(
                    'UPDATE dbo.extline SET ext_status = 0 WHERE sub_program_id = :machine_no'
                );
                $resetExt->execute(['machine_no' => $machineNo]);
            }
        }

        // 讓實體埠列表跟著 outPortCount/extPortCount 這個數字走，行為對齊
        // 這支 API 原本的設計（改數量會同步增減實體埠列）——這台部署的
        // dbo.outline/dbo.extline 沒辦法真的新增/刪除列（固定 240 筆的
        // 池子），所以用「認領一筆目前完全沒有被任何機碼用過的閒置列」
        // 取代「新增一列」，用「清掉機碼關聯」取代「刪除一列」。
        // 縮小數量：清掉超出新數量的舊指派，跟以前一樣。
        // 放大數量：從池子裡挑還沒被任何機碼用過的列，依序認領補滿新
        // 數量涵蓋的實體埠編號 1..N（如果池子裡完全沒有閒置列可以認領，
        // 就只能保持現況——這台部署沒辦法生出全新的實體埠，需要另外請
        // 資料庫管理員確認 dbo.outline/dbo.extline 裡是否還有真正閒置
        // 的列）。
        // outPortFailedCount/extPortFailedCount：新數量涵蓋範圍裡有幾個實體
        // 埠因為池子裡已經沒有真正閒置的列可以認領而指派失敗（2026-07-03
        // 之前這個數字是由 MFC 端自己重新走一遍全部 240 個虛擬埠、逐一發送
        // HTTP 請求算出來的，一次設定要等好幾百次同步 API 呼叫跑完，非常
        // 慢；現在改成這裡直接算好、跟著這次 PATCH 的回應一起回傳，MFC
        // 端不用再自己重複這整套邏輯）。
        $outFailedCount = 0;
        $extFailedCount = 0;
        if (isset($newOutPorts)) {
            $outFailedCount = $this->syncOutlinePorts($machineType, $machineNo, $newOutPorts);
        }
        if (isset($newExtPorts) && $machineType === 1) {
            $extFailedCount = $this->syncExtlinePorts($machineNo, $newExtPorts);
        }

        Response::ok([
            'outPortFailedCount' => $outFailedCount,
            'extPortFailedCount' => $extFailedCount,
        ], '機器資料已更新。');
    }

    private const OUTLINE_TYPE_COLUMNS = [1 => 'phy_port_typeA', 2 => 'phy_port_typeB', 3 => 'phy_port_typeC'];

    private function syncOutlinePorts(int $machineType, int $machineNo, int $newCount): int
    {
        $column = self::OUTLINE_TYPE_COLUMNS[$machineType];

        $stmt = $this->db()->prepare(
            "SELECT line_no, phy_port_typeA, phy_port_typeB, phy_port_typeC, $column AS phy_port
             FROM dbo.outline WHERE machine_id = :machine_no AND $column IS NOT NULL"
        );
        $stmt->execute(['machine_no' => $machineNo]);
        $wired = $stmt->fetchAll();

        $wiredPhyPorts = array_map(static fn ($row) => (int) $row['phy_port'], $wired);

        // 縮小：清掉超出新數量的舊指派。
        foreach ($wired as $row) {
            if ((int) $row['phy_port'] <= $newCount) {
                continue;
            }
            $otherTypesStillSet = false;
            foreach (self::OUTLINE_TYPE_COLUMNS as $otherColumn) {
                if ($otherColumn !== $column && $row[$otherColumn] !== null) {
                    $otherTypesStillSet = true;
                    break;
                }
            }
            $clearSql = $otherTypesStillSet
                ? "UPDATE dbo.outline SET $column = NULL WHERE line_no = :line_no"
                : "UPDATE dbo.outline SET $column = NULL, machine_id = NULL WHERE line_no = :line_no";
            $this->db()->prepare($clearSql)->execute(['line_no' => $row['line_no']]);
        }

        // 放大：把 1..newCount 裡還沒被指派的實體埠，從池子裡認領閒置列來補上。
        $missingPhyPorts = array_diff($newCount >= 1 ? range(1, $newCount) : [], $wiredPhyPorts);
        if (empty($missingPhyPorts)) {
            return 0;
        }

        $findFree = $this->db()->prepare(
            "SELECT TOP 1 line_no FROM dbo.outline WHERE $column IS NULL AND machine_id IS NULL ORDER BY line_no"
        );
        // 認領到的列要順便把 line_in_used/line_status 重置成待機 - 這台
        // 部署的 240 筆池子是既有資料庫，閒置列身上可能還留著很久以前
        // （或客戶原本系統）殘留的「通話中」狀態，剛認領的全新對應顯示
        // 成本來就在通話中會誤導管理員（實測發生過）。
        $claim = $this->db()->prepare(
            "UPDATE dbo.outline SET $column = :phy_port, machine_id = :machine_no, line_in_used = 0, line_status = 0 WHERE line_no = :line_no"
        );

        $remaining = count($missingPhyPorts);
        foreach ($missingPhyPorts as $phyPort) {
            $findFree->execute();
            $free = $findFree->fetch();
            if (!$free) {
                break; // 沒有閒置列可以認領了，剩下的埠（含這一個）都算失敗。
            }
            $claim->execute(['phy_port' => $phyPort, 'machine_no' => $machineNo, 'line_no' => $free['line_no']]);
            $remaining--;
        }
        return $remaining;
    }

    private function syncExtlinePorts(int $machineNo, int $newCount): int
    {
        $stmt = $this->db()->prepare(
            'SELECT phy_port FROM dbo.extline WHERE sub_program_id = :machine_no'
        );
        $stmt->execute(['machine_no' => $machineNo]);
        $wiredPhyPorts = array_map('intval', array_column($stmt->fetchAll(), 'phy_port'));

        // 縮小：清掉超出新數量的舊指派。
        $this->db()->prepare(
            'UPDATE dbo.extline SET sub_program_id = NULL, phy_port = NULL
             WHERE sub_program_id = :machine_no AND phy_port > :new_count'
        )->execute(['machine_no' => $machineNo, 'new_count' => $newCount]);

        // 放大：認領閒置列補上 1..newCount 裡還沒被指派的實體埠。
        $missingPhyPorts = array_diff($newCount >= 1 ? range(1, $newCount) : [], $wiredPhyPorts);
        if (empty($missingPhyPorts)) {
            return 0;
        }

        $findFree = $this->db()->prepare(
            'SELECT TOP 1 ID FROM dbo.extline WHERE sub_program_id IS NULL ORDER BY port_no'
        );
        // 同樣重置 ext_status，理由跟 syncOutlinePorts() 裡的一樣。
        $claim = $this->db()->prepare(
            'UPDATE dbo.extline SET sub_program_id = :machine_no, phy_port = :phy_port, ext_status = 0 WHERE ID = :id'
        );

        $remaining = count($missingPhyPorts);
        foreach ($missingPhyPorts as $phyPort) {
            $findFree->execute();
            $free = $findFree->fetch();
            if (!$free) {
                break;
            }
            $claim->execute(['machine_no' => $machineNo, 'phy_port' => $phyPort, 'id' => $free['ID']]);
            $remaining--;
        }
        return $remaining;
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
