/* (-*-c++-*-)
 * Authors:  Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: Abstract output format.
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#ifndef __OUTPUT__
#define __OUTPUT__

#include "Plist.h"
#include "array.h"
#include <iostream.h>
#include "auxilary.h"

// Output types are organized in trees. Each piece of output must have a type.
class OType : public Numbered {
    static unsigned long current_type_nr; 
    unsigned thisnum;  // Unique type number.
    const char* id;    // Symbolic name.
    const OType* farther;
    Plist<OType> sons;
public:
     // Construct a new root type.
    OType(const char*);
     // Construct a new type.
    OType(const char*, OType*);
    const OType* parent() const;
    const Plist<OType>& children() const;
    const char* name() const;
    unsigned number() const;
    void do_export(ostream&) const;
};

class Anchor {
    static unsigned long current_anchor; 
    unsigned long num;                   // Unique number.
    OType* belongsTo;                    // Which tree type.
public:
    Anchor(OType*);
    OType* isIn() const;
    unsigned anchor() const;
    void do_export(ostream&) const;
};

class TextAttribute {
    static unsigned long current_number; 
    unsigned num;   // Attribute number.
    const char* id; // Symbolic name.
public:
    TextAttribute(const char*);
    const char* name() const;
    unsigned number() const;
    void do_export(ostream&) const;
};

/*
    switch (mark) {
    case Block:
        break;
    case Break:
        break;
    case HardNL:
        break;
    case Text:
        break;
    case Anno:
        break;
    case Label:
        break;
    };
*/

class Output {
public:
    enum Mark { Block,
           Break,
           HardNL, 
           Text,
           Anno,
           Label };
    enum BlockType { Consistent, Inconsistent };
    Output(BlockType, Plist<Output>*, unsigned = 0); // Block.
    Output(unsigned offset, unsigned spaces); // Break.
    Output(const char*, TextAttribute*); // Text.
    Output(Output*, Plist<Anchor>*); // Associate a list of anchors with the
                                   // subtree.
    Output(Anchor*,Output*); // Attach an existing anchor to atree.
    Output(); // HardNL
    friend Output* oconcat(Output*,Output*);
    friend Output* oconcat(const Output*,const Output*);
    // Lookup
    BlockType type() const;
    unsigned level() const;
    Plist<Output>* blocks() const;
    unsigned break_offset() const;
    unsigned break_spaces() const;
    const char* text() const;
    TextAttribute* attribute() const;
    Output* anno_subtree() const;
    Plist<Anchor>* anno_anchors() const;
    Anchor* label() const;
    Output* label_subtree() const;
    void do_export(ostream&) const;
    const Mark mark;
public:
  struct US_b { // Block
    BlockType type;   // How to behave when lines are too small.
    unsigned level;   // Indent level
    Plist<Output>* lst; // Children.
  };
  struct US_brk { // Break
    unsigned spaces; // Spaces when line does not break. 
    unsigned offset; // Extra offset when breaking lines.
  };
  struct US_s {  // Text
    const char*    str;    
    TextAttribute* att;
  };        
  struct US_a {               // Annotations
    Output*        sub;
    Plist<Anchor>* anchors; 
  };
  struct US_lab {              // Anchor
    Anchor* anchor;
    Output* sub;
  };
private:
  union {
    struct US_b b ;
    struct US_brk brk ;
    struct US_s s ;
    struct US_a a ;
    struct US_lab lab ;
  };
};

class OutputContainer {
    Plist<const OType> alltypes ;
    array<OType,Plist<Output>*> outputList ;
    const Plist<TextAttribute>* attributes;
    const char* producer;
    void traverseTypes(const OType*);
public:
    // An output container must be given a type tree when created.
    OutputContainer(const OType*,const Plist<TextAttribute>*,const char*);
    void add(Output*,const OType*);
    void do_export(ostream&) const;
};

Output* oconcat(Output*,Output*);

#endif
