// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com

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
    , "2018-01-01 8:03:35-0800"
    , "2018-02-01 8:03:35-0800"
    , "2018-03-01 8:03:35-0800"
    , "2018-04-01 8:03:35-0800"
    , "2018-05-01 8:03:35-0800"
    , "2018-06-01 8:03:35-0800"
    , "2018-07-01 8:03:35-0800"
    , "2018-08-01 8:03:35-0800"
    , "2018-09-01 8:03:35-0800"
    , "2018-10-01 8:03:35-0800"
    , "2018-11-01 8:03:35-0800"
    , "2018-12-01 8:03:35-0800"    
    , "2018-12-31 8:03:35-0800"
};

int
main( int argc, char ** argv )
{
    std::cout << std::setw(24) << "input" << "\t->\tresult" << std::endl;

    for ( auto& p: inputs ) {
        auto save( p );
        std::tm tm { 0 };
        auto status = date_time::parse( p, tm );

        const int year = tm.tm_year + 1900;
        const int month = tm.tm_mon + 1;

        std::cout << std::setw(24) << std::setfill(' ') << save << "\t->\t";
        if ( status & date_time::date_time_date )
            std::cout << std::setfill('0') << std::setw(2) << year << "-" << std::setw(2) << month << "-" << std::setw(2) << tm.tm_mday;
        else
            std::cout << "----------";
        if ( status & date_time::date_time_time ) {
            auto tz = tm.tm_gmtoff >= 0 ? tm.tm_gmtoff : -tm.tm_gmtoff;
            std::cout << "T"
                      << std::setw(2) << std::setfill('0' )
                      << std::setw(2) << tm.tm_hour << ":"
                      << std::setw(2) << tm.tm_min << ":"
                      << std::setw(2) << tm.tm_sec
                      << (tm.tm_gmtoff >= 0 ? "+" : "-" ) << std::setw(4) << tz;
        }
        std::array< char, 30 > str;
        std::strftime( str.data(), str.size(), "%c", &tm );
        std::cout << "\tstrftime: " << str.data() << std::endl;
    }
}

