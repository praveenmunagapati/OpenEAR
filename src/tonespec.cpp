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

computes (or rather estimates) semi-tone spectrum from fft spectrum

*/


/*
TODO: 
 *remove normalisation with 1/binsize (or provide option...)
 *no sqrt option by usePower
 *max and rect filter type

 (*tuning correction)
*/

#include <tonespec.hpp>
#include <dbA.hpp>

#define MODULE "cTonespec"

SMILECOMPONENT_STATICS(cTonespec)

SMILECOMPONENT_REGCOMP(cTonespec)
{
  if (_confman == NULL) return NULL;
  int rA = 0;

  sconfman = _confman;
  scname = COMPONENT_NAME_CTONESPEC;
  sdescription = COMPONENT_DESCRIPTION_CTONESPEC;

  // we inherit cVectorProcessor configType and extend it:
  ConfigType *ct = new ConfigType( *(sconfman->getTypeObj("cVectorProcessor")) , scname );
  if (ct == NULL) {
    SMILE_WRN(4,"cVectorProcessor config Type not found!");
    rA=1;
  }
  if (rA==0) {
    ct->setField("nameAppend", NULL, "note");
    ct->setField("nOctaves","number of octaves the spectrum should span",6);
    ct->setField("firstNote","frequency of first note (in Hz)",55.0);
    ct->setField("filterType","note filter type:\n   tri (triangular)\n   trp (triangular-powered)\n   gau (gaussian)","gau");
    ct->setField("usePower","use power spectrum instead of magnitude (=square input values)",0);
    ct->setField("dbA","apply built-in dbA to (power) spectrum (1/0 = yes/no)",1);
  #ifdef DEBUG
    ct->setField("printBinMap","print mapping of fft bins to semi-tone intervals",0);
  #endif
    ConfigInstance *Tdflt = new ConfigInstance( scname, ct, 1 );
    sconfman->registerType(Tdflt);
  } 

  SMILECOMPONENT_MAKEINFO(cTonespec);
}

SMILECOMPONENT_CREATE(cTonespec)

//-----

cTonespec::cTonespec(const char *_name) :
  cVectorProcessor(_name),
  pitchClassFreq(NULL),
  binKey(NULL),
  distance2key(NULL),
  pitchClassNbins(NULL),
  filterMap(NULL),
  flBin(NULL),
  db(NULL),
  nOctaves(1),
  nNotes(8),
  usePower(0),
  printBinMap(0),
  filterType(WINF_GAUSS),
  dbA(0)
{

}

void cTonespec::fetchConfig()
{
  cVectorProcessor::fetchConfig();
  
  nOctaves = getInt("nOctaves");
  SMILE_DBG(2,"nOctaves = %i",nOctaves);
  nNotes = nOctaves * 12;
  firstNote = (FLOAT_DMEM)getDouble("firstNote");
  SMILE_DBG(2,"firstNote = %f",firstNote);

  lastNote = firstNote * (FLOAT_DMEM)pow(2.0, (double)nNotes/12.0);

  usePower = getInt("usePower");
  if (usePower) SMILE_DBG(2,"using power spectrum");
  
  dbA = getInt("dbA");
  if (dbA) SMILE_DBG(2,"dbA weighting for tonespec enabled");

  const char *f = getStr("filterType");
  if ( (!strcmp(f,"gau"))||(!strcmp(f,"Gau"))||(!strcmp(f,"gauss"))||(!strcmp(f,"Gauss"))||(!strcmp(f,"gaussian"))||(!strcmp(f,"Gaussian")) ) filterType = WINF_GAUSS;
  else if ( (!strcmp(f,"tri"))||(!strcmp(f,"Tri"))||(!strcmp(f,"triangular"))||(!strcmp(f,"Triangular")) ) filterType = WINF_TRIANGULAR;
  else if ( (!strcmp(f,"trp"))||(!strcmp(f,"TrP"))||(!strcmp(f,"Trp"))||(!strcmp(f,"triangular-powered"))||(!strcmp(f,"Triangular-Powered")) ) filterType = WINF_TRIANGULAR_POWERED;
  else if ( (!strcmp(f,"rec"))||(!strcmp(f,"Rec"))||(!strcmp(f,"rectangular"))||(!strcmp(f,"Rectangular")) ) filterType = WINF_RECTANGULAR;
  
  #ifdef DEBUG
  printBinMap = getInt("printBinMap");
  #endif
}


int cTonespec::dataProcessorCustomFinalise()
{
  if (namesAreSet) return 1;
  //Nfi = reader->getLevelNf();

  // allocate for multiple configurations..
  if (pitchClassFreq == NULL) pitchClassFreq = (FLOAT_DMEM**)multiConfAlloc();
  if (distance2key == NULL) distance2key = (FLOAT_DMEM**)multiConfAlloc();
  if (filterMap == NULL) filterMap = (FLOAT_DMEM**)multiConfAlloc();
  if (binKey == NULL) binKey = (int**)multiConfAlloc();
  if (pitchClassNbins == NULL) pitchClassNbins = (int**)multiConfAlloc();
  if (flBin == NULL) flBin = (int**)multiConfAlloc();
  if ((dbA)&&(db==NULL)) db = (FLOAT_DMEM**)multiConfAlloc();

  return cVectorProcessor::dataProcessorCustomFinalise();
}

/*
int cTonespec::configureWriter(const sDmLevelConfig *c)
{
  if (c==NULL) return 0;
  
  // you must return 1, in order to indicate configure success (0 indicated failure)
  return 1;
}
*/

void cTonespec::setPitchclassFreq(int idxc)
{
  FLOAT_DMEM *_pitchClassFreq = pitchClassFreq[idxc];

  double n = 0.0;
  int i;

  if (_pitchClassFreq != NULL) free(_pitchClassFreq);
  _pitchClassFreq = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM) * nNotes);

  _pitchClassFreq[0] = firstNote;
  // standard pitch frequencies
  for (i = 1; i < nNotes; i++) {
    n += 1.0;
    _pitchClassFreq[i] = firstNote * (FLOAT_DMEM)pow (2.0,n/12.0);
  }
  
  pitchClassFreq[idxc] = _pitchClassFreq;
}



void cTonespec::computeFilters(long blocksize, double frameSizeSec, int idxc)
{
  FLOAT_DMEM *_distance2key = distance2key[idxc];
  FLOAT_DMEM *_filterMap = filterMap[idxc];
  FLOAT_DMEM *_pitchClassFreq = pitchClassFreq[idxc];

  int * _binKey = binKey[idxc];
  int * _pitchClassNbins = pitchClassNbins[idxc];

  if (_distance2key != NULL) free(_distance2key);
  _distance2key = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM) * blocksize);
  if (_binKey != NULL) free(_binKey);
  _binKey = (int *)malloc(sizeof(int) * blocksize);
  if (_pitchClassNbins != NULL) free(_pitchClassNbins);
  _pitchClassNbins = (int *)calloc(1,sizeof(int) * nNotes);
  if (_filterMap != NULL) free(_filterMap);
  _filterMap = (FLOAT_DMEM *)malloc(sizeof(FLOAT_DMEM) * blocksize);

  if (flBin[idxc] != NULL) free(flBin[idxc]);
  flBin[idxc] = (int*)calloc(1,sizeof(FLOAT_DMEM)*2);
  int firstBin = *(flBin[idxc]);
  int lastBin = *(flBin[idxc]+1);


  FLOAT_DMEM distance;
  int i,b;

  FLOAT_DMEM F0 = (FLOAT_DMEM)(1.0/frameSizeSec);
  if (dbA) {
    if (db[idxc] != NULL) free(db[idxc]);
    db[idxc] = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*blocksize);
    computeDBA( db[idxc], blocksize, F0 );
  }

  firstBin = (int)round(firstNote / F0);
  lastBin = (int)round(lastNote / F0);
  if (firstBin < 1) firstBin = 1;
  if (lastBin >= blocksize) lastBin = blocksize-1;
  SMILE_DBG(2,"\tOne bin represents: %f Hz\n\t\t\t\tUsing FFT bin %i to %i\n\t\t\t\tFor freq %f Hz to %f Hz",F0,firstBin,lastBin,firstNote,lastNote);

  for (i = 0 ; i < blocksize; i++)  // checks the frequency of every FFT bin and maps it to a key frequency
  {
    _distance2key[i] = 999999.0;
    _binKey[i] = -1;
    for (b=0; b < nNotes; b++) // foreach bin
    {
        //SMILE_PRINT("   note %i (f=%f)",b,_pitchClassFreq[b]);
      distance = _pitchClassFreq[b] - ((FLOAT_DMEM)i * F0);   //*get the distance
      if (distance < 0.0) distance = (-1.0) * distance;
      if (distance < _distance2key[i])
      {
        _binKey[i] = b;   //maps bin to a note (b)
        _distance2key[i] = distance; 
      }
    }
  }
  #ifdef DEBUG
  if (printBinMap) {
    for(i=firstBin; i<lastBin;i++) {
      SMILE_PRINT("   bin %i (f=%f) -> note %i (f=%f) ",i,(FLOAT_DMEM)i*F0,_binKey[i], _pitchClassFreq[_binKey[i]]);
    }
  }
  #endif

  //Checks how many FFT bins belong to a certain pitchclass
  //used to get the mean power
  for (i=firstBin; i <= lastBin; i++) {
    if (_binKey[i] >= 0) {
      _pitchClassNbins[_binKey[i]]++;
    }
  }
  #ifdef DEBUG
  if (printBinMap) {
    for(i=0; i<nNotes; i++) {
      SMILE_PRINT("   _pitchClassNbins[%i]=%i ",i, _pitchClassNbins[i]);
    }
  }
  #endif

  //set up the filter map
  for (i=0; i < blocksize; i++) _filterMap[i] = 1.0;


  float start_bin, end_bin, middle_bin;
  int i_start_bin, i_end_bin, i_middle_bin;
  float start_freq, end_freq;

  if (filterType != WINF_RECTANGULAR) {
    for (b = 1; b < nNotes - 1; b++) {
      start_freq = (_pitchClassFreq[b - 1] + _pitchClassFreq[b]) / 2.0;
      end_freq = (_pitchClassFreq[b] + _pitchClassFreq[b+1]) / 2.0;

      start_bin  = start_freq / F0;
      end_bin  = end_freq / F0;
	  
      if ((int)(floor(end_bin) - floor(start_bin)) != _pitchClassNbins[b]) { SMILE_WRN(1,"pitchClassN mismatch %i <> %i (note %i)", (int)round(end_bin - start_bin), _pitchClassNbins[b], b); }
      middle_bin = (_pitchClassFreq[b]/F0);

      i_start_bin  = (int)(floor(start_bin));  //??????
      i_end_bin  = (int)(floor(end_bin));             //??????
      i_middle_bin = (int)(_pitchClassFreq[b]/F0) + 1;  //??????

  	  if (i_end_bin >= blocksize) i_end_bin = blocksize-1;
      if (i_start_bin >= blocksize) i_start_bin = blocksize-1;

      if ((filterType == WINF_TRIANGULAR)||(filterType == WINF_TRIANGULAR_POWERED)) {
        for (i=i_start_bin; i < i_middle_bin; i++) {
          _filterMap[i] *= (1.0 - ((middle_bin - i)/(i_middle_bin - i_start_bin)) );
        }
        for (i=i_middle_bin; i < i_end_bin; i++) {
          _filterMap[i] *= (1.0 - ((i - middle_bin)/(i_end_bin - i_middle_bin)) );
        }
      }

      if (filterType == WINF_GAUSS) {
        double x_val;
        double delta;
        double dist_val;
        for (i=i_start_bin; i < i_end_bin; i++) {
          dist_val = (double) (i_end_bin - i_start_bin);
          if (dist_val > 0.0) {
            x_val = (double)(i - i_middle_bin);
            delta = dist_val / 15.0;   // M_PI  // ?????
            _filterMap[i] = (10.0 / 4.0) * (1.0 / sqrt(2.0*M_PI)) * exp( -0.5 * (1.0/delta) * (1.0/delta) * pow(x_val,2.0));
          }
        }
      }

    }
  }

  if (filterType == WINF_TRIANGULAR_POWERED) {
    for (i = 0; i < blocksize; i++) {
      _filterMap[i] *= _filterMap[i];
    }
  }

  //Reduce bins to "first_bin <-> last_bin"
  for (i = 0; i < firstBin; i++) {
    _filterMap[i] = 0;
  }
  for (i = lastBin+1; i < blocksize; i++) {
    _filterMap[i] = 0;
  }

  if (dbA) {  // optional dbA
    FLOAT_DMEM *d = db[idxc];
    for (i = firstBin; i <= lastBin; i++) {
      _filterMap[i] *= *(d++);
    }
  }

  *(flBin[idxc]) = firstBin;
  *(flBin[idxc]+1) = lastBin;
  distance2key[idxc] = _distance2key;
  filterMap[idxc] = _filterMap;
  binKey[idxc] = _binKey;
  pitchClassNbins[idxc] = _pitchClassNbins;
}


int cTonespec::setupNamesForField(int i, const char*name, long nEl)
{
  const sDmLevelConfig *c = reader->getLevelConfig();
  setPitchclassFreq(getFconf(i));
  computeFilters(nEl, c->frameSizeSec, getFconf(i));

  return cVectorProcessor::setupNamesForField(i,"tone",nNotes);
}


/*
int cTonespec::myFinaliseInstance()
{
  int ret;
  ret = cVectorProcessor::myFinaliseInstance();
  return ret;
}
*/

// a derived class should override this method, in order to implement the actual processing
/*
int cTonespec::processVectorInt(const INT_DMEM *src, INT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  // do domething to data in *src, save result to *dst
  // NOTE: *src and *dst may be the same...
  
  return 1;
}
*/

// a derived class should override this method, in order to implement the actual processing
int cTonespec::processVectorFloat(const FLOAT_DMEM *src, FLOAT_DMEM *dst, long Nsrc, long Ndst, int idxi) // idxi=input field index
{
  int i;

  idxi=getFconf(idxi);
  FLOAT_DMEM *_distance2key = distance2key[idxi];
  FLOAT_DMEM *_filterMap = filterMap[idxi];
  int * _binKey = binKey[idxi];
  int * _pitchClassNbins = pitchClassNbins[idxi];
  int firstBin = *(flBin[idxi]);
  int lastBin = *(flBin[idxi]+1);


  // copy & square the fft magnitude
  FLOAT_DMEM *_src;
  if (usePower) {
    _src = (FLOAT_DMEM*)malloc(sizeof(FLOAT_DMEM)*Nsrc);
    if (src==NULL) OUT_OF_MEMORY;
    for (i=0; i<Nsrc; i++) {
      _src[i] = src[i]*src[i];
    }
    src = _src;
  }

  // do the tone filtering by multiplying with the filters and summing up
  bzero(dst, Ndst*sizeof(FLOAT_DMEM));

  // Sum the FFT bins for each pitch class and compute mean value
  for (i=firstBin; i <= lastBin; i++) {
    if (_binKey[i] >= 0) {
      dst[_binKey[i]] += *(src++) * _filterMap[i];
    }
  }

  for (i = 0; i < nNotes; i++) {
    if (_pitchClassNbins[i] > 0) {
      dst[i] /= (FLOAT_DMEM)(_pitchClassNbins[i]);
      if (usePower) if (dst[i]>=0.0) dst[i] = sqrt(dst[i]); else dst[i] = 0.0; // FIXME ????
    } else dst[i] = 0.0;
  }

  if ((usePower)&&(_src!=NULL)) free((void *)_src);

  return 1;
}

cTonespec::~cTonespec()
{
  multiConfFree(pitchClassFreq);
  multiConfFree(pitchClassNbins);
  multiConfFree(binKey);
  multiConfFree(distance2key);
  multiConfFree(flBin);
  multiConfFree(filterMap);
  if (dbA) multiConfFree(db);
}

