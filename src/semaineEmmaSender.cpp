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

the SEMAINE Emma message sender for openSMILE

*/


#include <semaineEmmaSender.hpp>
//#include <math.h>

#define MODULE "cSemaineEmmaSender"

#ifdef HAVE_SEMAINEAPI

SMILECOMPONENT_STATICS(cSemaineEmmaSender)

SMILECOMPONENT_REGCOMP(cSemaineEmmaSender)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CSEMAINEEMMASENDER;
  sdescription = COMPONENT_DESCRIPTION_CSEMAINEEMMASENDER;

  // configure your component's configType:

  SMILECOMPONENT_CREATE_CONFIGTYPE
  //ct->setField("reader", "dataMemory interface configuration (i.e. what slot to read from)", sconfman->getTypeObj("cDataReader"), NO_ARRAY, DONT_FREE);
  ct->setField("dummy","nothing",0);
  SMILECOMPONENT_IFNOTREGAGAIN( {} )

  SMILECOMPONENT_MAKEINFO(cSemaineEmmaSender);
}

SMILECOMPONENT_CREATE(cSemaineEmmaSender)



//-----

cSemaineEmmaSender::cSemaineEmmaSender(const char *_name) :
  cSmileComponent(_name)
{
  // initialisation code...

}

  /*
void cSemaineEmmaSender::mySetEnvironment()
{

}

int cSemaineEmmaSender::myConfigureInstance()
{

  return 1;
}

int cSemaineEmmaSender::myFinaliseInstance()
{

  return 1;
}
*/

// get smileTime from _msg, convert to semaine time by getting current time from smile and semaine and observing the difference
long long cSemaineEmmaSender::smileTimeToSemaineTime( double smileTime ) 
{
  smileTime *= 1000.0;
  if (meta != NULL) {
    long long smit = (long long)round(getSmileTime()*1000.0);
    return (long long)round(smileTime) + (meta->getTime() - smit);
  } else {
    SMILE_IWRN(3,"warning, no meta component found, times sent are SMILE times and NOT semaine times!");
    return (long long)round(smileTime);
  }
  return 0;
}

// send an XML document and delete it afterwards
void cSemaineEmmaSender::sendDocument( DOMDocument * document )
{
  // Now send it
  if ((meta != NULL)&&(meta->isSystemReady())) {
    emmaSender->sendXML(document, meta->getTime());
  } 
  
  delete document;
  /*
  else {
    emmaSender->sendXML(document, 0.0);
  }*/
}

void cSemaineEmmaSender::sendArousalC( cComponentMessage *_msg )
{
  char strtmp[50];
  sprintf(strtmp,"%.2f",_msg->floatData[0]);
  std::string aroStr(strtmp);

  sprintf(strtmp,"%ld",smileTimeToSemaineTime(_msg->userTime1));
  std::string startTm(strtmp);
  sprintf(strtmp,"%ld",(long long)round((_msg->userTime2 - _msg->userTime1)*1000.0));
  std::string duration(strtmp);

  // Create and fill a simple EMMA EmotionML document
  DOMDocument * document = XMLTool::newDocument(EMMA::E_EMMA, EMMA::namespaceURI, EMMA::version);
  DOMElement * interpretation = XMLTool::appendChildElement(document->getDocumentElement(), EMMA::E_INTERPRETATION);
  XMLTool::setAttribute(interpretation, EMMA::A_OFFSET_TO_START, startTm);
  XMLTool::setAttribute(interpretation, EMMA::A_DURATION, duration);
  XMLTool::setPrefix(interpretation, "emma");
  
  DOMElement * emotion = XMLTool::appendChildElement(interpretation, EmotionML::E_EMOTION, EmotionML::namespaceURI);
  XMLTool::setPrefix(emotion, "emotion");

  DOMElement * dimensions = XMLTool::appendChildElement(emotion, EmotionML::E_DIMENSIONS, EmotionML::namespaceURI);
  XMLTool::setAttribute(dimensions, EmotionML::A_SET, "valenceArousalPotency");
  XMLTool::setPrefix(dimensions, "emotion");

  DOMElement * arousal = XMLTool::appendChildElement(dimensions, EmotionML::E_AROUSAL, EmotionML::namespaceURI);
  XMLTool::setAttribute(arousal, EmotionML::A_VALUE, aroStr);
  XMLTool::setPrefix(arousal, "emotion");

  sendDocument(document);
}

void cSemaineEmmaSender::sendValenceC( cComponentMessage *_msg )
{
  char strtmp[50];
  sprintf(strtmp,"%.2f",_msg->floatData[0]);
  std::string valStr(strtmp);

  sprintf(strtmp,"%ld",smileTimeToSemaineTime(_msg->userTime1));
  std::string startTm(strtmp);
  sprintf(strtmp,"%ld",(long long)round((_msg->userTime2 - _msg->userTime1)*1000.0));
  std::string duration(strtmp);

  // Create and fill a simple EMMA EmotionML document
  DOMDocument * document = XMLTool::newDocument(EMMA::E_EMMA, EMMA::namespaceURI, EMMA::version);
  DOMElement * interpretation = XMLTool::appendChildElement(document->getDocumentElement(), EMMA::E_INTERPRETATION);
  XMLTool::setAttribute(interpretation, EMMA::A_OFFSET_TO_START, startTm);
  XMLTool::setAttribute(interpretation, EMMA::A_DURATION, duration);
  XMLTool::setPrefix(interpretation, "emma");

  DOMElement * emotion = XMLTool::appendChildElement(interpretation, EmotionML::E_EMOTION, EmotionML::namespaceURI);
  XMLTool::setPrefix(emotion, "emotion");

  DOMElement * dimensions = XMLTool::appendChildElement(emotion, EmotionML::E_DIMENSIONS, EmotionML::namespaceURI);
  XMLTool::setAttribute(dimensions, EmotionML::A_SET, "valenceArousalPotency");
  XMLTool::setPrefix(dimensions, "emotion");

  DOMElement * valence = XMLTool::appendChildElement(dimensions, EmotionML::E_VALENCE, EmotionML::namespaceURI);
  XMLTool::setAttribute(valence, EmotionML::A_VALUE, valStr);
  XMLTool::setPrefix(valence, "emotion");

  // Now send it
  sendDocument(document);
}

void cSemaineEmmaSender::sendInterestC( cComponentMessage *_msg )
{
  char strtmp[50];
  sprintf(strtmp,"%.2f",_msg->floatData[0]);
  std::string interestStr(strtmp);

  // Create and fill a simple EMMA EmotionML document
  DOMDocument * document = XMLTool::newDocument(EMMA::E_EMMA, EMMA::namespaceURI, EMMA::version);

  sprintf(strtmp,"%ld",smileTimeToSemaineTime(_msg->userTime1));
  std::string startTm(strtmp);
  sprintf(strtmp,"%ld",(long long)round((_msg->userTime2 - _msg->userTime1)*1000.0));
  std::string duration(strtmp);

  DOMElement * oneof = XMLTool::appendChildElement(document->getDocumentElement(), EMMA::E_ONEOF);
  XMLTool::setAttribute(oneof, EMMA::A_OFFSET_TO_START, startTm);
  XMLTool::setAttribute(oneof, EMMA::A_DURATION, duration);
  XMLTool::setPrefix(oneof, "emma");

  DOMElement * interpretation0 = XMLTool::appendChildElement(oneof, EMMA::E_INTERPRETATION);
  sprintf(strtmp,"%.3f",((double*)(_msg->custData))[0]);
  std::string conf0(strtmp);
  XMLTool::setAttribute(interpretation0, EMMA::A_CONFIDENCE, conf0);
  XMLTool::setPrefix(interpretation0, "emma");

  DOMElement * emotion0 = XMLTool::appendChildElement(interpretation0, EmotionML::E_EMOTION, EmotionML::namespaceURI);
  XMLTool::setPrefix(emotion0, "emotion");
  DOMElement * category0 = XMLTool::appendChildElement(emotion0, EmotionML::E_CATEGORY, EmotionML::namespaceURI);
  XMLTool::setAttribute(category0, EmotionML::A_SET, "interestLevels");
  XMLTool::setAttribute(category0, EmotionML::A_NAME, "bored");
  XMLTool::setPrefix(category0, "emotion");

  DOMElement * interpretation1 = XMLTool::appendChildElement(oneof, EMMA::E_INTERPRETATION);
  sprintf(strtmp,"%.3f",((double*)(_msg->custData))[1]);
  std::string conf1(strtmp);
  XMLTool::setAttribute(interpretation1, EMMA::A_CONFIDENCE, conf1);
  XMLTool::setPrefix(interpretation1, "emma");

  DOMElement * emotion1 = XMLTool::appendChildElement(interpretation1, EmotionML::E_EMOTION, EmotionML::namespaceURI);
  XMLTool::setPrefix(emotion1, "emotion");
  DOMElement * category1 = XMLTool::appendChildElement(emotion1, EmotionML::E_CATEGORY, EmotionML::namespaceURI);
  XMLTool::setAttribute(category1, EmotionML::A_SET, "interestLevels");
  XMLTool::setAttribute(category1, EmotionML::A_NAME, "neutral");
  XMLTool::setPrefix(category1, "emotion");
  
  DOMElement * interpretation2 = XMLTool::appendChildElement(oneof, EMMA::E_INTERPRETATION);
  sprintf(strtmp,"%.3f",((double*)(_msg->custData))[2]);
  std::string conf2(strtmp);
  XMLTool::setAttribute(interpretation2, EMMA::A_CONFIDENCE, conf2);
  XMLTool::setPrefix(interpretation2, "emma");

  DOMElement * emotion2 = XMLTool::appendChildElement(interpretation2, EmotionML::E_EMOTION, EmotionML::namespaceURI);
  XMLTool::setPrefix(emotion2, "emotion");
  DOMElement * category2 = XMLTool::appendChildElement(emotion2, EmotionML::E_CATEGORY, EmotionML::namespaceURI);
  XMLTool::setAttribute(category2, EmotionML::A_SET, "interestLevels");
  XMLTool::setAttribute(category2, EmotionML::A_NAME, "interested");
  XMLTool::setPrefix(category2, "emotion");

  // Now send it
  sendDocument(document);
}

// start = 1: speaking status changed to start, start = 0: status changed to stop
void cSemaineEmmaSender::sendSpeakingStatus( cComponentMessage *_msg, int start )
{
  const char *strtmp = "stop";
  if (start) strtmp = "start";
  
  std::string statusStr(strtmp);

  char strtmp2[50];
  sprintf(strtmp2,"%ld",smileTimeToSemaineTime(_msg->userTime1));
  std::string startTm(strtmp2);

  // Create and fill a simple EMMA EmotionML document
  DOMDocument * document = XMLTool::newDocument(EMMA::E_EMMA, EMMA::namespaceURI, EMMA::version);
  DOMElement * interpretation = XMLTool::appendChildElement(document->getDocumentElement(), EMMA::E_INTERPRETATION);
  XMLTool::setAttribute(interpretation, EMMA::A_OFFSET_TO_START, startTm);
  XMLTool::setAttribute(interpretation, EMMA::A_DURATION, "0");
  XMLTool::setPrefix(interpretation, "emma");

  DOMElement * speaking = XMLTool::appendChildElement(interpretation, SemaineML::E_SPEAKING, SemaineML::namespaceURI);
  XMLTool::setAttribute(speaking, SemaineML::A_STATUS_CHANGE, strtmp);
  XMLTool::setPrefix(speaking, "semaine");

  // Now send it
  sendDocument(document);
}

#include <kwsjKresult.h>

void cSemaineEmmaSender::sendKeywords( cComponentMessage *_msg )
{
  char strtmp[50];
  sprintf(strtmp,"%.2f",_msg->floatData[0]);
  std::string valStr(strtmp);
  long long startTime = smileTimeToSemaineTime(_msg->userTime1);
  sprintf(strtmp,"%ld",startTime);
  std::string startTm(strtmp);
  sprintf(strtmp,"%ld",(long long)round((_msg->userTime2 - _msg->userTime1)*1000.0));
  std::string duration(strtmp);

  // Create and fill a simple EMMA EmotionML document
  DOMDocument * document = XMLTool::newDocument(EMMA::E_EMMA, EMMA::namespaceURI, EMMA::version);
  DOMElement * sequence = XMLTool::appendChildElement(document->getDocumentElement(), EMMA::E_SEQUENCE);
  XMLTool::setAttribute(sequence, EMMA::A_OFFSET_TO_START, startTm);
  XMLTool::setAttribute(sequence, EMMA::A_DURATION, duration);
  XMLTool::setPrefix(sequence, "emma");

  int i;
  Kresult *k = (Kresult *)(_msg->custData);
  for (i=0; i<k->numOfKw; i++) {

  DOMElement * interpretation = XMLTool::appendChildElement(sequence, EMMA::E_INTERPRETATION);
  sprintf(strtmp,"%ld",startTime + (long long)round((k->kwStartTime[i])*1000.0));
  std::string offs(strtmp);
  sprintf(strtmp,"%s",k->keyword[i]);
  std::string keyword(strtmp);
  sprintf(strtmp,"%.3f",k->kwConf[i]);
  std::string confidence(strtmp);
  XMLTool::setAttribute(interpretation, EMMA::A_OFFSET_TO_START, offs);
  XMLTool::setAttribute(interpretation, EMMA::A_TOKENS, keyword);
  XMLTool::setAttribute(interpretation, EMMA::A_CONFIDENCE, confidence);
  XMLTool::setPrefix(interpretation, "emma");

  }

  // Now send it
  sendDocument(document);
}





int cSemaineEmmaSender::processComponentMessage( cComponentMessage *_msg ) 
{ 
  if (isMessageType(_msg,"classificationResult")) {  
    // determine origin by message's user-defined name, which can be set in the config file
    SMILE_IDBG(3,"received 'classificationResult' message");
    if (!strcmp(_msg->msgname,"arousal")) sendArousalC(_msg);
    if (!strcmp(_msg->msgname,"valence")) sendValenceC(_msg);
    if (!strcmp(_msg->msgname,"interest")) sendInterestC(_msg);
    return 1;  // message was processed
  }
  else if (isMessageType(_msg,"asrKeywordOutput")) {  
    SMILE_IDBG(3,"received 'asrKeywordOutput' message");
    sendKeywords(_msg);
    return 1;  // message was processed
  }
  else if (isMessageType(_msg,"turnSpeakingStatus")) {
    SMILE_IDBG(3,"received 'turnSpeakingStatus' message");
    sendSpeakingStatus(_msg, _msg->intData[0]);
    return 1; // message was processed
  }

  return 0; // if message was not processed
}  

int cSemaineEmmaSender::myTick(long long t) 
{

  // return 1;  // tick did succeed!

  return 0; // tick did not succeed, i.e. nothing was processed or there was nothing to process
}

cSemaineEmmaSender::~cSemaineEmmaSender()
{
  // cleanup code...

}

#endif //HAVE_SEMAINEAPI

