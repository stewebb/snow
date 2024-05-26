#ifndef CSV_READER_H
#define CSV_READER_H

#include <vector>
#include <string>

struct Data {
    std::string time;
    int minute;
    float temperature;
    float snow_amount;
    float light_intensity;
    float elevation_angle;
    float light_direction_x;
    float light_direction_y;
    float light_direction_z;
    float sky_color_r;
    float sky_color_g;
    float sky_color_b;
    float sun_color_r;
    float sun_color_g;
    float sun_color_b;
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
