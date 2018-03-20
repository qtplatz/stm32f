// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//


#include "date_time.hpp"
#include <numeric>

static constexpr int __days_in_month [2][12] = {
    // Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
    {   31,  28,  31,  30,  31,  30,  31,  31,  30,  31,  31,  31 }
    , { 31,  29,  31,  30,  31,  30,  31,  31,  30,  31,  31,  31 }
};

// Reference: https://groups.google.com/forum/#!msg/comp.lang.c/GPA5wwrVnVw/hi2wB0TXGkAJ

static inline int dayofweek( int y, int m, int d )  /* 1 <= m <= 12,  y > 1752 (in the U.K.) */
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    y -= m < 3;
    return (y + y/4 - y/100 + y/400 + t[m-1] + d) % 7;
}


static inline bool is_leap_year( int year )
{
    if ( year % 400 == 0 )
        return true;
    return ( ( year % 4 == 0 ) && ( year % 100 != 0 ) );
}

bool
date_time::parse_time( const char * s, int& hour, int& min, int& second, int& tzoffs )
{
    tzoffs = 0;
    if ( scanner().read( hour, s, 2 ) ) {          // hour
        scanner().skip< ':' >( s, 1 );
        if ( scanner().read( min, s, 2 ) ) {       // min
            scanner().skip< ':' >( s, 1 );
            scanner().read( second, s, 2 ); // second
            if ( *s ) {
                int sign = ( *s == '-' ) ? (-1) : 1;
                scanner().skip< 'Z', '+', '-' >( s, 1 );
                scanner().read( tzoffs, s, 4 );
                tzoffs *= sign;
            }
            return true;
        }
    }
    return false;
}

std::pair< bool, bool >
date_time::parse_date( const char * s
                       , int& year, int& month, int& day
                       , int& hour, int& min, int& second, int& tzoffs )
{
    year = month = day = hour = min = second = tzoffs = 0;

    std::pair< bool, bool > status{ false, false };
    
    auto save( s );
    if ( scanner().read( year, s, 4 ) ) {
        if ( *s == ':' ) {
            year = 0;
            status.second = parse_time( save, hour, min, second, tzoffs );
        } else {
            scanner().skip< '-' >( s, 1 );
            if ( scanner().read( month, s, 2 ) ) {
                scanner().skip< '-' >( s, 1 );
                if ( scanner().read( day, s, 2 ) ) {
                    status.first = true;
                    scanner().skip< ' ', '\t', 'T' >( s, 1 );
                    status.second = parse_time( s, hour, min, second, tzoffs );
                }
            }
        }
    }
    return status;
}

date_time::parse_state
date_time::parse( const char * s, tm& tm )
{
    int year, month, gmtoff;
    auto status = parse_date( s, year, month, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, gmtoff );

    if ( status.first ) {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;     // 0..11
        tm.tm_isdst = (-1);        // no information avilable
        tm.tm_gmtoff = gmtoff;
        tm.tm_wday = dayofweek( year, month, tm.tm_mday );
        size_t leap = is_leap_year(year) ? 1 : 0;
        
        tm.tm_yday = std::accumulate( __days_in_month[ leap ], __days_in_month[ leap ] + tm.tm_mon, tm.tm_mday );
    }

    return ( status.first && status.second ) ? date_time_both
        : ( status.first && !status.second ) ? date_time_date
        : ( !status.first && status.second ) ? date_time_time : date_time_none;
}
