#include <wdb_tui/display.h>
#include <wdb_tui/common.h>
#include <stdexcept>
#include <cmath>

namespace wdb {
    Display::Display(int numLines, int numCols, int topLeftY, int topLeftX) {
        // Initialize CDK Screen
        m_CDKScreen = initCDKScreen(newwin(numLines, numCols, topLeftY, topLeftX));
    }

    Display::~Display() {
        destroyCDKScreen(m_CDKScreen);
    }

    void Display::draw() {
        if(m_focus) {
            wattron(m_CDKScreen->window, A_STANDOUT);
            box(m_CDKScreen->window, 0, 0);
            wattroff(m_CDKScreen->window, A_STANDOUT);
        } else {
            box(m_CDKScreen->window, 0, 0);
        }
        drawCDKScreen(m_CDKScreen);
    }

    void Display::drawBorder(int &topLeftY, int &topLeftX, int &numLines, int &numCols, bool focus, std::string title) {
        if(focus) {
            wattron(m_CDKScreen->window, A_STANDOUT);
        }
        // Draw horizontal lines
        mvwhline(m_CDKScreen->window, topLeftY, topLeftX+1, 0, numCols-2);
        mvwhline(m_CDKScreen->window, topLeftY+numLines-1, topLeftX+1, 0, numCols-2);
        // Draw vertical lines
        mvwvline(m_CDKScreen->window, topLeftY+1, topLeftX, 0, numLines-1);
        mvwvline(m_CDKScreen->window, topLeftY+1, topLeftX+numCols-1, 0, numLines-1);
        // Draw corners
        mvwaddch(m_CDKScreen->window, topLeftY, topLeftX, ACS_ULCORNER);
        mvwaddch(m_CDKScreen->window, topLeftY, topLeftX+numCols-1, ACS_URCORNER);
        mvwaddch(m_CDKScreen->window, topLeftY+numLines-1, topLeftX, ACS_LLCORNER);
        mvwaddch(m_CDKScreen->window, topLeftY+numLines-1, topLeftX+numCols-1, ACS_LRCORNER);
        // Draw title in the center
        int titleX = topLeftX + numCols / 2 - (int)title.size()/2;
        title = " " + title + " ";
        drawMessage(topLeftY, titleX, title.size(), WDB_COLOR_NORMAL, A_BOLD, title);
        if(focus) {
            wattroff(m_CDKScreen->window, A_STANDOUT);
        }
        // Update dimensions
        topLeftY += 1;
        topLeftX += 2;
        numLines -= 2;
        numCols -= 4;
    }

    void Display::drawList(int topLeftY, int topLeftX, int numLines, int numCols, std::vector<std::string> &lines,
                           int &topIndex, int &highlightIndex, wdb::Display::Highlight highlight, bool followScroll) {
        std::vector<std::string> emptyHeader;
        std::vector<std::vector<std::string>> data;
        int leftIndex = 0;
        int highlightCol = 0;
        for(const std::string &entry : lines) {
            std::vector<std::string> row = {entry};
            data.emplace_back(row);
        }
        drawTable(topLeftY, topLeftX, numLines, numCols, emptyHeader, data, 1, 1, topIndex, leftIndex, highlightIndex,
                  highlightCol, highlight, followScroll);
    }

    void Display::drawTable(int topLeftY, int topLeftX, int numLines, int numCols, std::vector<std::string> &header,
                            std::vector<std::vector<std::string>> &data, int attributes, int visibleAttributes,
                            int &topIndex, int &leftIndex, int &highlightLineIndex, int &highlightColIndex,
                            wdb::Display::Highlight highlight, bool followScroll) {
        int centerY = topLeftY + (numLines / 2);
        int centerX = topLeftX + (numCols / 2);
        if (attributes <= 0 || visibleAttributes <= 0) {
            std::string message = "Error displaying data: No attributes to show";
            centerX -= message.size()/2;
            drawMessage(centerY, centerX, message.size(), WDB_COLOR_ERROR, A_BOLD, message);
        } else if (!header.empty() && header.size() != attributes) {
            std::string message = "Error displaying data: Header size conflict";
            centerX -= message.size()/2;
            drawMessage(centerY, centerX, message.size(), WDB_COLOR_ERROR, A_BOLD, message);
        } else {
            // Update horizontal indices
            leftIndex = std::min(leftIndex, attributes-visibleAttributes);
            leftIndex = std::max(leftIndex, 0);
            highlightColIndex = std::min(highlightColIndex, attributes-1);
            highlightColIndex = std::max(highlightColIndex, 0);
            // Update horizontal follow scroll
            if(followScroll) {
                if(leftIndex > highlightColIndex) {
                    leftIndex = highlightColIndex;
                } else if(leftIndex + visibleAttributes <= highlightColIndex) {
                    leftIndex = highlightColIndex - visibleAttributes + 1;
                }
            }
            // Compute one attribute width
            int attributeNumCols = numCols / visibleAttributes;
            // Draw table header
            if (!header.empty()) {
                for (int i = 0; i + leftIndex < header.size() && i < visibleAttributes; i++) {
                    drawMessage(topLeftY, topLeftX + i * attributeNumCols, header[i + leftIndex].size(),
                                WDB_COLOR_NORMAL, A_UNDERLINE | A_BOLD, header[i + leftIndex]);
                }
                topLeftY++;
                numLines--;
            }
            // Update vertical indices
            topIndex = std::min(topIndex, (int)data.size()-numLines);
            topIndex = std::max(topIndex, 0);
            highlightLineIndex = std::min(highlightLineIndex, (int)data.size()-1);
            highlightLineIndex = std::max(highlightLineIndex, 0);
            // Update vertical follow scroll
            if(followScroll) {
                // Vertical scroll follow
                if(topIndex > highlightLineIndex) {
                    topIndex = highlightLineIndex;
                } else if(topIndex + numLines <= highlightLineIndex) {
                    topIndex = highlightLineIndex - numLines + 1;
                }
            }
            // Check if data is empty
            if (data.empty()) {
                // Update highlight indices
                highlightLineIndex = -1;
                highlightColIndex = -1;
                // Draw message
                std::string message = "No data to display";
                centerY = topLeftY + (numLines / 2);
                centerX -= message.size()/2;
                drawMessage(centerY, centerX, message.size(), WDB_COLOR_NORMAL, A_ITALIC, message);
            } else {
                // Draw list data
                for(int i=0; i + topIndex < data.size() && i < numLines; i++) {
                    // Get current row
                    auto &currentRow = data[i + topIndex];
                    // Print each row's attributes
                    for(int j=0; j + leftIndex < currentRow.size() && j < visibleAttributes; j++) {
                        drawMessage(i + topLeftY, topLeftX + j * attributeNumCols, currentRow[j + leftIndex].size(),
                                    WDB_COLOR_NORMAL, A_NORMAL, currentRow[j + leftIndex]);
                    }
                }
                // Highlight cell
                if(highlightLineIndex >= topIndex
                   && highlightLineIndex < topIndex + numLines
                   && highlightColIndex >= leftIndex
                   && highlightColIndex < leftIndex + visibleAttributes
                   && highlight == HCELL) {
                    mvwchgat(m_CDKScreen->window, highlightLineIndex - topIndex + topLeftY,
                             (highlightColIndex - leftIndex) * attributeNumCols + topLeftX, attributeNumCols,
                             A_STANDOUT, 0, nullptr);
                } else {
                    // Highlight row in focus
                    if(highlightLineIndex >= topIndex
                       && highlightLineIndex < topIndex + numLines
                       && (highlight == HLINE || highlight == HLINE_AND_HCOLUMN)) {
                        mvwchgat(m_CDKScreen->window, highlightLineIndex - topIndex + topLeftY,
                                 topLeftX, numCols, A_STANDOUT, 0, nullptr);
                    }
                    // Highlight row in focus
                    if(highlightColIndex >= leftIndex
                       && highlightColIndex < leftIndex + visibleAttributes
                       && (highlight == HCOLUMN || highlight == HLINE_AND_HCOLUMN)) {
                        for(int i=0; i < numLines && topIndex + i < data.size(); i++) {
                            mvwchgat(m_CDKScreen->window, i + topLeftY,
                                     (highlightColIndex - leftIndex) * attributeNumCols + topLeftX, attributeNumCols,
                                     A_STANDOUT, 0, nullptr);
                        }
                    }
                }
            }
        }
    }

    void Display::drawMessage(int y, int x, int cols, short color, attr_t attr, std::string message) {
        // Number of columns must be positive
        if(cols > 0) {
            if(message.size() > cols) {
                // Truncate message
                message = message.substr(0, cols);
            } else if(message.size() < cols) {
                // Clear screen
                message += std::string(cols - message.size(), ' ');
            }
            mvwprintw(m_CDKScreen->window, y, x, "%s", message.c_str());
            mvwchgat(m_CDKScreen->window, y, x, cols, attr, color, nullptr);
        }
    }

    void Display::drawCursor(int y, int x, int cols, int cursorIndex) {
        if(cursorIndex < cols) {
            mvwchgat(m_CDKScreen->window, y, x + cursorIndex, 1, A_STANDOUT, WDB_COLOR_NORMAL, nullptr);
        }
    }

    void Display::drawDialog(std::string title, std::string text, short color, attr_t attr) {
        int numLines = 3;
        int numCols = (int)text.size() + 4;
        int messageY = getNumLines() / 2 - numLines / 2;
        int messageX = getNumCols() / 2 - numCols / 2;
        drawBorder(messageY, messageX, numLines, numCols, false, title);
        drawMessage(messageY, messageX, text.size(), color, attr, text);
    }

    int Display::getNumLines() {
        return getmaxy(m_CDKScreen->window);
    }

    int Display::getNumCols() {
        return getmaxx(m_CDKScreen->window);
    }
}