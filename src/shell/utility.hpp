// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <cstdint>
#include <cstddef>

int strcmp( const char *, const char * b );
int strncmp( const char *, const char * b, size_t n );
size_t strlen( const char * s );
int strtod( const char * s );
uint32_t strtox( const char * s, char ** end = nullptr );
char * strncpy( char * dst, const char * src, size_t size );
char * strncat( char * dst, const char * src, size_t size );
