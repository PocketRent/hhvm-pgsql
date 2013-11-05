## Postgres Extension for HipHop

This is an implementation of the `pgsql` PHP extension for the [HipHop PHP VM][fb-hphp].

### Prerequisites

This extension only requires the `libpq` library distributed with Postgres and HipHop VM itself.

### Building & Installation

Installation requires a copy of HipHop to be built from source on the local machine, instructions
on how to do this are available on the [HipHop Wiki][fb-wiki]. Once done, the following commands
will build the extension.

~~~
$ cd /path/to/extension
$ $HPHP_HOME/hphp/tools/hphpize/hphpize
$ cmake .
$ make
~~~

This will produce a `pgsql.so` file, the dynamically-loadable extension.

To enable the extension, you need to have the following section in your hhvm config file

~~~
DynamicExtensionPath = /path/to/hhvm/extensions
DynamicExtensions {
	* = pgsql.so
}
~~~

Where `/path/to/hhvm/extensions` is a folder containing all HipHop extensions, and `pgsql.so` is in
it. This will cause the extension to be loaded when the virtual machine starts up.

### Differences from Zend

There are a few differences from the standard Zend implementation.

* The connection resource is not optional.
* Persistent connections are not implemented
* The following functions are not implemented for various reasons:
  * `pg_convert`
  * `pg_copy_from`
  * `pg_copy_to`
  * `pg_field_type`
  * `pg_insert`
  * `pg_last_oid`
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
  * `pg_pconnect`
  * `pg_put_line`
  * `pg_select`
  * `pg_set_client_encoding`
  * `pg_set_error_verbosity`
  * `pg_trace`
  * `pg_tty`
  * `pg_untrace`
  * `pg_update`

Otherwise, all functionality is (or should be) the same as the Zend implementation.

As always, bugs should be reported to the issue tracker and patches are very welcome.

[fb-hphp]: https://github.com/facebook/hiphop-php "HipHop PHP"
[fb-wiki]: https://github.com/facebook/hiphop-php/wiki "HipHop Wiki"
