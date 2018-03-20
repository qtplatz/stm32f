//

#include "date_time.hpp"

bool
parse_time( const char * s, int& hour, int& min, int& second, int& tzoffs )
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
parse_date( const char * s
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

