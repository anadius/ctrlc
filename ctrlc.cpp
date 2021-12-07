#include <iostream>
#include <windows.h>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Usage:\nctrlc.exe <pid>\n";
        return 1;
    }

    int pid = atoi(argv[1]);
    FreeConsole();
    AttachConsole(pid);
    SetConsoleCtrlHandler(NULL, 1);
    GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);

    return 0;
}
