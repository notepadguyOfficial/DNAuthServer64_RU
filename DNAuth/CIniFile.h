#pragma once
#include <Windows.h>
#include <string>
#include <stdexcept>

class CIniFile {
public:
    explicit CIniFile(const std::wstring& filePath);
    ~CIniFile();

    std::string GetValue(const std::wstring& section, const std::wstring& key, const std::string& defaultValue = "") const;
    int GetIntValue(const std::wstring& section, const std::wstring& key, int defaultValue = 0) const;

private:
    std::wstring m_FilePath;
};