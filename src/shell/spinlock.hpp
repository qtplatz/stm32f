// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include <atomic>

template< typename T = std::atomic_flag >
class scoped_spinlock {
    T& flag_;
public:

    scoped_spinlock( T& flag ) : flag_( flag ) {
        while (flag_.test_and_set( std::memory_order_acquire ))  // acquire lock
            ; // spin        
    };

    ~scoped_spinlock() {
        flag_.clear( std::memory_order_release );
    };

};

