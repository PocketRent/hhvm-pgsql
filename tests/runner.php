<?php

$dir = dirname(__FILE__);
require_once "$dir/Test.php";

$ok = 0;
$fails = 0;

function endsWith($str, $prefix)
{
  return substr($str, -strlen($prefix)) === $prefix;
}

// Iterates over the `tests` directory, loading all the files except for this
// runner and the `Test` class. Each file contains just one class, with all
// the tests inside it.
if ($handle = opendir($dir)) {
  while (false !== ($entry = readdir($handle))) {
    if (!is_dir($entry) &&
      endsWith($entry, '.php') &&
      $entry !== 'runner.php' &&
      $entry !== 'Test.php') {

      $class = preg_replace('/\.php$/', '', $entry);
      require_once "$dir/$entry";

      $obj = new $class($argv[1], isset($argv[2]));
      $obj->run();
      $fails += $obj->fails;
      $ok += $obj->total - $obj->fails;
    }
  }
  closedir($handle);
}

// And finally print the results and exit with the proper status code.
print_r("\x1b[39;1mAsserts\x1b[0m: \033[32;1m$ok\033[0m/\033[31;1m$fails\033[0m\n");
exit(($fails > 0) ? 1 : 0);

