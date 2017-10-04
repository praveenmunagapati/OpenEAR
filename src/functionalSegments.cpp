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

functionals: number of segments based on delta thresholding

*/


#include <functionalSegments.hpp>

#define MODULE "cFunctionalSegments"


#define FUNCT_NUMSEGMENTS      0  // number of segments (relative to maxNumSeg)
#define FUNCT_SEGMEANLEN       1  // mean length of segment (relative to input length)
#define FUNCT_SEGMAXLEN        2  // max length of segment (relative to input length)
#define FUNCT_SEGMINLEN        3  // min length of segment (relative to input length)

#define N_FUNCTS  4

#define NAMES     "numSegments","meanSegLen","maxSegLen","minSegLen"

const char *segmentsNames[] = {NAMES};  

SMILECOMPONENT_STATICS(cFunctionalSegments)

SMILECOMPONENT_REGCOMP(cFunctionalSegments)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CFUNCTIONALSEGMENTS;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALSEGMENTS;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("maxNumSeg","max. number of segments to detect",20);
	ct->setField("rangeRelThreshold","segment threshold relative to signal range (max-min)",0.2);
    ct->setField("numSegments","1/0=enable/disable computation of number of segments (relative to maxNumSeg)",1);
    ct->setField("meanSegLen","1/0=enable/disable computation of mean segment length (relative to input length)",1);
    ct->setField("maxSegLen","1/0=enable/disable computation of max segment length (relative to input length)",1);
    ct->setField("minSegLen","1/0=enable/disable computation of min segment length (relative to input length)",1);
  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalSegments);
}

SMILECOMPONENT_CREATE(cFunctionalSegments)

//-----

cFunctionalSegments::cFunctionalSegments(const char *_name) :
  cFunctionalComponent(_name,N_FUNCTS,segmentsNames),
  maxNumSeg(20)
{
}

void cFunctionalSegments::fetchConfig()
{
  if (getInt("numSegments")) enab[FUNCT_NUMSEGMENTS] = 1;
  if (getInt("meanSegLen")) enab[FUNCT_SEGMEANLEN] = 1;
  if (getInt("maxSegLen")) enab[FUNCT_SEGMAXLEN] = 1;
  if (getInt("minSegLen")) enab[FUNCT_SEGMINLEN] = 1;

  maxNumSeg = getInt("maxNumSeg");
  SMILE_IDBG(2,"maxNumSeg = %i",maxNumSeg);

  rangeRelThreshold = (FLOAT_DMEM)getDouble("rangeRelThreshold");
  SMILE_IDBG(2,"rangeRelThreshold = %f",rangeRelThreshold);

  cFunctionalComponent::fetchConfig();
}

long cFunctionalSegments::process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM *out, long Nin, long Nout)
{
  int i;
  if ((Nin>0)&&(out!=NULL)) {
    int nSegments = 0;

	FLOAT_DMEM max=in[0];
	FLOAT_DMEM min=in[0];
	for (i=1; i<Nin; i++) {
      if (in[i]<min) min=in[i];
      if (in[i]>max) max=in[i];
	}
    FLOAT_DMEM range = max-min;

	FLOAT_DMEM segThresh = range * rangeRelThreshold;

	long segMinLng = Nin/maxNumSeg-1;
	if (segMinLng < 2) segMinLng = 2;
	long ravgLng = Nin/(maxNumSeg/2);
	long lastSeg = -segMinLng/2;

	long meanSegLen = 0;
	long maxSegLen = 0;
	long minSegLen = 0;

	FLOAT_DMEM ravg = 0.0;
	for (i=0; i<Nin; i++) {
      ravg += in[i];
      if (i>=ravgLng) ravg -= in[i-ravgLng];
    
	  FLOAT_DMEM ravgLngCur = (FLOAT_DMEM)( MIN(i,ravgLng) );
      FLOAT_DMEM ra = ravg / ravgLngCur;

      if ((in[i]-ra > segThresh)&&(i - lastSeg > segMinLng) )
      { // found new segment begin
        nSegments++;
		long segLen = i-lastSeg;
		meanSegLen += segLen;
		if (segLen > maxSegLen) maxSegLen = segLen;
		if ((minSegLen==0)||(segLen<minSegLen)) minSegLen = segLen;
        lastSeg = i;
      }

    }

    
    int n=0;
    
	if (enab[FUNCT_NUMSEGMENTS]) out[n++]=(FLOAT_DMEM)nSegments/(FLOAT_DMEM)(maxNumSeg);
	if (enab[FUNCT_SEGMEANLEN]) {
	  if (nSegments > 1) 
	    out[n++]=(FLOAT_DMEM)meanSegLen/((FLOAT_DMEM)nSegments*(FLOAT_DMEM)(Nin));
	  else 
		out[n++]=(FLOAT_DMEM)meanSegLen/((FLOAT_DMEM)(Nin));
	}
	if (enab[FUNCT_SEGMAXLEN]) out[n++]=(FLOAT_DMEM)maxSegLen/(FLOAT_DMEM)(Nin);
	if (enab[FUNCT_SEGMINLEN]) out[n++]=(FLOAT_DMEM)minSegLen/(FLOAT_DMEM)(Nin);

	return n;
  }
  return 0;
}

/*
long cFunctionalSegments::process(INT_DMEM *in, INT_DMEM *inSorted, INT_DMEM *out, long Nin, long Nout)
{

  return 0;
}
*/

cFunctionalSegments::~cFunctionalSegments()
{
}

