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


/*  openSMILE component: preemphasis

simple preemphasis : x(t) = x(t) - k*x(t-1)

*/


#include <preemphasis.hpp>
//#include <math.h>

#define MODULE "cPreemphasis"


SMILECOMPONENT_STATICS(cPreemphasis)

SMILECOMPONENT_REGCOMP(cPreemphasis)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CPREEMPHASIS;
  sdescription = COMPONENT_DESCRIPTION_CPREEMPHASIS;

  // we inherit cWindowProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cWindowProcessor")

  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("k","pre-emphasis coefficient y[n] = x[n] - k*x[n-1]",0.97);
  )

  SMILECOMPONENT_MAKEINFO(cPreemphasis);
}

SMILECOMPONENT_CREATE(cPreemphasis)

//-----

cPreemphasis::cPreemphasis(const char *_name) :
  cWindowProcessor(_name,1,0)
{
}


void cPreemphasis::fetchConfig()
{
  cWindowProcessor::fetchConfig();
  
  k = (FLOAT_DMEM)getDouble("k");
  SMILE_DBG(2,"k = %f",k);
  if ((k<0.0)||(k>1.0)) {
    SMILE_ERR(1,"k must be in the range [0;1]! Setting k=0.0 !");
    k=0.0;
  }
}

// order is the amount of memory (overlap) that will be present in _in
// buf will have nT timesteps, however also order negative indicies (i.e. you may access a negative array index up to -order!)
int cPreemphasis::processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post )
{
  long n;

  if (_in->type!=DMEM_FLOAT) COMP_ERR("dataType (%i) != DMEM_FLOAT not yet supported!",_in->type);
  FLOAT_DMEM *x=_in->dataF;
  FLOAT_DMEM *y=_out->dataF;
  for (n=0; n<_out->nT; n++) {
    *(y++) = *(x) - k * *(x-1);
    x++;
  }
  return 1;
}


cPreemphasis::~cPreemphasis()
{
}

