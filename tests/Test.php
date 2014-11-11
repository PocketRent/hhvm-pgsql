<?php

/**
 * @class Test
 *
 * This abstract class has to be subclassed by all the tests of this test
 * suite. A test is a method the name of which starts with "test". A test class
 * can have multiple tests and each test can have multiple asserts. This class
 * in combination with `runner.php` will clearly state which tests are failing
 * and how many asserts are ok/failing.
 *
 * Moreover, a class test requires a `specific` filter that is given by the
 * `runner.php` file. This filter can be:
 *
 * 1. Empty: no filter.
 * 2. Class: just execute the tests of the specified class.
 * 3. Class#test: just execute the specified method from the given class.
 *
 * For example:
 *
 * $ ./test.sh ExecuteTest#testNotPrepared
 *
 * The previous command will just execute the test named `testNotPrepared` from
 * the `ExecuteTest` class.
 */
abstract class Test
{
	/**
	 * @var int: the total number of asserts that have been run so far.
	 */
	public $total;

	/**
	 * @var int: the number of asserts with a positive result.
	 */
	public $ok;

	/**
	 * @var int: the number of asserts with a negative result.
	 */
	public $fails

	/**
	 * @var int: the number of failures of the last test.
	 */
	public $localFails;

	/**
	 * @var resource: a database connection that will be established before a
	 * test has to run and that will be closed after a test has run.
	 */
	protected $connection

	/**
	 * @var bool: whether the $connection mechanism has to be established or
	 * not. It defaults to true. You should change it by overriding the
	 * constructor and setting it to false manually.
	 */
	protected $db;

	/**
	 * @var string: he filter as given by the `runner.php` file.
	 */
	public $specific;

	/**
	 * Constructor. It should be subclassed only if you want to set the
	 * $this->db attribute manually.
	 *
	 * @param string $specific The filter to be applied.
	 */
	public function __construct($specific)
	{
		$this->total = 0;
		$this->ok = 0;
		$this->fails = 0;
		$this->localFails = 0;
		$this->db = true;
		$this->specific = $specific;
	}

	/**
	 * Hook that will be called before each test. By default it establishes the
	 * $this->connection.
	 */
	protected function before()
	{
		if ($this->db) {
			$this->connection = pg_connect("dbname=hhvm-pgsql");
			if ($this->connection === false) {
				die("Could not connect with the DB!\n");
			}
		}
	}

	/**
	 * Hook that will be called after each test. By default it closes the
	 * $this->connection that was established in the `before` function.
	 */
	protected function after()
	{
		if ($this->db && !is_null($this->connection)) {
			pg_close($this->connection);
		}
	}

	/**
	 * Run all the tests for this class.
	 */
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

		// And finally run all the available methods.
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

	/**
	 * Assert that the given condition has to be true.
	 *
	 * @param bool $cond The given condition.
	 */
	protected function assert($cond)
	{
		$this->total++;
		if (!$cond) {
			$this->localFails++;
			$this->fails++;
		}
	}

	/**
	 * Assert that the given condition has to be false.
	 *
	 * @param bool $cond The given condition.
	 */
	protected function fails($cond)
	{
		$this->assert(!$cond);
	}

	/**
	 * Get whether the given $name starts with the given $prefix.
	 *
	 * @param string $name The name to be matched.
	 * @param string $prefix The string that has to prefix the given $name.
	 * @return bool.
	 */
	private function startsWith($name, $prefix)
	{
		return $prefix === '' || strpos($name, $prefix) === 0;
	}
}

