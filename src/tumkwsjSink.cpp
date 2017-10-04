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


/*
 * Some code in this file is taken from the Julius LVCSR engine, which is copyright by:
 *
 * Copyright (c) 1991-2007 Kawahara Lab., Kyoto University
 * Copyright (c) 2000-2005 Shikano Lab., Nara Institute of Science and Technology
 * Copyright (c) 2005-2007 Julius project team, Nagoya Institute of Technology
 * All rights reserved
 *
 * See the file JULIUS_LICENSE for more details
 *
 */



/*  openSMILE component:

TUM keyword spotter (Julius version)

*/


#include <tumkwsjSink.hpp>

#define MODULE "cTumkwsjSink"

#ifdef HAVE_JULIUSLIB

SMILECOMPONENT_STATICS(cTumkwsjSink)

SMILECOMPONENT_REGCOMP(cTumkwsjSink)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CTUMKWSJSINK;
  sdescription = COMPONENT_DESCRIPTION_CTUMKWSJSINK;

  // we inherit cDataSink configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSink")

    SMILECOMPONENT_IFNOTREGAGAIN_BEGIN
    ct->makeMandatory(ct->setField("configfile","ASR configfile to load","kws.cfg"));
  ct->setField("logfile","Julius logfile (default=(null) : no log file)",(const char*)NULL);
  ct->setField("gprob","garbage log. probability",0.0);
  ct->setField("kprob","keyword log. probability",0.0);
  ct->setField("wlenWeight","weight of word length (prob += wordlenght*wlenWeight)",0.2);
  ct->setField("debug","1 = show julius debug log output, 0 = don't show",0);
  ct->setField("numkw","number of keywords",0);
  ct->setField("preSil","silence period at beginning of turn to keep (in seconds)",0.3);
  ct->setField("postSil","silence period at end of turn to keep (in seconds)",0.4);

  ct->setField("printResult","print output to console (1/0=yes/no)",1);

  ct->setField("resultRecp","component(s) to send 'asrKeywordOutput' messages to (use , to separate multiple recepients), leave blank (NULL) to not send any messages",(const char *) NULL);
  SMILECOMPONENT_IFNOTREGAGAIN_END

    SMILECOMPONENT_MAKEINFO(cTumkwsjSink);
}

SMILECOMPONENT_CREATE(cTumkwsjSink)

//-----

cTumkwsjSink::cTumkwsjSink(const char *_name) :
cDataSink(_name),
configfile(NULL),
logfile(NULL),
printResult(0),
juliusIsSetup(0), juliusIsRunning(0),
numkw(0), glogprob(0.0), klogprob(0.0),
dataFlag(0),
turnEnd(0), turnStart(0), isTurn(0), endWait(-1),
lag(0), nPre(0), nPost(0),
curVidx(0), vIdxStart(0), vIdxEnd(0), wst(0), writelen(0), resultRecp(NULL), period(0.0),
turnStartSmileTimeLast(0.0), turnStartSmileTime(0.0), turnStartSmileTimeCur(0.0),
decoderThread(NULL), wlenWeight(0.2)
{
  smileMutexCreate(terminatedMtx);
  smileMutexCreate(dataFlgMtx);
  smileCondCreate(tickCond);
}

void cTumkwsjSink::fetchConfig()
{
  cDataSink::fetchConfig();

  configfile = getStr("configfile");
  SMILE_IDBG(2,"ASR configfile to load = '%s'",configfile);

  logfile = getStr("logfile");
  SMILE_IDBG(2,"Julius logfile = '%s'",logfile);

  glogprob = getDouble("gprob");
  klogprob = getDouble("kprob");
  numkw = getInt("numkw");

  resultRecp = getStr("resultRecp");
  SMILE_IDBG(2,"resultRecp = '%s'",resultRecp);
  if (resultRecp != NULL) sendResult = 1;

  printResult = getInt("printResult");
  SMILE_IDBG(2,"printResult = %i",printResult);

  wlenWeight = getDouble("wlenWeight");
  SMILE_IDBG(2,"wlenWeight = %f",wlenWeight);
}

/*

static boolean opt_gprob(Jconf *jconf, char *arg[], int argnum)
{
  SMILE_WRN(1,"please do not use -gprob in julius config (it is ignored!), use 'gprob' in openSMILE config instead!");
  return TRUE;
}

static boolean opt_kprob(Jconf *jconf, char *arg[], int argnum)
{
  SMILE_WRN(1,"please do not use -kprob in julius config (it is ignored!), use 'kprob' in openSMILE config instead!");
  return TRUE;
}

static boolean opt_knum(Jconf *jconf, char *arg[], int argnum)
{
  SMILE_WRN(1,"please do not use -knum in julius config file (it is ignored!), use 'numkw' in openSMILE config instead!");
  return TRUE;
}
*/


/********************************** Julius callback loaders ***********************************/

LOGPROB userlm_uni_loader(WORD_INFO *winfo, WORD_ID w, LOGPROB ngram_prob)
{
  cTumkwsjSink * cl = (cTumkwsjSink *)(winfo->userptr);
  if (cl!=NULL) return cl->cbUserlmUni(winfo, w, ngram_prob);
  else return 0.0;
}

LOGPROB userlm_bi_loader(WORD_INFO *winfo, WORD_ID context, WORD_ID w, LOGPROB ngram_prob)
{
  cTumkwsjSink * cl = (cTumkwsjSink *)(winfo->userptr);
  if (cl!=NULL) return cl->cbUserlmBi(winfo, context, w, ngram_prob);
  else return 0.0;
}

LOGPROB userlm_lm_loader(WORD_INFO *winfo, WORD_ID *contexts, int clen, WORD_ID w, LOGPROB ngram_prob)
{
  cTumkwsjSink * cl = (cTumkwsjSink *)(winfo->userptr);
  if (cl!=NULL) return cl->cbUserlmLm(winfo, contexts, clen, w, ngram_prob);
  else return 0.0;
}


int external_fv_read_loader(void * cl, float *vec, int n) 
{
  // retval -2 : error
  // retval -3 : segment input
  // retval -1 : end of input
  if (cl!=NULL) return ((cTumkwsjSink*)cl)->getFv(vec, n);
  else return -1; 
}


static void result_pass2_loader(Recog *recog, void *dummy)
{
  cTumkwsjSink * cl = (cTumkwsjSink *)(recog->hook);
  if (cl!=NULL) cl->cbResultPass2(recog, dummy);
}



static void status_pass1_begin_loader(Recog *recog, void *dummy)
{
  cTumkwsjSink * cl = (cTumkwsjSink *)(recog->hook);
  if (cl!=NULL) cl->cbStatusPass1Begin(recog, dummy);
}

static void result_pass1_loader(Recog *recog, void *dummy)
{
  cTumkwsjSink * cl = (cTumkwsjSink *)(recog->hook);
  if (cl!=NULL) cl->cbResultPass1(recog, dummy);
}

void result_pass1_current_loader(Recog *recog, void *dummy)
{
  cTumkwsjSink * cl = (cTumkwsjSink *)(recog->hook);
  if (cl!=NULL) cl->cbResultPass1Current(recog, dummy);
}



/********************************** Julius output formatting functions ******************************/

void cTumkwsjSink::juAlignPass1Keywords(RecogProcess *r, HTK_Param *param)
{
  int n;
  Sentence *s;
  SentenceAlign *now;//, *prev;


  s = &(r->result.pass1);
  /* do forced alignment if needed */
  if (r->config->annotate.align_result_word_flag) {
    now = result_align_new();
    //if (s->align == NULL) now = result_align_new(); else now = s->align;
    word_align(s->word, s->word_num, param, now, r);
    if (s->align != NULL) result_align_free(s->align);
    s->align = now;
//    else prev->next = now;
//    prev = now;
  }
} 


void cTumkwsjSink::juPutHypoPhoneme(WORD_ID *seq, int n, WORD_INFO *winfo)
{
  int i,j;
  WORD_ID w;
  static char buf[MAX_HMMNAME_LEN];

  if (seq != NULL) {
    for (i=0;i<n;i++) {
      if (i > 0) printf(" |");
      w = seq[i];
      for (j=0;j<winfo->wlen[w];j++) {
        center_name(winfo->wseq[w][j]->name, buf);
        printf(" %s", buf);
      }
    }
  }
  printf("\n");  
}


void cTumkwsjSink::fillKresult(Kresult *k, WORD_ID *seq, int n, WORD_INFO *winfo, LOGPROB *cmscore, int numOfFrames, SentenceAlign *alignment)
{
  int i;
  int kwcount = 0;

  if (seq != NULL) {
    for (i=0;i<n;i++) {
      if (strcmp("garbage",winfo->woutput[seq[i]])!=0 && strcmp("<s>",winfo->woutput[seq[i]])!=0 && strcmp("</s>",winfo->woutput[seq[i]])!=0) {
        //keywords:
        k->keyword[kwcount]=winfo->woutput[seq[i]];
        k->kwStartTime[kwcount]= (period * (double)(alignment->begin_frame[i]));
        k->kwConf[kwcount]=cmscore[i];
        kwcount++;
        if (kwcount >= MAXNUMKW) {
          SMILE_IWRN(2,"keywords were discarded, more than MAXNUMKW=%i (tumkwsjSink.hpp!) keywords detected!",MAXNUMKW);  
          i=n; break; 
        }
      }
    }
  }
  k->numOfKw=kwcount;
  k->turnDuration=numOfFrames;
}



/************************************ Callback handlers ************************************************/


LOGPROB cTumkwsjSink::cbUserlmLm(WORD_INFO *winfo, WORD_ID *contexts, int clen, WORD_ID w, LOGPROB ngram_prob)
{
  float prob = klogprob;
  float wordlength = 0.0;
  if(w>numkw+1) {
    prob=glogprob;
  }	
  wordlength = winfo->wlen[w];
  prob = prob+(wordlength*wlenWeight);
  if(w==1) {
    prob=2;
  }
  return prob;
}

LOGPROB cTumkwsjSink::cbUserlmUni(WORD_INFO *winfo, WORD_ID w, LOGPROB ngram_prob)
{
  float prob = klogprob;
  float wordlength = 0.0;
  if(w>numkw+1) {
    prob=glogprob;
  }	
  wordlength = winfo->wlen[w];
  prob = prob+(wordlength*wlenWeight);
  if(w==1) {
    prob=2;
  }
  return prob;
}

LOGPROB cTumkwsjSink::cbUserlmBi(WORD_INFO *winfo, WORD_ID context, WORD_ID w, LOGPROB ngram_prob)
{
  float prob = klogprob;
  float wordlength = 0.0;
  if(w>numkw+1) {
    prob=glogprob;
  }	
  wordlength = winfo->wlen[w];
  prob = prob+(wordlength*wlenWeight);
  if(w==1) {
    prob=2;
  }
  return prob;
}



void cTumkwsjSink::cbStatusPass1Begin(Recog *recog, void *dummy)
{
  if (!recog->jconf->decodeopt.realtime_flag) {
    VERMES("### Recognition: 1st pass (LR beam)\n");
  }
  wst = 0;
}



void cTumkwsjSink::cbResultPass1Current(Recog *recog, void *dummy)
{
  int i, j, bgn;
  int len, num;
  WORD_INFO *winfo;
  WORD_ID *seq;
  RecogProcess *r;
  Sentence *s;
  r=recog->process_list;
  if (! r->live) return;
  if (! r->have_interim) return;

  winfo = r->lm->winfo;
  seq = r->result.pass1.word;
  num = r->result.pass1.word_num;
  s = &(r->result.pass1);
  if (num>0) {
    juAlignPass1Keywords(r, r->am->mfcc->param);

    //create smile message:
    Kresult k;
    fillKresult(&k, seq, num, winfo, s->confidence, r->result.num_frame, s->align);
    if (resultRecp != NULL) {
      cComponentMessage msg("asrKeywordOutput");
      msg.custData = &k;
      msg.userTime1 = turnStartSmileTimeCur;
      msg.userTime2 = turnStartSmileTimeCur + ((double)(k.turnDuration))*period;
      sendComponentMessage( resultRecp, &msg );
      SMILE_IDBG(3,"sending 'asrKeywordOutput' message to '%s'",resultRecp);
    }
   //output content of k:
    int kc = 0;
    printf("----------current hypothesis:------------\n");
    printf("numOfKw:%i\n",k.numOfKw);
    printf("turnDuration:%i\n",k.turnDuration);
    printf("keywords: ");
    for (kc=0;kc<(k.numOfKw);kc++) {
      printf("%s ",k.keyword[kc]);
    }
    printf("\n");
    printf("kwConf: ");
    for (kc=0;kc<(k.numOfKw);kc++) {
      printf("%5.3f ",k.kwConf[kc]);
    }
    printf("\n");
    printf("kwStartTimes: ");
    for (kc=0;kc<(k.numOfKw);kc++) {
      printf("%.3f ",k.kwStartTime[kc]);
    }
    printf("\n");
    printf("-----------------------------------------\n");

    printf("\n\n");
    fflush(stdout);

  }
}




void cTumkwsjSink::cbResultPass1(Recog *recog, void *dummy)
{
  int i,j;
  static char buf[MAX_HMMNAME_LEN];
  WORD_INFO *winfo;
  WORD_ID *seq;
  int num;
  RecogProcess *r;
  Sentence *s;
  boolean multi;
  int len;
  boolean have_progout = TRUE;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  r=recog->process_list;
  if (! r->live) return;
  if (r->result.status < 0) return;	/* search already failed  */
  if (have_progout && r->config->successive.enabled) return; /* short pause segmentation */
  if (r->config->output.progout_flag) printf("\r");

  winfo = r->lm->winfo;
  seq = r->result.pass1.word;
  num = r->result.pass1.word_num;
  s = &(r->result.pass1);
  /* words */


  if (verbose_flag) {		/* output further info */
    /* phoneme sequence */
    printf("p1_phon:");
    for (i=0;i<num;i++) {
      for (j=0;j<winfo->wlen[seq[i]];j++) {
        center_name(winfo->wseq[seq[i]][j]->name, buf);
        printf(" %s", buf);
      }
      if (i < num-1) printf(" |");
    }
    printf("\n");
    if (debug2_flag) {
    /* logical HMMs */
      printf("pass1_best_HMMseq_logical:");
      for (i=0;i<num;i++) {
        for (j=0;j<winfo->wlen[seq[i]];j++) {
          printf(" %s", winfo->wseq[seq[i]][j]->name);
        }
        if (i < num-1) printf(" |");
      }
      printf("\n");
    }
  }

  juAlignPass1Keywords(r, r->am->mfcc->param);

  //create smile message:
  Kresult k;
  fillKresult(&k, seq, num, winfo, s->confidence, r->result.num_frame, s->align);
  if (resultRecp != NULL) {
    cComponentMessage msg("asrKeywordOutput");
    msg.custData = &k;
    msg.userTime1 = turnStartSmileTimeCur;
    msg.userTime2 = turnStartSmileTimeCur + ((double)(k.turnDuration))*period;
    sendComponentMessage( resultRecp, &msg );
    SMILE_IDBG(3,"sending 'asrKeywordOutput' message to '%s'",resultRecp);
  }
 //output content of k:
  int kc = 0;
  printf("-------result package (pass 1):----------\n");
  printf("numOfKw:%i\n",k.numOfKw);
  printf("turnDuration:%i\n",k.turnDuration);
  printf("keywords: ");
  for (kc=0;kc<(k.numOfKw);kc++) {
    printf("%s ",k.keyword[kc]);
  }
  printf("\n");
  printf("kwConf: ");
  for (kc=0;kc<(k.numOfKw);kc++) {
    printf("%5.3f ",k.kwConf[kc]);
  }
  printf("\n");
  printf("kwStartTimes: ");
  for (kc=0;kc<(k.numOfKw);kc++) {
    printf("%.3f ",k.kwStartTime[kc]);
  }
  printf("\n");
  printf("-----------------------------------------\n");

  printf("\n\n");
  fflush(stdout);


}





void cTumkwsjSink::cbResultPass2(Recog *recog, void *dummy)
{
  int i, j;
  int len;
  char ec[5] = {0x1b, '[', '1', 'm', 0};
  WORD_INFO *winfo;
  WORD_ID *seq;
  int seqnum;
  int n, num;
  Sentence *s;
  RecogProcess *r;
  boolean multi;
  HMM_Logical *p;
  SentenceAlign *align;

  if (recog->process_list->next != NULL) multi = TRUE;
  else multi = FALSE;

  r=recog->process_list;
  if (! r->live) return;
  if (multi) printf("[#%d %s]\n", r->config->id, r->config->name);

  if (r->result.status < 0) {
    switch(r->result.status) {
    case J_RESULT_STATUS_REJECT_POWER:
      printf("<input rejected by power>\n");
      break;
    case J_RESULT_STATUS_TERMINATE:
      printf("<input teminated by request>\n");
      break;
    case J_RESULT_STATUS_ONLY_SILENCE:
      printf("<input rejected by decoder (silence input result)>\n");
      break;
    case J_RESULT_STATUS_REJECT_GMM:
      printf("<input rejected by GMM>\n");
      break;
    case J_RESULT_STATUS_REJECT_SHORT:
      printf("<input rejected by short input>\n");
      break;
    case J_RESULT_STATUS_FAIL:
      printf("<search failed>\n");
      break;
    }
    return;
  }

  winfo = r->lm->winfo;
//  num = r->result.sentnum;
//assume just one sentence:
  num = 1;
//consider just sentence with index n=0
  n = 0;
  s = &(r->result.sent[n]);
  seq = s->word;
  seqnum = s->word_num;
  if (debug2_flag) {
    printf("\n%s",ec);		/* newline & bold on */
  }
  if (verbose_flag) {
    printf("p2_phon:");
    juPutHypoPhoneme(seq, seqnum, winfo);
  }
  if (debug2_flag) {
    ec[2] = '0';
    printf("%s\n",ec);		/* bold off & newline */
  }
  if (verbose_flag) {
    if (r->lmtype == LM_DFA) {
  /* output which grammar the hypothesis belongs to on multiple grammar */
  /* determine only by the last word */
      if (multigram_get_all_num(r->lm) > 1) {
	printf("grammar%d: %d\n", n+1, s->gram_id);
      }
    }
  }
  //create smile message:
  Kresult k;
  fillKresult(&k, seq, seqnum, winfo, s->confidence, r->result.num_frame, s->align);
  if (resultRecp != NULL) {
    cComponentMessage msg("asrKeywordOutput");
    msg.custData = &k;
    msg.userTime1 = turnStartSmileTimeCur;
    msg.userTime2 = turnStartSmileTimeCur + ((double)(k.turnDuration))*period;
    sendComponentMessage( resultRecp, &msg );
    SMILE_IDBG(3,"sending 'asrKeywordOutput' message to '%s'",resultRecp);
  }
  //output content of k:
  int kc = 0;
  printf("-------result package (pass 2):----------\n");
  printf("numOfKw:%i\n",k.numOfKw);
  printf("turnDuration:%i\n",k.turnDuration);
  printf("keywords: ");
  for (kc=0;kc<(k.numOfKw);kc++) {
    printf("%s ",k.keyword[kc]);
  }
  printf("\n");
  printf("kwConf: ");
  for (kc=0;kc<(k.numOfKw);kc++) {
    printf("%5.3f ",k.kwConf[kc]);
  }
  printf("\n");
  printf("kwStartTimes: ");
  for (kc=0;kc<(k.numOfKw);kc++) {
    printf("%.3f ",k.kwStartTime[kc]);
  }
  printf("\n");
  printf("-----------------------------------------\n");

  printf("\n\n\n");
  fflush(stdout);
}




void cTumkwsjSink::setupCallbacks(Recog *recog, void *data)
{
  //  callback_add(recog, CALLBACK_EVENT_PROCESS_ONLINE, status_process_online, data);
  //  callback_add(recog, CALLBACK_EVENT_PROCESS_OFFLINE, status_process_offline, data);
  //  callback_add(recog, CALLBACK_EVENT_SPEECH_READY, status_recready, data);
  //  callback_add(recog, CALLBACK_EVENT_SPEECH_START, status_recstart, data);
  //  callback_add(recog, CALLBACK_EVENT_SPEECH_STOP, status_recend, data);
  //  callback_add(recog, CALLBACK_EVENT_RECOGNITION_BEGIN, status_recognition_begin, data);
  //  callback_add(recog, CALLBACK_EVENT_RECOGNITION_END, status_recognition_end, data);
  //  if (recog->jconf->decodeopt.segment) { /* short pause segmentation */
  //    callback_add(recog, CALLBACK_EVENT_SEGMENT_BEGIN, status_segment_begin, data);
  //    callback_add(recog, CALLBACK_EVENT_SEGMENT_END, status_segment_end, data);
  //  }

  callback_add(recog, CALLBACK_EVENT_PASS1_BEGIN, status_pass1_begin_loader, data); //sets wst=0

  //  {
  //    JCONF_SEARCH *s;
  //    boolean ok_p;
  //    ok_p = TRUE;
  //    for(s=recog->jconf->search_root;s;s=s->next) {
  //      if (s->output.progout_flag) ok_p = FALSE;
  //    }
  //    if (ok_p) {      
  //      have_progout = FALSE;
  //    } else {
  //      have_progout = TRUE;
  //    }
  //  }
  //  if (!recog->jconf->decodeopt.realtime_flag && verbose_flag && ! have_progout) {
  //    callback_add(recog, CALLBACK_EVENT_PASS1_FRAME, frame_indicator, data);
  //  }

  callback_add(recog, CALLBACK_RESULT_PASS1_INTERIM, result_pass1_current_loader, data);
  callback_add(recog, CALLBACK_RESULT_PASS1, result_pass1_loader, data);

  //#ifdef WORD_GRAPH
  //  callback_add(recog, CALLBACK_RESULT_PASS1_GRAPH, result_pass1_graph, data);
  //#endif
  //  callback_add(recog, CALLBACK_EVENT_PASS1_END, status_pass1_end, data);
  //  callback_add(recog, CALLBACK_STATUS_PARAM, status_param, data);
  //  callback_add(recog, CALLBACK_EVENT_PASS2_BEGIN, status_pass2_begin, data);
  //  callback_add(recog, CALLBACK_EVENT_PASS2_END, status_pass2_end, data);

  callback_add(recog, CALLBACK_RESULT, result_pass2_loader, data); // rejected, failed

  //  callback_add(recog, CALLBACK_RESULT_GMM, result_gmm, data);
  //  /* below will be called when "-lattice" is specified */
  //  callback_add(recog, CALLBACK_RESULT_GRAPH, result_graph, data);
  //  /* below will be called when "-confnet" is specified */
  //  callback_add(recog, CALLBACK_RESULT_CONFNET, result_confnet, data);
  //
  //  //callback_add_adin(CALLBACK_ADIN_CAPTURED, levelmeter, data);
}






int cTumkwsjSink::setupJulius()
{
  try {

    int argc=3;
    char* argv[3] = {"julius","-C",NULL};
    if (configfile != NULL)
      argv[2]=strdup(configfile);
    else
      argv[2]=strdup("kws.cfg");

    /* add application option dummies */
    /*
    j_add_option("-gprob", 1, 1, "garbage probability", opt_gprob);
    j_add_option("-kprob", 1, 1, "keyword probability", opt_kprob);
    j_add_option("-knum", 1, 1, "number of keywords", opt_knum);
    */  

    /* create a configuration variables container */
    jconf = j_jconf_new();
    //    j_config_load_file(jconf, strdup(configfile));
    if (j_config_load_args(jconf, argc, argv) == -1) {
      COMP_ERR("error parsing julius decoder options, this might be a bug. see tumkwsjSink.cpp!");
      j_jconf_free(jconf);
      free(argv[2]);
      return 0;
    }
    //free(argv[2]);

    /* output system log to a file */
    if (getInt("debug") != 1) {
      jlog_set_output(NULL);
    }

    /* here you can set/modify any parameter in the jconf before setup */
    jconf->input.type = INPUT_VECTOR;
    jconf->input.speech_input = SP_EXTERN;
    jconf->decodeopt.realtime_flag = TRUE; // ??? 
    jconf->ext.period = (float)(reader->getLevelT());
    jconf->ext.veclen = reader->getLevelN();
    jconf->ext.userptr = (void *)this;
    jconf->ext.fv_read = external_fv_read_loader;

    /* Fixate jconf parameters: it checks whether the jconf parameters
    are suitable for recognition or not, and set some internal
    parameters according to the values for recognition.  Modifying
    a value in jconf after this function may be errorous.
    */
    if (j_jconf_finalize(jconf) == FALSE) {
      SMILE_IERR(1,"error finalising julius jconf in setupJulius()!");
      j_jconf_free(jconf);
      return 0;
    }

    /* create a recognition instance */
    recog = j_recog_new();
    /* use user-definable data hook to set a pointer to our class instance */
    recog->hook = (void *)this;
    /* assign configuration to the instance */
    recog->jconf = jconf;
    /* load all files according to the configurations */
    if (j_load_all(recog, jconf) == FALSE) {
      SMILE_IERR(1, "Error in loading model for Julius decoder");
      j_recog_free(recog);
      return 0;
    }

    SMILE_IMSG(2,"garbage prob.: %f",glogprob);
    SMILE_IMSG(2,"keyword prob.: %f",klogprob);
    SMILE_IMSG(2,"number of keywords: %i",numkw);

    PROCESS_LM *lm;
    for(lm=recog->lmlist;lm;lm=lm->next) {
      if (lm->lmtype == LM_PROB) {
        lm->winfo->userptr = (void*)this;  // PATCH: sent/vocabulary.h (WORD_INFO struct): ++   void * userptr;   // Pointer to userdata....
        j_regist_user_lm_func(lm, userlm_uni_loader, userlm_bi_loader, userlm_lm_loader);
      }
    }


    /* checkout for recognition: build lexicon tree, allocate cache */
    if (j_final_fusion(recog) == FALSE) {
      fprintf(stderr, "ERROR: Error while setup work area for recognition\n");
      j_recog_free(recog);
      if (logfile) fclose(fp);
      return 0;
    }

    setupCallbacks(recog, NULL);

    /* output system information to log */
    j_recog_info(recog);

    terminated = FALSE;

  }
  catch (int) { }

  juliusIsSetup=1;

  return 1;
}

int cTumkwsjSink::myFinaliseInstance()
{
  int ret = cDataSink::myFinaliseInstance();
  if (ret==0) return 0;

//  int ap=0;

  // setup pre/post silence config:
  float _preSil = getDouble("preSil");
  float _postSil = getDouble("postSil");
  double _T = reader->getLevelT();
  if (_T!=0.0) preSil = (int)ceil(_preSil/_T);
  else preSil = (int)(_preSil);
  if (_T!=0.0) postSil = (int)ceil(_postSil/_T);
  else postSil = (int)(_postSil);

  period = _T;

  // setup julius, if not already set up
  if (!juliusIsSetup) ret *= setupJulius();

  return ret;
}

/*
void cTumkwsjSink::processResult(long long tick, long frameIdx, double time)
{
  if (printResult) {
    //p->Show();
  }
}
*/

int cTumkwsjSink::processComponentMessage( cComponentMessage *_msg )
{
  if (isMessageType(_msg,"turnEnd")) {
    turnEnd=1;
    nPost = (long)(_msg->floatData[0]);
    vIdxEnd = (long)(_msg->floatData[1]);
    turnStartSmileTimeLast = turnStartSmileTime;
    return 1;
  }
  if (isMessageType(_msg,"turnStart")) {
    turnStart=1;
    nPre = (long)(_msg->floatData[0]);
    vIdxStart = (long)(_msg->floatData[1]);
    turnStartSmileTime = _msg->userTime1;
    return 1;
  }
  return 0;
}

// this is called from julius decoder thread... 
int cTumkwsjSink::getFv(float *vec, int n)
{ 
  int ret=0;

  smileMutexLock(dataFlgMtx);

  if (terminated) { 
    smileMutexUnlock(dataFlgMtx);  
    recog->process_want_terminate = TRUE;
    return -1; 
  }  

  // we should wait for main thread to provide data, then set a flag that data was read..
  SMILE_IDBG(4,"yes... julius requests features from us (n=%i)!",n);

  int end=0;
  do {

    lockMessageMemory();

    turnStartSmileTimeCur = turnStartSmileTime;

    // handle pre/post silence and turn detector interface
    if (turnStart) { 
      // process old turnEnd message first...
      if ((turnEnd)&&(vIdxEnd < vIdxStart)) { 
        turnEnd = 0; isTurn = 0; ret=-3; end=1; 
        turnStartSmileTimeCur = turnStartSmileTimeLast;
        SMILE_IDBG(2,"processed turn end message!");  
      }
      turnStart = 0; 
      curVidx = vIdxStart-preSil;
      isTurn = 1;
      SMILE_IDBG(2,"received turn start message!"); 
    }
    if (turnEnd) { 
      SMILE_IDBG(4,"received turn end message!"); 
      if (curVidx >= vIdxEnd+postSil) { 
        turnEnd = 0; isTurn = 0; ret=-3; end=1; SMILE_IDBG(2,"processed turn end message!");  
        turnStartSmileTimeCur = turnStartSmileTimeLast;
      }
      // if no frames have been written...
      if (curVidx == vIdxStart) { 
        SMILE_IERR(1,"turn #%i has 0 frames!");
        turnEnd=0; isTurn=0;
      }
    }

    unlockMessageMemory();

    if (!isTurn) { 
      smileCondWaitWMtx( tickCond, dataFlgMtx ); // wait one smile tick for things to change
    }

    if (terminated) { 
      smileMutexUnlock(dataFlgMtx);  
      recog->process_want_terminate = TRUE;
      return -1; 
    }  

  } while ((!isTurn)&&(!end));

  //printf("turn getFv\n");

  if (isTurn) {
    int result=0;
    curVec = NULL;
    while (curVec == NULL)  {
      curVec = reader->getFrame(curVidx, -1, 0, &result);
      // TODO: check if curVidx is behind ringbuffer storage space!!
      if (result & DMRES_OORleft) {
        long tmp = reader->getMinR()+10;
        if (tmp > curVidx) {
          SMILE_IERR(1,"ASR processing tried to get feature data behind beginning of ringbuffer, which is no more available! Skipping a few frames... (%i) : %i -> %i",tmp-curVidx, curVidx, tmp);
          curVidx = tmp;
        }
      }
      if (curVec == NULL) { 
        smileCondWaitWMtx( tickCond, dataFlgMtx );  

        if (terminated) { 
          smileMutexUnlock(dataFlgMtx);  
          recog->process_want_terminate = TRUE;
          return -1; 
        }  
      }
    }
    curVidx++;
  }

  //printf("turn: %f n=%i\n",curVec->dataF[curVec->N-1],curVec->N);
  smileMutexUnlock(dataFlgMtx);

  int i;
  if (curVec != NULL) {
    for (i=0; i<MIN( curVec->N, n ); i++) {
      vec[i] = (float)(curVec->dataF[i]);
    }
  } else {
    for (i=0; i<n; i++) {
      vec[i] = 0.0;
    }
  }

  // TODO: input segmentation (via smile ComponentMessages...??)

  return ret;
}


SMILE_THREAD_RETVAL juliusThreadRunner(void *_obj)
{
  cTumkwsjSink * __obj = (cTumkwsjSink *)_obj;
  if (_obj != NULL) {
    __obj->juliusDecoderThread();
  }
  SMILE_THREAD_RET;
}

void cTumkwsjSink::juliusDecoderThread()
{
  bool term;
  smileMutexLock(terminatedMtx);
  term = terminated;
  smileMutexUnlock(terminatedMtx);

  /* enter recongnition loop */
  int ret = j_open_stream(recog, NULL);
  switch(ret) {
      case 0:			/* succeeded */
        break;
      case -1:      		/* error */
        /* go on to next input */
        //continue;
      case -2:			/* end of recognition process */
        if (jconf->input.speech_input == SP_RAWFILE) {
          //          fprintf(stderr, "%d files processed\n", file_counter);
        } else if (jconf->input.speech_input == SP_STDIN) {
          fprintf(stderr, "reached end of input on stdin\n");
        } else {
          fprintf(stderr, "failed to begin input stream\n");
        }
        return;
  }
  /*
  if (outfile_enabled) {
  outfile_set_fname(j_get_current_filename());
  }
  */

  // TODO: terminate mechanism!!

  /* start recognizing the stream */
  ret = j_recognize_stream(recog);
}

int cTumkwsjSink::startJuliusDecoder()
{
  juliusIsRunning = 1;
  smileThreadCreate( decoderThread, juliusThreadRunner, this );
}

int cTumkwsjSink::myTick(long long t)
{
  if (!juliusIsRunning) {
    if (!startJuliusDecoder()) return 0;
  }
  smileCondSignal( tickCond );
  reader->catchupCurR();


  // tick success?
  int res = 0;
/*
smileMutexLock(terminatedMtx);
  if (terminated == FALSE) res = 1;
  smileMutexUnlock(terminatedMtx);
*/

  return res;
}


cTumkwsjSink::~cTumkwsjSink()
{
  smileMutexLock(dataFlgMtx);
  terminated = TRUE;
  smileCondSignal( tickCond );
  smileMutexUnlock(dataFlgMtx);
  
  if (decoderThread != NULL) smileThreadJoin(decoderThread);

  /* release all */
  j_recog_free(recog);

  //if (logfile) fclose(fp);

  smileMutexDestroy(terminatedMtx);
  smileMutexDestroy(dataFlgMtx);
  smileCondDestroy(tickCond);
}


#endif // HAVE_JULIUSLIB

