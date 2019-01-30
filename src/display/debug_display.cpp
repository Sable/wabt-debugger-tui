#include <wdb_tui/debug_display.h>
#include <wabt/src/cast.h>
#include <wdb_tui/common.h>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <regex>

namespace wdb {
    DebugDisplay::DebugDisplay(wdb::WdbWabt *wdbWabt, wdb::WdbExecutor::Options options) :
            Display(DISPLAYS_LINES, DISPLAYS_COLS, 0, SIDE_MENU_COLS) {
        // Enable keypad on this window
        keypad(m_CDKScreen->window, true);
        // Set wasm interpreter
        m_wdbWabt = wdbWabt;
        // Set default panel in focus
        m_focusPanel = COMMAND;
        // Init command history with one element
        m_commandHistory = {{}};
        // Configure executor options
        m_executorOptions.preSetup = options.preSetup;
        m_executorOptions.outputStreamHandler = std::bind(&DebugDisplay::outputStreamHandler, this, std::placeholders::_1);
        m_executorOptions.errorStreamHandler = std::bind(&DebugDisplay::errorStreamHandler, this, std::placeholders::_1);
        // Create a default executor
        m_executor = m_wdbWabt->CreateWdbDebuggerExecutor(m_executorOptions);
    }

    std::string DebugDisplay::getMemoryHex(int byteIndex, int size) {
        std::stringstream ssHex;
        std::stringstream ssASCII;
        // Add address
        ssHex << "0x" << std::setfill ('0') << std::setw(sizeof(byteIndex)*2) << std::hex << byteIndex << " ";
        // Add bytes
        for(int i=0; i < size; i++) {
            if(i % 2 == 0) {
                ssHex << " ";
            }
            int currentIndex = i + byteIndex;
            if(currentIndex < m_executor->GetMemorySize(m_memoIndex)) {
                int currentData = m_executor->GetMemoryAt(m_memoIndex, currentIndex) & 0xff;
                ssHex << std::setfill('0') << std::setw(2) << std::hex << currentData;
                // If char is printable
                if(std::isprint(currentData)) {
                    ssASCII << (char) currentData;
                } else {
                    ssASCII << ".";
                }
            } else {
                ssHex << "00";
                ssASCII << ".";
            }
        }
        ssHex << "  " << ssASCII.str();
        return ssHex.str();
    }

    void DebugDisplay::updateStack() {
        // Update position
        int topLeftY = 1;
        int topLeftX = 1;

        // Update dimensions
        int numLines = (getNumLines() / 5);
        int numCols = getNumCols() - (2 * topLeftX);

        // Draw border around stack
        drawBorder(topLeftY, topLeftX, numLines, numCols, m_focusPanel == STACK, "(top) - STACK - (bottom)");

        // Check if stack is empty or no executor
        if(m_executor->GetStackSize() == 0) {
            // Draw stack is empty message
            std::string message = "Stack is empty";
            int messageY = topLeftY + numLines/2;
            int messageX = topLeftX + numCols/2 - (int)message.size()/2;
            drawMessage(messageY, messageX, message.size(), WDB_COLOR_NORMAL, A_ITALIC, message);
        } else {
            // Prepare table data
            size_t valueSize = 16; // 128-bits for the v128 type
            std::vector<std::vector<std::string>> stackData(valueSize/4); // 4-bytes per line
            int valueNumCols = 11;
            int visibleValues = numCols / valueNumCols;
            // Prepare table header
            std::vector<std::string> header;
            for(int i=0; i < m_executor->GetStackSize(); i++) {
                // Create header
                header.push_back(std::to_string(m_executor->GetStackSize() - i));
                // Copy stack value into a byte array
                wabt::interp::Value currentVal = m_executor->GetStackAt(m_executor->GetStackSize() - i - 1);
                uint8_t stackBytes[valueSize];
                memcpy(stackBytes, &currentVal, valueSize);
                for(int j=0; j < stackData.size(); j++) {
                    std::stringstream ss;
                    ss << " "
                       << std::setfill('0') << std::setw(2) << std::hex << (int) stackBytes[j*4+3]
                       << std::setfill('0') << std::setw(2) << std::hex << (int) stackBytes[j*4+2] << " "
                       << std::setfill('0') << std::setw(2) << std::hex << (int) stackBytes[j*4+1]
                       << std::setfill('0') << std::setw(2) << std::hex << (int) stackBytes[j*4];
                    stackData[stackData.size()-1-j].push_back(ss.str());
                }
            }
            // Draw table
            int topIndex = 0;
            int highlightLine = 0;
            drawTable(topLeftY, topLeftX, numLines, numCols, header, stackData, header.size(), visibleValues, topIndex,
                      m_stackLeftIndex, highlightLine, m_stackHighlightColIndex, Highlight::HCOLUMN, true);
        }
    }

    void DebugDisplay::updateCode() {
        // Update position
        int topLeftY = (getNumLines() / 5) + 1;
        int topLeftX = 1;

        // Update dimensions
        int numLines = (int)(getNumLines() / 2.5);
        int numCols = (getNumCols() / 2) - (2 * topLeftX);

        // Draw border around stack
        drawBorder(topLeftY, topLeftX, numLines, numCols, m_focusPanel == CODE, "CODE");
        // Populate code
        m_instructions = m_executor->DisassembleModule(m_executor->GetMainModule());
        std::vector<std::string> code = generateDebugCode();
        // Draw code list
        Display::Highlight highlight = Highlight::HCLEAR;
        bool follow = false;
        if(m_executor->MainFunctionIsSet() && !m_executor->MainFunctionHasReturned()) {
            highlight = Highlight::HLINE;
            follow = true;
        }
        drawList(topLeftY, topLeftX, numLines, numCols, code, m_codeTopIndex, m_codeHighlightLineIndex, highlight,
                 follow);
    }

    void DebugDisplay::updateMemory() {
        // Update position
        int topLeftY = (getNumLines() / 5) + 1;
        int topLeftX = getNumCols() / 2;

        // Update dimensions
        int numLines = (int)(getNumLines() / 2.5);
        int numCols = (getNumCols() / 2) - 1;

        // Draw border around stack
        std::string memoryTitle = "MEMORY";
        if(m_executor->GetMemoriesCount() > 0) {
            memoryTitle += " #" + std::to_string(m_memoIndex);
        }
        drawBorder(topLeftY, topLeftX, numLines, numCols, m_focusPanel == MEMORY, memoryTitle.c_str());
        // Check if at least one memory exit
        if(m_executor->GetMemoriesCount() == 0) {
            // Draw not memory found message
            std::string message = "No memory found";
            int messageY = topLeftY + numLines / 2;
            int messageX = topLeftX + numCols / 2 - (int)message.size()/2;
            drawMessage(messageY, messageX, message.size(), WDB_COLOR_NORMAL, A_ITALIC, message);
        } else {
            // Draw memory
            for(int i=0; i < numLines; i++) {
                std::string memoryHex = getMemoryHex(i * MEMORY_BYTES_PER_LINE + m_memoByteStart, MEMORY_BYTES_PER_LINE);
                drawMessage(topLeftY+i, topLeftX, numCols, WDB_COLOR_NORMAL, A_NORMAL, memoryHex);
            }
        }
    }

    void DebugDisplay::updateCommand() {
        // Update position
        int topLeftY = getNumLines() / 5 + (int)(getNumLines()  /2.5) + 1;
        int topLeftX = 1;

        // Update dimensions
        int numLines = getNumLines() - topLeftY - 2;
        int numCols = getNumCols() - (2 * topLeftX);

        // Draw border
        drawBorder(topLeftY, topLeftX, numLines, numCols, m_focusPanel == COMMAND, "CMD");
        // Draw command line
        drawMessage(topLeftY + numLines-1, topLeftX, numCols, WDB_COLOR_CMD, A_BOLD,
                    "> " + std::string(m_commandHistory[m_commandHistoryScroll].begin(),
                                       m_commandHistory[m_commandHistoryScroll].end()));
        // Draw cursor
        drawCursor(topLeftY + numLines-1, topLeftX+2, numCols-2, m_currentCommandIndex);
        // Draw output
        int highlightTopIndex = 0;
        drawList(topLeftY, topLeftX, numLines-2, numCols, m_consoleOutput, m_consoleTopIndex, highlightTopIndex,
                 Highlight::HCLEAR, false);
    }

    void DebugDisplay::update() {
        // Erase window
        werase(m_CDKScreen->window);
        if(m_executor) {
            // Update panels
            updateStack();
            updateCommand();
            updateCode();
            updateMemory();
            // Draw instructions
            drawMessage(getNumLines()-2, 1, getNumCols()-2, WDB_COLOR_INFO, A_BOLD,
                        "<TAB>Focus <F1>Console-Up <F2>Console-Down <PAGE-UP>Prev-Memo <PAGE-DOWN>Next-Memo");
        } else {
            drawDialog("Error", "Error creating an executor, please verify the wasm file is valid", WDB_COLOR_ERROR,
                       A_BOLD);
        }
    }

    void DebugDisplay::reset() {
        // Reset memory variables
        m_memoByteStart = 0;
        m_memoIndex = 0;

        // Reset code screen variables
        m_codeTopIndex = 0;

        // Reset stack variables
        m_stackLeftIndex = 0;
    }

    void DebugDisplay::handleCommand(std::string command) {
        std::istringstream ssCommand(command);
        std::vector<std::string> commandVector{std::istream_iterator<std::string>{ssCommand},
                                               std::istream_iterator<std::string>{}};
        // Log what was entered
        m_consoleOutput.emplace_back("> "+command);
        if(!commandVector.empty()) {
            std::string commandPart = commandVector.front();
            if(commandPart == "help" && commandVector.size() == 1) {
                m_consoleOutput.emplace_back("Commands:");
                m_consoleOutput.emplace_back("  help                    Display this message");
                m_consoleOutput.emplace_back("  clear                   Clear console");
                m_consoleOutput.emplace_back("  restart                 Debug function");
                m_consoleOutput.emplace_back("  main     <func-name>    Set main function");
                m_consoleOutput.emplace_back("  step                    Step into execution");
                m_consoleOutput.emplace_back("  continue                Continue execution");
                m_consoleOutput.emplace_back("  break    <pc>           Add breakpoint at given line");
                m_consoleOutput.emplace_back("  breakrm  <pc>           Remove breakpoint at given line");
                m_consoleOutput.emplace_back("  breakls                 List all breakpoint lines");
                m_consoleOutput.emplace_back("  print                   Print to the console:");
                m_consoleOutput.emplace_back("    stack[top=0].type     Stack value at an index with a type: i32, i64, f32, f64 or v128");
            } else if(commandPart == "clear" && commandVector.size() == 1) {
                m_consoleOutput.clear();
            } else if(commandPart == "restart" && commandVector.size() == 1) {
                // Reset debugger display
                reset();
                // Create a new executor
                m_executor = m_wdbWabt->CreateWdbDebuggerExecutor(m_executorOptions);
            } else if(commandPart == "main" && commandVector.size() == 2) {
                // Search for function
                wabt::interp::Export* e = nullptr;
                std::string funcName = commandVector[1];
                if(m_executor->SearchExportedModuleFunction(
                        m_executor->GetMainModule(), funcName, &e) == wabt::Result::Ok) {
                    // Get function details
                    auto func = m_executor->GetFunction(e->index);
                    // Set the main function
                    if(m_executor->SetMainFunction(func) == wabt::Result::Ok) {
                        m_consoleOutput.emplace_back("Program main function set to '" + funcName +"'");
                    } else {
                        m_consoleOutput.emplace_back("Failed to set '" + funcName + "' main function");
                    }
                } else {
                    m_consoleOutput.emplace_back("Function '" + funcName + "' was not found");
                }
            } else if(commandPart == "step" && commandVector.size() == 1) {
                if(m_executor->ExecuteNextInstruction() != wabt::Result::Ok) {
                    m_consoleOutput.emplace_back("Cannot execute next instruction");
                }
            } else if(commandPart == "continue" && commandVector.size() == 1) {
                if(m_executor->Execute() != wabt::Result::Ok) {
                    m_consoleOutput.emplace_back("Cannot continue executing instructions");
                }
            } else if(commandPart == "print" && commandVector.size() == 2) {
                // Parse the second argument
                std::regex printArg(R"(^(stack|memo)\[([0-9]{1,5})\]\.(i32|i64|f32|f64|v128)$)");
                std::smatch printArgMatch;
                std::regex_search(commandVector[1], printArgMatch, printArg);

                // If match was found
                if (!printArgMatch.empty()) {
                    // Load matched groups into variables
                    std::string variable = printArgMatch[1];
                    int index = std::stoi(printArgMatch[2]);
                    std::string type = printArgMatch[3];
                    // For stack variable
                    if (variable == "stack") {
                        // Check for index out of bound
                        if (index < 0 || index >= m_executor->GetStackSize()) {
                            m_consoleOutput.emplace_back("Index out of stack bound");
                        } else {
                            // Prepare a typed value for easy printing
                            wabt::interp::TypedValue valType;
                            // Fetch stack entry
                            auto stackEntry =
                                    m_executor->GetStackAt(m_executor->GetStackSize() - index - 1);
                            // Check the type
                            if (type == "i32") {
                                valType.type = wabt::Type::I32;
                            } else if (type == "i64") {
                                valType.type = wabt::Type::I64;
                            } else if (type == "f32") {
                                valType.type = wabt::Type::F32;
                            } else if (type == "f64") {
                                valType.type = wabt::Type::F64;
                            } else if (type == "v128") {
                                valType.type = wabt::Type::V128;
                            }
                            valType.value = stackEntry;
                            m_consoleOutput.emplace_back(wabt::interp::TypedValueToString(valType));
                        }
                    } else if (variable == "memo") {
                        // TODO
                        m_consoleOutput.emplace_back("No yet implemented");
                    }
                } else {
                    m_consoleOutput.emplace_back("Please type 'help' for a list of print commands");
                }
            } else if(commandPart == "break" && commandVector.size() == 2) {
                // Parse the second argument
                std::regex breakArg(R"(^([1-9][0-9]*)$)");
                std::smatch breakArgMatch;
                std::regex_search(commandVector[1], breakArgMatch, breakArg);

                // If match was found
                if (!breakArgMatch.empty()) {
                    int line =  std::stoi(commandVector[1]);
                    if(!addBreakpoint(line)) {
                        m_consoleOutput.emplace_back("Breakpoint line number is out of bound");
                    }
                } else {
                    m_consoleOutput.emplace_back("Error reading the breakpoint offset");
                }
            } else if(commandPart == "breakrm" && commandVector.size() == 2) {
                // Parse the second argument
                std::regex breakArg(R"(^([1-9][0-9]*)$)");
                std::smatch breakArgMatch;
                std::regex_search(commandVector[1], breakArgMatch, breakArg);

                // If match was found
                if (!breakArgMatch.empty()) {
                    int line =  std::stoi(commandVector[1]);
                    if(!removeBreakpoint(line)) {
                        m_consoleOutput.emplace_back("Breakpoint line number is out of bound");
                    }
                } else {
                    m_consoleOutput.emplace_back("Error reading the breakpoint offset");
                }
            } else if(commandPart == "breakls" && commandVector.size() == 1) {
                std::stringstream ss;
                auto breakpoints = getBreakpoints();
                ss << "[";
                for(auto i = breakpoints.begin(); i != breakpoints.end(); i++) {
                    if(i != breakpoints.begin()) {
                        ss << ",";
                    }
                    ss << *i;
                }
                ss << "]";
                m_consoleOutput.emplace_back(ss.str());
            } else {
                m_consoleOutput.emplace_back("Command '" + command + "' not found");
            }
            // Scroll console
            m_consoleTopIndex = INT_MAX;
        }
    }

    std::vector<std::string> DebugDisplay::generateDebugCode() {
        // Clear old debug code
        std::vector<std::string> code;
        if(!m_instructions.empty()) {
            int lineNumSpace = (int) (std::log10(m_instructions.size())+1);
            int lineNum = 1;
            // Load string into vector of string
            for(auto &instruction : m_instructions) {
                bool breakpoint = false;
                // Highlight current line
                if(m_executor->GetPcOffset() == instruction.istream_start) {
                    m_codeHighlightLineIndex = lineNum - 1;
                }
                // Set breakpoint mark
                if(m_breakLine.find(lineNum) != m_breakLine.end()) {
                    breakpoint = true;
                }
                // Generate line of code
                std::stringstream ss;
                if(breakpoint) {
                    ss << ">";
                } else {
                    ss << " ";
                }
                ss << std::setfill (' ') << std::setw(lineNumSpace) << lineNum++ << "  " << instruction.str;
                code.push_back(ss.str());
            }
        }
        return code;
    }

    bool DebugDisplay::addBreakpoint(int line) {
        if(line < 1 || line > m_instructions.size()) {
            return false;
        }
        m_executor->AddBreakpoint(m_instructions[line-1].istream_start);
        m_breakLine.insert(line);
        return true;
    }

    bool DebugDisplay::removeBreakpoint(int line) {
        if(line < 1 || line > m_instructions.size()) {
            return false;
        }
        m_executor->RemoveBreakpoint(m_instructions[line-1].istream_start);
        for(auto i = m_breakLine.begin(); i != m_breakLine.end(); i++) {
            if(*i == line) {
                m_breakLine.erase(i);
                break;
            }
        }
        return true;
    }

    void DebugDisplay::outputStreamHandler(std::string text) {
        m_consoleOutput.emplace_back(text);
    }

    void DebugDisplay::errorStreamHandler(std::string text) {
        m_consoleOutput.emplace_back("[ERR] " + text);
    }

    void DebugDisplay::listen() {
        // Update and draw screen
        update();
        draw();
        while(m_executor) {
            int c = wgetch(m_CDKScreen->window);
            // Quit
            if((m_focusPanel != COMMAND && c == 'q') || c == KEY_ESC) {
                return;
            }
            // Switch panel
            if(c == KEY_TAB) {
                m_focusPanel = static_cast<Panel>((m_focusPanel+1) % 4);
            }
            // Scroll output console
            else if(c == KEY_F(1)) {
                m_consoleTopIndex--;
            } else if(c == KEY_F(2)) {
                m_consoleTopIndex++;
            }

            // Check panel in focus
            switch (m_focusPanel) {
                case STACK:
                    if(c == KEY_LEFT) {
                        m_stackHighlightColIndex--;
                    } else if(c == KEY_RIGHT) {
                        m_stackHighlightColIndex++;
                    }
                    break;
                case MEMORY:
                    if(c == KEY_UP) {
                        if(m_memoByteStart - MEMORY_BYTES_PER_LINE >= 0) {
                            m_memoByteStart -= MEMORY_BYTES_PER_LINE;
                        }
                    } else if(c == KEY_DOWN) {
                        m_memoByteStart += MEMORY_BYTES_PER_LINE;
                    } else if(c == KEY_LEFT) {
                        if(m_memoByteStart > 0) {
                            m_memoByteStart--;
                        }
                    } else if(c == KEY_RIGHT) {
                        m_memoByteStart++;
                    } else if(c == KEY_NPAGE) {
                        if(m_memoIndex < m_executor->GetMemoriesCount()-1) {
                            m_memoIndex++;
                        }
                    } else if(c == KEY_PPAGE) {
                        if(m_memoIndex > 0) {
                            m_memoIndex--;
                        }
                    }
                    break;
                case CODE:
                    if(c == KEY_UP) {
                        m_codeTopIndex--;
                    } else if(c == KEY_DOWN) {
                        m_codeTopIndex++;
                    }
                    break;
                case COMMAND:
                    std::vector<char> &currentCommand = m_commandHistory[m_commandHistoryScroll];
                    if (c == KEY_ENTER || c == '\n') {
                        // If command is not empty or previous commands has been entered
                        if(!currentCommand.empty() || m_commandHistory.size() > 1) {
                            // If command is empty, then run the previous command
                            if(currentCommand.empty()) {
                                m_commandHistory[m_commandHistory.size()-1] = m_commandHistory[m_commandHistory.size()-2];
                            } else {
                                m_commandHistory[m_commandHistory.size() - 1] = currentCommand;
                            }
                            // Handle entered command
                            handleCommand(std::string(m_commandHistory[m_commandHistory.size()-1].begin(),
                                                      m_commandHistory[m_commandHistory.size()-1].end()));
                            // Place a new empty command
                            m_commandHistory.emplace_back(std::vector<char>());
                            // Update history scroll
                            m_commandHistoryScroll = (int) m_commandHistory.size() - 1;
                            // Update command index for cursor
                            m_currentCommandIndex = 0;
                        }
                    } else if (c == KEY_BACKSPACE || c == 127) {
                        if (m_currentCommandIndex > 0) {
                            m_currentCommandIndex--;
                            currentCommand.erase(currentCommand.begin() + m_currentCommandIndex);
                        }
                    } else if (c == KEY_DC) {
                        if (m_currentCommandIndex < currentCommand.size()) {
                            currentCommand.erase(currentCommand.begin() + m_currentCommandIndex);
                        }
                    } else if (c == KEY_UP) {
                        if (m_commandHistoryScroll > 0) {
                            m_commandHistoryScroll--;
                        }
                    } else if (c == KEY_DOWN) {
                        if (m_commandHistoryScroll < m_commandHistory.size() - 1) {
                            m_commandHistoryScroll++;
                        }
                    } else if (c == KEY_LEFT) {
                        if (m_currentCommandIndex > 0) {
                            m_currentCommandIndex--;
                        }
                    } else if (c == KEY_RIGHT) {
                        if (m_currentCommandIndex < currentCommand.size()) {
                            m_currentCommandIndex++;
                        }
                    } else if(c == KEY_END) {
                        m_currentCommandIndex = (int) currentCommand.size();
                    } else if(c == KEY_HOME) {
                        m_currentCommandIndex = 0;
                    } else if (std::isprint(c)) {
                        if (m_currentCommandIndex == currentCommand.size()) {
                            currentCommand.emplace_back(c);
                        } else {
                            currentCommand.insert(currentCommand.begin() + m_currentCommandIndex, c);
                        }
                        m_currentCommandIndex++;
                    }

                    // Update current command index in case of history scroll
                    if (m_currentCommandIndex > m_commandHistory[m_commandHistoryScroll].size()) {
                        m_currentCommandIndex = (int) m_commandHistory[m_commandHistoryScroll].size();
                    }
                    break;
            }
            update();
            draw();
        }
    }
}