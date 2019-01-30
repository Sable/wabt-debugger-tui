#include <wdb_tui/wast_display.h>
#include <wdb_tui/common.h>
#include <sstream>
#include <vector>

namespace wdb {
    WastDisplay::WastDisplay(wdb::WdbWabt *wdbWabt) : Display(DISPLAYS_LINES, DISPLAYS_COLS, 0, SIDE_MENU_COLS) {
        // Enable keypad
        keypad(m_CDKScreen->window, true);
        // Create a code generator
        m_codeGen = wdbWabt->CreateCodeGenerator();
    }

    void WastDisplay::setWast(std::string str) {
        // Clear old wast code
        m_wastCode.clear();
        // Load string into vector of string
        std::stringstream ss(str);
        std::string line;
        while(std::getline(ss, line, '\n')){
            m_wastCode.push_back(line);
        }
    }

    void WastDisplay::update() {
        // Erase screen
        werase(m_CDKScreen->window);

        // Set wast code
        wabt::WriteWatOptions options;
        options.fold_exprs = true;
        options.inline_export = true;
        options.inline_import = true;
        setWast(m_codeGen->GetWat(options));

        // Update code view configuration
        m_codeNumLines = getNumLines() - 2;
        m_codeNumCols = getNumCols() - 2;

        // Compute list offsets
        int offsetTopLeftY = 1;
        int offsetTopLeftX = 1;

        // Draw list data
        drawList(offsetTopLeftY, offsetTopLeftX, m_codeNumLines, m_codeNumCols, m_wastCode, m_vscroll, m_lineHighlight,
                 Highlight::HLINE, true);
    }

    void WastDisplay::listen() {
        // Listen for keyboard input
        while(true) {
            update();
            draw();
            int c = wgetch(m_CDKScreen->window);
            switch (c) {
                case 'q':
                case KEY_ESC:
                    return; // Quit
                case KEY_UP:
                    m_lineHighlight--;
                    break;
                case KEY_DOWN:
                    m_lineHighlight++;
                    break;
                default:
                    // Do nothing
                    break;
            }
        }

    }
}
