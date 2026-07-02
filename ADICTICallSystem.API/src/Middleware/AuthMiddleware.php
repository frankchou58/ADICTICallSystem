<?php

namespace App\Middleware;

use App\Core\Auth;
use App\Core\Database;
use App\Core\Request;
use App\Core\Response;

/**
 * Resolves the bearer token into the authenticated employee and attaches it
 * to the request. Any route array-wrapping this middleware requires a
 * valid, non-expired, non-revoked session.
 */
class AuthMiddleware
{
    public function handle(Request $request): bool
    {
        $token = $request->bearerToken();
        $employee = Auth::resolveSession(Database::connection(), $token);

        if ($employee === null) {
            Response::unauthorized('登入憑證遺失或已過期，請重新登入。');
            return false;
        }

        // Decode here, once, so every controller reading $request->employee['name']
        // gets UTF-8 without needing to remember Database::decodeText() itself.
        $employee['name'] = Database::decodeText($employee['name']);
        $request->employee = $employee;
        return true;
    }
}
