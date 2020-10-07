#include <experimental/filesystem>
#include <iostream>
#include <string>

namespace fs = std::experimental::filesystem;

namespace {

    int scan_files(std::string& root_dir)
    {
        auto dir_iter = fs::recursive_directory_iterator(root_dir);
        for (const auto& p: dir_iter)
        {
            std::cout << p.path() << '\n';
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

    int err = scan_files(root_dir);

    return err;
}