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

LPC, compute LPC coefficients from wave data (PCM) frames 

*/


#include <lpc.hpp>
#include <math.h>

#define MODULE "cLpc"



SMILECOMPONENT_STATICS(cLpc)

SMILECOMPONENT_REGCOMP(cLpc)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CLPC;
  sdescription = COMPONENT_DESCRIPTION_CLPC;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("p","order of predictor (= number of lpc coefficients)",8);
    ct->setField("saveRefCoeff","append reflection coefficients to output (1=yes/0=no)",0);
  )

  SMILECOMPONENT_MAKEINFO(cLpc);
}

SMILECOMPONENT_CREATE(cLpc)

//-----

cLpc::cLpc(const char *_name) :
  cVectorProcessor(_name),
  p(0),
  saveRefCoeff(0)
{

}

void cLpc::fetchConfig()
{
  cVectorProcessor::fetchConfig();

  p=getInt("p");
  SMILE_IDBG(2,"predictor order p = %i",p); 

  saveRefCoeff=getInt("saveRefCoeff");
  SMILE_IDBG(2,"saveRefCoeff = %i",saveRefCoeff); 
}

// setup size of output vector (= predictor order p)
int cLpc::setupNamesForField(int i, const char*name, long nEl)
{
  //char *xx; 
  int n=0;

  /*
  if ((nameAppend!=NULL)&&(strlen(nameAppend)>0)) {
    xx = myvprint("%s_Lpc%s",name,nameAppend);
  } else { xx = myvprint("%s_LOG",name); }*/
  writer->addField( "lpcCoeff", p ); n += p;
  //free(xx);

  if (saveRefCoeff) {
  /*  if ((nameAppend!=NULL)&&(strlen(nameAppend)>0)) {
      xx = myvprint("%s_RMS%s",name,nameAppend);
    } else { xx = myvprint("%s_RMS",name); }*/
    writer->addField( "reflectionCoeff", p ); n += p;
    //free(xx);
  }
  
  return n;

  // TODO: add reflection coeffs..
 // return cDataProcessor::setupNamesForField(i,name,p);
}

/*
int cLpc::myFinaliseInstance()
{
//  frameSizeFrames = reader->getLevelN();
  return cVectorProcessor::myFinaliseInstance();
}
*/

FLOAT_DMEM * cLpc::calcLpc(FLOAT_DMEM * acf, int _p, FLOAT_DMEM *lpc, FLOAT_DMEM *refl)
{
  int i,j;
  if (lpc == NULL) lpc = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM)*_p);

  FLOAT_DMEM k;
  FLOAT_DMEM error = acf[0];

  if ((acf[0] == 0.0)||(acf[0] == -0.0)) {
    for (i=0; i < _p; i++) lpc[i] = 0.0;
    return lpc;
  }

  for (i = 0; i < _p; i++) {

      /* compute this iteration's reflection coefficient k_m */
      FLOAT_DMEM rr = (acf[i+1]);
      for (j=0; j < i; j++) {
         rr += lpc[j] * acf[i-j]; 
      }
      if (error == 0.0) k=0.0;      
      else k = -rr/(error);  

      // TODO: save refl. coeff. 
      if (refl != NULL) {
        refl[i] = k;
      }

      /* update LPC coeffs. and error sum */ // ????? check this code
      lpc[i] = k;
      for (j = 0; j < i>>1; j++) {
         FLOAT_DMEM tmp  = lpc[j];
         lpc[j]     += k * lpc[i-1-j];
         lpc[i-1-j] += k * tmp;
      }
      if (i & 1) lpc[j] += lpc[j] * k;

      error -= (k*k)*(error); 
   }

   return lpc;
}


/* autoCorrelation on FLOAT_DMEM array, return FLOAT_DMEM* array with ac coeffs, caller must free the returned array
   on error, NULL is returned */
FLOAT_DMEM * cLpc::autoCorrF(const FLOAT_DMEM *x, int n, int lag)
{
  int i;
  FLOAT_DMEM *acf = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM)*lag);

  while (lag) {
    acf[--lag] = 0.0;
    for (i=lag; i < n; i++) acf[lag] += x[i] * x[i-lag];
    //lag--;
  }
  return acf;
}

int cLpc::processVectorInt(const INT_DMEM *src, INT_DMEM *dst, long Nsrc, long Ndst, int idxi)
{
  // not yet implemented
  return 0;
}

int cLpc::processVectorFloat(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  FLOAT_DMEM *acf = autoCorrF(src, Nsrc, p+1);
  // TODO: windowing of acf??

  // TODO: move lpc helper functions to smileUtil...

  if (saveRefCoeff) {
    calcLpc(acf, MIN(p,Ndst/2), dst, dst+(Ndst/2));
  } else {
    calcLpc(acf, MIN(p,Ndst), dst, NULL);
  }

  free(acf); // TODO: get rid of this free..

  return 1;
}


cLpc::~cLpc()
{
}

