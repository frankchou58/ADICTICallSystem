<?php

namespace App\Controllers;

use App\Core\Auth;
use App\Core\Database;
use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;

/**
 * Employees unify the legacy "operators" and "admin" tables. `role` is one
 * of operator | supervisor | admin. machineMask is the same bit-field the
 * original design used for supervisors watching multiple machine codes -
 * an ordinary operator simply has exactly one bit set.
 */
class EmployeeController extends Controller
{
    private const ROLES = ['operator', 'supervisor', 'admin'];

    /** POST /employees  { employeeNo, password, name, role } */
    public function create(Request $request): void
    {
        if (!$this->requireRole($request, ['admin'])) {
            return;
        }

        $employeeNo = trim((string) $request->requireInput('employeeNo'));
        $password = (string) $request->requireInput('password');
        $name = $request->input('name');
        $role = $request->input('role', 'operator');

        if (!in_array($role, self::ROLES, true)) {
            throw new ValidationException('role 必須是以下其中一種：' . implode('、', self::ROLES));
        }

        $stmt = $this->db()->prepare(
            'INSERT INTO dbo.employees (employee_no, password_hash, name, role)
             OUTPUT INSERTED.id
             VALUES (:employee_no, :password_hash, :name, :role)'
        );
        try {
            $this->executeWithParams($stmt, [
                'employee_no' => $employeeNo,
                'password_hash' => Auth::hashPassword($password),
                'name' => $name,
                'role' => $role,
            ], ['name']);
        } catch (\PDOException $e) {
            if ($this->isUniqueViolation($e)) {
                Response::error("員工編號「$employeeNo」已經存在。", 409);
                return;
            }
            throw $e;
        }

        $id = (int) $stmt->fetchColumn();
        Response::created(['id' => $id], '員工已建立。');
    }

    /** GET /employees?role=&limit=&offset= */
    public function index(Request $request): void
    {
        if (!$this->requireRole($request, ['admin', 'supervisor'])) {
            return;
        }

        [$limit, $offset] = $this->paginationArgs($request);
        $role = $request->query['role'] ?? null;

        $sql = 'SELECT id, employee_no, name, role, machine_mask, ext_num, login_at, logout_at, is_disabled
                FROM dbo.employees';
        $params = [];
        if ($role) {
            $sql .= ' WHERE role = :role';
            $params['role'] = $role;
        }
        $sql .= ' ORDER BY id OFFSET :offset ROWS FETCH NEXT :limit ROWS ONLY';

        $stmt = $this->db()->prepare($sql);
        foreach ($params as $key => $value) {
            $stmt->bindValue($key, $value);
        }
        $stmt->bindValue('offset', $offset, \PDO::PARAM_INT);
        $stmt->bindValue('limit', $limit, \PDO::PARAM_INT);
        $stmt->execute();

        Response::ok($this->mapRows($stmt->fetchAll()));
    }

    /**
     * GET /employees/by-ext/{extNum} - resolve "who's on this call" for the
     * live board. Any authenticated employee may call this (not just
     * admin/supervisor like index()/show()) - it only returns the minimal
     * non-sensitive fields an operator needs to see a colleague's name next
     * to a ringing/talking line, not their full profile.
     */
    public function showByExtNum(Request $request): void
    {
        $extNum = (int) $request->params['extNum'];

        $stmt = $this->db()->prepare(
            'SELECT id, employee_no, name FROM dbo.employees WHERE ext_num = :ext_num AND is_disabled = 0'
        );
        $stmt->execute(['ext_num' => $extNum]);
        $row = $stmt->fetch();

        if (!$row) {
            Response::notFound('這個分機沒有對應的員工。');
            return;
        }

        Response::ok([
            'id' => (int) $row['id'],
            'employeeNo' => $row['employee_no'],
            'name' => Database::decodeText($row['name']),
        ]);
    }

    /** GET /employees/{id} */
    public function show(Request $request): void
    {
        $id = (int) $request->params['id'];

        if (!$this->canAccessEmployee($request, $id)) {
            Response::forbidden();
            return;
        }

        $stmt = $this->db()->prepare(
            'SELECT id, employee_no, name, role, machine_mask, ext_num, login_at, logout_at, is_disabled
             FROM dbo.employees WHERE id = :id'
        );
        $stmt->execute(['id' => $id]);
        $row = $stmt->fetch();

        if (!$row) {
            Response::notFound('找不到這位員工。');
            return;
        }

        Response::ok($this->mapRow($row));
    }

    /** PATCH /employees/{id}  { name?, role?, extNum?, isDisabled? } */
    public function update(Request $request): void
    {
        $id = (int) $request->params['id'];
        $isSelf = (int) $request->employee['id'] === $id;

        // Only admins may change role/isDisabled; employees may edit their own name.
        $wantsPrivilegedFields = $request->input('role') !== null || $request->input('isDisabled') !== null;
        if ($wantsPrivilegedFields && !$this->requireRole($request, ['admin'])) {
            return;
        }
        if (!$wantsPrivilegedFields && !$isSelf && !$this->requireRole($request, ['admin'])) {
            return;
        }

        $fields = [];
        $params = ['id' => $id];

        if (($name = $request->input('name')) !== null) {
            $fields[] = 'name = :name';
            $params['name'] = $name;
        }
        if (($role = $request->input('role')) !== null) {
            if (!in_array($role, self::ROLES, true)) {
                throw new ValidationException('role 必須是以下其中一種：' . implode('、', self::ROLES));
            }
            $fields[] = 'role = :role';
            $params['role'] = $role;
        }
        if (($extNum = $request->input('extNum')) !== null) {
            $fields[] = 'ext_num = :ext_num';
            $params['ext_num'] = (int) $extNum;
        }
        if (($isDisabled = $request->input('isDisabled')) !== null) {
            $fields[] = 'is_disabled = :is_disabled';
            $params['is_disabled'] = $isDisabled ? 1 : 0;
        }

        if (empty($fields)) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $fields[] = 'updated_at = SYSUTCDATETIME()';
        $sql = 'UPDATE dbo.employees SET ' . implode(', ', $fields) . ' WHERE id = :id';
        $stmt = $this->db()->prepare($sql);
        $this->executeWithParams($stmt, $params, ['name']);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這位員工。');
            return;
        }

        Response::ok(null, '員工資料已更新。');
    }

    /** PATCH /employees/{id}/password  { currentPassword?, newPassword } */
    public function changePassword(Request $request): void
    {
        $id = (int) $request->params['id'];
        $isSelf = (int) $request->employee['id'] === $id;
        $isAdmin = $request->employee['role'] === 'admin';

        if (!$isSelf && !$isAdmin) {
            Response::forbidden();
            return;
        }

        $newPassword = (string) $request->requireInput('newPassword');

        if ($isSelf) {
            $currentPassword = (string) $request->requireInput('currentPassword');
            $stmt = $this->db()->prepare('SELECT password_hash FROM dbo.employees WHERE id = :id');
            $stmt->execute(['id' => $id]);
            $row = $stmt->fetch();
            if (!$row || !Auth::verifyPassword($currentPassword, $row['password_hash'])) {
                Response::unauthorized('目前密碼輸入錯誤。');
                return;
            }
        }

        $stmt = $this->db()->prepare(
            'UPDATE dbo.employees SET password_hash = :hash, updated_at = SYSUTCDATETIME() WHERE id = :id'
        );
        $stmt->execute(['hash' => Auth::hashPassword($newPassword), 'id' => $id]);

        Response::ok(null, '密碼已變更。');
    }

    /** POST /employees/{id}/machines/{machineNo} - bind a machine code (1-10) to this employee. */
    public function bindMachine(Request $request): void
    {
        if (!$this->requireRole($request, ['admin'])) {
            return;
        }
        $this->toggleMachineBit($request, true);
    }

    /** DELETE /employees/{id}/machines/{machineNo} - unbind a machine code. */
    public function unbindMachine(Request $request): void
    {
        if (!$this->requireRole($request, ['admin'])) {
            return;
        }
        $this->toggleMachineBit($request, false);
    }

    private function toggleMachineBit(Request $request, bool $set): void
    {
        $id = (int) $request->params['id'];
        $machineNo = (int) $request->params['machineNo'];

        if ($machineNo < 1 || $machineNo > 10) {
            throw new ValidationException('machineNo 必須介於 1 到 10 之間。');
        }

        $bit = 1 << ($machineNo - 1);
        $sql = $set
            ? 'UPDATE dbo.employees SET machine_mask = machine_mask | :bit, updated_at = SYSUTCDATETIME() WHERE id = :id'
            : 'UPDATE dbo.employees SET machine_mask = machine_mask & ~:bit, updated_at = SYSUTCDATETIME() WHERE id = :id';

        $stmt = $this->db()->prepare($sql);
        $stmt->execute(['bit' => $bit, 'id' => $id]);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這位員工。');
            return;
        }

        Response::ok(null, $set ? '座位機碼已綁定。' : '座位機碼已解除綁定。');
    }

    /** POST /employees/{id}/ext-lines/{vport} - assign a virtual internal line (dbo.extline_ports.vport) to this employee. */
    public function bindExtLine(Request $request): void
    {
        if (!$this->requireRole($request, ['admin'])) {
            return;
        }

        $id = (int) $request->params['id'];
        $vport = (int) $request->params['vport'];

        $stmt = $this->db()->prepare('INSERT INTO dbo.employee_ext_lines (employee_id, ext_vport) VALUES (:id, :vport)');
        try {
            $stmt->execute(['id' => $id, 'vport' => $vport]);
        } catch (\PDOException $e) {
            if (!$this->isUniqueViolation($e)) {
                throw $e;
            }
            // 已經指派過了，當作成功（跟 toggleMachineBit 的 idempotent 精神一致）。
        }

        Response::ok(null, '虛擬內線已指派。');
    }

    /** DELETE /employees/{id}/ext-lines/{vport} - unassign a virtual internal line. */
    public function unbindExtLine(Request $request): void
    {
        if (!$this->requireRole($request, ['admin'])) {
            return;
        }

        $id = (int) $request->params['id'];
        $vport = (int) $request->params['vport'];

        $stmt = $this->db()->prepare('DELETE FROM dbo.employee_ext_lines WHERE employee_id = :id AND ext_vport = :vport');
        $stmt->execute(['id' => $id, 'vport' => $vport]);

        Response::ok(null, '虛擬內線已取消指派。');
    }

    private function fetchExtVports(int $employeeId): array
    {
        $stmt = $this->db()->prepare('SELECT ext_vport FROM dbo.employee_ext_lines WHERE employee_id = :id ORDER BY ext_vport');
        $stmt->execute(['id' => $employeeId]);
        return array_map('intval', array_column($stmt->fetchAll(), 'ext_vport'));
    }

    private function canAccessEmployee(Request $request, int $id): bool
    {
        return (int) $request->employee['id'] === $id
            || in_array($request->employee['role'], ['admin', 'supervisor'], true);
    }

    private function isUniqueViolation(\PDOException $e): bool
    {
        // SQL Server unique-constraint violation codes.
        return in_array((int) ($e->errorInfo[1] ?? 0), [2601, 2627], true);
    }

    private function mapRow(array $row): array
    {
        return [
            'id' => (int) $row['id'],
            'employeeNo' => $row['employee_no'],
            'name' => Database::decodeText($row['name']),
            'role' => $row['role'],
            'machineMask' => (int) $row['machine_mask'],
            'extVports' => $this->fetchExtVports((int) $row['id']),
            'extNum' => $row['ext_num'],
            'loginAt' => $row['login_at'],
            'logoutAt' => $row['logout_at'],
            'isDisabled' => (bool) $row['is_disabled'],
        ];
    }

    private function mapRows(array $rows): array
    {
        return array_map([$this, 'mapRow'], $rows);
    }
}
