<?php

abstract class Test
{
	public $total, $ok, $fails, $localFails;
	public $connection, $db;
	public $specific;

	public function __construct($specific)
	{
		$this->total = 0;
		$this->ok = 0;
		$this->fails = 0;
		$this->localFails = 0;
		$this->db = true;
		$this->specific = $specific;
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

		// Check if we have to pick a specific class-method.
		if ($this->specific !== '') {
			$parts = explode('#', $this->specific, 2);
			if ($parts[0] !== $klass) {
				return;
			}
			if (count($parts) > 1) {
				$methods = [$parts[1]];
			}
		}

		foreach($methods as $method) {
			$this->localFails = 0;
			if ($this->startsWith($method, 'test')) {
				$name = $klass . '#' . $method;

				// Execute it.
				$this->before();
				if (method_exists($this, $method)) {
					call_user_func_array([$this, $method], []);
					$this->after();
				} else {
					$this->after();
					$msg = "\x1b[31;1mMethod $klass#$method does not";
					$msg .= " exist\x1b[0m\n";
					print_r($msg);
					continue;
				}

				// Print the results.
				if ($this->localFails > 0) {
					$str = 'assert';
					if ($this->localFails > 1) {
						$str .= 's';
					}
					$str = "({$this->localFails}/{$this->total} $str)";
					print_r("\x1b[31mFail: $name $str\033[0m\n");
				} else {
					print_r("\x1b[32mOK: $name\033[0m\n");
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

