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

compute mel spectrum from fft magnitude

*/


#include <melspec.hpp>

#define MODULE "cMelspec"

SMILECOMPONENT_STATICS(cMelspec)

SMILECOMPONENT_REGCOMP(cMelspec)
{
  SMILECOMPONENT_REGCOMP_INIT
  
  scname = COMPONENT_NAME_CMELSPEC;
  sdescription = COMPONENT_DESCRIPTION_CMELSPEC;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cVectorProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
//    ct->setField("nameAppend", NULL, (const);
    ct->setField("nBands","number of mel bands",26);
    ct->setField("lofreq","lower cut-off frequency of filterbank",20.0);
    ct->setField("hifreq","upper cut-off frequency of filterbank",8000.0);
    ct->setField("usePower","use power spectrum instead of magnitude",0);
    ct->setField("htkcompatible","htk compatible output (1/0 = yes/no)",1);
    ct->setField("specScale","frequency scale to use: \n   htkmel = htk compatible mel scale\n   mel = mel scale as used in Speex codec package\n   bark = Bark scale approximation as used in Speex codec\n   semi = semi tone scale with first note = 27.5Hz","htkmel");
  )

  SMILECOMPONENT_MAKEINFO(cMelspec);
}

SMILECOMPONENT_CREATE(cMelspec)

//-----

cMelspec::cMelspec(const char *_name) :
  cVectorProcessor(_name),
  filterCoeffs(NULL),  filterCfs(NULL),
  chanMap(NULL), nLoF(NULL), nHiF(NULL),
  nBands(26),
  usePower(0),
  htkcompatible(1),
  specScale(SPECSCALE_HTKMEL)
{

}

void cMelspec::fetchConfig()
{
  cVectorProcessor::fetchConfig();
  
  nBands = getInt("nBands");
  SMILE_DBG(2,"nBands = %i",nBands);
  lofreq = (FLOAT_DMEM)getDouble("lofreq");
  SMILE_DBG(2,"lofreq = %f",lofreq);
  hifreq = (FLOAT_DMEM)getDouble("hifreq");
  SMILE_DBG(2,"hifreq = %f",hifreq);

  usePower = getInt("usePower");
  if (usePower) SMILE_DBG(2,"using power spectrum");

  htkcompatible = getInt("htkcompatible");
  if (htkcompatible) SMILE_DBG(2,"HTK compatible output is enabled");

  if (!htkcompatible) {
    const char * specScaleStr = getStr("specScale");
    if (!strcasecmp(specScaleStr,"mel")) {
      specScale = SPECSCALE_MEL;
    } else if (!strcasecmp(specScaleStr,"htkmel")) {
      specScale = SPECSCALE_HTKMEL;
    } else if (!strcasecmp(specScaleStr,"bark")) {
      specScale = SPECSCALE_BARK;
    } else if (!strcasecmp(specScaleStr,"semi")) {
      specScale = SPECSCALE_SEMITONE;
    } else {
      SMILE_IERR(1,"unknown frequency scale '%s' (see -H for possible values), assuming 'htkmel'!",specScaleStr);
      specScale = SPECSCALE_HTKMEL;
    }
    SMILE_IDBG(2,"specScale = '%s'",specScaleStr);
  }

}

int cMelspec::dataProcessorCustomFinalise()
{
  // allocate for multiple configurations..
  if (filterCoeffs == NULL) filterCoeffs = (FLOAT_DMEM**)multiConfAlloc();
  if (filterCfs == NULL) filterCfs = (FLOAT_DMEM**)multiConfAlloc();
  if (chanMap == NULL) chanMap = (long**)multiConfAlloc();
  if (nLoF == NULL) nLoF = (long *)multiConfAlloc();
  if (nHiF == NULL) nHiF = (long *)multiConfAlloc();

  return cVectorProcessor::dataProcessorCustomFinalise();
}
/*
int cMelspec::myConfigureInstance()
{
  int ret=1;
  ret *= cVectorProcessor::myConfigureInstance();
  if (ret == 0) return 0;
  // allocate for multiple configurations..
  filterCoeffs = (FLOAT_DMEM**)multiConfAlloc();
  filterCfs = (FLOAT_DMEM**)multiConfAlloc();
  chanMap = (long**)multiConfAlloc();
  nLoF = (long *)multiConfAlloc();
  nHiF = (long *)multiConfAlloc();
//  sintable = (FLOAT_DMEM**)calloc(1,sizeof(FLOAT_DMEM*)*getNf());
//  costable = (FLOAT_DMEM**)calloc(1,sizeof(FLOAT_DMEM*)*getNf());

  return ret;
}
*/

/*
int cMelspec::configureWriter(const sDmLevelConfig *c)
{
  if (c==NULL) return 0;
  
  // you must return 1, in order to indicate configure success (0 indicated failure)
  return 1;
}
*/

void cMelspec::configureField(int idxi, long __N, long nOut)
{
  // compute filters:   // TODO:: compute filters for each FIELD (however, only if fields are of different blocksize!)
  const sDmLevelConfig *c = reader->getLevelConfig();
  computeFilters(__N, c->frameSizeSec, getFconf(idxi));
}

int cMelspec::setupNamesForField(int i, const char*name, long nEl)
{
  return cVectorProcessor::setupNamesForField(i,name,nBands);
/*
  char *tmp = myvprint("%s_mspec",name);
  writer->addField( tmp, nBands );
  free(tmp);
  return nBands;
  */
}

// MEL FUNCTIONS ----------------------------------------

// blocksize is size of fft block, _T is period of fft frames
// sampling period is reconstructed by: _T/((blocksize-1)*2)
int cMelspec::computeFilters( long blocksize, double frameSizeSec, int idxc )
{
  FLOAT_DMEM *_filterCoeffs = filterCoeffs[idxc];
  FLOAT_DMEM *_filterCfs = filterCfs[idxc];
  long * _chanMap = chanMap[idxc];

  if (blocksize < nBands) {
    SMILE_ERR(1,"nBands (%i) is greater than dimension of the input vector (%i)! This does not work... not computing mfcc filters!",nBands,blocksize);
    return 0;
  }
  if (_filterCoeffs != NULL) { free(_filterCoeffs);   }
  if (_chanMap != NULL) { free(_chanMap);   }
  if (_filterCfs != NULL) { free(_filterCfs);   }

  bs = blocksize;
  _filterCoeffs = (FLOAT_DMEM*)calloc(1,sizeof(FLOAT_DMEM) * blocksize);
  _chanMap = (long*)malloc(sizeof(long) * blocksize);
  _filterCfs = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * (nBands+2));

  FLOAT_DMEM _N = (FLOAT_DMEM) ((blocksize-1)*2);
  FLOAT_DMEM F0 = (FLOAT_DMEM)(1.0/frameSizeSec);
  FLOAT_DMEM Fs = (FLOAT_DMEM)(_N/frameSizeSec);
  FLOAT_DMEM M = (FLOAT_DMEM)nBands;
  
  if ((lofreq < 0.0)||(lofreq>Fs/2.0)||(lofreq>hifreq)) lofreq = 0.0;
  if ((hifreq<lofreq)||(hifreq>Fs/2.0)||(hifreq<=0.0)) hifreq = Fs/(FLOAT_DMEM)2.0; // Hertz(NtoFmel(blocksize+1,F0));
  FLOAT_DMEM LoF = Mel(lofreq);  // Lo Cutoff Freq (mel)
  FLOAT_DMEM HiF = Mel(hifreq);  // Hi Cutoff Freq (mel)
  nLoF[idxc] = FtoN(lofreq,F0);  // Lo Cutoff Freq (fft bin)
  nHiF[idxc] = FtoN(hifreq,F0);  // Hi Cutoff Freq (fft bin)

  if (nLoF[idxc] > blocksize) nLoF[idxc] = blocksize;
  if (nHiF[idxc] > blocksize) nHiF[idxc] = blocksize;
  if (nLoF[idxc] < 0) nLoF[idxc] = 0; // always exclude DC component
  if (nHiF[idxc] < 0) nHiF[idxc] = 0;

  int m,n;
  // compute mel center frequencies
  FLOAT_DMEM mBandw = (HiF-LoF)/(M+(FLOAT_DMEM)1.0); // half bandwidth of mel bands
  for (m=0; m<=nBands+1; m++) {
    _filterCfs[m] = LoF+(FLOAT_DMEM)m*mBandw;
  }

  // compute channel mapping table:
  m = 0;
  for (n=0; n<blocksize; n++) {
    if ( (n<=nLoF[idxc])||(n>=nHiF[idxc]) ) _chanMap[n] = -3;
    else {
         //printf("II: Cfs[%i]=%f n=%i F0=%f NtoFmel(n,F0)=%f\n",m,_filterCfs[m],n,F0,NtoFmel(n,F0));
      while (_filterCfs[m] < NtoFmel(n,F0)) {
            if (m>nBands) break;
            m++;
         //printf("Cfs[%i]=%f n=%i F0=%f\n",m,_filterCfs[m],n,F0);
            }
      _chanMap[n] = m-2;
    }
  }

  // compute filter weights (falling slope only):
  m = 0;
  FLOAT_DMEM nM;
  for (n=nLoF[idxc];n<nHiF[idxc];n++) {
    nM = NtoFmel(n,F0);
    while ((nM > _filterCfs[m+1]) && (m<=nBands)) m++;
    _filterCoeffs[n] = ( _filterCfs[m+1] - nM )/(_filterCfs[m+1] - _filterCfs[m]);
  }

  filterCoeffs[idxc] = _filterCoeffs;
  filterCfs[idxc] = _filterCfs;
  chanMap[idxc] = _chanMap;
  return 0;
}


/*
int cMelspec::myFinaliseInstance()
{
  int ret;
  ret = cVectorProcessor::myFinaliseInstance();
  return ret;
}
*/

// a derived class should override this method, in order to implement the actual processing
/*
int cMelspec::processVectorInt(const INT_DMEM *src, INT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...
  
  return 1;
}
*/

// a derived class should override this method, in order to implement the actual processing
int cMelspec::processVectorFloat(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  int m,n;
  idxi=getFconf(idxi);
  FLOAT_DMEM *_filterCoeffs = filterCoeffs[idxi];
  FLOAT_DMEM *_filterCfs = filterCfs[idxi];
  long *_chanMap = chanMap[idxi];


  // copy & square the fft magnitude
  FLOAT_DMEM *_src;
  if (usePower) {
    _src = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*Nsrc);
    if (src==NULL) OUT_OF_MEMORY;
    for (n=0; n<Nsrc; n++) {
      _src[n] = src[n]*src[n];
    }
    src = _src;
  }

  // do the mel filtering by multiplying with the filters and summing up
  memset(dst, 0, Ndst*sizeof(FLOAT_DMEM));

  FLOAT_DMEM a;
  for (n=nLoF[idxi]; n<nHiF[idxi]; n++) {
    m = _chanMap[n];
    a = src[n] * _filterCoeffs[n];
    if (m>-2) {
      if (m>-1) dst[m] += a;
      if (m < nBands-1)
        dst[m+1] += src[n] - a;
    }
  }

  if ((usePower)&&(_src!=NULL)) free((void *)_src);

  if (htkcompatible) {
    for (m=0; m<nBands; m++) {
      if (usePower) {
        dst[m] *= (FLOAT_DMEM)(32767.0*32767.0);
      } else {
        dst[m] *= (FLOAT_DMEM)32767.0;
      }
      // the above is for HTK compatibility....
	  // HTK does not scale the input sample values to -1 / +1
      // thus, we must multiply by the max 16bit sample value again.
    }
  }

  return 1;
}

cMelspec::~cMelspec()
{
  multiConfFree(filterCoeffs);
  multiConfFree(chanMap);
  multiConfFree(filterCfs);
  if (nLoF != NULL) free(nLoF);
  if (nHiF != NULL) free(nHiF);
}

