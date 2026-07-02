<?php

namespace App\Core;

use PDO;

/**
 * Password hashing + bearer-session-token issuance.
 *
 * Replaces the legacy scheme where a permanent token was derived as
 * md5("$employeeId:$password@adicti.com.tw") and reused forever as both
 * the password check *and* the API bearer key (sent as a URL query
 * parameter on every call, so it ended up in web server / proxy logs with
 * no way to revoke it short of changing the password).
 *
 * Here: passwords are hashed with bcrypt (password_hash), and login issues
 * a random, short-lived session token. Only a SHA-256 hash of the token is
 * persisted, so a DB leak alone does not yield usable bearer tokens.
 */
class Auth
{
    public static function hashPassword(string $password): string
    {
        return password_hash($password, PASSWORD_BCRYPT);
    }

    public static function verifyPassword(string $password, string $hash): bool
    {
        return password_verify($password, $hash);
    }

    /** Returns the raw token to hand to the client; only its hash is stored. */
    public static function issueSession(PDO $db, int $employeeId): array
    {
        $ttl = Config::all()['auth']['session_ttl_seconds'];

        $token = bin2hex(random_bytes(32));
        $tokenHash = hash('sha256', $token);

        $stmt = $db->prepare(
            'INSERT INTO dbo.sessions (employee_id, token_hash, expires_at)
             VALUES (:employee_id, :token_hash, DATEADD(SECOND, :ttl, SYSUTCDATETIME()))'
        );
        $stmt->execute([
            'employee_id' => $employeeId,
            'token_hash' => $tokenHash,
            'ttl' => $ttl,
        ]);

        return ['token' => $token, 'expiresInSeconds' => $ttl];
    }

    /** Returns the authenticated employee row, or null if the token is missing/expired/revoked. */
    public static function resolveSession(PDO $db, ?string $token): ?array
    {
        if (!$token) {
            return null;
        }

        $tokenHash = hash('sha256', $token);
        $stmt = $db->prepare(
            'SELECT e.id, e.employee_no, e.name, e.role, e.machine_mask, e.ext_num
             FROM dbo.sessions s
             JOIN dbo.employees e ON e.id = s.employee_id
             WHERE s.token_hash = :token_hash
               AND s.revoked_at IS NULL
               AND s.expires_at > SYSUTCDATETIME()
               AND e.is_disabled = 0'
        );
        $stmt->execute(['token_hash' => $tokenHash]);
        $row = $stmt->fetch();

        return $row ?: null;
    }

    public static function revokeSession(PDO $db, string $token): void
    {
        $tokenHash = hash('sha256', $token);
        $stmt = $db->prepare('UPDATE dbo.sessions SET revoked_at = SYSUTCDATETIME() WHERE token_hash = :token_hash');
        $stmt->execute(['token_hash' => $tokenHash]);
    }
}
