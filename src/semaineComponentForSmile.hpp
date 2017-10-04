/*
  Copyright (c) 2009 by TUM, Florian Eyben, Martin Woellmer, Bjoern Schuller
  All rights reserved.

  this file is part of the SEMAINE system for virtual agents
*/

#ifndef SEMAINE_COMPONENT_FOR_SMILE_H
#define SEMAINE_COMPONENT_FOR_SMILE_H

#include <semaine/config.h>

#include <cms/CMSException.h>

#include <semaine/components/Component.h>
#include <semaine/cms/sender/FeatureSender.h>
#include <semaine/cms/sender/EmmaSender.h>

#include <smileCommon.hpp>

#include <configManager.hpp>
#include <commandlineParser.hpp>
#include <componentManager.hpp>
#include <activeMqSink.hpp>
#include <activeMqSource.hpp>
#include <semaineEmmaSender.hpp>

using namespace cms;
using namespace semaine::components;
//using namespace tum::components:smile;
//using namespace semaine::cms::sender;


inline void registerComponentForSmileConfig(cConfigManager *_conf)
{
  if (_conf != NULL) {
    // TOOD: check if configType already exists....!

    ConfigType * ct= new ConfigType("semaineCfg");
    ct->setField("amqSource","associative array of cActiveMqSource instance names (values) mapped to semaine component names (currently hard-coded) (keys)",(const char *)NULL,ARRAY_TYPE);
    ct->setField("amqSink","associative array of cActiveMqSink instance names (values) mapped to semaine component names (currently hard-coded) (keys)",(const char *)NULL,ARRAY_TYPE);
    ct->setField("emmaSender","associative array of cSemaineEmmaSender names (values) mapped to semaine component names (currently hard-coded) (keys)",(const char *)NULL,ARRAY_TYPE);
    ct->setField("topicW","associative array of feature write topic names (values) mapped to semaine component names (currently hard-coded) (keys)",(const char *)NULL,ARRAY_TYPE);
    ct->setField("topicR","associative array of feature read topic names (values) mapped to semaine component names (currently hard-coded) (keys)",(const char *)NULL,ARRAY_TYPE);
    ct->setField("topicEmma","associative array of emma analysis topic names (values) mapped to semaine component names (currently hard-coded) (keys)",(const char *)NULL,ARRAY_TYPE);
    ConfigInstance *ci = new ConfigInstance("semaineCfg",ct,1);
    _conf->registerType(ci);
  }
}

class ComponentForSmile : public Component
{
public:
  ComponentForSmile(const std::string & componentName, cComponentManager *_cMan, cConfigManager *_conf, bool isInput=false, bool isOutput=false) throw (CMSException) :
      Component(componentName, isInput, isOutput),
        amqsink(NULL),
        amqsource(NULL),
        emmasender(NULL),
        topicW(NULL), topicR(NULL), topicEmma(NULL),
        cMan(_cMan)
      {

        // read destination (W) and source (R) topic names 
        if (_conf->isSet_f(myvprint("semaine.topicW[%s]",getName().c_str()))) topicW = _conf->getStr_f(myvprint("semaine.topicW[%s]",getName().c_str()));
        if (_conf->isSet_f(myvprint("semaine.topicR[%s]",getName().c_str()))) topicR = _conf->getStr_f(myvprint("semaine.topicR[%s]",getName().c_str()));
        if (_conf->isSet_f(myvprint("semaine.topicEmma[%s]",getName().c_str()))) topicEmma = _conf->getStr_f(myvprint("semaine.topicEmma[%s]",getName().c_str()));

        // read config for mapping of Semaine Component name to activeMqSources/Sinks
        const char *asrc = NULL;
        if (_conf->isSet_f(myvprint("semaine.amqSource[%s]",getName().c_str()))) asrc = _conf->getStr_f(myvprint("semaine.amqSource[%s]",getName().c_str()));
        const char *asink = NULL;
        if (_conf->isSet_f(myvprint("semaine.amqSink[%s]",getName().c_str()))) asink = _conf->getStr_f(myvprint("semaine.amqSink[%s]",getName().c_str()));
        const char *emmas = NULL;
        if (_conf->isSet_f(myvprint("semaine.emmaSender[%s]",getName().c_str()))) emmas = _conf->getStr_f(myvprint("semaine.emmaSender[%s]",getName().c_str()));

        // get openSMILE component pointers by name from _cMan
        if (cMan != NULL) {
          if (asink != NULL) setSmileAMQsink(cMan->getComponentInstance(asink));
          if (asrc != NULL) setSmileAMQsource(cMan->getComponentInstance(asrc)); 
          if (emmas != NULL) setSmileEMMAsender(cMan->getComponentInstance(emmas)); 
        }

      }

      virtual ~ComponentForSmile() {}

      int setSmileAMQsink(cSmileComponent *_amqsink) { return ((amqsink = (cActiveMqSink *)_amqsink) != NULL); }
      int setSmileAMQsource(cSmileComponent *_amqsource) { return ((amqsource = (cActiveMqSource *)_amqsource) != NULL); }
      int setSmileEMMAsender(cSmileComponent *_emmas) { return ((emmasender = (cSemaineEmmaSender *)_emmas) != NULL); }

protected:
  virtual void act() throw (CMSException) {}
  virtual void react(SEMAINEMessage * message) throw (std::exception) {}
  virtual void customStartIO() throw (CMSException) {}

  const char * topicW;   // topic to write feature data to
  const char * topicR;   // topic to read feature data from  (could also be wave data or any other binary data)
  const char * topicEmma;   // topic to send Emma data to

  FeatureSender * featureSender;
  //FeatureReceiver * featureReceiver;
  EmmaSender * emmaSender;

  cComponentManager *cMan;
  cActiveMqSink *amqsink;
  cActiveMqSource *amqsource;
  cSemaineEmmaSender *emmasender;

private:
};

#endif //SEMAINE_COMPONENT_FOR_SMILE_H


