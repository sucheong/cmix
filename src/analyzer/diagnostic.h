/* (-*-c++-*-)
 * Authors:  Henning Makholm (makholm@diku.dk)
 * Content:  C-Mix system: General diagnostic message engine
 *
 * Copyright © 1999. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __DIAGNOSTIC__
#define __DIAGNOSTIC__

#include <stdarg.h>
#include <stdlib.h>
#include <iostream.h>

//--------------------------------------------------------------
// class Position
//
// Abstracts a position in the input. Position objects are not
// intended to be allocated on the heap; rather passed directly
// around as though it was a basic type. If that turns out to be
// hurting efficiency, the *internal* layout of Position is
// going to be modified so that it contains only one machine word.
// 
// Constructors: Position(const char *file_name,int line_number)
//                 the most common one. The caller continues to
//                 own the space pointed to by file_name.
//               Position(const char *file_name)
//                 for messages concerning the entire file (very rare)
//               Position()
//                 for messages displayed without position information
//
// Applications: A Position can be written to a stream, that's all.
//               The most important applcation is to pass a Position to
//               one of the diagnostic primitives below.
//
// A Position makes a private copy of the file_name in the constructor;
// this is never freed! The class tries to cache the copies so that
// different Positions will share private copies.
// The current caching policy assumes that the Positions sharing a
// given file_name are constructed in a few "chunks" without any
// other Position creations being intermixed with each "chunk". This
// may change in the future, but it is probably never going to be
// efficient to allocate lots of short-lived Positions with different
// file_names.
//

class Position {
    friend class LexerTracker ;
    static const char* intern(const char*);
    int the_line;
    const char* file;
    
    friend class PositionInUnion ;
    friend class Diagnostic ;
    Position(int fl,const char* f): the_line(fl), file(f) {} // for the friend
public:
    Position() : the_line(-2), file("") {}
    Position(const char* f,int fl=-1) : the_line(fl), file(intern(f)) {}
    friend ostream& operator<<(ostream&,Position);
};

// Position outputs itself with a trailing space.
ostream& operator<<(ostream&,Position);

//--------------------------------------------------------------
// class PositionInUnion
//
// used for embedding Positions in unions - because the vanilla
// Position has nontrivial constructores it isn't allowed there.
// This class removes some of the safeguards for Position which
// is necessary because of the inherent insecurity of unions.

class PositionInUnion {
    int the_line ;
    const char* file;
public:
    operator Position() const { return Position(the_line,file); }
    void set(Position const &p) { the_line = p.the_line; file = p.file; }
};

//--------------------------------------------------------------
// class LexerTracker
//
// is used for generating Positions while reading in file.
// LexerTrackers can be either global objects (as of this writing,
// all existing specimen are) or member objects in classes that
// read files. There should be no reason to heap allocate them.
//
// Constructors: LexerTracker()
//                 Do not try to extract Positions from the
//                 LexerTracker before assigning values to it.
//
// Applications: A LexerTracker can be converted to a Position. Do
//               this when you need to store a Position that reflects
//               the current contents of the LexerTracker.
//               Right now LexerTracker is a descendant of Position
//               but please just imagine there is a conversion operator
//               that produces a Position from a LexerTracker.
//
//               A LexerTracker can have assigned a Position to set
//               the LexerTracker's idea of where it "is".
//
//               A LexerTracker can have assigned a const char* or
//               an integer, which sets just the filename or the
//               line number component of the position. The caller
//               continues to own the space used for the filename
//               argument.
//
//               The += and ++ operators manipulate the line number
//               component of the tracked position.
//

class LexerTracker : public Position {
public:
    LexerTracker() : Position() {}
    void operator =(const char*);
    void operator =(int n) { the_line = n; }
    void operator =(Position p) { the_line = p.the_line; file = p.file; }
    void operator +=(int n) { the_line += n; }
    void operator ++() { operator+=(1); }
    void operator ++(int) { operator+=(1); }
};

//--------------------------------------------------------------
// class CerrLikeStream
//
// Classes inherited from this class look like clones of cerr
// and can hence be used to output debugging messages or diagnostics
// with << and like operators.
//
// IMPORTANT convention: Inheritance from CerrLikeStream, while
// public (for technical reasons), is thought to be an implementation
// detail. If you're writing to a CerrLikeStream deriviate, please
// pretend you're sending output to somewhere unknown. Do not mix
// with output to cerr and expect that to work.
//
// Any descendant of CerrLikeStream has the right to stop being one
// without notice and instead derive itself from ostream in another
// way, referring to another streambuf.
//

class CerrLikeStream : public ostream {
public:
    CerrLikeStream() : ostream(cerr.rdbuf()) {}
};

//--------------------------------------------------------------
// enum Severity
// class Diagnostic
//
// Helps in writing diagnostic messages to the standard error stream.
// Several different 'severities' of diagnostic messages are supported:
//
//   ABORT    - dumps core after the message
//
//   INTERNAL - displays "Internal error" and dumps core
//
//   FATAL    - displays "Fatal error" and exits with an error
//              code of 2.
//
//   ERROR    - doesn't immediately terminate the program, but
//              eventually the program exits with an error code
//              of 1. This happens when 25 errors have been displayed,
//              or when Diagnostic::EnterNewPhase() is called.
//
//   WARNING  - display the error message but continue processing.
//
//   INFO     - operationally the same as a warning but does not
//              suggest there is a problem with the input.
//
// The way to use a Diagnostic is to construct it, write the error
// message text to it, and then destruct it. The destructor actually
// *does* something: it supplies the final newline and is responsible
// for terminating the program if its time has come.
// So make sure that the destructor is called at the right place.
//
// The easiest way to do this is to write a statement like e.g.
//     Diagnostic(INFO,pos) << "x is now " << x << ". Cool, heh?" ;
// The C++ standard guarantees that an anonymous object is destroyed
// called at the end of the full-expression containing its construction.
// G++ follows the standard in this respect, according to my experiments
// as well as the g++ FAQ in comp.lang.c++.
//
// If you need to output multi-line error messages or need more
// sophisticated control structure than a chain of <<'s, you'll
// need to make the Diagnostic a named, 'auto', variable in a
// local block created just to encompass the diagnostic output.
//
// Constructor: Diagnostic(Severity, Position)
//                 The constructor supplies the class of diagnostic
//                 and the position to output in front of the
//                 diagnostic. Use Position() if no input position
//                 is applicable.
//
// Applications: A Diagnostic is an ostream; you can output anything that
//               has a << operator to it.
//               *Do* *not* use the endl manipulator or explicit '\n's to
//               split the message up into several lines. Instead use
//                 addline()
//                 addline(Position)
//               to start off a new line; this either repeats the
//               once given position or outputs a new position in front
//               of the next lines---useful for messages that need
//               to refer to multiple positions in a way such that
//               tools like the emacs 'Compilation' mode can find them.
//
//                 vprintf(const char*,va_list)
//               for compatibility with C-like I/O. Do not use this!
//               Provided for the benefit of the semi-C++ lex++ and yacc++
//               tools we used previously.
//
// Static Member Function: EnterNewPhase()
//                           terminates the program if any ERROR's have
//                           been output.
//                           In C-Mix/II this is called from main()
//                           between the analysis phases.
//

enum Severity { ABORT = 0,
		INTERNAL,
		FATAL,
		ERROR,
		WARNING,
                INFO } ;

class Diagnostic : public CerrLikeStream {
  static int MetAnyErrors ;

  Severity mode ;
  Position pos ;
public:
  Diagnostic(Severity,Position);
  virtual ~Diagnostic();

  ostream& addline(Position) ;
  ostream& addline() ;

  void vprintf(const char *fmt,va_list) ;

  static void EnterNewPhase() { if ( MetAnyErrors ) exit(1); }
  // EnterNewPhase terminates the program if error messages have been output.
  // The philosophy is that if an error has occured, the next phase has no
  // correct data to operate on, anyway.
} ;

//--------------------------------------------------------------
// preprocessor macro DIAG_BODY
//
// Used in the parser and lexer interfaces to define printf-like
// error display functions.
// The first two arguments are arbitrary expressions used to
// construct the diagnostic; the third should be the "format
// string" parameter.
//
  
#define DIAG_BODY(mode,pos,fmt) { \
  va_list diag_va_list ; va_start(diag_va_list,fmt) ; \
  Diagnostic(mode,pos).vprintf(fmt,diag_va_list) ; \
  va_end(diag_va_list) ; }

//--------------------------------------------------------------
// preprocessor macro RIGHT_HERE
//
// Expands to a position that describes the source line where the
// macro occurs.

#define RIGHT_HERE (Position(__FILE__,__LINE__))

//--------------------------------------------------------------
// preprocessor macro DONT_CALL_THIS
//
// Insert this in the body of functions that should never be called.

#ifdef __GNUC__
  #define DONT_CALL_THIS Diagnostic(INTERNAL,RIGHT_HERE) \
                 << __PRETTY_FUNCTION__ << " is not implemented" ;
#else
  #define DONT_CALL_THIS Diagnostic(INTERNAL,RIGHT_HERE) \
                                 << "Call to unimplemented function" ;
#endif
#endif
