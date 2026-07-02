<?php

namespace App\Controllers;

use App\Core\Database;
use App\Core\Request;
use App\Core\Response;
use PDO;

abstract class Controller
{
    protected function db(): PDO
    {
        return Database::connection();
    }

    /** Call after AuthMiddleware has run. Ends the request with 403 if the role doesn't match. */
    protected function requireRole(Request $request, array $allowedRoles): bool
    {
        if (!in_array($request->employee['role'] ?? null, $allowedRoles, true)) {
            Response::forbidden('您沒有權限執行此操作。');
            return false;
        }
        return true;
    }

    protected function paginationArgs(Request $request): array
    {
        $limit = max(1, min(200, (int) ($request->query['limit'] ?? 50)));
        $offset = max(0, (int) ($request->query['offset'] ?? 0));
        return [$limit, $offset];
    }

    /**
     * Bind every value in $params by name and execute() with no arguments.
     * PDO_ODBC doesn't reliably support mixing a manual bindValue() call
     * (needed for $textFields, see Database::bindText()) with a subsequent
     * execute($array) for the rest - it emulates named parameters by
     * position internally, and mixing the two styles silently misaligns
     * them. Always go through this helper instead of calling
     * $stmt->execute($params) directly whenever $params can contain
     * free-text values that might have non-ASCII characters.
     *
     * @param array<string,mixed> $params keyed by column name, no leading colon
     * @param string[] $textFields subset of $params keys that hold free-text
     *        (customer/employee/machine names, addresses, etc.) needing
     *        codepage-aware binding via Database::bindText().
     */
    protected function executeWithParams(\PDOStatement $stmt, array $params, array $textFields = []): void
    {
        foreach ($params as $key => $value) {
            if (in_array($key, $textFields, true)) {
                Database::bindText($stmt, ":$key", $value);
                continue;
            }
            $type = is_int($value) ? PDO::PARAM_INT : (is_null($value) ? PDO::PARAM_NULL : PDO::PARAM_STR);
            $stmt->bindValue(":$key", $value, $type);
        }
        $stmt->execute();
    }
}
