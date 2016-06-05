#ifndef _FILEREADER_H
#define _FILEREADER_H

#include <string>
#include <vector>

using String = std::string;

class FileHelper
{
public:
    static bool ReadAllData(const String & fileName, std::vector<unsigned char> & content);
    static bool WriteAllData(const String & fileName, const void* data, size_t size);
    static bool ReadAllText(const String & fileName, String & content);
    static bool WriteAllText(const String & fileName, const String & content);
};

#endif //_FILEREADER_H