<?php

namespace App\Controllers;

use App\Core\Auth;
use App\Core\Database;
use App\Core\Request;
use App\Core\Response;

class AuthController extends Controller
{
    /** POST /auth/login  { employeeNo, password } */
    public function login(Request $request): void
    {
        $employeeNo = trim((string) $request->requireInput('employeeNo'));
        $password = (string) $request->requireInput('password');

        $stmt = $this->db()->prepare(
            'SELECT id, employee_no, password_hash, name, role, machine_mask, ext_num, is_disabled
             FROM dbo.employees WHERE employee_no = :employee_no'
        );
        $stmt->execute(['employee_no' => $employeeNo]);
        $employee = $stmt->fetch();

        // Always run password_verify, even on a miss, against a fixed dummy
        // hash so a timing difference can't be used to enumerate valid
        // employee numbers.
        $hashToCheck = $employee['password_hash'] ?? '$2y$10$invalidsaltinvalidsaltinvalidsaltinvalidsaltuY9Ktm';
        $passwordOk = Auth::verifyPassword($password, $hashToCheck);

        if (!$employee || !$passwordOk || (bool) $employee['is_disabled']) {
            Response::unauthorized('員工編號或密碼錯誤。');
            return;
        }

        $session = Auth::issueSession($this->db(), (int) $employee['id']);

        $update = $this->db()->prepare('UPDATE dbo.employees SET login_at = SYSUTCDATETIME() WHERE id = :id');
        $update->execute(['id' => $employee['id']]);

        Response::ok([
            'token' => $session['token'],
            'expiresInSeconds' => $session['expiresInSeconds'],
            'employee' => [
                'id' => (int) $employee['id'],
                'employeeNo' => $employee['employee_no'],
                'name' => Database::decodeText($employee['name']),
                'role' => $employee['role'],
                'machineMask' => (int) $employee['machine_mask'],
                'extNum' => $employee['ext_num'],
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
            'SELECT employee_no, name FROM dbo.employees WHERE is_disabled = 0 ORDER BY employee_no'
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

        $stmt = $this->db()->prepare('UPDATE dbo.employees SET logout_at = SYSUTCDATETIME() WHERE id = :id');
        $stmt->execute(['id' => $request->employee['id']]);

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
            'machineMask' => (int) $request->employee['machine_mask'],
            'extNum' => $request->employee['ext_num'],
        ]);
    }
}
