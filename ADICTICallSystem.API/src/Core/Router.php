<?php

namespace App\Core;

/**
 * Minimal dependency-free router. Supports `{param}` placeholders and an
 * optional list of middleware callables per route, e.g.:
 *
 *   $router->get('/employees/{id}', [EmployeeController::class, 'show'], [AuthMiddleware::class]);
 *
 * Deliberately dependency-free (no Composer install step) so deployment is
 * "copy the folder onto a PHP + pdo_sqlsrv host", matching how the legacy
 * one-file-per-endpoint API was deployed.
 */
class Router
{
    private array $routes = [];

    /** @param callable|array $handler */
    public function get(string $pattern, $handler, array $middleware = []): void
    {
        $this->add('GET', $pattern, $handler, $middleware);
    }

    /** @param callable|array $handler */
    public function post(string $pattern, $handler, array $middleware = []): void
    {
        $this->add('POST', $pattern, $handler, $middleware);
    }

    /** @param callable|array $handler */
    public function patch(string $pattern, $handler, array $middleware = []): void
    {
        $this->add('PATCH', $pattern, $handler, $middleware);
    }

    /** @param callable|array $handler */
    public function delete(string $pattern, $handler, array $middleware = []): void
    {
        $this->add('DELETE', $pattern, $handler, $middleware);
    }

    /** @param callable|array $handler */
    private function add(string $method, string $pattern, $handler, array $middleware): void
    {
        $this->routes[] = compact('method', 'pattern', 'handler', 'middleware');
    }

    public function dispatch(string $method, string $uri, Request $request): void
    {
        $path = parse_url($uri, PHP_URL_PATH) ?: '/';
        $path = rtrim($path, '/');
        if ($path === '') {
            $path = '/';
        }

        foreach ($this->routes as $route) {
            if ($route['method'] !== $method) {
                continue;
            }

            $params = $this->match($route['pattern'], $path);
            if ($params === null) {
                continue;
            }

            $request->params = $params;

            foreach ($route['middleware'] as $middlewareClass) {
                $middleware = new $middlewareClass();
                // A middleware may short-circuit the request (e.g. 401) by
                // returning false after writing its own Response.
                if ($middleware->handle($request) === false) {
                    return;
                }
            }

            [$controllerClass, $action] = $route['handler'];
            $controller = new $controllerClass();
            $controller->$action($request);
            return;
        }

        Response::notFound("找不到符合的路由：$method $path");
    }

    private function match(string $pattern, string $path): ?array
    {
        $pattern = rtrim($pattern, '/');
        if ($pattern === '') {
            $pattern = '/';
        }

        $regex = preg_replace('#\{([a-zA-Z_][a-zA-Z0-9_]*)\}#', '(?P<$1>[^/]+)', $pattern);
        $regex = '#^' . $regex . '$#';

        if (!preg_match($regex, $path, $matches)) {
            return null;
        }

        $params = [];
        foreach ($matches as $key => $value) {
            if (is_string($key)) {
                $params[$key] = $value;
            }
        }
        return $params;
    }
}
