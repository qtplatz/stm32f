// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//
// Quick dirty (dummy) implemetation of atexit

#ifdef __cplusplus
extern "C" {
#endif
    int __aeabi_atexit( void (*func)(void *), void *arg, void *dso );
    void * __dso_handle __attribute__ ((__weak__)); 
#ifdef __cplusplus    
}
#endif

int
__aeabi_atexit( void (*func)(void *), void *arg, void *dso )
{
    // do nothing
    __dso_handle = 0;
    return 0;
}

