<?php

namespace App\Controllers;

use App\Core\Database;
use App\Core\Request;
use App\Core\Response;
use App\Core\ValidationException;
use PDO;

class CustomerController extends Controller
{
    /** Free-text columns that may contain non-ASCII (Chinese) content - see Database::bindText(). */
    private const TEXT_FIELDS = ['name', 'county', 'township', 'address'];

    /** POST /customers */
    public function create(Request $request): void
    {
        $telNo = $request->input('telNo');
        $name = $request->input('name');

        $stmt = $this->db()->prepare(
            'INSERT INTO dbo.customers (customer_no, name, birthday, gender, tel_no, email, county, township, address)
             OUTPUT INSERTED.id
             VALUES (:customer_no, :name, :birthday, :gender, :tel_no, :email, :county, :township, :address)'
        );
        $this->executeWithParams($stmt, [
            'customer_no' => $request->input('customerNo'),
            'name' => $name,
            'birthday' => $request->input('birthday'),
            'gender' => $request->input('gender'),
            'tel_no' => $telNo,
            'email' => $request->input('email'),
            'county' => $request->input('county'),
            'township' => $request->input('township'),
            'address' => $request->input('address'),
        ], self::TEXT_FIELDS);

        Response::created(['id' => (int) $stmt->fetchColumn()], '客戶已建立。');
    }

    /** GET /customers?telNo=&blacklisted=&limit=&offset= */
    public function index(Request $request): void
    {
        [$limit, $offset] = $this->paginationArgs($request);

        $where = [];
        $params = [];
        if ($telNo = $request->query['telNo'] ?? null) {
            $where[] = 'tel_no LIKE :tel_no';
            $params['tel_no'] = '%' . $telNo . '%';
        }
        if (isset($request->query['blacklisted'])) {
            $where[] = 'is_blacklisted = :blacklisted';
            $params['blacklisted'] = filter_var($request->query['blacklisted'], FILTER_VALIDATE_BOOLEAN) ? 1 : 0;
        }

        $sql = 'SELECT id, customer_no, name, birthday, gender, tel_no, email, county, township, address, is_blacklisted
                FROM dbo.customers';
        if ($where) {
            $sql .= ' WHERE ' . implode(' AND ', $where);
        }
        $sql .= ' ORDER BY id DESC OFFSET :offset ROWS FETCH NEXT :limit ROWS ONLY';

        $stmt = $this->db()->prepare($sql);
        foreach ($params as $key => $value) {
            $stmt->bindValue($key, $value);
        }
        $stmt->bindValue('offset', $offset, PDO::PARAM_INT);
        $stmt->bindValue('limit', $limit, PDO::PARAM_INT);
        $stmt->execute();

        Response::ok(array_map([$this, 'mapRow'], $stmt->fetchAll()));
    }

    /** GET /customers/{id} */
    public function show(Request $request): void
    {
        $row = $this->fetchById((int) $request->params['id']);
        if (!$row) {
            Response::notFound('找不到這位客戶。');
            return;
        }
        Response::ok($this->mapRow($row));
    }

    /** GET /customers/by-tel/{telNo} */
    public function showByTel(Request $request): void
    {
        $stmt = $this->db()->prepare(
            'SELECT TOP 1 id, customer_no, name, birthday, gender, tel_no, email, county, township, address, is_blacklisted
             FROM dbo.customers WHERE tel_no = :tel_no ORDER BY id DESC'
        );
        $stmt->execute(['tel_no' => $request->params['telNo']]);
        $row = $stmt->fetch();

        if (!$row) {
            Response::notFound('找不到這位客戶。');
            return;
        }
        Response::ok($this->mapRow($row));
    }

    /** PATCH /customers/{id} */
    public function update(Request $request): void
    {
        $id = (int) $request->params['id'];

        $columnMap = [
            'customerNo' => 'customer_no',
            'name' => 'name',
            'birthday' => 'birthday',
            'gender' => 'gender',
            'telNo' => 'tel_no',
            'email' => 'email',
            'county' => 'county',
            'township' => 'township',
            'address' => 'address',
        ];

        $fields = [];
        $params = ['id' => $id];
        foreach ($columnMap as $inputKey => $column) {
            $value = $request->input($inputKey);
            if ($value !== null) {
                $fields[] = "$column = :$column";
                $params[$column] = $value;
            }
        }
        if (($isBlacklisted = $request->input('isBlacklisted')) !== null) {
            $fields[] = 'is_blacklisted = :is_blacklisted';
            $params['is_blacklisted'] = $isBlacklisted ? 1 : 0;
        }

        if (empty($fields)) {
            throw new ValidationException('沒有提供任何可更新的欄位。');
        }

        $fields[] = 'updated_at = SYSUTCDATETIME()';
        $sql = 'UPDATE dbo.customers SET ' . implode(', ', $fields) . ' WHERE id = :id';
        $stmt = $this->db()->prepare($sql);
        $this->executeWithParams($stmt, $params, self::TEXT_FIELDS);

        if ($stmt->rowCount() === 0) {
            Response::notFound('找不到這位客戶。');
            return;
        }

        Response::ok(null, '客戶資料已更新。');
    }

    /** @return array|false */
    private function fetchById(int $id)
    {
        $stmt = $this->db()->prepare(
            'SELECT id, customer_no, name, birthday, gender, tel_no, email, county, township, address, is_blacklisted
             FROM dbo.customers WHERE id = :id'
        );
        $stmt->execute(['id' => $id]);
        return $stmt->fetch();
    }

    private function mapRow(array $row): array
    {
        return [
            'id' => (int) $row['id'],
            'customerNo' => $row['customer_no'],
            'name' => Database::decodeText($row['name']),
            'birthday' => $row['birthday'],
            'gender' => $row['gender'],
            'telNo' => $row['tel_no'],
            'email' => $row['email'],
            'county' => Database::decodeText($row['county']),
            'township' => Database::decodeText($row['township']),
            'address' => Database::decodeText($row['address']),
            'isBlacklisted' => (bool) $row['is_blacklisted'],
        ];
    }
}
