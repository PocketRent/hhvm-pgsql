
find_package(PostgreSQL REQUIRED)

option(HACK_FRIENDLY "Alters the API to make it work better with the Hack typechecker")

if (HACK_FRIENDLY)
    add_definitions(-DHACK_FRIENDLY)
endif()

include_directories(${PGSQL_INCLUDE_DIR})

HHVM_EXTENSION(pgsql src/pgsql.cpp)
HHVM_SYSTEMLIB(pgsql src/ext_pgsql.php)

target_link_libraries(pgsql ${PostgreSQL_LIBRARIES})

