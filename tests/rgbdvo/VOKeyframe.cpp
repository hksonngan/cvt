#include <VOKeyframe.h>

#include <cvt/gfx/IMapScoped.h>
#include <cvt/math/SE3.h>
#include <cvt/util/EigenBridge.h>

#include <Eigen/Dense>

namespace cvt
{
    VOKeyframe::VOKeyframe( const Image& rgb, const Image& depth, const Matrix4f& pose, const Matrix3f& K, float depthScaling ) :
        _pose( pose )
    {
        rgb.convert( _gray, IFormat::GRAY_FLOAT );
        computeGradients();
        computeJacobians( depth, K, 1.0f / depthScaling );
    }

    VOKeyframe::~VOKeyframe()
    {
    }

    void VOKeyframe::computeJacobians( const Image& depth, const Matrix3f& intrinsics, float depthScaling )
    {
        float invFx = 1.0f / intrinsics[ 0 ][ 0 ];
        float invFy = 1.0f / intrinsics[ 1 ][ 1 ];
        float cx    = intrinsics[ 0 ][ 2 ];
        float cy    = intrinsics[ 1 ][ 2 ];

        // temp vals
        std::vector<float> tmpx( depth.width() );
        std::vector<float> tmpy( depth.height() );

        for( size_t i = 0; i < tmpx.size(); i++ ){
            tmpx[ i ] = ( i - cx ) * invFx;
        }
        for( size_t i = 0; i < tmpy.size(); i++ ){
            tmpy[ i ] = ( i - cy ) * invFy;
        }

        IMapScoped<const float> gxMap( _gx );
        IMapScoped<const float> gyMap( _gy );
        IMapScoped<const float> grayMap( _gray );
        IMapScoped<const uint16_t> depthMap( depth );

        // eval the jacobians:
        Eigen::Vector3f p3d;
        Eigen::Vector2f g;
        Eigen::Matrix<float, 1, 6> j;
        SE3<float>::ScreenJacType J;

        HessianType H( HessianType::Zero() );

        Eigen::Matrix3f K;
        EigenBridge::toEigen( K, intrinsics );
        SE3<float> pose;

        float gradThreshold = Math::sqr( 0.1f );

        size_t npts = 0;

        size_t ptIdx = 0;
        for( size_t y = 0; y < depth.height(); y++ ){
            const float* gx = gxMap.ptr();
            const float* gy = gyMap.ptr();
            const float* value = grayMap.ptr();
            const uint16_t* d = depthMap.ptr();
            for( size_t x = 0; x < depth.width(); x++, ptIdx++ ){
                if( d[ x ] == 0 ){
                    continue;
                } else {
                    g[ 0 ] = -gx[ x ];
                    g[ 1 ] = -gy[ x ];

                    if( g.squaredNorm() < gradThreshold )
                        continue;

                    p3d[ 2 ] = d[ x ] * depthScaling;
                    p3d[ 0 ] = tmpx[ x ] * p3d[ 2 ];
                    p3d[ 1 ] = tmpy[ y ] * p3d[ 2 ];

                    pose.screenJacobian( J, p3d, K );


                    j = 0.5f * g.transpose() * J;

                    _jacobians.push_back( j );
                    _pixelValues.push_back( value[ x ] );
                    _points3d.push_back( Vector3f( p3d[ 0 ], p3d[ 1 ], p3d[ 2 ] ) );
                    _pixelPositions.push_back( Vector2f( x, y ) );

                    H.noalias() += j.transpose() * j;
                    npts++;
                }
            }
            gxMap++;
            gyMap++;
            grayMap++;
            depthMap++;
        }

        // precompute the inverse hessian
        _inverseHessian = H.inverse();

    }

    void VOKeyframe::computeGradients()
    {
        _gx.reallocate( _gray.width(), _gray.height(), IFormat::GRAY_FLOAT);
        _gy.reallocate( _gray.width(), _gray.height(), IFormat::GRAY_FLOAT );

        _gray.convolve( _gx, IKernel::HAAR_HORIZONTAL_3 );
        _gray.convolve( _gy, IKernel::HAAR_VERTICAL_3 );
    }
}
