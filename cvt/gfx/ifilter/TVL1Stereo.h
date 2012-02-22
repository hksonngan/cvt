/*
			CVT - Computer Vision Tools Library

 	 Copyright (c) 2012, Philipp Heise, Sebastian Klose

 	THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 	PARTICULAR PURPOSE.
 */
#ifndef CVT_TVL1STEREO_H
#define CVT_TVL1STEREO_H

#include <cvt/gfx/IFilter.h>
#include <cvt/cl/CLKernel.h>

namespace cvt {
	class TVL1Stereo : public IFilter {
		public:
			TVL1Stereo( float scalefactor, size_t levels );
			~TVL1Stereo();
			void apply( Image& flow, const Image& src1, const Image& src2 );
			void apply( const ParamSet* set, IFilterType t = IFILTER_CPU ) const {};

		private:
			void fillPyramidCL( const Image& img, size_t index );
			void solveTVL1( Image& flow, const Image& src1, const Image& src2, bool median );

			float		 _scalefactor;
			size_t		 _levels;
			CLKernel	 _pyrup;
			CLKernel	 _pyrdown;
			CLKernel	 _pyrdownbinom;
			CLKernel	 _tvl1;
			CLKernel	 _tvl1_warp;
			CLKernel	 _clear;
			CLKernel	 _median3;
			float		 _lambda;
			Image*		 _pyr[ 2 ];
	};
}

#endif
