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

inherit this class, if you want custom handling of classifier output values..

*/


#ifndef __CLIBSVMLIVESINK_HPP
#define __CLIBSVMLIVESINK_HPP

#include <smileCommon.hpp>
#include <smileComponent.hpp>
#include <dataSink.hpp>
#include <svm/svm.h>

#define COMPONENT_DESCRIPTION_CLIBSVMLIVESINK "classifies data from dataMemory directly using the LibSVM library"
#define COMPONENT_NAME_CLIBSVMLIVESINK "cLibsvmLiveSink"

/**************************************************************************/
/*********              LibSVM   addon:   scale functions  ****************/
/**************************************************************************/

struct svm_scale {
  int max_index;
  int y_scaling;
  double lower,upper;
  double y_lower,y_upper;
  double y_min,y_max;
  double *feature_max;
  double *feature_min;
};
struct svm_scale * svm_load_scale(const char* restore_filename);
void svm_destroy_scale(struct svm_scale *scale);
void svm_apply_scale(struct svm_scale *scale, struct svm_node * x);

/**************************************************************************/

typedef struct{
   long n;
   char **names;
} sOutputSelectionStr;  // list of names of selected features (more flexible..)
typedef sOutputSelectionStr *pOutputSelectionStr;

typedef struct{
   long nFull;   // n here is the full length of the "unselected" feature vector
   long nSel;
   long *enabled;  // flag : 0 or 1  when feature is disabled/enabled respectively
} sOutputSelectionIdx;  
typedef sOutputSelectionIdx *pOutputSelectionIdx;

class cLibsvmLiveSink : public cDataSink {
  private:
    int sendResult;
    int predictProbability, lag;
    const char * resultRecp;
    const char * resultMessageName;
    const char *fselection, *classes;
    const char *modelfile, *scalefile;
    struct svm_model* model;
    struct svm_scale* scale;
    int nClasses, svmType;
    double *probEstimates;
//    char *labels;
    int *labels;
  
    long nCls;
    char ** classNames;

    int fselType; long Nsel;
    sOutputSelectionStr outputSelStr;
    sOutputSelectionIdx outputSelIdx;

    int loadSelection( const char *selFile );
    int buildEnabledSelFromNames(long N, const FrameMetaInfo *fmeta);
    int loadClasses( const char *file );

  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    int printResult;

    virtual void fetchConfig();
    //virtual int myConfigureInstance();
    virtual int myFinaliseInstance();
    virtual int myTick(long long t);

    virtual void processResult(long long tick, long frameIdx, double time, float res, double *probEstim, int nClasses, double dur=0.0);

  public:
    //static sComponentInfo * registerComponent(cConfigManager *_confman);
    //static cSmileComponent * create(const char *_instname);
    SMILECOMPONENT_STATIC_DECL
    
    cLibsvmLiveSink(const char *_name);

    virtual ~cLibsvmLiveSink();
};




#endif // __CLIBSVMLIVESINK_HPP
