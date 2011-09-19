#ifndef CVT_QTKITCAMERA_H
#define CVT_QTKITCAMERA_H

#include <cvt/io/Camera.h>
#include <cvt/gfx/Image.h>
#include <cvt/util/String.h>

namespace cvt {

	class QTKitCameraInterface;
	
	class QTKitCamera : public Camera
	{
		public:
			QTKitCamera( size_t camIndex, const CameraMode & mode );
		
			virtual ~QTKitCamera();
		
			size_t width() const;
			size_t height() const;
			void nextFrame();
			const Image & frame() const;
			void startCapture();
			void stopCapture();
		
			const IFormat & format() const;
            const String &  identifier() const;
		
			static size_t count();		
			static void cameraInfo( size_t index, CameraInfo & info );
		
		private:
			QTKitCameraInterface *  _device;
            String                  _identifier;
			Image                   _frame;
            bool                    _capturing;
	};
	
	inline size_t QTKitCamera::width() const
	{
		return _frame.width();
	}
	
	inline size_t QTKitCamera::height() const
	{
		return _frame.height();
	}
	
	inline const Image & QTKitCamera::frame() const
	{
		return _frame;
	}
		
	inline const IFormat & QTKitCamera::format() const
	{
		return _frame.format();
	}
    
    inline const String & QTKitCamera::identifier() const 
    {
        return _identifier;
    }
	
}

#endif