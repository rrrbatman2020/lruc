#include <iostream>
#include <string>

#include "http_file_downloader.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Try " << argv[0] << " <url> <output_file_name>" << std::endl;
        return 0;
    }

    const std::string url(argv[1]);
    const std::string outputFilePath(argv[2]);

    try {
        THttpFileDownloader downloader(url);
        downloader.Download(outputFilePath);
        std::cout << "OK" << std::endl;

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "An error occurred: " << error.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error" << std::endl;
    }

    return -1;
}
