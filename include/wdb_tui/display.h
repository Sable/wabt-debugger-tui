#ifndef WDB_TUI_DISPLAY_H
#define WDB_TUI_DISPLAY_H

#include <cdk.h>
#include <vector>
#include <string>

namespace wdb {
    class Display {
    protected:
        CDKSCREEN* m_CDKScreen = nullptr;
        bool m_focus = false;
        enum Highlight {
            HLINE = 0,
            HCOLUMN,
            HLINE_AND_HCOLUMN,
            HCELL,
            HCLEAR
        };
    public:
        /**
         * Construct screen
         * @param numLines
         * @param numCols
         * @param topLeftY
         * @param topLeftX
         */
        Display(int numLines, int numCols, int topLeftY, int topLeftX);

        /**
         * Destroy screen
         */
        virtual ~Display();

        /**
         * Draw display
         */
        void draw();

        /**
         * Draw border
         * @param topLeftY
         * @param topLeftX
         * @param numLines
         * @param numCols
         * @param focus
         */
        void drawBorder(int &topLeftY, int &topLeftX, int &numLines, int &numCols, bool focus, std::string title);

        /**
         * Draw list
         * @param topLeftY
         * @param topLeftX
         * @param numLines
         * @param numCols
         * @param lines
         * @param topIndex
         * @param highlightIndex
         * @param highlight
         * @param followScroll
         */
        void drawList(int topLeftY, int topLeftX, int numLines, int numCols, std::vector<std::string> &lines,
                      int &topIndex, int &highlightIndex, Highlight highlight, bool followScroll);

        /**
         * Draw table
         * @param topLeftY
         * @param topLeftX
         * @param numLines
         * @param numCols
         * @param header
         * @param data
         * @param attributes
         * @param visibleAttributes
         * @param topIndex
         * @param leftIndex
         * @param highlightLineIndex
         * @param highlightColIndex
         * @param highlight
         * @param followScroll
         */
        void drawTable(int topLeftY, int topLeftX, int numLines, int numCols, std::vector<std::string> &header,
                       std::vector<std::vector<std::string>> &data, int attributes, int visibleAttributes,
                       int &topIndex, int &leftIndex, int &highlightLineIndex, int &highlightColIndex,
                       Highlight highlight, bool followScroll);

        /**
         * Draw message
         * @param y
         * @param x
         * @param cols
         * @param color
         * @param attr
         * @param message
         */
        void drawMessage(int y, int x, int cols, short color, attr_t  attr, std::string message);

        /**
         * Draw cursor
         * @param y
         * @param x
         * @param cols
         * @param cursorIndex
         */
        void drawCursor(int y, int x, int cols, int cursorIndex);

        /**
         * Show dialog message
         * @param title
         * @param text
         * @param color
         * @param attr
         */
        void drawDialog(std::string title, std::string text, short color, attr_t attr);

        /**
         * Get CDK Screen
         * @return
         */
        CDKSCREEN* getCDKScreen() const { return m_CDKScreen; }

        /**
         * Check if display is in focus
         * @return true if in focus
         */
        bool inFocus() const { return m_focus; }

        /**
         * Set display focus
         * @param focus
         */
        void setFocus(bool focus) { m_focus = focus; }

        /**
         * Get window number of lines
         * @return number of lines
         */
        int getNumLines();

        /**
         * Get window number of cols
         * @return number of cols
         */
        int getNumCols();
    };
}

#endif
