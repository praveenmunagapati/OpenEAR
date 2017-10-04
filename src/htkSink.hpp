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

ARFF file output (for WEKA)

*/


#ifndef __CHTKSINK_HPP
#define __CHTKSINK_HPP

#include <smileCommon.hpp>
#include <smileComponent.hpp>
#include <dataSink.hpp>

#define COMPONENT_DESCRIPTION_CHTKSINK "write dataMemory data to an HTK feature file"
#define COMPONENT_NAME_CHTKSINK "cHtkSink"

typedef struct {
  uint32_t nSamples;
  uint32_t samplePeriod;
  uint16_t sampleSize;
  uint16_t parmKind;
} sHTKheader;

class cHtkSink : public cDataSink {
  private:
    FILE * filehandle;
    const char *filename;
    int lag;
    int append;
    int vax;
    uint16_t parmKind;
    uint32_t vecSize;
    uint32_t nVec;
    double period;
    sHTKheader header;

    int IsVAXOrder ()
    {
    	short x;
    	unsigned char *pc;
    	pc = (unsigned char *) &x;
    	*pc = 1; *( pc + 1 ) = 0;			// store bytes 1 0
    	return x==1;					// does it read back as 1?
    }

    void Swap32 ( uint32_t *p )
    {
    	uint8_t temp,*q;
    	q = (uint8_t*) p;
    	temp = *q; *q = *( q + 3 ); *( q + 3 ) = temp;
    	temp = *( q + 1 ); *( q + 1 ) = *( q + 2 ); *( q + 2 ) = temp;
    }

    void Swap16 ( uint16_t *p )
    {
    	uint8_t temp,*q;
    	q = (uint8_t*) p;
    	temp = *q; *q = *( q + 1 ); *( q + 1 ) = temp;
    }

    void SwapFloat( float *p )
    {
    	uint8_t temp,*q;
    	q = (uint8_t*) p;
    	temp = *q; *q = *( q + 3 ); *( q + 3 ) = temp;
    	temp = *( q + 1 ); *( q + 1 ) = *( q + 2 ); *( q + 2 ) = temp;
    }

    void prepareHeader( sHTKheader *h )
    {
      if ( vax )
      {
        Swap32 ( &(h->nSamples) );
        Swap32 ( &(h->samplePeriod) );
        Swap16 ( &(h->sampleSize) );
        Swap16 ( &(h->parmKind) );
      }
    }

    int writeHeader();
    int readHeader();
    
  protected:
    SMILECOMPONENT_STATIC_DECL_PR
    
    virtual void fetchConfig();
    //virtual int myConfigureInstance();
    virtual int myFinaliseInstance();
    virtual int myTick(long long t);


  public:
    //static sComponentInfo * registerComponent(cConfigManager *_confman);
    //static cSmileComponent * create(const char *_instname);
    SMILECOMPONENT_STATIC_DECL
    
    cHtkSink(const char *_name);

    virtual ~cHtkSink();
};




#endif // __CHTKSINK_HPP
