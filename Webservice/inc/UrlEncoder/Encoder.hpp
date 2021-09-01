#pragma once
#include <string>
#include <codecvt>
#include <locale>

class Encoder
{
public:
    Encoder() {}
    ~Encoder() {}

    std::string UrlEncode(const std::string &str);
    std::string UrlDecode(const std::string &str);

    std::string UTF8UrlEncode(const std::string &str);
    std::string UTF8UrlDecode(const std::string &str);

    std::string dotNetUrlEncode(const std::string& str);
    std::string dotNetUrlDecode(const std::string& str);

private:

    std::string UTF8StringToAnsiString(const std::string &strUtf8, const std::string& loc = std::locale{}.name());
    std::string AnsiStringToUTF8String(const std::string& strAnsi, const std::string& loc = std::locale{}.name());

    char CharToInt(char ch);
    char StrToBin(char *pString);

};

using namespace std;
string Encoder::UrlEncode(const string& str)
{
    string strResult;
    size_t nLength = str.length();
    unsigned char* pBytes = (unsigned char*)str.c_str();
    char szAlnum[2];
    char szOther[4];
    for (size_t i = 0; i < nLength; i++)
    {
        if (isalnum((unsigned char)str[i]))
        {
            sprintf(szAlnum, "%c", str[i]);
            strResult.append(szAlnum);
        }
        else if (isspace((unsigned char)str[i]))
        {
            strResult.append("+");
        }
        else
        {
            sprintf(szOther, "%%%X%X", pBytes[i] >> 4, pBytes[i] % 16);
            strResult.append(szOther);
        }
    }
    return strResult;
}

string Encoder::UrlDecode(const string& str)
{
    string strResult;
    char szTemp[2];
    size_t i = 0;
    size_t nLength = str.length();
    while (i < nLength)
    {
        if (str[i] == '%')
        {
            szTemp[0] = str[i + 1];
            szTemp[1] = str[i + 2];
            strResult += StrToBin(szTemp);
            i = i + 3;
        }
        else if (str[i] == '+')
        {
            strResult += ' ';
            i++;
        }
        else
        {
            strResult += str[i];
            i++;
        }
    }
    return strResult;
}

string Encoder::UTF8UrlEncode(const string& str)
{
    return UrlEncode(AnsiStringToUTF8String(str));
}

string Encoder::UTF8UrlDecode(const string& str)
{
    return UTF8StringToAnsiString(UrlDecode(str));
}

std::string Encoder::AnsiStringToUTF8String(const std::string& str, const std::string& loc)
{
    using namespace std;
    class mycodecvt : public codecvt_byname<wchar_t, char, mbstate_t> {
    public:
        mycodecvt(const string& loc) : codecvt_byname(loc) { }
    };
    wstring_convert<codecvt_utf8<wchar_t>> Conver_UTF8;
    wstring_convert<codecvt_byname<wchar_t, char, mbstate_t>> Conver_Locale(new mycodecvt(loc));
    return Conver_UTF8.to_bytes(Conver_Locale.from_bytes(str));
}

std::string Encoder::UTF8StringToAnsiString(const std::string& str, const std::string& loc)
{
    using namespace std;
    class mycodecvt : public codecvt_byname<wchar_t, char, mbstate_t> {
    public:
        mycodecvt(const string& loc) : codecvt_byname(loc) { }
    };
    wstring_convert<codecvt_utf8<wchar_t>> Conver_UTF8;
    wstring_convert<codecvt_byname<wchar_t, char, mbstate_t>> Conver_Locale(new mycodecvt(loc));
    return Conver_Locale.to_bytes(Conver_UTF8.from_bytes(str));
}

char Encoder::CharToInt(char ch)
{
    if (ch >= '0' && ch <= '9')
    {
        return (char)(ch - '0');
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return (char)(ch - 'a' + 10);
    }
    if (ch >= 'A' && ch <= 'F')
    {
        return (char)(ch - 'A' + 10);
    }
    return -1;
}

char Encoder::StrToBin(char* pString)
{
    char szBuffer[2];
    char ch;
    szBuffer[0] = CharToInt(pString[0]); //make the B to 11 -- 00001011 
    szBuffer[1] = CharToInt(pString[1]); //make the 0 to 0 -- 00000000 
    ch = (szBuffer[0] << 4) | szBuffer[1]; //to change the BO to 10110000 
    return ch;
}

string Encoder::dotNetUrlEncode(const string& str)
{
    const char IGNORED_CHARS[] = { '-','_','.','!','*','(',')','\0' };
    string strResult;
    size_t nLength = str.length();
    unsigned char* pBytes = (unsigned char*)str.c_str();
    char szAlnum[2];
    char szOther[4];
    for (size_t i = 0; i < nLength; i++)
    {
        if (isalnum((unsigned char)str[i]) || std::strchr(IGNORED_CHARS, str[i]) != nullptr)
        {
            sprintf(szAlnum, "%c", str[i]);
            strResult.append(szAlnum);
        }
        else if (isspace((unsigned char)str[i]))
        {
            strResult.append("+");
        }
        else
        {
            sprintf(szOther, "%%%X%X", pBytes[i] >> 4, pBytes[i] % 16);
            strResult.append(szOther);
        }
    }
    return strResult;
}


string Encoder::dotNetUrlDecode(const string& str)
{
    return UrlDecode(str);
}
