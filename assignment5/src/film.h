#pragma once

#include "image.h"

class Filter;

// A helper class for super-sampling and smart filtering.

template<typename Scalar, int D>
class Film
{
public:
    Film(shared_ptr<ImageBase<Vector<Scalar, D>>> image, shared_ptr<Filter> filter) { image_ = image; filter_ = filter; }
    ~Film() {}

    // YOUR CODE HERE (EXTRA)
    // Implement this function to perform smarter filtering.
    void addSample( const Vector2f& pixelCoordinates, const Vector<Scalar, D-1>& sampleColor );

    void normalize_weights();   // divide all pixels with last entry (weight)

private:
    shared_ptr<ImageBase<Vector<Scalar, D>>>    image_     = nullptr;
    shared_ptr<Filter>                          filter_    = nullptr;
};


/*** YOUR CODE HERE (EXTRA)
     The idea is that each incoming sample is turned from an infinitesimal point-like
     thing into a function defined on a continuous domain by centering a filter around
     the sample position. Then, it's a simple matter to evaluate the filter at the centers
     of the affected pixels, and add the color, weighted by the filter into the touched
     pixels. A neat trick is to carry the accumulated filter weight along in the fourth
     channel, so that you can divide by it in the end, and not worry about normalizing
     the filters.
**/
template<typename Scalar, int D>
void Film<Scalar, D>::addSample(const Vector2f& samplePosition, const Vector<Scalar, D-1>& sampleColor)
{
}

template<typename Scalar, int D>
void Film<Scalar, D>::normalize_weights()
{
    for (int j = 0; j < image_->getSize()(1); ++j)
        for (int i = 0; i < image_->getSize()(0); ++i)
            image_->pixel(i, j) = image_->pixel(i, j) / image_->pixel(i, j)[D-1];
}

