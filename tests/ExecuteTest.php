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
		$this->assertExactly($msg, $expected);
	}

	public function testNoConnection()
	{
		$ret = pg_prepare($this->connection, 'query', 'SELECT * from test');
		$this->assert($ret);

		// Fails because the given connection is not valid.
		pg_close($this->connection);
		$ret = @pg_execute($this->connection, 'query', ['data']);
		$this->assertExactly($ret, false);

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
}

