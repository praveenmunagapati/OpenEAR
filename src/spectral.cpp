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

compute spectral features such as flux, roll-off, centroid, etc.

*/


#include <spectral.hpp>
#include <math.h>

#define MODULE "cSpectral"


SMILECOMPONENT_STATICS(cSpectral)

SMILECOMPONENT_REGCOMP(cSpectral)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CSPECTRAL;
  sdescription = COMPONENT_DESCRIPTION_CSPECTRAL;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("squareInput","1/0 = square input values (e.g. if input is magnitude and not power spectrum)",1);
    ct->setField("bands","bands[n] = Lof/Hz-Hif/Hz  (e.g. 0-250), compute energy in this spectral band","250-650",ARRAY_TYPE);
    ct->setField("rollOff","rollOff[n] = X  (X in range 0..1), compute X*100 percent spectral roll-off",0.90,ARRAY_TYPE);
    ct->setField("flux","(1/0=yes/no) enable computation of spectral flux",1);
    ct->setField("centroid","(1/0=yes/no) enable computation of spectral centroid",1);
    ct->setField("maxPos","(1/0=yes/no) enable computation of position of spectral maximum",1);
    ct->setField("minPos","(1/0=yes/no) enable computation of position of spectral minimum",1);

  )

  SMILECOMPONENT_MAKEINFO(cSpectral);
}

SMILECOMPONENT_CREATE(cSpectral)

//-----


cSpectral::cSpectral(const char *_name) :
  cVectorProcessor(_name),
  squareInput(1), nBands(0), nRollOff(0),
  bandsL(NULL), bandsH(NULL), rollOff(NULL),
  fsSec(-1.0)
{

}

void cSpectral::fetchConfig()
{
  cVectorProcessor::fetchConfig();
  
  squareInput = getInt("squareInput");
  if (squareInput) SMILE_DBG(2,"squaring of input values enabled");

  flux = getInt("flux");
  if (flux) SMILE_DBG(2,"spectral flux computation enabled");

  centroid = getInt("centroid");
  if (centroid) SMILE_DBG(2,"spectral centroid computation enabled");

  maxPos = getInt("maxPos");
  if (maxPos) SMILE_DBG(2,"spectral max. pos. computation enabled");

  minPos = getInt("minPos");
  if (minPos) SMILE_DBG(2,"spectral min. pos. computation enabled");

  int i;
  
  nBands = getArraySize("bands");
  if (nBands > 0) {
    bandsL = (int *)calloc(1,sizeof(int)*nBands);
    bandsH = (int *)calloc(1,sizeof(int)*nBands);
    for (i=0; i<nBands; i++) {
        char *tmp = strdup(getStr_f(myvprint("bands[%i]",i)));
        char *orig = strdup(tmp);
        int l = (int)strlen(tmp);
        int err=0;
        // remove spaces and tabs at beginning and end
//        while ( (l>0)&&((*tmp==' ')||(*tmp=='\t')) ) { *(tmp++)=0; l--; }
//        while ( (l>0)&&((tmp[l-1]==' ')||(tmp[l-1]=='\t')) ) { tmp[l-1]=0; l--; }
        // find '-'
        char *s2 = strchr(tmp,'-');
        if (s2 != NULL) {
          *(s2++) = 0;
          char *ep=NULL;
          int r1 = strtol(tmp,&ep,10);
          if ((r1==0)&&(ep==tmp)) { err=1; }
          else if (r1 < 0) {
                 SMILE_ERR(1,"(inst '%s') low frequency of band %i in '%s'  is out of range (allowed: [0..+inf])",getInstName(),i,orig);
               }
          ep=NULL;
          int r2 = strtol(s2,&ep,10);
          if ((r2==0)&&(ep==tmp)) { err=1; }
          else {
               if (r2 < 0) {
                 SMILE_ERR(1,"(inst '%s') high frequency of band %i in '%s'  is out of range (allowed: [0..+inf])",getInstName(),i,orig);
               }
             }
          if (!err) {
            if (r1 < r2) {
              bandsL[i] = r1;
              bandsH[i] = r2;
            } else {
              bandsL[i] = r2;
              bandsH[i] = r1;
            }
          }
        } else { err=1; }

        if (err==1) {
          SMILE_ERR(1,"(inst '%s') Error parsing bands[%i] = '%s'! (The band range must be X-Y, where X and Y are positive integer numbers specifiying frequency in Hertz!)",getInstName(),i,orig);
          bandsL[i] = -1;
          bandsH[i] = -1;
        }
        free(orig);
        free(tmp);
    }
  }
  
  nRollOff = getArraySize("rollOff");
  if (nRollOff > 0) {
    rollOff = (double*)calloc(1,sizeof(double)*nRollOff);
    for (i=0; i<nRollOff; i++) {
      rollOff[i] = getDouble_f(myvprint("rollOff[%i]",i));
      if (rollOff[i] < 0.0) {
        SMILE_ERR(1,"rollOff[%i] = %f is out of range (allowed 0..1), clipping to 0.0",i,rollOff[i]);
        rollOff[i] = 0.0;
      }
      else if (rollOff[i] > 1.0) {
        SMILE_ERR(1,"rollOff[%i] = %f is out of range (allowed 0..1), clipping to 1.0",i,rollOff[i]);
        rollOff[i] = 1.0;
      }
    }
  }

}


int cSpectral::setupNamesForField(int i, const char*name, long nEl)
{
  int newNEl = 0;
  int ii;

  if (fsSec == -1.0) {
    const sDmLevelConfig *c = reader->getLevelConfig();
    fsSec = c->frameSizeSec;
  }

  for (ii=0; ii<nBands; ii++) {
    if (isBandValid(bandsL[ii],bandsH[ii])) {
      char *xx = myvprint("%s_fband%i-%i",name,bandsL[ii],bandsH[ii]);
      writer->addField( xx, 1 ); newNEl++; free(xx);
    }
  }

  for (ii=0; ii<nRollOff; ii++) {
    char *xx = myvprint("%s_spectralRollOff%.1f",name,rollOff[ii]*100.0);
    writer->addField( xx, 1 ); newNEl++; free(xx);
  }

  if (flux) {
    char *xx = myvprint("%s_spectralFlux",name);
    writer->addField( xx, 1 ); newNEl++; free(xx);
  }
  if (centroid) {
    char *xx = myvprint("%s_spectralCentroid",name);
    writer->addField( xx, 1 ); newNEl++; free(xx);
  }
  if (maxPos) {
    char *xx = myvprint("%s_spectralMaxPos",name);
    writer->addField( xx, 1 ); newNEl++; free(xx);
  }
  if (minPos) {
    char *xx = myvprint("%s_spectralMinPos",name);
    writer->addField( xx, 1 ); newNEl++; free(xx);
  }


  return newNEl;
}


// a derived class should override this method, in order to implement the actual processing
int cSpectral::processVectorFloat(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  long n=0;
  long i,j;

  double _N = (double) ((Nsrc-1)*2);  // assumes FFT magnitude input array
  double F0 = 1.0/fsSec;

  FLOAT_DMEM *_src;
  if (squareInput) {
    _src = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*Nsrc);
    for (i=0; i<Nsrc; i++) _src[i] = src[i]*src[i];
  } else {
    _src = (FLOAT_DMEM *)src;  // typecast ok here.. since we treat _src as read-only below...
  }
  
  if (Nsrc<=0) return 0;
  
  // process spectral bands:
  for (i=0; i<nBands; i++) {
    if (isBandValid(bandsL[i],bandsH[i])) {
      double idxL =  (double)bandsL[i] / F0 ;
      double wghtL = ceil(idxL) - idxL;
      if (wghtL == 0.0) wghtL = 1.0;

      double idxR =  (double)bandsH[i] / F0 ;
      double wghtR = idxR-floor(idxR);

      long iL = (long)floor(idxL);
      long iR = (long)floor(idxR);

      if (iL >= Nsrc) iR=Nsrc-1;
      if (iR >= Nsrc) iR=Nsrc-1;
      if (iL < 0) iL=0;
      if (iR < 0) iR=0;

      double sum=(double)_src[iL]*wghtL;
      for (j=iL+1; j<iR; j++) sum += (double)_src[j];
      sum += (double)_src[iR]*wghtR;
      
      dst[n++] = (FLOAT_DMEM)( sum/(double)Nsrc );  // normalise band energy to frame size
    }
  }


  double sumA=0.0, sumB=0.0, sumC=0.0, f=0.0;
  for (j=0; j<Nsrc; j++) {
    sumB += (double)_src[j];
  }
  
  // compute rollOff(s):
  double *ro = (double *)calloc(1,sizeof(double)*nRollOff);
  for (j=0; j<Nsrc; j++) {
    for (i=0; i<nRollOff; i++) {
      sumC += (double)_src[j];
      if ((ro[i] == 0.0)&&(sumC >= rollOff[i]*sumB))
        ro[i] = (double)j * F0;   // TODO: norm frequency ??
    }
  }
  for (i=0; i<nRollOff; i++) {
    dst[n++] = (FLOAT_DMEM)ro[i];
  }
  free(ro);
  
  // flux
  if (flux) {
    // flux
    cVector *vecPrev = reader->getFrameRel(2,1,1);
    if (vecPrev != NULL) {
      FLOAT_DMEM normP=1.0, normC=1.0;
/*      if (lldex->current[level]->energy != NULL) {
        normC = lldex->current[level]->energy->rmsEnergy;
        if (normC <= 0.0) normC = 1.0;
      }*/
      FLOAT_DMEM *magP = vecPrev->dataF;
/*      if (prev->energy != NULL) {
        normP = prev->energy->rmsEnergy;
        if (normP <= 0.0) normP = 1.0;
      }*/
      double myA = 0.0; double myB;
      if (squareInput) {
        for (j=1; j<Nsrc; j++) {
          myB = ((double)src[j]/normC - (double)magP[j]/normP);
          myA += myB*myB;
        }
      } else {
        for (j=1; j<Nsrc; j++) {
          myB = (sqrt(fabs((double)src[j]))/normC - sqrt(fabs((double)magP[j]))/normP);
          myA += myB*myB;
        }
      }
      myA /= (double)(Nsrc-1);
      dst[n++] = (FLOAT_DMEM)sqrt(myA);
      
      delete vecPrev;
    } else {
      dst[n++] = 0.0;
    }
  }
  
  // centroid
  
//  sumB=0.0;
  if (centroid) {
    for (j=0; j<Nsrc; j++) {
      sumA += f * (double)_src[j];
  //    sumB += (double)_src[j];
      f += F0;
    }
	if (sumB == 0.0) {
      dst[n++] = 0.0;
	} else {
      dst[n++] = (FLOAT_DMEM)(sumA/sumB);  // spectral centroid in Hz (unnormalised)  // TODO: normFrequency option
	}
  }
  
  if ((maxPos)||(minPos)) {
    long maP=1, miP=1;
    FLOAT_DMEM max=_src[1], min=_src[1];
    for (j=2; j<Nsrc; j++) {
      if (_src[j] < min) { min = _src[j]; miP = j; }
      if (_src[j] > max) { max = _src[j]; maP = j; }
    }

    if (maxPos) dst[n++] = (FLOAT_DMEM)(maP*F0);  // spectral minimum in Hz
    if (minPos) dst[n++] = (FLOAT_DMEM)(maP*F0); // spectral maximum in Hz
  }

  if (squareInput) { free(_src); }
  
  return 1;
}

cSpectral::~cSpectral()
{
  if (bandsL!=NULL) free(bandsL);
  if (bandsH!=NULL) free(bandsH);
  if (rollOff!=NULL) free(rollOff);
}

