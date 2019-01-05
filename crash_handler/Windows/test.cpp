#include <iostream>
#include "crash_handler.h"

int main()
{
    z::RunCrashHandler();

    int x = 0;
    int y = 0;

    x /= y;

    return 0;
}