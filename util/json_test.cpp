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

    process_json_file(filePath);
}

void process_json_file(const std::filesystem::path& filePath) {
    using json = nlohmann::json;
    std::ifstream f(filePath);
    if (!f.is_open()) {
        std::cerr << "Could not open file: " << filePath << std::endl;
        return ;
    }

    json data;
    f >> data;

    for(const auto& test : data ){
        auto initial = test["initial"];
        set_cpu_registers(
            initial["a"].get<byte>(), 
            initial["b"].get<byte>(), 
            initial["c"].get<byte>(), 
            initial["d"].get<byte>(), 
            initial["e"].get<byte>(), 
            initial["f"].get<byte>(), 
            initial["h"].get<byte>(), 
            initial["l"].get<byte>(),
            initial["pc"].get<word>(),
            initial["sp"].get<word>()
        );
        print_registers();

        for(const auto& ram : initial["ram"]) {
            word addr = ram[0].get<word>();
            byte data = ram[1].get<byte>();
            mem_write(addr, data);
        }

        for(const auto& cycle : test["cycles"]) {
            cpu_cycle();
        }
        print_registers();
        
        auto final = test["final"];
        check_cpu_registers(
            final["a"].get<byte>(), 
            final["b"].get<byte>(), 
            final["c"].get<byte>(), 
            final["d"].get<byte>(), 
            final["e"].get<byte>(), 
            final["f"].get<byte>(), 
            final["h"].get<byte>(), 
            final["l"].get<byte>(),
            final["pc"].get<word>(),
            final["sp"].get<word>()
        );

        for(const auto& ram : final["ram"]) {
            word addr = ram[0].get<word>();
            byte data = ram[1].get<byte>();
            assert(mem_read(addr) == data);
        }
    }
}