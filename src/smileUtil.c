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

/*

Smile Util:

contains modular DSP functions
and other utility functions

*/

#include <smileUtil.h>


/*******************************************************************************************
 ***********************=====   Sort functions   ===== **************************************
 *******************************************************************************************/

/* QuickSort algorithm for a float array with nEl elements */
void smileUtil_quickSort_float(float *arr, long nEl)
{
  #ifdef MAX_LEVELS
  #undef MAX_LEVELS
  #endif
  #define  MAX_LEVELS  300

  float piv;
  long beg[MAX_LEVELS], end[MAX_LEVELS],swap;
  long i=0, L, R;

  beg[0]=0; end[0]=nEl;
  while (i>=0) {
    L=beg[i]; R=end[i]-1;
    if (L<R) {
      piv=arr[L]; 
      while (L<R) {
        while (arr[R]>=piv && L<R) R--; if (L<R) arr[L++]=arr[R];
        while (arr[L]<=piv && L<R) L++; if (L<R) arr[R--]=arr[L]; }
      arr[L]=piv; beg[i+1]=L+1; end[i+1]=end[i]; end[i++]=L;
      if (end[i]-beg[i]>end[i-1]-beg[i-1]) {
        swap=beg[i]; beg[i]=beg[i-1]; beg[i-1]=swap;
        swap=end[i]; end[i]=end[i-1]; end[i-1]=swap;
      }
    } else { i--; }
  }
}

/* QuickSort algorithm for a double array with nEl elements */
void smileUtil_quickSort_double(double *arr, long nEl)
{
  #ifndef MAX_LEVELS
  #define MAX_LEVELS  300
  #endif

  double piv;
  long beg[MAX_LEVELS], end[MAX_LEVELS],swap;
  long i=0, L, R;
  
  beg[0]=0; end[0]=nEl;
  while (i>=0) {
    L=beg[i]; R=end[i]-1;
    if (L<R) {
      piv=arr[L];
      while (L<R) {
        while (arr[R]>=piv && L<R) R--; if (L<R) arr[L++]=arr[R];
        while (arr[L]<=piv && L<R) L++; if (L<R) arr[R--]=arr[L]; }
      arr[L]=piv; beg[i+1]=L+1; end[i+1]=end[i]; end[i++]=L;
      if (end[i]-beg[i]>end[i-1]-beg[i-1]) {
        swap=beg[i]; beg[i]=beg[i-1]; beg[i-1]=swap;
        swap=end[i]; end[i]=end[i-1]; end[i-1]=swap;
      }
    } else { i--; }
  }
}


/*******************************************************************************************
 ***********************=====   Math functions   ===== **************************************
 *******************************************************************************************/

/* check if number is power of 2 (positive or negative) */
long smileMath_isPowerOf2(long x)
{
  if (x==1) return 1;  // 1 is a power of 2
  if (((x&1) == 0)&&(x != 0)) { // only even numbers > 1
    x=x>>1;
    while ((x&1) == 0) { x=x>>1;  }
    return ((x==1)||(x==-1));
  }
  return 0;
}

/* round to nearest power of two */
long smileMath_roundToNextPowOf2(long x)
{
  // round x up to nearest power of 2
  unsigned long int flng = (unsigned long int)x;
  unsigned long int fmask = 0x8000;
  while ( (fmask & flng) == 0) { fmask = fmask >> 1; }
  // fmask now contains the MSB position
  if (fmask > 1) {
    if ( (fmask>>1)&flng ) { flng = fmask<<1; }
    else { flng = fmask; }
  } else {
    flng = 2;
  }

  return (long)flng;
}

/* round up to next power of 2 */
long smileMath_ceilToNextPowOf2(long x)
{
  long y = smileMath_roundToNextPowOf2(x);
  if (y<x) y *= 2;
  return y;
}

/* round down to next power of two */
long smileMath_floorToNextPowOf2(long x)
{
  long y = smileMath_roundToNextPowOf2(x);
  if (y>x) y /= 2;
  return y;
}

/*******************************************************************************************
 ***********************=====   DSP functions   ===== **************************************
 *******************************************************************************************/

  /*======= window functions ==========*/

/* rectangular window */
double * smileDsp_winRec(long _N)
{
  int i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  for (i=0; i<_N; i++) {
    *x = 1.0; x++;
  }
  return ret;
}

/* triangular window (non-zero endpoints) */
double * smileDsp_winTri(long _N)
{
  long i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  for (i=0; i<_N/2; i++) {
    *x = 2.0*(double)(i+1)/(double)_N;
    x++;
  }
  for (i=_N/2; i<_N; i++) {
    *x = 2.0*(double)(_N-i)/(double)_N;
    x++;
  }
  return ret;
}

/* powered triangular window (non-zero endpoints) */
double * smileDsp_winTrP(long _N)
{
  double *w = smileDsp_winTri(_N);
  double *x = w;
  long n; for (n=0; n<_N; n++) *x = *x * (*(x++));
  return w;
}

/* bartlett (triangular) window (zero endpoints) */
double * smileDsp_winBar(long _N)
{
  long i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  for (i=0; i<_N/2; i++) {
    *x = 2.0*(double)(i)/(double)(_N-1);
    x++;
  }
  for (i=_N/2; i<_N; i++) {
    *x = 2.0*(double)(_N-1-i)/(double)(_N-1);
    x++;
  }
  return ret;
}

/* hann(ing) window */
double * smileDsp_winHan(long _N)
{
  double i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  double NN = (double)_N;
  for (i=0.0; i<NN; i += 1.0) {
    *x = 0.5*(1.0-cos( (2.0*M_PI*i)/(NN-1.0) ));
    x++;
  }
  return ret;
}

/* hamming window */
double * smileDsp_winHam(long _N)
{
  double i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  double NN = (double)_N;
  for (i=0.0; i<NN; i += 1.0) {
/*    *x = 0.53836 - 0.46164 * cos( (2.0*M_PI*i)/(NN-1.0) ); */
    *x = 0.54 - 0.46 * cos( (2.0*M_PI*i)/(NN-1.0) );
    x++;
  }
  return ret;
}

/* half-wave sine window (cosine window) */
double * smileDsp_winSin(long _N)
{
  double i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  double NN = (double)_N;
  for (i=0.0; i<NN; i += 1.0) {
    *x = sin( (1.0*M_PI*i)/(NN-1.0) );
    x++;
  }
  return ret;
}

/* Lanczos window */
double * smileDsp_winLac(long _N)
{
  double i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  double NN = (double)_N;
  for (i=0.0; i<NN; i += 1.0) {
    *x = smileDsp_lcSinc( (2.0*i)/(NN-1.0) - 1.0 );
    x++;
  }
  return ret;
}

/* gaussian window ...??? */
double * smileDsp_winGau(long _N, double sigma)
{
  double i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  double NN = (double)_N;
  double tmp;
  if (sigma <= 0.0) sigma = 0.01;
  if (sigma > 0.5) sigma = 0.5;
  for (i=0.0; i<NN; i += 1.0) {
    tmp = (i-(NN-1.0)/2.0)/(sigma*(NN-1.0)/2.0);
    *x = exp( -0.5 * ( tmp*tmp ) );
    x++;
  }
  return ret;
}

/* Blackman window */
double * smileDsp_winBla(long _N, double alpha0, double alpha1, double alpha2)
{
  double i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  double NN = (double)_N;
  double tmp;
  for (i=0.0; i<NN; i += 1.0) {
    tmp = (2.0*M_PI*i)/(NN-1.0);
    *x = alpha0 - alpha1 * cos( tmp ) + alpha2 * cos( 2.0*tmp );
    x++;
  }
  return ret;

}

/* Bartlett-Hann window */
double * smileDsp_winBaH(long _N, double alpha0, double alpha1, double alpha2)
{
  double i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  double NN = (double)_N;
  for (i=0.0; i<NN; i += 1.0) {
    *x = alpha0 - alpha1 * fabs( i/(NN-1.0) - 0.5 ) - alpha2 * cos( (2.0*M_PI*i)/(NN-1.0) );
    x++;
  }
  return ret;
}

/* Blackman-Harris window */
double * smileDsp_winBlH(long _N, double alpha0, double alpha1, double alpha2, double alpha3)
{
  double i;
  double * ret = (double *)malloc(sizeof(double)*_N);
  double * x = ret;
  double NN = (double)_N;
  double tmp;
  for (i=0.0; i<NN; i += 1.0) {
    tmp = (2.0*M_PI*i)/(NN-1.0);
    *x = alpha0 - alpha1 * cos( tmp ) + alpha2 * cos( 2.0*tmp ) - alpha3 * cos( 3.0*tmp );
    x++;
  }
  return ret;
}
