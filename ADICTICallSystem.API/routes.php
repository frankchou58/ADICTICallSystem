<?php

use App\Controllers\AuthController;
use App\Controllers\CallRecordController;
use App\Controllers\EmployeeController;
use App\Controllers\ExtlinePortController;
use App\Controllers\MachineController;
use App\Controllers\OutlinePortController;
use App\Core\Router;
use App\Middleware\AuthMiddleware;

/** @var Router $router */

// ---- Auth ---------------------------------------------------------------
$router->post('/auth/login', [AuthController::class, 'login']);
$router->get('/auth/employee-numbers', [AuthController::class, 'employeeNumbers']);
$router->post('/auth/logout', [AuthController::class, 'logout'], [AuthMiddleware::class]);
$router->get('/auth/me', [AuthController::class, 'me'], [AuthMiddleware::class]);

// ---- Employees (operators / supervisors / admins) ------------------------
$router->post('/employees', [EmployeeController::class, 'create'], [AuthMiddleware::class]);
$router->get('/employees', [EmployeeController::class, 'index'], [AuthMiddleware::class]);
$router->get('/employees/by-ext/{extNum}', [EmployeeController::class, 'showByExtNum'], [AuthMiddleware::class]);
$router->get('/employees/{id}', [EmployeeController::class, 'show'], [AuthMiddleware::class]);
$router->patch('/employees/{id}', [EmployeeController::class, 'update'], [AuthMiddleware::class]);
$router->patch('/employees/{id}/password', [EmployeeController::class, 'changePassword'], [AuthMiddleware::class]);
$router->post('/employees/{id}/machines/{machineNo}', [EmployeeController::class, 'bindMachine'], [AuthMiddleware::class]);
$router->delete('/employees/{id}/machines/{machineNo}', [EmployeeController::class, 'unbindMachine'], [AuthMiddleware::class]);

// ---- Customers --------------------------------------------------------------
// 2026-07-03: dropped. This deployment's ADICTICallCenter database has no
// dbo.customers table and can't have new tables created (see
// sql/migrate-legacy-adicticallcenter.sql's header comment) - customer
// management is unavailable here.

// ---- Machines (PBX / CallerID Box / Voice Card slots) ----------------------
// 這五組（machines / outline-ports / extline-ports）刻意不掛 AuthMiddleware：
// ADICTICallCenter.Web（網頁版控制台，port 8842）不再要求登入，這些是它
// 唯一會呼叫的端點。employees/customers/call-records 不受影響，仍然要求
// Bearer Token（OperatorWeb 的登入/座席身分辨識靠這幾組維持）。
$router->get('/machines', [MachineController::class, 'index']);
$router->get('/machines/{machineType}/{machineNo}', [MachineController::class, 'show']);
$router->patch('/machines/{machineType}/{machineNo}', [MachineController::class, 'update']);

// ---- Outline ports (one row per physical external-line port; vport is a
//      plain column, not a fixed-size pool - see doc/README.md) -------------
$router->get('/outline-ports', [OutlinePortController::class, 'index']);
$router->get('/outline-ports/by-vport/{vport}', [OutlinePortController::class, 'showByVport']);
$router->get('/outline-ports/{id}', [OutlinePortController::class, 'show']);
$router->patch('/outline-ports/{id}', [OutlinePortController::class, 'update']);
$router->delete('/outline-ports/{id}', [OutlinePortController::class, 'destroy']);

// ---- Extline ports (one row per physical internal-line port) --------------
$router->get('/extline-ports', [ExtlinePortController::class, 'index']);
$router->get('/extline-ports/by-vport/{vport}', [ExtlinePortController::class, 'showByVport']);
$router->get('/extline-ports/by-ext-num/{machineType}/{machineNo}/{extNum}', [ExtlinePortController::class, 'showByExtNum']);
$router->get('/extline-ports/{id}', [ExtlinePortController::class, 'show']);
$router->patch('/extline-ports/{id}', [ExtlinePortController::class, 'update']);
$router->delete('/extline-ports/{id}', [ExtlinePortController::class, 'destroy']);

// ---- Call records -----------------------------------------------------------
$router->post('/call-records', [CallRecordController::class, 'create'], [AuthMiddleware::class]);
$router->get('/call-records', [CallRecordController::class, 'index'], [AuthMiddleware::class]);
$router->get('/call-records/{id}', [CallRecordController::class, 'show'], [AuthMiddleware::class]);
$router->patch('/call-records/{id}', [CallRecordController::class, 'update'], [AuthMiddleware::class]);
