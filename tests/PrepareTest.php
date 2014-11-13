<?php

class PrepareTest extends Test
{
	public function testNoConnection()
	{
		// Fails because the connection is not valid.
		pg_close($this->connection);
		$ret = @pg_prepare($this->connection, 'query', 'SELECT * from test');
		$this->assert($ret === false);

		// Tell the `after` method that the connection has already been closed.
		$this->connection = null;
	}

	public function testMalformed()
	{
		// Fails because the given query is malformed.
		$ret = @pg_prepare($this->connection, 'query', 'SELECT * fro');
		$this->assert($ret === false);
	}

	public function testNormal()
	{
		$ret = pg_prepare($this->connection, 'query', 'SELECT * from test');
		$this->fails($ret === false);
	}
}
