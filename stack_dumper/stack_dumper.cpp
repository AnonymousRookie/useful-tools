#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stack_dumper.h"


StackDumper::StackDumper() {
    stack_info_str_stream_.str("");
#ifdef _WIN32
    enum { MAX_NAME_LENGTH = 256 };  // max length of symbols' name.
    // Initialize PSYMBOL_INFO structure.  
    // Allocate a properly-sized block.  
    symbol_ = (PSYMBOL_INFO)malloc(sizeof(SYMBOL_INFO)+(MAX_NAME_LENGTH - 1) * sizeof(TCHAR));
    memset(symbol_, 0, sizeof(SYMBOL_INFO)+(MAX_NAME_LENGTH - 1) * sizeof(TCHAR));
    symbol_->SizeOfStruct = sizeof(SYMBOL_INFO);  // SizeOfStruct *MUST BE* set to sizeof(SYMBOL_INFO).  
    symbol_->MaxNameLen = MAX_NAME_LENGTH;
    // Initialize IMAGEHLP_LINE64 structure.  
    memset(&source_info_, 0, sizeof(IMAGEHLP_LINE64));
    source_info_.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    // Initialize STACKFRAME64 structure.  
    RtlCaptureContext(&context_);  // Get context.  
    memset(&stackframe_, 0, sizeof(STACKFRAME64));
    stackframe_.AddrPC.Offset = context_.Eip;  // Fill in register addresses (EIP, ESP, EBP).  
    stackframe_.AddrPC.Mode = AddrModeFlat;
    stackframe_.AddrStack.Offset = context_.Esp;
    stackframe_.AddrStack.Mode = AddrModeFlat;
    stackframe_.AddrFrame.Offset = context_.Ebp;
    stackframe_.AddrFrame.Mode = AddrModeFlat;
    process_ = GetCurrentProcess();  // Get current process & thread.  
    thread_ = GetCurrentThread();
    // Initialize dbghelp library.  
    if (!SymInitialize(process_, NULL, TRUE)) {
        stack_info_str_stream_ << "Initialize dbghelp library ERROR!\n";
    }
#endif  // _WIN32
}

StackDumper::~StackDumper() {
    Destory();
}

void StackDumper::Destory() {
#ifdef _WIN32
    SymCleanup(process_);  // Clean up and exit.  
    free(symbol_);
    stack_info_str_stream_ << "StackDumper is cleaned up!\n";
#endif  // _WIN32
}

std::string StackDumper::DumpStack() {
#ifdef _WIN32
    stack_info_str_stream_ << "Call stack: \n";
    // Enumerate call stack frame.  
    while (StackWalk64(IMAGE_FILE_MACHINE_I386, process_, thread_, &stackframe_,
            &context_, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
        if (stackframe_.AddrFrame.Offset == 0) {  // End reaches.  
            break;
        }
        if (SymFromAddr(process_, stackframe_.AddrPC.Offset, NULL, symbol_)) {  // Get symbol.  
            stack_info_str_stream_ << " ==> " << symbol_->Name << "\n";
        }
        if (SymGetLineFromAddr64(process_, stackframe_.AddrPC.Offset, &displacement_, &source_info_)) {
            // Get source information.  
            stack_info_str_stream_ << "\t[" << source_info_.FileName << ":" << source_info_.LineNumber << "]\n";
        }
        else {
            if (GetLastError() == 0x1E7) {  // If err_code == 0x1e7, no symbol was found.  
                stack_info_str_stream_ << "\tNo debug symbol loaded for this function.\n";
            }
        }
    }
#endif  // _WIN32

#ifdef __linux__
    Record_trace();
#endif  // __linux__
    return stack_info_str_stream_.str();
}



#ifdef __linux__
int32 StackDumper::Exec_cmd(const char* cmd) {
    FILE *pp = popen(cmd, "r");  // 建立管道
    if (!pp) {
        stack_info_str_stream_ << "[Error] popen error in function StackDumper::Exec_cmd(const char* cmd)\n";
        return -1;
    }
    char tmp[1024];
    while (fgets(tmp, sizeof(tmp), pp) != NULL) {
        if (tmp[strlen(tmp) - 1] == '\n') {
            tmp[strlen(tmp) - 1] = '\0';  // 去除换行符
        }
        
        stack_info_str_stream_ << "[" << tmp << "]\t";
    }
    stack_info_str_stream_ << "\n";
    pclose(pp);  // 关闭管道

    return 0;
}

void StackDumper::ParseName(char* str, char* exeName, char* addr) {
    if (str == nullptr || exeName == nullptr || addr == nullptr) {
        stack_info_str_stream_ << "[Error] nullptr in function StackDumper::ParseName(char* str, char* exeName, char* addr)\n";
        return;
    }
    char* strTemp = str;
    char* addrTemp;
    while (*strTemp != '\0') {
        if (*strTemp == '(')
            memcpy(exeName, str, strTemp - str);

        if (*strTemp == '[')
            addrTemp = strTemp;

        if (*strTemp == ']')
            memcpy(addr, str + (addrTemp - str) + 1, strTemp - addrTemp - 1);
        strTemp++;
    }
}

void StackDumper::Record_trace()
{
    enum {MAX_CALLSTACK_DEPTH = 10};  // 需要打印堆栈的最大深度
    void *array[MAX_CALLSTACK_DEPTH];

    char **strings;

    int32 depth = backtrace(array, MAX_CALLSTACK_DEPTH);
    strings = backtrace_symbols(array, depth);

    stack_info_str_stream_ << " ==> Obtained "<< depth << "stack frames.\n";

    char cmd[1024] = {0}; 
    char exeName[1024] = {0};
    char addr[100] = {0};
    for(int32 i = 0; i < depth; ++i) {
      memset(cmd, 0, sizeof(cmd));
      memset(exeName, 0, sizeof(exeName));
      memset(addr, 0, sizeof(addr));
      ParseName(strings[i], exeName, addr);
      snprintf(cmd, sizeof(cmd)-1, "addr2line -f -e %s %s", exeName, addr);
      Exec_cmd(cmd);
    }     
}
#endif  // __linux__