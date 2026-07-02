<?php

/**
 * Tiny PSR-4-ish autoloader for the App\ namespace, mapped onto src/.
 * No Composer dependency, so deployment stays "copy the folder onto a
 * PHP + pdo_sqlsrv host" - matching how the legacy API was deployed.
 */
spl_autoload_register(function (string $class) {
    $prefix = 'App\\';
    if (strncmp($class, $prefix, strlen($prefix)) !== 0) {
        return;
    }

    $relative = substr($class, strlen($prefix));
    $path = __DIR__ . '/' . str_replace('\\', '/', $relative) . '.php';

    if (is_file($path)) {
        require $path;
    }
});
