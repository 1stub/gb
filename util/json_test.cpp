#include "json_test.h"

extern "C" {
    #include "../include/cpu.h"
    #include "../include/mmu.h"
}

static inline void iterate_files(std::string dir, std::string prefix, bool isextended)
{
    for(int i = 0; i <= 0xFF; i++) {
        if(i == 0xCB && !isextended) {
            i++;
            iterate_files(dir, "cb ", true);    
        }

        //skip stop and halt
        if(!isextended) {
            if(i == 0x10) continue;
        }

        std::stringstream ss;
        ss << prefix;
        ss << std::hex << std::setw(2) << std::setfill('0') << i << ".json";
        std::string filename = ss.str();

        std::filesystem::path filePath = dir + filename;
        if (!std::filesystem::exists(filePath)) {
            std::cerr << "Failed to open " << filename << std::endl;
        }

        std::cout << "running test " << "0x"<< prefix <<std::hex << std::setw(2) << std::setfill('0') << i << std::endl;
        process_json_file(filePath);
    }
}

void run_json_tests() {
    std::string directory = "../tests/sm83/v1/";
    iterate_files(directory, "", false);
    printf("congrats! you passed the sm83 cpu instructions test!\n");
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

        for(const auto& ram : initial["ram"]) {
            word addr = ram[0].get<word>();
            byte data = ram[1].get<byte>();
            mem_write(addr, data);
        }

        const char* log = log_registers();
        //Note: I do not handle anything related to r/w/whatever pins
        for(const auto& cycle : test["cycles"]) {
            cpu_cycle();
            //print_registers();
        }
        
        //Maybe clean this up - doesn't really matter though
        //BIG NOTE: could need to check IME and EI
        auto final = test["final"];
        if(!check_cpu_registers(
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
        )) {
            printf("initial state: %s", log);
            printf("failure state: "); print_registers();
            printf("expected final state:");
            printf("A:%2X F:%2X B:%2X C:%2X D:%2X E:%2X H:%2X L:%2X SP:%4X PC:%4X\n",
                   final["a"].get<byte>(),
                   final["f"].get<byte>(),
                   final["b"].get<byte>(),
                   final["c"].get<byte>(),
                   final["d"].get<byte>(),
                   final["e"].get<byte>(),
                   final["h"].get<byte>(),
                   final["l"].get<byte>(),
                   final["sp"].get<word>(),
                   final["pc"].get<word>());

            find_wrong_register(
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
        }

        for(const auto& ram : final["ram"]) {
            word addr = ram[0].get<word>();
            byte data = ram[1].get<byte>();
            if(mem_read(addr) != data) {
                printf("failed with address %x\n", addr);
                printf("data was %x\n", data);
                assert(false);
            }
        }
    }
}