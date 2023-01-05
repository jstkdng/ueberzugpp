#include <iostream>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <vips/vips8>

#include "CLI/App.hpp"
#include "logging.hpp"

using json = nlohmann::json;

int main(int argc, char *argv[])
{
    if (VIPS_INIT(argv[0])) {
        vips_error_exit(NULL);
    }
    //
    // if (argc != 3) {
    //     vips_error_exit ("usage: %s input-file output-filee", argv[0]);
    // }
    //
    // VImage in = VImage::new_from_file(argv[1]);
    //
    // std::cout << "width: " << in.width() << std::endl;
    // std::cout << "height: " << in.height() << std::endl;
    //
    // VImage thumb = in.thumbnail_image(500);
    // std::cout << "width: " << thumb.width() << std::endl;
    // std::cout << "height: " << thumb.height() << std::endl;
    //
    // thumb.write_to_file(argv[2]);

    Logging logger;

    std::string cmd;
    std::stringstream ss;
    json j;
    while (true) {
        std::cin >> cmd;
        ss << cmd;
        try {
            j = json::parse(ss.str());
            // clean stream
            ss.str(std::string());
            logger.log(j.dump());
        } catch (json::parse_error e) {
            continue;
        }
    }
    return 0;
}
