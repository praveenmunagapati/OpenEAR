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

data reader from activeMq... TODO

*/


#include <activeMqSource.hpp>
#define MODULE "cActiveMqSource"

#ifdef HAVE_SEMAINEAPI

SMILECOMPONENT_STATICS(cActiveMqSource)

SMILECOMPONENT_REGCOMP(cActiveMqSource)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CACTIVEMQSOURCE;
  sdescription = COMPONENT_DESCRIPTION_CACTIVEMQSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->makeMandatory(ct->setField("topic","topic to read data vectors from",(const char*) NULL));
  )

  SMILECOMPONENT_MAKEINFO(cActiveMqSource);
}

SMILECOMPONENT_CREATE(cActiveMqSource)

//-----

cActiveMqSource::cActiveMqSource(const char *_name) :
  cDataSource(_name)
{
  // ...
}

void cActiveMqSource::fetchConfig()
{
  cDataSource::fetchConfig();
}

/*
int cActiveMqSource::myConfigureInstance()
{
    // call writer->setConfig here to set the dataMemory level configuration and override config file and defaults...
    
  int ret = 1;
  ret *= cDataSource::myConfigureInstance();
  // ...
  return ret;
}
*/

int cActiveMqSource::myFinaliseInstance()
{
  int ret = cDataSource::myFinaliseInstance();

  //if ((ret)&&(featureReceiver == NULL)) 
  {
 	  int period = 1; // ms   , TODO: correct period from input level T (reader->getLevelT())
    const char * topic = getStr("topic");
    SMILE_IDBG(3,"reading from ActiveMQ topic '%s' [TODO]",topic);
	//  featureSender = new FeatureSender(topic, "", getInstName(), period);
  }
  
  // configure dateMemory level, names, etc.
  // TODO: get names from featureReceiver... and add the fields...

  // PROBLEM: actually we must wait till first feature message has arrived in order to obtain the names...

//  writer->addField("randVal",nValues);
//  writer->addField("const");

//??  allocVec(nValues+1);

  return ret;

//  return cDataSource::myFinaliseInstance();
}


int cActiveMqSource::myTick(long long t)
{
  SMILE_DBG(4,"tick # %i, writing value vector",t);

  // todo: fill with random values...
//  vec->dataF[0] = (FLOAT_DMEM)t+(FLOAT_DMEM)3.3;
//  writer->setNextFrame(vec);
  
  return 1;
}


cActiveMqSource::~cActiveMqSource()
{
  // if (featureReceiver != NULL) delete featureReceiver;
}


#endif // HAVE_SEMAINEAPI
