/*
			CVT - Computer Vision Tools Library

 	 Copyright (c) 2012, Philipp Heise, Sebastian Klose

 	THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 	PARTICULAR PURPOSE.
 */

#include <cvt/gfx/IMapScoped.h>
#include <cvt/util/SIMD.h>
#include <cvt/util/ScopedBuffer.h>
#include <cvt/gfx/IMorphological.h>

namespace cvt
{

	template<typename TYPE>
	static void morphTemplate( Image& dst, const Image& src, size_t radius,
								void ( SIMD::*hfunc )( TYPE*, const TYPE*, size_t, size_t ) const,
								void ( SIMD::*hfuncborder )( TYPE*, const TYPE*, const TYPE*, size_t ) const,
								void ( SIMD::*vfunc )( TYPE*, const TYPE**, size_t, size_t ) const
							 )
	{
		SIMD* simd = SIMD::instance();
		const size_t boxsize = radius * 2 + 1;
		size_t curbuf;
		size_t w = src.width();
		size_t h = src.height();
		size_t bstride = Math::pad16( sizeof( TYPE ) * w ) / sizeof( TYPE ); //FIXME: does this always work - it should
		TYPE** buf;
		size_t i;

		IMapScoped<TYPE> mapdst( dst );
		IMapScoped<const TYPE> mapsrc( src );

		/* allocate and fill buffer */
		ScopedBuffer<TYPE,true> bufmem( bstride * boxsize );
		ScopedBuffer<TYPE*,true> bufptr( boxsize );

		buf = bufptr.ptr();
		buf[ 0 ] = bufmem.ptr();
		for( i = 0; i < boxsize; i++ ) {
			if( i != 0 )
				buf[ i ] = buf[ i - 1 ] + bstride;
			( simd->*hfunc )( buf[ i ], mapsrc.ptr(), w, radius );
			mapsrc++;
		}

		/* upper border */
		( simd->*vfunc )( mapdst.ptr(), ( const TYPE** ) buf, radius + 1, w );
		for( i = 1; i < radius; i++ ) {
			TYPE* prev = mapdst.ptr();
			mapdst++;
			( simd->*hfuncborder )( mapdst.ptr(), ( const TYPE* ) prev, ( const TYPE* ) buf[ radius + 1 + i ], w );
		}
		mapdst++;

		/* center */
		curbuf = 0;
		i = h - 2 * radius - 1;
		( simd->*vfunc )( mapdst.ptr(), ( const TYPE** ) buf, boxsize, w );
		mapdst++;
		while( i-- ) {
			( simd->*hfunc )( buf[ curbuf ], mapsrc.ptr(), w, radius );
			curbuf = ( curbuf + 1 ) % boxsize;
			( simd->*vfunc )( mapdst.ptr(), ( const TYPE** ) buf, boxsize, w );
			mapdst++;
			mapsrc++;
		}

		/* lower border */
		/* reorder buffer */
		ScopedBuffer<TYPE*,true> bufptr2( boxsize );
		TYPE** buf2 = bufptr2.ptr();
		for( i = 0; i < boxsize; i++ )
			buf2[ i ] = buf[ ( curbuf + i ) % boxsize ];

		for( i = 1; i <= radius; i++ ) {
			( simd->*vfunc )( mapdst.ptr(), ( const TYPE** ) ( buf2 + i ), boxsize - i, w );
			mapdst++;
		}
	}

	void IMorphological::dilate( Image& dst, const Image& src, size_t radius )
	{
		if( src.channels() != 1 )
			throw CVTException( "Not implemented IMorphological::dilate" );

		dst.reallocate( src.width(), src.height(), src.format() );

		switch( src.format().formatID ) {
			case IFORMAT_GRAY_UINT8:
				morphTemplate<uint8_t>( dst, src, radius, &SIMD::dilateSpanU8, &SIMD::MaxValueU8, &SIMD::MaxValueVertU8 );
				return;
			case IFORMAT_GRAY_UINT16:
				morphTemplate<uint16_t>( dst, src, radius, &SIMD::dilateSpanU16, &SIMD::MaxValueU16, &SIMD::MaxValueVertU16 );
				break;
			case IFORMAT_GRAY_FLOAT:
				morphTemplate<float>( dst, src, radius, &SIMD::dilateSpan1f, &SIMD::MaxValue1f, &SIMD::MaxValueVert1f );
				break;
			default:
				throw CVTException( "Not implemented" );
		}
	}

	void IMorphological::erode( Image& dst, const Image& src, size_t radius )
	{
		if( src.channels() != 1 )
			throw CVTException( "Not implemented IMorphological::erode" );

		dst.reallocate( src.width(), src.height(), src.format() );

		switch( src.format().formatID ) {
			case IFORMAT_GRAY_UINT8:
				morphTemplate<uint8_t>( dst, src, radius, &SIMD::erodeSpanU8, &SIMD::MinValueU8, &SIMD::MinValueVertU8 );
				return;
			case IFORMAT_GRAY_UINT16:
				morphTemplate<uint16_t>( dst, src, radius, &SIMD::erodeSpanU16, &SIMD::MinValueU16, &SIMD::MinValueVertU16 );
				break;
			case IFORMAT_GRAY_FLOAT:
				morphTemplate<float>( dst, src, radius, &SIMD::erodeSpan1f, &SIMD::MinValue1f, &SIMD::MinValueVert1f );
				break;
			default:
				throw CVTException( "Not implemented" );
		}
	}
}
