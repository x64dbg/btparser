#pragma once

#include <string>
#include <vector>
#include <windows.h>
#include <cstdint>

namespace StringUtils
{
    static std::string sprintf(const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        std::vector<char> buffer(256, '\0');
        while(true)
        {
            int res = _vsnprintf_s(buffer.data(), buffer.size(), _TRUNCATE, format, args);
            if(res == -1)
            {
                buffer.resize(buffer.size() * 2);
                continue;
            }
            else
                break;
        }
        va_end(args);
        return std::string(buffer.data());
    }

    static std::string Escape(const std::string & s)
    {
        auto escape = [](unsigned char ch) -> std::string
        {
            char buf[8] = "";
            switch(ch)
            {
            case '\0':
                return "\\0";
            case '\t':
                return "\\t";
            case '\f':
                return "\\f";
            case '\v':
                return "\\v";
            case '\n':
                return "\\n";
            case '\r':
                return "\\r";
            case '\\':
                return "\\\\";
            case '\"':
                return "\\\"";
            default:
                if(!isprint(ch)) //unknown unprintable character
                    sprintf_s(buf, "\\x%02X", ch);
                else
                    *buf = ch;
                return buf;
            }
        };
        std::string escaped;
        escaped.reserve(s.length() + s.length() / 2);
        for(size_t i = 0; i < s.length(); i++)
            escaped.append(escape((unsigned char)s[i]));
        return escaped;
    }

    static std::string Utf16ToUtf8(const std::wstring & wstr)
    {
        std::string convertedString;
        auto requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if(requiredSize > 0)
        {
            std::vector<char> buffer(requiredSize);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, nullptr, nullptr);
            convertedString.assign(buffer.begin(), buffer.end() - 1);
        }
        return convertedString;
    }

    static std::wstring Utf8ToUtf16(const std::string & str)
    {
        std::wstring convertedString;
        int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        if(requiredSize > 0)
        {
            std::vector<wchar_t> buffer(requiredSize);
            MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], requiredSize);
            convertedString.assign(buffer.begin(), buffer.end() - 1);
        }
        return convertedString;
    }
};

namespace FileHelper
{
    static bool ReadAllData(const std::string & fileName, std::vector<uint8_t> & content)
    {
        auto hFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        auto result = false;
        if(hFile != INVALID_HANDLE_VALUE)
        {
            unsigned int filesize = GetFileSize(hFile, nullptr);
            if(!filesize)
            {
                content.clear();
                result = true;
            }
            else
            {
                content.resize(filesize);
                DWORD read = 0;
                result = !!ReadFile(hFile, content.data(), filesize, &read, nullptr);
            }
            CloseHandle(hFile);
        }
        return result;
    }

    static bool WriteAllData(const std::string & fileName, const void* data, size_t size)
    {
        auto hFile = CreateFileW(StringUtils::Utf8ToUtf16(fileName).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
        auto result = false;
        if(hFile != INVALID_HANDLE_VALUE)
        {
            DWORD written = 0;
            result = !!WriteFile(hFile, data, DWORD(size), &written, nullptr);
            CloseHandle(hFile);
        }
        return result;
    }

    static bool ReadAllText(const std::string & fileName, std::string & content)
    {
        std::vector<unsigned char> data;
        if(!ReadAllData(fileName, data))
            return false;
        data.push_back(0);
        content = std::string((const char*)data.data());
        return true;
    }

    static bool WriteAllText(const std::string & fileName, const std::string & content)
    {
        return WriteAllData(fileName, content.c_str(), content.length());
    }
};