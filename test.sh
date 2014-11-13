#!/bin/sh

# Paths
DIR="$( cd "$( dirname "$0" )" && pwd )"
HHVM=`which hhvm 2>/dev/null`

# Check that HHVM could be found.
if [ -z "${HHVM}" ]; then
    echo -e "\x1b[31;1mCould not locate HHVM!\x1b[0m"
    exit 1
fi

# Create the needed tables with some test values. The `hhvm-pgsql`
# database has to exist already.
psql hhvm-pgsql < tests/test.sql

# And finally execute the test suite.
${HHVM} \
    -vDynamicExtensions.0=${DIR}/pgsql.so   \
    ${DIR}/tests/runner.php "$1"
if [ $? -ne 0 ]; then
    exit 1
fi

${HHVM} \
    -vDynamicExtensions.0=${DIR}/pgsql.so   \
    -c ${DIR}/tests/respect.hdf             \
    ${DIR}/tests/runner.php "$1" respect

