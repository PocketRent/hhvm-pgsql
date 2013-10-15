# Find include directory
FIND_PATH(PGSQL_INCLUDE_DIR NAMES libpq-fe.h
    PATHS /usr/include /usr/include/postgresql /usr/local/include /usr/local/include/postgresql)

FIND_LIBRARY(PGSQL_LIBRARY NAMES pq PATHS /lib /usr/lib /usr/local/lib)

IF (PGSQL_INCLUDE_DIR AND PGSQL_LIBRARY)
	MESSAGE(STATUS "pgSQL Include dir: ${PGSQL_INCLUDE_DIR}")
	MESSAGE(STATUS "libpq library: ${PGSQL_LIBRARY}")
ELSE()
    MESSAGE(FATAL_ERROR "Cannot find libpq library")
ENDIF()

include_directories(${PGSQL_INCLUDE_DIR})

HHVM_EXTENSION(pgsql pgsql.cpp)
HHVM_SYSTEMLIB(pgsql pgsql.php)

target_link_libraries(pgsql ${PGSQL_LIBRARY})
