#ifndef incl_HPHP_PDO_PGSQL_H_
#define incl_HPHP_PDO_PGSQL_H_

#include "hphp/runtime/ext/pdo_driver.h"

namespace HPHP {
    class PDOPgSql : public PDODriver {
    public:
        PDOPgSql();

        virtual PDOConnection* createConnectionObject();
    };

    long pdo_attr_lval(const Array& options, int opt, long defaultValue);

    String pdo_attr_strval(const Array& options, int opt, const char *def);
}
#endif
