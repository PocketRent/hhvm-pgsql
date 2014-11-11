#!/bin/sh

# Paths
DIR="$( cd "$( dirname "$0" )" && pwd )"
HHVM=`which hhvm 2>/dev/null`

# Check that HHVM & PHPUnit could be found.
if [ -z "${HHVM}" ]; then
    echo -e "\x1b[31;1mCould not locate HHVM!\x1b[0m"
    exit 1
fi

# And finally execute the test suite.
${HHVM} -vDynamicExtensions.0=${DIR}/pgsql.so ${DIR}/tests/runner.php "$1"

