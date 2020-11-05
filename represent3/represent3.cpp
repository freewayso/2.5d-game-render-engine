
//	required compiler settings: /MDd (Multi-threaded Debug DLL) or /MD (Multi-threaded DLL), /GR (Runtime Type Info)

#include "shell.h"

extern KShell g_cShell;

extern "C" __declspec(dllexport) 
IKShell* CreateRepresentShell() {return &g_cShell;}