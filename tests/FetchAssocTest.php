<?php

class FetchAssocTest extends Test
{
	public function testFetchAssoc()
	{
		pg_prepare($this->connection, 'query', 'SELECT * FROM complete');
		$ret = pg_execute($this->connection, 'query', []);
		$this->fails($ret === false);

		$row = pg_fetch_assoc($ret);
		pg_free_result($ret);
		$this->assert($row['smallintex'] === '1');
		$this->assert($row['integerex'] === '2');
		$this->assert($row['bigintex'] === '3');
		$this->assert($row['decimalex'] === '4');
		$this->assert($row['numericex'] === '5');
		$this->assert($row['realex'] === '6.5');
		$this->assert($row['doubleex'] === '7.6');
		$this->assert($row['smallserialex'] === '8');
		$this->assert($row['serialex'] === '9');
		$this->assert($row['bigserialex'] === '10');
	}

	// Same as testFetchAssoc but respecting the types.
	public function testRespectFetchAssoc()
	{
		pg_prepare($this->connection, 'query', 'SELECT * FROM complete');
		$ret = pg_execute($this->connection, 'query', []);
		$this->fails($ret === false);

		$row = pg_fetch_assoc($ret);
		pg_free_result($ret);
		$this->assert($row['smallintex'] === 1);
		$this->assert($row['integerex'] === 2);
		$this->assert($row['bigintex'] === 3);
		$this->assert($row['decimalex'] === '4');
		$this->assert($row['numericex'] === '5');
		$this->assert($row['realex'] === 6.5);
		$this->assert($row['doubleex'] === 7.6);
		$this->assert($row['smallserialex'] === 8);
		$this->assert($row['serialex'] === 9);
		$this->assert($row['bigserialex'] === 10);
	}
}

