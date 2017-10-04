/*F******************************************************************************
 *
 * openSMILE - open Speech and Music Interpretation by Large-space Extraction
 *       the open-source Munich Audio Feature Extraction Toolkit
 * Copyright (C) 2008-2009  Florian Eyben, Martin Woellmer, Bjoern Schuller
 *
 *
 * Institute for Human-Machine Communication
 * Technische Universitaet Muenchen (TUM)
 * D-80333 Munich, Germany
 *
 *
 * If you use openSMILE or any code from openSMILE in your research work,
 * you are kindly asked to acknowledge the use of openSMILE in your publications.
 * See the file CITING.txt for details.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ******************************************************************************E*/


/*  openSMILE component:

Mel-frequency spectrum

*/


#ifndef __MELSPEC_HPP
#define __MELSPEC_HPP

#include <smileCommon.hpp>
#include <vectorProcessor.hpp>
#include <math.h>

#define COMPONENT_DESCRIPTION_CMELSPEC "computes N-band Mel/Bark-frequency spectrum by using overlapping triangular filters equidistant on the Mel/Bark-frequency scale"
#define COMPONENT_NAME_CMELSPEC "cMelspec"

#define SPECSCALE_HTKMEL   0
#define SPECSCALE_MEL      1
#define SPECSCALE_BARK     2
// TODO:
#define SPECSCALE_SEMITONE 3
#define TWELFTH_ROOT_OF_2  1.1224620483093729814335330496787

class cMelspec : public cVectorProcessor {
  private:
    int nBands, htkcompatible, usePower;
    FLOAT_DMEM **filterCoeffs;
    FLOAT_DMEM **filterCfs;
    long **chanMap;
    long bs;
    FLOAT_DMEM lofreq, hifreq;
    long *nLoF, *nHiF;
    int specScale;

    // Hertz to Mel/Bark/..
    FLOAT_DMEM Mel(FLOAT_DMEM fhz) {
      switch(specScale) {
        case SPECSCALE_HTKMEL : return (FLOAT_DMEM)(1127.0*log(1.0+(double)fhz/700.0));
        case SPECSCALE_MEL   : return (FLOAT_DMEM)(2595.0*log10(1.0+(double)fhz/700.0));
        case SPECSCALE_BARK :  return (FLOAT_DMEM)(13.1*atan(.00074*(double)fhz)+2.24*atan(((double)fhz)*((double)fhz)*1.85e-8)+1e-4*((double)fhz));
        // EXPERIMENTAL:
        case SPECSCALE_SEMITONE : {
                          if (fhz < 27.5) fhz = 27.5;
                          return (FLOAT_DMEM)( log( (double)(fhz) / 27.5 ) / log (TWELFTH_ROOT_OF_2) ) ;
                       }
        default: // = Htk Mel
          return (FLOAT_DMEM)(1127.0*log(1.0+(double)fhz/700.0));
      }
    }

    /*
    // Mel to Hertz
    FLOAT_DMEM Hertz(FLOAT_DMEM fmel) {
      switch(specScale) {
        case SPECSCALE_HTKMEL : return (FLOAT_DMEM)(700.0*(exp((double)fmel/1127.0)-1.0));        
        case SPECSCALE_MEL : return (FLOAT_DMEM)(700.0*(exp((double)fmel/2595.0)-1.0));
        case SPECSCALE_BARK:   break;
        default: // = Htk Mel
          return (FLOAT_DMEM)(700.0*(exp((double)fmel/1127.0)-1.0));
      }
      return 
    }
*/

    // convert frequency (hz) to FFT bin number
    long FtoN(FLOAT_DMEM fhz, FLOAT_DMEM baseF)
    {
      return (long)round((double)(fhz/baseF));
    }

    // convert bin number to frequency in Mel/Bark/...
    FLOAT_DMEM NtoFmel(long N, FLOAT_DMEM baseF)
    {
      return Mel( ((FLOAT_DMEM)N)*baseF );
    }

    int computeFilters( long blocksize, double frameSizeSec, int idxc );
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR

    virtual void fetchConfig();
    //virtual int myConfigureInstance();
    //virtual int myFinaliseInstance();
    //virtual int myTick(long long t);

    //virtual int configureWriter(const sDmLevelConfig *c);
    virtual int dataProcessorCustomFinalise();

    virtual void configureField(int idxi, long __N, long _nOut);
    virtual int setupNamesForField(int i, const char*name, long nEl);
    //virtual int processVectorInt(const INT_DMEM *src, INT_DMEM *dst, long Nsrc, long Ndst, int idxi);
    virtual int processVectorFloat(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi);

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cMelspec(const char *_name);

    virtual ~cMelspec();
};




#endif // __MELSPEC_HPP
