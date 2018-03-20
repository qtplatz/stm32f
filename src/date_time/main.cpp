
#include "date_time.hpp"
#include <iostream>
#include <iomanip>

constexpr const char * inputs [] = {
    "2018-03-20 18:03:30"
    , "2018-03-20\t18:03:30"
    , "2019-04-20T8:03:35"
    , "2020-05-20"
    , "9:15"
    , "9:15:01"
    , "20210102123031"
    , "2019-04-20T8:03:35+0900"
    , "2019-04-20T8:03:35-0800"
};

int
main( int argc, char ** argv )
{
    std::cout << std::setw(24) << "input" << "\t->\tresult" << std::endl;

    for ( auto& p: inputs ) {
        auto save( p );
        int year, month, day, hour, min, second, tzoffs;
        auto status = date_time::parse_date( p, year, month, day, hour, min, second, tzoffs );
        std::cout << std::setw(24) << std::setfill(' ') << save << "\t->\t";
        if ( status.first )
            std::cout << std::setfill('0') << std::setw(2) << year << "-" << std::setw(2) << month << "-" << std::setw(2) << day;
        else
            std::cout << "----------";
        if ( status.second ) {
            auto tz = tzoffs >= 0 ? tzoffs : -tzoffs;
            std::cout << "T"
                      << std::setw(2) << std::setfill('0' )
                      << std::setw(2) << hour << ":"
                      << std::setw(2) << min << ":"
                      << std::setw(2) << second
                      << (tzoffs >= 0 ? "+" : "-" ) << std::setw(4) << tz;
        }
        std::cout << std::endl;
    }
}

