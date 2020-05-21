#include "interop.hpp"

#include <windows.h>

namespace mal {
    bool InLoadLibrary(Interpreter& interp, const char* fname) {
        HMODULE lib = LoadLibraryA(fname);
        // ! Handle leak
        if (!lib)
            return false;
        EntryProc entry = reinterpret_cast<EntryProc>(GetProcAddress(lib, EntrySymbol));
        if (!entry) {
            FreeLibrary(lib);
            return false;
        }
        if (!entry(interp)) {
            FreeLibrary(lib);
            return false;
        }
        return true;
    }
}