#include <cctag/Level.hpp>
#include <cctag/filter/cvRecode.hpp>
#include <cctag/filter/thinning.hpp>
#include "cctag/talk.hpp"
#include "cuda/tag.h"

namespace cctag {

Level::Level( std::size_t width, std::size_t height, int level, bool cuda_allocates )
    : _level( level )
    , _cuda_allocates( cuda_allocates )
    , _mat_initialized_from_cuda( false )
    , _cols( width )
    , _rows( height )
{
    if( _cuda_allocates ) {
        _src   = 0;
        _dx    = 0;
        _dy    = 0;
        _mag   = 0;
        _edges = 0;
    } else {
        // Allocation
        _src   = new cv::Mat(height, width, CV_8UC1);
        _dx    = new cv::Mat(height, width, CV_16SC1 );
        _dy    = new cv::Mat(height, width, CV_16SC1 );
        _mag   = new cv::Mat(height, width, CV_16SC1 );
        _edges = new cv::Mat(height, width, CV_8UC1);
    }
    _temp = cv::Mat(height, width, CV_8UC1);

    _processed.data = new int8_t[height * width];
    _processed.step = width * sizeof(int8_t);
    _processed.cols = width;
    _processed.rows = height;
  
#ifdef CCTAG_EXTRA_LAYER_DEBUG
    _edgesNotThin = cv::Mat(height, width, CV_8UC1);
#endif
  
}

Level::~Level( )
{
    delete _src;
    delete _dx;
    delete _dy;
    delete _mag;
    delete _edges;
    delete _processed.data;
}

void Level::setLevel( const cv::Mat & src,
                      const double thrLowCanny,
                      const double thrHighCanny,
                      const cctag::Parameters* params )
{
    if( _cuda_allocates ) {
        std::cerr << "This function makes no sense with CUDA in " << __FUNCTION__ << ":" << __LINE__ << std::endl;
        exit( -__LINE__ );
    }

    cv::resize( src, *_src, cv::Size(_src->cols,_src->rows) );
    // ASSERT TODO : check that the data are allocated here
    // Compute derivative and canny edge extraction.
    cvRecodedCanny( *_src, *_edges, *_dx, *_dy,
                    thrLowCanny * 256, thrHighCanny * 256,
                    3 | CV_CANNY_L2_GRADIENT,
                    _level, params );
    // Perform the thinning.

#ifdef CCTAG_EXTRA_LAYER_DEBUG
    _edgesNotThin = _edges->clone();
#endif
  
    thin(*_edges,_temp);
}

#ifdef WITH_CUDA
void Level::setLevel( popart::TagPipe*         cuda_pipe,
                      const cctag::Parameters& params )
{
    if( not _cuda_allocates ) {
        std::cerr << "This function makes no sense without CUDA in " << __FUNCTION__ << ":" << __LINE__ << std::endl;
        exit( -__LINE__ );
    }

    _src   = cuda_pipe->getPlane( _level );
    _dx    = cuda_pipe->getDx( _level );
    _dy    = cuda_pipe->getDy( _level );
    _mag   = cuda_pipe->getMag( _level );
    _edges = cuda_pipe->getEdges( _level );
}
#endif // WITH_CUDA

const cv::Mat & Level::getSrc() const
{
    return *_src;
}

#ifdef CCTAG_EXTRA_LAYER_DEBUG
const cv::Mat & Level::getCannyNotThin() const
{
    return _edgesNotThin;
}
#endif

const cv::Mat & Level::getDx() const
{
    return *_dx;
}

const cv::Mat & Level::getDy() const
{
    return *_dy;
}

const cv::Mat & Level::getMag() const
{
    return *_mag;
}

const cv::Mat & Level::getEdges() const
{
    return *_edges;
}

void Level::resetProcessed( )
{
    memset( _processed.data, -1, _processed.cols * _processed.rows );
}

#if 0
void Level::setProcessed( int x, int y, int8_t val )
{
    if( x >= 0 && x < _processed.cols && y >= 0 && y < _processed.rows ) {
        _processed.ptr(y)[x] = val;
    } else {
        std::cerr << __FILE__ << ":" << __LINE__ << std::endl
                  << "    Coordinates out of bounds" << std::endl;
    }
}

int8_t Level::getProcessed( int x, int y ) const
{
    if( x >= 0 && x < _processed.cols && y >= 0 && y < _processed.rows ) {
        return _processed.ptr(y)[x];
    } else {
        std::cerr << __FILE__ << ":" << __LINE__ << std::endl
                  << "    Coordinates out of bounds" << std::endl;
        return -1;
    }
}
#endif


}
