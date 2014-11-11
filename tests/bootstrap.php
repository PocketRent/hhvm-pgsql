<?php

if (!extension_loaded('pgsql')) {
	print_r("\x1b[31;1mCould not load the `pgsql` extension!\x1b[0m\n");
	exit(1);
}

