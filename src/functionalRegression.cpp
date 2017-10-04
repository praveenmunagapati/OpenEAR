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

functionals: linear and quadratic regression coefficients

*/


#include <functionalRegression.hpp>

#define MODULE "cFunctionalRegression"


#define FUNCT_LINREGC1     0
#define FUNCT_LINREGC2     1
#define FUNCT_LINREGERRA   2
#define FUNCT_LINREGERRQ   3
#define FUNCT_QREGC1       4
#define FUNCT_QREGC2       5
#define FUNCT_QREGC3       6
#define FUNCT_QREGERRA     7
#define FUNCT_QREGERRQ     8
#define FUNCT_CENTROID     9

#define N_FUNCTS  10

#define NAMES  "linregc1","linregc2","linregerrA","linregerrQ","qregc1","qregc2","qregc3","qregerrA","qregerrQ","centroid"

const char *regressionNames[] = {NAMES};  // change variable name to your clas...

SMILECOMPONENT_STATICS(cFunctionalRegression)

SMILECOMPONENT_REGCOMP(cFunctionalRegression)
{
  SMILECOMPONENT_REGCOMP_INIT
  
  scname = COMPONENT_NAME_CFUNCTIONALREGRESSION;
  sdescription = COMPONENT_DESCRIPTION_CFUNCTIONALREGRESSION;

  // configure your component's configType:
  SMILECOMPONENT_CREATE_CONFIGTYPE
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("linregc1","1/0=enable/disable computation of slope",1);
    ct->setField("linregc2","1/0=enable/disable computation of offset",1);
    ct->setField("linregerrA","1/0=enable/disable computation of linear linear regression error",1);
    ct->setField("linregerrQ","1/0=enable/disable computation of quadratic linear regression error",1);
    ct->setField("qregc1","1/0=enable/disable computation of quadratic regression coefficient 1",1);
    ct->setField("qregc2","1/0=enable/disable computation of quadratic regression coefficient 2",1);
    ct->setField("qregc3","1/0=enable/disable computation of quadratic regression coefficient 3 (offset)",1);
    ct->setField("qregerrA","1/0=enable/disable computation of linear quadratic regression error",1);
    ct->setField("qregerrQ","1/0=enable/disable computation of quadratic quadratic regression error",1);
    ct->setField("centroid","1/0=enable/disable computation of centroid",1);

  )

  SMILECOMPONENT_MAKEINFO_NODMEM(cFunctionalRegression);
}

SMILECOMPONENT_CREATE(cFunctionalRegression)

//-----

cFunctionalRegression::cFunctionalRegression(const char *_name) :
  cFunctionalComponent(_name,N_FUNCTS,regressionNames),
  enQreg(0)
{
}

void cFunctionalRegression::fetchConfig()
{
  if (getInt("linregc1")) enab[FUNCT_LINREGC1] = 1;
  if (getInt("linregc2")) enab[FUNCT_LINREGC2] = 1;
  if (getInt("linregerrA")) enab[FUNCT_LINREGERRA] = 1;
  if (getInt("linregerrQ")) enab[FUNCT_LINREGERRQ] = 1;
  if (getInt("qregc1")) { enab[FUNCT_QREGC1] = 1; enQreg=1; }
  if (getInt("qregc2")) { enab[FUNCT_QREGC2] = 1; enQreg=1; }
  if (getInt("qregc3")) { enab[FUNCT_QREGC3] = 1; enQreg=1; }
  if (getInt("qregerrA")) { enab[FUNCT_QREGERRA] = 1; enQreg=1; }
  if (getInt("qregerrQ")) { enab[FUNCT_QREGERRQ] = 1; enQreg=1; }
  if (getInt("centroid")) { enab[FUNCT_CENTROID] = 1; enQreg=1; }

  cFunctionalComponent::fetchConfig();
}

long cFunctionalRegression::process(FLOAT_DMEM *in, FLOAT_DMEM *inSorted, FLOAT_DMEM min, FLOAT_DMEM max, FLOAT_DMEM mean, FLOAT_DMEM *out, long Nin, long Nout)
{
  if ((Nin>0)&&(out!=NULL)) {
    //compute centroid
    FLOAT_DMEM *iE=in+Nin, *i0=in;
    FLOAT_DMEM Nind = (FLOAT_DMEM)Nin;
    
    FLOAT_DMEM num=0.0, num2=0.0;
    FLOAT_DMEM tmp,asum=mean*Nind;
    FLOAT_DMEM ii=0.0;
    while (in<iE) {
//      asum += *(in);
      tmp = *(in++) * ii;
      num += tmp;
      tmp *= ii;
      ii += 1.0;
      num2 += tmp;
    }

    FLOAT_DMEM centroid;
    if (asum != 0.0)
      centroid = num / ( asum * Nind);
    else
      centroid = 0.0;

    in=i0;

    
    FLOAT_DMEM m,t,leq=0.0,lea=0.0;
    FLOAT_DMEM a,b,c,qeq=0.0,qea=0.0;
    FLOAT_DMEM S1,S2,S3,S4;
    if (Nin > 1) {
      // LINEAR REGRESSION:
/*
      S1 = (Nind-1.0)*Nind/2.0;  // sum of all i=0..N-1
      S2 = (Nind-1.0)*Nind*(2.0*Nind-1.0)/6.0; // sum of all i^2 for i=0..N-1
                                              // num: if sum of y_i*i for all i=0..N-1
      t = ( asum - num*S1/S2) / ( Nind - S1*S1/S2 );
      m = ( num - t * S1 ) / S2;
*/ // optimised computation:
      FLOAT_DMEM NNm1 = (Nind)*(Nind-(FLOAT_DMEM)1.0);

      S1 = NNm1/(FLOAT_DMEM)2.0;  // sum of all i=0..N-1
      S2 = NNm1*((FLOAT_DMEM)2.0*Nind-(FLOAT_DMEM)1.0)/(FLOAT_DMEM)6.0; // sum of all i^2 for i=0..N-1

      FLOAT_DMEM S1dS2 = S1/S2;
      t = ( asum - num*S1dS2) / ( Nind - S1*S1dS2 );
      m = ( num - t * S1 ) / S2;

      S3 = S1*S1;
      FLOAT_DMEM Nind1 = Nind-(FLOAT_DMEM)1.0;
      S4 = S2 * ((FLOAT_DMEM)3.0*(Nind1*Nind1 + Nind1)-(FLOAT_DMEM)1.0) / (FLOAT_DMEM)5.0;

      // QUADRATIC REGRESSION:
      if (enQreg) {

        FLOAT_DMEM det;
        FLOAT_DMEM S3S3 = S3*S3;
        FLOAT_DMEM S2S2 = S2*S2;
        FLOAT_DMEM S1S2 = S1*S2;
        FLOAT_DMEM S1S1 = S3;
        det = S4*S2*Nind + (FLOAT_DMEM)2.0*S3*S1S2 - S2S2*S2 - S3S3*Nind - S1S1*S4;
      
        if (det != 0.0) {
          a = ( (S2*Nind - S1S1)*num2 + (S1S2 - S3*Nind)*num + (S3*S1 - S2S2)*asum ) / det;
          b = ( (S1S2 - S3*Nind)*num2 + (S4*Nind - S2S2)*num + (S3*S2 - S4*S1)*asum ) / det;
          c = ( (S3*S1 - S2S2)*num2 + (S3*S2-S4*S1)*num + (S4*S2 - S3S3)*asum ) / det;
        } else {
          a=0.0; b=0.0; c=0.0;
        }

      }
//    printf("nind:%f  S1=%f,  S2=%f  S3=%f  S4=%f  num2=%f  num=%f  asum=%f t=%f\n",Nind,S1,S2,S3,S4,num2,num,asum,t);
    } else {
      m = 0; t=c=*in;
      a = 0.0; b=0.0;
    }
    
    // linear regression error:
    ii=0.0; FLOAT_DMEM e;
    while (in<iE) {
      e = *(in++) - (m*ii + t);
      lea += fabs(e);
      ii += 1.0;
      leq += e*e;
    }
    in=i0;

    // quadratic regresssion error:
    if (enQreg) {
      ii=0.0; FLOAT_DMEM e;
      while (in<iE) {
        e = *(in++) - (a*ii*ii + b*ii + c);
        qea += fabs(e);
        ii += 1.0;
        qeq += e*e;
      }
      in=i0;
    }

    int n=0;
    if (enab[FUNCT_LINREGC1]) out[n++]=m;
    if (enab[FUNCT_LINREGC2]) out[n++]=t;
    if (enab[FUNCT_LINREGERRA]) out[n++]=lea/Nind;
    if (enab[FUNCT_LINREGERRQ]) out[n++]=leq/Nind;

    if (enab[FUNCT_QREGC1]) out[n++]=a;
    if (enab[FUNCT_QREGC2]) out[n++]=b;
    if (enab[FUNCT_QREGC3]) out[n++]=c;
    if (enab[FUNCT_QREGERRA]) out[n++]=qea;
    if (enab[FUNCT_QREGERRQ]) out[n++]=qeq;

    if (enab[FUNCT_CENTROID]) out[n++]=centroid;

    return n;
  }
  return 0;
}

/*
long cFunctionalRegression::process(INT_DMEM *in, INT_DMEM *inSorted, INT_DMEM *out, long Nin, long Nout)
{

  return 0;
}
*/

cFunctionalRegression::~cFunctionalRegression()
{
}

