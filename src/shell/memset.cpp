/* memset.c
 * Copyright (C) MS-Cheminformatics LLC
 * Author: Toshinobu Hondo, Ph.D., 
 */

#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif
    void * memset( void * dest, int value, size_t num );
    
#ifdef __cplusplus
}
#endif

void *
memset( void * dest, int value, size_t num )
{
    char * p = reinterpret_cast< char * >( dest );
    while ( num-- )
        *p++ = value;
    return dest;
}
