#include "json_test.h"

extern "C" {
    #include "../include/cpu.h"
}

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
        auto initial = test["initial"];
        set_registers(
            initial["a"].get<byte>(), 
            initial["b"].get<byte>(), 
            initial["c"].get<byte>(), 
            initial["d"].get<byte>(), 
            initial["e"].get<byte>(), 
            initial["f"].get<byte>(), 
            initial["h"].get<byte>(), 
            initial["l"].get<byte>(),
            initial["pc"].get<byte>(),
            initial["sp"].get<byte>()
        );
    }
    print_registers();

    return true;
}