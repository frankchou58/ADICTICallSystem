<?php

/**
 * One-time (re-runnable) local deployment script.
 *
 *   php bin/setup.php [--admin-no=admin] [--admin-password=ChangeMe123!]
 *
 * Replaces the legacy design's CreateOutlineTable.php / CreateExtlineTable.php /
 * CreateMachineTable.php / CreateRecordTable.php / CreateAdminTable.php /
 * CreateOperatorTable.php - which were unauthenticated HTTP endpoints that
 * anyone could hit to (re)create/wipe the schema. Bootstrapping is a
 * one-time deployment action, not a runtime API, so it lives here as a CLI
 * script instead.
 */

require __DIR__ . '/../src/autoload.php';

use App\Core\Auth;
use App\Core\Config;
use App\Core\Database;

if (PHP_SAPI !== 'cli') {
    http_response_code(403);
    die('This script may only be run from the command line.');
}

$options = getopt('', ['admin-no::', 'admin-password::']);
$adminNo = $options['admin-no'] ?? 'admin';
$adminPassword = $options['admin-password'] ?? null;

$config = Config::all();
$db = $config['db'];
$requiredExtension = ($db['driver'] ?? 'sqlsrv') === 'odbc' ? 'odbc' : 'sqlsrv';

if (!in_array($requiredExtension, PDO::getAvailableDrivers(), true)) {
    fwrite(STDERR, "ERROR: the pdo_$requiredExtension PHP extension is not enabled.\n");
    fwrite(STDERR, "Add extension=pdo_$requiredExtension to php.ini and restart PHP/IIS,\n");
    fwrite(STDERR, "or adjust db.driver in config.\n");
    exit(1);
}

function runBatches(PDO $pdo, string $sqlFile): void
{
    $sql = file_get_contents($sqlFile);
    // SQL Server's `GO` batch separator is an SSMS/sqlcmd convention, not
    // real T-SQL, so it must be stripped out and each batch run separately.
    $batches = preg_split('/^\s*GO\s*$/mi', $sql);

    foreach ($batches as $batch) {
        $batch = trim($batch);
        if ($batch === '') {
            continue;
        }
        $pdo->exec($batch);
    }
}

echo "==> Connecting to SQL Server at {$db['host']} (server-level, no database) ...\n";
$serverPdo = new PDO(
    Database::buildDsn($db, ''),
    $db['user'],
    $db['password'],
    [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
);

// Avoid PDO::quote() here: PDO_ODBC's implementation of it is unreliable on
// some driver/PHP combinations (it can silently mis-escape or return an
// unusable value), which previously caused this whole IF/CREATE DATABASE
// statement to become invalid SQL that failed *silently* - no exception,
// but no database either. Bracket/quote-escaping a config-supplied name by
// hand is safe here since it never comes from an HTTP request.
$dbNameBracketEscaped = str_replace(']', ']]', $db['database']);
$dbNameLiteralEscaped = str_replace("'", "''", $db['database']);
echo "==> Ensuring database [{$db['database']}] exists ...\n";
$serverPdo->exec("IF DB_ID('$dbNameLiteralEscaped') IS NULL CREATE DATABASE [$dbNameBracketEscaped]");

$check = $serverPdo->prepare('SELECT 1 FROM sys.databases WHERE name = :name');
$check->execute(['name' => $db['database']]);
if (!$check->fetchColumn()) {
    fwrite(STDERR, "ERROR: database [{$db['database']}] still does not exist after CREATE DATABASE.\n");
    fwrite(STDERR, "Check that '{$db['user']}' has the dbcreator server role.\n");
    exit(1);
}

echo "==> Connecting to [{$db['database']}] ...\n";
$pdo = new PDO(
    Database::buildDsn($db),
    $db['user'],
    $db['password'],
    [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
);

echo "==> Applying schema.sql ...\n";
runBatches($pdo, __DIR__ . '/../sql/schema.sql');

echo "==> Applying seed.sql (240 outlines, 240 extlines, 30 machine slots) ...\n";
runBatches($pdo, __DIR__ . '/../sql/seed.sql');

echo "==> Ensuring an initial admin account exists ...\n";
$stmt = $pdo->prepare('SELECT COUNT(*) FROM dbo.employees WHERE employee_no = :employee_no');
$stmt->execute(['employee_no' => $adminNo]);

if ((int) $stmt->fetchColumn() > 0) {
    echo "    Employee '$adminNo' already exists - leaving it untouched.\n";
} else {
    if ($adminPassword === null) {
        $adminPassword = bin2hex(random_bytes(9)); // 18-char random default
        echo "    No --admin-password supplied - generated one (shown once below).\n";
    }
    $insert = $pdo->prepare(
        'INSERT INTO dbo.employees (employee_no, password_hash, name, role) VALUES (:employee_no, :password_hash, :name, :role)'
    );
    $insert->execute([
        'employee_no' => $adminNo,
        'password_hash' => Auth::hashPassword($adminPassword),
        'name' => 'System Administrator',
        'role' => 'admin',
    ]);
    echo "    Created admin employeeNo='$adminNo' password='$adminPassword'\n";
    echo "    STORE THIS PASSWORD NOW - it is not recoverable, only resettable.\n";
}

echo "==> Done.\n";
