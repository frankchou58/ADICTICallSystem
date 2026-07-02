<?php

namespace App\Core;

/**
 * Memoized config loader. config/config.php is `require`d exactly once per
 * request no matter how many places call Config::all() - plain `require`
 * would re-run (and re-declare env()) on every call site, while
 * `require_once` returns `true` (not the original array) on the 2nd+ call,
 * which silently corrupted every caller after the first. Both were tried
 * and broke in different ways before landing here.
 */
class Config
{
    private static ?array $config = null;

    public static function all(): array
    {
        if (self::$config === null) {
            self::$config = require __DIR__ . '/../../config/config.php';
        }
        return self::$config;
    }
}
