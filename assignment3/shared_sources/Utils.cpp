#include <nfd.h>
#include "Utils.h"
#include <iostream>
#include <cstdarg>
#include <string>
#include <fmt/core.h>
#include <filesystem>

using namespace std;

// TODO TEST
void fail(const string& reason)
{
    cerr << reason.c_str();
    exit(1);
}

//------------------------------------------------------------------------

string fileOpenDialog(const string& fileTypeName, const string& fileExtensions)
{
    NFD_Init();

    string retval;

    nfdu8char_t* outPath;
    nfdu8filteritem_t filters[2] = { { fileTypeName.c_str(), fileExtensions.c_str()} };
    nfdopendialogu8args_t args = { 0 };
    args.filterList = filters;
    args.filterCount = 1;
    auto current_directory = filesystem::current_path().generic_string();
    args.defaultPath = current_directory.c_str(); // must do this in two steps! If we don't declare current_directory,
                                                  // the string goes out of scope immediately and the pointer becomes stale.
    nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
    if (result == NFD_OKAY)
    {
        retval = outPath;
        NFD_FreePathU8(outPath);
    }
    else if (result == NFD_CANCEL)
    {
    }
    else
    {
        fail(fmt::format("nativefiledialog Error: {}\n", NFD_GetError()));
    }
    NFD_Quit();
    
    return retval;
}