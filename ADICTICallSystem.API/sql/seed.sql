-- ADICTICallSystem.API seed data
-- Populates the 30 machine slots (3 machine types x 10 machine codes).
-- outline_ports / extline_ports are NOT seeded here - they're created
-- on demand by MachineController when a machine's out_port_count /
-- ext_port_count is set to something greater than zero (see
-- MachineController::syncOutlinePorts()/syncExtlinePorts()), since the
-- whole point of the redesign is that the port pool size tracks actual
-- configuration instead of a fixed pre-allocated pool.
-- This is idempotent: every insert is guarded so re-running is a no-op.

SET NOCOUNT ON;

;WITH Types AS (SELECT 1 AS t UNION ALL SELECT 2 UNION ALL SELECT 3),
Codes AS
(
    SELECT 1 AS n
    UNION ALL
    SELECT n + 1 FROM Codes WHERE n < 10
)
INSERT INTO dbo.machines (machine_type, machine_no)
SELECT Types.t, Codes.n
FROM Types CROSS JOIN Codes
WHERE NOT EXISTS (
    SELECT 1 FROM dbo.machines m WHERE m.machine_type = Types.t AND m.machine_no = Codes.n
)
OPTION (MAXRECURSION 10);
GO
