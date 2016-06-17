#include <iostream>
#include <string>
#include <sstream>
#include "stack_dumper.h"

#define LOG_SHOW \
    { \
        auto &str = StackDumper().DumpStack(); \
        printf("%s\n", str.c_str()); \
    }

int func(int arc) {
    LOG_SHOW;
    return arc;
}

void gunc() {
    func(11);
}

void main() {
    gunc();
    system("pause");
}