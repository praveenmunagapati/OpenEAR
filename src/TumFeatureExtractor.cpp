/*******************************************************************************
 * openSMILE
 *  - open Speech and Music Interpretation by Large-space Extraction -
 * Copyright (C) 2008  Florian Eyben, Martin Woellmer, Bjoern Schuller
 * 
 * Institute for Human-Machine Communication
 * Technische Universitaet Muenchen (TUM)
 * D-80333 Munich, Germany
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
 *******************************************************************************/


/*
 *  TumFeatureExtractor.cpp
 *  semaine
 *  
 */
#include <smileCommon.hpp>

#ifdef HAVE_SEMAINEAPI


#include <TumFeatureExtractor.h>

#undef MODULE
#define MODULE "TumFeatureExtractor"

#include <cstdlib>
#include <sstream>

#include <semaine/util/XMLTool.h>
#include <semaine/datatypes/xml/EMMA.h>
#include <semaine/datatypes/xml/EmotionML.h>
#include <semaine/datatypes/xml/SemaineML.h>

using namespace semaine::util;
using namespace semaine::datatypes::xml;
using namespace XERCES_CPP_NAMESPACE;

namespace semaine {
namespace components {
namespace smile {


TumFeatureExtractor::TumFeatureExtractor(cComponentManager *_cMan, cConfigManager *_conf) throw(CMSException) : 
ComponentForSmile("TumFeatureExtractor",_cMan,_conf,true,false)
{

  int period = 10; // ms

  if ((amqsink != NULL)&&(topicW != NULL)) {

    featureSender = new FeatureSender(topicW, "", getName(), period);
    //	featureSender = new FeatureSender("semaine.data.analysis.features.voice", "", getName(), period);

    waitingTime = period;
    senders.push_back(featureSender);

    amqsink->setFeatureSender(featureSender,&meta);
  } else {
    // TODO: use semaine exception here...
    SMILE_WRN(1,"amqsink == NULL in TumFeatureExtractor, please check semaineCfg section in config file (no features will be sent now!).");
  }

  if ((emmasender != NULL)&&(topicEmma != NULL)) {
    emmaSender = new EmmaSender(topicEmma, getName());
    senders.push_back(emmaSender);
    emmasender->setEmmaSender(emmaSender,&meta);

  }
  // Marc, 21 Dec 08: Deactivated, because it leads to dropped messages when the ASR
  // is on a different machine where the clock time is not the same.
  //featureSender->setTimeToLive(100); // discard messages after 100 ms


  if ((amqsource != NULL)&&(topicR != NULL)) {
    // TODO: create feature Receiver...
    //featureReceiver = new FeatureReceiver(topicR, "", getName(), period);
  }
}



TumFeatureExtractor::~TumFeatureExtractor()
{
  if (cMan != NULL) {
    cMan->requestAbort();
    smileThreadJoin( smileMainThread );
  }
}

SMILE_THREAD_RETVAL smileThreadRunner(void *_obj)
{
  cComponentManager * __obj = (cComponentManager *)_obj;
  if (_obj != NULL) {
    __obj->runMultiThreaded(-1);
  }
  SMILE_THREAD_RET;
}

void TumFeatureExtractor::customStartIO() throw(CMSException)
{
  // start the smile main thread, and call run
  smileThreadCreate( smileMainThread, smileThreadRunner, (void*)cMan  );
  if (cMan == NULL) { SMILE_ERR(1,"componentManager (cMan) is NULL, smileMainThread has not been started!"); }
}

void TumFeatureExtractor::react(SEMAINEMessage * message) throw (std::exception) 
{
  // use amqsource->writer to save data to dataMemory  (if configured to use source..)
if (amqsource == NULL) {
// TODO: use semaine exception here...
  SMILE_WRN(1,"amqsource == NULL in TumFeatureExtractor, please check semaineCfg section in config file (discarding received data!).");
  return;
}

  cDataWriter *writer = amqsource->getWriter();

  // TOOD: parse Semaine Message and write features to dataMemory...
  // Problem: featureNames.....!! We must assign generic names like "feature01" to "featureNN" and update them after the first message.... yet, we also don't know how many features we will be receiving, if we have not received the first message... BIG PROBLEM!!

}

void TumFeatureExtractor::act() throw(CMSException)
{
//	SMILE_DBG(1,"act() called!");

// NOTE: act is unused here, since the activeMqSink components will handle the sending of data directly...


//------------------------------------------------------------------------------------------------
//	SMILE_DBG(9,"calling getFrame (id1=%i)",framerId1);

					// ++AMQ++  send (arousal, valence, interest) as EMMA
/*
					char strtmp[50];
					sprintf(strtmp,"%.2f",a);
					std::string aroStr(strtmp);
					sprintf(strtmp,"%.2f",v);
					std::string valStr(strtmp);
					sprintf(strtmp,"%1.0f",i);
					std::string interestStr(strtmp);

					// Create and fill a simple EMMA EmotionML document
					DOMDocument * document = XMLTool::newDocument(EMMA::E_EMMA, EMMA::namespaceURI, EMMA::version);
					DOMElement * interpretation = XMLTool::appendChildElement(document->getDocumentElement(), EMMA::E_INTERPRETATION);
					XMLTool::setAttribute(interpretation, EMMA::A_START, "time not implemented");
					DOMElement * emotion = XMLTool::appendChildElement(interpretation, EmotionML::E_EMOTION, EmotionML::namespaceURI);
					if ((svmPredA != NULL)||(svmPredV != NULL)) {
					  DOMElement * dimensions = XMLTool::appendChildElement(emotion, EmotionML::E_DIMENSIONS, EmotionML::namespaceURI);
					  XMLTool::setAttribute(dimensions, EmotionML::A_SET, "valenceArousalPotency");
					  if (svmPredA != NULL) {
					    DOMElement * arousal = XMLTool::appendChildElement(dimensions, EmotionML::E_AROUSAL, EmotionML::namespaceURI);
					    XMLTool::setAttribute(arousal, EmotionML::A_VALUE, aroStr);
					  }
					  if (svmPredV != NULL) {
					    DOMElement * valence = XMLTool::appendChildElement(dimensions, EmotionML::E_VALENCE, EmotionML::namespaceURI);
					    XMLTool::setAttribute(valence, EmotionML::A_VALUE, valStr);
					  }
					}
					if (svmPredI != NULL) {
					  DOMElement * category = XMLTool::appendChildElement(emotion, EmotionML::E_CATEGORY, EmotionML::namespaceURI);
					  XMLTool::setAttribute(category, EmotionML::A_NAME, "interest");
					  XMLTool::setAttribute(category, EmotionML::A_VALUE, interestStr);
					}

					// Now send it
					emmaSender->sendXML(document, meta.getTime());
*/

/*}    else {
          				// TODO: skip frame...
        			}
				turntime = 0;


			}
		}
		#endif

	}
*/


}


} // namespace smile
} // namespace components
} // namespace semaine


#endif // HAVE_SEMAINEAPI
