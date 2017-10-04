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


*/


#ifndef __DATA_PROCESSOR_HPP
#define __DATA_PROCESSOR_HPP

#include <smileCommon.hpp>
#include <dataWriter.hpp>
#include <dataReader.hpp>

#define COMPONENT_DESCRIPTION_CDATAPROCESSOR "dataSource, which writes data to dataMemory"
#define COMPONENT_NAME_CDATAPROCESSOR "cDataProcessor"

class cDataProcessor : public cSmileComponent {
  private:
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    cDataWriter *writer;
    cDataReader *reader;
    int namesAreSet;
    
    double buffersize_sec; /* buffersize of write level, as requested by config file (in seconds)*/
    double blocksizeR_sec, blocksizeW_sec; /* blocksizes for block processing (in seconds)*/
    long buffersize; /* buffersize of write level, as requested by config file (in frames)*/
    long blocksizeR, blocksizeW; /* blocksizes for block processing (in frames)*/
    
    int copyInputName; /* 1=copy input name and append "nameAppend" (if != NULL) , 0 = do not copy and set name only to "nameAppend" */
    const char * nameAppend; /* suffix to append to element names */
    void addNameAppendField(const char*base, const char*append, int _N=1, int arrNameOffset=0);
    void addNameAppendFieldAuto(const char*base, const char *customFixed, int _N=1, int arrNameOffset=0);

    virtual void fetchConfig();
    virtual void mySetEnvironment();

    virtual int myRegisterInstance(int *runMe=NULL);
    virtual int myConfigureInstance();
    virtual int myFinaliseInstance();
    
    //---- *runMe return value for component manager : 0, don't call my tick of this component, 1, call myTick
    virtual int runMeConfig() { return 1; }

    //---- HOOKs during configure (for setting level parameters, buffersize/type, blocksize requirements)
    // overwrite this, to configure the writer AFTER the reader config is available
    // possible return values (others will be ignored): 
    // -1 : configureWriter has overwritten c->nT value for the buffersize, myConfigureInstance will not touch nT !
    virtual int configureWriter(sDmLevelConfig &c) { return 1; }
    
    // configure reader, i.e. setup matrix reading, blocksize requests etc...
    virtual int configureReader();

    //---- HOOKs during finalise (for setting names)
    virtual int dataProcessorCustomFinalise() { return 1; }

    virtual int setupNewNames(long nEl);

    virtual int setupNamesForField(int i, const char*name, long nEl);
    virtual void configureField(int idxi, long __N, long _nOut) {}
	  //virtual int setupNamesForField(int i, const char*name, long nEl, int arrNameOffset);


    virtual void setEOI() {
      cSmileComponent::setEOI();
      if (reader!=NULL) reader->setEOI();
      if (writer!=NULL) writer->setEOI();
    }

  public:
    SMILECOMPONENT_STATIC_DECL
    
    cDataProcessor(const char *_name);
    virtual ~cDataProcessor();
};




#endif // __DATA_PROCESSOR_HPP
