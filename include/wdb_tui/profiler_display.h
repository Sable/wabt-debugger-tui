#ifndef WDB_TUI_PROFILER_DISPLAY_H
#define WDB_TUI_PROFILER_DISPLAY_H

#include <wdb_tui/display.h>
#include <wdb/wdb_wabt.h>

namespace wdb {
    class ProfilerDisplay : public Display {
    private:
        wdb::WdbProfilerExecutor::Sort m_listSort;
        wdb::WdbProfilerExecutor *m_executor = nullptr;
        wdb::WdbWabt* m_wdbWabt = nullptr;
        wdb::WdbExecutor::Options m_executorOptions;
        enum Panel {
            FUNCTIONS = 0,
            RESULTS
        };
        Panel m_focusPanel;

        // Function list screen
        int m_funcHighlight = 0;
        int m_funcTopIndex = 0;
        int m_funcLeftIndex = 0;
        std::vector<wabt::interp::Export> m_funcList;

        // Data list screen
        int m_dataHighlight = 0;
        int m_dataTopIndex = 0;
        int m_dataLeftIndex = 0;

        /**
         * Update list
         */
        void update();

        /**
         * Update function list
         */
        void updateFuncList();

        /**
         * Update profiler data list
         */
        void updateDataList();

        /**
         * Update status
         * @param color
         * @param message
         * @param pause
         */
        void setStatus(short color, std::string message, bool pause);

        /**
         * Execute a function
         */
        void executeFunction();
    public:
        /**
         * Construct profiler display
         * @param wdbWabt
         * @param wdbWabt
         */
        ProfilerDisplay(wdb::WdbWabt* wdbWabt, wdb::WdbExecutor::Options options);

        /**
         * Listen for user input
         * @return false on exit
         */
        void listen();
    };
}

#endif
