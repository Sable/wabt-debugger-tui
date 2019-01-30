#include <wdb_tui/home_display.h>
#include <wdb_tui/common.h>

namespace wdb {
    HomeDisplay::HomeDisplay() : Display(LINES, DISPLAYS_COLS, 0, SIDE_MENU_COLS) {

        // Create home screen
        char* homeTextArray[9] = {
                const_cast<char *>("#     # ######  ######"),
                const_cast<char *>("#  #  # #     # #     #"),
                const_cast<char *>("#  #  # #     # #     #"),
                const_cast<char *>("#  #  # #     # ###### "),
                const_cast<char *>("#  #  # #     # #     #"),
                const_cast<char *>("#  #  # #     # #     #"),
                const_cast<char *>(" ## ##  ######  ######"),
                const_cast<char *>("        WDB TUI"),
                const_cast<char *>("     Version 0.0.0")
        };
        m_homeText = newCDKLabel(m_CDKScreen, CENTER, CENTER, homeTextArray, 9, TRUE, TRUE);
    }

    HomeDisplay::~HomeDisplay() {
        destroyCDKLabel(m_homeText);
    }
}
