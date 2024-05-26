#include "csv_reader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

csv_reader::csv_reader(const std::string& filename) : filename(filename) {}

bool csv_reader::read_csv() {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file\n";
        return false;
    }

    std::string line;
    getline(file, line); // Skip the header line

    while (getline(file, line)) {
        std::stringstream ss(line);
        Data entry;
        std::string temp;

        getline(ss, entry.time, ',');
        getline(ss, temp, ','); entry.minute = std::stoi(temp);
        getline(ss, temp, ','); entry.temperature = std::stof(temp);
        getline(ss, temp, ','); entry.snow_amount = std::stof(temp);
        getline(ss, temp, ','); entry.light_intensity = std::stof(temp);
        getline(ss, temp, ','); entry.light_direction_x = std::stof(temp);
        getline(ss, temp, ','); entry.light_direction_y = std::stof(temp);
        getline(ss, temp, ','); entry.light_direction_z = std::stof(temp);

        dataEntries.push_back(entry);
    }

    file.close();
    return true;
}

const std::vector<Data>& csv_reader::getData() const {
    return dataEntries;
}
