#include <wdb_tui/profiler_display.h>
#include <wdb_tui/common.h>
#include <wabt/src/cast.h>

namespace wdb {
    ProfilerDisplay::ProfilerDisplay(wdb::WdbWabt *wdbWabt, wdb::WdbExecutor::Options options) :
            Display(DISPLAYS_LINES, DISPLAYS_COLS, 0, SIDE_MENU_COLS) {
        // Enable keypad on this window
        keypad(m_CDKScreen->window, true);
        // Set default sorting
        m_listSort = WdbProfilerExecutor::Sort::OPCODE_ASC;
        // Set wabt
        m_wdbWabt = wdbWabt;
        // Set options
        m_executorOptions.preSetup = options.preSetup;
        // Create a default executor
        m_executor = m_wdbWabt->CreateWdbProfilerExecutor(m_executorOptions);
        // Set default panel focus
        m_focusPanel = FUNCTIONS;
    }

    void ProfilerDisplay::setStatus(short color, std::string message, bool pause) {
        drawMessage(getNumLines()-2, 1, getNumCols()-2, color, A_BOLD, message);
        if(pause) {
            wgetch(m_CDKScreen->window);
        }
    }

    void ProfilerDisplay::updateFuncList() {
        // Compute list offsets
        int topLeftY = 1;
        int topLeftX = 1;

        // Update list configuration
        int numLines = getNumLines() / 2;
        int numCols = getNumCols() - (2 * topLeftX);

        // Draw border
        drawBorder(topLeftY, topLeftX, numLines, numCols, m_focusPanel == FUNCTIONS, "Functions");

        // Clear list of functions
        m_funcList.clear();
        // Prepare data vector
        std::vector<std::vector<std::string>> data;
        // Fetch modules
        for(int i=0; i < m_executor->GetModuleSize(); i++) {
            auto currentModule = m_executor->GetModuleAt(i);
            // Fetch functions in modules
            auto functions = m_executor->GetExportedModuleFunctions(currentModule);
            // Populate data
            for (int j = 0; j < functions.size(); j++) {
                auto moduleName = (currentModule == m_executor->GetMainModule()) ? "<main>" : currentModule->name;
                auto currentFunction = functions[j];
                auto func = m_executor->GetFunction(currentFunction.index);
                auto funcSig = m_executor->GetFunctionSignature((func->sig_index));
                // Create param string
                std::stringstream params;
                for(int i=0; i < funcSig->param_types.size(); i++) {
                    if(i > 0 ){
                        params << ",";
                    }
                    params << wabt::GetTypeName(funcSig->param_types[i]);
                }
                // Create results string
                std::stringstream results;
                for(int i=0; i < funcSig->result_types.size(); i++) {
                    if(i > 0 ){
                        results << ",";
                    }
                    results << wabt::GetTypeName(funcSig->result_types[i]);
                }

                m_funcList.emplace_back(currentFunction);
                data.push_back({moduleName,
                                currentFunction.name,
                                funcSig->param_types.empty() ? "<empty>" : params.str(),
                                funcSig->result_types.empty() ? "<empty>" : results.str(),
                                func->is_host ? "Yes" : "No",
                                m_executor->CanBeMain(func) ? "Yes" : "No"});
            }
        }
        // Create table header
        std::vector<std::string> header = {"Module", "Func Name", "Func Params", "Func Returns", "Is Host?",
                                           "Can Run?"};
        // Draw table
        int highlightCol = 0;
        drawTable(topLeftY, topLeftX, numLines, numCols, header, data, header.size(), header.size(), m_funcTopIndex,
                  m_funcLeftIndex, m_funcHighlight, highlightCol, Highlight::HLINE, true);
    }

    void ProfilerDisplay::updateDataList() {
        // Compute list offsets
        int topLeftY = getNumLines() / 2 + 1;
        int topLeftX = 1;

        // Update list configuration
        int numLines = getNumLines() - topLeftY - 2;
        int numCols = getNumCols() - (2 * topLeftX);

        // Draw border
        drawBorder(topLeftY, topLeftX, numLines, numCols, m_focusPanel == RESULTS, "Profiling Result");

        // Prepare data vector
        std::vector<std::vector<std::string>> data;
        // Fetch profiler entries
        auto entries = m_executor->GetProfilerSorted(m_listSort);
        // Populate data
        for (int i = 0; i < entries.size(); i++) {
            auto &currentEntry = entries[i];
            data.push_back({currentEntry.GetOpcode().GetName(),
                            std::to_string(currentEntry.GetCount()),
                            std::to_string(currentEntry.GetTotalTime()),
                            std::to_string(currentEntry.GetAverageTime())});
        }
        // Create table header
        std::vector<std::string> header = {"Opcode", "Total Count", "Total Time(ns)", "Avg. Time(ns)"};
        // Draw table
        int highlightCol = 0;
        drawTable(topLeftY, topLeftX, numLines, numCols, header, data, header.size(), header.size(), m_dataTopIndex,
                  m_dataLeftIndex, m_dataHighlight, highlightCol, Highlight::HLINE, true);
    }

    void ProfilerDisplay::update() {
        // Erase screen
        werase(m_CDKScreen->window);
        if(m_executor) {
            // Update panels
            updateFuncList();
            updateDataList();
            // Draw instruction
            setStatus(WDB_COLOR_INFO,
                      "<ENTER>Run | <TAB>Focus | Sort:<F1>Opcode <F2>Total Count <F3>Total Time <F4>Avg. Time", false);
        } else {
            drawDialog("Error", "Error creating an executor, please verify the wasm file is valid", WDB_COLOR_ERROR,
                       A_BOLD);
        }
    }

    void ProfilerDisplay::executeFunction() {
        if(m_funcHighlight < 0) {
            setStatus(WDB_COLOR_ERROR, "No function is selected", true);
        } else {
            // Get highlighted function
            auto entryExport = m_funcList[m_funcHighlight];
            auto func = m_executor->GetFunction(entryExport.index);
            if(!m_executor->CanBeMain(func)) {
                setStatus(WDB_COLOR_ERROR, "Selected function cannot be executed", true);
            } else {
                setStatus(WDB_COLOR_INFO, "Running function ...", false);
                draw();
                // Set new executor
                m_executor = m_wdbWabt->CreateWdbProfilerExecutor(m_executorOptions);
                // Set main function
                if(m_executor->SetMainFunction(func) == wabt::Result::Ok) {
                    // Execute function
                    if(m_executor->Execute() == wabt::Result::Ok){
                        setStatus(WDB_COLOR_SUCCESS, "Function finished executing, press any key to see results", true);
                    } else {
                        setStatus(WDB_COLOR_ERROR, "Error executing function", true);
                    }
                } else {
                    setStatus(WDB_COLOR_ERROR, "Failed to set make function as main", true);
                }
            }
        }
    }

    void ProfilerDisplay::listen() {
        // Update and draw screen
        update();
        draw();
        // Listen for keyboard input
        while(m_executor) {
            int c = wgetch(m_CDKScreen->window);
            switch (c) {
                case 'q':
                case KEY_ESC:
                    return; // Quit
                case KEY_TAB:
                    m_focusPanel = static_cast<Panel>((m_focusPanel+1) % 2);
                    break;
                case KEY_F(1):
                    if(m_listSort == wdb::WdbProfilerExecutor::Sort::OPCODE_ASC) {
                        m_listSort = wdb::WdbProfilerExecutor::Sort::OPCODE_DESC;
                    } else {
                        m_listSort = wdb::WdbProfilerExecutor::Sort::OPCODE_ASC;
                    }
                    break;
                case KEY_F(2):
                    if(m_listSort == wdb::WdbProfilerExecutor::Sort::TOTAL_COUNT_ASC) {
                        m_listSort = wdb::WdbProfilerExecutor::Sort::TOTAL_COUNT_DESC;
                    } else {
                        m_listSort = wdb::WdbProfilerExecutor::Sort::TOTAL_COUNT_ASC;
                    }
                    break;
                case KEY_F(3):
                    if(m_listSort == wdb::WdbProfilerExecutor::Sort::TOTAL_TIME_ASC) {
                        m_listSort = wdb::WdbProfilerExecutor::Sort::TOTAL_TIME_DESC;
                    } else {
                        m_listSort = wdb::WdbProfilerExecutor::Sort::TOTAL_TIME_ASC;
                    }
                    break;
                case KEY_F(4):
                    if(m_listSort == wdb::WdbProfilerExecutor::Sort::AVG_TIME_ASC) {
                        m_listSort = wdb::WdbProfilerExecutor::Sort::AVG_TIME_DESC;
                    } else {
                        m_listSort = wdb::WdbProfilerExecutor::Sort::AVG_TIME_ASC;
                    }
                    break;
                case KEY_UP:
                    if(m_focusPanel == FUNCTIONS) {
                        m_funcHighlight--;
                    } else if(m_focusPanel == RESULTS) {
                        m_dataHighlight--;
                    }
                    break;
                case KEY_DOWN:
                    if(m_focusPanel == FUNCTIONS) {
                        m_funcHighlight++;
                    } else if(m_focusPanel == RESULTS) {
                        m_dataHighlight++;
                    }
                    break;
                case '\n':
                case KEY_ENTER:
                    if(m_focusPanel == FUNCTIONS) {
                        executeFunction();
                    }
                    break;
                default:
                    // Do nothing
                    break;
            }
            update();
            draw();
        }
    }
}