// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com

#include "date_time.hpp"
#include <iostream>
#include <iomanip>

constexpr const char * inputs [] = {
    "1969-12-31 09:00:00+0000"
    , "1752-12-31 18:00:00+0000"
    , "1970-01-01 00:00:00"             // epoch
    , "1970-12-01 00:00:10"           
    , "1971-01-01 00:00:15"             // epoch + 1    
    , "2018-03-20 18:03:30"
    , "2018-03-20\t18:03:45"
    , "2019-04-20T8:03:35"
    , "2020-05-20"
    , "9:15"
    , "9:15:01"
    , "20210102123031"
    , "2019-04-20T8:03:35+0900"
    , "2018-01-01 8:03:35+0000"
    , "2018-02-01 8:03:35-0000"
    , "2018-03-01 8:03:35-0000"
    , "2038-01-19 03:14:07+0000"
    , "2058-12-31 8:03:35-0800"
    , "3001-12-31 8:03:35-0800"
};

int
main( int argc, char ** argv )
{
    std::cout << "Although gmt-offset is pased, but it does not takeing in account for the calendar calculation.\n";
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
        
        std::array< char, 30 > str, str2;
        std::strftime( str.data(), str.size(), "%FT%TZ", &tm );
        std::cout << "\tstrftime: " << str.data();// << std::endl;

        std::array< char, 30 > tstr;
        date_time::to_string( tstr.data(), tstr.size(), tm );
        std::cout << "\t" << tstr.data()
                  << ( ( std::string( str.data() ) == std::string( tstr.data() ) ) ? "\tOK" : "\tFAIL" ) << std::endl;

        // revserse conversion ( tm -> time_t )
        std::time_t t = date_time::time( tm );
        std::strftime( str2.data(), str2.size(), "%FT%TZ", gmtime(&t) );
        std::cout << "\t\t\t\t\t\t\t\t\t        : " <<  str2.data()
                  << ( ( std::string( str.data() ) == std::string( str2.data() ) ) ? "\tOK" : "\tFAIL" )
                  << "\t"; //std::endl;

        struct tm tm2;
        std::array< char, 30 > str3;
        std::strftime( str3.data(), str3.size(), "%FT%TZ", date_time::gmtime( t, tm2 ) );
        std::cout << "\t" <<  str3.data()
                  << ( ( std::string( str3.data() ) == std::string( str2.data() ) ) ? "\tOK" : "\tFAIL" )
                  << std::endl;
    }
}

