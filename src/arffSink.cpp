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

arffSink
ARFF file output (for WEKA)

*/


#include <arffSink.hpp>

#define MODULE "cArffSink"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataSink::registerComponent(_confman);
}
*/

SMILECOMPONENT_STATICS(cArffSink)

//sComponentInfo * cArffSink::registerComponent(cConfigManager *_confman)
SMILECOMPONENT_REGCOMP(cArffSink)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CARFFSINK;
  sdescription = COMPONENT_DESCRIPTION_CARFFSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")

  SMILECOMPONENT_IFNOTREGAGAIN_BEGIN
    ct->setField("filename","ARFF file to write to","smileoutput.arff");
    ct->setField("lag","output data <lag> frames behind",0);
    ct->setField("append","append to existing file (1/0 = yes/no)",0);
    ct->setField("timestamp","print timestamp attribute (1/0 = yes/no)",1);
    ct->setField("relation","name of relation","smile");
    ct->setField("instanceBase","if not empty, print instance name attribute <instanceBase_Nr>","");
    ct->setField("instanceName","if not empty, print instance name attribute <instanceName>","");
    ct->setField("number","print instance number (= frameIndex) attribute (1/0 = yes/no)",1);

    ConfigType * classType = new ConfigType("arffClass");
    classType->setField("name", "name of target", "class");
    classType->setField("type", "numeric, or nominal (= list of classes)", "numeric");
    ct->setField("class","definition of class target attributes (array for multiple targets/classes)", classType, ARRAY_TYPE);

    ConfigType * targetType = new ConfigType("arffTarget");
    targetType->setField("instance", "array containing targets for each instance", "",ARRAY_TYPE);
    targetType->setField("all", "one common target for all processed instances", "");
    
    ct->setField("target","targets (classes) for each target (class) attribute",targetType,ARRAY_TYPE);
    // TODO: custom fields, import from file, merge arff streams... read data from multiple levels... etc.
    
  SMILECOMPONENT_IFNOTREGAGAIN_END

  SMILECOMPONENT_MAKEINFO(cArffSink);
}

SMILECOMPONENT_CREATE(cArffSink)

//-----

cArffSink::cArffSink(const char *_name) :
  cDataSink(_name),
  prname(0),
  filehandle(NULL),
  filename(NULL),
  nInst(0),
  nClasses(0),
  inr(0),
  classname(NULL), classtype(NULL),
  targetall(NULL), targetinst(NULL)
{
}

void cArffSink::fetchConfig()
{
  cDataSink::fetchConfig();
  
  filename = getStr("filename");
  SMILE_DBG(3,"filename = '%s'",filename);

  lag = getInt("lag");
  SMILE_DBG(3,"lag = %i",lag);

  append = getInt("append");
  if (append) SMILE_DBG(3,"append to file is enabled");

  timestamp = getInt("timestamp");
  if (append) SMILE_DBG(3,"printing timestamp attribute enabled");

  number = getInt("number");
  if (append) SMILE_DBG(3,"printing instance number (=frame number) attribute enabled");

  relation = getStr("relation");
  SMILE_DBG(3,"ARFF relation = '%s'",relation);

  instanceBase = getStr("instanceBase");
  SMILE_DBG(3,"instanceBase = '%s'",instanceBase);

  instanceName = getStr("instanceName");
  SMILE_DBG(3,"instanceName = '%s'",instanceName);
  
  int i;
  nClasses = getArraySize("class");
  classname = (char**)calloc(1,sizeof(char*)*nClasses);
  classtype = (char**)calloc(1,sizeof(char*)*nClasses);
  for (i=0; i<nClasses; i++) {
    const char *tmp = getStr_f(myvprint("class[%i].name",i));
    if (tmp!=NULL) classname[i] = strdup(tmp);
    tmp = getStr_f(myvprint("class[%i].type",i));
    if (tmp!=NULL) classtype[i] = strdup(tmp);
  }
/*
    ConfigType * classType = new ConfigType("arffClass");
    classType->setField("name", "name of target", "class");
    classType->setField("type", "numeric, or nominal (= list of classes)", "numeric");
    ct->setField("class","definition of class target attributes (array for multiple targets/classes)", classType, ARRAY_TYPE);

    ConfigType * targetType = new ConfigType("arffTarget");
    targetType->setField("instance", "array containing targets for each instance", 0,ARRAY_TYPE);
    targetType->setField("all", "one common target for all processed instances", 0);
*/

  if (getArraySize("target") != nClasses) {
    SMILE_ERR(1,"number of targets is != number of class attributes!");
  } else {
    targetall = (char**)calloc(1,sizeof(char*)*nClasses);
    targetinst = (char***)calloc(1,sizeof(char**)*nClasses);
    nInst = -2;
    for (i=0; i<nClasses; i++) {
      char *tmp = myvprint("target[%i].instance",i);
      const char *t = getStr_f(myvprint("target[%i].all",i));
      if (t!=NULL) targetall[i] = strdup(t);
      long ni = getArraySize(tmp);
      if (nInst==-2) nInst = ni; // -1 if no array
      else {
        if (nInst != ni) COMP_ERR("number of instances in target[].instance array is not constant among all targets! %i <> %i",nInst,ni);
      }
      int j;
      if (nInst > 0) {
        targetinst[i] = (char**)calloc(1,sizeof(char*)*nInst);
        for (j=0; j<nInst; j++) {
          t = getStr_f(myvprint("%s[%i]",tmp,j));
          if (t!=NULL) targetinst[i][j] = strdup(t);
        }
      }
      free(tmp);
    }
  }
//    ct->setField("target","targets (classes) for each target (class) attribute",targetType,ARRAY_TYPE);

}

/*
int cArffSink::myConfigureInstance()
{
  int ret=1;
  ret *= cDataSink::myConfigureInstance();
  // ....
  //return ret;
}
*/


int cArffSink::myFinaliseInstance()
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

  if ((instanceBase!=NULL)&&(strlen(instanceBase)>0)) prname = 2;
  if ((instanceName!=NULL)&&(strlen(instanceName)>0)) prname = 1;
  long _N = reader->getLevelN();

  if (!ap) {
    // write arff header ....
    fprintf(filehandle, "@relation %s%s%s",relation,NEWLINE,NEWLINE);
    if (prname) {
      fprintf(filehandle, "@attribute name string%s",NEWLINE);
    }
    if (number) {
      fprintf(filehandle, "@attribute frameIndex numeric%s",NEWLINE);
    }
    if (timestamp) {
      fprintf(filehandle, "@attribute frameTime numeric%s",NEWLINE);
    }

    long i;
    for(i=0; i<_N; i++) {
      char *tmp = reader->getElementName(i);
      fprintf(filehandle, "@attribute %s numeric%s",tmp,NEWLINE);
      free(tmp);
    }

    // TODO: classes..... as config file options...
    if (nClasses > 0) {
      for (i=0; i<nClasses; i++) {
        if (classtype[i] == NULL) fprintf(filehandle, "@attribute %s numeric%s",classname[i],NEWLINE);
        else fprintf(filehandle, "@attribute %s %s%s",classname[i],classtype[i],NEWLINE);
      }
    } else {
      // default dummy class attribute...
      fprintf(filehandle, "@attribute class {0,1,2,3}%s",NEWLINE);
    }

    fprintf(filehandle, "%s@data%s%s",NEWLINE,NEWLINE,NEWLINE);

    fflush(filehandle);
  }

  return ret;
}


int cArffSink::myTick(long long t)
{
  if (filehandle == NULL) return 0;

  SMILE_DBG(4,"tick # %i, reading value vector (lag=%i):",t,lag);
  cVector *vec= reader->getFrameRel(lag);  //new cVector(nValues+1);
  if (vec == NULL) return 0;
  //else reader->nextFrame();

  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->time;

  if (prname==1) {
    fprintf(filehandle,"'%s',",instanceName);
  } else if (prname==2) {
    fprintf(filehandle,"'%s_%i',",instanceBase,vi);
  }
  if (number) fprintf(filehandle,"%i,",vi);
  if (timestamp) fprintf(filehandle,"%f,",tm);
  
  // now print the vector:
  int i;
  fprintf(filehandle,"%e",vec->dataF[0]);
  for (i=1; i<vec->N; i++) {
    fprintf(filehandle,",%e",vec->dataF[i]);
    //printf("  (a=%i vi=%i, tm=%fs) %s.%s = %f\n",reader->getCurR(),vi,tm,reader->getLevelName(),vec->name(i),vec->dataF[i]);
  }

  // classes: TODO:::
  if (nClasses > 0) {
    if (nInst>0) {
      if (inr >= nInst) {
        SMILE_WRN(3,"more instances writte to ARFF file, then there are targets available for (%i)!",nInst);
        if (targetall != NULL) {
          for (i=0; i<nClasses; i++) {
            if (targetall[i] != NULL)
              fprintf(filehandle,",%s",targetall[i]);
            else
              fprintf(filehandle,",NULL");
          }
        } else {
          for (i=0; i<nClasses; i++) {
            fprintf(filehandle,",NULL");
          }
        }
		//inr++;
      } else {
        for (i=0; i<nClasses; i++) {
          fprintf(filehandle,",%s",targetinst[i][inr]);
        }
		inr++;
      }
    } else {
      if (targetall != NULL) {
        for (i=0; i<nClasses; i++) {
          if (targetall[i] != NULL)
            fprintf(filehandle,",%s",targetall[i]);
          else
            fprintf(filehandle,",NULL");
        }
      } else {
        for (i=0; i<nClasses; i++) {
          fprintf(filehandle,",NULL");
        }
      }
    }
  } else {
    // dummy class attribute, always 0
    fprintf(filehandle,",0");
  }

  fprintf(filehandle,"%s",NEWLINE);

  fflush(filehandle);

  // tick success
  return 1;
}


cArffSink::~cArffSink()
{
  fclose(filehandle);
  int i;
  if (classname!=NULL) {
    for (i=0; i<nClasses; i++) if (classname[i] != NULL) free(classname[i]);
    free(classname);
  }
  if (classtype!=NULL) {
    for (i=0; i<nClasses; i++) if (classtype[i] != NULL) free(classtype[i]);
    free(classtype);
  }
  if (targetall!=NULL) {
    for (i=0; i<nClasses; i++) if (targetall[i] != NULL) free(targetall[i]);
    free(targetall);
  }
  if (targetinst!=NULL) {
    int j;
    for (i=0; i<nClasses; i++) {
      if (targetinst[i] != NULL) {
        for (j=0; j<nInst; j++) {
          if (targetinst[i][j] != NULL) free(targetinst[i][j]);
        }
        free(targetinst[i]);
      }
    }
    free(targetinst);
  }
}

