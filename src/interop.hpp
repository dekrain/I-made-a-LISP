// Interoperationality for MAL
// Load a dynamic library & search for `MalInit` symbol

#pragma once

#include "malvalue.hpp"

namespace mal {
    using EntryProc = bool(__cdecl *)(Interpreter&);
    static constexpr const char* EntrySymbol = "MalInit";

    bool InLoadLibrary(Interpreter& interp, const char* fname);
}