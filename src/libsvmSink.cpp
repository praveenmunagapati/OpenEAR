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

LibSVM feature file output

*/


#include <libsvmSink.hpp>

#define MODULE "cLibsvmSink"


SMILECOMPONENT_STATICS(cLibsvmSink)

SMILECOMPONENT_REGCOMP(cLibsvmSink)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CLIBSVMSINK;
  sdescription = COMPONENT_DESCRIPTION_CLIBSVMSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")

  SMILECOMPONENT_IFNOTREGAGAIN_BEGIN
    ct->setField("filename","libsvm file to write to","smileoutput.lsvm");
    ct->setField("lag","output data <lag> frames behind",0);
    ct->setField("append","append to existing file (1/0 = yes/no)",0);
    ct->setField("timestamp","print timestamp attribute (1/0 = yes/no)",1);
    ct->setField("instanceBase","if not empty, print instance name attribute <instanceBase_Nr>","");
    ct->setField("instanceName","if not empty, print instance name attribute <instanceName>","");
//    ct->setField("number","print instance number (= frameIndex) attribute (1/0 = yes/no)",1);

    ct->setField("class","optional definition of class-name strings (array for multiple classes)", "classX", ARRAY_TYPE);
    ct->setField("targetNum","targets (as numbers/indicies) for each instance",0,ARRAY_TYPE);
    ct->setField("targetStr","targets (as strings) for each instance","classX",ARRAY_TYPE);
    ct->setField("targetNumAll","target (as numbers/indicies) for all instances",0,ARRAY_TYPE);
    ct->setField("targetStrAll","target (as strings) for all instances","classX",ARRAY_TYPE);

  SMILECOMPONENT_IFNOTREGAGAIN_END

  SMILECOMPONENT_MAKEINFO(cLibsvmSink);
}

SMILECOMPONENT_CREATE(cLibsvmSink)

//-----

cLibsvmSink::cLibsvmSink(const char *_name) :
  cDataSink(_name),
  filehandle(NULL),
  filename(NULL),
  nInst(0),
  nClasses(0),
  inr(0),
  targetNumAll(0),
  classname(NULL), target(NULL)
{
}

void cLibsvmSink::fetchConfig()
{
  cDataSink::fetchConfig();
  
  filename = getStr("filename");
  SMILE_DBG(3,"filename = '%s'",filename);

  lag = getInt("lag");
  SMILE_DBG(3,"lag = %i",lag);

  append = getInt("append");
  if (append) SMILE_DBG(3,"append to file is enabled");

  timestamp = getInt("timestamp");
  if (append) SMILE_DBG(3,"printing timestamp attribute (index 1) enabled");

  instanceBase = getStr("instanceBase");
  SMILE_DBG(3,"instanceBase = '%s'",instanceBase);

  instanceName = getStr("instanceName");
  SMILE_DBG(3,"instanceName = '%s'",instanceName);
  
  int i;
  nClasses = getArraySize("class");
  classname = (char**)calloc(1,sizeof(char*)*nClasses);
  for (i=0; i<nClasses; i++) {
    const char *tmp = getStr_f(myvprint("class[%i]",i));
    if (tmp!=NULL) classname[i] = strdup(tmp);
  }

  if (isSet("targetNumAll")) {
    targetNumAll = getInt("targetNumAll");
  }
  if (isSet("targetStrAll")) {
    if (nClasses <=0) COMP_ERR("cannt have 'targetStrAll' option if no class names have been defined using the 'class' option! (inst '%s')",getInstName());
    targetNumAll = getClassIndex(getStr("targetStrAll"));
  }
  nInst = getArraySize("targetNum");
  if (nInst > 0) {
    target = (int *)calloc(1,sizeof(int)*nInst);
    for (i=0; i<nInst; i++) {
      target[i] = getInt_f(myvprint("targetNum[%i]",i));
      if (target[i] < 0) COMP_ERR("invalid class index %i for instance %i (in 'targetNum' option of instance '%s')",target[i],i,getInstName());
    }
  } else {
    nInst = getArraySize("targetStr");
    if (nInst > 0) {
      if (nClasses <=0) COMP_ERR("cannt have 'targetStr' option if no class names have been defined using the 'class' option! (inst '%s')",getInstName());
      target = (int *)calloc(1,sizeof(int)*nInst);
      for (i=0; i<nInst; i++) {
        target[i] = getClassIndex(getStr_f(myvprint("targetStr[%i]",i)));
        if (target[i] < 0) COMP_ERR("invalid class index %i for instance %i (from class '%s' in 'targetStr' option of instance '%s')",target[i],i,getStr_f(myvprint("targetStr[%i]",i)),getInstName());
      }
    } else { nInst = 0; }
  }

}

/*
int cLibsvmSink::myConfigureInstance()
{
  int ret=1;
  ret *= cDataSink::myConfigureInstance();
  // ....
  //return ret;
}
*/


int cLibsvmSink::myFinaliseInstance()
{
  int ap=0;

  int ret = cDataSink::myFinaliseInstance();
  if (ret==0) return 0;
  
  if (append) {
    // check if file exists:
    filehandle = fopen(filename, "r");
    if (filehandle != NULL) {
      fclose(filehandle);
      filehandle = fopen(filename, "a");
      ap=1;
    } else {
      filehandle = fopen(filename, "w");
    }
  } else {
    filehandle = fopen(filename, "w");
  }
  if (filehandle == NULL) {
    COMP_ERR("Error opening file '%s' for writing (component instance '%s', type '%s')",filename, getInstName(), getTypeName());
  }
  
//  if (!ap) {
  // write header ....
//  if (timestamp) {
//    fprintf(filehandle, "@attribute frameTime numeric%s",NEWLINE);
//  }
/*
  long _N = reader->getLevelN();
  long i;
  for(i=0; i<_N; i++) {
    char *tmp = reader->getElementName(i);
    fprintf(filehandle, "@attribute %s numeric%s",tmp,NEWLINE);
    free(tmp);
  }
*/
//  }
  
  return ret;
}


int cLibsvmSink::myTick(long long t)
{
  if (filehandle == NULL) return 0;

  SMILE_DBG(4,"tick # %i, writing to lsvm file (lag=%i):",t,lag);
  cVector *vec= reader->getFrameRel(lag);  //new cVector(nValues+1);
  if (vec == NULL) return 0;
  //else reader->nextFrame();

  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;
  long idx = 1;
  
/*
  if (prname==1) {
    fprintf(filehandle,"'%s',",instanceName);
  } else if (prname==2) {
    fprintf(filehandle,"'%s_%i',",instanceBase,vi);
  }
  */
  // classes: TODO:::
  if ((nClasses > 0)&&(nInst>0)) {  // per instance classes
    if (inr >= nInst) {
      SMILE_WRN(3,"more instances written to LibSVM file (%i), then there are targets available for (%i)!",inr,nInst);
      fprintf(filehandle,"%i ",targetNumAll);
    } else {
      fprintf(filehandle,"%i ",target[inr++]);
    }
  } else {
    fprintf(filehandle,"%i ",targetNumAll);
  }

//  if (number) fprintf(filehandle,"%i:%i ",idx++,vi);
  if (timestamp) fprintf(filehandle,"%i:%f ",idx++,tm);

  
  // now print the vector:
  int i;
  fprintf(filehandle,"%i:%e ",idx++,vec->dataF[0]);
  for (i=1; i<vec->N; i++) {
    fprintf(filehandle,"%i:%e ",idx++,vec->dataF[i]);
  }

  fprintf(filehandle,"%s",NEWLINE);

  // tick success
  return 1;
}


cLibsvmSink::~cLibsvmSink()
{
  fclose(filehandle);
  int i;
  if (classname!=NULL) {
    for (i=0; i<nClasses; i++) if (classname[i] != NULL) free(classname[i]);
    free(classname);
  }
  if (target!=NULL) free(target);
}

