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


/*  openSMILE component: htkSink

HTK binary feature file output

*/


#include <htkSink.hpp>

#define MODULE "cHtkSink"


SMILECOMPONENT_STATICS(cHtkSink)

SMILECOMPONENT_REGCOMP(cHtkSink)
{
  SMILECOMPONENT_REGCOMP_INIT;
  scname = COMPONENT_NAME_CHTKSINK;
  sdescription = COMPONENT_DESCRIPTION_CHTKSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink");
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","ARFF file to write to","smileoutput.htk");
    ct->setField("lag","output data <lag> frames behind",0);
    ct->setField("append","append to existing file (1/0 = yes/no)",0);
    ct->setField("parmKind","HTK parmKind (9=USER)",9);
  )

  SMILECOMPONENT_MAKEINFO(cHtkSink);
}

SMILECOMPONENT_CREATE(cHtkSink)

//-----

cHtkSink::cHtkSink(const char *_name) :
  cDataSink(_name),
  filehandle(NULL),
  filename(NULL),
  nVec(0),
  vecSize(0),
  period(0.0)
{
  bzero(&header, sizeof(sHTKheader));
  if ( IsVAXOrder() ) vax = 1;
  else vax = 0;
}

void cHtkSink::fetchConfig()
{
  cDataSink::fetchConfig();
  
  filename = getStr("filename");
  SMILE_DBG(3,"filename = '%s'",filename);

  lag = getInt("lag");
  SMILE_DBG(3,"lag = %i",lag);

  append = getInt("append");
  if (append) SMILE_DBG(3,"append to file is enabled");

  parmKind = (uint16_t)getInt("parmKind");
  SMILE_DBG(3,"parmKind = %i",parmKind);
}

/*
int cHtkSink::myConfigureInstance()
{
  int ret=1;
  ret *= cDataSink::myConfigureInstance();
  // ....
  //return ret;
}
*/

int cHtkSink::writeHeader()
{
  if (filehandle==NULL) return 0;

  header.nSamples = nVec;
  header.samplePeriod = (uint32_t)round(period*10000000.0);
  if ((uint32_t)sizeof(float) * vecSize >= (uint32_t)2<<16) {
    SMILE_ERR(1,"vecSize overflow for HTK output: vecSize (%i) > max. HTK vecSize (%i)! limiting vecSize",(uint32_t)sizeof(float) * (uint32_t)vecSize,(uint32_t)2<<16);
    vecSize = (2<<16) - 1;
  }
  header.sampleSize = (uint16_t)(sizeof(float) * vecSize);
  header.parmKind = parmKind;  // TODO: support others like MFCC, MELSPEC, etc.

  // adjust endianness
  sHTKheader head;  // local copy, due to prepareHeader! we don't want to change 'header' variable!
  memcpy(&head, &header, sizeof(sHTKheader));
  prepareHeader(&head);

  // seek to beginning of file:
  fseek(filehandle, 0 , SEEK_SET);
  
  // write header:
  if (!fwrite(&head, sizeof(sHTKheader), 1, filehandle)) {
    SMILE_ERR(1,"Error writing to htk feature file '%s'!",filename);
    return 0;
  }

  return 1;
}

int cHtkSink::readHeader()
{
  if (filehandle==NULL) return 0;
  if (!fread(&header, sizeof(sHTKheader), 1, filehandle)) {
    SMILE_ERR(1,"error reading header from file '%s'",filename);
    return 0;
  }
  prepareHeader(&header); // convert to host byte order
  return 1;
}

int cHtkSink::myFinaliseInstance()
{
  int ap=0;

  int ret = cDataSink::myFinaliseInstance();
  if (ret==0) return 0;

  period = reader->getLevelT();
  vecSize = reader->getLevelN();
  
  if (append) {
    // check if file exists:
    filehandle = fopen(filename, "rb");
    if (filehandle != NULL) {
      if (!readHeader()) {
        SMILE_ERR(1,"error reading header from file '%s' (which seems to exist)! we cannot append to that file!");
        // TODO: force overwrite via config file option in this case...
        ret = 0;
      }
      if (ret) {
        if (header.samplePeriod != (uint32_t)round(period*10000000.0)) {
          SMILE_ERR(1,"cannot append to '%s': samplePeriod mismatch (should be: %i, in file on disk: %i)",filename,(uint32_t)round(period*10000000.0),header.samplePeriod);
          ret = 0;
        }
        if (header.sampleSize != (uint16_t)(sizeof(float) * vecSize)) {
          SMILE_ERR(1,"cannot append to '%s': sampleSize mismatch (should be: %i, in file on disk: %i)",filename,(uint16_t)(sizeof(float) * vecSize),header.sampleSize);
          ret = 0;
        }
      }
      nVec = header.nSamples;
      fclose(filehandle); filehandle = NULL;
      if (ret==0) return 0;
      filehandle = fopen(filename, "ab");
      ap=1;
    } else {
      filehandle = fopen(filename, "wb");
    }
  } else {
    filehandle = fopen(filename, "wb");
  }
  if (filehandle == NULL) {
    COMP_ERR("Error opening binary file '%s' for writing (component instance '%s', type '%s')",filename, getInstName(), getTypeName());
  }
  
  if (!ap) {
    // write dummy htk header ....
    writeHeader();
  }
  
  return ret;
}


int cHtkSink::myTick(long long t)
{
  if (filehandle == NULL) return 0;

  SMILE_DBG(4,"tick # %i, reading value vector (lag=%i):",t,lag);
  cVector *vec= reader->getFrameRel(lag);  //new cVector(nValues+1);
  if (vec == NULL) return 0;

  // now print the vector:
  int i; float *tmp = (float *)malloc(sizeof(float)*vec->N);
  if (tmp==NULL) OUT_OF_MEMORY;

  if (vec->type == DMEM_FLOAT) {
    for (i=0; i<vec->N; i++) {
      tmp[i] = (float)(vec->dataF[i]);
      if (vax) SwapFloat(tmp+i);
    }
  } else if (vec->type == DMEM_INT) {
    for (i=0; i<vec->N; i++) {
      tmp[i] = (float)(vec->dataI[i]);
      if (vax) SwapFloat(tmp+i);
    }
  } else {
    SMILE_ERR(1,"unknown data type %i",vec->type);
    return 0;
  }

  int ret = 1;
  
  if (!fwrite(tmp,sizeof(float),vec->N,filehandle)) {
    SMILE_ERR(1,"Error writing to raw feature file '%s'!",filename);
    ret = 0;
  } else {
    //reader->nextFrame();
    nVec++;
  }

  free(tmp);

  // tick success
  return ret;
}


cHtkSink::~cHtkSink()
{
  writeHeader();
  fclose(filehandle);
}

