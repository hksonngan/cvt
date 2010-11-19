#include <cvt/gfx/ImageAllocatorGL.h>
#include <cvt/util/SIMD.h>

#include <stdio.h>
#include <execinfo.h>

namespace cvt {

	ImageAllocatorGL::ImageAllocatorGL() : ImageAllocator(), _ptr( NULL ), _ptrcount( 0 ), _dirty( false )
	{
		glGenTextures( 1, &_tex2d );
		glGenBuffers( 1, &_glbuf );
	}

	ImageAllocatorGL::~ImageAllocatorGL()
	{
		glDeleteTextures( 1, &_tex2d );
		glDeleteBuffers( 1, &_glbuf );
	}

	void ImageAllocatorGL::alloc( size_t width, size_t height, const IFormat & format )
	{
		GLenum glformat, gltype;

		_width = width;
		_height = height;
		_format = format;
		_stride = Math::pad16( _width * _format.bpp );
		_size = _stride * _height;

		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _glbuf );
		glBufferData( GL_PIXEL_UNPACK_BUFFER, _size, NULL, GL_DYNAMIC_DRAW );
		glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );

		glBindTexture( GL_TEXTURE_2D, _tex2d );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glPixelStorei( GL_UNPACK_ROW_LENGTH, ( GLint ) ( _stride / ( _format.bpp ) ) );
		getGLFormat( _format, glformat, gltype );
		/* do not copy non-meaningful PBO content - just allocate space, since current PBO content is undefined */
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, ( GLsizei ) _width, ( GLsizei ) _height, 0, glformat, gltype, NULL );
	}

	void ImageAllocatorGL::copy( const ImageAllocator* x, const Recti* r = NULL )
	{
		const uint8_t* src;
		const uint8_t* osrc;
		size_t sstride, dstride;
		uint8_t* dst;
		uint8_t* odst;
		size_t i, n;
		Recti rect( 0, 0, ( int ) x->_width, ( int ) x->_height );
		SIMD* simd = SIMD::get();

		if( r )
			rect.intersect( *r );
		alloc( rect.width, rect.height, x->_format );

		osrc = src = x->map( &sstride );
		src += rect.y * sstride + x->_format.bpp * rect.x;
		odst = dst = map( &dstride );
		n =  x->_format.bpp * rect.width;

		i = rect.height;
		while( i-- ) {
			simd->Memcpy( dst, src, n );
			dst += sstride;
			src += dstride;
		}
		x->unmap( osrc );
		unmap( odst );
	}

	uint8_t* ImageAllocatorGL::map( size_t* stride )
	{
		uint8_t* ptr;

		*stride = _stride;
		if( !_ptrcount ) {
			glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _glbuf );
			_ptr = ( uint8_t* ) glMapBuffer( GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE );
			glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
		}
		_ptrcount++;
		_dirty = true;
		return ( uint8_t* ) _ptr;
	}

	const uint8_t* ImageAllocatorGL::map( size_t* stride ) const
	{
		uint8_t* ptr;

		*stride = _stride;
		if( !_ptrcount ) {
			glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _glbuf );
			_ptr = ( uint8_t* ) glMapBuffer( GL_PIXEL_UNPACK_BUFFER, GL_READ_WRITE );
			glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
		}
		_ptrcount++;
		return ( uint8_t* ) _ptr;
	}

	void ImageAllocatorGL::unmap( const uint8_t* ptr ) const
	{
		_ptrcount--;
		if( !_ptrcount ) {
			glBindBuffer( GL_PIXEL_UNPACK_BUFFER, _glbuf );
			if( _dirty ) {
				GLenum glformat, gltype;
				
				glUnmapBuffer( GL_PIXEL_UNPACK_BUFFER );

				glBindTexture( GL_TEXTURE_2D, _tex2d );
				glPixelStorei( GL_UNPACK_ROW_LENGTH, ( GLint ) ( _stride / ( _format.bpp ) ) );
				getGLFormat( _format, glformat, gltype );
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ( GLsizei ) _width, ( GLsizei ) _height, glformat, gltype, NULL);
				_dirty = false;
			} else {
				glUnmapBuffer( GL_PIXEL_UNPACK_BUFFER );
			}
			glBindBuffer( GL_PIXEL_UNPACK_BUFFER, 0 );
			_ptr = NULL;
		}
	}


	void ImageAllocatorGL::getGLFormat( const IFormat & format, GLenum& glformat, GLenum& gltype ) const
	{
		switch ( format.formatID ) {
			case IFORMAT_GRAY_UINT8:		glformat = GL_RED; gltype = GL_UNSIGNED_BYTE; break;
			case IFORMAT_GRAY_UINT16:		glformat = GL_RED; gltype = GL_UNSIGNED_SHORT; break;
			case IFORMAT_GRAY_INT16:		glformat = GL_RED; gltype = GL_SHORT; break;
			case IFORMAT_GRAY_FLOAT:		glformat = GL_RED; gltype = GL_FLOAT; break;
				
			case IFORMAT_GRAYALPHA_UINT8:	glformat = GL_RG; gltype = GL_UNSIGNED_BYTE; break;
			case IFORMAT_GRAYALPHA_UINT16:	glformat = GL_RG; gltype = GL_UNSIGNED_SHORT; break;
			case IFORMAT_GRAYALPHA_INT16:	glformat = GL_RG; gltype = GL_SHORT; break;
			case IFORMAT_GRAYALPHA_FLOAT:	glformat = GL_RG; gltype = GL_FLOAT; break;
				
			case IFORMAT_RGBA_UINT8:		glformat = GL_RGBA; gltype = GL_UNSIGNED_BYTE; break;
			case IFORMAT_RGBA_UINT16:		glformat = GL_RGBA; gltype = GL_UNSIGNED_SHORT; break;
			case IFORMAT_RGBA_INT16:		glformat = GL_RGBA; gltype = GL_SHORT; break;
			case IFORMAT_RGBA_FLOAT:		glformat = GL_RGBA; gltype = GL_FLOAT; break;
				
			case IFORMAT_BGRA_UINT8:		glformat = GL_BGRA; gltype = GL_UNSIGNED_BYTE; break;
			case IFORMAT_BGRA_UINT16:		glformat = GL_BGRA; gltype = GL_UNSIGNED_SHORT; break;
			case IFORMAT_BGRA_INT16:		glformat = GL_BGRA; gltype = GL_SHORT; break;
			case IFORMAT_BGRA_FLOAT:		glformat = GL_BGRA; gltype = GL_FLOAT; break;
				
			case IFORMAT_BAYER_RGGB_UINT8:	glformat = GL_RED; gltype = GL_UNSIGNED_BYTE; break;
			default:
				throw CVTException( "No equivalent CL format found" );
				break;
		}
	}
}
