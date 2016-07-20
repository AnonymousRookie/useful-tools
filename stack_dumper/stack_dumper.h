#ifndef STACK_DUMPER_H
#define STACK_DUMPER_H

#include <sstream>

#ifdef _WIN32
#include <windows.h>  
#include <dbghelp.h>
#pragma comment (lib, "dbghelp.lib")  
#else
#include <execinfo.h>
#endif  // _WIN32



class StackDumper {
 public:
    StackDumper();

    ~StackDumper();

    void Destory();

    std::string DumpStack();

#ifdef __linux__
    int32 Exec_cmd(const char* cmd);
    void ParseName(char* str, char* exeName, char* addr);
    void Record_trace();
#endif  // __linux__

 private:
#ifdef _WIN32
    UINT max_name_length_;              // Max length of symbols' name.
    CONTEXT context_;                   // Store register addresses.
    STACKFRAME64 stackframe_;           // Call stack.
    HANDLE process_, thread_;           // Handle to current process & thread.
    PSYMBOL_INFO symbol_;               // Debugging symbol's information.
    IMAGEHLP_LINE64 source_info_;       // Source information (file name & line number)
    DWORD displacement_;                // Source line displacement.
#endif  // _WIN32
    std::ostringstream stack_info_str_stream_;
};


#endif  // STACK_DUMPER_H