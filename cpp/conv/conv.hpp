#ifndef CONV_HPP
#define CONV_HPP

#include <iostream>
#include <thread>

#include "eswb.hpp"
#include "eswb/services/eqrb.h"
#include "eswb/services/sdtl.h"

namespace eswb {

class ConverterToCsv {
private:
    const std::string m_path_to_raw;
    const std::string m_path_to_csv;
    const std::string m_frame_sep;

private:
    std::string m_bus;

public:
    ConverterToCsv(const std::string &path_to_raw,
                   const std::string &path_to_csv)
        : m_path_to_csv(path_to_csv), m_path_to_raw(path_to_raw), m_frame_sep("ebdf") {}

    ConverterToCsv(const std::string &path_to_raw,
                   const std::string &path_to_csv,
                   const std::string &frame_sep)
        : m_path_to_csv(path_to_csv), m_path_to_raw(path_to_raw), m_frame_sep(frame_sep) {}

public:
    int convert(void);
};

}  // namespace eswb

#endif  // CONV_HPP
