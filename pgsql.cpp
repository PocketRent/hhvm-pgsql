#include "pq.h"

#include "hphp/runtime/base/base-includes.h"
#include "hphp/runtime/base/zend-string.h"

#include "hphp/runtime/base/runtime-option.h"
#include "hphp/runtime/server/server-stats.h"
#include "hphp/runtime/ext/ext_string.h"

#define PGSQL_ASSOC 1
#define PGSQL_NUM 2
#define PGSQL_BOTH (PGSQL_ASSOC | PGSQL_NUM)
#define PGSQL_STATUS_LONG 1
#define PGSQL_STATUS_STRING 2

namespace HPHP {

namespace { // Anonymous namespace

struct ScopeNonBlocking {
    ScopeNonBlocking(PQ::Connection& conn, bool mode) :
        m_conn(conn), m_mode(mode) {}

    ~ScopeNonBlocking() {
        m_conn.setNonBlocking(m_mode);
    }

    PQ::Connection& m_conn;
    bool m_mode;
};

class PGSQL : public SweepableResourceData {
    DECLARE_RESOURCE_ALLOCATION(PGSQL);
public:
    static bool AllowPersistent;
    static int MaxPersistent;
    static int MaxLinks;
    static bool AutoResetPersistent;
    static bool IgnoreNotice;
    static bool LogNotice;

    static PGSQL *Get(CVarRef conn_id);

public:
    PGSQL(String conninfo);
    ~PGSQL();

    static StaticString s_class_name;
    virtual const String& o_getClassNameHook() const { return s_class_name; }
    virtual bool isResource() const { return (bool)m_conn; }

    PQ::Connection &get() { return m_conn; }

    ScopeNonBlocking asNonBlocking() {
        auto mode = m_conn.isNonBlocking();
        return ScopeNonBlocking(m_conn, mode);
    }

private:
    PQ::Connection m_conn;

public:
    std::string m_conn_string;

    std::string m_db;
    std::string m_user;
    std::string m_pass;
    std::string m_host;
    std::string m_port;
    std::string m_options;

    std::string m_last_notice;
};

class PGSQLResult : public SweepableResourceData {
    DECLARE_RESOURCE_ALLOCATION(PGSQLResult);
public:
    static PGSQLResult *Get(CVarRef result);
public:
    PGSQLResult(PGSQL* conn, PQ::Result res);
    ~PGSQLResult();

    static StaticString s_class_name;
    virtual const String& o_getClassNameHook() const { return s_class_name; }
    virtual bool isResource() const { return (bool)m_res; }

    void close();

    PQ::Result& get() { return m_res; }

    int getFieldNumber(CVarRef field);
    int getNumFields();
    int getNumRows();

    bool convertFieldRow(CVarRef row, CVarRef field,
        int *out_row, int *out_field, const char *fn_name = nullptr);

    Variant fieldIsNull(CVarRef row, CVarRef field, const char *fn_name = nullptr);

    Variant getFieldVal(CVarRef row, CVarRef field, const char *fn_name = nullptr);
    String getFieldVal(int row, int field, const char *fn_name = nullptr);

    PGSQL * getConn() { return m_conn; }

public:
    int m_current_row;
private:
    PQ::Result m_res;
    int m_num_fields;
    int m_num_rows;
    PGSQL * m_conn;
};
}

//////////////////////////////////////////////////////////////////////////////////

StaticString PGSQL::s_class_name("pgsql connection");
StaticString PGSQLResult::s_class_name("pgsql result");

PGSQL *PGSQL::Get(CVarRef conn_id) {
    if (conn_id.isNull()) {
        return nullptr;
    }

    PGSQL *pgsql = conn_id.toResource().getTyped<PGSQL>
        (!RuntimeOption::ThrowBadTypeExceptions,
         !RuntimeOption::ThrowBadTypeExceptions);
    return pgsql;
}

static void notice_processor(PGSQL *pgsql, const char *message) {
    if (pgsql != nullptr) {
        pgsql->m_last_notice = message;

        if (PGSQL::LogNotice) {
            raise_notice("%s", message);
        }
    }
}

PGSQL::PGSQL(String conninfo)
    : m_conn(conninfo.data()), m_conn_string(conninfo.data()), m_last_notice("") {

    if (RuntimeOption::EnableStats && RuntimeOption::EnableSQLStats) {
        ServerStats::Log("sql.conn", 1);
    }

    ConnStatusType st = m_conn.status();
    if (m_conn && st == CONNECTION_OK) {
        // Load up the fixed information
        m_db = m_conn.db();
        m_user = m_conn.user();
        m_pass = m_conn.pass();
        m_host = m_conn.host();
        m_port = m_conn.port();
        m_options = m_conn.options();

        if (!PGSQL::IgnoreNotice) {
            m_conn.setNoticeProcessor(notice_processor, this);
        } else {
            m_conn.setNoticeProcessor<PGSQL>(notice_processor, nullptr);
        }
    } else if (st == CONNECTION_BAD) {
        m_conn.finish();
    }

}

PGSQL::~PGSQL() { m_conn.finish(); }
void PGSQL::sweep() { m_conn.finish(); }

PGSQLResult *PGSQLResult::Get(CVarRef result) {
    if (result.isNull()) {
        return nullptr;
    }

    auto *res = result.toResource().getTyped<PGSQLResult>
        (!RuntimeOption::ThrowBadTypeExceptions,
         !RuntimeOption::ThrowBadTypeExceptions);
    return res;
}

PGSQLResult::PGSQLResult(PGSQL * conn, PQ::Result res)
    : m_current_row(0), m_res(std::move(res)),
      m_num_fields(-1), m_num_rows(-1), m_conn(conn) {
    m_conn->incRefCount();
}

void PGSQLResult::close() {
    m_res.clear();
}

PGSQLResult::~PGSQLResult() {
    m_conn->decRefCount();
    close();
}

void PGSQLResult::sweep() {
	close();
}

int PGSQLResult::getFieldNumber(CVarRef field) {
    int n;
    if (field.isNumeric(true)) {
        n = field.toInt32();
    } else if (field.isString()){
        n = m_res.fieldNumber(field.asCStrRef().data());
    } else {
        n = -1;
    }

    return n;
}

int PGSQLResult::getNumFields() {
    if (m_num_fields == -1) {
        m_num_fields = m_res.numFields();
    }
    return m_num_fields;
}

int PGSQLResult::getNumRows() {
    if (m_num_rows == -1) {
        m_num_rows = m_res.numTuples();
    }
    return m_num_rows;
}

bool PGSQLResult::convertFieldRow(CVarRef row, CVarRef field,
        int *out_row, int *out_field, const char *fn_name) {

    Variant actual_field;
    int actual_row;

    assert(out_row && out_field && "Output parameters cannot be null");

    if (fn_name == nullptr) {
        fn_name = "__internal_pgsql_func";
    }

    if (field.isInitialized()) {
        actual_row = row.toInt64();
        actual_field = field;
    } else {
        actual_row = m_current_row;
        actual_field = row;
    }

    int field_number = getFieldNumber(actual_field);

    if (field_number < 0 || field_number >= getNumFields()) {
        if (actual_field.isString()) {
            raise_warning("%s(): Unknown column name \"%s\"",
                    fn_name, actual_field.asCStrRef().data());
        } else {
            raise_warning("%s(): Column offset `%d` out of range", fn_name, field_number);
        }
        return false;
    }

    if (actual_row < 0 || actual_row >= getNumRows()) {
        raise_warning("%s(): Row `%d` out of range", fn_name, actual_row);
        return false;
    }

    *out_row = actual_row;
    *out_field = field_number;

    return true;
}

Variant PGSQLResult::fieldIsNull(CVarRef row, CVarRef field, const char *fn_name) {
    int r, f;
    if (convertFieldRow(row, field, &r, &f, fn_name)) {
        return m_res.fieldIsNull(r, f);
    }

    return false;
}

Variant PGSQLResult::getFieldVal(CVarRef row, CVarRef field, const char *fn_name) {
    int r, f;
    if (convertFieldRow(row, field, &r, &f, fn_name)) {
        return getFieldVal(r, f, fn_name);
    }

    return false;
}

String PGSQLResult::getFieldVal(int row, int field, const char *fn_name) {
    if (m_res.fieldIsNull(row, field)) {
        return null_string;
    } else {
        char * value = m_res.getValue(row, field);
        int length = m_res.getLength(row, field);

        return String(value, length, CopyString);
    }
}

// Simple RAII helper to convert a CArrRef to a
// list of C strings to pass to pgsql functions. Needs
// to be like this because string conversion may-or-may
// not allocate and therefore needs to ensure that the
// underlying data lasts long enough.
struct CStringArray {
    std::vector<String> m_strings;
    std::vector<const char *> m_c_strs;

public:
    CStringArray(CArrRef arr) {
        int size = arr.size();

        m_strings.reserve(size);

        for(int i = 0; i < size; i++) {
            const Variant &param = arr[i];
            if (param.isNull()) {
                m_strings.push_back(null_string);
            } else {
                m_strings.push_back(param.toString());
            }
        }

        m_c_strs.reserve(size);
        for (int i = 0; i < size; i++) {
            if (m_strings[i].isNull()) {
                m_c_strs.push_back(nullptr);
            } else {
                m_c_strs.push_back(m_strings[i].data());
            }
        }
    }

    const char * const *data() {
        return m_c_strs.data();
    }

};

//////////////////// Connection functions /////////////////////////

static Variant HHVM_FUNCTION(pg_connect, const String& connection_string, int connect_type /* = 0 */) {
    PGSQL * pgsql = nullptr;

    pgsql = NEWOBJ(PGSQL)(connection_string);

    if (!pgsql->get()) {
        delete pgsql;
        return false;
    }
    return Resource(pgsql);
}

static bool HHVM_FUNCTION(pg_close, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql) {
        pgsql->get().finish();
        return true;
    } else {
        return false;
    }
}

static bool HHVM_FUNCTION(pg_ping, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (!pgsql->get()) {
        return false;
    }

    PGPing response = PQping(pgsql->m_conn_string.data());

    if (response == PQPING_OK) {
        if (pgsql->get().status() == CONNECTION_BAD) {
            pgsql->get().reset();
            return pgsql->get().status() != CONNECTION_BAD;
        } else {
            return true;
        }
    }

    return false;
}

static bool HHVM_FUNCTION(pg_connection_reset, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (!pgsql->get()) {
        return false;
    }

    pgsql->get().reset();

    return pgsql->get().status() != CONNECTION_BAD;
}

///////////// Interrogation Functions ////////////////////

static int64_t HHVM_FUNCTION(pg_connection_status, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) return CONNECTION_BAD;
    return (int64_t)pgsql->get().status();
}

static bool HHVM_FUNCTION(pg_connection_busy, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return false;
    }

    auto blocking = pgsql->asNonBlocking();

    pgsql->get().consumeInput();
    return pgsql->get().isBusy();
}

static Variant HHVM_FUNCTION(pg_dbname, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (pgsql == nullptr) {
        return false;
    }

    return pgsql->m_db;
}

static Variant HHVM_FUNCTION(pg_host, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (pgsql == nullptr) {
        return false;
    }

    return pgsql->m_host;
}

static Variant HHVM_FUNCTION(pg_port, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (pgsql == nullptr) {
        return false;
    }

    String ret = pgsql->m_port;
    if (ret.isNumeric()) {
        return ret.toInt32();
    } else {
        return ret;
    }
}

static Variant HHVM_FUNCTION(pg_options, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (pgsql == nullptr) {
        return false;
    }

    return pgsql->m_options;
}

static Variant HHVM_FUNCTION(pg_parameter_status, CResRef connection, const String& param_name) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (pgsql == nullptr) {
        return false;
    }

    String ret(pgsql->get().parameterStatus(param_name.data()), CopyString);

    return ret;
}

static Variant HHVM_FUNCTION(pg_client_encoding, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (pgsql == nullptr) {
        return false;
    }

    String ret(pgsql->get().clientEncoding(), CopyString);

    return ret;
}

static int64_t HHVM_FUNCTION(pg_transaction_status, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);

    if (pgsql == nullptr) {
        return PQTRANS_UNKNOWN;
    }

    return (int64_t)pgsql->get().transactionStatus();
}

static Variant HHVM_FUNCTION(pg_last_error, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return false;
    }

    String ret(pgsql->get().errorMessage(), CopyString);

    return f_trim(ret);
}

static Variant HHVM_FUNCTION(pg_last_notice, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return false;
    }

    return pgsql->m_last_notice;
}

static Variant HHVM_FUNCTION(pg_version, CResRef connection) {
    static StaticString client_key("client");
    static StaticString protocol_key("protocol");
    static StaticString server_key("server");

    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return false;
    }

    Array ret;

    int proto_ver = pgsql->get().protocolVersion();
    if (proto_ver) {
        ret.set(protocol_key, String(proto_ver) + ".0");
    }

    int server_ver = pgsql->get().serverVersion();
    if (server_ver) {
        int revision = server_ver % 100;
        int minor = (server_ver / 100) % 100;
        int major = server_ver / 10000;

        ret.set(server_key, String(major) + "." + String(minor) + "." + String(revision));
    }

    int client_ver = PQlibVersion();
    if (client_ver) {
        int revision = client_ver % 100;
        int minor = (client_ver / 100) % 100;
        int major = client_ver / 10000;

        ret.set(client_key, String(major) + "." + String(minor) + "." + String(revision));
    }

    return ret;
}

static int64_t HHVM_FUNCTION(pg_get_pid, CResRef connection) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return -1;
    }

    return (int64_t)pgsql->get().backendPID();
}

//////////////// Escaping Functions ///////////////////////////

static String HHVM_FUNCTION(pg_escape_bytea, CResRef connection, const String& data) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return null_string;
    }

    std::string escaped = pgsql->get().escapeByteA(data.data(), data.size());

    if (escaped.empty()) {
        raise_warning("pg_escape_bytea(): %s", pgsql->get().errorMessage());
        return null_string;
    }

    String ret(escaped);

    return ret;
}

static String HHVM_FUNCTION(pg_escape_identifier, CResRef connection, const String& data) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return null_string;
    }

    std::string escaped = pgsql->get().escapeIdentifier(data.data(), data.size());

    if (escaped.empty()) {
        raise_warning("pg_escape_identifier(): %s", pgsql->get().errorMessage());
        return null_string;
    }

    String ret(escaped);

    return ret;
}

static String HHVM_FUNCTION(pg_escape_literal, CResRef connection, const String& data) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return null_string;
    }

    std::string escaped = pgsql->get().escapeLiteral(data.data(), data.size());

    if (escaped.empty()) {
        raise_warning("pg_escape_literal(): %s", pgsql->get().errorMessage());
        return null_string;
    }

    String ret(escaped);

    return ret;
}

static String HHVM_FUNCTION(pg_escape_string, CResRef connection, const String& data) {
    PGSQL * pgsql = PGSQL::Get(connection);
    if (pgsql == nullptr) {
        return null_string;
    }

    String ret((data.size()*2)+1, ReserveString); // Reserve enough space as defined by PQescapeStringConn

    int error = 0;
    size_t size = pgsql->get().escapeString(
            ret.get()->mutableData(), data.data(), data.size(),
            &error);

    if (error) {
        return null_string;
    }

    ret.shrink(size); // Shrink to the returned size, `shrink` may re-alloc and free up space

    return ret;
}

static String HHVM_FUNCTION(pg_unescape_bytea, const String& data) {
    size_t to_len = 0;
    char * unesc = (char *)PQunescapeBytea((unsigned char *)data.data(), &to_len);
    String ret = String(unesc, to_len, CopyString);
    PQfreemem(unesc);
    return ret;
}

///////////// Command Execution / Querying /////////////////////////////

static int64_t HHVM_FUNCTION(pg_affected_rows, CResRef result) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) return 0;

    return (int64_t)res->get().cmdTuples();
}

static Variant HHVM_FUNCTION(pg_result_status, CResRef result, int64_t type /* = PGSQL_STATUS_LONG */) {
    PGSQLResult *res = PGSQLResult::Get(result);

    if (type == PGSQL_STATUS_LONG) {
        if (res == nullptr) return 0;

        return (int64_t)res->get().status();
    } else {
        if (res == nullptr) return null_string;

        String ret(res->get().cmdStatus(), CopyString);
        return ret;
    }
}

static bool HHVM_FUNCTION(pg_free_result, CResRef result) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res) {
        res->close();
        return true;
    } else {
        return false;
    }
}

static bool _handle_query_result(const char *fn_name, PQ::Connection &conn, PQ::Result &result) {
    if (!result) {
        const char * err = conn.errorMessage();
        raise_warning("%s(): Query Failed: %s", fn_name, err);
        return true;
    } else {
        int st = result.status();
        switch (st) {
            default:
                break;
            case PGRES_EMPTY_QUERY:
            case PGRES_BAD_RESPONSE:
            case PGRES_NONFATAL_ERROR:
            case PGRES_FATAL_ERROR:
                const char * msg = result.errorMessage();
                raise_warning("%s(): Query Failed: %s", fn_name, msg);
                return true;
        }
        return false;
    }

}

static Variant HHVM_FUNCTION(pg_query, CResRef connection, const String& query) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    PQ::Result res = conn->get().exec(query.data());

    if (_handle_query_result("pg_query", conn->get(), res))
        return false;

    PGSQLResult *pgresult = NEWOBJ(PGSQLResult)(conn, std::move(res));

    return Resource(pgresult);
}

static Variant HHVM_FUNCTION(pg_query_params, CResRef connection, const String& query, CArrRef params) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    CStringArray str_array(params);

    PQ::Result res = conn->get().exec(query.data(), params.size(), str_array.data());

    if (_handle_query_result("pg_query_params", conn->get(), res))
        return false;

    PGSQLResult *pgresult = NEWOBJ(PGSQLResult)(conn, std::move(res));

    return Resource(pgresult);
}

static Variant HHVM_FUNCTION(pg_prepare, CResRef connection, const String& stmtname, const String& query) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    PQ::Result res = conn->get().prepare(stmtname.data(), query.data(), 0);

    if (_handle_query_result("pg_prepare", conn->get(), res))
        return false;

    PGSQLResult *pgres = NEWOBJ(PGSQLResult)(conn, std::move(res));

    return Resource(pgres);
}

static Variant HHVM_FUNCTION(pg_execute, CResRef connection, const String& stmtname, CArrRef params) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    CStringArray str_array(params);

    PQ::Result res = conn->get().execPrepared(stmtname.data(), params.size(), str_array.data());

    PGSQLResult *pgres = NEWOBJ(PGSQLResult)(conn, std::move(res));

    return Resource(pgres);
}

static bool HHVM_FUNCTION(pg_send_query, CResRef connection, const String& query) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    auto nb = conn->asNonBlocking();

    bool empty = true;
    PQ::Result res = conn->get().result();
    while (res) {
        res.clear();
        empty = false;
        res = conn->get().result();
    }
    if (!empty) {
        raise_notice("There are results on this connection."
                     " Call pg_get_result() until it returns FALSE");
    }

    if (!conn->get().sendQuery(query.data())) {
        // TODO: Do persistent auto-reconnect
        return false;
    }

    int ret;
    while ((ret = conn->get().flush())) {
        if (ret == -1) {
            raise_notice("Could not empty PostgreSQL send buffer");
            break;
        }
        usleep(5000);
    }

    return true;
}

static Variant HHVM_FUNCTION(pg_get_result, CResRef connection) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    PQ::Result res = conn->get().result();

    if (!res) {
        return false;
    }

    PGSQLResult *pgresult = NEWOBJ(PGSQLResult)(conn, std::move(res));

    return Resource(pgresult);
}

static bool HHVM_FUNCTION(pg_send_query_params, CResRef connection, const String& query, CArrRef params) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    auto nb = conn->asNonBlocking();

    bool empty = true;
    PQ::Result res = conn->get().result();
    while (res) {
        res.clear();
        empty = false;
        res = conn->get().result();
    }
    if (!empty) {
        raise_notice("There are results on this connection."
                     " Call pg_get_result() until it returns FALSE");
    }

    CStringArray str_array(params);

    if (!conn->get().sendQuery(query.data(), params.size(), str_array.data())) {
        return false;
    }

    int ret;
    while ((ret = conn->get().flush())) {
        if (ret == -1) {
            raise_notice("Could not empty PostgreSQL send buffer");
            break;
        }
        usleep(5000);
    }

    return true;
}

static bool HHVM_FUNCTION(pg_send_prepare, CResRef connection, const String& stmtname, const String& query) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    return conn->get().sendPrepare(stmtname.data(), query.data(), 0);
}

static bool HHVM_FUNCTION(pg_send_execute, CResRef connection, const String& stmtname, CArrRef params) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    CStringArray str_array(params);

    return conn->get().sendQueryPrepared(stmtname.data(),
            params.size(), str_array.data());
}

static bool HHVM_FUNCTION(pg_cancel_query, CResRef connection) {
    PGSQL *conn = PGSQL::Get(connection);
    if (conn == nullptr) {
        return false;
    }

    auto nb = conn->asNonBlocking();

    bool ret = conn->get().cancelRequest();

    PQ::Result res = conn->get().result();
    while(res) {
        res.clear();
        res = conn->get().result();
    }

    return ret;
}

////////////////////////

static Variant HHVM_FUNCTION(pg_fetch_all_columns, CResRef result, int64_t column /* = 0 */) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    if (column < 0 || column >= res->getNumFields()) {
        raise_warning("pg_fetch_all_columns(): Column offset `%d` out of range", (int)column);
        return false;
    }

    int num_rows = res->getNumRows();

    Array arr;
    for (int i = 0; i < num_rows; i++) {
        Variant field = res->getFieldVal(i, column);
        arr.set(i, field);
    }

    return arr;
}

static Variant HHVM_FUNCTION(pg_fetch_array, CResRef result, CVarRef row /* = null_variant */, int64_t result_type /* = PGSQL_BOTH */) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    int r;
    if (row.isNull()) {
        r = res->m_current_row;
        if (r >= res->getNumRows()) {
            return false;
        }
        res->m_current_row++;
    } else {
        r = row.toInt32();
    }

    if (r < 0 || r >= res->getNumRows()) {
        raise_warning("Row `%d` out of range", r);
        return false;
    }

    Array arr;

    for (int i = 0; i < res->getNumFields(); i++) {
        Variant field = res->getFieldVal(r, i);
        if (result_type & PGSQL_NUM) {
            arr.set(i, field);
        }
        if (result_type & PGSQL_ASSOC) {
            char * name = res->get().fieldName(i);
            String fn(name, CopyString);
            arr.set(fn, field);
        }
    }

    return arr;
}

static Variant HHVM_FUNCTION(pg_fetch_assoc, CResRef result, CVarRef row /* = null_variant */) {
    return f_pg_fetch_array(result, row, PGSQL_ASSOC);
}

static Variant HHVM_FUNCTION(pg_fetch_all, CResRef result) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    int num_rows = res->getNumRows();

    Array rows;
    for (int i = 0; i < num_rows; i++) {
        Variant row = f_pg_fetch_assoc(result, i);
        rows.set(i, row);
    }

    return rows;
}

static Variant HHVM_FUNCTION(pg_fetch_result, CResRef result, CVarRef row /* = null_variant */, CVarRef field /* = null_variant */) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    return res->getFieldVal(row, field, "pg_fetch_result");
}

static Variant HHVM_FUNCTION(pg_fetch_row, CResRef result, CVarRef row /* = null_variant */) {
    return f_pg_fetch_array(result, row, PGSQL_NUM);
}

///////////////////// Field information //////////////////////////

static Variant HHVM_FUNCTION(pg_field_is_null, CResRef result, CVarRef row, CVarRef field /* = null_variant */) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    return res->fieldIsNull(row, field, "pg_field_is_null");
}

static Variant HHVM_FUNCTION(pg_field_name, CResRef result, int64_t field_number) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return null_variant;
    }

    if (field_number < 0 || field_number >= res->getNumFields()) {
        raise_warning("pg_field_name(): Column offset `%d` out of range", (int)field_number);
        return false;
    }

    char * name = res->get().fieldName((int)field_number);
    if (name == nullptr) {
        raise_warning("pg_field_name(): %s", res->get().errorMessage());
        return false;
    } else {
        return String(name, CopyString);
    }
}

static int64_t HHVM_FUNCTION(pg_field_num, CResRef result, const String& field_name) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return -1;
    }

    return res->get().fieldNumber(field_name.data());
}

static Variant HHVM_FUNCTION(pg_field_prtlen, CResRef result, CVarRef row_number, CVarRef field /* = null_variant */) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    int r, f;
    if (res->convertFieldRow(row_number, field, &r, &f, "pg_field_prtlen")) {
        return res->get().getLength(r, f);
    }
    return false;
}

static Variant HHVM_FUNCTION(pg_field_size, CResRef result, int64_t field_number) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    if (field_number < 0 || field_number > res->getNumFields()) {
        raise_warning("pg_field_size(): Column offset `%d` out of range", (int)field_number);
        return false;
    }

    return res->get().size(field_number);
}

static Variant HHVM_FUNCTION(pg_field_table, CResRef result, int64_t field_number, bool oid_only /* = false */) {
    PGSQLResult *res = PGSQLResult::Get(result);

    if (res == nullptr) {
        return false;
    }

    if (field_number < 0 || field_number > res->getNumFields()) {
        raise_warning("pg_field_table(): Column offset `%d` out of range", (int)field_number);
        return false;
    }

    Oid id = res->get().table(field_number);
    if (id == InvalidOid) return false;

    if (oid_only) {
        return (int64_t)id;
    } else {
        // TODO: cache the Oids somewhere
        std::ostringstream query;
        query << "SELECT relname FROM pg_class WHERE oid=" << id;

        PQ::Result name_res = res->getConn()->get().exec(query.str().c_str());
        if (!name_res)
            return false;

        if (name_res.status() != PGRES_TUPLES_OK)
            return false;

        char * name = name_res.getValue(0, 0);
        if (name == nullptr) {
            return false;
        }

        String ret(name, CopyString);

        return ret;
    }
}

static Variant HHVM_FUNCTION(pg_field_type_oid, CResRef result, int64_t field_number) {
    PGSQLResult *res = PGSQLResult::Get(result);

    if (res == nullptr) {
        return false;
    }

    if (field_number < 0 || field_number > res->getNumFields()) {
        raise_warning("pg_field_table(): Column offset `%d` out of range", (int)field_number);
        return false;
    }

    Oid id = res->get().type(field_number);
    if (id == InvalidOid) return false;
    return (int64_t)id;
}

// TODO: Cache the results of this function somewhere
static Variant HHVM_FUNCTION(pg_field_type, CResRef result, int64_t field_number) {
    PGSQLResult *res = PGSQLResult::Get(result);

    if (res == nullptr) {
        return false;
    }

    if (field_number < 0 || field_number > res->getNumFields()) {
        raise_warning("pg_field_type(): Column offset `%d` out of range", (int)field_number);
        return false;
    }

    Oid id = res->get().type(field_number);
    if (id == InvalidOid) return false;

    // This should really get all of the rows in pg_type and build a map
    std::ostringstream query;
        query << "SELECT typname FROM pg_type WHERE oid=" << id;

    PQ::Result name_res = res->getConn()->get().exec(query.str().c_str());
    if (!name_res)
        return false;

    if (name_res.status() != PGRES_TUPLES_OK)
        return false;

    char * name = name_res.getValue(0, 0);
    if (name == nullptr)
        return false;

    String ret(name, CopyString);

    return ret;
}

static int64_t HHVM_FUNCTION(pg_num_fields, CResRef result) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return -1;
    }

    return res->getNumFields();
}

static int64_t HHVM_FUNCTION(pg_num_rows, CResRef result) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return -1;
    }

    return res->getNumRows();
}

static Variant HHVM_FUNCTION(pg_result_error_field, CResRef result, int64_t fieldcode) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    char * msg = res->get().errorField(fieldcode);
    if (msg) {
        return f_trim(String(msg, CopyString));
    }

    return false;
}

static Variant HHVM_FUNCTION(pg_result_error, CResRef result) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    const char * msg = res->get().errorMessage();
    if (msg) {
        return f_trim(String(msg, CopyString));
    }

    return false;
}

static bool HHVM_FUNCTION(pg_result_seek, CResRef result, int64_t offset) {
    PGSQLResult *res = PGSQLResult::Get(result);
    if (res == nullptr) {
        return false;
    }

    if (offset < 0 || offset > res->getNumRows()) {
        raise_warning("pg_result_seek(): Cannot seek to row %d", (int)offset);
        return false;
    }

    res->m_current_row = (int)offset;
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool PGSQL::AllowPersistent     = true;
int  PGSQL::MaxPersistent       = -1;
int  PGSQL::MaxLinks            = -1;
bool PGSQL::AutoResetPersistent = false;
bool PGSQL::IgnoreNotice        = false;
bool PGSQL::LogNotice           = false;

namespace { // Anonymous Namespace
static class pgsqlExtension : public Extension {
public:
    pgsqlExtension() : Extension("pgsql") {}

    virtual void moduleLoad(Hdf hdf)
    {
        Hdf pgsql = hdf["PGSQL"];

        PGSQL::AllowPersistent       = pgsql["AllowPersistent"].getBool(true);
        PGSQL::MaxPersistent         = pgsql["MaxPersistent"].getInt32(-1);
        PGSQL::MaxLinks              = pgsql["MaxLinks"].getInt32(-1);
        PGSQL::AutoResetPersistent   = pgsql["AutoResetPersistent"].getBool();
        PGSQL::IgnoreNotice          = pgsql["IgnoreNotice"].getBool();
        PGSQL::LogNotice             = pgsql["LogNotice"].getBool();

    }

    virtual void moduleInit() {

        HHVM_FE(pg_affected_rows);
        HHVM_FE(pg_cancel_query);
        HHVM_FE(pg_client_encoding);
        HHVM_FE(pg_close);
        HHVM_FE(pg_connect);
        HHVM_FE(pg_connection_busy);
        HHVM_FE(pg_connection_reset);
        HHVM_FE(pg_connection_status);
        HHVM_FE(pg_dbname);
        HHVM_FE(pg_escape_bytea);
        HHVM_FE(pg_escape_identifier);
        HHVM_FE(pg_escape_literal);
        HHVM_FE(pg_escape_string);
        HHVM_FE(pg_execute);
        HHVM_FE(pg_fetch_all_columns);
        HHVM_FE(pg_fetch_all);
        HHVM_FE(pg_fetch_array);
        HHVM_FE(pg_fetch_assoc);
        HHVM_FE(pg_fetch_result);
        HHVM_FE(pg_fetch_row);
        HHVM_FE(pg_field_is_null);
        HHVM_FE(pg_field_name);
        HHVM_FE(pg_field_num);
        HHVM_FE(pg_field_prtlen);
        HHVM_FE(pg_field_size);
        HHVM_FE(pg_field_table);
        HHVM_FE(pg_field_type_oid);
        HHVM_FE(pg_field_type);
        HHVM_FE(pg_free_result);
        HHVM_FE(pg_get_pid);
        HHVM_FE(pg_get_result);
        HHVM_FE(pg_host);
        HHVM_FE(pg_last_error);
        HHVM_FE(pg_last_notice);
        HHVM_FE(pg_num_fields);
        HHVM_FE(pg_num_rows);
        HHVM_FE(pg_options);
        HHVM_FE(pg_parameter_status);
        HHVM_FE(pg_ping);
        HHVM_FE(pg_port);
        HHVM_FE(pg_prepare);
        HHVM_FE(pg_query_params);
        HHVM_FE(pg_query);
        HHVM_FE(pg_result_error_field);
        HHVM_FE(pg_result_error);
        HHVM_FE(pg_result_seek);
        HHVM_FE(pg_result_status);
        HHVM_FE(pg_send_execute);
        HHVM_FE(pg_send_prepare);
        HHVM_FE(pg_send_query_params);
        HHVM_FE(pg_send_query);
        HHVM_FE(pg_transaction_status);
        HHVM_FE(pg_unescape_bytea);
        HHVM_FE(pg_version);

#define C(name, value) Native::registerConstant<KindOfInt64>(makeStaticString("PGSQL_" #name), (value))
        // Register constants

        C(ASSOC, PGSQL_ASSOC);
        C(NUM, PGSQL_NUM);
        C(BOTH, PGSQL_BOTH);

        C(CONNECT_FORCE_NEW, 1);
        C(CONNECTION_BAD, CONNECTION_BAD);
        C(CONNECTION_OK, CONNECTION_OK);
        C(CONNECTION_STARTED, CONNECTION_STARTED);
        C(CONNECTION_MADE, CONNECTION_MADE);
        C(CONNECTION_AWAITING_RESPONSE, CONNECTION_AWAITING_RESPONSE);
        C(CONNECTION_AUTH_OK, CONNECTION_AUTH_OK);
        C(CONNECTION_SETENV, CONNECTION_SETENV);
        C(CONNECTION_SSL_STARTUP, CONNECTION_SSL_STARTUP);

        C(SEEK_SET, SEEK_SET);
        C(SEEK_CUR, SEEK_CUR);
        C(SEEK_END, SEEK_END);

        C(EMPTY_QUERY, PGRES_EMPTY_QUERY);
        C(COMMAND_OK, PGRES_COMMAND_OK);
        C(TUPLES_OK, PGRES_TUPLES_OK);
        C(COPY_OUT, PGRES_COPY_OUT);
        C(COPY_IN, PGRES_COPY_IN);
        C(BAD_RESPONSE, PGRES_BAD_RESPONSE);
        C(NONFATAL_ERROR, PGRES_NONFATAL_ERROR);
        C(FATAL_ERROR, PGRES_FATAL_ERROR);

        C(TRANSACTION_IDLE, PQTRANS_IDLE);
        C(TRANSACTION_ACTIVE, PQTRANS_ACTIVE);
        C(TRANSACTION_INTRANS, PQTRANS_INTRANS);
        C(TRANSACTION_INERROR, PQTRANS_INERROR);
        C(TRANSACTION_UNKNOWN, PQTRANS_UNKNOWN);

        C(DIAG_SEVERITY, PG_DIAG_SEVERITY);
        C(DIAG_SQLSTATE, PG_DIAG_SQLSTATE);
        C(DIAG_MESSAGE_PRIMARY, PG_DIAG_MESSAGE_PRIMARY);
        C(DIAG_MESSAGE_DETAIL, PG_DIAG_MESSAGE_DETAIL);
        C(DIAG_MESSAGE_HINT, PG_DIAG_MESSAGE_HINT);
        C(DIAG_STATEMENT_POSITION, PG_DIAG_STATEMENT_POSITION);
        C(DIAG_INTERNAL_POSITION, PG_DIAG_INTERNAL_POSITION);
        C(DIAG_INTERNAL_QUERY, PG_DIAG_INTERNAL_QUERY);
        C(DIAG_CONTEXT, PG_DIAG_CONTEXT);
        C(DIAG_SOURCE_FILE, PG_DIAG_SOURCE_FILE);
        C(DIAG_SOURCE_LINE, PG_DIAG_SOURCE_LINE);
        C(DIAG_SOURCE_FUNCTION, PG_DIAG_SOURCE_FUNCTION);

        C(ERRORS_TERSE, PQERRORS_TERSE);
        C(ERRORS_DEFAULT, PQERRORS_DEFAULT);
        C(ERRORS_VERBOSE, PQERRORS_VERBOSE);

        C(STATUS_LONG, PGSQL_STATUS_LONG);
        C(STATUS_STRING, PGSQL_STATUS_STRING);

        C(CONV_IGNORE_DEFAULT, 1);
        C(CONV_FORCE_NULL, 2);
        C(CONV_IGNORE_NOT_NULL, 4);

#undef C
		loadSystemlib();
    }
} s_pgsql_extension;

}

extern "C" Extension *getModule() {
	return &s_pgsql_extension;
}

///////////////////////////////////////////////////////////////////////////////

}
