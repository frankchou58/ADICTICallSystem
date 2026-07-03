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
 * 2026-07-03: this customer's ADICTICallCenter database is a pre-existing
 * one and can't have new tables added (only new columns on existing
 * tables), so there's no dedicated dbo.sessions table here - the current
 * session's token hash + expiry live directly on dbo.operators
 * (session_token_hash/session_expires_at), one active session per operator
 * (a fresh login overwrites the previous token, matching the legacy
 * "operator_uuid is the one credential" model but now with expiry).
 * Passwords are still hashed with bcrypt (password_hash), and login issues
 * a random, short-lived session token - only a SHA-256 hash of the token is
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
    public static function issueSession(PDO $db, int $operatorId): array
    {
        $ttl = Config::all()['auth']['session_ttl_seconds'];

        $token = bin2hex(random_bytes(32));
        $tokenHash = hash('sha256', $token);

        $stmt = $db->prepare(
            'UPDATE dbo.operators
             SET session_token_hash = :token_hash, session_expires_at = DATEADD(SECOND, :ttl, SYSUTCDATETIME())
             WHERE ID = :id'
        );
        $stmt->execute([
            'token_hash' => $tokenHash,
            'ttl' => $ttl,
            'id' => $operatorId,
        ]);

        return ['token' => $token, 'expiresInSeconds' => $ttl];
    }

    /** Returns the authenticated operator row, or null if the token is missing/expired/revoked. */
    public static function resolveSession(PDO $db, ?string $token): ?array
    {
        if (!$token) {
            return null;
        }

        $tokenHash = hash('sha256', $token);
        $stmt = $db->prepare(
            'SELECT ID AS id, employee_no, name, role, machine_id, ext_num
             FROM dbo.operators
             WHERE session_token_hash = :token_hash
               AND session_expires_at > SYSUTCDATETIME()
               AND is_disabled = 0'
        );
        $stmt->execute(['token_hash' => $tokenHash]);
        $row = $stmt->fetch();

        return $row ?: null;
    }

    public static function revokeSession(PDO $db, string $token): void
    {
        $tokenHash = hash('sha256', $token);
        $stmt = $db->prepare(
            'UPDATE dbo.operators SET session_token_hash = NULL, session_expires_at = NULL WHERE session_token_hash = :token_hash'
        );
        $stmt->execute(['token_hash' => $tokenHash]);
    }
}
