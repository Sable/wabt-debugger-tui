#ifndef WDB_TUI_WASM_DISPLAY_H
#define WDB_TUI_WASM_DISPLAY_H

#include <wdb_tui/display.h>
#include <wdb/wdb_wabt.h>
#include <string>
#include <vector>

namespace wdb {
    class WastDisplay : public Display {
    private:
        wdb::WdbCodeGen* m_codeGen = nullptr;
        std::vector<std::string> m_wastCode;
        int m_vscroll = 0;
        int m_lineHighlight = 0;
        int m_codeNumLines = 0;
        int m_codeNumCols = 0;

        /**
         * Update screen
         */
        void update();
    public:
        WastDisplay(wdb::WdbWabt* wdbWabt);

        /**
         * Set wast code
         * @param str
         */
        void setWast(std::string str);

        /**
         * Listen for user input
         */
        void listen();
    };
}
#endif
