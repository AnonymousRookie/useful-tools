#ifndef STACK_DUMPER_H
#define STACK_DUMPER_H

#ifdef _WIN32
#include <windows.h>  
#include <dbghelp.h>
#include <string>
#include <sstream>
#pragma comment (lib, "dbghelp.lib")  
#endif  // _WIN32


class StackDumper {
 public:
    StackDumper();

    ~StackDumper();

    void Destory();

    std::string DumpStack();

 private:
#ifdef _WIN32
    UINT max_name_length_;              // Max length of symbols' name.
    CONTEXT context_;				    // Store register addresses.
    STACKFRAME64 stackframe_;           // Call stack.
    HANDLE process_, thread_;           // Handle to current process & thread.
    PSYMBOL_INFO symbol_;               // Debugging symbol's information.
    IMAGEHLP_LINE64 source_info_;       // Source information (file name & line number)
    DWORD displacement_;				// Source line displacement.
#endif  // _WIN32
    std::ostringstream stack_info_str_stream_;
};


#endif  // STACK_DUMPER_H