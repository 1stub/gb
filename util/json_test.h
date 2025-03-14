#pragma once

#include <iostream>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

void run_json_tests();
void process_json_file(const std::filesystem::path& filePath);