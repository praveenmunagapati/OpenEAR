
# ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = 
  
# if BUILD_SMILExtract
  bin_PROGRAMS += SMILExtract SEMAINExtract
#  bin_PROGRAMS += SMILExtractTest

  SMILE_SOURCES=\
	src/SMILExtract.cpp \
	src/configManager.cpp \
	src/exceptions.cpp \
	src/smileLogger.cpp \
	src/commandlineParser.cpp \
	src/smileUtil.c \
	src/smileCommon.cpp \
	src/smileComponent.cpp \
	src/dataMemory.cpp \
	src/buffer.cpp \
	src/vecGlMean.cpp \
	src/fullinputMean.cpp \
	src/vectorMVStd.cpp \
	src/turnDetector.cpp \
	src/componentManager.cpp \
	src/dataReader.cpp \
	src/dataWriter.cpp \
	src/dataSource.cpp \
	src/dataSink.cpp \
	src/dataProcessor.cpp \
	src/dataSelector.cpp \
	src/vectorConcat.cpp \
	src/vectorProcessor.cpp \
	src/winToVecProcessor.cpp \
	src/windowProcessor.cpp \
	src/portaudioSource.cpp \
	src/portaudioSink.cpp \
	src/portaudioDuplex.cpp \
	src/portaudioDuplexS.cpp \
	src/portaudioDuplexD.cpp \
	src/preemphasis.cpp \
	src/exampleSource.cpp \
	src/exampleSink.cpp \
	src/arffSink.cpp \
	src/libsvmSink.cpp \
	src/libsvmliveSink.cpp \
	src/csvSink.cpp \
	src/arffSource.cpp \
	src/htkSink.cpp \
	src/datadumpSink.cpp \
	src/exampleProcessor.cpp \
	src/vectorPreemphasis.cpp \
	src/deltaRegression.cpp \
	src/contourSmoother.cpp \
	src/waveSource.cpp \
	src/framer.cpp \
	src/windower.cpp \
	src/spectral.cpp \
	src/functionals.cpp \
	src/functionalsVecToVec.cpp \
	src/functionalComponent.cpp \
	src/functionalExtremes.cpp \
	src/functionalMeans.cpp \
	src/functionalPeaks.cpp \
	src/functionalSegments.cpp \
	src/functionalMoments.cpp \
	src/functionalCrossings.cpp \
	src/functionalPercentiles.cpp \
	src/functionalRegression.cpp \
	src/functionalTimes.cpp \
	src/functionalDCT.cpp \
	src/energy.cpp \
	src/intensity.cpp \
	src/dbA.cpp \
	src/amdf.cpp \
	src/acf.cpp \
	src/tonespec.cpp \
	src/tonefilt.cpp \
	src/fftsg.c \
	src/transformFft.cpp \
	src/fftmagphase.cpp \
	src/melspec.cpp \
	src/chroma.cpp \
  src/chromaFeatures.cpp  \
	src/mfcc.cpp \
  src/tumkwsaSink.cpp \
  src/tumkwsjSink.cpp \
  src/volanalyse.cpp \
  src/fingerprint.cpp \
  src/waveSink.cpp \
  src/waveSinkCut.cpp \
	src/pitchACF.cpp \
	src/mzcr.cpp \
	src/lpc.cpp \
	src/lsp.cpp \
	src/echoAttenuator.cpp \
	src/echoCanceller.cpp \
	src/speexPreprocess.cpp \
	src/speexResample.cpp \
	src/activeMqSink.cpp \
	src/activeMqSource.cpp \
	svm/svm.cpp \
	src/nnlPlugin.cpp \
	src/semaineEmmaSender.cpp 

  SMILE_CPPFLAGS = -Isrc $(SEMAINEAPI_CPPFLAGS) $(PORTAUDIO_CPPFLAGS) -ggdb $(ATKLIB_CPPFLAGS) $(PTHREAD_CFLAGS) $(RTNNLLIB_CFLAGS)
  SMILE_LIBS = $(PORTAUDIO_LDFLAGS) -lm $(SEMAINEAPI_LDFLAGS) $(ATKLIB_LDFLAGS) $(PTHREAD_LIBS) $(DLIB) $(RTNNLLIB_LIBS)
   
  SMILExtract_SOURCES = src/SMILExtract.cpp 
  SMILExtract_CPPFLAGS = $(SMILE_CPPFLAGS)
  SMILExtract_LDADD = $(SMILE_LIBS) -lopensmile  

  SEMAINExtract_SOURCES = src/SEMAINExtract.cpp src/TumFeatureExtractor.cpp
  SEMAINExtract_CPPFLAGS = $(SMILE_CPPFLAGS) 
  SEMAINExtract_LDADD = $(SMILE_LIBS) -lopensmile  

  lib_LTLIBRARIES = libopensmile.la
  libopensmile_la_SOURCES = $(SMILE_SOURCES)
  libopensmile_la_CPPFLAGS = $(SMILE_CPPFLAGS)
  libopensmile_la_LIBADD = $(SMILE_LIBS)
  libopensmile_la_LDFLAGS = -no-undefined


# endif 


