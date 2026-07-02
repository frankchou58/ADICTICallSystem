<?php

namespace App\Core;

/**
 * Uniform JSON response envelope for every endpoint:
 *   { "success": bool, "data": ..., "message": string|null, "meta": {...} }
 *
 * Replaces the legacy per-file ad-hoc shape (status/esptime/fields/data)
 * that differed subtly across the ~60 old endpoints.
 */
class Response
{
    public static function json(bool $success, $data = null, ?string $message = null, int $httpStatus = 200, array $meta = []): void
    {
        http_response_code($httpStatus);
        header('Content-Type: application/json; charset=UTF-8');

        echo json_encode([
            'success' => $success,
            'data' => $data,
            'message' => $message,
            'meta' => $meta,
        ], JSON_UNESCAPED_UNICODE | JSON_UNESCAPED_SLASHES);
    }

    public static function ok($data = null, ?string $message = null, array $meta = []): void
    {
        self::json(true, $data, $message, 200, $meta);
    }

    public static function created($data = null, ?string $message = null): void
    {
        self::json(true, $data, $message, 201);
    }

    public static function error(string $message, int $httpStatus = 400, $data = null): void
    {
        self::json(false, $data, $message, $httpStatus);
    }

    public static function notFound(string $message = '找不到資源'): void
    {
        self::error($message, 404);
    }

    public static function unauthorized(string $message = '未授權'): void
    {
        self::error($message, 401);
    }

    public static function forbidden(string $message = '沒有權限'): void
    {
        self::error($message, 403);
    }
}
