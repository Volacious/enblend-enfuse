/*
 * Copyright (C) 2004 Andrew Mihal
 *
 * This file is part of Enblend.
 *
 * Enblend is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Enblend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Enblend; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __BLEND_H__
#define __BLEND_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "fixmath.h"

#include "vigra/combineimages.hxx"
#include "vigra/numerictraits.hxx"

using std::cout;
using std::vector;
using vigra::combineThreeImages;
using vigra::NumericTraits;

namespace enblend {

struct BlendFunctor {
    BlendFunctor(double maskMax) : scale(maskMax) {}

    template <typename MaskPixelType, typename ImagePixelType>
    ImagePixelType operator()(const MaskPixelType &maskP, const ImagePixelType &wP, const ImagePixelType &bP) const {

        typedef typename NumericTraits<ImagePixelType>::RealPromote RealImagePixelType;

        double whiteCoeff =
                NumericTraits<MaskPixelType>::toRealPromote(maskP) / scale;
        double blackCoeff = 1.0 - whiteCoeff;

        RealImagePixelType rwP = NumericTraits<ImagePixelType>::toRealPromote(wP);
        RealImagePixelType rbP = NumericTraits<ImagePixelType>::toRealPromote(bP);

        RealImagePixelType blendP = (whiteCoeff * rwP) + (blackCoeff * rbP);

        return NumericTraits<ImagePixelType>::fromRealPromote(blendP);
    }

    double scale;
};

template <typename OrigMaskType, typename MaskPyramidType, typename ImagePyramidType>
void blend(vector<MaskPyramidType*> *maskGP,
        vector<ImagePyramidType*> *whiteLP,
        vector<ImagePyramidType*> *blackLP) {

    typedef typename MaskPyramidType::value_type MaskPixelType;
    typedef typename OrigMaskType::value_type OrigMaskPixelType;

    // Discovert the maximum value that will be found in the mask pyramid.
    // We need this to scale the mask values to the range [0.0, 1.0].
    ConvertScalarToPyramidFunctor<OrigMaskPixelType, MaskPixelType> f;
    MaskPixelType maxMaskPixel = f(NumericTraits<OrigMaskPixelType>::max());
    double maxMaskPixelD = NumericTraits<MaskPixelType>::toRealPromote(maxMaskPixel);

    if (Verbose > VERBOSE_BLEND_MESSAGES) {
        cout << "Blending layers:";
        cout.flush();
    }

    for (unsigned int layer = 0; layer < maskGP->size(); layer++) {

        if (Verbose > VERBOSE_BLEND_MESSAGES) {
            cout << " l" << layer;
            cout.flush();
        }

        combineThreeImages(srcImageRange(*((*maskGP)[layer])),
                srcImage(*((*whiteLP)[layer])),
                srcImage(*((*blackLP)[layer])),
                destImage(*((*blackLP)[layer])),
                BlendFunctor(maxMaskPixelD));
    }

    if (Verbose > VERBOSE_BLEND_MESSAGES) {
        cout << endl;
    }

};

} // namespace enblend

#endif /* __BLEND_H__ */
