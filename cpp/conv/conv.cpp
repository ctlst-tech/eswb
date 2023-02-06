#include "conv.hpp"

#include <fstream>

int eswb::ConverterToCsv::convert(void) {
    int rv;
    std::ifstream raw_log;
    std::ofstream csv_log;
    std::string raw_string;

    raw_log.open(this->m_path_to_raw, std::ios::in);
    csv_log.open(this->m_path_to_csv, std::ios::out | std::ios::trunc);

    if (!raw_log.is_open()) {
        std::cout << "Failed to open raw log: " << this->m_path_to_raw
                  << std::endl;
        return -1;
    }

    if (!csv_log.is_open()) {
        std::cout << "Failed to create csv log: " << this->m_path_to_csv
                  << std::endl;
        return -1;
    }

    std::getline(raw_log, this->m_bus);
    std::cout << "Bus: " << this->m_bus << std::endl;

    while (std::getline(raw_log, raw_string)) {
        
    }

    raw_log.close();
    csv_log.close();

    return rv;
}
