<?php
/**
 * Central configuration. Values are read from environment variables first
 * (set them in your web server / PHP-FPM pool config for real deployments)
 * and fall back to sane local-development defaults so `php -S` / a local
 * IIS site works out of the box against a local MSSQL instance.
 *
 * To override locally without editing this file (and without risking the
 * override being committed), copy config.local.php.example to
 * config.local.php - it is merged in automatically if present.
 */

function env(string $key, $default = null)
{
    $value = getenv($key);
    return $value === false ? $default : $value;
}

$config = [
    'app' => [
        // Never enable debug in a real deployment: it puts raw exception
        // messages (including SQL) into API responses.
        'debug' => filter_var(env('APP_DEBUG', 'false'), FILTER_VALIDATE_BOOLEAN),
    ],

    'db' => [
        // 'sqlsrv' (pdo_sqlsrv extension) or 'odbc' (pdo_odbc over the
        // Windows "SQL Server" ODBC driver - use this where pdo_sqlsrv
        // isn't installed but pdo_odbc/odbc are, e.g. this box's PHP 7.4 build).
        'driver'           => env('DB_DRIVER', 'sqlsrv'),
        'odbc_driver_name' => env('DB_ODBC_DRIVER_NAME', 'SQL Server'),
        // PDO_ODBC binds strings as SQL_C_CHAR using the OS's ANSI codepage,
        // not UTF-8 - non-ASCII text (e.g. Chinese) has to be transcoded to
        // this codepage before binding or the driver miscounts characters
        // and rejects the value. Only used when driver=odbc. Default matches
        // Traditional Chinese Windows (Big5); change if deployed on a
        // different-locale Windows box. See Database::bindText().
        'odbc_ansi_codepage' => env('DB_ODBC_ANSI_CODEPAGE', 'CP950'),
        'host'             => env('DB_HOST', 'localhost'),
        'database'         => env('DB_NAME', 'ADICTICallSystem'),
        'user'             => env('DB_USER', 'sa'),
        'password'         => env('DB_PASSWORD', ''),
    ],

    'auth' => [
        // Session token lifetime in seconds (default 8 hours).
        'session_ttl_seconds' => (int) env('SESSION_TTL_SECONDS', 8 * 3600),
    ],

    'cors' => [
        // Explicit allow-list instead of "*". Add your front-end origin(s).
        // Example: 'http://localhost:8080,http://192.168.1.50'
        'allowed_origins' => array_filter(array_map('trim', explode(',', env('CORS_ALLOWED_ORIGINS', 'http://localhost')))),
    ],
];

$localOverride = __DIR__ . '/config.local.php';
if (is_file($localOverride)) {
    $override = require $localOverride;
    if (is_array($override)) {
        $config = array_replace_recursive($config, $override);
    }
}

return $config;
