-- Additive migration for the customer's pre-existing "ADICTICallCenter" database.
--
-- Background: this database already has 6 tables from the old PHP tools.php-era
-- system (admin, operators, machine, outline, extline, record) and the customer
-- cannot have new tables created in it. This script only ADDs columns to the two
-- tables that need extra fields to support this API's auth/connection-status
-- features - it never creates a new table and never touches outline/extline/
-- record/admin. See doc/ADICTICallSystem.API-說明書.md for the full mapping
-- between these legacy tables and this API's routes.
--
-- Safe to re-run: every ALTER is guarded with a COL_LENGTH existence check.

-- ============================================================
-- operators: this becomes the sole employee/auth table. 'admin' (the old
-- singleton table) is left untouched and unused - admin-level access is now
-- just an operators row with role='admin'.
-- ============================================================
IF COL_LENGTH('dbo.operators', 'employee_no') IS NULL
BEGIN
    ALTER TABLE dbo.operators ADD employee_no NVARCHAR(50) NULL;
END
GO

SET QUOTED_IDENTIFIER ON;
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = 'UQ_operators_employee_no')
BEGIN
    -- Filtered unique index: allows existing rows with NULL employee_no
    -- (legacy rows created before this migration) while still enforcing
    -- uniqueness for every row that does have one.
    CREATE UNIQUE INDEX UQ_operators_employee_no ON dbo.operators(employee_no) WHERE employee_no IS NOT NULL;
END
GO

IF COL_LENGTH('dbo.operators', 'password_hash') IS NULL
BEGIN
    ALTER TABLE dbo.operators ADD password_hash NVARCHAR(255) NULL;
END
GO

IF COL_LENGTH('dbo.operators', 'role') IS NULL
BEGIN
    ALTER TABLE dbo.operators ADD role NVARCHAR(20) NOT NULL CONSTRAINT DF_operators_role DEFAULT ('operator');
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = 'CK_operators_role')
BEGIN
    ALTER TABLE dbo.operators ADD CONSTRAINT CK_operators_role CHECK (role IN ('operator', 'supervisor', 'admin'));
END
GO

IF COL_LENGTH('dbo.operators', 'session_token_hash') IS NULL
BEGIN
    ALTER TABLE dbo.operators ADD session_token_hash NVARCHAR(64) NULL;
END
GO

IF COL_LENGTH('dbo.operators', 'session_expires_at') IS NULL
BEGIN
    ALTER TABLE dbo.operators ADD session_expires_at DATETIME2 NULL;
END
GO

IF COL_LENGTH('dbo.operators', 'is_disabled') IS NULL
BEGIN
    ALTER TABLE dbo.operators ADD is_disabled BIT NOT NULL CONSTRAINT DF_operators_disabled DEFAULT (0);
END
GO

-- ============================================================
-- machine: connection-status reporting. ADICTICallCenter.exe (MFC) already
-- calls PATCH /machines/{type}/{no} with isConnected/ipAddress when a real
-- sub-machine connects/disconnects (see CDatabaseAccessURL.cpp) - these
-- columns give that call somewhere to land instead of silently no-op'ing.
-- ============================================================
IF COL_LENGTH('dbo.machine', 'is_connected') IS NULL
BEGIN
    ALTER TABLE dbo.machine ADD is_connected BIT NOT NULL CONSTRAINT DF_machine_connected DEFAULT (0);
END
GO

IF COL_LENGTH('dbo.machine', 'ip_address') IS NULL
BEGIN
    ALTER TABLE dbo.machine ADD ip_address NVARCHAR(45) NULL;
END
GO

IF COL_LENGTH('dbo.machine', 'sw_version') IS NULL
BEGIN
    ALTER TABLE dbo.machine ADD sw_version NVARCHAR(50) NULL;
END
GO

IF COL_LENGTH('dbo.machine', 'last_seen_at') IS NULL
BEGIN
    ALTER TABLE dbo.machine ADD last_seen_at DATETIME2 NULL;
END
GO
