#pragma once

#include <string>

using namespace std;

// Exit with an error message formatted with sprintf
void fail(const string& reason);

// Portable 
string fileOpenDialog(const string& fileTypeName, const string& fileExtensions);

// http://stackoverflow.com/questions/2270726/how-to-determine-the-size-of-an-array-of-strings-in-c
template <typename T, std::size_t N>
char (&static_sizeof_array( T(&)[N] ))[N];   // declared, not defined
#define SIZEOF_ARRAY( x ) sizeof(static_sizeof_array(x))
