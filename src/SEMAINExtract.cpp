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
 


/****

This is the main file of the smile SEMAINE component.
It handles the option parsing and creates an instance of
the semaine::components::smile::TumFeatureExtractor class
and runs the main loop in a separate thread.

****/


#include <iostream>
#include <stdio.h>
#include <list>
#include <smileCommon.hpp>

#ifdef HAVE_SEMAINEAPI

#include <semaine/config.h>

#include <semaine/util/XMLTool.h>
#include <semaine/components/Component.h>
#include <semaine/system/ComponentRunner.h>

//#include <opensmile/TumFeatureExtractor.h>
#include <TumFeatureExtractor.h>


#define MODULE "SEMAINExtract"

/************** Ctrl+C signal handler **/
#include  <signal.h>

cComponentManager *cmanGlob = NULL;
semaine::components::smile::TumFeatureExtractor *tumExtrGlob = NULL;

void INThandler(int);
int ctrlc = 0;

void INThandler(int sig)
{
  //  char  c;
  signal(sig, SIG_IGN);
  //if (cmanGlob != NULL) cmanGlob->requestAbort();
  if (tumExtrGlob != NULL) tumExtrGlob->requestExit();
  signal(SIGINT, INThandler);
  ctrlc = 1;
}
/*******************************************/



int main (int argc, char *argv[]) {

  cConfigManager *configManager = NULL;
  cComponentManager *cMan = NULL;
  cCommandlineParser cmdline(argc,argv);

  try {

    // setup the openSMILE logger
    LOGGER.setLogFile("smile.log");
    LOGGER.setLogLevel(1);
    LOGGER.enableConsoleOutput();
    SMILE_MSG(1,"openSMILE starting!");

    // openSMILE commandline parser:
    cmdline.addStr( "configfile", 'C', "Path to openSMILE config file", "smile.conf" );
    cmdline.addInt( "loglevel", 'l', "Verbosity level (0-9)", 2 );
    cmdline.addInt( "nticks", 't', "Number of ticks to process (-1 = infinite)", -1 );
#ifdef DEBUG
    cmdline.addBoolean( "debug", 'd', "Show debug messages (on/off)", 0 );
#endif
    cmdline.addBoolean( "configHelp", 'H', "Show documentation of registered config types (on/off)", 0 );
    cmdline.addBoolean( "components", 'L', "Show component list", 0 );
    cmdline.addBoolean( "ccmdHelp", 'c', "Show custom commandline option help (those specified in config file)", 0 );

    int help = 0;
    if (cmdline.doParse() == -1) {
      LOGGER.setLogLevel(0);
      help = 1;
    }
    LOGGER.setLogLevel(cmdline.getInt("loglevel"));

#ifdef DEBUG
    if (!cmdline.getBoolean("debug"))
      LOGGER.setLogLevel(LOG_DEBUG, 0);
#endif

    //SMILE_PRINT("config file is: %s",cmdline.getStr("configfile"));

    // create configManager:
    configManager = new cConfigManager(&cmdline);
    // create config type that maps semaine component names to corresponding openSMILE activeMqSink/Source components
    registerComponentForSmileConfig(configManager);
    // add the file config reader:
    configManager->addReader( new cFileConfigReader( cmdline.getStr("configfile") ) );


    cMan = new cComponentManager(configManager,componentlist);
    if (cmdline.getBoolean("configHelp")) {
      configManager->printTypeHelp(0);
      help = 1;
    }
    if (cmdline.getBoolean("components")) {
      cMan->printComponentList();
      help = 1;
    }

    if (help==1) { 
      delete configManager;
      delete cMan;
      return -1; 
    }

    // TODO: read config here and print ccmdHelp...
    configManager->readConfig();
    cmdline.doParse(1,0); // warn if unknown options are detected on the commandline
    if (cmdline.getBoolean("ccmdHelp")) {
      cmdline.showUsage();
      delete configManager;
      delete cMan;
      return -1;
    }

    cmanGlob = cMan;  // initialise global, so the Ctrl+C handler has access to it...
    signal(SIGINT, INThandler); // now set the signal handler for Ctrl+C (SIGINT)

    cMan->createInstances(0); // 0 = do not read config (we already did that above..)

  } catch (cSMILException) { return EXIT_ERROR; }

  if (!ctrlc) {

	// start the actual component and its main loop
	try {
		semaine::util::XMLTool::startupXMLTools();

		std::list<semaine::components::Component *> comps;

    tumExtrGlob = new semaine::components::smile::TumFeatureExtractor(cMan,configManager);
		comps.push_back(tumExtrGlob);
		
		semaine::system::ComponentRunner cr(comps);
		cr.go();
		cr.waitUntilCompleted();

		semaine::util::XMLTool::shutdownXMLTools();
	} catch (cms::CMSException & ce) {
		ce.printStackTrace();
	} catch (std::exception & e) {
		std::cerr << e.what();
	} catch(cSMILException) { return EXIT_ERROR; } 

  }
  // it is important that configManager is deleted BEFORE componentManger! (since component Manger unregisters plugin Dlls)
  delete configManager;
  delete cMan;

  if (ctrlc) return EXIT_CTRLC;
  return EXIT_SUCCESS;

}

#else // not HAVE_SEMAINEAPI

#ifndef _MSC_VER // Visual Studio specific macro
#warning cannot build SEMAINExtract without being configured with libsemaineapi support!
#endif

int main (int argc, char *argv[]) {
  printf("\nSEMAINExtract was not compiled, because openSMILE was not configured with SEMAINE API support!\n\n");
  return 2;
}


#endif  //  HAVE_SEMAINEAPI

