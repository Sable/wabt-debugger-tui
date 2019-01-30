#include <wdb_tui/common.h>
#include <wdb_tui/side_menu_display.h>
#include <wdb_tui/home_display.h>
#include <wdb_tui/wast_display.h>
#include <wdb_tui/profiler_display.h>
#include <wdb_tui/debug_display.h>
#include <wdb_tui/host_functions.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <fstream>

// Program arguments
std::vector<std::string> inputFiles;
std::string f_arg_function;
bool f_profiler = false;
bool f_tuiEnabled = false;
bool f_initHostFunctions = false;

/**
 * Print usage message
 */
void printUsage() {
    std::cerr
            << "wdb_tui - Debug wasm on the terminal" << std::endl
            << "Usage: wdb [OPTION]... [FILE]..." << std::endl
            << "    -t, --tui               Open in tui mode" << std::endl
            << "    -i, --init-host         Initialize host functions" << std::endl
            << "    -r, --run      <func>   Execute an exported function" << std::endl
            << "    -p, --profiler <func>   Show profiler info for an exported function" << std::endl
            << "    -h, --help              Display this help message" << std::endl;
}

/**
 * Initialize parameter
 * @param argc
 * @param argv
 */
void initParams(int argc, char *argv[]) {

    struct option longOptions[] = {
            {"tui", no_argument, 0, 't'},
            {"init-host", no_argument, 0, 'i'},
            {"run", required_argument, 0, 'r'},
            {"profiler", required_argument, 0, 'p'},
            {"help", no_argument, 0, 'h'},
            {0, 0,                0, 0}
    };

    int optionIndex = 0;
    int c;
    while ((c = getopt_long(argc, argv, "tir:p:h", longOptions, &optionIndex)) != -1) {
        switch (c) {
            case 't':
                f_tuiEnabled = true;
                break;
            case 'i':
                f_initHostFunctions = true;
                break;
            case 'p':
                f_profiler = true;
                f_arg_function = optarg;
                break;
            case 'r':
                f_arg_function = optarg;
                break;
            case 'h':
            default:
                // Print by default
                break;
        }
    }
}

void initColors() {
    // Starts the Cdk color capabilities
    initCDKColor();
    // Initialize color pairs
    init_pair(WDB_COLOR_NORMAL, COLOR_WHITE, COLOR_BLACK);
    init_pair(WDB_COLOR_CMD, COLOR_GREEN, COLOR_BLACK);
    init_pair(WDB_COLOR_SUCCESS, COLOR_WHITE, COLOR_GREEN);
    init_pair(WDB_COLOR_ERROR, COLOR_WHITE, COLOR_RED);
    init_pair(WDB_COLOR_INFO, COLOR_WHITE, COLOR_CYAN);
}

/**
 * Initialize ncurses
 */
void initNCurses() {
    // Determine the terminal type and
    // initialises all implementation data structures
    initscr();
    // Initialize colors
    initColors();
    // Break on ctrl+c
    cbreak();
    // Hide input typed by the user
    noecho();
    // Enable user keypad
    keypad(stdscr, TRUE);
    // Set cursor to invisible
    curs_set(0);
}

/**
 * End ncurses
 */
void endNCurses() {
    // Shutdown ncurses
    endCDK();
}

void tui(wdb::WdbWabt &wdbWabt, wdb::WdbExecutor::Options &options) {
    // Init NCurses
    initNCurses();
    // Init displays
    wdb::SideMenu sideMenu;
    wdb::HomeDisplay homeDisplay;
    wdb::WastDisplay wastDisplay(&wdbWabt);
    wdb::ProfilerDisplay profilerDisplay(&wdbWabt, options);
    wdb::DebugDisplay debugDisplay(&wdbWabt, options);

    // Draw side menu
    sideMenu.draw();
    // By default display home
    homeDisplay.draw();

    // Listen for menu events
    wdb::SideMenu::MENU_ITEM selectedDisplay;
    while((selectedDisplay = sideMenu.listen()) != wdb::SideMenu::MENU_ITEM::Exit) {
        switch (selectedDisplay) {
            case wdb::SideMenu::MENU_ITEM::HOME:
            default:
                homeDisplay.draw();
                break;
            case wdb::SideMenu::MENU_ITEM::WAST:
                wastDisplay.setFocus(true);
                wastDisplay.listen();
                wastDisplay.setFocus(false);
                wastDisplay.draw();
                break;
            case wdb::SideMenu::MENU_ITEM::PROFILER:
                profilerDisplay.setFocus(true);
                profilerDisplay.listen();
                profilerDisplay.setFocus(false);
                profilerDisplay.draw();
                break;
            case wdb::SideMenu::MENU_ITEM::DEBUG:
                debugDisplay.setFocus(true);
                debugDisplay.listen();
                debugDisplay.setFocus(false);
                debugDisplay.draw();
                break;
        }
    }

    // Destroy ncurses
    endNCurses();
}

void InitHostFunctions(wdb::WdbExecutor* executor) {
    if(f_initHostFunctions) {
        std::string moduleName = "wdb_tui";
        // Print function
        // params: memory offset, length of string
        if(!wabt::Succeeded(executor->AppendHostFuncExport(moduleName, "prints", {{wabt::Type::I32, wabt::Type::I32}, {}},
                                       [=](const wabt::interp::HostFunc *func, const wabt::interp::FuncSignature *sig,
                                           const wabt::interp::TypedValues &args, wabt::interp::TypedValues &results) {
                                           if (executor->GetMemoriesCount() > 0) {
                                               int offset = args[0].get_i32();
                                               int length = args[1].get_i32();
                                               std::stringstream ss;
                                               for (int i = 0; i < length; i++) {
                                                   ss << executor->GetMemoryAt(0, offset + i);
                                               }
                                               executor->PostOutput(ss.str());
                                           } else {
                                               executor->PostError("No memory found!");
                                           }
                                           return wabt::interp::Result::Ok;
                                       }))) {
            std::cerr << "Failed to add '" << moduleName << "' 'prints' functions" << std::endl;
        }

        // Get time in ms
        // results: time in ms
        if(!wabt::Succeeded(executor->AppendHostFuncExport("wdb_tui", "get_time_ms", {{}, {wabt::Type::I64}},
                                       [=](const wabt::interp::HostFunc *func, const wabt::interp::FuncSignature *sig,
                                           const wabt::interp::TypedValues &args, wabt::interp::TypedValues &results) {
                                           {
                                               using namespace std::chrono;
                                               auto time = duration_cast<milliseconds>(
                                                       system_clock::now().time_since_epoch());
                                               results[0].set_i64(time.count());
                                           }
            return wabt::interp::Result::Ok;
        }))) {
            std::cerr << "Failed to add '" << moduleName << "' 'get_time_ms' functions" << std::endl;
        }
    }

    // Initialize user's host functions
    wdb_stubs::InitHostFunctions(executor);
}

wabt::Result Execute(wdb::WdbExecutor* executor) {
    wabt::Result result;
    wabt::interp::Export* eFunction;
    result = executor->SearchExportedModuleFunction(executor->GetMainModule(), f_arg_function, &eFunction);
    if(result == wabt::Result::Ok) {
        wabt::interp::Func* func = executor->GetFunction(eFunction->index);
        result = executor->SetMainFunction(func);
        if(result == wabt::Result::Ok) {
            result = executor->Execute();
            if(result == wabt::Result::Ok) {
                return wabt::Result::Ok;
            } else {
                std::cerr << "Error executing '" << f_arg_function << "'" << std::endl;
            }
        } else {
            std::cerr << "Error setting '" << f_arg_function << "' as main function" << std::endl;
        }
    } else {
        std::cerr << "Function '" << f_arg_function << "' was not found!" << std::endl;
    }
    return wabt::Result::Error;
}

int main(int argc, char* argv[]) {
    // Init parameters
    initParams(argc, argv);

    // Fetch files names
    for(int i = optind; i < argc; ++i) {
        std::string fileName = argv[i];
        if(!fileName.empty()) {
            inputFiles.push_back(fileName);
        }
    }

    // Check for require arguments
    if(inputFiles.empty() || (!f_tuiEnabled && f_arg_function.empty())) {
        printUsage();
        return 1;
    }

    // Check if file exists
    std::string inputFile = inputFiles.front();
    std::ifstream inputFileStream(inputFile);
    if(!inputFileStream.good()) {
        std::cerr << "Error opening file: " << inputFile << std::endl;
        return 1;
    }

    // Init application
    wdb::WdbWabt wdbWabt;
    wdb::WdbExecutor::Options options;
    options.preSetup = InitHostFunctions;
    if(wdbWabt.LoadModuleFile(inputFile) == wabt::Result::Ok) {
        if(f_tuiEnabled) {
            // Open in tui mode
            tui(wdbWabt, options);
        } else {
            // Update options for non-tui
            options.outputStreamHandler = [](std::string text) {
                std::cout << text;
            };
            options.errorStreamHandler = [](std::string text) {
                std::cerr << text;
            };
            if(f_profiler) {
                wdb::WdbProfilerExecutor* profilerExecutor = wdbWabt.CreateWdbProfilerExecutor(options);
                if(profilerExecutor) {
                    if(Execute(profilerExecutor) == wabt::Result::Ok) {
                        std::cout << "[Profiler results]" << std::endl;
                        auto data = profilerExecutor->GetProfilerMap();
                        for(auto entry : data) {
                            std::cout << "  " << entry.second.GetOpcode().GetName() << std::endl
                                      << "  ├ Count:      " << entry.second.GetCount() << std::endl
                                      << "  ├ Total Time: " << entry.second.GetTotalTime() << " ns" << std::endl
                                      << "  └ Avg. Time:  " << entry.second.GetAverageTime() << " ns" << std::endl;
                        }
                        std::cout << "[End of results]" << std::endl;
                    }
                } else {
                    std::cerr << "Error creating profiler executor" << std::endl;
                }
            } else {
                wdb::WdbExecutor* executor = wdbWabt.CreateWdbExecutor(options);
                if(executor) {
                    Execute(executor);
                } else {
                    std::cerr << "Error creating executor" << std::endl;
                }
            }
        }
    } else {
        std::cerr << "Error reading file: " << inputFile << std::endl;
    }
}

