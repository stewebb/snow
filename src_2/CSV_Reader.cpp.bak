#include "CSV_Reader.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

CSV_Reader::CSV_Reader(const std::string& filename) : filename(filename) {}

bool CSV_Reader::readCSV() {
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
        getline(ss, temp, ','); entry.lightIntensityR = std::stoi(temp);
        getline(ss, temp, ','); entry.lightIntensityG = std::stoi(temp);
        getline(ss, temp, ','); entry.lightIntensityB = std::stoi(temp);
        getline(ss, temp, ','); entry.lightAngle = std::stof(temp);
        getline(ss, temp, ','); entry.backgroundColorR = std::stoi(temp);
        getline(ss, temp, ','); entry.backgroundColorG = std::stoi(temp);
        getline(ss, temp, ','); entry.backgroundColorB = std::stoi(temp);

        dataEntries.push_back(entry);
    }

    file.close();
    return true;
}

const std::vector<Data>& CSV_Reader::getData() const {
    return dataEntries;
}
