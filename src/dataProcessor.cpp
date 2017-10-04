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

dataProcessor
write data to data memory...

*/


#include <dataProcessor.hpp>

#define MODULE "cDataProcessor"

SMILECOMPONENT_STATICS(cDataProcessor)

SMILECOMPONENT_REGCOMP(cDataProcessor)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CDATAPROCESSOR;
  sdescription = COMPONENT_DESCRIPTION_CDATAPROCESSOR;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  if (ct->setField("writer", "dataMemory interface configuration",
                  sconfman->getTypeObj("cDataWriter"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }
  if (ct->setField("reader", "dataMemory interface configuration",
                  sconfman->getTypeObj("cDataReader"), NO_ARRAY, DONT_FREE) == -1) {
     rA=1; // if subtype not yet found, request , re-register in the next iteration
  }

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("buffersize", "buffer size for output level in frames (default [0] = same as input level)", 0);
    ct->setField("buffersize_sec", "buffer size for output level in seconds (default [0] = same as input level)", 0);
    ct->setField("blocksize", "size of data blocks to process in frames", 0);
    ct->setField("blocksizeR", "size of data blocks to read in frames (overwrites blocksize!)", 0);
    ct->setField("blocksizeW", "size of data blocks to write in frames (overwrites blocksize!)", 0);
    ct->setField("blocksize_sec", "size of data blocks to process in seconds", 0.0);
    ct->setField("blocksizeR_sec", "size of data blocks to read in seconds (overwrites blocksize!)", 0.0);
    ct->setField("blocksizeW_sec", "size of data blocks to write in seconds (overwrites blocksize!)", 0.0);

    ct->setField("nameAppend", "suffix to append to dataMemory field names (default: empty)", (const char*)NULL);
    ct->setField("copyInputName", "1 = copy input name, 0 = use only nameAppend name", 1);
  )
  
  SMILECOMPONENT_MAKEINFO_ABSTRACT(cDataProcessor);
}

SMILECOMPONENT_CREATE_ABSTRACT(cDataProcessor)

//-----

cDataProcessor::cDataProcessor(const char *_name) :
  cSmileComponent(_name),
  writer(NULL),
  reader(NULL),
  namesAreSet(0),
  buffersize_sec(0.0),
  blocksizeR_sec(0.0), blocksizeW_sec(0.0),
  buffersize(0),
  blocksizeR(0), blocksizeW(0),
  nameAppend(NULL),
  copyInputName(0)
{
  // create the child instances, reader and writer
  char *tmp = myvprint("%s.reader",getInstName());
  reader = (cDataReader *)(cDataReader::create(tmp));
  if (reader == NULL) {
    COMP_ERR("Error creating dataReader '%s'",tmp);
  }
  if (tmp!=NULL) free(tmp);

  tmp = myvprint("%s.writer",getInstName());
  writer = (cDataWriter *)(cDataWriter::create(tmp));
  if (writer == NULL) {
    COMP_ERR("Error creating dataWriter '%s'",tmp);
  }
  if (tmp!=NULL) free(tmp);

  //( TODO: configure dateWriter level according to (descending priority)
  //   1. dataReader level clone
  //   2. application code (based on info from dataReader level)
  //   3. config file )

  //( TODO: automatically set dmInstance field based on field in cDataProcessor config! )
}

void cDataProcessor::fetchConfig()
{
  buffersize_sec = getDouble("buffersize_sec");
  SMILE_IDBG(2,"buffersize (sec.) = %f",buffersize_sec);

  buffersize = getInt("buffersize");
  SMILE_IDBG(2,"buffersize (frames.) = %i",buffersize);

  //double blocksize_sec = -1.0;
  //if (isSet("blocksize_sec")) {
  double blocksize_sec = getDouble("blocksize_sec");
  //}
  blocksizeR_sec = blocksizeW_sec = blocksize_sec;
  
  if ( (blocksizeR_sec <= 0.0) || (isSet("blocksizeR_sec")) ) {
    blocksizeR_sec = getDouble("blocksizeR_sec");
  }
  
  if ( (blocksizeW_sec <= 0.0) || (isSet("blocksizeW_sec")) ) {
    blocksizeW_sec = getDouble("blocksizeW_sec");
  }

  SMILE_IDBG(2,"blocksizeR (sec.) = %f",blocksizeR_sec);
  SMILE_IDBG(2,"blocksizeW (sec.) = %f",blocksizeW_sec);

  //long blocksize = -1;
  //if (isSet("blocksize")) {
  long blocksize = getInt("blocksize");
  //}
  blocksizeR = blocksizeW = blocksize;

  if ( (blocksizeR <= 0) || (isSet("blocksizeR")) ) {
    blocksizeR = getInt("blocksizeR");
  }
  
  if ( (blocksizeW <= 0) || (isSet("blocksizeW")) ) {
    blocksizeW = getInt("blocksizeW");
  }

  SMILE_IDBG(2,"blocksizeR (frames, from config only) = %i",blocksizeR);
  SMILE_IDBG(2,"blocksizeW (frames, from config only) = %i",blocksizeW);

  nameAppend = getStr("nameAppend");
  if (nameAppend != NULL) SMILE_IDBG(2,"nameAppend = '%s'",nameAppend);

  copyInputName = getInt("copyInputName");
  SMILE_IDBG(2,"copyInputName = %i",copyInputName);
}

void cDataProcessor::mySetEnvironment()
{
  writer->setComponentEnvironment(getCompMan(), -1, this);
  reader->setComponentEnvironment(getCompMan(), -1, this);
}

/* register both read and write request, order is arbitrary */
// TODO: *runMe config... ? from config file... or from a custom hook for base classes
int cDataProcessor::myRegisterInstance(int *runMe)
{
  int ret = 1;
  ret *= reader->registerInstance();
  ret *= writer->registerInstance();
  if ((ret)&&(runMe!=NULL)) {
    // call runMe config hook
    *runMe = runMeConfig();
  }
  return ret;
}

int cDataProcessor::configureReader() 
{ 
/*  reader->setBlocksizeSec(blocksizeR_sec);
  reader->setBlocksize(blocksizeR);
  return 1; */
  
  reader->setBlocksize(blocksizeR);
  return 1; 
}

/* configure both reader and writer. 
  here, the reader has to be configured first, in order to be able to auto configure the writer
 */
int cDataProcessor::myConfigureInstance()
{
  if (!(reader->configureInstance())) return 0;
  // finalise the reader first. this makes sure that the names in the input level are set up correctly
  if (!(reader->finaliseInstance())) return 0;

  // allow derived class to configure the writer AFTER the reader config is available
  // if the derived class returns 1, we will continue
  const sDmLevelConfig *c = reader->getConfig();
  if (c==NULL) COMP_ERR("myConfigureInstance: Error getting reader dmLevel config! returned sDmLevelConfig = NULL!");

  // now copy config so that we can safely modify it...
  sDmLevelConfig c2(*c);

  // convert blocksize options, so all options are accessible, if possible:
  // 1. blocksize values in frames override those in seconds:
  // 2. now do the inverse...
  if (blocksizeW > 0) {
    blocksizeW_sec = (double)blocksizeW * c2.T;
  } else if ((blocksizeW_sec > 0.0)&&(c2.T != 0.0)) {
    blocksizeW = (long) ceil (blocksizeW_sec / c2.T);
  }

  if (blocksizeR > 0) {
    blocksizeR_sec = (double)blocksizeW * c2.T;
  } else if ((blocksizeR_sec > 0.0)&&(c2.T != 0.0)) {
    blocksizeR = (long) ceil (blocksizeR_sec / c2.T);
  } else {
    SMILE_IDBG(3,"using fallback blocksizeR of 1, because blocksizeR or blocksize_sec was not set in config!");
    blocksizeR = 1;
  }

  // if only blocksizeR was set instead of blocksize, auto set blocksizeW to blocksizeR
  // NOTE: if blocksizeW != 0 this auto setting will not be performed...
  if (blocksizeW <= 0) {
    blocksizeW = blocksizeR;
    blocksizeW_sec = blocksizeR_sec;
  }
  
  // set writer blocksize from "blocksizeW" config option
  c2.blocksizeWriter = blocksizeW;
  long oldBsw = blocksizeW;

  // allow custom config of reader... 
  // main purpose: "communicating" blocksizeR to dataMemory !
  if (!configureReader()) {
    // NOTE: reader config (setup matrix reading /blocksize / etc. may also be performed in configureWriter!! 
    SMILE_IERR(1,"configureReader() returned 0 (failure)!");
    return 0;  
  }

  // provide a hook, to allow a derived component to modify the writer config we have obtained from the parent level:
  int ret = configureWriter(c2);
  // possible return values (others will be ignored): 
  // -1 : configureWriter has overwritten c->nT value for the buffersize, myConfigureInstance will not touch nT !
  // 0 : failure, myConfigure must exit with code 0
  // 1 : default / success
  if (!ret) {
    SMILE_IERR(1,"configureWriter() returned 0 (failure)!");
    return 0;
  }

  // since configure writer may modify either c2.blocksizeWriter OR blocksizeW alone, we sync again if they differ:
  if (c2.blocksizeWriter != oldBsw) {
    blocksizeW = c2.blocksizeWriter;
  } else if (blocksizeW != oldBsw) {
    c2.blocksizeWriter = blocksizeW;
  }

  if (ret!=-1) {
    if (buffersize > 0) {
      c2.nT = buffersize;
    } else if (buffersize_sec > 0.0) {
      if (c2.T != 0.0) {
        c2.nT = (long)ceil(buffersize_sec/c2.T);
      } else {
        c2.nT = (long)ceil(buffersize_sec);
      } 
    } // else, don't modify c2.nT : buffersize will be the same as in input level or as set by configureWriter!
  }

  writer->setConfig(c2,0);

  return writer->configureInstance();
}

// simply add a field with given parameters, concat base and append (if not NULL)
void cDataProcessor::addNameAppendField(const char*base, const char*append, int _N, int arrNameOffset)
{
  char *xx;

  if  ((append != NULL)&&(strlen(append)>0)) {
    if ((base != NULL)&&(strlen(base)>0)) {
      xx = myvprint("%s_%s",base,append);
      writer->addField( xx, _N, arrNameOffset );
      free(xx);
    } else {
      writer->addField( append, _N, arrNameOffset );
    }
  } else {
    if ((base != NULL)&&(strlen(base)>0)) {
      writer->addField( base, _N, arrNameOffset );
    } else {
      writer->addField( "noname", _N, arrNameOffset );
    }
  }
}

// automatically append "nameAppend", also allow for a fixed cutom part of the name
// base is automatically appended or not depending on the value of copyInputName
void cDataProcessor::addNameAppendFieldAuto(const char*base, const char *customFixed, int _N, int arrNameOffset)
{
  char *xx;

  if  ((nameAppend != NULL)&&(strlen(nameAppend)>0)) {
    if ((customFixed != NULL)&&(strlen(customFixed)>0)) {

      if ((copyInputName)&&(base != NULL)&&(strlen(base)>0)) {
        xx = myvprint("%s_%s%s",base,customFixed,nameAppend);
        writer->addField( xx, _N, arrNameOffset );
        free(xx);
      } else {
        xx = myvprint("%s%s",customFixed,nameAppend);
        writer->addField( xx, _N, arrNameOffset );
        free(xx);
      }

    } else {

      if ((copyInputName)&&(base != NULL)&&(strlen(base)>0)) {
        xx = myvprint("%s_%s",base,nameAppend);
        writer->addField( xx, _N, arrNameOffset );
        free(xx);
      } else {
        writer->addField( nameAppend, _N, arrNameOffset );
      }

    }
  } else {
    if ((customFixed != NULL)&&(strlen(customFixed)>0)) {

      if ((copyInputName)&&(base != NULL)&&(strlen(base)>0)) {
        xx = myvprint("%s_%s",base,customFixed);
        writer->addField( xx, _N, arrNameOffset );
        free(xx);
      } else {
        writer->addField( customFixed, _N, arrNameOffset );
      }


    } else {

      if ((copyInputName)&&(base != NULL)&&(strlen(base)>0)) {
        writer->addField( base, _N, arrNameOffset );
      } else {
        writer->addField( "noname", _N, arrNameOffset );
      }

    }

  }
}

int cDataProcessor::setupNewNames(long nEl)
{
	// if you overwrite this method, uncomment the following line in your own class'es code:
	// namesAreSet=1 
   return 1;
}

int cDataProcessor::setupNamesForField(int i, const char*name, long nEl)
{
  if (copyInputName) {
    addNameAppendField( name, nameAppend, nEl );
  } else {
    addNameAppendField( NULL, nameAppend, nEl );
  }
  return nEl;
}

/*
int cDataProcessor::setupNamesForField(int i, const char*name, long nEl, int arrNameOffset)
{
  if ((nameAppend != NULL)&&(strlen(nameAppend)>0)) {
    addNameAppendField(name,nameAppend,nEl,arrNameOffset);
  } else {
    writer->addField( name, nEl, arrNameOffset);
  }
  return nEl;
}
*/


/* finalise reader an writer
   first finalise the reader, so we can read field/element names from it
   then set names for the writer
   then finalise the writer...
 */
int cDataProcessor::myFinaliseInstance()
{
  // an extra hook to allow custom configure of e.g. vectorProcessors, etc. without overwriting myFinaliseInstance!
  if (!dataProcessorCustomFinalise()) {
    SMILE_IERR(1,"dataProcessorCustomFinalise returned 0 (failure) !");
    return 0;
  }

  /* available hooks for setting data element names in the output level:
    1. setupNewNames() : you are free to set any names in the output level, independent of the names in the input level
       (configureField will not be called! you must do everything in setupNewNames!)

    2. setupNamesForField() : this will be called for every input field(!), use this if you only want to append a suffix or change the number of elements in each field!
           setupNamesFor filed must return the number of elements that were set for the current field
       + configureField() : custom configuration (e.g. allocation of buffers, etc.) that the component must perform shall be put into this method!

    a hook that actually sets names MUST set the variable "namesAreSet" to 1

    if no hook sets the names, the names will be copied from the input level, and the suffix "nameAppend" will be appended!
  */

  // hook 1.
  if (!namesAreSet) {
    if (!setupNewNames(reader->getLevelNf())) {
      SMILE_IERR(1,"setupNewNames() returned 0 (failure)!");
      return 0;
    }
  }

  // hook 2.
  if (!namesAreSet) {
    int _N = reader->getLevelNf();
    int i;
    for (i=0; i<_N; i++) {
      int __N=0;
	    int arrNameOffset=0;
      const char *tmp = reader->getFieldName(i,&__N,&arrNameOffset);
      long nOut = setupNamesForField(i,tmp,__N);
      if (nOut==__N) {
  		  writer->setArrNameOffset(arrNameOffset);
	    }
      configureField(i,__N,nOut);
    }
    namesAreSet = 1;
  }
  
  return writer->finaliseInstance();
}

cDataProcessor::~cDataProcessor()
{
  if (writer != NULL) { delete writer; }
  if (reader != NULL) { delete reader; }
}


