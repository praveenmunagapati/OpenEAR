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

functionals: extreme values and ranges

*/


#include <functionalExtremes.hpp>

#define MODULE "cFunctionalExtremes"


#define FUNCT_MAX     0
#define FUNCT_MIN     1
#define FUNCT_RANGE   2
#define FUNCT_MAXPOS   3
#define FUNCT_MINPOS   4
#define FUNCT_AMEAN   5
#define FUNCT_MAXAMEANDIST   6
#define FUNCT_MINAMEANDIST   7

#define N_FUNCTS  8

#define NAMES     "max","min","range","maxPos","minPos","amean","maxameandist","minameandist"

const char *extremesNames[] = {NAMES};  // change variable name to your clas...

SMILECOMPONENT_STATICS(cFunctionalExtremes)

SMILECOMPONENT_REGCOMP(cFunctionalExtremes)
{
  SMILECOMPONENT_REGCOMP_INIT
  
  scname = COMPONENT_NAME_CFUNCTIONALEXTREMES;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALEXTREMES;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("max","1/0=enable/disable computation of maximum value",1);
    ct->setField("min","1/0=enable/disable computation of minimum value",1);
    ct->setField("range","1/0=enable/disable computation of range (max-min)",1);
    ct->setField("maxpos","1/0=enable/disable computation of pos. of max. val.",1);
    ct->setField("minpos","1/0=enable/disable computation of pos. of min. val.",1);
    ct->setField("amean","1/0=enable/disable computation of arithmetic mean",0);
    ct->setField("maxameandist","1/0=enable/disable computation of (max-amean)",1);
    ct->setField("minameandist","1/0=enable/disable computation of (amean-min)",1);
  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalExtremes);
}

SMILECOMPONENT_CREATE(cFunctionalExtremes)

//-----

cFunctionalExtremes::cFunctionalExtremes(const char *_name) :
  cFunctionalComponent(_name,N_FUNCTS,extremesNames)
{
}

void cFunctionalExtremes::fetchConfig()
{
  if (getInt("max")) enab[FUNCT_MAX] = 1;
  if (getInt("min")) enab[FUNCT_MIN] = 1;
  if (getInt("range")) enab[FUNCT_RANGE] = 1;
  if (getInt("maxpos")) enab[FUNCT_MAXPOS] = 1;
  if (getInt("minpos")) enab[FUNCT_MINPOS] = 1;
  if (getInt("amean")) enab[FUNCT_AMEAN] = 1;
  if (getInt("maxameandist")) enab[FUNCT_MAXAMEANDIST] = 1;
  if (getInt("minameandist")) enab[FUNCT_MINAMEANDIST] = 1;

  cFunctionalComponent::fetchConfig();
}

long cFunctionalExtremes::process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout)
{
  int i;
  if ((Nin>0)&&(out!=NULL)) {
    long minpos=-1, maxpos=-1;
    
    for (i=0; i<Nin; i++) {
      if ((*in == max)&&(maxpos==-1)) { maxpos=i; }
      if ((*in == min)&&(minpos==-1)) { minpos=i; }
      in++;
    }

    int n=0;
    if (enab[FUNCT_MAX]) out[n++]=max;
    if (enab[FUNCT_MIN]) out[n++]=min;
    if (enab[FUNCT_RANGE]) out[n++]=max-min;
    if (enab[FUNCT_MAXPOS]) out[n++]=(FLOAT_DMEM)maxpos;
    if (enab[FUNCT_MINPOS]) out[n++]=(FLOAT_DMEM)minpos;
    if (enab[FUNCT_AMEAN]) out[n++]=mean;
    if (enab[FUNCT_MAXAMEANDIST]) out[n++]=max-mean;
    if (enab[FUNCT_MINAMEANDIST]) out[n++]=mean-min;
    return n;
  }
  return 0;
}

/*
long cFunctionalExtremes::process(INT_DMEM *in, INT_DMEM *inSorted, INT_DMEM *out, long Nin, long Nout)
{

  return 0;
}
*/

cFunctionalExtremes::~cFunctionalExtremes()
{
}

