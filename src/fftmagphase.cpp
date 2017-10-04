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

compute magnitude and phase from complex valued fft output
TODO: support inverse??

*/


#include <fftmagphase.hpp>
#include <math.h>

#define MODULE "cFFTmagphase"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataProcessor::registerComponent(_confman);
}
*/

SMILECOMPONENT_STATICS(cFFTmagphase)

SMILECOMPONENT_REGCOMP(cFFTmagphase)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFFTMAGPHASE;
  sdescription = COMPONENT_DESCRIPTION_CFFTMAGPHASE;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    //ct->setField("inverse","1 = perform inverse FFT",0);
    ct->setField("magnitude","1/0 = compute magnitude yes/no",1);
    ct->setField("phase","1/0 = compute phase yes/no",0);
  )

  SMILECOMPONENT_MAKEINFO(cFFTmagphase);
}

SMILECOMPONENT_CREATE(cFFTmagphase)

//-----


cFFTmagphase::cFFTmagphase(const char *_name) :
  cVectorProcessor(_name),
  magnitude(1),
  phase(0)
{

}

void cFFTmagphase::fetchConfig()
{
  cVectorProcessor::fetchConfig();
  
  magnitude = getInt("magnitude");
  phase = getInt("phase");
  if ((!magnitude)&&(!phase)) { magnitude = 1; }
  if (magnitude) SMILE_DBG(2,"magnitude computation enabled");
  if (phase) SMILE_DBG(2,"phase computation enabled");
}


int cFFTmagphase::setupNamesForField(int i, const char*name, long nEl)
{
  long newNEl = 0;
  char *xx;
  if (magnitude) {
    addNameAppendFieldAuto(name, "Mag", nEl/2 + 1);
    newNEl += nEl/2 +1; 
    /*
    if ((nameAppend!=NULL)&&(strlen(nameAppend)>0))
      xx = myvprint("%s_%sMag",name,nameAppend);
    else
      xx = strdup(name);
    writer->addField( xx, nEl/2 +1 ); 
    newNEl += nEl/2 +1; 
    free(xx);
    */
  }
  if (phase) {
    addNameAppendFieldAuto(name, "Phase", nEl/2 + 1);
    newNEl += nEl/2 +1; 
    /*
    if ((nameAppend!=NULL)&&(strlen(nameAppend)>0))
      xx = myvprint("%s_%sPhase",name,nameAppend);
    else
      xx = myvprint("%s_phase",name);
    writer->addField( xx, nEl/2 ); // +1
    newNEl += nEl/2; //+1
    free(xx);
    */
  }
  return newNEl;
}



// a derived class should override this method, in order to implement the actual processing
int cFFTmagphase::processVectorFloat(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  long n;
  
  if (magnitude) {
    dst[0] = fabs(src[0]);
    for(n=2; n<Nsrc; n += 2) {
      dst[n/2] = sqrt(src[n]*src[n] + src[n+1]*src[n+1]);
    }
    dst[Nsrc/2] = fabs(src[1]);
    dst += Nsrc/2+1;
//    dst += Nsrc/2;
  }
  if (phase) {
    dst[0] = 0.0;
    for(n=2; n<Nsrc-1; n++) {
      dst[n/2] = atan(src[n+1]/src[n]);
    }
//    dst[Nsrc/2] = 0.0;
    //dst += Nsrc/2+1;
  }

  return 1;
}

cFFTmagphase::~cFFTmagphase()
{
}

