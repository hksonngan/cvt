#include <cvt/util/SIMDSSE2.h>

#include <xmmintrin.h>
#include <emmintrin.h>

namespace cvt
{
	void SIMDSSE2::MulValue1fx( Fixed* dst, const Fixed* src, Fixed value, size_t n ) const
	{
		size_t i = n >> 2;

		__m128 mul = _mm_set1_ps( value.toFloat() );
		__m128i in;
		__m128 inf;

		if( ( ( size_t ) src | ( size_t ) dst ) & 0xf ) {
			while( i-- ) {
				in = _mm_loadu_si128( ( __m128i* ) src );
				inf = _mm_mul_ps( mul, _mm_cvtepi32_ps( in ) );
				_mm_storeu_si128( ( __m128i* ) dst, _mm_cvtps_epi32( inf ) );
				src += 4;
				dst += 4;
			}
		} else {
			while( i-- ) {
				in = _mm_load_si128( ( __m128i* ) src );
				inf = _mm_mul_ps( mul, _mm_cvtepi32_ps( in ) );
				_mm_store_si128( ( __m128i* ) dst, _mm_cvtps_epi32( inf ) );
				src += 4;
				dst += 4;
			}
		}

		i = n & 0x03;
		while( i-- )
			*dst++ += *src++ * value;
	}

	void SIMDSSE2::MulAddValue1fx( Fixed* dst, const Fixed* src, Fixed value, size_t n ) const
	{
		size_t i = n >> 3;

		if( value.native() == 0 )
			return;

		const __m128 mul = _mm_set1_ps( value.toFloat() );
		__m128i in, out, in2, out2;
		__m128 inf, inf2;

		if( ( ( size_t ) src | ( size_t ) dst ) & 0xf ) {
			while( i-- ) {
				in = _mm_loadu_si128( ( __m128i* ) src );
				out = _mm_loadu_si128( ( __m128i* ) dst );
				inf = _mm_mul_ps( _mm_cvtepi32_ps( in ), mul );
				out = _mm_add_epi32( out, _mm_cvtps_epi32( inf )  );
				_mm_storeu_si128( ( __m128i* ) dst, out );

				src += 4;
				dst += 4;

				in2 = _mm_loadu_si128( ( __m128i* ) ( src ) );
				out2 = _mm_loadu_si128( ( __m128i* ) ( dst ) );
				inf2 = _mm_mul_ps( _mm_cvtepi32_ps( in2 ), mul );
				out2 = _mm_add_epi32( out2, _mm_cvtps_epi32( inf2 )  );
				_mm_storeu_si128( ( __m128i* ) ( dst ), out2 );

				src += 4;
				dst += 4;
			}
		} else {
			while( i-- ) {
				in = _mm_load_si128( ( __m128i* ) ( src ) );
				in2 = _mm_load_si128( ( __m128i* ) ( src + 4 ) );

				out = _mm_load_si128( ( __m128i* ) ( dst ) );
				inf = _mm_cvtepi32_ps( in );
				inf = _mm_mul_ps( inf, mul );
				out = _mm_add_epi32( out, _mm_cvtps_epi32( inf )  );
				_mm_store_si128( ( __m128i* ) ( dst ), out );

				out2 = _mm_load_si128( ( __m128i* ) ( dst + 4 ) );
				inf2 = _mm_cvtepi32_ps( in2 );
				inf2 = _mm_mul_ps( inf2 , mul );
				out2 = _mm_add_epi32( out2, _mm_cvtps_epi32( inf2 )  );
				_mm_store_si128( ( __m128i* ) ( dst + 4 ), out2 );

				src += 8;
				dst += 8;
			}
		}

		i = n & 0x07;
		while( i-- )
			*dst++ += *src++ * value;
	}

	size_t SIMDSSE2::SAD( uint8_t const* src1, uint8_t const* src2, const size_t n ) const
	{
		size_t i = n >> 4;	
		size_t sad = 0;
				
		__m128i simdA, simdB, simdC;
		if( ( ( size_t ) src1 | ( size_t ) src2 ) & 0xf ) {
			while( i-- ) {
				simdA = _mm_loadu_si128( ( __m128i* )src1 );
				simdB = _mm_loadu_si128( ( __m128i* )src2 );			
				simdC = _mm_sad_epu8( simdA, simdB );
				sad += _mm_extract_epi16( simdC, 0 );
				sad += _mm_extract_epi16( simdC, 4 );				
				src1 += 16; src2 += 16;
			}
		} else {
			while( i-- ) {
				simdA = _mm_load_si128( ( __m128i* )src1 );
				simdB = _mm_load_si128( ( __m128i* )src2 );			
				simdC = _mm_sad_epu8( simdA, simdB );
				sad += _mm_extract_epi16( simdC, 0 );
				sad += _mm_extract_epi16( simdC, 4 );				
				src1 += 16; src2 += 16;
			}
		}
		
		i = n & 0x03;
		while( i-- ) {
			sad += Math::abs( ( int16_t )*src1++ - ( int16_t )*src2++ );
		}
		
		return sad;
	}

	void SIMDSSE2::ConvolveClampSet1fx( Fixed* dst, uint8_t const* src, const size_t width, const Fixed* weights, const size_t wn ) const
	{
		const Fixed* wp;
		const uint8_t* sp;
		Fixed tmp;
		size_t i, k, b1, b2;

		if( wn == 1 ) {
			MulAddU8Value1fx( dst, src, *weights, width );
			return;
		}

		b1 = ( wn - ( 1 - ( wn & 1 ) ) ) / 2;
		b2 = ( wn + ( 1 - ( wn & 1 ) ) ) / 2;

		/* border 1 */
		i = b1;
		while( i-- ) {
			wp = weights;
			sp = src;
			tmp = *sp * *wp++;
			k = i;
			while( k-- )
				tmp += *sp * *wp++;
			k = wn - 1 - i;
			while( k-- ) {
				tmp += *sp++ * *wp++;
			}
			*dst++ = tmp;
		}


		/* center */
		i = ( width - wn + 1 ) >> 4;
		while( i-- ) {
			__m128i f, z = _mm_setzero_si128(), s0 = z, s1 = z, s2 = z, s3 = z;
			__m128i x0, x1, x2, x3;
			k = wn;
			sp = src;
			wp = weights;

			while( k-- )
			{
				f = _mm_cvtsi32_si128( (*wp).native() );
				wp++;
				f = _mm_shuffle_epi32( f, 0 );
				f = _mm_packs_epi32( f, f );

				x0 = _mm_loadu_si128( (const __m128i*) sp );
				x2 = _mm_unpackhi_epi8( x0, z );
				x0 = _mm_unpacklo_epi8( x0, z );
				x1 = _mm_mulhi_epi16( x0, f );
				x3 = _mm_mulhi_epi16( x2, f );
				x0 = _mm_mullo_epi16( x0, f );
				x2 = _mm_mullo_epi16( x2, f );

				s0 = _mm_add_epi32( s0, _mm_unpacklo_epi16( x0, x1 ) );
				s1 = _mm_add_epi32( s1, _mm_unpackhi_epi16( x0, x1 ) );
				s2 = _mm_add_epi32( s2, _mm_unpacklo_epi16( x2, x3 ) );
				s3 = _mm_add_epi32( s3, _mm_unpackhi_epi16( x2, x3 ) );
				sp++;
			}
			_mm_storeu_si128((__m128i*)( dst ), s0);
			_mm_storeu_si128((__m128i*)( dst + 4 ), s1);
			_mm_storeu_si128((__m128i*)( dst + 8 ), s2);
			_mm_storeu_si128((__m128i*)( dst + 12 ), s3);
			dst += 16;
			src += 16;
		}

		i = ( width - wn + 1 ) & 0xf;
		while( i-- ) {
			k = wn;
			sp = src;
			wp = weights;
			tmp = *sp++ * *wp++;
			k--;
			while( k-- )
				tmp += *sp++ * *wp++;
			*dst++ = tmp;
			src++;
		}

		/* border 2 */
		i = b2;
		while( i-- ) {
			wp = weights;
			sp = src;
			tmp = *sp++ * *wp++;
			k = b1 + i;
			while( k-- ) {
				tmp += *sp++ * *wp++;
			}
			k = b2 - i;
			sp--;
			while( k-- )
				tmp += *sp * *wp++;
			*dst++ = tmp;
			src++;
		}
	}

	void SIMDSSE2::ConvolveClampAdd1fx( Fixed* dst, uint8_t const* src, const size_t width, const Fixed* weights, const size_t wn ) const
	{
		const Fixed* wp;
		const uint8_t* sp;
		Fixed tmp;
		size_t i, k, b1, b2;

		if( wn == 1 ) {
			MulAddU8Value1fx( dst, src, *weights, width );
			return;
		}

		b1 = ( wn - ( 1 - ( wn & 1 ) ) ) / 2;
		b2 = ( wn + ( 1 - ( wn & 1 ) ) ) / 2;

		/* border 1 */
		i = b1;
		while( i-- ) {
			wp = weights;
			sp = src;
			tmp = *sp * *wp++;
			k = i;
			while( k-- )
				tmp += *sp * *wp++;
			k = wn - 1 - i;
			while( k-- ) {
				tmp += *sp++ * *wp++;
			}
			*dst++ += tmp;
		}


		/* center */
		i = ( width - wn + 1 ) >> 4;
		while( i-- ) {
			__m128i f, z = _mm_setzero_si128(), s0 = z, s1 = z, s2 = z, s3 = z;
			__m128i x0, x1, x2, x3;
			k = wn;
			sp = src;
			wp = weights;

			while( k-- )
			{
				f = _mm_cvtsi32_si128( (*wp).native() );
				wp++;
				f = _mm_shuffle_epi32( f, 0 );
				f = _mm_packs_epi32( f, f );

				x0 = _mm_loadu_si128( (const __m128i*) sp );
				x2 = _mm_unpackhi_epi8( x0, z );
				x0 = _mm_unpacklo_epi8( x0, z );
				x1 = _mm_mulhi_epi16( x0, f );
				x3 = _mm_mulhi_epi16( x2, f );
				x0 = _mm_mullo_epi16( x0, f );
				x2 = _mm_mullo_epi16( x2, f );

				s0 = _mm_add_epi32( s0, _mm_unpacklo_epi16( x0, x1 ) );
				s1 = _mm_add_epi32( s1, _mm_unpackhi_epi16( x0, x1 ) );
				s2 = _mm_add_epi32( s2, _mm_unpacklo_epi16( x2, x3 ) );
				s3 = _mm_add_epi32( s3, _mm_unpackhi_epi16( x2, x3 ) );
				sp++;
			}

			x0 = _mm_loadu_si128( (__m128i*) dst );
			s0 = _mm_add_epi32( s0, x0 );
			_mm_storeu_si128((__m128i*)( dst ), s0);

			x1 = _mm_loadu_si128( (__m128i*) ( dst + 4 ) );
			s1 = _mm_add_epi32( s1, x1 );
			_mm_storeu_si128((__m128i*)( dst + 4 ), s1);

			x2 = _mm_loadu_si128( (__m128i*) ( dst + 8 ) );
			s2 = _mm_add_epi32( s2, x2 );
			_mm_storeu_si128((__m128i*)( dst + 8 ), s2);

			x3 = _mm_loadu_si128( (__m128i*) ( dst + 12 ) );
			s3 = _mm_add_epi32( s3, x3 );
			_mm_storeu_si128((__m128i*)( dst + 12 ), s3);

			dst += 16;
			src += 16;
		}

		i = ( width - wn + 1 ) & 0xf;
		while( i-- ) {
			k = wn;
			sp = src;
			wp = weights;
			tmp = *sp++ * *wp++;
			k--;
			while( k-- )
				tmp += *sp++ * *wp++;
			*dst++ += tmp;
			src++;
		}

		/* border 2 */
		i = b2;
		while( i-- ) {
			wp = weights;
			sp = src;
			tmp = *sp++ * *wp++;
			k = b1 + i;
			while( k-- ) {
				tmp += *sp++ * *wp++;
			}
			k = b2 - i;
			sp--;
			while( k-- )
				tmp += *sp * *wp++;
			*dst++ += tmp;
			src++;
		}
	}

	void SIMDSSE2::ConvolveClampSet4fx( Fixed* dst, uint8_t const* src, const size_t width, const Fixed* weights, const size_t wn ) const
	{
		const Fixed* wp;
		const uint8_t* sp;
		Fixed tmp[ 4 ], w;
		size_t i, k, b1, b2;

		if( wn == 1 ) {
			MulU8Value1fx( dst, src, *weights, width * 4 );
			return;
		}

		b1 = ( wn - ( 1 - ( wn & 1 ) ) ) / 2;
		b2 = ( wn + ( 1 - ( wn & 1 ) ) ) / 2;

		/* border 1 */
		i = b1;
		while( i-- ) {
			wp = weights;
			sp = src;
			tmp[ 0 ] = *( sp + 0 ) * *wp;
			tmp[ 1 ] = *( sp + 1 ) * *wp;
			tmp[ 2 ] = *( sp + 2 ) * *wp;
			tmp[ 3 ] = *( sp + 3 ) * *wp++;
			k = i;
			while( k-- ) {
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
			}
			k = wn - 1 - i;
			while( k-- ) {
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
				sp += 4;
			}
			*dst++ = tmp[ 0 ];
			*dst++ = tmp[ 1 ];
			*dst++ = tmp[ 2 ];
			*dst++ = tmp[ 3 ];
		}


		/* center */
		i = ( width - wn + 1 ) >> 2;
		while( i-- ) {
			__m128i f, z, s0, s1, s2, s3;
			__m128i x0, x1, x2, x3;
			k = wn;
			sp = src;
			wp = weights;

			z = s0 = s1 = s2 = s3 = _mm_setzero_si128();

			while( k-- )
			{
				f = _mm_cvtsi32_si128( (*wp).native() );
				wp++;
				f = _mm_shuffle_epi32( f, 0 );
				f = _mm_packs_epi32( f, f );

				x0 = _mm_loadu_si128( (const __m128i*) sp );
				x2 = _mm_unpackhi_epi8( x0, z );
				x0 = _mm_unpacklo_epi8( x0, z );
				x1 = _mm_mulhi_epi16( x0, f );
				x3 = _mm_mulhi_epi16( x2, f );
				x0 = _mm_mullo_epi16( x0, f );
				x2 = _mm_mullo_epi16( x2, f );

				s0 = _mm_add_epi32( s0, _mm_unpacklo_epi16( x0, x1 ) );
				s1 = _mm_add_epi32( s1, _mm_unpackhi_epi16( x0, x1 ) );
				s2 = _mm_add_epi32( s2, _mm_unpacklo_epi16( x2, x3 ) );
				s3 = _mm_add_epi32( s3, _mm_unpackhi_epi16( x2, x3 ) );
				sp += 4;
			}

			_mm_store_si128((__m128i*)( dst ), s0);
			_mm_store_si128((__m128i*)( dst + 4 ), s1);
			_mm_store_si128((__m128i*)( dst + 8 ), s2);
			_mm_store_si128((__m128i*)( dst + 12 ), s3);

			dst += 16;
			src += 16;
		}


		i = ( width - wn + 1 ) & 0x3;
		while( i-- ) {
			k = wn;
			sp = src;
			wp = weights;
			tmp[ 0 ] = *( sp + 0 ) * *wp;
			tmp[ 1 ] = *( sp + 1 ) * *wp;
			tmp[ 2 ] = *( sp + 2 ) * *wp;
			tmp[ 3 ] = *( sp + 3 ) * *wp++;
			sp += 4;
			k--;
			while( k-- ) {
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
				sp += 4;
			}
			*dst++ = tmp[ 0 ];
			*dst++ = tmp[ 1 ];
			*dst++ = tmp[ 2 ];
			*dst++ = tmp[ 3 ];
			src += 4;
		}

		/* border 2 */
		i = b2;
		while( i-- ) {
			wp = weights;
			sp = src;
			tmp[ 0 ] = *( sp + 0 ) * *wp;
			tmp[ 1 ] = *( sp + 1 ) * *wp;
			tmp[ 2 ] = *( sp + 2 ) * *wp;
			tmp[ 3 ] = *( sp + 3 ) * *wp++;
			k = b1 + i;
			while( k-- ) {
				sp += 4;
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
			}
			k = b2 - i;
			while( k-- ) {
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
			}
			*dst++ = tmp[ 0 ];
			*dst++ = tmp[ 1 ];
			*dst++ = tmp[ 2 ];
			*dst++ = tmp[ 3 ];
			src += 4;
		}
	}

	void SIMDSSE2::ConvolveClampAdd4fx( Fixed* dst, uint8_t const* src, const size_t width, const Fixed* weights, const size_t wn ) const
	{
		const Fixed* wp;
		const uint8_t* sp;
		Fixed tmp[ 4 ], w;
		size_t i, k, b1, b2;

		if( wn == 1 ) {
			MulAddU8Value1fx( dst, src, *weights, width * 4 );
			return;
		}

		b1 = ( wn - ( 1 - ( wn & 1 ) ) ) / 2;
		b2 = ( wn + ( 1 - ( wn & 1 ) ) ) / 2;

		/* border 1 */
		i = b1;
		while( i-- ) {
			wp = weights;
			sp = src;
			tmp[ 0 ] = *( sp + 0 ) * *wp;
			tmp[ 1 ] = *( sp + 1 ) * *wp;
			tmp[ 2 ] = *( sp + 2 ) * *wp;
			tmp[ 3 ] = *( sp + 3 ) * *wp++;
			k = i;
			while( k-- ) {
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
			}
			k = wn - 1 - i;
			while( k-- ) {
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
				sp += 4;
			}
			*dst++ += tmp[ 0 ];
			*dst++ += tmp[ 1 ];
			*dst++ += tmp[ 2 ];
			*dst++ += tmp[ 3 ];
		}


		/* center */
		i = ( width - wn + 1 ) >> 2;
		while( i-- ) {
			__m128i f, z = _mm_setzero_si128(), s0 = z, s1 = z, s2 = z, s3 = z;
			__m128i x0, x1, x2, x3;
			k = wn;
			sp = src;
			wp = weights;

			while( k-- )
			{
				f = _mm_cvtsi32_si128( (*wp).native() );
				wp++;
				f = _mm_shuffle_epi32(f, 0);
				f = _mm_packs_epi32(f, f);

				x0 = _mm_loadu_si128( (const __m128i*) sp );
				x2 = _mm_unpackhi_epi8(x0, z);
				x0 = _mm_unpacklo_epi8(x0, z);
				x1 = _mm_mulhi_epi16(x0, f);
				x3 = _mm_mulhi_epi16(x2, f);
				x0 = _mm_mullo_epi16(x0, f);
				x2 = _mm_mullo_epi16(x2, f);

				s0 = _mm_add_epi32(s0, _mm_unpacklo_epi16(x0, x1));
				s1 = _mm_add_epi32(s1, _mm_unpackhi_epi16(x0, x1));
				s2 = _mm_add_epi32(s2, _mm_unpacklo_epi16(x2, x3));
				s3 = _mm_add_epi32(s3, _mm_unpackhi_epi16(x2, x3));
				sp += 4;
			}

			x0 = _mm_load_si128( (__m128i*) dst );
			s0 = _mm_add_epi32( s0, x0 );
			_mm_store_si128((__m128i*)( dst ), s0);

			x1 = _mm_load_si128( (__m128i*) ( dst + 4 ) );
			s1 = _mm_add_epi32( s1, x1 );
			_mm_store_si128((__m128i*)( dst + 4 ), s1);

			x2 = _mm_load_si128( (__m128i*) ( dst + 8 ) );
			s2 = _mm_add_epi32( s2, x2 );
			_mm_store_si128((__m128i*)( dst + 8 ), s2);

			x3 = _mm_load_si128( (__m128i*) ( dst + 12 ) );
			s3 = _mm_add_epi32( s3, x3 );
			_mm_store_si128((__m128i*)( dst + 12 ), s3);

			dst += 16;
			src += 16;
		}

		i = ( width - wn + 1 ) & 0x3;
		while( i-- ) {
			k = wn;
			sp = src;
			wp = weights;
			w = *wp++;
			tmp[ 0 ] = *( sp + 0 ) * w;
			tmp[ 1 ] = *( sp + 1 ) * w;
			tmp[ 2 ] = *( sp + 2 ) * w;
			tmp[ 3 ] = *( sp + 3 ) * w;
			sp += 4;
			k--;
			while( k-- ) {
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
				sp += 4;
			}
			*dst++ += tmp[ 0 ];
			*dst++ += tmp[ 1 ];
			*dst++ += tmp[ 2 ];
			*dst++ += tmp[ 3 ];
			src += 4;
		}

		/* border 2 */
		i = b2;
		while( i-- ) {
			wp = weights;
			sp = src;
			tmp[ 0 ] = *( sp + 0 ) * *wp;
			tmp[ 1 ] = *( sp + 1 ) * *wp;
			tmp[ 2 ] = *( sp + 2 ) * *wp;
			tmp[ 3 ] = *( sp + 3 ) * *wp++;
			k = b1 + i;
			while( k-- ) {
				sp += 4;
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
			}
			k = b2 - i;
			while( k-- ) {
				tmp[ 0 ] += *( sp + 0 ) * *wp;
				tmp[ 1 ] += *( sp + 1 ) * *wp;
				tmp[ 2 ] += *( sp + 2 ) * *wp;
				tmp[ 3 ] += *( sp + 3 ) * *wp++;
			}
			*dst++ += tmp[ 0 ];
			*dst++ += tmp[ 1 ];
			*dst++ += tmp[ 2 ];
			*dst++ += tmp[ 3 ];
			src += 4;
		}
	}

	void SIMDSSE2::ConvolveClampVert_fx_to_u8( uint8_t* dst, const Fixed** bufs, const Fixed* weights, size_t numw, size_t width ) const
	{
		float w[ numw ];
		size_t x;
		__m128 s0, s1, s2, s3, mul;
		__m128i x0, x1, x2, x3, rnd;

		for( x = 0; x < numw; x++ )
			w[ x ] = weights[ x ].toFloat();

		rnd = _mm_set1_epi32( 0x8000 );

		for( x = 0; x <= width - 16; x += 16 ) {
			mul = _mm_load_ss( w );
			mul = _mm_shuffle_ps( mul, mul, 0 );

			x0 = _mm_load_si128( ( __m128i* ) ( bufs[ 0 ] + x ) );
			x1 = _mm_load_si128( ( __m128i* ) ( bufs[ 0 ] + x + 4 ) );
			x2 = _mm_load_si128( ( __m128i* ) ( bufs[ 0 ] + x + 8 ) );
			x3 = _mm_load_si128( ( __m128i* ) ( bufs[ 0 ] + x + 12 ) );
			s0 = _mm_mul_ps( _mm_cvtepi32_ps( x0 ), mul );
			s1 = _mm_mul_ps( _mm_cvtepi32_ps( x1 ), mul );
			s2 = _mm_mul_ps( _mm_cvtepi32_ps( x2 ), mul );
			s3 = _mm_mul_ps( _mm_cvtepi32_ps( x3 ), mul );

			for( size_t k = 1; k < numw; k++ ) {
				mul = _mm_load_ss( ( w + k ) );
				mul = _mm_shuffle_ps( mul, mul, 0 );

				x0 = _mm_load_si128( ( __m128i* ) ( bufs[ k ] + x ) );
				x1 = _mm_load_si128( ( __m128i* ) ( bufs[ k ] + x + 4 ) );
				x2 = _mm_load_si128( ( __m128i* ) ( bufs[ k ] + x + 8 ) );
				x3 = _mm_load_si128( ( __m128i* ) ( bufs[ k ] + x + 12 ) );
				s0 = _mm_add_ps( s0, _mm_mul_ps( _mm_cvtepi32_ps( x0 ), mul ) );
				s1 = _mm_add_ps( s1, _mm_mul_ps( _mm_cvtepi32_ps( x1 ), mul ) );
				s2 = _mm_add_ps( s2, _mm_mul_ps( _mm_cvtepi32_ps( x2 ), mul ) );
				s3 = _mm_add_ps( s3, _mm_mul_ps( _mm_cvtepi32_ps( x3 ), mul ) );
			}
			x0 = _mm_srai_epi32( _mm_add_epi32( _mm_cvtps_epi32( s0 ), rnd ), 16 );
			x1 = _mm_srai_epi32( _mm_add_epi32( _mm_cvtps_epi32( s1 ), rnd ), 16 );
			x2 = _mm_srai_epi32( _mm_add_epi32( _mm_cvtps_epi32( s2 ), rnd ), 16 );
			x3 = _mm_srai_epi32( _mm_add_epi32( _mm_cvtps_epi32( s3 ), rnd ), 16 );

			x0 = _mm_packs_epi32( x0, x1 );
			x1 = _mm_packs_epi32( x2, x3 );

			x0 = _mm_packus_epi16( x0, x1 );
			_mm_store_si128( ( __m128i* ) dst, x0 );
			dst += 16;
		}

		Fixed tmp[ 4 ];
		for( ; x <= width - 4; x += 4 ) {
			tmp[ 0 ] = bufs[ 0 ][ x + 0 ] * *weights;
			tmp[ 1 ] = bufs[ 0 ][ x + 1 ] * *weights;
			tmp[ 2 ] = bufs[ 0 ][ x + 2 ] * *weights;
			tmp[ 3 ] = bufs[ 0 ][ x + 3 ] * *weights;

			for( size_t k = 1; k < numw; k++ ) {
				tmp[ 0 ] += bufs[ k ][ x + 0 ] * weights[ k ];
				tmp[ 1 ] += bufs[ k ][ x + 1 ] * weights[ k ];
				tmp[ 2 ] += bufs[ k ][ x + 2 ] * weights[ k ];
				tmp[ 3 ] += bufs[ k ][ x + 3 ] * weights[ k ];
			}
			*dst++ = ( uint8_t ) Math::clamp( tmp[ 0 ].round(), 0x0, 0xff );
			*dst++ = ( uint8_t ) Math::clamp( tmp[ 1 ].round(), 0x0, 0xff );
			*dst++ = ( uint8_t ) Math::clamp( tmp[ 2 ].round(), 0x0, 0xff );
			*dst++ = ( uint8_t ) Math::clamp( tmp[ 3 ].round(), 0x0, 0xff );
		}

		for( ; x < width; x++ ) {
			tmp[ 0 ] = bufs[ 0 ][ x + 0 ] * *weights;

			for( size_t k = 1; k < numw; k++ ) {
				tmp[ 0 ] += bufs[ k ][ x + 0 ] * weights[ k ];
			}
			*dst++ = ( uint8_t ) Math::clamp( tmp[ 0 ].round(), 0x0, 0xff );
		}

	}

	void SIMDSSE2::ConvolveClampVert_f( float* dst, const float** bufs, const float* weights, size_t numw, size_t width ) const
	{
		size_t x;
		__m128 s0, s1, s2, s3, mul;
		__m128 x0, x1, x2, x3;

		for( x = 0; x <= width - 16; x += 16 ) {
			mul = _mm_load_ss( weights );
			mul = _mm_shuffle_ps( mul, mul, 0 );

			x0 = _mm_load_ps( bufs[ 0 ] + x );
			x1 = _mm_load_ps( bufs[ 0 ] + x + 4 );
			x2 = _mm_load_ps( bufs[ 0 ] + x + 8 );
			x3 = _mm_load_ps( bufs[ 0 ] + x + 12 );
			s0 = _mm_mul_ps( x0, mul );
			s1 = _mm_mul_ps( x1, mul );
			s2 = _mm_mul_ps( x2, mul );
			s3 = _mm_mul_ps( x3, mul );

			for( size_t k = 1; k < numw; k++ ) {
				mul = _mm_load_ss( ( weights + k ) );
				mul = _mm_shuffle_ps( mul, mul, 0 );

				x0 = _mm_load_ps( bufs[ k ] + x  );
				x1 = _mm_load_ps( bufs[ k ] + x + 4 );
				x2 = _mm_load_ps( bufs[ k ] + x + 8 );
				x3 = _mm_load_ps( bufs[ k ] + x + 12 );

				s0 = _mm_add_ps( s0, _mm_mul_ps( x0, mul ) );
				s1 = _mm_add_ps( s1, _mm_mul_ps( x1, mul ) );
				s2 = _mm_add_ps( s2, _mm_mul_ps( x2, mul ) );
				s3 = _mm_add_ps( s3, _mm_mul_ps( x3, mul ) );
			}
			_mm_store_ps( dst + 0 , s0 );
			_mm_store_ps( dst + 4 , s1 );
			_mm_store_ps( dst + 8 , s2 );
			_mm_store_ps( dst + 12 , s3 );
			dst += 16;
		}

		float tmp;
		for( ; x < width; x++ ) {
			tmp = bufs[ 0 ][ x + 0 ] * *weights;

			for( size_t k = 1; k < numw; k++ ) {
				tmp += bufs[ k ][ x + 0 ] * weights[ k ];
			}
			*dst++ = tmp;
		}
	}

	void SIMDSSE2::Conv_fx_to_u8( uint8_t* dst, const Fixed* src, const size_t n ) const
	{
		size_t i = n >> 4;
		__m128i x0, x1, x2, x3, rnd;
		rnd = _mm_set1_epi32( 0x8000 );

		if( ( ( size_t ) src | ( size_t ) dst ) & 0xf ) {
			while( i-- ) {
				x0 = _mm_loadu_si128( ( __m128i* ) src );
				x1 = _mm_loadu_si128( ( __m128i* ) ( src + 4 ) );
				x2 = _mm_loadu_si128( ( __m128i* ) ( src + 8 ) );
				x3 = _mm_loadu_si128( ( __m128i* ) ( src + 12 ) );

				x0 = _mm_srai_epi32( _mm_add_epi32( x0, rnd ), 16 );
				x1 = _mm_srai_epi32( _mm_add_epi32( x1, rnd ), 16 );
				x2 = _mm_srai_epi32( _mm_add_epi32( x2, rnd ), 16 );
				x3 = _mm_srai_epi32( _mm_add_epi32( x3, rnd ), 16 );

				x0 = _mm_packs_epi32( x0, x1 );
				x1 = _mm_packs_epi32( x2, x3 );

				x0 = _mm_packus_epi16( x0, x1 );
				_mm_storeu_si128( ( __m128i* ) dst, x0 );
				src += 16;
				dst += 16;
			}
		} else {
			while( i-- ) {
				x0 = _mm_load_si128( ( __m128i* ) src );
				x1 = _mm_load_si128( ( __m128i* ) ( src + 4 ) );
				x2 = _mm_load_si128( ( __m128i* ) ( src + 8 ) );
				x3 = _mm_load_si128( ( __m128i* ) ( src + 12 ) );

				x0 = _mm_srai_epi32( _mm_add_epi32( x0, rnd ), 16 );
				x1 = _mm_srai_epi32( _mm_add_epi32( x1, rnd ), 16 );
				x2 = _mm_srai_epi32( _mm_add_epi32( x2, rnd ), 16 );
				x3 = _mm_srai_epi32( _mm_add_epi32( x3, rnd ), 16 );

				x0 = _mm_packs_epi32( x0, x1 );
				x1 = _mm_packs_epi32( x2, x3 );

				x0 = _mm_packus_epi16( x0, x1 );
				_mm_store_si128( ( __m128i* ) dst, x0 );
				src += 16;
				dst += 16;
			}

		}

		i = n & 0x0f;
		while( i-- ) {
			*dst++ = ( uint8_t ) Math::clamp( src->round(), 0x0, 0xff );
			src++;
		}
	}

	void SIMDSSE2::Conv_YUYVu8_to_RGBAu8( uint8_t* dst, const uint8_t* src, const size_t n ) const
	{
		const __m128i Y2RGB = _mm_set_epi16( 1192, 0, 1192, 0, 1192, 0, 1192, 0 );
		const __m128i UV2R  = _mm_set_epi16( 1634, 0, 1634, 0, 1634, 0, 1634, 0 );
		const __m128i UV2G  = _mm_set_epi16( -832, -401, -832, -401, -832, -401, -832, -401 );
		const __m128i UV2B  = _mm_set_epi16( 0, 2066, 0, 2066, 0, 2066, 0, 2066 );
		const __m128i UVOFFSET = _mm_set1_epi16( 128 );
		const __m128i YOFFSET = _mm_set1_epi16( 16 );
		const __m128i A32  = _mm_set1_epi32( 0xff );
		const __m128i mask = _mm_set1_epi16( 0xff00 );

		__m128i uyvy;
		__m128i uv, yz, y, z;
		__m128i uvR, uvG, uvB;
		__m128i r, g, b, a;
		__m128i RB0, RB1, GA0, GA1;

		int i = n >> 3;
		if( ( ( size_t ) src | ( size_t ) dst ) & 0xf ) {
			while( i-- ) {
				uyvy = _mm_loadu_si128( ( __m128i* ) src );
				src += 16;
				/* U0 Y0 V0 Z0 U1 Y1 V1 Z1 U2 Y2 V2 Z2 U3 Y3 V3 Z3 */

				uv = _mm_and_si128( mask, uyvy );
				uv = _mm_srli_si128( uv, 1 );
				uv = _mm_sub_epi16( uv, UVOFFSET ); /* U0 V0 U1 V1 ... */

				yz = _mm_andnot_si128( mask, uyvy );
				yz = _mm_sub_epi16( yz, YOFFSET );  /* Y0 Z0 Y1 Z1 ...  */

				z = _mm_madd_epi16( yz, Y2RGB );                      /* Z0 Z1 Z2 Z3 */
				y = _mm_madd_epi16( yz, _mm_srli_si128( Y2RGB, 2 ) ); /* Y0 Y1 Y2 Y3 */

				uvR = _mm_madd_epi16( uv, UV2R );
				uvG = _mm_madd_epi16( uv, UV2G );
				uvB = _mm_madd_epi16( uv, UV2B );

				r  = _mm_srai_epi32( _mm_add_epi32( y, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( y, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( y, uvB ), 10 );

				RB0 = _mm_packs_epi32( r, b );
				GA0 = _mm_packs_epi32( g, A32 );

				r  = _mm_srai_epi32( _mm_add_epi32( z, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( z, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( z, uvB ), 10 );

				RB1 = _mm_packs_epi32( r, b );
				GA1 = _mm_packs_epi32( g, A32 );

				r  = _mm_unpacklo_epi16( RB0, RB1 );
				b  = _mm_unpackhi_epi16( RB0, RB1 );
				g  = _mm_unpacklo_epi16( GA0, GA1 );
				a  = _mm_unpackhi_epi16( GA0, GA1 );

				RB0 = _mm_unpacklo_epi16( r, b );
				RB1 = _mm_unpackhi_epi16( r, b );
				RB0 = _mm_packus_epi16( RB0, RB1 );

				GA0 = _mm_unpacklo_epi16( g, a );
				GA1 = _mm_unpackhi_epi16( g, a );
				GA0 = _mm_packus_epi16( GA0, GA1 );

				_mm_storeu_si128( ( __m128i* ) dst,  _mm_unpacklo_epi8( RB0, GA0 ) );
				dst += 16;
				_mm_storeu_si128( ( __m128i* ) dst,  _mm_unpackhi_epi8( RB0, GA0 ) );
				dst += 16;
			}
		} else {
			while( i-- ) {
				uyvy = _mm_load_si128( ( __m128i* ) src );
				src += 16;
				/* U0 Y0 V0 Z0 U1 Y1 V1 Z1 U2 Y2 V2 Z2 U3 Y3 V3 Z3 */

				uv = _mm_and_si128( mask, uyvy );
				uv = _mm_srli_si128( uv, 1 );
				uv = _mm_sub_epi16( uv, UVOFFSET ); /* U0 V0 U1 V1 ... */

				yz = _mm_andnot_si128( mask, uyvy );
				yz = _mm_sub_epi16( yz, YOFFSET );  /* Y0 Z0 Y1 Z1 ...  */

				z = _mm_madd_epi16( yz, Y2RGB );                      /* Z0 Z1 Z2 Z3 */
				y = _mm_madd_epi16( yz, _mm_srli_si128( Y2RGB, 2 ) ); /* Y0 Y1 Y2 Y3 */

				uvR = _mm_madd_epi16( uv, UV2R );
				uvG = _mm_madd_epi16( uv, UV2G );
				uvB = _mm_madd_epi16( uv, UV2B );

				r  = _mm_srai_epi32( _mm_add_epi32( y, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( y, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( y, uvB ), 10 );

				RB0 = _mm_packs_epi32( r, b );
				GA0 = _mm_packs_epi32( g, A32 );

				r  = _mm_srai_epi32( _mm_add_epi32( z, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( z, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( z, uvB ), 10 );

				RB1 = _mm_packs_epi32( r, b );
				GA1 = _mm_packs_epi32( g, A32 );

				r  = _mm_unpacklo_epi16( RB0, RB1 );
				b  = _mm_unpackhi_epi16( RB0, RB1 );
				g  = _mm_unpacklo_epi16( GA0, GA1 );
				a  = _mm_unpackhi_epi16( GA0, GA1 );

				RB0 = _mm_unpacklo_epi16( r, b );
				RB1 = _mm_unpackhi_epi16( r, b );
				RB0 = _mm_packus_epi16( RB0, RB1 );

				GA0 = _mm_unpacklo_epi16( g, a );
				GA1 = _mm_unpackhi_epi16( g, a );
				GA0 = _mm_packus_epi16( GA0, GA1 );

				_mm_stream_si128( ( __m128i* ) dst,  _mm_unpacklo_epi8( RB0, GA0 ) );
				dst += 16;
				_mm_stream_si128( ( __m128i* ) dst,  _mm_unpackhi_epi8( RB0, GA0 ) );
				dst += 16;
			}
		}

		uint32_t* dst32 = ( uint32_t* ) dst;
		uint32_t* src32 = ( uint32_t* ) src;
		i = ( n & 0x7 ) >> 1;
		while( i-- ) {
			uint32_t in, out;
			int r, g, b, y0, y1, u, v;

			in = *src32++;
			v = ( in >> 24 ) - 128;
			y1 = ( ( ( int ) ( ( in >> 16 ) & 0xff ) - 16 ) * 1192 ) >> 10;
			u = ( ( in >> 8 ) & 0xff ) - 128;
			y0 = ( ( ( int ) ( in & 0xff ) - 16 ) * 1192 ) >> 10;
			r = ((v*1634) >> 10);
			g = ((u*401 + v*832) >> 10);
			b = ((u*2066) >> 10);

			// clamp the values
			out = 0xff000000;
			out |= Math::clamp( y0 + r, 0, 255 );
			out |= Math::clamp( y0 - g, 0, 255 ) << 8;
			out |= Math::clamp( y0 + b, 0, 255 ) << 16;
			*dst32++ = out;
			out = 0xff000000;
			out |= Math::clamp( y1 + r, 0, 255 );
			out |= Math::clamp( y1 - g, 0, 255 ) << 8;
			out |= Math::clamp( y1 + b, 0, 255 ) << 16;
			*dst32++ = out;
		}

	}

	void SIMDSSE2::Conv_YUYVu8_to_BGRAu8( uint8_t* dst, const uint8_t* src, const size_t n ) const
	{
		const __m128i Y2RGB = _mm_set_epi16( 1192, 0, 1192, 0, 1192, 0, 1192, 0 );
		const __m128i UV2R  = _mm_set_epi16( 1634, 0, 1634, 0, 1634, 0, 1634, 0 );
		const __m128i UV2G  = _mm_set_epi16( -832, -401, -832, -401, -832, -401, -832, -401 );
		const __m128i UV2B  = _mm_set_epi16( 0, 2066, 0, 2066, 0, 2066, 0, 2066 );
		const __m128i UVOFFSET = _mm_set1_epi16( 128 );
		const __m128i YOFFSET = _mm_set1_epi16( 16 );
		const __m128i A32  = _mm_set1_epi32( 0xff );
		const __m128i mask = _mm_set1_epi16( 0xff00 );

		__m128i uyvy;
		__m128i uv, yz, y, z;
		__m128i uvR, uvG, uvB;
		__m128i r, g, b, a;
		__m128i RB0, RB1, GA0, GA1;

		int i = n >> 3;
		if( ( ( size_t ) src | ( size_t ) dst ) & 0xf ) {
			while( i-- ) {
				uyvy = _mm_loadu_si128( ( __m128i* ) src );
				src += 16;
				/* U0 Y0 V0 Z0 U1 Y1 V1 Z1 U2 Y2 V2 Z2 U3 Y3 V3 Z3 */

				uv = _mm_and_si128( mask, uyvy );
				uv = _mm_srli_si128( uv, 1 );
				uv = _mm_sub_epi16( uv, UVOFFSET ); /* U0 V0 U1 V1 ... */

				yz = _mm_andnot_si128( mask, uyvy );
				yz = _mm_sub_epi16( yz, YOFFSET );  /* Y0 Z0 Y1 Z1 ...  */

				z = _mm_madd_epi16( yz, Y2RGB );                      /* Z0 Z1 Z2 Z3 */
				y = _mm_madd_epi16( yz, _mm_srli_si128( Y2RGB, 2 ) ); /* Y0 Y1 Y2 Y3 */

				uvR = _mm_madd_epi16( uv, UV2R );
				uvG = _mm_madd_epi16( uv, UV2G );
				uvB = _mm_madd_epi16( uv, UV2B );

				r  = _mm_srai_epi32( _mm_add_epi32( y, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( y, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( y, uvB ), 10 );

				RB0 = _mm_packs_epi32( r, b );
				GA0 = _mm_packs_epi32( g, A32 );

				r  = _mm_srai_epi32( _mm_add_epi32( z, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( z, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( z, uvB ), 10 );

				RB1 = _mm_packs_epi32( r, b );
				GA1 = _mm_packs_epi32( g, A32 );

				r  = _mm_unpacklo_epi16( RB0, RB1 );
				b  = _mm_unpackhi_epi16( RB0, RB1 );
				g  = _mm_unpacklo_epi16( GA0, GA1 );
				a  = _mm_unpackhi_epi16( GA0, GA1 );

				RB0 = _mm_unpacklo_epi16( b, r );
				RB1 = _mm_unpackhi_epi16( b, r );
				RB0 = _mm_packus_epi16( RB0, RB1 );

				GA0 = _mm_unpacklo_epi16( g, a );
				GA1 = _mm_unpackhi_epi16( g, a );
				GA0 = _mm_packus_epi16( GA0, GA1 );

				_mm_storeu_si128( ( __m128i* ) dst,  _mm_unpacklo_epi8( RB0, GA0 ) );
				dst += 16;
				_mm_storeu_si128( ( __m128i* ) dst,  _mm_unpackhi_epi8( RB0, GA0 ) );
				dst += 16;
			}
		} else {
			while( i-- ) {
				uyvy = _mm_load_si128( ( __m128i* ) src );
				src += 16;
				/* U0 Y0 V0 Z0 U1 Y1 V1 Z1 U2 Y2 V2 Z2 U3 Y3 V3 Z3 */

				uv = _mm_and_si128( mask, uyvy );
				uv = _mm_srli_si128( uv, 1 );
				uv = _mm_sub_epi16( uv, UVOFFSET ); /* U0 V0 U1 V1 ... */

				yz = _mm_andnot_si128( mask, uyvy );
				yz = _mm_sub_epi16( yz, YOFFSET );  /* Y0 Z0 Y1 Z1 ...  */

				z = _mm_madd_epi16( yz, Y2RGB );                      /* Z0 Z1 Z2 Z3 */
				y = _mm_madd_epi16( yz, _mm_srli_si128( Y2RGB, 2 ) ); /* Y0 Y1 Y2 Y3 */

				uvR = _mm_madd_epi16( uv, UV2R );
				uvG = _mm_madd_epi16( uv, UV2G );
				uvB = _mm_madd_epi16( uv, UV2B );

				r  = _mm_srai_epi32( _mm_add_epi32( y, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( y, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( y, uvB ), 10 );

				RB0 = _mm_packs_epi32( r, b );
				GA0 = _mm_packs_epi32( g, A32 );

				r  = _mm_srai_epi32( _mm_add_epi32( z, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( z, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( z, uvB ), 10 );

				RB1 = _mm_packs_epi32( r, b );
				GA1 = _mm_packs_epi32( g, A32 );

				r  = _mm_unpacklo_epi16( RB0, RB1 );
				b  = _mm_unpackhi_epi16( RB0, RB1 );
				g  = _mm_unpacklo_epi16( GA0, GA1 );
				a  = _mm_unpackhi_epi16( GA0, GA1 );

				RB0 = _mm_unpacklo_epi16( b, r );
				RB1 = _mm_unpackhi_epi16( b, r );
				RB0 = _mm_packus_epi16( RB0, RB1 );

				GA0 = _mm_unpacklo_epi16( g, a );
				GA1 = _mm_unpackhi_epi16( g, a );
				GA0 = _mm_packus_epi16( GA0, GA1 );

				_mm_stream_si128( ( __m128i* ) dst,  _mm_unpacklo_epi8( RB0, GA0 ) );
				dst += 16;
				_mm_stream_si128( ( __m128i* ) dst,  _mm_unpackhi_epi8( RB0, GA0 ) );
				dst += 16;
			}
		}

		uint32_t* dst32 = ( uint32_t* ) dst;
		uint32_t* src32 = ( uint32_t* ) src;
		i = ( n & 0x7 ) >> 1;
		while( i-- ) {
			uint32_t in, out;
			int r, g, b, y0, y1, u, v;

			in = *src32++;
			v = ( in >> 24 ) - 128;
			y1 = ( ( ( int ) ( ( in >> 16 ) & 0xff ) - 16 ) * 1192 ) >> 10;
			u = ( ( in >> 8 ) & 0xff ) - 128;
			y0 = ( ( ( int ) ( in & 0xff ) - 16 ) * 1192 ) >> 10;
			r = ((v*1634) >> 10);
			g = ((u*401 + v*832) >> 10);
			b = ((u*2066) >> 10);

			// clamp the values
			out = 0xff000000;
			out |= Math::clamp( y0 + r, 0, 255 ) << 16;
			out |= Math::clamp( y0 - g, 0, 255 ) << 8;
			out |= Math::clamp( y0 + b, 0, 255 );
			*dst32++ = out;
			out = 0xff000000;
			out |= Math::clamp( y1 + r, 0, 255 ) << 16;
			out |= Math::clamp( y1 - g, 0, 255 ) << 8;
			out |= Math::clamp( y1 + b, 0, 255 );
			*dst32++ = out;
		}
	}


	void SIMDSSE2::Conv_UYVYu8_to_RGBAu8( uint8_t* dst, const uint8_t* src, const size_t n ) const
	{
		const __m128i Y2RGB = _mm_set_epi16( 1192, 0, 1192, 0, 1192, 0, 1192, 0 );
		const __m128i UV2R  = _mm_set_epi16( 1634, 0, 1634, 0, 1634, 0, 1634, 0 );
		const __m128i UV2G  = _mm_set_epi16( -832, -401, -832, -401, -832, -401, -832, -401 );
		const __m128i UV2B  = _mm_set_epi16( 0, 2066, 0, 2066, 0, 2066, 0, 2066 );
		const __m128i UVOFFSET = _mm_set1_epi16( 128 );
		const __m128i YOFFSET = _mm_set1_epi16( 16 );
		const __m128i A32  = _mm_set1_epi32( 0xff );
		const __m128i mask = _mm_set1_epi16( 0xff00 );

		__m128i uyvy;
		__m128i uv, yz, y, z;
		__m128i uvR, uvG, uvB;
		__m128i r, g, b, a;
		__m128i RB0, RB1, GA0, GA1;

		int i = n >> 3;
		if( ( ( size_t ) src | ( size_t ) dst ) & 0xf ) {
			while( i-- ) {
				uyvy = _mm_loadu_si128( ( __m128i* ) src );
				src += 16;
				/* U0 Y0 V0 Z0 U1 Y1 V1 Z1 U2 Y2 V2 Z2 U3 Y3 V3 Z3 */

				yz = _mm_and_si128( mask, uyvy );
				yz = _mm_srli_si128( yz, 1 );
				yz = _mm_sub_epi16( yz, YOFFSET );  /* Y0 Z0 Y1 Z1 ...  */

				uv = _mm_andnot_si128( mask, uyvy );
				uv = _mm_sub_epi16( uv, UVOFFSET ); /* U0 V0 U1 V1 ... */

				z = _mm_madd_epi16( yz, Y2RGB );                      /* Z0 Z1 Z2 Z3 */
				y = _mm_madd_epi16( yz, _mm_srli_si128( Y2RGB, 2 ) ); /* Y0 Y1 Y2 Y3 */

				uvR = _mm_madd_epi16( uv, UV2R );
				uvG = _mm_madd_epi16( uv, UV2G );
				uvB = _mm_madd_epi16( uv, UV2B );

				r  = _mm_srai_epi32( _mm_add_epi32( y, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( y, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( y, uvB ), 10 );

				RB0 = _mm_packs_epi32( r, b );
				GA0 = _mm_packs_epi32( g, A32 );

				r  = _mm_srai_epi32( _mm_add_epi32( z, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( z, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( z, uvB ), 10 );

				RB1 = _mm_packs_epi32( r, b );
				GA1 = _mm_packs_epi32( g, A32 );

				r  = _mm_unpacklo_epi16( RB0, RB1 );
				b  = _mm_unpackhi_epi16( RB0, RB1 );
				g  = _mm_unpacklo_epi16( GA0, GA1 );
				a  = _mm_unpackhi_epi16( GA0, GA1 );

				RB0 = _mm_unpacklo_epi16( r, b );
				RB1 = _mm_unpackhi_epi16( r, b );
				RB0 = _mm_packus_epi16( RB0, RB1 );

				GA0 = _mm_unpacklo_epi16( g, a );
				GA1 = _mm_unpackhi_epi16( g, a );
				GA0 = _mm_packus_epi16( GA0, GA1 );

				_mm_storeu_si128( ( __m128i* ) dst,  _mm_unpacklo_epi8( RB0, GA0 ) );
				dst += 16;
				_mm_storeu_si128( ( __m128i* ) dst,  _mm_unpackhi_epi8( RB0, GA0 ) );
				dst += 16;
			}
		} else {
			while( i-- ) {
				uyvy = _mm_load_si128( ( __m128i* ) src );
				src += 16;
				/* U0 Y0 V0 Z0 U1 Y1 V1 Z1 U2 Y2 V2 Z2 U3 Y3 V3 Z3 */

				yz = _mm_and_si128( mask, uyvy );
				yz = _mm_srli_si128( yz, 1 );
				yz = _mm_sub_epi16( yz, YOFFSET );  /* Y0 Z0 Y1 Z1 ...  */

				uv = _mm_andnot_si128( mask, uyvy );
				uv = _mm_sub_epi16( uv, UVOFFSET ); /* U0 V0 U1 V1 ... */

				z = _mm_madd_epi16( yz, Y2RGB );                      /* Z0 Z1 Z2 Z3 */
				y = _mm_madd_epi16( yz, _mm_srli_si128( Y2RGB, 2 ) ); /* Y0 Y1 Y2 Y3 */

				uvR = _mm_madd_epi16( uv, UV2R );
				uvG = _mm_madd_epi16( uv, UV2G );
				uvB = _mm_madd_epi16( uv, UV2B );

				r  = _mm_srai_epi32( _mm_add_epi32( y, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( y, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( y, uvB ), 10 );

				RB0 = _mm_packs_epi32( r, b );
				GA0 = _mm_packs_epi32( g, A32 );

				r  = _mm_srai_epi32( _mm_add_epi32( z, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( z, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( z, uvB ), 10 );

				RB1 = _mm_packs_epi32( r, b );
				GA1 = _mm_packs_epi32( g, A32 );

				r  = _mm_unpacklo_epi16( RB0, RB1 );
				b  = _mm_unpackhi_epi16( RB0, RB1 );
				g  = _mm_unpacklo_epi16( GA0, GA1 );
				a  = _mm_unpackhi_epi16( GA0, GA1 );

				RB0 = _mm_unpacklo_epi16( r, b );
				RB1 = _mm_unpackhi_epi16( r, b );
				RB0 = _mm_packus_epi16( RB0, RB1 );

				GA0 = _mm_unpacklo_epi16( g, a );
				GA1 = _mm_unpackhi_epi16( g, a );
				GA0 = _mm_packus_epi16( GA0, GA1 );

				_mm_stream_si128( ( __m128i* ) dst,  _mm_unpacklo_epi8( RB0, GA0 ) );
				dst += 16;
				_mm_stream_si128( ( __m128i* ) dst,  _mm_unpackhi_epi8( RB0, GA0 ) );
				dst += 16;
			}
		}

		uint32_t* dst32 = ( uint32_t* ) dst;
		uint32_t* src32 = ( uint32_t* ) src;
		i = ( n & 0x7 ) >> 1;
		while( i-- ) {
			uint32_t in, out;
			int r, g, b, y0, y1, u, v;

			in = *src32++;
			v = ( ( in >> 16 ) & 0xff ) - 128;
			y1 = ( ( ( int ) ( in >> 24 ) - 16 ) * 1192 ) >> 10;
			u = ( in & 0xff ) - 128;
			y0 = ( ( ( int ) ( ( in >> 8 ) & 0xff ) - 16 ) * 1192 ) >> 10;
			r = ((v*1634) >> 10);
			g = ((u*401 + v*832) >> 10);
			b = ((u*2066) >> 10);

			// clamp the values
			out = 0xff000000;
			out |= Math::clamp( y0 + r, 0, 255 );
			out |= Math::clamp( y0 - g, 0, 255 ) << 8;
			out |= Math::clamp( y0 + b, 0, 255 ) << 16;
			*dst32++ = out;
			out = 0xff000000;
			out |= Math::clamp( y1 + r, 0, 255 );
			out |= Math::clamp( y1 - g, 0, 255 ) << 8;
			out |= Math::clamp( y1 + b, 0, 255 ) << 16;
			*dst32++ = out;
		}

	}

	void SIMDSSE2::Conv_UYVYu8_to_BGRAu8( uint8_t* dst, const uint8_t* src, const size_t n ) const
	{
		const __m128i Y2RGB = _mm_set_epi16( 1192, 0, 1192, 0, 1192, 0, 1192, 0 );
		const __m128i UV2R  = _mm_set_epi16( 1634, 0, 1634, 0, 1634, 0, 1634, 0 );
		const __m128i UV2G  = _mm_set_epi16( -832, -401, -832, -401, -832, -401, -832, -401 );
		const __m128i UV2B  = _mm_set_epi16( 0, 2066, 0, 2066, 0, 2066, 0, 2066 );
		const __m128i UVOFFSET = _mm_set1_epi16( 128 );
		const __m128i YOFFSET = _mm_set1_epi16( 16 );
		const __m128i A32  = _mm_set1_epi32( 0xff );
		const __m128i mask = _mm_set1_epi16( 0xff00 );

		__m128i uyvy;
		__m128i uv, yz, y, z;
		__m128i uvR, uvG, uvB;
		__m128i r, g, b, a;
		__m128i RB0, RB1, GA0, GA1;

		int i = n >> 3;
		if( ( ( size_t ) src | ( size_t ) dst ) & 0xf ) {
			while( i-- ) {
				uyvy = _mm_loadu_si128( ( __m128i* ) src );
				src += 16;
				/* U0 Y0 V0 Z0 U1 Y1 V1 Z1 U2 Y2 V2 Z2 U3 Y3 V3 Z3 */

				yz = _mm_and_si128( mask, uyvy );
				yz = _mm_srli_si128( yz, 1 );
				yz = _mm_sub_epi16( yz, YOFFSET );  /* Y0 Z0 Y1 Z1 ...  */

				uv = _mm_andnot_si128( mask, uyvy );
				uv = _mm_sub_epi16( uv, UVOFFSET ); /* U0 V0 U1 V1 ... */

				z = _mm_madd_epi16( yz, Y2RGB );                      /* Z0 Z1 Z2 Z3 */
				y = _mm_madd_epi16( yz, _mm_srli_si128( Y2RGB, 2 ) ); /* Y0 Y1 Y2 Y3 */

				uvR = _mm_madd_epi16( uv, UV2R );
				uvG = _mm_madd_epi16( uv, UV2G );
				uvB = _mm_madd_epi16( uv, UV2B );

				r  = _mm_srai_epi32( _mm_add_epi32( y, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( y, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( y, uvB ), 10 );

				RB0 = _mm_packs_epi32( r, b );
				GA0 = _mm_packs_epi32( g, A32 );

				r  = _mm_srai_epi32( _mm_add_epi32( z, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( z, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( z, uvB ), 10 );

				RB1 = _mm_packs_epi32( r, b );
				GA1 = _mm_packs_epi32( g, A32 );

				r  = _mm_unpacklo_epi16( RB0, RB1 );
				b  = _mm_unpackhi_epi16( RB0, RB1 );
				g  = _mm_unpacklo_epi16( GA0, GA1 );
				a  = _mm_unpackhi_epi16( GA0, GA1 );

				RB0 = _mm_unpacklo_epi16( b, r );
				RB1 = _mm_unpackhi_epi16( b, r );
				RB0 = _mm_packus_epi16( RB0, RB1 );

				GA0 = _mm_unpacklo_epi16( g, a );
				GA1 = _mm_unpackhi_epi16( g, a );
				GA0 = _mm_packus_epi16( GA0, GA1 );

				_mm_storeu_si128( ( __m128i* ) dst,  _mm_unpacklo_epi8( RB0, GA0 ) );
				dst += 16;
				_mm_storeu_si128( ( __m128i* ) dst,  _mm_unpackhi_epi8( RB0, GA0 ) );
				dst += 16;
			}
		} else {
			while( i-- ) {
				uyvy = _mm_load_si128( ( __m128i* ) src );
				src += 16;
				/* U0 Y0 V0 Z0 U1 Y1 V1 Z1 U2 Y2 V2 Z2 U3 Y3 V3 Z3 */

				yz = _mm_and_si128( mask, uyvy );
				yz = _mm_srli_si128( yz, 1 );
				yz = _mm_sub_epi16( yz, YOFFSET );  /* Y0 Z0 Y1 Z1 ...  */

				uv = _mm_andnot_si128( mask, uyvy );
				uv = _mm_sub_epi16( uv, UVOFFSET ); /* U0 V0 U1 V1 ... */

				z = _mm_madd_epi16( yz, Y2RGB );                      /* Z0 Z1 Z2 Z3 */
				y = _mm_madd_epi16( yz, _mm_srli_si128( Y2RGB, 2 ) ); /* Y0 Y1 Y2 Y3 */

				uvR = _mm_madd_epi16( uv, UV2R );
				uvG = _mm_madd_epi16( uv, UV2G );
				uvB = _mm_madd_epi16( uv, UV2B );

				r  = _mm_srai_epi32( _mm_add_epi32( y, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( y, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( y, uvB ), 10 );

				RB0 = _mm_packs_epi32( r, b );
				GA0 = _mm_packs_epi32( g, A32 );

				r  = _mm_srai_epi32( _mm_add_epi32( z, uvR ), 10 );
				g  = _mm_srai_epi32( _mm_add_epi32( z, uvG ), 10 );
				b  = _mm_srai_epi32( _mm_add_epi32( z, uvB ), 10 );

				RB1 = _mm_packs_epi32( r, b );
				GA1 = _mm_packs_epi32( g, A32 );

				r  = _mm_unpacklo_epi16( RB0, RB1 );
				b  = _mm_unpackhi_epi16( RB0, RB1 );
				g  = _mm_unpacklo_epi16( GA0, GA1 );
				a  = _mm_unpackhi_epi16( GA0, GA1 );

				RB0 = _mm_unpacklo_epi16( b, r );
				RB1 = _mm_unpackhi_epi16( b, r );
				RB0 = _mm_packus_epi16( RB0, RB1 );

				GA0 = _mm_unpacklo_epi16( g, a );
				GA1 = _mm_unpackhi_epi16( g, a );
				GA0 = _mm_packus_epi16( GA0, GA1 );

				_mm_stream_si128( ( __m128i* ) dst,  _mm_unpacklo_epi8( RB0, GA0 ) );
				dst += 16;
				_mm_stream_si128( ( __m128i* ) dst,  _mm_unpackhi_epi8( RB0, GA0 ) );
				dst += 16;
			}
		}

		uint32_t* dst32 = ( uint32_t* ) dst;
		uint32_t* src32 = ( uint32_t* ) src;
		i = ( n & 0x7 ) >> 1;
		while( i-- ) {
			uint32_t in, out;
			int r, g, b, y0, y1, u, v;

			in = *src32++;
			v = ( ( in >> 16 ) & 0xff ) - 128;
			y1 = ( ( ( int ) ( in >> 24 ) - 16 ) * 1192 ) >> 10;
			u = ( in & 0xff ) - 128;
			y0 = ( ( ( int ) ( ( in >> 8 ) & 0xff ) - 16 ) * 1192 ) >> 10;
			r = ((v*1634) >> 10);
			g = ((u*401 + v*832) >> 10);
			b = ((u*2066) >> 10);

			// clamp the values
			out = 0xff000000;
			out |= Math::clamp( y0 + r, 0, 255 ) << 16;
			out |= Math::clamp( y0 - g, 0, 255 ) << 8;
			out |= Math::clamp( y0 + b, 0, 255 );
			*dst32++ = out;
			out = 0xff000000;
			out |= Math::clamp( y1 + r, 0, 255 ) << 16;
			out |= Math::clamp( y1 - g, 0, 255 ) << 8;
			out |= Math::clamp( y1 + b, 0, 255 );
			*dst32++ = out;
		}
	}

	void SIMDSSE2::pyrdownHalfHorizontal_1u8_to_1u16( uint16_t* dst, const uint8_t* src, size_t n ) const
	{
		const __m128i mask = _mm_set1_epi16( 0xff00 );
		__m128i odd, even, even6, res;

		*dst++ =  ( ( ( uint16_t ) *( src + 1 ) ) << 2 ) + ( ( ( uint16_t ) *( src + 1 ) ) << 1 ) +
			( ( ( uint16_t ) *( src ) + ( uint16_t ) *( src + 2 ) ) << 2 ) +
			( ( ( uint16_t ) *( src + 3 ) ) << 1 );

		const uint8_t* end = src + n - 16;
		while( src < end ) {
			odd = _mm_loadu_si128( ( __m128i* ) src );
			even = _mm_srli_si128( _mm_and_si128( mask, odd ), 1 );
			odd = _mm_andnot_si128( mask, odd );

			odd = _mm_slli_epi16( odd, 2 );
			even6 = _mm_add_epi16( _mm_slli_epi16( even, 2 ),  _mm_slli_epi16( even, 1 ) );

			res = _mm_srli_si128( _mm_add_epi16( odd, even ), 2 );
			res = _mm_add_epi16( res, _mm_add_epi16( odd, even6 ) );
			res = _mm_add_epi16( res, _mm_slli_si128( even, 2 ) );
			res = _mm_srli_si128( res, 2 );

			_mm_storeu_si128( ( __m128i* ) dst, res );

			dst += 6;
			src += 12;
		}

		size_t n2 = ( ( n >> 1 ) - 2 ) % 6;
		src += 3;
		while( n2-- ) {
			*dst++ = ( ( ( ( uint16_t ) *src ) << 2 ) + ( ( ( uint16_t ) *src ) << 1 ) +
					  ( ( ( uint16_t ) *( src + 1 ) ) << 2 ) + ( ( ( uint16_t ) *( src - 1 ) ) << 2 ) +
					  ( uint16_t ) *( src + 2 ) + ( uint16_t ) *( src - 2 ) );
			src += 2;
		}

		if( n & 1 ) {
			*dst++ = ( ( ( uint16_t ) *src ) << 2 ) + ( ( ( uint16_t ) *src ) << 1 ) + ( ( ( uint16_t ) *( src - 2 ) ) << 1 ) +
				( ( ( uint16_t ) *( src + 1 ) ) << 2 ) + ( ( ( uint16_t ) *( src - 1 ) ) << 2 );
		} else {
			*dst++ = ( ( ( uint16_t ) *src ) << 2 ) +
				( ( ( uint16_t ) *src ) << 1 ) +
				( ( ( ( ( uint16_t ) *( src - 1 ) ) << 2 ) +
				   ( uint16_t ) *( src - 2 ) ) << 1 );
		}
	}

	void SIMDSSE2::pyrdownHalfVertical_1u16_to_1u8( uint8_t* dst, uint16_t* rows[ 5 ], size_t n ) const
	{
		uint16_t tmp;
		uint16_t* src1 = rows[ 0 ];
		uint16_t* src2 = rows[ 1 ];
		uint16_t* src3 = rows[ 2 ];
		uint16_t* src4 = rows[ 3 ];
		uint16_t* src5 = rows[ 4 ];
		__m128i r, t, zero;

		zero = _mm_setzero_si128();
		size_t n2 = n >> 3;
		while( n2-- ) {
			r = _mm_loadu_si128( ( __m128i* ) src1 );
			r = _mm_add_epi16( r, _mm_loadu_si128( ( __m128i* ) src5 ) );
			t = _mm_loadu_si128( ( __m128i* ) src2 );
			t = _mm_add_epi16( t, _mm_loadu_si128( ( __m128i* ) src4 ) );
			r = _mm_add_epi16( r, _mm_slli_epi16( t, 2 ) );
			t = _mm_loadu_si128( ( __m128i* ) src3 );
			t = _mm_add_epi16( _mm_slli_epi16( t, 2 ), _mm_slli_epi16( t, 1 ) );
			r = _mm_add_epi16( r, t );
			r = _mm_srli_epi16( r, 8 );
			r = _mm_packus_epi16( r, zero );
			_mm_storel_epi64( ( __m128i* ) dst, r );

			src1 += 8;
			src2 += 8;
			src3 += 8;
			src4 += 8;
			src5 += 8;
			dst  += 8;
		}

		n &= 0x7;
		while( n-- ) {
			tmp = *src1++ + *src5++ + ( ( *src2++ + *src4++ ) << 2 ) + 6 * *src3++;
			*dst++ = ( uint8_t ) ( tmp >> 8 );
		}
	}

	/*
	float SIMDSSE2::harrisResponse1u8( const uint8_t* ptr, size_t stride, size_t , size_t , const float k ) const
	{
		const uint8_t* src = ptr - 4 * stride - 4 + 1;
		__m128i dx, dy, t1, t2, zero;
		__m128i ix, iy, ixy;
		float a, b, c;

#define	__horizontal_sum(r, rw) do { \
	rw = _mm_shuffle_epi32( r, _MM_SHUFFLE(1, 0, 3, 2)); \
	r = _mm_add_epi32(r, rw); \
	rw = _mm_shuffle_epi32( r, _MM_SHUFFLE(2, 3, 0, 1)); \
	r = _mm_add_epi32(r, rw); \
} while( 0 )

		zero = _mm_setzero_si128();
		ix = iy = ixy = _mm_setzero_si128();

		for( int i = 0; i < 7; i++ ) {
			t1 = _mm_loadl_epi64( ( __m128i* ) src );
			t2 = _mm_loadl_epi64( ( __m128i* ) ( src + 2 * stride ) );
			t1 = _mm_unpacklo_epi8( t1, zero );
			t2 = _mm_unpacklo_epi8( t2, zero );
			dy = _mm_sub_epi16( t2, t1 );
			dy = _mm_slli_si128( dy, 2 );
			src += stride;

			t1 = _mm_madd_epi16( dy, dy );
			iy = _mm_add_epi32( iy, t1 );

			t1 = _mm_loadl_epi64( ( __m128i* ) ( src - 1 ) );
			t2 = _mm_loadl_epi64( ( __m128i* ) ( src ) );
			t1 = _mm_slli_si128( t1, 1 );
			t1 = _mm_unpacklo_epi8( t1, zero );
			t2 = _mm_unpacklo_epi8( t2, zero );
			dx = _mm_sub_epi16( t2, t1 );
			dx = _mm_srli_si128( dx, 2 ); // FIXME: use and
			dx = _mm_slli_si128( dx, 2 );

			t1 = _mm_madd_epi16( dx, dx );
			ix = _mm_add_epi32( ix, t1 );

			t1 = _mm_madd_epi16( dx, dy );
			ixy = _mm_add_epi32( ixy, t1 );
		}
		__horizontal_sum( ix, t1 );
		__horizontal_sum( iy, t1 );
		__horizontal_sum( ixy, t1 );

#undef __horizontal_sum

		a = ( float ) _mm_cvtsi128_si32( ix );
		b = ( float ) _mm_cvtsi128_si32( iy );
		c = ( float ) _mm_cvtsi128_si32( ixy );

		return ( a * b - 2.0f * c * c ) - ( k * Math::sqr(a + b) );
	}
*/

    void SIMDSSE2::prefixSum1_u8_to_f( float * _dst, size_t dstStride, const uint8_t * _src, size_t srcStride, size_t width, size_t height ) const
    {
        // first row
        __m128i x;
        __m128i xl16, xh16, zero;
        __m128  xf, y;
        
        zero = _mm_setzero_si128();
        y = _mm_setzero_ps();
        
        size_t n = width >> 4;
        
        float * dst = _dst;
        const uint8_t * src = _src;
        
        while( n-- ){
            x = _mm_loadu_si128( ( __m128i* )src );
            
            // cast to 2x8 uint16:
            xl16 = _mm_unpacklo_epi8( x, zero );
            xh16 = _mm_unpackhi_epi8( x, zero );
            
            xl16 = _mm_add_epi16( xl16, _mm_slli_si128( xl16, 2 ) );
            xl16 = _mm_add_epi16( xl16, _mm_slli_si128( xl16, 4 ) );
            xl16 = _mm_add_epi16( xl16, _mm_slli_si128( xl16, 8 ) );
            
            xh16 = _mm_add_epi16( xh16, _mm_slli_si128( xh16, 2 ) );
            xh16 = _mm_add_epi16( xh16, _mm_slli_si128( xh16, 4 ) );
            xh16 = _mm_add_epi16( xh16, _mm_slli_si128( xh16, 8 ) );
            xh16 = _mm_add_epi16( xh16, _mm_set1_epi16( _mm_extract_epi16( xl16, 7 ) ) );            
            
            // process lower 8: 
            xf = _mm_cvtepi32_ps( _mm_unpacklo_epi16( xl16, zero ) );
            xf = _mm_add_ps( xf, y );            
            _mm_store_ps( dst, xf );
            
            xf = _mm_cvtepi32_ps( _mm_unpackhi_epi16( xl16, zero ) );
            xf = _mm_add_ps( xf, y );
            _mm_store_ps( dst + 4, xf );
            
            xf = _mm_cvtepi32_ps( _mm_unpacklo_epi16( xh16, zero ) );
            xf = _mm_add_ps( xf, y );            
            _mm_store_ps( dst + 8, xf );
            
            xf = _mm_cvtepi32_ps( _mm_unpackhi_epi16( xh16, zero ) );
            xf = _mm_add_ps( xf, y );
            _mm_store_ps( dst + 12, xf );
            
            y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );
            
            src += 16; 
            dst += 16;
        }
        n = width & 0xf;
        float yl;
        _mm_store_ss( &yl, y );
        while( n-- ){
            yl += *src++;
            *dst++ = yl;
        }
        height--;
        
        float * prevRow = _dst;
        _src += srcStride;
        _dst += dstStride;
        
        __m128 prev;
        
        while( height-- ){
            n = width >> 4;
            
            src = _src;
            dst = _dst;
            y = _mm_setzero_ps();
            
            while( n-- ){
                // next 16 values
                x = _mm_loadu_si128( ( __m128i* )src );
                
                // cast to 2x8 uint16:
                xl16 = _mm_unpacklo_epi8( x, zero );
                xh16 = _mm_unpackhi_epi8( x, zero );
                
                xl16 = _mm_add_epi16( xl16, _mm_slli_si128( xl16, 2 ) );
                xl16 = _mm_add_epi16( xl16, _mm_slli_si128( xl16, 4 ) );
                xl16 = _mm_add_epi16( xl16, _mm_slli_si128( xl16, 8 ) );
                
                xh16 = _mm_add_epi16( xh16, _mm_slli_si128( xh16, 2 ) );
                xh16 = _mm_add_epi16( xh16, _mm_slli_si128( xh16, 4 ) );
                xh16 = _mm_add_epi16( xh16, _mm_slli_si128( xh16, 8 ) );
                xh16 = _mm_add_epi16( xh16, _mm_set1_epi16( _mm_extract_epi16( xl16, 7 ) ) );            
                
                // process lower 8: 
                prev = _mm_load_ps( prevRow );
                xf = _mm_cvtepi32_ps( _mm_unpacklo_epi16( xl16, zero ) );
                xf = _mm_add_ps( xf, y );
                xf = _mm_add_ps( xf, prev );
                _mm_store_ps( dst, xf );
                
                prev = _mm_load_ps( prevRow + 4 );
                xf = _mm_cvtepi32_ps( _mm_unpackhi_epi16( xl16, zero ) );
                xf = _mm_add_ps( xf, y );
                xf = _mm_add_ps( xf, prev );
                _mm_store_ps( dst + 4, xf );
                
                prev = _mm_load_ps( prevRow + 8 );
                xf = _mm_cvtepi32_ps( _mm_unpacklo_epi16( xh16, zero ) );
                xf = _mm_add_ps( xf, y );   
                xf = _mm_add_ps( xf, prev );
                _mm_store_ps( dst + 8, xf );
                
                prev = _mm_load_ps( prevRow + 12 );
                xf = _mm_cvtepi32_ps( _mm_unpackhi_epi16( xh16, zero ) );
                xf = _mm_add_ps( xf, y );
                y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );
                xf = _mm_add_ps( xf, prev );
                _mm_store_ps( dst + 12, xf );
                
                src += 16; 
                dst += 16;
                prevRow += 16;
            }
            
            n = width & 0xf;
            _mm_store_ss( &yl, y );
            while( n-- ){
                yl += *src++;
                *dst++ = yl + *prevRow++;
            }
            
            prevRow = _dst;
            _dst += dstStride;
            _src += srcStride;
        }        
    }
    
    
    void SIMDSSE2::prefixSumSqr1_u8_to_f( float * _dst, size_t dStride, const uint8_t * _src, size_t srcStride, size_t width, size_t height ) const
    {
        // first row
        __m128i x;
        __m128i xl16, xh16, zero, xl32, xh32;        
        __m128  xf, y;
    
        
        zero = _mm_setzero_si128();
        y = _mm_setzero_ps();
        
        size_t n = width >> 4;
        
        float * dst = _dst;
        const uint8_t * src = _src;
        
        while( n-- ){
            // load next 16 bytes
            x = _mm_loadu_si128( ( __m128i* )src );
            
            // cast to 2x8 uint16:
            xl16 = _mm_unpacklo_epi8( x, zero );
            xh16 = _mm_unpackhi_epi8( x, zero );
            
            // calc the squares of the values:
            xl16 = _mm_mullo_epi16( xl16, xl16 );
            xh16 = _mm_mullo_epi16( xh16, xh16 );
            
            // now convert each into 2x4 float for save computations
            
            // the lower 8 of the overall 16
            xl32 = _mm_unpacklo_epi16( xl16, zero );
            xh32 = _mm_unpackhi_epi16( xl16, zero );
   
            // process lower 4 floats
            xf = _mm_cvtepi32_ps( xl32 ); // convert to float            
            xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 4 ) );
            xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 8 ) );            
            xf = _mm_add_ps( xf, y );// add current row sum
            y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );            
            _mm_store_ps( dst, xf );            
            
            // process upper 4 floats
            xf = _mm_cvtepi32_ps( xh32 );            
            xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 4 ) );
            xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 8 ) );            
            xf = _mm_add_ps( xf, y );// add current row sum
            _mm_store_ps( dst + 4, xf );            
            y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );
            
            // the lower 8 of the overall 16
            xl32 = _mm_unpacklo_epi16( xh16, zero );
            xh32 = _mm_unpackhi_epi16( xh16, zero );
            
            // process lower 4 floats
            xf = _mm_cvtepi32_ps( xl32 );            
            xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 4 ) );
            xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 8 ) );            
            xf = _mm_add_ps( xf, y );// add current row sum
            _mm_store_ps( dst + 8, xf );            
            y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );
            
            // process upper 4 floats
            xf = _mm_cvtepi32_ps( xh32 );            
            xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 4 ) );
            xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 8 ) );            
            xf = _mm_add_ps( xf, y );// add current row sum
            _mm_store_ps( dst + 12, xf );            
            y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );            
            
            src += 16; 
            dst += 16;
        }
        
        n = width & 0xf;
        float yl;
        _mm_store_ss( &yl, y );
        while( n-- ){
            yl += Math::sqr( ( float )*src++ );
            *dst++ = yl;
        }
        
        height--;
        
        float * prevRow = _dst;
        _src += srcStride;
        _dst += dStride;
        
        __m128 prev;
        
        while( height-- ){
            n = width >> 4;
            
            src = _src;
            dst = _dst;
            y = _mm_setzero_ps();
            
            while( n-- ){
                // load next 16 bytes
                x = _mm_loadu_si128( ( __m128i* )src );
                
                // cast to 2x8 uint16:
                xl16 = _mm_unpacklo_epi8( x, zero );
                xh16 = _mm_unpackhi_epi8( x, zero );
                
                // calc the squares of the values:
                xl16 = _mm_mullo_epi16( xl16, xl16 );
                xh16 = _mm_mullo_epi16( xh16, xh16 );
                
                // now convert each into 2x4 float for save computations
                
                // the lower 8 of the overall 16
                xl32 = _mm_unpacklo_epi16( xl16, zero );
                xh32 = _mm_unpackhi_epi16( xl16, zero );
                
                // process lower 4 floats
                xf = _mm_cvtepi32_ps( xl32 );                                                  
                xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 4 ) );
                xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 8 ) );            
                xf = _mm_add_ps( xf, y );// add current row sum
                y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );
                prev = _mm_load_ps( prevRow );
                xf = _mm_add_ps( xf, prev ); // add previous row
                _mm_store_ps( dst, xf );            
                
                // process upper 4 floats
                xf = _mm_cvtepi32_ps( xh32 );                
                xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 4 ) );
                xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 8 ) );            
                xf = _mm_add_ps( xf, y );// add current row sum
                y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );
                prev = _mm_load_ps( prevRow + 4 );
                xf = _mm_add_ps( xf, prev ); // add previous row
                _mm_store_ps( dst + 4, xf );            
                
                // the lower 8 of the overall 16
                xl32 = _mm_unpacklo_epi16( xh16, zero );
                xh32 = _mm_unpackhi_epi16( xh16, zero );
                
                // process lower 4 floats
                xf = _mm_cvtepi32_ps( xl32 );  
                xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 4 ) );
                xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 8 ) );            
                xf = _mm_add_ps( xf, y );// add current row sum
                y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );
                prev = _mm_load_ps( prevRow + 8 );
                xf = _mm_add_ps( xf, prev ); // add previous row
                _mm_store_ps( dst + 8, xf );
                
                // process upper 4 floats
                xf = _mm_cvtepi32_ps( xh32 );   
                xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 4 ) );
                xf = _mm_add_ps( xf, ( __m128 )_mm_slli_si128( ( __m128i )xf, 8 ) );            
                xf = _mm_add_ps( xf, y );// add current row sum
                y = _mm_shuffle_ps( xf, xf, _MM_SHUFFLE( 3, 3, 3, 3 ) );
                prev = _mm_load_ps( prevRow + 12 );
                xf = _mm_add_ps( xf, prev ); // add previous row
                _mm_store_ps( dst + 12, xf );
                
                src += 16; 
                dst += 16;    
                prevRow += 16;
            }
            
            n = width & 0xf;
            _mm_store_ss( &yl, y );
            while( n-- ){
                yl += Math::sqr( ( float )*src++ );
                *dst++ = yl + *prevRow++;
            }
            
            prevRow = _dst;
            _dst += dStride;
            _src += srcStride;
        }
    }
    
}
