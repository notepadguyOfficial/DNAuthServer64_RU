#include "CIniFile.h"

CIniFile::CIniFile(const std::wstring& filePath) : m_FilePath(filePath) {
    if (m_FilePath.empty()) {
        throw std::invalid_argument("INI file path cannot be empty.");
    }
}

CIniFile::~CIniFile() = default;

std::string CIniFile::GetValue(const std::wstring& section, const std::wstring& key, const std::string& defaultValue) const {
    wchar_t buffer[256];
    GetPrivateProfileStringW(section.c_str(), key.c_str(), std::wstring(defaultValue.begin(), defaultValue.end()).c_str(),
                             buffer, sizeof(buffer) / sizeof(wchar_t), m_FilePath.c_str());
    return std::string(buffer, buffer + wcslen(buffer));
}

int CIniFile::GetIntValue(const std::wstring& section, const std::wstring& key, int defaultValue) const {
    return GetPrivateProfileIntW(section.c_str(), key.c_str(), defaultValue, m_FilePath.c_str());
}
