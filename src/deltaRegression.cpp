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


/*  openSMILE component: delta coefficients

compute delta regression using regression formula

*/



#include <deltaRegression.hpp>
//#include <math.h>

#define MODULE "cDeltaRegression"


SMILECOMPONENT_STATICS(cDeltaRegression)

SMILECOMPONENT_REGCOMP(cDeltaRegression)
{
  SMILECOMPONENT_REGCOMP_INIT
  scname = COMPONENT_NAME_CDELTAREGRESSION;
  sdescription = COMPONENT_DESCRIPTION_CDELTAREGRESSION;

  // we inherit cWindowProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cWindowProcessor")
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("nameAppend", NULL, "de");
    ct->setField("deltawin","half size of delta regression window (0=special case: simple difference x[n]-x[n-1])",2);
    ct->setField("blocksize", NULL , 1);
  )
  SMILECOMPONENT_MAKEINFO(cDeltaRegression);
}

SMILECOMPONENT_CREATE(cDeltaRegression)

//-----

cDeltaRegression::cDeltaRegression(const char *_name) :
  cWindowProcessor(_name),
  deltawin(0),
  norm(1.0)
{
}


void cDeltaRegression::fetchConfig()
{
  cWindowProcessor::fetchConfig();
  
  deltawin = getInt("deltawin");
  SMILE_DBG(2,"deltawin = %i",deltawin);
  if (deltawin < 0) {
    SMILE_ERR(1,"deltawin must be >= 0 ! (setting to 0)");
    deltawin = 0;
  }
  
  int i;
  norm = 0.0;
  for (i=1; i<=deltawin; i++) norm += (FLOAT_DMEM)i*(FLOAT_DMEM)i;
  norm *= 2.0;
  
  if (deltawin > 0) {
    setWindow(deltawin,deltawin);
  } else {
    setWindow(1,0);
  }
}

/*
int cDeltaRegression::setupNamesForField(int i, const char*name, long nEl)
{
  char *tmp = myvprint("%s_de",name);
  writer->addField( name, nEl );
  return nEl;
}
*/

// order is the amount of memory (overlap) that will be present in _in
// buf will have nT timesteps, however also order negative indicies (i.e. you may access a negative array index up to -order!)
int cDeltaRegression::processBuffer(cMatrix *_in, cMatrix *_out, int _pre, int _post )
{
  long n;
  int i;
  FLOAT_DMEM num;
  if (_in->type!=DMEM_FLOAT) COMP_ERR("dataType (%i) != DMEM_FLOAT not yet supported!",_in->type);
  FLOAT_DMEM *x=_in->dataF;
  FLOAT_DMEM *y=_out->dataF;

  if (deltawin > 0) {
    for (n=0; n<_out->nT; n++) {
      num = 0.0;
      for (i=1; i<=deltawin; i++) num += (FLOAT_DMEM)i * (x[n+i] - x[n-i]);
      y[n] = num / norm;
    }
  } else { // simple difference
    for (n=0; n<_out->nT; n++) {
      y[n] = x[n]-x[n-1];
    }
  }
  return 1;
}


cDeltaRegression::~cDeltaRegression()
{
}

