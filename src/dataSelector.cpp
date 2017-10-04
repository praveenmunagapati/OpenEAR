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

selects data elements by element name

*/


#include <dataSelector.hpp>

#define MODULE "cDataSelector"

SMILECOMPONENT_STATICS(cDataSelector)

SMILECOMPONENT_REGCOMP(cDataSelector)
{
  SMILECOMPONENT_REGCOMP_INIT

  scname = COMPONENT_NAME_CDATASELECTOR;
  sdescription = COMPONENT_DESCRIPTION_CDATASELECTOR;

  // we inherit cDataProcessor configType and extend it:
  SMILECOMPONENT_INHERIT_CONFIGTYPE("cDataProcessor")

    SMILECOMPONENT_IFNOTREGAGAIN(
    ct->setField("selected","array of exact (case-sensitive) names of features / data elements to select",(const char *)NULL, ARRAY_TYPE);
  //--  TODO: --
  ct->setField("newNames","array of new names of selected features / data elements (optional)",(const char *)NULL, ARRAY_TYPE);
  //-----
  ct->setField("elementMode","1=select elements exactly as given in 'selected' array / 0 = automatically copy arrays or partial arrays, e.g. if field[1-4] or only 'field' is given as name",1);
  )

    SMILECOMPONENT_MAKEINFO(cDataSelector);
}


SMILECOMPONENT_CREATE(cDataSelector)

//-----

cDataSelector::cDataSelector(const char *_name) :
cDataProcessor(_name),
names(NULL),
mapping(NULL),
elementMode(0),
vecO(NULL)
{
}

void cDataSelector::fetchConfig()
{
  cDataProcessor::fetchConfig();

  // load names of selected features:
  nSel = getArraySize("selected");
  if (nSel>0) {

    names = (const char**)calloc(1,sizeof(char*) * nSel);

    int i;
    for (i=0; i<nSel; i++) {
      names[i] = getStr_f(myvprint("selected[%i]",i));
      SMILE_IDBG(2,"selected: '%s'",names[i]);
    }


  }

  elementMode = getInt("elementMode");
  if (elementMode) SMILE_IDBG(2,"exact element name matching enabled");

}

/*
int cDataSelector::myConfigureInstance()
{
  int ret=1;
  // if you would like to change the write level parameters... do so HERE:

  //
  // .. or override configureWriter() to do so, after the reader config is available!
  //
  ret *= cDataProcessor::myConfigureInstance();
  return ret;
}
*/

/*
int cDataSelector::configureWriter(const sDmLevelConfig *c)
{

// we must return 1, in order to indicate configure success (0 indicates failure)
return 1;
}
*/

// TODO!!!!!!!!!!   -> setupNewNames instead of finaliseInstance

// field selection -> fields (arrays) will be added as arrays in output
// element selection -> every output field will be one element
// how to choose the mode?? 
// --> a) if array index is given -> element selection
// --> b) if no array index for an array is given (field selection, select full field)
// --> c) if array index contains "-", assume range -> field selection (partial)

int cDataSelector::setupNewNames(long nEl)
{
  // match our list of selected names to the list of names obtained from the reader
  int i,j;
  if (elementMode) {
    int _N = reader->getLevelN();  
    nElSel = 0;
    mapping = (sDataSelectorSelData *)calloc(1,sizeof(sDataSelectorSelData) * nSel);
    for (i=0; i<_N; i++) {
      char * tmp = reader->getElementName(i);
      for (j=0; j<nSel; j++) {
        if (!strcmp(tmp,names[j])) {
          // we found a match...
          mapping[nElSel++].eIdx = i;
          const char *newname = getStr_f(myvprint("newNames[%i]",j));
          if (newname != NULL) {
            writer->addField(newname);
          } else {
            writer->addField(tmp);  // if no newName is given, add old name
          }
        }
      }
      free(tmp);
    }
    SMILE_IDBG(2,"selected %i elements of %i requested elements",nElSel,nSel);

  } else {

    // in non-element mode, create a list of expanded element names, then use the same code as in element mode
    int _N = reader->getLevelNf();  
    /// !!!!!!!!!!!!!!!!!!! TODO !!!!!!!!!!!!!!!!!!!!!!!
    for (i=0; i<_N; i++) {
      int __N=0;
      int arrNameOffset=0;
      const char *tmp = reader->getFieldName(i,&__N,&arrNameOffset);

      char *tmp2 = strdup(tmp);
      char *ar = strchr(tmp2,'[');
      if (ar == NULL) { // is non-array field (no [] in name)

      } else {    // is array field ( with [] in name )

        // look for element [x] OR range [x-y]
        char *rng = strchr(ar,'-');
        if (rng != NULL) { // is range

        } else { // is element in array field

        }


      }

      free(tmp2);

      // set names in mapping, and set nElSel correct

    }


  }
  namesAreSet = 1;
  return 1;
}

/*
int cDataSelector::myFinaliseInstance()
{
  return cDataProcessor::myFinaliseInstance();
}
*/

int cDataSelector::myTick(long long t)
{
  SMILE_DBG(4,"tick # %i, processing value vector",t);

  // get next frame from dataMemory
  cVector *vec = reader->getNextFrame();
  if (vec != NULL) {

    if (vecO == NULL) vecO = new cVector(nElSel, vec->type);
    int i;

    if (vec->type == DMEM_FLOAT) {

      for (i=0; i<nElSel; i++) {
        vecO->dataF[i] = vec->dataF[mapping[i].eIdx];
      }

    } else if (vec->type == DMEM_INT) {

      for (i=0; i<nElSel; i++) {
        vecO->dataI[i] = vec->dataI[mapping[i].eIdx];
      }

    }


    vecO->tmetaReplace(vec->tmeta);

    // save to dataMemory
    writer->setNextFrame(vecO);

    //   writer->setNextFrame(myVec);
    return 1;

  } 

  return 0;

}


cDataSelector::~cDataSelector()
{
  if (vecO != NULL) delete(vecO);
  if (mapping!=NULL) free(mapping);
  if (names!=NULL) free(names); // we do not free individual names... they are const pointers to memory allocated in the configManager
}

