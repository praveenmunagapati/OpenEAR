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

simple silence threshold based turn detector

*/


#include <turnDetector.hpp>

#define MODULE "cTurnDetector"

// default values (can be changed via config file...)
#define N_PRE  10
#define N_POST 20

SMILECOMPONENT_STATICS(cTurnDetector)

SMILECOMPONENT_REGCOMP(cTurnDetector)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CTURNDETECTOR;
  sdescription = COMPONENT_DESCRIPTION_CTURNDETECTOR;

  // we inherit cVectorProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("threshold","silence threshold to use",0.001);
    ct->setField("autoThreshold","1=automatically adjust threshold",1);
    ct->setField("minmaxDecay","decay constant used for min/max values in auto-thresholder (a larger value means a slower recovery from loud sounds)",0.9995);
  //TODO: these should be given in seconds and they should be converted to frames via period or via secToVidx..?
    ct->setField("nPre","number of frames > threshold until a turn start is detected",N_PRE);
    ct->setField("nPost","number of frames < threshold until a turn end is detected",N_POST);
  //---  
    ct->setField("useRMS","use rms energy instead of log energy",1);
    ct->setField("idx","index of RMS energy field to use (-1 for auto index)",-1);
    ct->setField("messageRecp","(cWinToVecProcessor type) component(s) to send 'frameTime' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages",(const char *) NULL);
    ct->setField("eventRecp","component(s) to send 'turnStart/turnEnd' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages",(const char *) NULL);
    ct->setField("statusRecp","component(s) to send 'turnSpeakingStatus' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages",(const char *) NULL);
  )
  SMILECOMPONENT_MAKEINFO(cTurnDetector);
}

SMILECOMPONENT_CREATE(cTurnDetector)

//-----

cTurnDetector::cTurnDetector(const char *_name) :
  cDataProcessor(_name),
  turnState(0), actState(0),
  useRMS(1),
  threshold(0.0),
  nPost(N_POST),
  nPre(N_PRE),
  cnt1(0), cnt2(0),
  startP(0),
  recFramer(NULL),
  recComp(NULL),
  statusRecp(NULL),
  rmsIdx(-1),
  autoRmsIdx(-1),
  autoThreshold(0),
  minmaxDecay(0.9995),
  nmin(0), nmax(0),
  rmin(1.0), rmax(0.0), rmaxlast(0.0),
  tmpmin(0.0), tmpmax(0.0),
  dtMeanO(0.0), dtMeanAll(0.0), dtMeanT(0.0), dtMeanS(0.0),
  nE(0.0),
  nTurn(0), nSil(0),
  alphaM(0.9999),
  calCnt(0)
{
}

void cTurnDetector::fetchConfig()
{
  cDataProcessor::fetchConfig();
  
  useRMS = getInt("useRMS");
  SMILE_IDBG(2,"useRMS = %i",useRMS);

  nPre = getInt("nPre");
  SMILE_IDBG(2,"nPre = %i",nPre);

  nPost = getInt("nPost");
  SMILE_IDBG(2,"nPost = %i",nPost);

  threshold = (FLOAT_DMEM)getDouble("threshold");
  if ((useRMS)&&(threshold<(FLOAT_DMEM)0.0)) { threshold = (FLOAT_DMEM)0.001; }
  SMILE_IDBG(2,"threshold = %f",threshold);

  autoThreshold = getInt("autoThreshold");
  SMILE_IDBG(2,"autoThreshold = %i",autoThreshold);

  rmsIdx = getInt("idx");
  SMILE_IDBG(2,"idx = %i",rmsIdx);

  recFramer = getStr("messageRecp");
  SMILE_IDBG(2,"messageRecp = %i",recFramer);

  recComp = getStr("eventRecp");
  SMILE_IDBG(2,"eventRecp = %i",recComp);

  statusRecp = getStr("statusRecp");
  SMILE_IDBG(2,"statusRecp = %i",statusRecp);

}

int cTurnDetector::setupNewNames(long nEl)
{
  writer->addField( "isTurn" );
  namesAreSet = 1;

  return 1;
}

/*
int cTurnDetector::myFinaliseInstance()
{
  writer->addField( "isTurn" );
  namesAreSet = 1;

  return cDataProcessor::myFinaliseInstance();
}
*/

void cTurnDetector::updateThreshold(FLOAT_DMEM eRmsCurrent)
{
  // compute various statistics on-line:

  // min "percentile" (robust min.)
  // "value below which Nmin values are.."
  if (eRmsCurrent < rmin) {
    nmin++;
    tmpmin += eRmsCurrent;
    if (nmin > 10) {
      rmin = tmpmin / (double)nmin;
      //SMILE_IDBG(2,"adjusted rmin: %f",rmin);
      nmin = 0;
      tmpmin = 0;
    }
  }

  // auto decay..
  rmin *= 1.0 + (1.0 - minmaxDecay);

  // max "percentile" (robust max.)
  if (eRmsCurrent > rmax) {
    nmax++;
    tmpmax += eRmsCurrent;
    if (nmax > 10) {
      rmaxlast = rmax;
      rmax = tmpmax / (double)nmax;
      //SMILE_IDBG(2,"adjusted rmax: %f",rmax);
      nmax = 0;
      tmpmax = 0.0;
    }
  }

  // auto decay..
  rmax *= minmaxDecay;

  //}

  // mean overall (exponential decay)
  dtMeanO = minmaxDecay * (dtMeanO - eRmsCurrent) + eRmsCurrent;

  // mean overall, non decaying
  dtMeanAll = (dtMeanAll * nE + eRmsCurrent) / (nE+1.0);
  nE+=1.0;

  // mean of turns

  if (turnState) {
    nTurn++;
    dtMeanT = alphaM * (dtMeanT - eRmsCurrent) + eRmsCurrent;
  } else {
    nSil++;
  // mean of non-turns
    dtMeanS = alphaM * (dtMeanS - eRmsCurrent) + eRmsCurrent;
  }


  // update threshold based on collected statistics:
  FLOAT_DMEM newThresh;
/*  if (dtMeanT == 0.0) {
    //newThresh = ( rmax + (rmax - dtMeanS) ) * 1.1;
  } else {
    //newThresh = 0.5*(dtMeanT + ( rmax + (rmax - dtMeanS) )*1.1) ;
    newThresh = MAX( 0.5*(dtMeanT + rmaxlast ), rmaxlast * 1.41) ;
  }*/

  if (nTurn == 0) { newThresh = dtMeanAll * 2.0; }
  else {
    newThresh = 0.15 * ( rmax + rmin ) ;
    FLOAT_DMEM w = (FLOAT_DMEM)nTurn / ( (FLOAT_DMEM)nSil + (FLOAT_DMEM)nTurn);
    FLOAT_DMEM w2 = sqrt(1.0-w)+1.0;
    if (dtMeanO < newThresh) { newThresh = w*newThresh + (1.0-w)*MAX(dtMeanO*w2, 1.2*w2*dtMeanAll) ; }
  }
  

  threshold = 0.8*threshold + 0.2*newThresh;

#ifdef DEBUG
  tmpCnt++;
  if (tmpCnt>200) {
    SMILE_IDBG(2,"THRESH: %f  rmax %f  rmin %f  dtMeanO %f  dtMeanAll %f",threshold,rmax,rmin,dtMeanO,dtMeanAll);
    //SMILE_IDBG(2,"dtMeanO: %f",dtMeanO);
    //SMILE_IDBG(2,"dtMeanT: %f",dtMeanT);
    //SMILE_IDBG(2,"dtMeanS: %f",dtMeanS);
    tmpCnt = 0;
  }
#endif
}

int cTurnDetector::isVoice(FLOAT_DMEM *src)
{
  if (src[rmsIdx] > threshold) return 1;
  else return 0;
}

// a derived class should override this method, in order to implement the actual processing
int cTurnDetector::myTick(long long t)
{
  // get next frame from dataMemory
  cVector *vec = reader->getNextFrame();
  if (vec == NULL) return 0;
  
  cVector *vec0 = new cVector(1);  // TODO: move vec0 to class...
  
  FLOAT_DMEM *src = vec->dataF;
  FLOAT_DMEM *dst = vec0->dataF;

  if (rmsIdx < 0) {
    if (autoRmsIdx < 0) { // if autoRmsIdx has not been set yet
      autoRmsIdx = vec->fmeta->fieldToElementIdx( vec->fmeta->findFieldByPartialName( "RMS" ) );
      SMILE_IDBG(3,"automatically found RMSenergy field at index %i in input vector",autoRmsIdx);
    }
    rmsIdx = autoRmsIdx;
  }

  // just to be sure we don't exceed arrray bounds...
  if (rmsIdx >= vec->N) rmsIdx = vec->N-1;
  
  //printf("s : %f\n",src[rmsIdx]);
  if (autoThreshold) updateThreshold(src[rmsIdx]);

  //if (src[rmsIdx] > threshold) {
  if (calCnt > 200) {

  if (isVoice(src)) {
    cnt2 = 0;
    if (!turnState) {
      if (cnt1<=0) { 
        if (vec->tmeta != NULL) startSmileTime = vec->tmeta->smileTime; 
      }
      cnt1++;
      if ((cnt1 > 1)&&(!actState)) {
        actState = 1;
        SMILE_IMSG(4,"detected voice activity start at vIdx %i!",startP);
        if (statusRecp!=NULL) {
          cComponentMessage cmsg("turnSpeakingStatus");
          cmsg.intData[0] = 1;
          cmsg.floatData[1] = (double)startP;
          cmsg.floatData[2] = (double)(reader->getLevelT());
          cmsg.userTime1 = startSmileTime;
          sendComponentMessage( statusRecp, &cmsg );
          SMILE_IDBG(4,"sending turnSpeakingStatus (1) message to '%s'",recComp);
        }
      }
      if (cnt1 > nPre) {
        turnState = 1; cnt1 = 0; cnt2 = 0;
        startP = vec->tmeta->vIdx - nPre;
        SMILE_IMSG(3,"detected turn start at vIdx %i!",startP);
        if (recComp!=NULL) {
          cComponentMessage cmsg("turnStart");
          cmsg.floatData[0] = (double)nPre;
          cmsg.floatData[1] = (double)startP;
          cmsg.floatData[2] = (double)(reader->getLevelT());
          cmsg.userTime1 = startSmileTime;
          sendComponentMessage( recComp, &cmsg );
          SMILE_IDBG(3,"sending turnStart message to '%s'",recComp);
        }
      }
    }
  } else {
    cnt1 = 0;
    if (cnt2<=0) { 
      if (vec->tmeta != NULL) endSmileTime = vec->tmeta->smileTime; 
    }
    cnt2++;
    if ((cnt2 > 2)&&(actState)) {
      actState = 0;
      SMILE_IMSG(4,"detected voice activity end at vIdx %i!",startP);
      if (statusRecp!=NULL) {
        cComponentMessage cmsg("turnSpeakingStatus");
        cmsg.intData[0] = 0;
        cmsg.floatData[1] = (double)(vec->tmeta->vIdx - 2);
        cmsg.floatData[2] = (double)(reader->getLevelT());
        cmsg.userTime1 = endSmileTime;
        sendComponentMessage( statusRecp, &cmsg );
        SMILE_IDBG(4,"sending turnSpeakingStatus (0) message to '%s'",recComp);
      }
    }
    if (turnState) {
      if (cnt2 > nPost) {
        turnState = 0; cnt1 = 0; cnt2 = 0;
        SMILE_IMSG(3,"detected turn end at vIdx %i !",(vec->tmeta->vIdx)-nPost);
        if (recFramer!=NULL) {
          cComponentMessage cmsg("turnFrameTime");
          // send start/end in frames of input level
          cmsg.floatData[0] = (double)startP;
          cmsg.floatData[1] = (double)(vec->tmeta->vIdx - nPost);
          double _T = (double)(reader->getLevelT());
          if (_T!=0.0) {
            // also send start/end as actual data time in seconds
            cmsg.floatData[2] = ((double)startP) * _T;
            cmsg.floatData[3] = ((double)(vec->tmeta->vIdx - nPost)) * _T;
            // and send period of input level
            cmsg.floatData[4] = _T;
          }
          cmsg.userTime1 = startSmileTime;
          cmsg.userTime2 = endSmileTime;
          sendComponentMessage( recFramer, &cmsg );
          SMILE_IDBG(3,"sending turnFrameTime message to '%s'",recFramer);
        }
        if (recComp!=NULL) {
          cComponentMessage cmsg("turnEnd");
          cmsg.floatData[0] = (double)nPost;
          cmsg.floatData[1] = (double)(vec->tmeta->vIdx - nPost);
          cmsg.floatData[2] = (double)(reader->getLevelT());
          cmsg.userTime1 = endSmileTime;
          sendComponentMessage( recComp, &cmsg );
          SMILE_IDBG(3,"sending turnEnd message to '%s'",recComp);
        }
      }
    }
  }

  } else { calCnt++; }

  dst[0] = (FLOAT_DMEM)turnState;

  // save to dataMemory
  writer->setNextFrame(vec0);
  delete vec0;

  return 1;
}

cTurnDetector::~cTurnDetector()
{
}

