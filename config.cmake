
# Find include directory
# TODO: we could just use: find_package(PostgreSQL REQUIRED). But there are
# some Ubuntu versions with a buggy FindPostgreSQL.cmake file.
FIND_PATH(PGSQL_INCLUDE_DIR NAMES libpq-fe.h
    PATHS /usr/include /usr/include/postgresql /usr/local/include /usr/local/include/postgresql)

FIND_LIBRARY(PostgreSQL_LIBRARIES NAMES pq PATHS /lib /usr/lib /usr/local/lib)

IF (PGSQL_INCLUDE_DIR AND PostgreSQL_LIBRARIES)
    MESSAGE(STATUS "pgSQL Include dir: ${PGSQL_INCLUDE_DIR}")
    MESSAGE(STATUS "libpq library: ${PostgreSQL_LIBRARIES}")
ELSE()
    MESSAGE(FATAL_ERROR "Cannot find libpq library")
ENDIF()

option(HACK_FRIENDLY "Alters the API to make it work better with the Hack typechecker")

if (HACK_FRIENDLY)
    add_definitions(-DHACK_FRIENDLY)
endif()

include_directories(${PGSQL_INCLUDE_DIR})

HHVM_EXTENSION(pgsql src/pgsql.cpp)
HHVM_SYSTEMLIB(pgsql src/ext_pgsql.php)

target_link_libraries(pgsql ${PostgreSQL_LIBRARIES})

