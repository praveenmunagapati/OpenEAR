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

LibSVM live classifier/regressor

*/



#include <libsvmliveSink.hpp>

#define MODULE "cLibsvmLiveSink"


SMILECOMPONENT_STATICS(cLibsvmLiveSink)

//sComponentInfo * cLibsvmLiveSink::registerComponent(cConfigManager *_confman)
SMILECOMPONENT_REGCOMP(cLibsvmLiveSink)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CLIBSVMLIVESINK;
  sdescription = COMPONENT_DESCRIPTION_CLIBSVMLIVESINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")

  SMILECOMPONENT_IFNOTREGAGAIN_BEGIN
    ct->makeMandatory(ct->setField("model","LibSVM model file to load","svm.model"));
    ct->makeMandatory(ct->setField("scale","LibSVM scale file to load",(const char*)NULL));
    ct->setField("fselection","feature selection file to apply (leave empty to use all features)",(const char*)NULL);
    ct->setField("classes","class name lookup file (leave empty to display libsvm class numbers/indicies)",(const char*)NULL);
    ct->setField("predictProbability","predict class probabilities (1/0=yes/no)",0);
    ct->setField("printResult","print classification/regression result to console (1/0=yes/no)",0);
    ct->setField("resultRecp","component(s) to send 'classificationResult' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages",(const char *) NULL);
    ct->setField("resultMessageName","custom name that is sent with 'classificationResult' message","svm_result");
    ct->setField("lag","output data <lag> frames behind (obsolete for this component...?)",0);
    
  SMILECOMPONENT_IFNOTREGAGAIN_END

  SMILECOMPONENT_MAKEINFO(cLibsvmLiveSink);
}

SMILECOMPONENT_CREATE(cLibsvmLiveSink)

//-----

cLibsvmLiveSink::cLibsvmLiveSink(const char *_name) :
  cDataSink(_name),
  modelfile(NULL), scalefile(NULL), predictProbability(0),
  labels(NULL), probEstimates(NULL),
  nClasses(0), printResult(1),
  fselType(0), Nsel(-1),
  classNames(0), nCls(0),
  sendResult(0),
  resultMessageName(NULL), resultRecp(NULL)
{
  outputSelIdx.enabled = NULL;
  outputSelStr.n = 0;
  outputSelStr.names = NULL;
}

void cLibsvmLiveSink::fetchConfig()
{
  cDataSink::fetchConfig();
  
  modelfile = getStr("model");
  SMILE_IDBG(2,"filename of model to load = '%s'",modelfile);

  scalefile = getStr("scale");
  SMILE_IDBG(2,"filename of scale data to load = '%s'",scalefile);

  fselection = getStr("fselection");
  if (fselection != NULL) SMILE_IDBG(2,"filename of fselection to load = '%s'",fselection);

  classes = getStr("classes");
  if (classes != NULL) SMILE_IDBG(2,"filename of class mapping to load = '%s'",classes);

  lag = getInt("lag");
  if (lag != 0) SMILE_IDBG(2,"lag = %i frames",lag);

  printResult = getInt("printResult");
  SMILE_IDBG(2,"printResult = %i",printResult);

  predictProbability = getInt("predictProbability");
  SMILE_IDBG(2,"predictProbability = %i",predictProbability);

  resultRecp = getStr("resultRecp");
  SMILE_IDBG(2,"resultRecp = '%s'",resultRecp);
  if (resultRecp != NULL) sendResult = 1;

  resultMessageName = getStr("resultMessageName");
  SMILE_IDBG(2,"resultMessageName = '%s'",resultMessageName);
}

/*
int cLibsvmLiveSink::myConfigureInstance()
{
  int ret=1;
  ret *= cDataSink::myConfigureInstance();
  // ....
  //return ret;
}
*/

#define MAX_LINE_LENGTH 1024

int cLibsvmLiveSink::loadClasses( const char *file )
{
  if (file != NULL) {
    if (strlen(file)<1) return 0;

    FILE *f = fopen(file,"r");
    if (f== NULL) {
      SMILE_IERR(2,"error opening class map file '%s' for reading! NOT using a class map!",file);
      return 0;
    }

    char line[MAX_LINE_LENGTH+1];
    nCls=0;
    
    while(fgets(line,MAX_LINE_LENGTH,f) != NULL) {
        if (strlen( line ) > 1) { 
          line[strlen( line ) - 1] = 0;
          const char *cn = strchr(line,':');
          if (cn!=NULL) {
            nCls++;
            // TODO: use class number instead of cont. index
          }
        }
    }
    fclose(f);

    f = fopen(file,"r");
    if (f== NULL) {
      SMILE_IERR(2,"error opening class map file '%s' for reading (2nd pass)! NOT using a class map!",file);
      return 0;
    }

    int i=0;
    classNames=(char**)calloc(1,sizeof(char*)*nCls);
    while(fgets(line,MAX_LINE_LENGTH,f) != NULL) {
        if (strlen( line ) > 1) { 
          line[strlen( line ) - 1] = 0;
          const char *cn = strchr(line,':');
          if (cn!=NULL) {
            classNames[i++] = strdup(cn+1);
            // TODO: use class number instead of cont. index
          }
        }
    }
    fclose(f);

    return 1;
  }
  return 0;
}

int cLibsvmLiveSink::loadSelection( const char *selFile )
{
  if (selFile != NULL) {
    if (strlen(selFile)<1) return 0;

    FILE *f = fopen(selFile,"r");
    if (f== NULL) {
      SMILE_IERR(2,"error opening feature selection file '%s' for reading! NOT using a feature selection!",selFile);
      return 0;
    }
    
    // read first line to determine filetype:
    char line[MAX_LINE_LENGTH+1];
    long nStr=0;
    fgets( line, 5, f);
    line[3] = 0;
    if (!strcmp(line,"str")) { // string list
      fselType = 2;
      SMILE_IDBG(5,"reading string list of features");
      fscanf( f, "%u\n", &nStr);
      if (nStr < 1) { COMP_ERR("Error reading feature selection file, nFeatures < 1!"); }
      outputSelStr.n = nStr;
      Nsel=nStr;
      outputSelStr.names = (char **)calloc(1,sizeof(char *)*nStr);
      int i=0; line[0] = 0;
      while(fgets(line,MAX_LINE_LENGTH,f) != NULL) {
        if (strlen( line ) > 1) { 
          line[strlen( line ) - 1] = 0;
          outputSelStr.names[i++] = strdup(line);
        }
      }
      SMILE_IDBG(5,"enabled %i features",i);
      fclose(f);
      return 1;
    } else if (!strcmp(line,"idx")) { // index list
      fselType = 1;
      SMILE_IDBG(5,"reading index list of features");
      long idx=0; int i;
      // pass1: parse for max index
      while(fscanf(f,"%u\n",&idx) == 1)
        outputSelIdx.nFull = MAX(outputSelIdx.nFull, idx);
      SMILE_IDBG(5,"max index in selection file was found to be %i",outputSelIdx.nFull);
      outputSelIdx.nFull++;
      outputSelIdx.enabled = (long *)calloc(1,sizeof(long)*outputSelIdx.nFull);
      rewind( f );
      fgets(line, 5, f); // skip header line;
      // pass2: parse for max index
      i=0;
      while(fscanf(f,"%u\n",&idx) == 1) {
        outputSelIdx.enabled[idx] = 1; // enable feature "idx"
        i++;
      }
      outputSelIdx.nSel = i;
      Nsel = i;
      SMILE_IDBG(5,"enabled %i features",i);
      fclose(f);
      return 1;
    } else { // bogus file...
      COMP_ERR("error parsing fselection file '%s'. bogus header! expected 'str' or 'idx' at beginning. found '%s'.",selFile,line);
      fclose( f );
    }
  }
  return 0;
}

int cLibsvmLiveSink::myFinaliseInstance()
{
  int ap=0;

  int ret = cDataSink::myFinaliseInstance();
  if (ret==0) return 0;
  
  // TODO: binary model files...
  // load model
  SMILE_MSG(2,"loading LibSVM model for instance '%s' ...",getInstName()); 
  if((model=svm_load_model(modelfile))==0) {
    COMP_ERR("can't open libSVM model file '%s'",modelfile);
  }

  nClasses = svm_get_nr_class(model);
  svmType = svm_get_svm_type(model);

  if(predictProbability) {
    if ((svmType==NU_SVR) || (svmType==EPSILON_SVR)) {
      nClasses = 0;
      SMILE_MSG(2,"LibSVM prob. model (regression) for test data: target value = predicted value + z,\nz: Laplace distribution e^(-|z|/sigma)/(2sigma),sigma=%g",svm_get_svr_probability(model));
    } else {
      labels=(int *) malloc(nClasses*sizeof(int));
      svm_get_labels(model,labels);

      SMILE_MSG(3,"LibSVM %i labels in model '%s':",nClasses,modelfile);
      int j;
      for(j=0;j<nClasses;j++)
        SMILE_MSG(3,"  Label[%i] : '%d'",j,labels[j]);
	}
  }

  //?? move this in front of above if() block ?
  if ((predictProbability)&&(nClasses>0))
    probEstimates = (double *) malloc(nClasses*sizeof(double));

  // load scale
  if((scale=svm_load_scale(scalefile))==0) {
	COMP_ERR("can't open libSVM scale file '%s'",scalefile);
  }

  // load selection
  loadSelection(fselection);

  //TODO: check compatibility of getLevelN() (possibly after selection), number of features in model, and scale
  
  if (nClasses>0) {
    // load class mapping
    loadClasses(classes);
  } else {
    if (classes != NULL) SMILE_IWRN(2,"not loading given class mapping file for regression SVR model (there are no classes...)!");
  }

  return ret;
}

void cLibsvmLiveSink::processResult(long long tick, long frameIdx, double time, float res, double *probEstim, int nClasses, double dur)
{
  if (printResult) {
    if ((nCls>0)&&(nClasses > 0)&&(classNames != NULL)) {
      if (labels!=NULL) {
        if ((int)res >= nClasses) res = (float)(nClasses-1);
        if (res < 0.0) res = 0.0;
        res = (float)labels[(int)res];
      }
      if ((int)res >= nCls) res = (float)nCls;
      if (res < 0.0) res = 0.0;
      SMILE_PRINT("\n LibSVM  '%s' result (@ time: %f) :  ~~> %s <~~",getInstName(),time,classNames[(int)res]);
    } else {
      SMILE_PRINT("\n LibSVM  '%s' result (@ time: %f) :  ~~> %.2f <~~",getInstName(),time,res);
    }
    if (probEstim != NULL) {
      int i;
      for (i=0; i<nClasses; i++) {
        int idx = i;
        if (labels!=NULL) idx = labels[i];
        if ((nCls>0)&&(nClasses > 0)&&(classNames != NULL)) {
          if (idx >= nCls) idx=nCls-1;
          if (idx < 0) idx = 0;
          SMILE_PRINT("     prob. class '%s': \t %f",classNames[idx],probEstim[i]);
        } else {
          SMILE_PRINT("     prob. class %i : \t %f",idx,probEstim[i]);
        }
      }
    }
  }

  // send result as componentMessage 
  if (sendResult) {
    cComponentMessage msg("classificationResult", resultMessageName);
    if ((nCls>0)&&(nClasses > 0)&&(classNames != NULL)) {
      if (labels!=NULL) {
        if ((int)res >= nClasses) res = (float)(nClasses-1);
        if (res < 0.0) res = 0.0;
        res = (float)labels[(int)res];
      }
      if ((int)res >= nCls) res = (float)nCls;
      if (res < 0.0) res = 0.0;
      strncpy(msg.msgtext, classNames[(int)res], CMSG_textLen);
    }
    msg.floatData[0] = res;
    msg.intData[0]   = nClasses;
    msg.custData     = probEstim;
    msg.userTime1    = time;
    msg.userTime2    = time+dur;

    // TO TEST .....
    sendComponentMessage( resultRecp, &msg );
    SMILE_IDBG(3,"sending 'classificationResult' message to '%s'",resultRecp);
  }
}

/*
   // genOutput_applySelection(obj);
int genOutput_applySelection( pGenOutput obj )
#define FUNCTION "genOutput_applySelection"
{_FUNCTION_ENTER_
  int i,j,N=0;
  
  if (obj != NULL) {
    if (obj->outputSelType == 1) {  // index list
      if (obj->outputDef.namesFull != NULL) {
         FEATUM_DEBUG(6,"applying index list feature selection...");
         obj->outputDef.names = (char **)calloc(1,sizeof(char *)*obj->outputSelIdx.nSel);
         for (i=0; i<obj->outputDef.nFull; i++) {
           if ((i<obj->outputSelIdx.nFull)&&(obj->outputSelIdx.enabled[i])) { 
             obj->outputDef.names[N++] = strdup(obj->outputDef.namesFull[i]);
           }
         }
         if (N==0) {
           FEATUM_ERROR(1,"No features enabled! Ignoring feature selection");
           obj->outputDef.n = obj->outputDef.nFull;        
           free(obj->outputDef.names);  
           obj->outputDef.names = obj->outputDef.namesFull;
           obj->outputSelIdx.nFull = 0;           
         } else {
           obj->outputDef.n = N;
           obj->outputVec.n = N;
         }
         FEATUM_DEBUG(7,"DONE : applying index list feature selection.");
      }     
      
      _FUNCTION_RETURN_(1);
    } else
    if (obj->outputSelType == 2) {  // string list
      if (obj->outputDef.namesFull != NULL) {
         FEATUM_DEBUG(6,"applying string list feature selection...");
         obj->outputDef.names = (char **)calloc(1,sizeof(char *)*obj->outputSelStr.n);
         if (obj->outputSelIdx.enabled) free(obj->outputSelIdx.enabled);
         obj->outputSelIdx.enabled = (int *)calloc(1,sizeof(int)*obj->outputDef.nFull);
         for (i=0; i<  obj->outputDef.nFull; i++) {
             for (j=0; j<obj->outputSelStr.n; j++) {
               if (!strcmp(obj->outputDef.namesFull[i], obj->outputSelStr.names[j])) {
                 obj->outputDef.names[N++] = strdup(obj->outputDef.namesFull[i]);
                 obj->outputSelIdx.enabled[i] = 1;
                 break;                                     
               }
             }             
         }
         if (N==0) {
           FEATUM_ERROR(1,"No matching features! Ignoring feature selection");
           obj->outputDef.n = obj->outputDef.nFull;        
           free(obj->outputDef.names);  
           obj->outputDef.names = obj->outputDef.namesFull;
           obj->outputSelIdx.nFull = 0;
         } else {
           obj->outputDef.n = N;
           obj->outputVec.n = N;
           // we must now build the index mapping:
           obj->outputSelIdx.nFull = obj->outputDef.nFull;
           obj->outputSelIdx.nSel = N;
           
         }
         FEATUM_DEBUG(7,"DONE : applying string list feature selection.");
      }     
      _FUNCTION_RETURN_(1);
    }
  }
  _FUNCTION_RETURN_(0);
}
#undef FUNCTION

*/

int cLibsvmLiveSink::buildEnabledSelFromNames(long N, const FrameMetaInfo *fmeta)
{
  int n,i;
  SMILE_IDBG(3,"computing mapping from feature names to feature indicies");
  outputSelIdx.nFull = N;
  outputSelIdx.nSel = outputSelStr.n;
  outputSelIdx.enabled = (long *)calloc(1,sizeof(long)*outputSelIdx.nFull);
  for (n=0; n<N; n++) {
    for (i=0; i<outputSelStr.n; i++) {
      if (!strcmp(fmeta->getName(n,NULL),outputSelStr.names[i]))
        outputSelIdx.enabled[n] = 1;
    }
  }
  SMILE_IDBG(3,"mapping computed successfully");
  return 1;
}

int cLibsvmLiveSink::myTick(long long t)
{
  if (model == NULL) return 0;

  SMILE_DBG(4,"tick # %i, classifiy value vector using LibSVM (lag=%i):",t,lag);
  cVector *vec= reader->getFrameRel(lag);  //new cVector(nValues+1);
  if (vec == NULL) return 0;
//  else reader->nextFrame();

  struct svm_node *x = NULL;
  int i = 0;
  double v;
                                          // need one more for index = -1
  long Nft = Nsel;
  if (Nft <= 0) Nft = vec->N;

  if (fselType) {
    if ((outputSelIdx.enabled == NULL)&&(outputSelStr.names != NULL)) {
      buildEnabledSelFromNames(vec->N, vec->fmeta);
    } else if (outputSelIdx.enabled == NULL) Nft = vec->N;
  }
    
  // TODO: fselection by names... 
  // TODO: compute Nsel in loadSelection

  x = (struct svm_node *) malloc( (Nft + 1) * sizeof(struct svm_node));
  int j = 0;
  for (i=0; i<vec->N; i++) {
    if ((outputSelIdx.enabled == NULL)||(Nsel<=0)) {
      x[i].index = i+1; // FIXME!!! +1 is ok??? (no!?)
      x[i].value = vec->dataF[i];
    } else {
      if (outputSelIdx.enabled[i]) {
        x[j].index = j+1; // FIXME!!! +1 is ok??? (no!?)
        x[j].value = vec->dataF[i];
        j++;
      }
    }
  }
  if ((outputSelIdx.enabled == NULL)||(Nsel<=0)) {
    x[i].index = -1;
    x[i].value = 0.0;
  } else {
    x[j].index = -1;
    x[j].value = 0.0;
  }

  svm_apply_scale(scale,x);

     /*
        for (i=0; i<vec->n; i++) {
          printf("%i:%f ",i,x[i].value);
        }
        printf("\n");
     */
  long vi = vec->tmeta->vIdx;
  double tm = vec->tmeta->smileTime;
  double dur = vec->tmeta->lengthSec;

  if ( (predictProbability) && (svmType==C_SVC || svmType==NU_SVC) ) {
    v = svm_predict_probability(model,x,probEstimates);
    processResult(t, vi, tm, v, probEstimates, nClasses, dur);
//    printf("%g",v);
//    for(j=0;j<nr_class;j++)
//      printf(" %g",prob_estimates[j]);
//    printf("\n");
  } else {
    v = svm_predict(model,x);
    processResult(t, vi, tm, v, NULL, nClasses, dur);
//    result = v;
//    printf("%g\n",v);
  }
  free(x);

/*
	if (svm_type==NU_SVR || svm_type==EPSILON_SVR)
	{
		printf("Mean squared error = %g (regression)\n",error/total);
		printf("Squared correlation coefficient = %g (regression)\n",
		       ((total*sumvy-sumv*sumy)*(total*sumvy-sumv*sumy))/
		       ((total*sumvv-sumv*sumv)*(total*sumyy-sumy*sumy))
		       );
	}
	else
		printf("Accuracy = %g%% (%d/%d) (classification)\n",
		       (double)correct/total*100,correct,total);
  */
  
  


  // tick success
  return 1;
}


cLibsvmLiveSink::~cLibsvmLiveSink()
{
  svm_destroy_model(model);
  svm_destroy_scale(scale);
  if ((predictProbability)&&(probEstimates!=NULL)) free(probEstimates);
  if (labels != NULL) free(labels);

  if (outputSelIdx.enabled != NULL) { 
    free(outputSelIdx.enabled);
  }
  int n;
  if (outputSelStr.names != NULL) {
    for(n=0; n<outputSelStr.n; n++) {
      if (outputSelStr.names[n] != NULL) free(outputSelStr.names[n]);
    }                             
    free( outputSelStr.names );
  }
  if (classNames!=NULL) {
    for (n=0; n<nCls; n++) {
      if (classNames[n] != NULL) free(classNames[n]);
    }
    free(classNames);
  }
}


/**************************************************************************/
/*********              LibSVM   addon:   scale functions  ****************/
/**************************************************************************/

#include <float.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef __WINDOWS
#define max(x,y) (((x)>(y))?(x):(y))
#define min(x,y) (((x)<(y))?(x):(y))
#endif

/*
struct svm_scale {
  int max_index;
  double lower,upper;
  double y_lower,y_upper;
  double y_min,y_max;
  double *feature_max;
  double *feature_min;
};
*/
char *line = NULL;
int max_line_len = 1024;

char* readline(FILE *input)
{
	int len;

	if (line == NULL) line = (char *) calloc(1, max_line_len*sizeof(char));

	if(fgets(line,max_line_len,input) == NULL)
		return NULL;

	while(strrchr(line,'\n') == NULL)
	{
		max_line_len *= 2;
		line = (char *) realloc(line, max_line_len);
		len = (int) strlen(line);
		if(fgets(line+len,max_line_len-len,input) == NULL)
			break;
	}
	return line;
}

struct svm_scale * svm_load_scale(const char* restore_filename)
{

  if (restore_filename==NULL) { return NULL; }

  int idx, c;
  FILE *fp_restore = NULL;
  struct svm_scale *ret = NULL;
  double fmin, fmax;
  double y_max = -DBL_MAX;
  double y_min = DBL_MAX;
  
  fp_restore = fopen(restore_filename,"r");
  if(fp_restore==NULL)
  {
    fprintf(stderr,"can't open scale file %s\n", restore_filename);
    return NULL;
  }

  c = fgetc(fp_restore);
  if(c == 'y')
  {
    readline(fp_restore);
	readline(fp_restore);
	readline(fp_restore);
  }
  readline(fp_restore);
  readline(fp_restore);

  ret = (struct svm_scale *) calloc(1, sizeof(struct svm_scale) );
  if (ret == NULL)
  {
    fprintf(stderr,"Error allocating memory!\n");
    return NULL;
  }

  while(fscanf(fp_restore,"%d %*f %*f\n",&idx) == 1)
    ret->max_index = max(idx,ret->max_index);

  rewind(fp_restore);


  ret->feature_max = (double *)calloc(1,(ret->max_index+1)* sizeof(double));
  ret->feature_min = (double *)calloc(1,(ret->max_index+1)* sizeof(double));
  ret->feature_min[0] = 0.0;
  ret->feature_max[0] = 0.0;

  if((c = fgetc(fp_restore)) == 'y')
  {
    fscanf(fp_restore, "%lf %lf\n", &(ret->y_lower), &(ret->y_upper));
    fscanf(fp_restore, "%lf %lf\n", &(ret->y_min), &(ret->y_max));
    ret->y_scaling = 1;
  }
  else {
    ungetc(c, fp_restore);
    ret->y_scaling = 0;
  }

  ret->lower = -1.0;
  ret->upper = 1.0;

  if (fgetc(fp_restore) == 'x') {
     fscanf(fp_restore, "%lf %lf\n", &(ret->lower), &(ret->upper));
     while(fscanf(fp_restore,"%d %lf %lf\n",&idx,&fmin,&fmax)==3)
     {
       if(idx<=ret->max_index)
       {
         ret->feature_min[idx] = fmin;
         ret->feature_max[idx] = fmax;
       }
     }
  }
  fclose(fp_restore);

  return ret;
}

void svm_destroy_scale(struct svm_scale *scale)
{
  if (scale) {
    if (scale->feature_min) free(scale->feature_min);
    if (scale->feature_max) free(scale->feature_max);
    free(scale);
  }
}

void svm_apply_scale(struct svm_scale *scale, struct svm_node * x)
{
  int i=0;
  if (scale == NULL) return;
  if (x == NULL) return;

  while(1)
  {
    if (x[i].index == -1) break;

    if (x[i].index <= scale->max_index)
    {
      int index = x[i].index;
      double value = x[i].value;

      /* skip single-valued attribute */
      if(scale->feature_max[index] == scale->feature_min[index])
        { i++; continue; }

      if(value == scale->feature_min[index])
        value = scale->lower;
      else if(value == scale->feature_max[index])
		value = scale->upper;
      else
		value = scale->lower + (scale->upper - scale->lower) *
			(value- scale->feature_min[index])/
			(scale->feature_max[index] - scale->feature_min[index]);

      x[i].value = value;

    }
    i++;
  }

}


/**************************************************************************/

