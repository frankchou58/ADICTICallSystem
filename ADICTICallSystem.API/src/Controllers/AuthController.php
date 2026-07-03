<?php

namespace App\Controllers;

use App\Core\Auth;
use App\Core\Database;
use App\Core\Request;
use App\Core\Response;

/**
 * 2026-07-03: rewritten against the customer's pre-existing ADICTICallCenter
 * database. "Employees" here are rows in the legacy dbo.operators table
 * (with employee_no/password_hash/role/session_* columns added by
 * sql/migrate-legacy-adicticallcenter.sql - no new tables were created).
 * The old dbo.admin singleton table is left untouched and unused; an
 * "admin" is now just an operators row with role='admin'.
 */
class AuthController extends Controller
{
    /** POST /auth/login  { employeeNo, password } */
    public function login(Request $request): void
    {
        $employeeNo = trim((string) $request->requireInput('employeeNo'));
        $password = (string) $request->requireInput('password');

        $stmt = $this->db()->prepare(
            'SELECT ID AS id, employee_no, password_hash, name, role, machine_id, ext_num, is_disabled
             FROM dbo.operators WHERE employee_no = :employee_no'
        );
        $stmt->execute(['employee_no' => $employeeNo]);
        $operator = $stmt->fetch();

        // Always run password_verify, even on a miss, against a fixed dummy
        // hash so a timing difference can't be used to enumerate valid
        // employee numbers.
        $hashToCheck = $operator['password_hash'] ?? '$2y$10$invalidsaltinvalidsaltinvalidsaltinvalidsaltuY9Ktm';
        $passwordOk = Auth::verifyPassword($password, $hashToCheck);

        if (!$operator || !$passwordOk || (bool) $operator['is_disabled']) {
            Response::unauthorized('員工編號或密碼錯誤。');
            return;
        }

        $session = Auth::issueSession($this->db(), (int) $operator['id']);

        // login_time/logout_time are legacy int Unix-epoch columns (see
        // dbo.record's timestamp columns for the same convention).
        $update = $this->db()->prepare('UPDATE dbo.operators SET login_time = :now WHERE ID = :id');
        $update->execute(['now' => time(), 'id' => $operator['id']]);

        Response::ok([
            'token' => $session['token'],
            'expiresInSeconds' => $session['expiresInSeconds'],
            'employee' => [
                'id' => (int) $operator['id'],
                'employeeNo' => $operator['employee_no'],
                'name' => Database::decodeText($operator['name']),
                'role' => $operator['role'],
                'machineMask' => (int) $operator['machine_id'],
                'extNum' => $operator['ext_num'],
            ],
        ], '登入成功。');
    }

    /**
     * GET /auth/employee-numbers - public, no auth. Lets the login page show
     * a dropdown of employee numbers instead of a free-text field, matching
     * the legacy behaviour where the seat page (also pre-login) listed
     * employees for a QR-code claim flow. Only the minimal, non-sensitive
     * fields needed for a picker are returned - no role/extNum/etc.
     */
    public function employeeNumbers(Request $request): void
    {
        $stmt = $this->db()->prepare(
            'SELECT employee_no, name FROM dbo.operators WHERE is_disabled = 0 AND employee_no IS NOT NULL ORDER BY employee_no'
        );
        $stmt->execute();

        Response::ok(array_map(static function (array $row): array {
            return [
                'employeeNo' => $row['employee_no'],
                'name' => Database::decodeText($row['name']),
            ];
        }, $stmt->fetchAll()));
    }

    /** POST /auth/logout */
    public function logout(Request $request): void
    {
        $token = $request->bearerToken();
        if ($token) {
            Auth::revokeSession($this->db(), $token);
        }

        $stmt = $this->db()->prepare('UPDATE dbo.operators SET logout_time = :now WHERE ID = :id');
        $stmt->execute(['now' => time(), 'id' => $request->employee['id']]);

        Response::ok(null, '已登出。');
    }

    /** GET /auth/me */
    public function me(Request $request): void
    {
        Response::ok([
            'id' => (int) $request->employee['id'],
            'employeeNo' => $request->employee['employee_no'],
            'name' => Database::decodeText($request->employee['name']),
            'role' => $request->employee['role'],
            'machineMask' => (int) $request->employee['machine_id'],
            'extNum' => $request->employee['ext_num'],
        ]);
    }
}
