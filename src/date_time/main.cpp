
#include "date_time.hpp"
#include <iostream>

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
    for ( auto& p: inputs ) {
        auto save( p );
        int year, month, day, hour, min, second, tzoffs;
        auto status = parse_date( p, year, month, day, hour, min, second, tzoffs );
        std::cout << save << "\t->\t";
        if ( status.first )
            std::cout << year << "-" << month << "-" << day << " ";
        if ( status.second )
            std::cout << hour << ":" << min << ":" << second << (tzoffs >= 0 ? "+" : "" ) << tzoffs;
        std::cout << std::endl;
    }
}

