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


#ifndef CVT_RGBDVISUALODOMETRY_H
#define CVT_RGBDVISUALODOMETRY_H

#include <cvt/gfx/Image.h>
#include <cvt/util/EigenBridge.h>
#include <cvt/util/Signal.h>
#include <cvt/util/CVTAssert.h>
#include <cvt/util/ConfigFile.h>

#include <cvt/vision/rgbdvo/RGBDKeyframe.h>
#include <cvt/vision/rgbdvo/Optimizer.h>

namespace cvt {

    template <class KFType, class LossFunction>
    class RGBDVisualOdometry
    {
        public:
            typedef typename KFType::WarpType               Warp;
            typedef typename KFType::AlignDataType          AlignDataType;
            typedef RGBDKeyframe<AlignDataType>             KFBaseType;
            typedef std::vector<KFBaseType*>                KFVector;
            typedef Optimizer<AlignDataType, LossFunction>	OptimizerType;
            typedef typename OptimizerType::Result			Result;

            struct Params {
                Params() :
                    pyrOctaves( 3 ),
                    pyrScale( 0.5f ),
                    maxTranslationDistance( 0.4f ),
                    maxRotationDistance( Math::deg2Rad( 5.0f ) ),
                    maxSSDSqr( Math::sqr( 0.2f ) ),
                    minPixelPercentage( 0.3f ),
                    autoReferenceUpdate( true ),
                    depthScale( 1000.0f ),
                    minDepth( 0.5f ),
                    maxDepth( 10.0f ),
                    gradientThreshold( 0.02f ),
                    useInformationSelection( false ),
                    maxIters( 10 ),
                    minParameterUpdate( 1e-6 ),
                    maxNumKeyframes( 1 )
                {}

                Params( ConfigFile& cfg ) :
                    pyrOctaves( cfg.valueForName<int>( "pyrOctaves", 3 ) ),
                    pyrScale( cfg.valueForName<float>( "pyrScale", 0.5f ) ),
                    maxTranslationDistance( cfg.valueForName<float>( "maxTranslationDistance", 0.4f ) ),
                    maxRotationDistance( Math::deg2Rad( cfg.valueForName<float>( "maxRotationDistance", 5.0f ) ) ),
                    maxSSDSqr( Math::sqr( cfg.valueForName<float>( "maxSSD", 0.2f ) ) ),
                    minPixelPercentage( cfg.valueForName<float>( "minPixelPercentage", 0.3f ) ),
                    autoReferenceUpdate( cfg.valueForName<bool>( "autoReferenceUpdate", true ) ),
                    depthScale( cfg.valueForName<float>( "depthFactor", 1000.0f ) *
                                cfg.valueForName<float>( "depthScale", 1.0f ) ),
                    minDepth( cfg.valueForName<float>( "minDepth", 0.5f ) ),
                    maxDepth( cfg.valueForName<float>( "maxDepth", 10.0f ) ),
                    gradientThreshold( cfg.valueForName<float>( "gradientThreshold", 0.02f ) ),
                    useInformationSelection( cfg.valueForName<bool>( "useInformationSelection", false ) ),
                    selectionPixelPercentage( cfg.valueForName<float>( "selectionPixelPercentage", 0.3f ) ),
                    maxIters( cfg.valueForName<int>( "maxIters", 10 ) ),
                    minParameterUpdate( cfg.valueForName<float>( "minParameterUpdate", 1e-6f ) ),
                    maxNumKeyframes( cfg.valueForName<int>( "maxNumKeyframes", 1 ) )
                {
                    // TODO: Params should become a parameterset
                    // conversion between paramset and configfile!
                }

               // pyramid
               size_t pyrOctaves;
               float  pyrScale;

               // keyframe recreation thresholds
               float maxTranslationDistance;
               float maxRotationDistance;
               float maxSSDSqr;
               float minPixelPercentage;

               // automatic update of reference when needed
               bool autoReferenceUpdate;

			   // depthScale = #pixels / m
               float depthScale;
               float minDepth;
               float maxDepth;
               float gradientThreshold;

               bool  useInformationSelection;
               float selectionPixelPercentage;

               // optimizer:
               size_t   maxIters;
               float    minParameterUpdate;

               int      maxNumKeyframes;
            };

            RGBDVisualOdometry( OptimizerType* optimizer, const Matrix3f& K, const Params& params );
            ~RGBDVisualOdometry();

            /**
             *  \brief  update the pose by using the given pose as starting point
             *  \param  pose will be the initial value for the optimization and contains the computed result
             *          it has to be the pose from world to camera!
             */
            void updatePose( Matrix4f& pose, const Image& gray, const Image& depth );

            /**
             *  \param kfPose   pose of the keyframe: T_wk
             */
            void addNewKeyframe( const Image& gray, const Image& depth, const Matrix4f& kfPose );

            /**
             *  \brief  get the absolute (world) pose of the last aligned image
             */
            const Matrix4f& pose() const;

            size_t          numKeyframes()  const                   { return _keyframes.size(); }
            const Matrix3f& intrinsics()    const                   { return _intrinsics; }
            void            setPose( const Matrix4f& pose )         { _currentPose = pose; }
            size_t          numOverallKeyframes() const             { return _numCreated; }
            float           lastSSD()             const             { return _lastResult.costs; }
            size_t          lastNumPixels()       const             { return _lastResult.numPixels; }
            float           lastPixelPercentage() const             { return _lastResult.pixelPercentage * 100.0f; }
            void            setParameters( const Params& p )        { _params = p; }// TODO: some updates might not get reflected this way!
            const Params&   parameters() const                      { return _params; }
            OptimizerType*  optimizer()                             { return _optimizer; }
            const Matrix4f& activeKeyframePose() const              { return _activeKeyframe->pose(); }

            /******** SIGNALS ************/
            /**
             *  \brief  Signal that will be emitted when a new keyframe was added
             *  \param  Matrix4f will be the pose of the new keyframe in the map
             */
            Signal<const Matrix4f&>    keyframeAdded;

            EIGEN_MAKE_ALIGNED_OPERATOR_NEW
        private:
            Params                      _params;

            OptimizerType*              _optimizer;
            Matrix3f                    _intrinsics;

            // current active keyframe
            KFBaseType*                 _activeKeyframe;
            size_t                      _numCreated;
            KFVector                    _keyframes;

            ImagePyramid                _pyramid;
            Matrix4<float>              _currentPose;

            Result                      _lastResult;

            bool needNewKeyframe() const;
            void setKeyframeParams( KFBaseType& kf );
    };

    template <class KFType, class LossFunction>
    inline RGBDVisualOdometry<KFType, LossFunction>::RGBDVisualOdometry( OptimizerType* optimizer, const Matrix3f& K, const Params& p ) :
        _params( p ),
        _optimizer( optimizer ),
        _intrinsics( K ),
        _activeKeyframe( 0 ),
        _numCreated( 0 ),
        _pyramid( p.pyrOctaves, p.pyrScale )
    {
        _optimizer->setMaxIterations( _params.maxIters );
        _optimizer->setMinUpdate( _params.minParameterUpdate );
        _optimizer->setMinPixelPercentage( _params.minPixelPercentage );
        _currentPose.setIdentity();
    }

    template <class DerivedKF, class LossFunction>
    inline RGBDVisualOdometry<DerivedKF, LossFunction>::~RGBDVisualOdometry()
    {
        _activeKeyframe = 0;

        for( size_t i = 0; i < _keyframes.size(); ++i )
            delete _keyframes[ i ];
        _keyframes.clear();
    }

    template <class DerivedKF, class LossFunction>
    inline void RGBDVisualOdometry<DerivedKF, LossFunction>::updatePose( Matrix4f& pose, const Image& gray, const Image& depth )
    {
        _pyramid.update( gray );
        _optimizer->optimizeMultiframe( _lastResult, pose, &_keyframes[ 0 ], _keyframes.size(), _pyramid, depth );
        _currentPose = _lastResult.warp.pose();

        // check if we need a new keyframe
        if( _params.autoReferenceUpdate && needNewKeyframe() ){
            addNewKeyframe( gray, depth, _currentPose );
        }

        pose = _currentPose;
    }

    template <class KFType, class LossFunction>
    inline void RGBDVisualOdometry<KFType, LossFunction>::addNewKeyframe( const Image& gray, const Image& depth, const Matrix4f& kfPose )
    {
        CVT_ASSERT( ( gray.format()  == IFormat::GRAY_FLOAT ), "Gray image format has to be GRAY_FLOAT" );
        CVT_ASSERT( ( depth.format() == IFormat::GRAY_FLOAT ), "Depth image format has to be GRAY_FLOAT" );

        // update the current pose, to the pose of the new keyframe
        _currentPose = kfPose;
		if( _keyframes.size() < ( size_t )_params.maxNumKeyframes ){
			_keyframes.push_back( new KFType( _intrinsics, _pyramid.octaves(), _pyramid.scaleFactor() ) );
			_activeKeyframe = _keyframes[ _keyframes.size() - 1 ];

            // _pyramid only needs to be updated if its the first keyframe! -> this is ugly!
            _pyramid.update( gray );
        } else {
            // select one of the keyframes to be exchanged:
            float maxDist = 0;
            size_t idx = 0;
            if( _keyframes.size() > 1 ){
                Matrix4f curInv = kfPose.inverse();
                for( size_t i = 0; i < _keyframes.size(); i++ ){
                    // compute relative pose of this
                    Matrix4f rel = curInv * _keyframes[ i ]->pose();
                    Quaternionf q( rel.toMatrix3() );
                    float dist = q.toEuler().length();
                    dist += Math::sqr( rel.col( 3 ).lengthSqr() - 1.0f );
                    if( dist > maxDist ){
                        idx = i;
                    }
                }
            }
            _activeKeyframe = _keyframes[ idx ];
        }

        setKeyframeParams( *_activeKeyframe );
        _activeKeyframe->updateOfflineData( kfPose, _pyramid, depth );
        _lastResult.warp.initialize( kfPose );
        _numCreated++;

        // notify observers
        keyframeAdded.notify( _currentPose );
    }

    template <class KFType, class LossFunction>
    inline void RGBDVisualOdometry<KFType, LossFunction>::setKeyframeParams( KFBaseType &kf )
    {
        kf.setDepthMapScaleFactor( _params.depthScale );
        kf.setMinimumDepth( _params.minDepth );
        kf.setMaximumDepth( _params.maxDepth );
        kf.setGradientThreshold( _params.gradientThreshold );
        kf.setUseInformationSelection( _params.useInformationSelection );
        kf.setSelectionPixelPercentage( _params.selectionPixelPercentage );
    }

    template <class DerivedKF, class LossFunction>
    inline bool RGBDVisualOdometry<DerivedKF, LossFunction>::needNewKeyframe() const
    {
        // check the ssd:
        float avgSSD = -1.0f;
        if( _lastResult.numPixels ){
            avgSSD = _lastResult.costs / _lastResult.numPixels;
        }

        if( _lastResult.pixelPercentage < _params.minPixelPercentage ){
            if( avgSSD < _params.maxSSDSqr ){
                // we only should add a keyframe, if the last SSD was ok
                return true;
            }
        }

        Matrix4f relPose = _currentPose.inverse() * _activeKeyframe->pose();

        Vector4f t = relPose.col( 3 );
        t[ 3 ] = 0;
        float tmp = t.length();
        if( tmp > _params.maxTranslationDistance ){
            return true;
        }

        Matrix3f R = relPose.toMatrix3();
        Quaternionf q( R );
        Vector3f euler = q.toEuler();
        tmp = euler.length();
        if( tmp > _params.maxRotationDistance ){
            return true;
        }
        return false;
    }

    template <class DerivedKF, class LossFunction>
    inline const Matrix4f& RGBDVisualOdometry<DerivedKF, LossFunction>::pose() const
    {
        return _currentPose;
    }

}

#endif // RGBDVISUALODOMETRY_H
