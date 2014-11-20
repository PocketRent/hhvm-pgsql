<?php

class ExecuteTest extends Test
{
  public function testNotPrepared()
  {
    // Should fail because the 'query' prepared statement does not exist.
    $res = @pg_execute($this->connection, 'query', ['data']);
    $this->fails($res);

    $msg = pg_last_error($this->connection);
    $expected = 'ERROR:  prepared statement "query" does not exist';
    $this->assert($msg === $expected);
  }

  public function testNoConnection()
  {
    $ret = pg_prepare($this->connection, 'query', 'SELECT * from test');
    $this->assert($ret);

    // Fails because the given connection is not valid.
    pg_close($this->connection);
    $ret = @pg_execute($this->connection, 'query', ['data']);
    $this->assert($ret === false);

    // Tell the `after` method that the connection has already been closed.
    $this->connection = null;
  }

  public function testNormal()
  {
    $ret = pg_prepare($this->connection, 'query', 'SELECT * from test');
    $this->fails($ret === false);

    // This execution should be successful.
    $ret = pg_execute($this->connection, 'query', []);
    $this->fails($ret === false);
  }

  public function testBooleanInsert()
  {
    $sql = 'INSERT INTO test(id, valid) VALUES($1, $2)';
    $ret = pg_prepare($this->connection, 'query', $sql);
    $this->fails($ret === false);

    // This execution fails: boolean converted to empty string.
    $ret = @pg_execute($this->connection, 'query', [1, false]);
    $this->assert($ret === false);
  }

  public function testRespectTypes()
  {
    $sql = 'INSERT INTO test(id, valid) VALUES($1, $2)';
    $ret = pg_prepare($this->connection, 'query', $sql);
    $this->fails($ret === false);

    // This execution should be successful.
    $ret1 = pg_execute($this->connection, 'query', [1, true]);
    $ret2 = pg_execute($this->connection, 'query', [2, false]);
    $this->fails($ret1 === false);
    $this->fails($ret2 === false);

    // Check that the inserted values are ok.
    $stmt = pg_query($this->connection, 'SELECT * FROM test');
    $this->fails($stmt === false);
    $row = pg_fetch_assoc($stmt);
    $this->assert($row['id'] === 1);
    $this->assert($row['name'] === null);
    $this->assert($row['valid'] === true);
    $this->assert($row['someint'] === null);
    $this->assert($row['sometime'] === null);
    $row = pg_fetch_assoc($stmt);
    $this->assert($row['id'] === 2);
    $this->assert($row['name'] === null);
    $this->assert($row['valid'] === false);
    $this->assert($row['someint'] === null);
    $this->assert($row['sometime'] === null);
  }
}

