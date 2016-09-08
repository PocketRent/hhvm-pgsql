# Extension has moved

As of the 3.15.0 release, this extension is now included in [HHVM core][fb-hphp].

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
