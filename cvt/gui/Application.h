/*
			CVT - Computer Vision Tools Library

 	 Copyright (c) 2012, Philipp Heise, Sebastian Klose

 	THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 	PARTICULAR PURPOSE.
 */
#ifndef CVT_APPLICATION_H
#define CVT_APPLICATION_H

#include <stdlib.h>
#include <stdint.h>

#include <cvt/gui/TimeoutHandler.h>

namespace cvt {
	class WidgetImpl;
	class Widget;
	class GLContextImpl;

	class Application {
		friend class Widget;
		friend class GLContext;

		public:
			static void init() { instance(); }

			static void run() { ::atexit( Application::atexit ); instance()->runApp(); };
			static void exit() { instance()->exitApp(); };

			static uint32_t registerTimer( size_t interval, TimeoutHandler* t ) { return instance()->_registerTimer( interval, t ); }
			static void unregisterTimer( uint32_t id ) { instance()->_unregisterTimer( id ); }

			static bool hasGLSupport() { return instance()->_hasGLSupport(); }
			static bool hasCLSupport() { return instance()->_hasCLSupport(); }

		protected:
			Application() {};
			Application( const Application& );
			virtual ~Application() {};

			static WidgetImpl* registerWindow( Widget* w ) { return instance()->_registerWindow( w ); }
			static void unregisterWindow( WidgetImpl* w ) { instance()->_unregisterWindow( w ); }

			virtual WidgetImpl* _registerWindow( Widget* widget ) = 0;
			virtual void _unregisterWindow( WidgetImpl* widget ) = 0;

			virtual uint32_t _registerTimer( size_t interval, TimeoutHandler* t ) = 0;
			virtual void _unregisterTimer( uint32_t id ) = 0;

			virtual bool _hasGLSupport() = 0;
			virtual bool _hasCLSupport() = 0;

			virtual void runApp() = 0;
			virtual void exitApp() = 0;

			static void atexit();

			static Application* instance();
			static Application* _app;
	};
}

#endif
