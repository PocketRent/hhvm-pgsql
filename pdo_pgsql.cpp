#include "pdo_pgsql.h"
#include "pdo_pgsql_connection.h"

namespace HPHP {

	// Initialize the extension
	static PDOPgSql s_pgsql_driver;

	PDOPgSql::PDOPgSql() : PDODriver("pgsql") {
	}

	PDOConnection *PDOPgSql::createConnectionObject() {
		return new PDOPgSqlConnection();
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
}
