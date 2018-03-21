// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//
// Prepared for STM32F103 bare metal 

#include "date_time.hpp"
#include <numeric>

static constexpr int __days_in_month [2][12] = {
    // Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec
    {   31,  28,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31 }
    , { 31,  29,  31,  30,  31,  30,  31,  31,  30,  31,  30,  31 }
};

// Reference: https://groups.google.com/forum/#!msg/comp.lang.c/GPA5wwrVnVw/hi2wB0TXGkAJ

static inline int dayofweek( int y, int m, int d )  /* 1 <= m <= 12,  y > 1752 (in the U.K.) */
{
    static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
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

        auto leap = is_leap_year(year) ? 1 : 0;
        tm.tm_yday = std::accumulate( __days_in_month[ leap ], __days_in_month[ leap ] + tm.tm_mon, tm.tm_mday );
    }

    return ( status.first && status.second ) ? date_time_both
        : ( status.first && !status.second ) ? date_time_date
        : ( !status.first && status.second ) ? date_time_time : date_time_none;
}

std::time_t
date_time::time( const tm& tm )
{
    int days = 0;
    if ( tm.tm_year < 70 ) {
        for ( int year = 1970; year > ( 1900 + tm.tm_year + 1 ); --year )
            days -= is_leap_year( year ) ? 366 : 365;
        days -= ( is_leap_year( tm.tm_year + 1900 ) ? 366 : 365 ) - tm.tm_yday;
        return std::time_t( days - 1 ) * 24 * 3600 + (tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec);
    } else {
        for ( int year = 1970; year < ( 1900 + tm.tm_year ); ++year )
            days += is_leap_year( year ) ? 366 : 365;
        days += tm.tm_yday;
        return std::time_t( days - 1 ) * 24 * 3600 + (tm.tm_hour * 3600 + tm.tm_min * 60 + tm.tm_sec);
    }
}

std::tm *
date_time::gmtime( const std::time_t& time, tm& tm )
{
    tm = { 0 };

    if ( time & 0xffffffff00000000ll ) {  // overflow -- Cortex M3 has no signed 64bit div 
        return &tm;
    }

    if ( time >= 0 ) {
        int day_seconds = uint32_t( time ) % (3600*24);                  // workaround

        tm.tm_sec = day_seconds % 60;
        tm.tm_min =  ( day_seconds % 3600 ) / 60;
        tm.tm_hour =  day_seconds / 3600;

        int days = uint32_t ( time ) / ( 24 * 3600 );                    // workaround
        int year = 1970;

        while ( days >= ( is_leap_year( year ) ? 366 : 365 ) )
            days -= is_leap_year( year++ ) ? 366 : 365;
        tm.tm_year = year - 1900;
        tm.tm_yday = days;
        for( auto& mdays: __days_in_month[ is_leap_year( year ) ] ) {
            if ( days < mdays )
                break;
            days -= mdays;
            tm.tm_mon++;
        };

        tm.tm_mday = days + 1;  // 1-origin
        
    } else {
        int day_seconds = (3600*24) - (uint32_t( -time ) % (3600*24));   // workaround
        
        tm.tm_sec = day_seconds % 60;
        tm.tm_min =  ( day_seconds % 3600 ) / 60;
        tm.tm_hour =  day_seconds / 3600;

        int days = uint32_t(-time) / ( 24 * 3600 );                      // workaround
        int year = 1969;

        while ( days >= ( is_leap_year( year ) ? 366 : 365 ) )
            days -= is_leap_year( year-- ) ? 366 : 365;

        tm.tm_year = year - 1900;
        tm.tm_yday = ( is_leap_year( year ) ? 366 : 365 ) - days;

        days = tm.tm_yday; // flip to positive (forwad) value
        tm.tm_mon = 0;
        for ( auto& mdays: __days_in_month[ is_leap_year( year ) ] ) {
            if ( days <= mdays )
                break;
            days -= mdays;
            tm.tm_mon++;
        }
        tm.tm_mday = days;
    }

    tm.tm_wday = dayofweek( tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday );

    return &tm;
}

static inline bool
append_char( char *& sp, size_t& size, const char c )
{
    if ( size ) {
        --size;
        *sp++ = c;
    }
    return size;
}

static bool
make_string( char *& sp, size_t& size, int d, int width )
{
    const static char * __chars__ = "0123456789";

    if ( width >= size )
        return false;

    size -= width;
    sp += width;

    auto p = sp;
    
    // ignore sign
    if ( d < 0 )
        d = -d;

    while ( width-- ) {
        *--p = __chars__[ d % 10 ];
        d /= 10;
    };
    return size;
}

const char *
date_time::to_string( char * str, size_t size, const tm& tm, bool gmtoffs )
{
    auto save( str );

    if ( make_string( str, size, tm.tm_year + 1900, 4 ) ) {
        append_char( str, size, '-' );
        if ( make_string( str, size, tm.tm_mon + 1, 2 ) ) {
            append_char( str, size, '-' );
            if ( make_string( str, size, tm.tm_mday, 2 ) ) {
                append_char( str, size, 'T');
                if ( make_string( str, size, tm.tm_hour, 2 ) ) {
                    append_char( str, size, ':');
                    if ( make_string( str, size, tm.tm_min, 2 ) ) {
                        append_char( str, size, ':');
                        if ( make_string( str, size, tm.tm_sec, 2 ) ) {
                            if ( gmtoffs ) {
                                append_char( str, size, tm.tm_gmtoff >= 0 ? '+' : '-' );
                                if ( make_string( str, size, tm.tm_gmtoff, 4 ) ) {
                                    append_char( str, size, '\0' );
                                }
                            } else {
                                append_char( str, size, 'Z' );
                                append_char( str, size, '\0' );
                            }
                        }
                    }
                }
            }
        }
    }
    return save;
}

const char *
date_time::to_string( char * str, size_t size, const std::time_t& time, bool gmtoffs )
{
    std::tm tm;
    return to_string( str, size, *gmtime( time, tm ), gmtoffs );
}
