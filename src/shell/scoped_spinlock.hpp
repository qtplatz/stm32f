// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <atomic>

template< typename T = std::atomic_flag >
struct scoped_spinlock {
    T& _;
    scoped_spinlock( T& flag ) : _( flag ) {
        while( _.test_and_set( std::memory_order_acquire ) )
            ;
    }
    ~scoped_spinlock() {
        _.clear( std::memory_order_release );
    }
};
