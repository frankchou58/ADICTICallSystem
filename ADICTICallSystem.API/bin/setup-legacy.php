<?php

/**
 * One-time (re-runnable) deployment script for the customer's pre-existing
 * "ADICTICallCenter" database - a different bootstrap path from
 * bin/setup.php, which creates a brand-new database from sql/schema.sql.
 *
 *   php bin/setup-legacy.php [--admin-no=admin] [--admin-password=ChangeMe123!]
 *
 * This script does NOT create a database and does NOT create any tables -
 * the customer's database already has dbo.admin/operators/machine/outline/
 * extline/record and cannot have new tables added. It only:
 *   1. Runs sql/migrate-legacy-adicticallcenter.sql (idempotent ALTER TABLE
 *      statements that add the columns this API needs onto dbo.operators
 *      and dbo.machine - see that file's header for the full rationale).
 *   2. Ensures an initial admin account exists in dbo.operators.
 * See doc/ADICTICallSystem.API-說明書.md for the full legacy-schema mapping.
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

echo "==> Connecting to [{$db['database']}] on {$db['host']} ...\n";
$pdo = new PDO(
    Database::buildDsn($db),
    $db['user'],
    $db['password'],
    [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
);

echo "==> Applying sql/migrate-legacy-adicticallcenter.sql (additive columns only) ...\n";
runBatches($pdo, __DIR__ . '/../sql/migrate-legacy-adicticallcenter.sql');

echo "==> Ensuring an initial admin account exists in dbo.operators ...\n";
$stmt = $pdo->prepare('SELECT COUNT(*) FROM dbo.operators WHERE employee_no = :employee_no');
$stmt->execute(['employee_no' => $adminNo]);

if ((int) $stmt->fetchColumn() > 0) {
    echo "    Operator '$adminNo' already exists - leaving it untouched.\n";
} else {
    if ($adminPassword === null) {
        $adminPassword = bin2hex(random_bytes(9)); // 18-char random default
        echo "    No --admin-password supplied - generated one (shown once below).\n";
    }
    $insert = $pdo->prepare(
        'INSERT INTO dbo.operators (employee_no, password_hash, name, role) VALUES (:employee_no, :password_hash, :name, :role)'
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
