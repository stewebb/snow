#ifndef CSV_READER_HPP
#define CSV_READER_HPP

#include <vector>
#include <string>

struct Data {
    std::string time;
    int minute;
    int lightIntensityR;
    int lightIntensityG;
    int lightIntensityB;
    float lightAngle;
    int backgroundColorR;
    int backgroundColorG;
    int backgroundColorB;
};

class CSV_Reader {
private:
    std::vector<Data> dataEntries;
    std::string filename;

public:
    CSV_Reader(const std::string& filename);
    bool readCSV();
    const std::vector<Data>& getData() const;
};

#endif // CSV_READER_HPP
