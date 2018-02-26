// Copyright (C) 2018 MS-Cheminformatics LLC
// Licence: CC BY-NC
// Author: Toshinobu Hondo, Ph.D.
// Contact: toshi.hondo@qtplatz.com
//

#include "command_processor.hpp"
#include "stream.hpp"
#include <algorithm>

command_processor::command_processor()
{
}

bool
command_processor::operator()( size_t argc, const char ** argv ) const
{
    stream() << "command_procedssor: argc=" << argc << " argv = {";
    for ( size_t i = 0; i < argc; ++i )
        stream() << argv[i] << ( ( i < argc - 1 ) ? ", " : "" );
    stream() << "}" << std::endl;

    stream() << "\tnothing implemented yet." << std::endl;
}
