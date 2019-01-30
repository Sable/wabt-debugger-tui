#ifndef WDB_TUI_SIDE_MENU_H
#define WDB_TUI_SIDE_MENU_H

#include <wdb_tui/display.h>
#include <string>
#include <vector>

namespace wdb {
    class SideMenu : public Display {
    private:
        std::vector<std::string> m_menuItemsText;
        int m_menuHighlightIndex = 0;

        /**
         * Update menu items
         */
        void update();
    public:
        enum MENU_ITEM {
            HOME = 0,
            WAST,
            DEBUG,
            PROFILER,
            Exit,
        };

        /**
         * Construct side menu
         */
        SideMenu();

        /**
         * Listen for user menu selection
         */
        SideMenu::MENU_ITEM listen();
    };
}
#endif
