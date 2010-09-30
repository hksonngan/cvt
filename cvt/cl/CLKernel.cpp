#include <cvt/cl/CLKernel.h>
#include <cvt/cl/CLImage.h>
#include <cvt/gfx/ImageAllocatorCL.h>
#include <cvt/gfx/Color.h>
#include <cvt/gfx/IFilterScalar.h>
#include <cvt/gfx/IFilterVector.h>

namespace cvt {

	void CLKernel::build( const char* name, const char* src, size_t len, std::string& buildinfo )
	{
		cl_int err;
		cl::Program::Sources source( 1, std::make_pair<const char*, size_t>( src, len ) );
		std::vector<cl::Device> devices;

		_name = name;
		_cl = CLContext::getCurrent();

		_prog = cl::Program( _cl->getCLContext( ), source, &err );
		if( err != CL_SUCCESS ) {
			throw CLException( err );
		}

		devices.push_back( _cl->getCLDevice( ) );
		err = _prog.build( devices, "-w -cl-mad-enable" );
		buildinfo = _prog.getBuildInfo<CL_PROGRAM_BUILD_LOG>( _cl->getCLDevice( ) );
		if( err != CL_SUCCESS ) {
			std::cout << buildinfo << std::endl;
			throw CLException( err, buildinfo );
		}

		_kernel = cl::Kernel( _prog, name, &err );
		if( err != CL_SUCCESS ) {
			throw CLException( err );
		}
	}

	void CLKernel::setArg( size_t i, IFilterParameter* p )
	{
		switch( p->getIFilterParameterType() )
		{
			case IFILTERPARAMETER_COLOR:
				_kernel.setArg( ( cl_uint ) i, sizeof( cl_float4 ), ( void* ) ( ( Color* ) p )->v );
				break;
			case IFILTERPARAMETER_SCALAR:
				_kernel.setArg( ( cl_uint ) i, sizeof( cl_float ), ( void* ) &( ( IFilterScalar* ) p )->value );
				break;
			case IFILTERPARAMETER_VECTOR2:
				_kernel.setArg( ( cl_uint ) i, sizeof( cl_float2 ), ( void* ) ( ( IFilterVector2* ) p )->vec );
				break;
			case IFILTERPARAMETER_VECTOR4:
				_kernel.setArg( ( cl_uint ) i, sizeof( cl_float4 ), ( void* ) ( ( IFilterVector4* ) p )->vec );
				break;
			case IFILTERPARAMETER_VECTOR8:
				_kernel.setArg( ( cl_uint ) i, sizeof( cl_float8 ), ( void* ) ( ( IFilterVector8* ) p )->vec );
				break;
			case IFILTERPARAMETER_VECTOR16:
				_kernel.setArg( ( cl_uint ) i, sizeof( cl_float16 ), ( void* ) ( ( IFilterVector16* ) p )->vec );
				break;
			case IFILTERPARAMETER_IMAGE:
				CLImage* climg;
				Image* img;
				if( ( climg = dynamic_cast<CLImage*>( p ) ) != NULL ) {
					_kernel.setArg( ( cl_uint ) i, climg->_climage() );
				} else if( ( img = dynamic_cast<Image*>( p ) ) != NULL ) {
					if( img->memType() != IALLOCATOR_CL )
						throw CVTException("Need CLImage for CLKernel");
					ImageAllocatorCL* clmem = ( ImageAllocatorCL* ) img->_mem;
					_kernel.setArg( ( cl_uint ) i, *clmem->_climage );
				} else
					throw CVTException("Need CLImage for CLKernel");
				break;
		}
	}

	void CLKernel::setArg( size_t n, size_t size )
	{
		_kernel.setArg( ( cl_uint ) n, size, NULL );
	}


	void CLKernel::run( const cl::NDRange& offset, const cl::NDRange& global, const cl::NDRange& local, std::vector<cl::Event>* events, cl::Event* event )
	{
		if( _cl ) {
			cl_int err = _cl->getCLQueue( ).enqueueNDRangeKernel( _kernel, offset, global, local, events, event );
			if( err != CL_SUCCESS ) {
				throw CLException( _name, err );
			}
		}
	}


	size_t CLKernel::workGroupSize( ) const
	{
		size_t ret;
		cl_int err;
		err = _kernel.getWorkGroupInfo( _cl->getCLDevice(), CL_KERNEL_WORK_GROUP_SIZE, &ret );
		if( err != CL_SUCCESS )
			throw CLException( _name, err );
		return ret;
	}


	std::vector<char*>* CLKernel::getBinaries( )
	{
		std::vector<size_t> sizes;
		std::vector<char*>* ret;

		_prog.getInfo( CL_PROGRAM_BINARY_SIZES, &sizes );
		ret = new std::vector<char*>;
		for( std::vector<size_t>::iterator it = sizes.begin(); it != sizes.end(); it++ ) {
			ret->push_back( new char[ *it ] );
		}
		_prog.getInfo( CL_PROGRAM_BINARIES, ret );
		return ret;
	}

}
