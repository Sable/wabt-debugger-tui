#include <wdb_tui/side_menu_display.h>
#include <wdb_tui/common.h>
#include <stdexcept>

namespace wdb {
    SideMenu::SideMenu() : Display(SIDE_MENU_LINES, SIDE_MENU_COLS, 0, 0) {

        // Create menu items
        m_menuItemsText.emplace_back("Home");
        m_menuItemsText.emplace_back("Wast");
        m_menuItemsText.emplace_back("Debug");
        m_menuItemsText.emplace_back("Profiler");
        m_menuItemsText.emplace_back("Exit");

        // Enable user keypad in menu window
        keypad(m_CDKScreen->window, TRUE);
    }

    void SideMenu::update() {
        // Erase window
        werase(m_CDKScreen->window);

        // Draw instructions
        std::string messageLine1 = "Press 'q' or 'Esc'";
        std::string messageLine2 = "to return to menu";
        drawMessage(getNumLines()-3, 1, messageLine1.size(), WDB_COLOR_NORMAL, A_ITALIC, messageLine1);
        drawMessage(getNumLines()-2, 1, messageLine2.size(), WDB_COLOR_NORMAL, A_ITALIC, messageLine2);

        // Draw title
        std::string title = "WDB TUI";
        int titleY = 1;
        int titleX = getNumCols()/2 - title.size()/2;
        drawMessage(titleY, titleX, title.size(), WDB_COLOR_NORMAL, A_BOLD, title);

        // Menu offset
        int topLeftY = 3;
        int topLeftX = 1;

        // Menu dimensions
        int numLines = (int)m_menuItemsText.size();
        int numCols = getNumCols() - 2;

        // Draw menu items
        int topIndex = 0;
        drawList(topLeftY, topLeftX, numLines, numCols, m_menuItemsText, topIndex, m_menuHighlightIndex,
                 Highlight::HLINE, true);
    }

    SideMenu::MENU_ITEM SideMenu::listen() {
        while (true) {
            update();
            draw();
            int c = wgetch(m_CDKScreen->window);
            switch (c) {
                case KEY_UP:
                    m_menuHighlightIndex--;
                    break;
                case KEY_DOWN:
                    m_menuHighlightIndex++;
                    break;
                case KEY_ENTER:
                case 10: // Enter
                    return static_cast<MENU_ITEM >(m_menuHighlightIndex);
                default:
                    // Do nothing
                    break;
            }
        }
    }
}