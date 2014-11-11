<?php

abstract class Test
{
	public $total, $ok, $fails, $localFails;
	public $connection, $db;

	public function __construct()
	{
		$this->total = 0;
		$this->ok = 0;
		$this->fails = 0;
		$this->localFails = 0;
		$this->db = true;
	}

	protected function before()
	{
		if ($this->db) {
			$this->connection = pg_connect("dbname=hhvm-pgsql");
			if ($this->connection === false) {
				die("Could not connect with the DB!\n");
			}
		}
	}

	protected function after()
	{
		if ($this->db && !is_null($this->connection)) {
			pg_close($this->connection);
		}
	}

	public final function run()
	{
		$klass = get_class($this);
		$methods = get_class_methods($this);

		foreach($methods as $method) {
			$this->localFails = 0;
			if ($this->startsWith($method, 'test')) {
				$name = $klass . '#' . $method;

				// Execute it.
				$this->before();
				call_user_func_array([$this, $method], []);
				$this->after();

				// Print the results.
				if ($this->localFails > 0) {
					$str = 'assert';
					if ($this->localFails > 1) {
						$str .= 's';
					}
					$str = "({$this->localFails}/{$this->total} $str)";
					echo "\x1b[31mFail: $name $str\033[0m\n";
				} else {
					echo "\x1b[32mOK: $name\033[0m\n";
				}

			}
		}
	}

	protected function assert($cond)
	{
		$this->total++;
		if (!$cond) {
			$this->localFails++;
			$this->fails++;
		}
	}

	protected function fails($cond)
	{
		$this->assert(!$cond);
	}

	protected function assertEqual($a, $b)
	{
		$this->assert($a == $b);
	}

	protected function assertExactly($a, $b)
	{
		$this->assert($a === $b);
	}

	protected function assertNotEqual($a, $b)
	{
		$this->assert($a != $b);
	}

	private function startsWith($name, $prefix)
	{
		return $prefix === '' || strpos($name, $prefix) === 0;
	}
}

