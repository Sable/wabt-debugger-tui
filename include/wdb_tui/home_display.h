#ifndef WDB_TUI_HOME_DISPLAY_H
#define WDB_TUI_HOME_DISPLAY_H

#include <wdb_tui/display.h>

namespace wdb {
    class HomeDisplay : public Display {
    private:
        CDKLABEL* m_homeText = nullptr;
    public:
        HomeDisplay();
        ~HomeDisplay();
    };
}
#endif
