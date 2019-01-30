#ifndef WDB_TUI_HOST_FUNCTIONS_H
#define WDB_TUI_HOST_FUNCTIONS_H

#include <wdb/wdb_executor.h>

namespace wdb_stubs {
    /**
     * Initialize host functions
     * @param executor
     */
    void InitHostFunctions(wdb::WdbExecutor* executor);
}

#endif
