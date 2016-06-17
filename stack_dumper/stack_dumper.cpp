#include "stack_dumper.h"

StackDumper::StackDumper() {
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
    stack_info_str_stream_.str("");
    process_ = GetCurrentProcess();  // Get current process & thread.  
    thread_ = GetCurrentThread();
	// Initialize dbghelp library.  
    if (!SymInitialize(process_, NULL, TRUE)) {
        stack_info_str_stream_ << "Initialize dbghelp library ERROR!\n";
	}
#endif  //_WIN32
}

StackDumper::~StackDumper() {
	Destory();
}

void StackDumper::Destory() {
	SymCleanup(process_);  // Clean up and exit.  
	free(symbol_);
    stack_info_str_stream_ << "StackDumper is cleaned up!\n";
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
#endif  //_WIN32
    return stack_info_str_stream_.str();
}