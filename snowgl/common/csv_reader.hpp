#ifndef CSV_READER_H
#define CSV_READER_H

#include <vector>
#include <string>

struct Data {
    std::string time;
    int minute;
    float temperature;
    float snow_amount;
};

class csv_reader {
private:
    std::vector<Data> dataEntries;
    std::string filename;

public:
    csv_reader(const std::string& filename);
    bool read_csv();
    const std::vector<Data>& getData() const;
};

#endif // CSV_READER_H
