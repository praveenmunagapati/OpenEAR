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


/*  SmileUtil
    =========

contains modular DSP functions
and other utility functions

*/


#ifndef __SMILE_UTIL_H
#define __SMILE_UTIL_H

/* you may remove this include if you are using smileUtil outside of openSMILE */
//#include <smileCommon.hpp>
/* --------------------------------------------------------------------------- */

#ifndef __SMILE_COMMON_H

#ifdef _MSC_VER // Visual Studio specific macro
  #ifdef BUILDING_DLL
    #define DLLEXPORT __declspec(dllexport)
    #define class class __declspec(dllexport)
  #else
    #define DLLEXPORT __declspec(dllimport)
    #define class class __declspec(dllimport)
  #endif
  #define DLLLOCAL 
#else 
    #define DLLEXPORT 
    #define DLLLOCAL  
#endif

#include <stdlib.h>

#endif  // __SMILE_COMMON_H


#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************************
 ***********************=====   Sort functions   ===== **************************************
 *******************************************************************************************/

/* QuickSort algorithm for a double array with nEl elements */
DLLEXPORT void smileUtil_quickSort_double(double *arr, long nEl);

/* QuickSort algorithm for a float array with nEl elements */
DLLEXPORT void smileUtil_quickSort_float(float *arr, long nEl);



/*******************************************************************************************
 ***********************=====   Math functions   ===== **************************************
 *******************************************************************************************/

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

/* check if number x is power of 2 (positive or negative) */
DLLEXPORT long smileMath_isPowerOf2(long x);

/* round x to nearest power of two */
DLLEXPORT long smileMath_roundToNextPowOf2(long x);

/* round up x to next power of 2 */
DLLEXPORT long smileMath_ceilToNextPowOf2(long x);

/* round down x to next power of two */
DLLEXPORT long smileMath_floorToNextPowOf2(long x);


/*******************************************************************************************
 ***********************=====   DSP functions   ===== **************************************
 *******************************************************************************************/

  /***** window functions *****/

/* definition of window function types */
#define WINF_HANNING    0
#define WINF_HAMMING    1
#define WINF_RECTANGLE  2
#define WINF_RECTANGULAR 2
#define WINF_SINE       3
#define WINF_COSINE     3
#define WINF_GAUSS      4
#define WINF_TRIANGULAR 5
#define WINF_TRIANGLE   5
#define WINF_BARTLETT   6
#define WINF_LANCZOS    7
#define WINF_BARTHANN   8
#define WINF_BLACKMAN   9
#define WINF_BLACKHARR  10
#define WINF_TRIANGULAR_POWERED 11
#define WINF_TRIANGLE_POWERED   11


#ifdef _MSC_VER // Visual Studio specific macro
#define inline __inline
#endif

/* sinc function (modified) : (sin 2pi*x) / x */
inline double smileDsp_lcSinc(double x)
{
  double y = M_PI * x;
  return sin(y)/(y);
}

/* sinc function : (sin x) / x  */
inline double smileDsp_sinc(double x)
{
  return sin(x)/(x);
}

#ifdef _MSC_VER // Visual Studio specific macro
#undef inline
#endif

/* Rectangular window */
DLLEXPORT double * smileDsp_winRec(long _N);

/* Triangular window (non-zero endpoint) */
DLLEXPORT double * smileDsp_winTri(long _N);

/* Powered triangular window (non-zero endpoint) */
DLLEXPORT double * smileDsp_winTrP(long _N);

/* Bartlett window (triangular, zero endpoint) */
DLLEXPORT double * smileDsp_winBar(long _N);

/* Hann window */
DLLEXPORT double * smileDsp_winHan(long _N);

/* Hamming window */
DLLEXPORT double * smileDsp_winHam(long _N);

/* Sine window */
DLLEXPORT double * smileDsp_winSin(long _N);

/* Gauss window */
DLLEXPORT double * smileDsp_winGau(long _N, double sigma);

/* Lanczos window */
DLLEXPORT double * smileDsp_winLac(long _N);

/* Blackman window */
DLLEXPORT double * smileDsp_winBla(long _N, double alpha0, double alpha1, double alpha2);

/* Bartlett-Hann window */
DLLEXPORT double * smileDsp_winBaH(long _N, double alpha0, double alpha1, double alpha2);

/* Blackman-Harris window */
DLLEXPORT double * smileDsp_winBlH(long _N, double alpha0, double alpha1, double alpha2, double alpha3);

#ifdef __cplusplus
}
#endif

#endif  // __SMILE_UTIL_H
