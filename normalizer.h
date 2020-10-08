#pragma once

#include <experimental/filesystem>
#include <string>
#include <vector>

class Normalizer
{
public:
    void normalize(const std::experimental::filesystem::path& path);

private:
    using Buffer = std::vector<char>;

    bool load_file(const std::experimental::filesystem::path& path);

    //  Returns false if no errors found
    bool find_errors();

    int data_size() const { return int(m_data.size()); }

    bool is_utf16() const;

    void add_error_message(const char* text);

    unsigned m_errors;
    std::string m_full_name;
    std::string m_error_message;
    Buffer m_data;
};
