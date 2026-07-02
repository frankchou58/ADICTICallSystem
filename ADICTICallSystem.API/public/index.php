<?php

require __DIR__ . '/../src/autoload.php';

use App\Core\Config;
use App\Core\Request;
use App\Core\Response;
use App\Core\Router;
use App\Core\ValidationException;

$config = Config::all();

// ---- CORS: explicit allow-list, never "*" -----------------------------
// The legacy API sent `Access-Control-Allow-Origin: *` on every response,
// which combined with GET-triggerable state changes and no CSRF token made
// every endpoint reachable from any third-party page in a victim's browser.
$origin = $_SERVER['HTTP_ORIGIN'] ?? '';
$originAllowed = $origin !== '' && in_array($origin, $config['cors']['allowed_origins'], true);
if (!$originAllowed && $origin !== '' && !empty($config['cors']['allowed_origin_pattern'])) {
    $originAllowed = (bool) preg_match($config['cors']['allowed_origin_pattern'], $origin);
}
if ($originAllowed) {
    header("Access-Control-Allow-Origin: $origin");
    header('Access-Control-Allow-Headers: Content-Type, Authorization');
    header('Access-Control-Allow-Methods: GET, POST, PATCH, DELETE, OPTIONS');
    header('Vary: Origin');
}

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(204);
    exit;
}

$router = new Router();
require __DIR__ . '/../routes.php';

try {
    $request = Request::capture();

    // Prefer a real path (REQUEST_URI, when a rewrite rule maps clean URLs
    // to this file - see public/.htaccess / public/web.config) or PATH_INFO
    // (when the PHP handler mapping has "invoke handler only if request is
    // mapped to... AND path info" enabled). Fall back to an explicit ?route=
    // query parameter, which needs no IIS/Apache module at all: this box
    // has neither pdo_sqlsrv nor the IIS URL Rewrite module installed, so
    // clean paths like /employees/5 never reach PHP - callers must use
    // index.php?route=/employees/5 instead.
    $routePath = $_GET['route'] ?? $_SERVER['PATH_INFO'] ?? ($_SERVER['REQUEST_URI'] ?? '/');

    $router->dispatch($_SERVER['REQUEST_METHOD'], $routePath, $request);
} catch (ValidationException $e) {
    Response::error($e->getMessage(), 422);
} catch (Throwable $e) {
    // Never leak raw exception/SQL text to the client - the legacy API's
    // habit of echoing odbc_error() back to callers doubled as a SQL
    // injection reconnaissance tool.
    error_log('[ADICTICallSystem.API] ' . $e->getMessage() . "\n" . $e->getTraceAsString());
    $detail = $config['app']['debug'] ? $e->getMessage() : null;
    Response::error('伺服器內部錯誤。', 500, $detail);
}
