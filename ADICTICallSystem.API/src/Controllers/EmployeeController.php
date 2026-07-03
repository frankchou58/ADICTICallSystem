<?php

namespace App\Controllers;

use App\Core\Auth;
use App\Core\Database;
use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;

/**
 * 2026-07-03: rewritten against the customer's pre-existing ADICTICallCenter
 * database - "employees" are rows in the legacy dbo.operators table
 * (employee_no/password_hash/role/is_disabled/session_* columns added by
 * sql/migrate-legacy-adicticallcenter.sql; no new tables were created).
 * `role` is one of operator | supervisor | admin (the old dbo.admin
 * singleton table is left untouched and unused - an "admin" is now just an
 * operators row with role='admin'). machineMask reuses operators.machine_id,
 * which the legacy code already treated as a bit-field (see
 * SetOperatorMachineID.php) - an ordinary operator has exactly one bit set.
 *
 * Dropped in this rewrite: the "虛擬內線指派" (employee↔ext-line) feature
 * added 2026-07-02 needed a new dbo.employee_ext_lines join table, which
 * isn't possible here (no new tables allowed) - extVports/bindExtLine/
 * unbindExtLine are gone until a column-based redesign is agreed on.
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

        // employee_id (the legacy int column, distinct from the new
        // employee_no this API actually authenticates with) has a
        // pre-existing UNIQUE constraint from before this rewrite
        // (UQ__operator__...) that isn't filtered to exclude NULLs - SQL
        // Server only allows ONE row with employee_id IS NULL per that
        // constraint, so leaving it unset would let exactly one employee
        // ever be created. Give every new row a synthetic-but-unique value
        // instead of leaving it NULL; UPDLOCK+HOLDLOCK on the MAX()
        // subquery avoids a race between two concurrent creates picking
        // the same next value (this table sees admin-only, low-frequency
        // writes, so a table-level lock for the duration of one INSERT is
        // an acceptable cost here).
        $stmt = $this->db()->prepare(
            'INSERT INTO dbo.operators (employee_id, employee_no, password_hash, name, role)
             OUTPUT INSERTED.ID
             VALUES ((SELECT ISNULL(MAX(employee_id), 0) + 1 FROM dbo.operators WITH (UPDLOCK, HOLDLOCK)),
                     :employee_no, :password_hash, :name, :role)'
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
                // {$employeeNo} (braced), not "$employeeNo」" bare: PHP's
                // double-quoted-string variable lexer treats bytes
                // \x80-\xFF as valid identifier characters (legacy
                // behaviour), so a bare $var immediately followed by a
                // full-width CJK character (whose UTF-8 encoding starts
                // with such a byte) gets greedily absorbed into the
                // variable name instead of ending at the intended
                // boundary - silently producing an undefined-variable
                // notice and an empty interpolation. Confirmed reproducing
                // against this exact line before this fix.
                Response::error("員工編號「{$employeeNo}」已經存在。", 409);
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

        $sql = 'SELECT ID AS id, employee_no, name, role, machine_id, ext_num, login_time, logout_time, is_disabled
                FROM dbo.operators';
        $params = [];
        if ($role) {
            $sql .= ' WHERE role = :role';
            $params['role'] = $role;
        }
        $sql .= ' ORDER BY ID OFFSET :offset ROWS FETCH NEXT :limit ROWS ONLY';

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
            'SELECT ID AS id, employee_no, name FROM dbo.operators WHERE ext_num = :ext_num AND is_disabled = 0'
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
            'SELECT ID AS id, employee_no, name, role, machine_id, ext_num, login_time, logout_time, is_disabled
             FROM dbo.operators WHERE ID = :id'
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

        $sql = 'UPDATE dbo.operators SET ' . implode(', ', $fields) . ' WHERE ID = :id';
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
            $stmt = $this->db()->prepare('SELECT password_hash FROM dbo.operators WHERE ID = :id');
            $stmt->execute(['id' => $id]);
            $row = $stmt->fetch();
            if (!$row || !Auth::verifyPassword($currentPassword, $row['password_hash'])) {
                Response::unauthorized('目前密碼輸入錯誤。');
                return;
            }
        }

        $stmt = $this->db()->prepare('UPDATE dbo.operators SET password_hash = :hash WHERE ID = :id');
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
            ? 'UPDATE dbo.operators SET machine_id = ISNULL(machine_id, 0) | :bit WHERE ID = :id'
            : 'UPDATE dbo.operators SET machine_id = ISNULL(machine_id, 0) & ~:bit WHERE ID = :id';

        $stmt = $this->db()->prepare($sql);
        $stmt->execute(['bit' => $bit, 'id' => $id]);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這位員工。');
            return;
        }

        Response::ok(null, $set ? '座位機碼已綁定。' : '座位機碼已解除綁定。');
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
            'machineMask' => (int) $row['machine_id'],
            'extNum' => $row['ext_num'],
            // login_time/logout_time are legacy int Unix-epoch columns, not DATETIME2.
            'loginAt' => $row['login_time'] !== null ? (int) $row['login_time'] : null,
            'logoutAt' => $row['logout_time'] !== null ? (int) $row['logout_time'] : null,
            'isDisabled' => (bool) $row['is_disabled'],
        ];
    }

    private function mapRows(array $rows): array
    {
        return array_map([$this, 'mapRow'], $rows);
    }
}
