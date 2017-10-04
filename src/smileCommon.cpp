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


#include <smileCommon.hpp>

void smilePrintHeader()
{
  SMILE_PRINT(" ");
  SMILE_PRINT(" =============================================================== ");
  SMILE_PRINT("   %s version %s",APPNAME,APPVERSION);
  SMILE_PRINT("   Build date: %s",APPDATE);
  SMILE_PRINT("   (c) %s by %s",APPCPYEAR,APPCPAUTHOR);
  SMILE_PRINT("   %s",APPCPINST);
  SMILE_PRINT(" =============================================================== ");
  SMILE_PRINT(" ");
}

// realloc and initialize additional memory with zero, like calloc
void * crealloc(void *a, size_t size, size_t old_size)
{
  a = realloc(a,size);
  if ((old_size < size)&&(a!=NULL)) {
    char *b = (char *)a + old_size;
    //fill with zeros:
    bzero((void*)b, size-old_size);
  }
  return a;
}

/* allocate a string and expand vararg format string expression into it */
// WARNING: memory allocated by myvprint must freed by the code calling myvprint!!
char *myvprint(const char *fmt, ...) {
  char *s= (char *)malloc(sizeof(char)*(MIN_LOG_STRLEN+1));
//TODO: use vasprintf();
  if (s==NULL) return NULL;
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(s,MIN_LOG_STRLEN+1, fmt, ap);
  if (len > MIN_LOG_STRLEN) {
    free(s);
    s = (char *)malloc(sizeof(char)*len+2);
    va_start(ap, fmt);
    len = vsnprintf(s,len+1, fmt, ap);
  }
  va_end(ap);
  return s;
}

void * memdup(const void *in, size_t size)
{
  void *ret = malloc(size);
  if (ret!=NULL) memcpy(ret,in,size);
  return ret;
}



#ifdef __WINDOWS
void gettimeofday(timeval* p, void* tz /* IGNORED */){
  union {
    long long ns100; /*time since 1 Jan 1601 in 100ns units */
    FILETIME ft;
  } _now;

  GetSystemTimeAsFileTime( &(_now.ft) );
  p->tv_usec=(long)((_now.ns100 / 10LL) % 1000000LL );
  p->tv_sec= (long)((_now.ns100-(116444736000000000LL))/10000000LL);
  return;
}
#endif /* __WINDOWS */


#ifdef __MACOS
int clock_gettime (int clock_id, struct timespec *tp)
{
	struct timeval tv;
	int retval = gettimeofday (&tv, NULL);
	if (retval == 0)
		TIMEVAL_TO_TIMESPEC (&tv, tp); // Convert into `timespec'.
	return retval;
}
#endif

#ifdef __HAVENT_GNULIBS

#define MIN_CHUNK 64
long getstr (char **lineptr, size_t *n, FILE *stream, char terminator, int offset)
{
  int nchars_avail;		/* Allocated but unused chars in *LINEPTR.  */
  char *read_pos;		/* Where we're reading into *LINEPTR. */
  long ret;

  if (!lineptr || !n || !stream)
    {
      return -1;
    }

  if (!*lineptr)
    {
      *n = MIN_CHUNK;
      *lineptr = (char *)malloc (*n);
      if (!*lineptr)
	{
	  return -1;
	}
    }

  nchars_avail = *n - offset;
  read_pos = *lineptr + offset;

  for (;;)
    {
      register int c = getc (stream);


      /* We always want at least one char left in the buffer, since we
	 always (unless we get an error while reading the first char)
	 NUL-terminate the line buffer.  */


      if (nchars_avail < 2)
	{
	  if (*n > MIN_CHUNK)
	    *n *= 2;
	  else
	    *n += MIN_CHUNK;

	  nchars_avail = *n + *lineptr - read_pos;
	  *lineptr = (char *)realloc (*lineptr, *n);
	  if (!*lineptr)
	    {
	      return -1;
	    }
	  read_pos = *n - nchars_avail + *lineptr;
	}

      if (ferror (stream))
	{
	  return -1;
	}

      if (c == EOF)
	{
	  /* Return partial line, if any.  */
	  if (read_pos == *lineptr)
	    return -1;
	  else
	    break;
	}

      *read_pos++ = c;
      nchars_avail--;

      if (c == terminator)
	/* Return the line.  */
	break;
   }

  /* Done - NUL terminate and return the number of chars read.  */
  *read_pos = '\0';

  ret = read_pos - (*lineptr + offset);
  return ret;
}

long getline (char **lineptr, size_t *n, FILE *stream)
{
  return getstr (lineptr, n, stream, '\n', 0);
}

#endif

