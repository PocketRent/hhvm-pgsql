# PostgreSQL for HHVM releases

These releases add support for PostgreSQL to HHVM. See the README of the main branch for details on what is and isn't supported.

## Installation

1. Download the apropriate `pgsql.so` file and copy it to the `/etc/hhvm/` folder.
2. Edit `/etc/hhvm/php.ini` and add:
~~~ini
extension_dir = /etc/hhvm
hhvm.extensions[pgsql] = pgsql.so
~~~

You can check that everything is working by running `hhvm --php -r 'var_dump(function_exists("pg_connect"));'`.
If everything is working fine, this will output `bool(true)`.

You may also need to restart HHVM to have the server pick up the extension.

## HHVM 3.11.0

* Debian [8 (jessie)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.11.0/debian/jessie/pgsql.so)
* Ubuntu [14.04 (trusty)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.11.0/ubuntu/trusty/pgsql.so) / [15.04 (vivid)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.11.0/ubuntu/vivid/pgsql.so) / [15.10 (wily)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.11.0/ubuntu/wily/pgsql.so)

## HHVM 3.10.0

* Debian [8 (jessie)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.10.0/debian/jessie/pgsql.so)
* Ubuntu [14.04 (trusty)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.10.0/ubuntu/trusty/pgsql.so) / [15.04 (vivid)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.10.0/ubuntu/vivid/pgsql.so)

## HHVM 3.7.0

* Debian [8 (jessie)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.7.0/debian/jessie/pgsql.so)
* Ubuntu [14.04 (trusty)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.7.0/ubuntu/trusty/pgsql.so) / [14.10 (utopic)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.7.0/ubuntu/utopic/pgsql.so) / [15.04 (vivid)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.7.0/ubuntu/vivid/pgsql.so)

## HHVM 3.6.0

* Debian [8 (jessie)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.6.0/debian/jessie/pgsql.so)
* Ubuntu [12.04 (precise)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.6.0/ubuntu/precise/pgsql.so) / [14.04 (trusty)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.6.0/ubuntu/trusty/pgsql.so)

## HHVM 3.3.0

* Debian [8 (jessie)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.3.0/debian/jessie/pgsql.so)
* Ubuntu [12.04 (precise)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.3.0/ubuntu/precise/pgsql.so) / [14.04 (trusty)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.3.0/ubuntu/trusty/pgsql.so)

## HHVM 3.2.0

* Debian [8 (jessie)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.2.0/debian/jessie/pgsql.so)
* Ubuntu [12.04 (precise)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.2.0/ubuntu/precise/pgsql.so) / [14.04 (trusty)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.2.0/ubuntu/trusty/pgsql.so)

As HHVM no longer supports building on Debian 7 (wheezy), DSOs are no longer available for it.

## HHVM 3.1.0

* Debian [7 (wheezy)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.1.0/debian/wheezy/pgsql.so) / [8 (jessie)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.1.0/debian/jessie/pgsql.so)
* Ubuntu [12.04 (precise)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.1.0/ubuntu/precise/pgsql.so) / [14.04 (trusty)](https://github.com/PocketRent/hhvm-pgsql/raw/releases/3.1.0/ubuntu/trusty/pgsql.so)
