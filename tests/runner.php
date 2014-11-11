<?php

$dir = dirname(__FILE__);
require_once "$dir/Test.php";

$ok = 0;
$fails = 0;

// Iterates over the `tests` directory, loading all the files except for this
// runner and the `Test` class. Each file contains just one class, with all
// the tests inside it.
if ($handle = opendir($dir)) {
	while (false !== ($entry = readdir($handle))) {
		if (!is_dir($entry) && $entry !== 'runner.php' &&
				$entry !== 'Test.php') {

			$class = preg_replace('/\.php$/', '', $entry);
			require_once "$dir/$entry";

			$obj = new $class();
			$obj->run();
			$fails += $obj->fails;
			$ok += $obj->total - $obj->fails;
		}
	}
	closedir($handle);
}

// And finally print the results.
print_r("\x1b[39;1mAsserts\x1b[0m: \033[32;1m$ok\033[0m/\033[31;1m$fails\033[0m\n");

