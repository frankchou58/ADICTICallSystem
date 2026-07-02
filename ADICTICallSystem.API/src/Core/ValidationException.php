<?php

namespace App\Core;

/** Thrown for bad client input; the front controller turns this into a 422 JSON response. */
class ValidationException extends \RuntimeException
{
}
