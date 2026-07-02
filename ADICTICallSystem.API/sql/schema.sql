-- ADICTICallSystem.API database schema (MSSQL / T-SQL)
--
-- Rewritten schema fixing issues found in the legacy ADICTICallSystem/API implementation:
--   * trailing-comma syntax errors in CREATE TABLE (extline, log)
--   * admin_uuid vs supervisor_uuid column-name mismatch
--   * password never persisted (only a deterministic MD5 token was stored)
--   * no indexes / no foreign keys anywhere
--   * separate "admin" table duplicating "operators" (merged into employees.role)
--
-- Run once via bin/setup.php, which creates the target database first and
-- then applies this file. Safe to re-run: every statement is guarded with
-- an existence check so re-running does not error out on a partially-built DB.

-- ============================================================
-- employees: unifies the legacy "operators" + "admin" tables.
-- role distinguishes normal operators from supervisors/admins;
-- supervisors/admins use the same machine_mask bit-field to see
-- multiple machine codes at once (matches original design intent).
-- ============================================================
IF OBJECT_ID('dbo.employees', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.employees
    (
        id              INT IDENTITY(1,1) PRIMARY KEY,
        employee_no     NVARCHAR(50)  NOT NULL,
        password_hash   NVARCHAR(255) NOT NULL,
        name            NVARCHAR(50)  NULL,
        role            NVARCHAR(20)  NOT NULL CONSTRAINT DF_employees_role DEFAULT ('operator'),
        machine_mask    INT           NOT NULL CONSTRAINT DF_employees_machine_mask DEFAULT (0),
        ext_num         INT           NULL,
        login_at        DATETIME2     NULL,
        logout_at       DATETIME2     NULL,
        is_disabled     BIT           NOT NULL CONSTRAINT DF_employees_disabled DEFAULT (0),
        created_at      DATETIME2     NOT NULL CONSTRAINT DF_employees_created DEFAULT (SYSUTCDATETIME()),
        updated_at      DATETIME2     NOT NULL CONSTRAINT DF_employees_updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT UQ_employees_employee_no UNIQUE (employee_no),
        CONSTRAINT CK_employees_role CHECK (role IN ('operator', 'supervisor', 'admin'))
    );
    CREATE INDEX IX_employees_ext_num ON dbo.employees(ext_num);
END
GO

-- ============================================================
-- sessions: short-lived bearer tokens issued at login.
-- Only a SHA-256 hash of the token is stored (defense in depth,
-- same rationale as never storing raw passwords).
-- ============================================================
IF OBJECT_ID('dbo.sessions', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.sessions
    (
        id           INT IDENTITY(1,1) PRIMARY KEY,
        employee_id  INT       NOT NULL,
        token_hash   CHAR(64)  NOT NULL,
        issued_at    DATETIME2 NOT NULL CONSTRAINT DF_sessions_issued DEFAULT (SYSUTCDATETIME()),
        expires_at   DATETIME2 NOT NULL,
        revoked_at   DATETIME2 NULL,
        CONSTRAINT UQ_sessions_token_hash UNIQUE (token_hash),
        CONSTRAINT FK_sessions_employee FOREIGN KEY (employee_id) REFERENCES dbo.employees(id)
    );
    CREATE INDEX IX_sessions_employee ON dbo.sessions(employee_id);
    CREATE INDEX IX_sessions_expires ON dbo.sessions(expires_at);
END
GO

-- ============================================================
-- customers
-- ============================================================
IF OBJECT_ID('dbo.customers', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.customers
    (
        id              INT IDENTITY(1,1) PRIMARY KEY,
        customer_no     NVARCHAR(50)  NULL,
        name            NVARCHAR(50)  NULL,
        birthday        DATE          NULL,
        gender          CHAR(1)       NULL,
        tel_no          NVARCHAR(50)  NULL,
        email           NVARCHAR(100) NULL,
        county          NVARCHAR(50)  NULL,
        township        NVARCHAR(50)  NULL,
        address         NVARCHAR(200) NULL,
        is_blacklisted  BIT           NOT NULL CONSTRAINT DF_customers_blacklisted DEFAULT (0),
        created_at      DATETIME2     NOT NULL CONSTRAINT DF_customers_created DEFAULT (SYSUTCDATETIME()),
        updated_at      DATETIME2     NOT NULL CONSTRAINT DF_customers_updated DEFAULT (SYSUTCDATETIME())
    );
    -- A plain UNIQUE constraint would only allow a single NULL customer_no
    -- across the whole table (SQL Server treats NULL as a duplicate-checkable
    -- value there); most customers are created without one, so this must be
    -- a filtered index instead.
    CREATE UNIQUE INDEX IX_customers_customer_no ON dbo.customers(customer_no) WHERE customer_no IS NOT NULL;
    CREATE INDEX IX_customers_tel_no ON dbo.customers(tel_no);
END
GO

-- ============================================================
-- machines: one row per (machine_type, machine_no) sub-machine slot.
-- machine_type: 1 = PBX, 2 = CallerID Box, 3 = Voice Card
-- machine_no:   1..10 (合併機碼)
-- ============================================================
IF OBJECT_ID('dbo.machines', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.machines
    (
        id              INT IDENTITY(1,1) PRIMARY KEY,
        machine_type    TINYINT      NOT NULL,
        machine_no      TINYINT      NOT NULL,
        alias           NVARCHAR(50) NULL,
        out_port_count  SMALLINT     NOT NULL CONSTRAINT DF_machines_out DEFAULT (0),
        ext_port_count  SMALLINT     NOT NULL CONSTRAINT DF_machines_ext DEFAULT (0),
        ip_address      VARCHAR(45)  NULL,
        sw_version      VARCHAR(20)  NULL,
        is_connected    BIT          NOT NULL CONSTRAINT DF_machines_connected DEFAULT (0),
        last_seen_at    DATETIME2    NULL,
        CONSTRAINT UQ_machines_type_no UNIQUE (machine_type, machine_no),
        CONSTRAINT CK_machines_type CHECK (machine_type IN (1, 2, 3)),
        CONSTRAINT CK_machines_no CHECK (machine_no BETWEEN 1 AND 10)
    );
END
GO

-- ============================================================
-- outline_ports / extline_ports
--
-- Second design pass, replacing the first attempt's fixed 240-row
-- "outlines"/"extlines" pool tables (dropped below if present).
-- That design pre-seeded a hard-coded 240 virtual ports regardless
-- of how many physical ports were actually configured, and modeled
-- each virtual port as one row with three nullable phy-port columns
-- (one per machine type) - baking in an assumption that at most the
-- 3 machine types share a fixed slot.
--
-- This design instead has one row per *physical* port
-- (machine_type, machine_no, phy_port), with `vport` as an ordinary
-- nullable column holding whichever virtual/unified line number that
-- physical port currently represents. Consequences:
--   - the virtual port "pool size" is simply how many physical ports
--     are actually configured - no artificial 240 ceiling, no wasted
--     pre-allocated rows.
--   - a single vport value can appear on 1-3 rows across the three
--     machine types at once - that's exactly what "virtual" is meant
--     to express: several independent pieces of hardware (a PBX FXO
--     port, a CallerID box FXO port, a voice-recording card FXO port)
--     physically wired to the *same* real phone line, each with their
--     own independent physical numbering, unified under one shared
--     number for reporting/monitoring.
--   - any machine_type/machine_no combination is allowed (including
--     all three types at the same machine_no simultaneously) - MachineController
--     no longer enforces "CallerID Box and Voice Card are mutually
--     exclusive" or "phy port count can't exceed the PBX's", per
--     clarified requirements. The only remaining cross-type rule is
--     that only machine_type=1 (PBX) may have extension ports at all,
--     enforced the same way as before.
--   - rows are created/removed by MachineController automatically
--     when a machine's out_port_count/ext_port_count changes (grow:
--     insert new phy_port rows with vport left NULL; shrink: delete
--     phy_port rows beyond the new count) - see EmployeeController's
--     sibling, MachineController::syncOutlinePorts()/syncExtlinePorts().
-- ============================================================
IF OBJECT_ID('dbo.outlines', 'U') IS NOT NULL DROP TABLE dbo.outlines;
IF OBJECT_ID('dbo.extlines', 'U') IS NOT NULL DROP TABLE dbo.extlines;
GO

IF OBJECT_ID('dbo.outline_ports', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.outline_ports
    (
        id           INT IDENTITY(1,1) PRIMARY KEY,
        machine_type TINYINT  NOT NULL,
        machine_no   TINYINT  NOT NULL,
        phy_port     SMALLINT NOT NULL, -- this device's own 1..N physical port index
        vport        INT      NULL,     -- shared virtual line number; NULL = not yet assigned
        in_use       BIT      NOT NULL CONSTRAINT DF_outline_ports_inuse DEFAULT (0),
        call_status  TINYINT  NOT NULL CONSTRAINT DF_outline_ports_status DEFAULT (0), -- 0 idle,1 callin,2 callout,3 talking,4 hangup
        created_at   DATETIME2 NOT NULL CONSTRAINT DF_outline_ports_created DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT UQ_outline_ports_phy UNIQUE (machine_type, machine_no, phy_port),
        CONSTRAINT FK_outline_ports_machine FOREIGN KEY (machine_type, machine_no) REFERENCES dbo.machines(machine_type, machine_no)
    );
    CREATE INDEX IX_outline_ports_vport ON dbo.outline_ports(vport);
    CREATE INDEX IX_outline_ports_machine ON dbo.outline_ports(machine_type, machine_no);
END
GO

IF OBJECT_ID('dbo.extline_ports', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.extline_ports
    (
        id                   INT      IDENTITY(1,1) PRIMARY KEY,
        machine_type         TINYINT  NOT NULL,
        machine_no           TINYINT  NOT NULL,
        phy_port             SMALLINT NOT NULL,
        vport                INT      NULL,
        ext_num              INT      NULL,
        status               TINYINT  NOT NULL CONSTRAINT DF_extline_ports_status DEFAULT (0),
        current_employee_id  INT      NULL,
        created_at           DATETIME2 NOT NULL CONSTRAINT DF_extline_ports_created DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT UQ_extline_ports_phy UNIQUE (machine_type, machine_no, phy_port),
        CONSTRAINT FK_extline_ports_machine FOREIGN KEY (machine_type, machine_no) REFERENCES dbo.machines(machine_type, machine_no),
        CONSTRAINT FK_extline_ports_employee FOREIGN KEY (current_employee_id) REFERENCES dbo.employees(id)
    );
    CREATE UNIQUE INDEX IX_extline_ports_ext_num ON dbo.extline_ports(ext_num) WHERE ext_num IS NOT NULL;
    CREATE INDEX IX_extline_ports_vport ON dbo.extline_ports(vport);
    CREATE INDEX IX_extline_ports_machine ON dbo.extline_ports(machine_type, machine_no);
END
GO

-- ============================================================
-- employee_ext_lines: which virtual internal lines (extline_ports.vport)
-- an employee's seat is responsible for, shown on the OperatorWeb
-- "虛擬內線" board. Deliberately a separate table from
-- employees.machine_mask - that field is still read natively by
-- ADICTICallCenter's WSServer.cpp to decide which connected operator's
-- WebSocket session receives a given machine's call events, so
-- repurposing it for this would break live call delivery. A vport can
-- also exceed 10 (extline_ports has no fixed ceiling - see the block
-- above), which a 32-bit bitmask couldn't represent anyway.
-- ============================================================
IF OBJECT_ID('dbo.employee_ext_lines', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.employee_ext_lines
    (
        id           INT IDENTITY(1,1) PRIMARY KEY,
        employee_id  INT NOT NULL,
        ext_vport    INT NOT NULL,
        created_at   DATETIME2 NOT NULL CONSTRAINT DF_employee_ext_lines_created DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT UQ_employee_ext_lines UNIQUE (employee_id, ext_vport),
        CONSTRAINT FK_employee_ext_lines_employee FOREIGN KEY (employee_id) REFERENCES dbo.employees(id)
    );
    CREATE INDEX IX_employee_ext_lines_vport ON dbo.employee_ext_lines(ext_vport);
END
GO

-- ============================================================
-- call_records
-- Timestamps are stored as DATETIME2 (not raw 32-bit unix time)
-- to avoid compounding the wire-protocol's 2038 rollover problem
-- inside the database layer; conversion happens at the API edge.
-- ============================================================
IF OBJECT_ID('dbo.call_records', 'U') IS NULL
BEGIN
    CREATE TABLE dbo.call_records
    (
        id                BIGINT IDENTITY(1,1) PRIMARY KEY,
        machine_no        TINYINT       NOT NULL,
        call_type         TINYINT       NOT NULL, -- 0 in, 1 out, 2 internal
        out_vport         SMALLINT      NULL,
        out_phy_port      SMALLINT      NULL,
        tel_no            NVARCHAR(50)  NULL,
        ext_num           INT           NULL,      -- answering extension for external calls
        from_ext_num      INT           NULL,      -- internal call origin extension
        to_ext_num        INT           NULL,      -- internal call destination extension
        ring_times        SMALLINT      NULL,
        start_time        DATETIME2     NULL,
        connect_time      DATETIME2     NULL,
        end_time          DATETIME2     NULL,
        duration_sec      INT           NULL,
        second_dtmf       NVARCHAR(100) NULL,
        voice_record_uuid CHAR(32)      NULL,
        customer_id       INT           NULL,
        employee_id       INT           NULL,
        created_at        DATETIME2     NOT NULL CONSTRAINT DF_call_records_created DEFAULT (SYSUTCDATETIME()),
        updated_at        DATETIME2     NOT NULL CONSTRAINT DF_call_records_updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_call_records_type CHECK (call_type IN (0, 1, 2)),
        CONSTRAINT FK_call_records_customer FOREIGN KEY (customer_id) REFERENCES dbo.customers(id),
        CONSTRAINT FK_call_records_employee FOREIGN KEY (employee_id) REFERENCES dbo.employees(id)
    );
    CREATE INDEX IX_call_records_start_time ON dbo.call_records(start_time);
    CREATE INDEX IX_call_records_machine_no ON dbo.call_records(machine_no);
    CREATE INDEX IX_call_records_tel_no ON dbo.call_records(tel_no);
    CREATE INDEX IX_call_records_employee ON dbo.call_records(employee_id);
END
GO
