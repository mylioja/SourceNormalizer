#include <experimental/filesystem>
#include <iostream>
#include <regex>
#include <string>

namespace fs = std::experimental::filesystem;

namespace {

    const auto cpp_regex = std::regex(
        "\\.(h|hpp|c|cc|cpp)",
        std::regex::optimize | std::regex::extended
    );


    bool is_source_file(const fs::path& path)
    {
        std::string ext = path.extension().string();
        return std::regex_match(ext, cpp_regex);
    }


    int scan_for_files(std::string& root_dir)
    {
        auto dir_iter = fs::recursive_directory_iterator(root_dir);
        for (const auto& entry: dir_iter)
        {
            if (fs::is_regular_file(entry.status()) && is_source_file(entry.path()))
            {
                std::cout << entry.path() << '\n';
            }
        }

        return 0;
    }

}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "usage: source_normalizer directory\n";
        return 1;
    }

    std::string root_dir = argv[1];

    int err = scan_for_files(root_dir);

    return err;
}