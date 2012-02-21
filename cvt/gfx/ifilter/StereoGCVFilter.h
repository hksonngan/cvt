/*
			CVT - Computer Vision Tools Library

 	 Copyright (c) 2012, Philipp Heise, Sebastian Klose

 	THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 	PARTICULAR PURPOSE.
 */
#ifndef CVT_STEREOGCV_H
#define CVT_STEREOGCV_H

#include <cvt/gfx/IFilter.h>
#include <cvt/gfx/ifilter/IntegralFilter.h>
#include <cvt/gfx/ifilter/BoxFilter.h>
#include <cvt/cl/CLKernel.h>

namespace cvt {
	class StereoGCVFilter : public IFilter {
		public:
					StereoGCVFilter();
					~StereoGCVFilter();

			void	apply( Image& dst, const Image& cam0, const Image& cam1, float dmin, float dmax, float dt = 1.0f ) const;
			void	apply( const ParamSet* attribs, IFilterType iftype ) const {}

		private:
			void	depthmap( Image& dst, const Image& cam0, const Image& cam1, float dmin, float dmax, float dt = 1.0f ) const;

			CLKernel		_cldepthcost;
			CLKernel		_cldepthcostgrad;
			CLKernel		_cldepthcostncc;
			CLKernel		_cldepthmin;
			CLKernel		_clfill;
			CLKernel		_clcdconv;
			CLKernel		_clgrad;
//			CLKernel		_cldepthrefine;
			CLKernel	   _clguidedfilter_calcab_outerrgb;
			CLKernel	   _clguidedfilter_applyab_gc_outer;
			CLKernel	   _clocclusioncheck;
			IntegralFilter _intfilter;
			BoxFilter	   _boxfilter;

	};

	inline StereoGCVFilter::~StereoGCVFilter()
	{
	}
}

#endif
