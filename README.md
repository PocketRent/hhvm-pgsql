## Postgres Extension for HipHop

This is an implementation of the `pgsql` and `pdo_pgsql` PHP extensions for
[HHVM][fb-hphp].

### Prerequisites

To run, this extension only requires the HipHop VM itself and the `libpq`
library distributed with Postgres. Building it requires the HHVM header files,
the HHVM CMake files and the `libpq` header files. These can usually be
installed with the `hhvm-dev` and `libpq-dev` packages.

### Pre-built versions

Pre-built versions of this extension are available in the
[releases][pr-releases] branch.

### Building & Installation

Building requires the `hhvm-dev` and `libpq-dev` packages to be installed. Once
they have been installed, the following commands will build the extensions.

~~~
$ cd /path/to/extension
$ hphpize
$ cmake .
$ make
~~~

This will produce a `pgsql.so` file, the dynamically-loadable extension.
Copy this file to `/etc/hhvm/pgsql.so`.

To enable the extension, you need to have the following section in your hhvm
config file:

~~~
extension_dir = /etc/hhvm
hhvm.extensions[pgsql] = pgsql.so
~~~

This will cause the extension to be loaded when the virtual machine starts up.

### Hack Friendly Mode

If you are using Hack, then you can use the provided `pgsql.hhi` file to type
the functions. There is also a compile-time option that can be passed to cmake
that makes some minor adjustments to the API to make the Hack type checker more
useful with them. This mostly consists of altering functions that would normally
return `FALSE` on error and making them return `null` instead. This takes
advantage of the nullable types in Hack.

To enable Hack-friendly mode use this command instead of the `cmake` one above:

~~~
$ cmake -DHACK_FRIENDLY=ON .
~~~

### Differences from Zend

There are a few differences from the standard Zend implementation.

* The connection resource is not optional.
* The following functions are not implemented for various reasons:
  * `pg_convert`
  * `pg_copy_from`
  * `pg_copy_to`
  * `pg_insert`
  * `pg_lo_close`
  * `pg_lo_create`
  * `pg_lo_export`
  * `pg_lo_import`
  * `pg_lo_open`
  * `pg_lo_read_all`
  * `pg_lo_read`
  * `pg_lo_seek`
  * `pg_lo_tell`
  * `pg_lo_unlink`
  * `pg_lo_write`
  * `pg_meta_data`
  * `pg_put_line`
  * `pg_select`
  * `pg_set_client_encoding`
  * `pg_set_error_verbosity`
  * `pg_trace`
  * `pg_tty`
  * `pg_untrace`
  * `pg_update`

There is a connection pool you can use with the `pg_pconnect` function.

The `$connection_type` parameter is ignored for both `pg_connect` and
`pg_pconnect`.

There are a few new function:

* `pg_connection_pool_stat`: It gives some information, eg. count of
connections, free connections, etc.
* `pg_connection_pool_sweep_free`: Closing all unused connection in all pool.

The `pg_pconnect` function creates a different connection pool for each
connection string.

The `pg_fetch_object` function only supports returning `stdClass` objects.

Otherwise, all functionality is (or should be) the same as the Zend
implementation.

As always, bugs should be reported to the issue tracker and patches are very
welcome.

[fb-hphp]: https://github.com/facebook/hhvm "HHVM"
[pr-releases]: https://github.com/PocketRent/hhvm-pgsql/tree/releases
