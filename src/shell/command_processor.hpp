// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#pragma once

#include <array>
#include <cstddef>

class command_processor {
    command_processor( const command_processor& ) = delete;
    command_processor& operator = ( const command_processor& ) = delete;
    
public:
    command_processor();
    // ~command_processor() {} // dtor causing undefined references: __cxa_end_cleanup, __gxx_personality_v0
    
    bool operator()( size_t argc, const char ** argv ) const;
};
