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

Compute chroma values from semi-tone spectrum

*/


#include <chroma.hpp>

#define MODULE "cChroma"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataProcessor::registerComponent(_confman);
}
*/

SMILECOMPONENT_STATICS(cChroma)

SMILECOMPONENT_REGCOMP(cChroma)
//sComponentInfo * cChroma::registerComponent(cConfigManager *_confman)
{
  if (_confman == NULL) return NULL;
  int rA = 0;

  sconfman = _confman;
  scname = COMPONENT_NAME_CCHROMA;
  sdescription = COMPONENT_DESCRIPTION_CCHROMA;

  // we inherit cVectorProcessor configType and extend it:
  ConfigType *ct = new ConfigType( *(sconfman->getTypeObj("cVectorProcessor")) , scname );
  if (ct == NULL) {
    SMILE_WRN(4,"cVectorProcessor config Type not found!");
    rA=1;
  }
  if (rA==0) {
    ct->setField("nameAppend",NULL,"chroma");
    ct->setField("copyInputName",NULL,0);
    //ct->setField("inverse","1 = perform inverse FFT",0);
    ct->setField("octaveSize","size of warping",12);
    ConfigInstance *Tdflt = new ConfigInstance( scname, ct, 1 );
    sconfman->registerType(Tdflt);
  } 

  SMILECOMPONENT_MAKEINFO(cChroma);
}

SMILECOMPONENT_CREATE(cChroma)

//-----

cChroma::cChroma(const char *_name) :
  cVectorProcessor(_name)
{

}

void cChroma::fetchConfig()
{
  cVectorProcessor::fetchConfig();
  
  octaveSize = getInt("octaveSize");
  SMILE_DBG(2,"octaveSize = %i",octaveSize);

/*
  inverse = getInt("inverse");
  if (inverse) {
    SMILE_DBG(2,"transformFFT set for inverse FFT",inverse);
    inverse = 1;  // sign of exponent
  } else {
    inverse = -1; // sign of exponent
  }
*/

}

/*
int cChroma::myConfigureInstance()
{
  int ret=1;
  ret *= cVectorProcessor::myConfigureInstance();
  if (ret == 0) return 0;

//...


  return ret;
}
*/

/*
int cChroma::configureWriter(const sDmLevelConfig *c)
{
  if (c==NULL) return 0;

  // you must return 1, in order to indicate configure success (0 indicated failure)
  return 1;
}

*/

// optional

int cChroma::setupNamesForField(int i, const char*name, long nEl)
{
// reader->getLevelN ...  Nfi... etc. in vectorProcessor
  return cVectorProcessor::setupNamesForField(i,name,octaveSize);
}

/*
// use this to allocate data like filter coefficient arrays, etc...
void cDataProcessor::configureField(int idxi, long __N, long nOut)
{
//     const sDmLevelConfig *c = reader->getLevelConfig();
// idxc = getFconf(idxi);
}
*/


/*
int cChroma::myFinaliseInstance()
{
  int ret=1;
  ret *= cVectorProcessor::myFinaliseInstance();
//.....
  return ret;
}
*/

/*
// a derived class should override this method, in order to implement the actual processing
int cChroma::processVectorInt(const INT_DMEM *src, INT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...
  
  return 1;
}
*/

// a derived class should override this method, in order to implement the actual processing
int cChroma::processVectorFloat(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...

  int i,j,k;
  double sum = 0.0;
  
  nOctaves = Nsrc / octaveSize;
  if (Nsrc % octaveSize == 0)
  {
    for (i = 0;i < octaveSize; i++)
    {
      dst[i] = 0.0;
      for (j = 0;j < nOctaves; j++)
      {
        dst[i] += src[j * octaveSize + i];
      }
      sum += dst[i]*dst[i];
    }
    if (sum != 0.0) {
      double power = sqrt(sum);
      for (i = 0;i < octaveSize; i++)
        dst[i] = dst[i] / power;
    }
  }

  return 1;
}

cChroma::~cChroma()
{
}

