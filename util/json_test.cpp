#include "json_test.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
//#include <iomanip>
//#include <sstream>
#include <filesystem>

void run_json_tests() {
    namespace fs = std::filesystem;

    std::string directory = "../tests/sm83/v1/";
    std::string filename = "00.json";

    fs::path filePath = directory + filename;
    if (!fs::exists(filePath)) {
        std::cerr << "Failed to open " << filename << std::endl;
    }
}

bool process_json_file(const std::filesystem::path& filePath) {
    using json = nlohmann::json;
    std::ifstream f(filePath);
    if (!f.is_open()) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return false;
    }

    json data;
    f >> data;

    for(const auto& test : data ){
        
    }

    return true;
}