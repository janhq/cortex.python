#include "src/PythonRuntimeUtils.hpp"
#include "src/PythonRuntimeEngine.h"


#if defined(__APPLE__) && defined(__MACH__)
    #include <libgen.h> // for dirname()
    #include <mach-o/dyld.h>
#elif defined(__linux__)
    #include <libgen.h> // for dirname()
    #include <unistd.h> // for readlink()
#elif defined(_WIN32)
    #include <windows.h>
    #undef max
#else
    #error "Unsupported platform!"
#endif


#if defined(_WIN32)
  #include <process.h>
#else
  #include <spawn.h>
  #include <sys/wait.h>
  extern char **environ;
#endif


int main(int argc, char *argv[]) {

    if (argc > 1) {
        if (strcmp(argv[1], "--run_python_file") == 0) {
            std::string py_home_path = (argc >= 4) ? argv[3] : "";
            std::string cur_dir_path = PythonRuntimeUtils::getDirectoryPathFromFilePath(argv[0]);

            // fprintf(stderr, "0|%s\n", cur_dir_path.c_str());
            // fprintf(stderr, "1|%s\n", py_home_path.c_str());
            PythonRuntimeUtils::executePythonFile(cur_dir_path, argv[2], py_home_path);
            return 0;
        }
    } 
    else {
        LOG_INFO << "python-runtime started\n";
        std::string py_home_path = (argc > 1) ? argv[1] : "";
        std::string py_file_path;
        
        while (true) {
            std::cin>>py_file_path;
            
#if defined(_WIN32)
            std::wstring exePath = PythonRuntimeUtils::getCurrentExecutablePath();
            std::string pyArgsString = " --run_python_file " + py_file_path;
            if (py_home_path != "")
                pyArgsString += " " + py_home_path;
            std::wstring pyArgs = exePath + PythonRuntimeUtils::stringToWString(pyArgsString);

            STARTUPINFOW si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            if (!CreateProcessW(const_cast<wchar_t*>(exePath.data()), // the path to the executable file
                                const_cast<wchar_t*>(pyArgs.data()), // command line arguments passed to the child
                                NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
                LOG_ERROR << "Failed to create child process: " << GetLastError();
            } else {
                LOG_INFO << "Created child process for Python embedding";
            }
#else // _Apple and _Linux
            std::string child_process_exe_path = PythonRuntimeUtils::getCurrentExecutablePath();
            std::vector<char*> child_process_args;
            child_process_args.push_back(const_cast<char*>(child_process_exe_path.c_str()));
            child_process_args.push_back(const_cast<char*>("--run_python_file"));
            child_process_args.push_back(const_cast<char*>(py_file_path.c_str()));
            if (py_home_path != "")
                child_process_args.push_back(const_cast<char*>(py_home_path.c_str()));

            pid_t pid;

            int status = posix_spawn(&pid, child_process_exe_path.c_str(), nullptr, 
                                    nullptr, child_process_args.data(), environ);
                            
            if (status) {
                LOG_ERROR << "Failed to spawn process: " << strerror(status);
            } else {
                LOG_INFO << "Created child process for Python embedding";
                int stat_loc;
                if (waitpid(pid, &stat_loc, 0) == -1) {
                    LOG_ERROR << "Error waiting for child process";
                }
            }
#endif
        }        
    }

    return 0;
}
