<?php

namespace App\Core;

use PDO;
use PDOException;

/**
 * PDO singleton. Every caller must use prepared statements - the legacy
 * implementation built raw SQL strings by concatenating request input,
 * which is the root cause of its SQL-injection exposure.
 *
 * Supports two drivers via config('db.driver'):
 *   - 'sqlsrv' : Microsoft's pdo_sqlsrv extension (preferred where available)
 *   - 'odbc'   : PDO_ODBC over the Windows "SQL Server" ODBC driver, i.e.
 *                the same driver the legacy tools.php used via odbc_connect().
 *                Use this where pdo_sqlsrv isn't installed but pdo_odbc is.
 */
class Database
{
    private static ?PDO $instance = null;

    public static function connection(): PDO
    {
        if (self::$instance !== null) {
            return self::$instance;
        }

        $db = Config::all()['db'];

        $dsn = self::buildDsn($db);

        try {
            self::$instance = new PDO($dsn, $db['user'], $db['password'], [
                PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
                PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            ]);
        } catch (PDOException $e) {
            // Wrap so callers never see (and never leak to clients) the raw
            // PDO exception, which can include the DSN/credentials context.
            throw new \RuntimeException('Database connection failed.', 0, $e);
        }

        return self::$instance;
    }

    /**
     * @param array $db config('db') array
     * @param string|null $databaseOverride pass '' (empty string) to build a
     *        server-level DSN with no Database= clause, e.g. to CREATE DATABASE
     *        from bin/setup.php. Pass null (default) to use $db['database'].
     */
    public static function buildDsn(array $db, ?string $databaseOverride = null): string
    {
        $driver = $db['driver'] ?? 'sqlsrv';
        $database = $databaseOverride ?? $db['database'];

        if ($driver === 'odbc') {
            if (!in_array('odbc', PDO::getAvailableDrivers(), true)) {
                throw new \RuntimeException(
                    'The pdo_odbc PHP extension is not enabled. Add extension=pdo_odbc to php.ini and restart PHP/IIS.'
                );
            }
            $odbcDriverName = $db['odbc_driver_name'] ?? 'SQL Server';
            $dsn = sprintf('odbc:Driver={%s};Server=%s', $odbcDriverName, $db['host']);
            if ($database !== '') {
                $dsn .= ";Database=$database";
            }
            // The modern "ODBC Driver 17/18 for SQL Server" drivers default to
            // requiring an encrypted connection with a trusted certificate,
            // which a local dev instance's self-signed cert fails. The legacy
            // built-in "SQL Server" driver doesn't understand these keywords,
            // so only add them for the modern driver names.
            if (stripos($odbcDriverName, 'ODBC Driver') !== false) {
                $dsn .= ';Encrypt=no;TrustServerCertificate=yes';
            }
            return $dsn;
        }

        if (!in_array('sqlsrv', PDO::getAvailableDrivers(), true)) {
            throw new \RuntimeException(
                'The pdo_sqlsrv PHP extension is not enabled. Install the Microsoft Drivers for PHP ' .
                'for SQL Server and enable extension=pdo_sqlsrv in php.ini, or set db.driver=odbc in ' .
                'config to use PDO_ODBC instead.'
            );
        }

        $dsn = sprintf('sqlsrv:Server=%s;TrustServerCertificate=1', $db['host']);
        if ($database !== '') {
            $dsn .= ";Database=$database";
        }
        return $dsn;
    }

    /**
     * PDO_ODBC binds strings as SQL_C_CHAR using the OS's ANSI codepage
     * (Big5/CP950 on this Traditional Chinese Windows box), not UTF-8.
     * Handing it PHP's native UTF-8 bytes directly makes the ODBC driver
     * miscount characters during its codepage->UTF-16 conversion and
     * reject the value ("String data, length mismatch" / "right
     * truncated" - both were tried and failed here, including the
     * PARAM_LOB + pre-encoded UTF-16LE approach, which this driver also
     * rejects for a plain scalar NVARCHAR update). The fix that actually
     * works: transcode to the codepage the driver already expects, and
     * let its normal (codepage-aware) conversion into NVARCHAR do the
     * rest. Only kicks in for the 'odbc' driver and only when the value
     * has non-ASCII bytes - plain ASCII text and the 'sqlsrv' driver
     * already work fine as a normal bindValue().
     */
    public static function bindText(\PDOStatement $stmt, string $param, ?string $value): void
    {
        if ($value === null) {
            $stmt->bindValue($param, null, PDO::PARAM_NULL);
            return;
        }

        $db = Config::all()['db'];
        if (($db['driver'] ?? 'sqlsrv') === 'odbc' && preg_match('/[\x80-\xFF]/', $value) === 1) {
            $codepage = $db['odbc_ansi_codepage'] ?? 'CP950';
            $converted = @iconv('UTF-8', $codepage . '//TRANSLIT', $value);
            if ($converted !== false) {
                $stmt->bindValue($param, $converted, PDO::PARAM_STR);
                return;
            }
        }

        $stmt->bindValue($param, $value, PDO::PARAM_STR);
    }

    /**
     * Symmetric counterpart to bindText() for reads: PDO_ODBC's fetch also
     * hands back NVARCHAR text through the same ANSI-codepage path (not
     * UTF-8), so any row written with real Unicode content comes back as
     * raw codepage bytes rather than valid UTF-8 - passing that straight
     * into json_encode() fails silently (empty response body, no error).
     * Only transcodes when the value ISN'T already valid UTF-8, so plain
     * ASCII (and the 'sqlsrv' driver, which never needed this) pass
     * through untouched.
     */
    public static function decodeText(?string $value): ?string
    {
        if ($value === null || $value === '') {
            return $value;
        }

        $db = Config::all()['db'];
        // mbstring isn't enabled on this box's PHP build, so validate UTF-8
        // via PCRE's /u modifier instead (it fails the match on invalid
        // UTF-8 byte sequences) rather than mb_check_encoding().
        if (($db['driver'] ?? 'sqlsrv') !== 'odbc' || preg_match('//u', $value) === 1) {
            return $value;
        }

        $codepage = $db['odbc_ansi_codepage'] ?? 'CP950';
        $converted = @iconv($codepage, 'UTF-8//TRANSLIT', $value);
        return $converted !== false ? $converted : $value;
    }
}
