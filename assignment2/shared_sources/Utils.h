#pragma once

#include <string>

using namespace std;

// Exit with an error message formatted with sprintf
void fail(const string& reason);

// Portable 
string fileOpenDialog(const string& fileTypeName, const string& fileExtensions);
