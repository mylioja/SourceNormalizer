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


    //  Return true if the path denotes an uninteresting directory
    //  that should be skipped because it won't contain any sources
    bool should_be_skipped(const fs::path& path)
    {
        std::string name = path.filename().string();

        //  Skip if the name begins with a '.'
        if (name[0] == '.')
        {
            return true;
        }

        //  Skip the "build" directory
        if (name == "build")
        {
            return true;
        }

        return false;
    }


    bool is_source_file(const fs::path& path)
    {
        std::string ext = path.extension().string();
        return std::regex_match(ext, cpp_regex);
    }


    int scan_for_files(std::string& root_dir)
    {
        auto iter = fs::recursive_directory_iterator(root_dir);
        const auto end =  fs::recursive_directory_iterator();
        for (; iter != end; ++iter)
        {
            const auto& entry = *iter;
            if (fs::is_directory(entry.status()) && should_be_skipped(entry.path()))
            {
                iter.disable_recursion_pending();
                continue;
            }

            if (fs::is_regular_file(entry.status()) && is_source_file(entry.path()))
            {
                std::cout << "Examine: " << entry.path() << '\n';
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