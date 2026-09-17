
#include "Engine.h"

//#include "libprojectM/Logging.hpp"

#include <cstdio>  // vsnprintf
#include <cstring> // strcmp, strcasecmp
#include <cstdlib> // strtol
#include <locale>
#include <string>
#include <sstream>
#include <vector>


namespace M4 {

// Engine/String.cpp

int String_PrintfArgList(char * buffer, int size, const char * format, va_list args) {

    va_list tmp;
    va_copy(tmp, args);

#if _MSC_VER >= 1400
	int n = vsnprintf_s(buffer, size, _TRUNCATE, format, tmp);
#else
	int n = vsnprintf(buffer, size, format, tmp);
#endif

    va_end(tmp);

	if (n < 0 || n > size) return -1;
	return n;
}

int String_Printf(char * buffer, int size, const char * format, ...) {

    va_list args;
    va_start(args, format);

    int n = String_PrintfArgList(buffer, size, format, args);

    va_end(args);

	return n;
}

int String_FormatFloat(char * buffer, int size, float value) {
    std::ostringstream oss;
    oss.imbue(std::locale("C"));
    oss << value;

    return String_Printf(buffer, size, "float(%s)", oss.str().c_str());
}

bool String_Equal(const char * a, const char * b) {
	if (a == b) return true;
	if (a == NULL || b == NULL) return false;
	return strcmp(a, b) == 0;
}

bool String_EqualNoCase(const char * a, const char * b) {
	if (a == b) return true;
	if (a == NULL || b == NULL) return false;
#if _MSC_VER
	return _stricmp(a, b) == 0;
#else
	return strcasecmp(a, b) == 0;
#endif
}

// Every character reading a double from a stream can accept, in either C++ library: libstdc++
// takes digits, signs, a point and an exponent; libc++ also gathers hex digits, x, p, and the
// letters of inf and nan before it converts. Whitespace comes first. A stream stops at the first
// character outside these, so the leading run of them is all it will ever read.
static inline bool iss_number_char(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
           c == '+' || c == '-' || c == '.' || c == 'x' || c == 'X' || c == 'p' || c == 'P' ||
           c == 'i' || c == 'I' || c == 'n' || c == 'N';
}

static inline double iss_strtod(const char * in, char ** end) {
    char * in_var = const_cast<char *>(in);
    // The stream used to be made from the whole rest of the source, which it copies: the tokenizer
    // asks at every number, so translating a shader was quadratic in its length. It is made from
    // the run a stream could read at all, which parses the same and ends in the same place.
    const char * run = in;
    while (*run == ' ' || *run == '\t' || *run == '\n' || *run == '\r' || *run == '\v' || *run == '\f') {
        run++;
    }
    while (iss_number_char(*run)) {
        run++;
    }
    double df;
    std::istringstream iss(std::string(in, run));
    iss.imbue(std::locale::classic());
    iss >> df;
    if(iss.fail()) {
        *end = in_var;
        return 0.0;
    }
    if(iss.eof()) {
        *end = in_var + (run - in);
        return df;
    }

    std::streampos pos = iss.tellg();
    if(iss.fail()) {
        *end = in_var;
        return 0.0;
    }
    *end = in_var + pos;
    return df;
}

double String_ToDouble(const char * str, char ** endptr) {
	return iss_strtod(str, endptr);
}

int String_ToInteger(const char * str, char ** endptr) {
	return strtol(str, endptr, 10);
}

//int String_ToIntegerHex(const char * str, char ** endptr) {
//	return strtol(str, endptr, 16);
//}



// Engine/Log.cpp

void Log_Error(const char * format, ...) {
    va_list args;
    va_start(args, format);
    Log_ErrorArgList(format, args);
    va_end(args);
}

void Log_ErrorArgList(const char * format, va_list args) {
    //using libprojectM::Logging;

    va_list tmp;
    va_copy(tmp, args);
    std::vector<char> buffer(static_cast<size_t>(std::vsnprintf(nullptr, 0, format, tmp)) + 1, '\0');
    va_end(tmp);
    vsnprintf(buffer.data(), buffer.size(), format, args );
    //LOG_ERROR("[HLSLParser] " + std::string(buffer.data()));
}


// Engine/StringPool.cpp

StringPool::StringPool(Allocator * allocator) : stringArray(allocator) {
}
StringPool::~StringPool() {
    for (int i = 0; i < stringArray.GetSize(); i++) {
        free((void *)stringArray[i]);
        stringArray[i] = NULL;
    }
}

const char * StringPool::AddString(const char * string) {
    for (int i = 0; i < stringArray.GetSize(); i++) {
        if (String_Equal(stringArray[i], string)) return stringArray[i];
    }
#if _MSC_VER
    const char * dup = _strdup(string);
#else
    const char * dup = strdup(string);
#endif
    stringArray.PushBack(dup);
    return dup;
}

// @@ From mprintf.cpp
static char *mprintf_valist(int size, const char *fmt, va_list args) {
    ASSERT(size > 0);
    char *res = NULL;
    va_list tmp;

    while (1) {
        res = new char[size];
        if (!res) return NULL;

        va_copy(tmp, args);
        int len = vsnprintf(res, size, fmt, tmp);
        va_end(tmp);

        if ((len >= 0) && (size >= len + 1)) {
            break;
        }

        delete [] res;

        if (len > -1 ) {
            size = len + 1;
        }
        else {
            size *= 2;
        }
    }

    return res;
}

const char * StringPool::AddStringFormatList(const char * format, va_list args) {
    va_list tmp;
    va_copy(tmp, args);
    const char * string = mprintf_valist(256, format, tmp);
    va_end(tmp);

    for (int i = 0; i < stringArray.GetSize(); i++) {
        if (String_Equal(stringArray[i], string)) {
            delete [] string;
            return stringArray[i];
        }
    }

    stringArray.PushBack(string);
    return string;
}

const char * StringPool::AddStringFormat(const char * format, ...) {
    va_list args;
    va_start(args, format);
    const char * string = AddStringFormatList(format, args);
    va_end(args);

    return string;
}

bool StringPool::GetContainsString(const char * string) const {
    for (int i = 0; i < stringArray.GetSize(); i++) {
        if (String_Equal(stringArray[i], string)) return true;
    }
    return false;
}

} // M4 namespace
