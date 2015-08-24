#include "pdo_pgsql.h"
#include "pdo_pgsql_connection.h"
#include "pdo_pgsql_resource.h"

namespace HPHP {

// Initialize the extension
static PDOPgSql s_pgsql_driver;

PDOPgSql::PDOPgSql() : PDODriver("pgsql") {
    PQinitSSL(0);
}

SmartPtr<PDOResource> PDOPgSql::createResourceImpl() {
    return makeSmartPtr<PDOPgSqlResource>(std::make_shared<PDOPgSqlConnection>());
}

SmartPtr<PDOResource> PDOPgSql::createResourceImpl(const sp_PDOConnection& conn) {
    return makeSmartPtr<PDOPgSqlResource>(
        std::dynamic_pointer_cast<PDOPgSqlConnection>(conn)
    );
}

long pdo_attr_lval(const Array& options, int opt, long defaultValue){
    if(options.exists(opt)){
        return options[opt].toInt64();
    }
    return defaultValue;
}

String pdo_attr_strval(const Array& options, int opt, const char *def){
    if(options.exists(opt)){
        return options[opt].toString();
    }
    if(def){
        return def;
    }
    return String();
}

const StaticString s_PDO("PDO");

static class PDOPGSQLExtension : public Extension {
  public:
    PDOPGSQLExtension() : Extension("pdo_pgsql") {}

    virtual void moduleLoad(const IniSetting::Map& ini, Hdf hdf) {
        Native::registerClassConstant<KindOfInt64>(
            s_PDO.get(),
            s_PGSQL_ATTR_DISABLE_NATIVE_PREPARED_STATEMENT.get(),
            PDO_PGSQL_ATTR_DISABLE_NATIVE_PREPARED_STATEMENT
        );
        Native::registerClassConstant<KindOfInt64>(
            s_PDO.get(),
            s_PGSQL_ATTR_DISABLE_PREPARES.get(),
            PDO_PGSQL_ATTR_DISABLE_PREPARES
        );
    }
} s_pdopgsql_extension;
}
