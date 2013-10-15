/*
   The MIT License (MIT)

   Copyright (c) 2011 - 2013, Philipp Heise and Sebastian Klose

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#ifndef CVT_MAPOPTIMIZER_H
#define CVT_MAPOPTIMIZER_H

#include <cvt/vision/SparseBundleAdjustment.h>
#include <cvt/util/Thread.h>

namespace cvt
{
    class MapOptimizer : public Thread<SlamMap>
    {
        public:
            MapOptimizer();
            ~MapOptimizer();

            void execute( SlamMap* map );
            bool isRunning() const;

        private:            
            TerminationCriteria<double>	_termCrit;
            bool						_isRunning;
    };

    inline MapOptimizer::MapOptimizer() :
        _isRunning( false )
    {
        _termCrit.setCostThreshold( 0.05 );
        _termCrit.setMaxIterations( 40 );
    }

    inline MapOptimizer::~MapOptimizer()
    {
        if( _isRunning )
            join();
    }

    inline void MapOptimizer::execute( SlamMap* map )
    {
        SparseBundleAdjustment sba;
        _isRunning = true;
        sba.optimize( *map, _termCrit );
        _isRunning = false;
    }

    inline bool MapOptimizer::isRunning() const
    {
        return _isRunning;
    }
}

#endif
