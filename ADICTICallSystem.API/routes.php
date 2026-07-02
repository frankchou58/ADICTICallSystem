<?php

use App\Controllers\AuthController;
use App\Controllers\CallRecordController;
use App\Controllers\CustomerController;
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
$router->post('/employees/{id}/ext-lines/{vport}', [EmployeeController::class, 'bindExtLine'], [AuthMiddleware::class]);
$router->delete('/employees/{id}/ext-lines/{vport}', [EmployeeController::class, 'unbindExtLine'], [AuthMiddleware::class]);

// ---- Customers ------------------------------------------------------------
$router->post('/customers', [CustomerController::class, 'create'], [AuthMiddleware::class]);
$router->get('/customers', [CustomerController::class, 'index'], [AuthMiddleware::class]);
$router->get('/customers/by-tel/{telNo}', [CustomerController::class, 'showByTel'], [AuthMiddleware::class]);
$router->get('/customers/{id}', [CustomerController::class, 'show'], [AuthMiddleware::class]);
$router->patch('/customers/{id}', [CustomerController::class, 'update'], [AuthMiddleware::class]);

// ---- Machines (PBX / CallerID Box / Voice Card slots) ----------------------
$router->get('/machines', [MachineController::class, 'index'], [AuthMiddleware::class]);
$router->get('/machines/{machineType}/{machineNo}', [MachineController::class, 'show'], [AuthMiddleware::class]);
$router->patch('/machines/{machineType}/{machineNo}', [MachineController::class, 'update'], [AuthMiddleware::class]);

// ---- Outline ports (one row per physical external-line port; vport is a
//      plain column, not a fixed-size pool - see doc/README.md) -------------
$router->get('/outline-ports', [OutlinePortController::class, 'index'], [AuthMiddleware::class]);
$router->get('/outline-ports/by-vport/{vport}', [OutlinePortController::class, 'showByVport'], [AuthMiddleware::class]);
$router->get('/outline-ports/{id}', [OutlinePortController::class, 'show'], [AuthMiddleware::class]);
$router->patch('/outline-ports/{id}', [OutlinePortController::class, 'update'], [AuthMiddleware::class]);

// ---- Extline ports (one row per physical internal-line port) --------------
$router->get('/extline-ports', [ExtlinePortController::class, 'index'], [AuthMiddleware::class]);
$router->get('/extline-ports/by-vport/{vport}', [ExtlinePortController::class, 'showByVport'], [AuthMiddleware::class]);
$router->get('/extline-ports/by-ext-num/{machineType}/{machineNo}/{extNum}', [ExtlinePortController::class, 'showByExtNum'], [AuthMiddleware::class]);
$router->get('/extline-ports/{id}', [ExtlinePortController::class, 'show'], [AuthMiddleware::class]);
$router->patch('/extline-ports/{id}', [ExtlinePortController::class, 'update'], [AuthMiddleware::class]);

// ---- Call records -----------------------------------------------------------
$router->post('/call-records', [CallRecordController::class, 'create'], [AuthMiddleware::class]);
$router->get('/call-records', [CallRecordController::class, 'index'], [AuthMiddleware::class]);
$router->get('/call-records/{id}', [CallRecordController::class, 'show'], [AuthMiddleware::class]);
$router->patch('/call-records/{id}', [CallRecordController::class, 'update'], [AuthMiddleware::class]);
