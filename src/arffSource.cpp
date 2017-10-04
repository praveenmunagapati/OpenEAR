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

example dataSource
writes data to data memory...

*/


#include <arffSource.hpp>
#define MODULE "cArffSource"

/*Library:
sComponentInfo * registerMe(cConfigManager *_confman) {
  cDataSource::registerComponent(_confman);
}
*/
#define N_ALLOC_BLOCK 50

SMILECOMPONENT_STATICS(cArffSource)

//sComponentInfo * cArffSource::registerComponent(cConfigManager *_confman)
SMILECOMPONENT_REGCOMP(cArffSource)
{
  SMILECOMPONENT_REGCOMP_INIT
/*  if (_confman == NULL) return NULL;
  int rA = 0;
  sconfman = _confman;
*/
  scname = COMPONENT_NAME_CARFFSOURCE;
  sdescription = COMPONENT_DESCRIPTION_CARFFSOURCE;

  // we inherit cDataSource configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataSource")
/*
  ConfigType *ct = new ConfigType( *(sconfman->getTypeObj("cDataSource")) , scname );
  if (ct == NULL) {
    SMILE_WRN(4,"cDataSource config Type not found!");
    rA=1;
  }*/
  
  SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("filename","arff file to read","input.arff");
    ct->setField("skipClasses","number of numeric(!) attributes (values) at end of each instance to skip",0);
  )

/*
    ConfigInstance *Tdflt = new ConfigInstance( scname, ct, 1 );
    sconfman->registerType(Tdflt);
  } else {
    if (ct!=NULL) delete ct;
  }
*/
  SMILECOMPONENT_MAKEINFO(cArffSource);
}

SMILECOMPONENT_CREATE(cArffSource)

//-----

cArffSource::cArffSource(const char *_name) :
  cDataSource(_name),
  field(NULL),
  fieldNalloc(0),
  lineNr(0),
  eof(0)
{
}

void cArffSource::fetchConfig()
{
  cDataSource::fetchConfig();
  
  filename = getStr("filename");
  SMILE_DBG(2,"filename = '%s'",filename);
  skipClasses = getInt("skipClasses");
  SMILE_DBG(2,"skipClasses = %i",skipClasses);
}

/*
int cArffSource::myConfigureInstance()
{
    // call writer->setConfig here to set the dataMemory level configuration and override config file and defaults...
//  double T = 1.0 / (double)(pcmParam.sampleRate);
//  writer->setConfig(1,2*buffersize,T, 0.0, 0.0 , 0, DMEM_FLOAT);  // lenSec = (double)(2*buffersize)*T

  int ret = 1;
  ret *= cDataSource::myConfigureInstance();
  // ...
  return ret;
}
*/

int cArffSource::setupNewNames(long nEl)
{
  // read header lines...
  int ret=1;
  size_t n,read;
  char *line=NULL;
  int head=1;
  int fnr = 0;
  int nnr = 0;
  do {
    read = getline(&line, &n, filehandle);
    if ((read > 0)&&(line!=NULL)) {
      lineNr++;
      if ( (!strncmp(line,"@attribute ",11))||(!strncmp(line,"@Attribute ",11))||(!strncmp(line,"@ATTRIBUTE ",11)) ) {
        char *name = line+11;
        char *type = strchr(name,' ');
        if (type != NULL) {
          *(type++) = 0;
          if (!strncmp(type,"numeric",7)) { // add numeric attribute:
            writer->addField(name,1);
            if (fnr >= fieldNalloc) {
              field = (int*)crealloc( field, sizeof(int)*(fieldNalloc+N_ALLOC_BLOCK), sizeof(int)*(fieldNalloc) );
              fieldNalloc += N_ALLOC_BLOCK;
            }
            field[fnr] = 1;
            nnr++;

            // TODO: detect array fields [X]
          }
          fnr++;

        } else { // ERROR:...
          ret=0;
        }
      } else if ( (!strncmp(line,"@data",5))||(!strncmp(line,"@Data",5))||(!strncmp(line,"@DATA",5)) ) {
        head = 0;
      }
    } else {
      head = 0; eof=1;
      SMILE_ERR(1,"incomplete arff file '%s', could not find '@data' line!",filename);
      ret=0;
    } // ERROR: EOF in header!!!
  } while (head);
  if (line!=NULL) free(line);

  // skip 'skipClasses' numeric classes from end
  if (skipClasses) {
    int i;
    int s=skipClasses;
    for (i=fnr-1; i>=0; i++) {
      if (field[i]) { field[i]=0; s--; nnr--; }
      if (s<=0) break;
    }
  }
 
  nFields = fnr;
  nNumericFields = nnr;

  allocVec(nnr);

  namesAreSet=1;
  return 1;
}

int cArffSource::myFinaliseInstance()
{
  filehandle = fopen(filename, "r");
  if (filehandle == NULL) {
    COMP_ERR("Error opening file '%s' for reading (component instance '%s', type '%s')",filename, getInstName(), getTypeName());
  }

  int ret = cDataSource::myFinaliseInstance();

  if (ret == 0) {
    fclose(filehandle); filehandle = NULL;
  }
  return ret;
  
}


int cArffSource::myTick(long long t)
{
  if (isEOI()) return 0;
  
  SMILE_DBG(4,"tick # %i, reading value vector from arff file",t);
  if (eof) {
    SMILE_DBG(4,"(inst '%s') EOF, no more data to read",getInstName());
    return 0;
  }

  if (!(writer->checkWrite(1))) return 0;
  
  size_t n,read;
  char *line=NULL;
  int l=1;
  int len=0;
  int i=0, ncnt=0;
  // get next non-empty line
  do {
    read = getline(&line, &n, filehandle);
    if ((read != -1)&&(line!=NULL)) {
      lineNr++;
      len=strlen(line);
      if (len>0) { if (line[len-1] == '\n') { line[len-1] = 0; len--; } }
      if (len>0) { if (line[len-1] == '\r') { line[len-1] = 0; len--; } }
      while (((line[0] == ' ')||(line[0] == '\t'))&&(len>=0)) { line[0] = 0; line++; len--; }
      if (len > 0) {
        l=0;
        char *x, *x0=line;
        do {
          x = strchr(x0,',');
          if (x!=NULL) {
            *(x++)=0;
          }
          if (field[i]) { // if this field is numeric
            // convert value in x0
            char *ep=NULL;
            double val = strtod( x0, &ep );
            if ((val==0.0)&&(ep==x0)) { SMILE_ERR(1,"error parsing value in arff file '%s' (line %i), expected double value (element %i).",filename,lineNr,i); }
            vec->dataF[ncnt++] = (FLOAT_DMEM)val;
          }
          if (x!=NULL) {
            i++;
            x0=x;
          }
        } while (x!=NULL);
      }
    } else {
      l=0; // EOF....  signal EOF...???
      eof=1;
    }
  } while (l);

  if (!eof) {
    writer->setNextFrame(vec);
    return 1;
  }
  
  return 0;
}


cArffSource::~cArffSource()
{
  if (filehandle!=NULL) fclose(filehandle);
}
