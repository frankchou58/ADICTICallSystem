<?php

namespace App\Core;

/**
 * Wraps the current HTTP request: parsed JSON/query input, path params
 * (filled in by the Router) and the authenticated employee (filled in by
 * AuthMiddleware). Centralising this avoids every controller re-reading
 * php://input / $_GET the way every legacy endpoint did independently.
 */
class Request
{
    public array $query;
    public array $body;
    public array $params = [];
    public ?array $employee = null;

    private function __construct(array $query, array $body)
    {
        $this->query = $query;
        $this->body = $body;
    }

    public static function capture(): self
    {
        $query = $_GET ?? [];

        $body = [];
        $contentType = $_SERVER['CONTENT_TYPE'] ?? '';
        if (stripos($contentType, 'application/json') !== false) {
            $raw = file_get_contents('php://input');
            $decoded = json_decode($raw, true);
            if (is_array($decoded)) {
                $body = $decoded;
            }
        } else {
            $body = $_POST ?? [];
        }

        return new self($query, $body);
    }

    /** Merged view: path params > body > query, matching REST expectations. */
    public function input(string $key, $default = null)
    {
        if (array_key_exists($key, $this->params)) {
            return $this->params[$key];
        }
        if (array_key_exists($key, $this->body)) {
            return $this->body[$key];
        }
        if (array_key_exists($key, $this->query)) {
            return $this->query[$key];
        }
        return $default;
    }

    /** @return mixed */
    public function requireInput(string $key)
    {
        $value = $this->input($key);
        if ($value === null || $value === '') {
            throw new ValidationException("缺少必要欄位：$key");
        }
        return $value;
    }

    public function bearerToken(): ?string
    {
        $header = $_SERVER['HTTP_AUTHORIZATION'] ?? '';
        if ($header === '' && function_exists('apache_request_headers')) {
            $headers = apache_request_headers();
            $header = $headers['Authorization'] ?? '';
        }
        if (preg_match('/Bearer\s+(\S+)/i', $header, $matches)) {
            return $matches[1];
        }
        return null;
    }
}
