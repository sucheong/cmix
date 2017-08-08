/* Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: General error message engine
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include <stdio.h>
#include <stdlib.h>
#include "diagnostic.h"
#include "options.h"
#include "auxilary.h"
#include "symboltable.h"

char const* Position::intern(char const *file_name) {
  static SymbolTable<char const> cache ;
  char const *cached = cache.lookup(file_name) ;
  if ( cached )
    return cached ;
  else {
    cached = stringDup(file_name);
    cache.insert(cached,cached);
    return cached ;
  }
}

ostream& operator<<(ostream& ost,Position p) {
  if ( p.the_line == -2 )
      return ost << argv0 << ": " ;
  ost << p.file ;
  if ( p.the_line >= 0 )
    ost << ':' << p.the_line << ':' ; // unfortunately, xemacs does not
  // highlight the (xxx) format that isn't used by gcc or HP's cc
  return ost << ' ' ;
}

void LexerTracker::operator=(char const* newfile) {
    file = Position::intern(newfile);
}

//--------------------------------

int Diagnostic::MetAnyErrors = 0 ;
  
Diagnostic::Diagnostic(Severity m_mode, Position m_pos)
  : mode(m_mode), pos(m_pos)
{
  char const * const lead[] =
    {"Panic: ",
     "Internal error: ",
     "Fatal: ",
     "Error: ",
     "Warning: ",
     "Info: " } ;
  // XXX: (JPS) It would be nice to be able to not produce INFO when in quiet mode.
  cerr << pos << lead[mode] ;
}

Diagnostic::~Diagnostic() {
  cerr << endl ;
  switch(mode) {
  case ABORT:
  case INTERNAL:
    abort();
  case FATAL:
    exit(2);
  case ERROR:
    if (MetAnyErrors++ >= 25) {
      cerr << pos << "Too many errors" << endl ;
      exit(1);
    }
    else
      return ;
  case WARNING:
  case INFO:
    return ;
  }
}

ostream& Diagnostic::addline(Position n_pos) {
  if ( n_pos.the_line != -2 )
      pos = n_pos ;
  return addline() ;
}

ostream& Diagnostic::addline() {
  return *this << endl << pos << "- " ;
}

const int linelen = 200 ;
void Diagnostic::vprintf(const char* fmt,va_list valist) {
  char line[linelen] ;
  int retval = vsprintf(line,fmt,valist) ;
  if ( retval < 0 || retval >= linelen-1 ) {
    cerr << "Fatal: error message overflow" << endl ;
    exit(2) ;
  }
  cerr << line ;
}
