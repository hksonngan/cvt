/*
			CVT - Computer Vision Tools Library

 	 Copyright (c) 2012, Philipp Heise, Sebastian Klose

 	THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 	PARTICULAR PURPOSE.
 */
#include <cvt/gl/progs/GLTexMLSProg.h>

#include <cvt/gfx/ImageAllocatorGL.h>

#include <cvt/gl/shader/120/texmls_120_vert.h>
#include <cvt/gl/shader/120/texmls_120_frag.h>
#include <cvt/gl/shader/150/texmls_150_vert.h>
#include <cvt/gl/shader/150/texmls_150_frag.h>

namespace cvt {
	GLTexMLSProg::GLTexMLSProg() : _vbo( GL_ARRAY_BUFFER ), _dptex( GL_TEXTURE_2D )
	{
		try {
			if( GL::isGLSLVersionSupported( 1, 50 ) ) {
				_glsl120 = false;
				build( _texmls_150_vert_source, _texmls_150_frag_source );
			} else {
				_glsl120 = true;
				build( _texmls_120_vert_source, _texmls_120_frag_source );
			}
		} catch( GLException e ) {
			std::cout << e.what() << e.log() << std::endl;
		}
		setGridSize( 100, 100 );

		bind();
		_mvploc = uniformLocation( "MVP" );
		_texloc = uniformLocation( "Tex" );
		_dptexloc = uniformLocation( "DPTex" );
		if( _glsl120 )
			_dptexsizeloc = uniformLocation( "DPTexSize" );
		glUniform1i( _texloc, 0 );
		glUniform1i( _dptexloc, 1 );
		unbind();
	}

	GLTexMLSProg::~GLTexMLSProg()
	{
	}

	void GLTexMLSProg::setGridSize( size_t gridx, size_t gridy )
	{
		_gridsize = gridx * gridy * 4;

		_vbo.alloc( GL_STATIC_DRAW, sizeof( GLfloat ) * 2 * 4 * gridx * gridy );
		GLfloat* v = ( GLfloat* ) _vbo.map( GL_WRITE_ONLY );
		float gx, gy = 0;
	    float incx = 1.0 / ( float ) gridx;
	    float incy = 1.0 / ( float ) gridy;
		for( size_t y = 0; y < gridy; y++ ) {
			gx = 0;
			for( size_t x = 0; x < gridx; x++ ) {
				*v++ = gx;
				*v++ = gy;
				*v++ = gx + incx;
				*v++ = gy;
				*v++ = gx + incx;
				*v++ = gy + incy;
				*v++ = gx;
				*v++ = gy + incy;
				gx += incx;
			}
			gy += incy;
		}
		_vbo.unmap();
		_vao.setVertexData( _vbo, 2, GL_FLOAT );
	}

	void GLTexMLSProg::setDisplacements( const Vector4f* disp, size_t size )
	{
		_dptex.alloc( GL_RGBA, size, 1, GL_RGBA, GL_FLOAT, ( float* ) disp, 0 );
		_dptex.bind();
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		_dptex.unbind();
	}

	void GLTexMLSProg::setProjection( const Matrix4f& projection )
	{
		_proj = projection;
	}

	void GLTexMLSProg::setAlpha( float alpha )
	{
		Color c( 1.0f, 1.0f, 1.0f, alpha );
		_vao.setColor( c );
	}

	void GLTexMLSProg::drawImage( int x, int y, int width, int height, const Image& img )
	{
		GLint w, h;

		if( img.memType() != IALLOCATOR_GL )
			return;

		w = Math::max( width, 1 );
		h = Math::max( height, 1 );

		Matrix4f proj = _proj;
		Matrix4f mdl( ( float ) w, 0, 0, ( float ) x,
					 0, ( float )h, 0, ( float )y,
					 0, 0, 1, 0,
					 0, 0, 0, 1 );
		proj *= mdl;
		glUniformMatrix4fv( _mvploc, 1, true , ( const GLfloat* ) proj.ptr() );
		if( _glsl120 ) {
			glUniform1f( _dptexsizeloc, ( float ) _dptex.width() );
		}
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, ( ( ImageAllocatorGL* ) img._mem  )->_tex2d );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		glActiveTexture( GL_TEXTURE1 );
		_dptex.bind();
		_vao.draw( GL_QUADS, 0, _gridsize );
		_dptex.unbind();
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, 0 );
	}

}
