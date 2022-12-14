#pragma once
#include <string>
#include <regex>

const std::string WHITESPACE = " \n\r\t\f\v";

//std::string을 std::printf처럼 조합하여 반환해주는 함수
template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("string_format Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

//공백(' ') N개를 1개로 줄여주는 함수
std::string string_ReplaceNSpaceTo1Space(std::string str)
{
    std::regex reg(R"(\s+)");
    str = std::regex_replace(str, reg, " ");
    return str;
}

std::string string_ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

std::string string_rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string string_trim(const std::string& s)
{
    return string_rtrim(string_ltrim(s));
}

std::string string_Utf8ToMultiByte(std::string utf8_str)
{
    std::string resultString; char* pszIn = new char[utf8_str.length() + 1];
    strncpy_s(pszIn, utf8_str.length() + 1, utf8_str.c_str(), utf8_str.length());
    int nLenOfUni = 0, nLenOfANSI = 0; wchar_t* uni_wchar = NULL;
    char* pszOut = NULL;
    // 1. utf8 Length
    if ((nLenOfUni = MultiByteToWideChar(CP_UTF8, 0, pszIn, (int)strlen(pszIn), NULL, 0)) <= 0)
        return nullptr;
    uni_wchar = new wchar_t[nLenOfUni + 1];
    memset(uni_wchar, 0x00, sizeof(wchar_t) * (nLenOfUni + 1));
    // 2. utf8 --> unicode
    nLenOfUni = MultiByteToWideChar(CP_UTF8, 0, pszIn, (int)strlen(pszIn), uni_wchar, nLenOfUni);
    // 3. ANSI(multibyte) Length
    if ((nLenOfANSI = WideCharToMultiByte(CP_ACP, 0, uni_wchar, nLenOfUni, NULL, 0, NULL, NULL)) <= 0)
    {
        delete[] uni_wchar; return 0;
    }
    pszOut = new char[nLenOfANSI + 1];
    memset(pszOut, 0x00, sizeof(char) * (nLenOfANSI + 1));
    // 4. unicode --> ANSI(multibyte)
    nLenOfANSI = WideCharToMultiByte(CP_ACP, 0, uni_wchar, nLenOfUni, pszOut, nLenOfANSI, NULL, NULL);
    pszOut[nLenOfANSI] = 0;
    resultString = pszOut;
    delete[] uni_wchar;
    delete[] pszOut;
    return resultString;
}

std::string string_MultiByteToUtf8(std::string multibyte_str)
{
    char* pszIn = new char[multibyte_str.length() + 1];
    strncpy_s(pszIn, multibyte_str.length() + 1, multibyte_str.c_str(), multibyte_str.length());

    std::string resultString;

    int nLenOfUni = 0, nLenOfUTF = 0;
    wchar_t* uni_wchar = NULL;
    char* pszOut = NULL;

    // 1. ANSI(multibyte) Length
    if ((nLenOfUni = MultiByteToWideChar(CP_ACP, 0, pszIn, (int)strlen(pszIn), NULL, 0)) <= 0)
        return 0;

    uni_wchar = new wchar_t[nLenOfUni + 1];
    memset(uni_wchar, 0x00, sizeof(wchar_t) * (nLenOfUni + 1));

    // 2. ANSI(multibyte) ---> unicode
    nLenOfUni = MultiByteToWideChar(CP_ACP, 0, pszIn, (int)strlen(pszIn), uni_wchar, nLenOfUni);

    // 3. utf8 Length
    if ((nLenOfUTF = WideCharToMultiByte(CP_UTF8, 0, uni_wchar, nLenOfUni, NULL, 0, NULL, NULL)) <= 0)
    {
        delete[] uni_wchar;
        return 0;
    }

    pszOut = new char[nLenOfUTF + 1];
    memset(pszOut, 0, sizeof(char) * (nLenOfUTF + 1));

    // 4. unicode ---> utf8
    nLenOfUTF = WideCharToMultiByte(CP_UTF8, 0, uni_wchar, nLenOfUni, pszOut, nLenOfUTF, NULL, NULL);
    pszOut[nLenOfUTF] = 0;
    resultString = pszOut;

    delete[] uni_wchar;
    delete[] pszOut;

    return resultString;
}

size_t string_UTF8ByteLength(const std::string& str)
{
    size_t uint8_byte_count = 0;
    for (int i = 0; i < str.length();)
    {
        // 4바이트 문자인지 확인
        // 0xF0 = 1111 0000
        if (0xF0 == (0xF0 & str[i]))
        {
            // 나머지 3바이트 확인
            // 0x80 = 1000 0000
            if (0x80 != (0x80 & str[i + 1]) || 0x80 != (0x80 & str[i + 2]) || 0x80 != (0x80 & str[i + 3]))
            {
                throw std::exception("not utf-8 encoded string");
            }
            i += 4;
            uint8_byte_count += 4;
            continue;
        }
        // 3바이트 문자인지 확인
        // 0xE0 = 1110 0000
        else if (0xE0 == (0xE0 & str[i]))
        {
            // 나머지 2바이트 확인
            // 0x80 = 1000 0000
            if (0x80 != (0x80 & str[i + 1]) || 0x80 != (0x80 & str[i + 2]))
            {
                throw std::exception("not utf-8 encoded string");
            }
            i += 3;
            uint8_byte_count += 3;
            continue;
        }
        // 2바이트 문자인지 확인
        // 0xC0 = 1100 0000
        else if (0xC0 == (0xC0 & str[i]))
        {
            // 나머지 1바이트 확인
            // 0x80 = 1000 0000
            if (0x80 != (0x80 & str[i + 1]))
            {
                throw std::exception("not utf-8 encoded string");
            }
            i += 2;
            uint8_byte_count += 2;
            continue;
        }
        // 최상위 비트가 0인지 확인
        else if (0 == (str[i] >> 7))
        {
            i += 1;
            uint8_byte_count += 1;
        }
        else
        {
            throw std::exception("not utf-8 encoded string");
        }
    }
    return uint8_byte_count;
}

size_t string_UTF8Length(const std::string& str)
{
    size_t utf8_char_count = 0;
    for (int i = 0; i < str.length();)
    {
        // 4바이트 문자인지 확인
        // 0xF0 = 1111 0000
        if (0xF0 == (0xF0 & str[i]))
        {
            // 나머지 3바이트 확인
            // 0x80 = 1000 0000
            if (0x80 != (0x80 & str[i + 1]) || 0x80 != (0x80 & str[i + 2]) || 0x80 != (0x80 & str[i + 3]))
            {
                throw std::exception("not utf-8 encoded string");
            }
            i += 4;
            utf8_char_count++;
            continue;
        }
        // 3바이트 문자인지 확인
        // 0xE0 = 1110 0000
        else if (0xE0 == (0xE0 & str[i]))
        {
            // 나머지 2바이트 확인
            // 0x80 = 1000 0000
            if (0x80 != (0x80 & str[i + 1]) || 0x80 != (0x80 & str[i + 2]))
            {
                throw std::exception("not utf-8 encoded string");
            }
            i += 3;
            utf8_char_count++;
            continue;
        }
        // 2바이트 문자인지 확인
        // 0xC0 = 1100 0000
        else if (0xC0 == (0xC0 & str[i]))
        {
            // 나머지 1바이트 확인
            // 0x80 = 1000 0000
            if (0x80 != (0x80 & str[i + 1]))
            {
                throw std::exception("not utf-8 encoded string");
            }
            i += 2;
            utf8_char_count++;
            continue;
        }
        // 최상위 비트가 0인지 확인
        else if (0 == (str[i] >> 7))
        {
            i += 1;
            utf8_char_count++;
        }
        else
        {
            throw std::exception("not utf-8 encoded string");
        }
    }
    return utf8_char_count;
}