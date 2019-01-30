#ifndef WDB_TUI_DEBUG_DISPLAY_H
#define WDB_TUI_DEBUG_DISPLAY_H

#include <wdb_tui/display.h>
#include <wdb/wdb_wabt.h>

namespace wdb {
    class DebugDisplay : public Display {
    public:
        /**
         * Construct debug display
         * @param wdbWabt
         * @param options
         */
        DebugDisplay(wdb::WdbWabt* wdbWabt, wdb::WdbExecutor::Options options);

        /**
         * Listen for user input
         */
        void listen();

        /**
         * Generate debug code
         * @param code
         * @return vector of code
         */
        std::vector<std::string> generateDebugCode();
    private:
        wdb::WdbWabt *m_wdbWabt = nullptr;
        wdb::WdbExecutor::Options m_executorOptions;
        wdb::WdbDebuggerExecutor *m_executor = nullptr;
        enum Panel {
            STACK = 0,
            CODE,
            MEMORY,
            COMMAND
        };
        Panel m_focusPanel;

        // Memory variables
        int m_memoByteStart = 0;
        int m_memoIndex = 0;
        const int MEMORY_BYTES_PER_LINE = 16;

        // Command variables
        std::vector<std::vector<char>> m_commandHistory;
        int m_currentCommandIndex = 0;
        int m_commandHistoryScroll = 0;
        std::vector<std::string> m_consoleOutput;
        int m_consoleTopIndex = 0;

        // Code screen variables
        std::vector<wdb::WdbDebuggerExecutor::Instruction> m_instructions;
        std::set<int> m_breakLine;
        int m_codeTopIndex = 0;
        int m_codeHighlightLineIndex = 0;

        // Stack variables
        int m_stackLeftIndex = 0;
        int m_stackHighlightColIndex = 0;

        /**
         * Get memory memory at byte
         * @param byteIndex
         * @return
         */
        std::string getMemoryHex(int byteIndex, int size);

        /**
         * Update stack screen
         */
        void updateStack();

        /**
         * Update code screen
         */
        void updateCode();

        /**
         * Update memory screen
         */
        void updateMemory();

        /**
         * Update command screen
         */
        void updateCommand();

        /**
         * Update all screens
         */
        void update();

        /**
         * Handle user command
         * @param command
         */
        void handleCommand(std::string command);

        /**
         * Reset debugger variables
         */
        void reset();

        /**
         * Output stream handler
         */
        void outputStreamHandler(std::string text);

        /**
         * Error stream handler
         * @param text
         */
        void errorStreamHandler(std::string text);

        /**
         * Add breakpoint
         * @param line
         */
        bool addBreakpoint(int line);

        /**
         * Remove breakpoint
         * @param line
         */
        bool removeBreakpoint(int line);

        /**
         * List all breakpoints
         * @return breakpoints
         */
        std::set<int> getBreakpoints() const { return m_breakLine; }
    };
}

#endif
