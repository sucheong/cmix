/* Authors:  Peter Holst Andersen (txix@diku.dk)
 *           Jens Peter Secher (jpsecher@diku.dk)
 * Content:  C-Mix system: 
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <cmixconf.h>
#include "output.h"
#include "auxilary.h"
#include "diagnostic.h"

unsigned long Anchor::current_anchor = 0; 
unsigned long TextAttribute::current_number = 0; 
unsigned long OType::current_type_nr = 0; 


////////// Constructors ////////////

// Block.
Output::Output(BlockType t, Plist<Output>* os, unsigned i)
  : mark(Block)
{
  assert(os != NULL);
  b.type = t;
  b.level = i;
  b.lst = os;
}

// Break.
Output::Output(unsigned offset, unsigned spaces)
  : mark(Break)
{
  brk.spaces = spaces;
  brk.offset = offset;
}

// Text.
Output::Output(const char* n, TextAttribute* a)
  : mark(Text)
{
  assert(n != NULL && a != NULL);
  s.str = n;
  s.att = a;
}

// Associate a list of anchors with the subtree.
Output::Output(Output* o, Plist<Anchor>* as)
  : mark(Anno)
{
  assert(as != NULL && o != NULL);
  a.sub = o;
  a.anchors = as;
}

// Attach an existing anchor to atree.
Output::Output(Anchor* a,Output* o)
  : mark(Label)
{
  assert(a != NULL && o != NULL);
  lab.anchor = a;
  lab.sub = o;
}

// HardNL
Output::Output()
  : mark(HardNL)
{}


////////// Lookup functions ////////////

Output::BlockType
Output::type() const
{
  assert(mark==Block);
  return b.type;
}

unsigned
Output::level() const
{
  assert(mark==Block);
  return b.level;
}

Plist<Output>*
Output::blocks() const
{
  assert(mark==Block);
  return b.lst;
}

unsigned
Output::break_offset() const
{
  assert(mark==Break);
  return brk.offset;
}

unsigned
Output::break_spaces() const
{
  assert(mark==Break);
  return brk.spaces;
}

const char*
Output::text() const
{
  assert(mark==Text);
  return s.str;
}

TextAttribute*
Output::attribute() const
{
  assert(mark==Text);
  return s.att;
}

Output*
Output::anno_subtree() const
{
  assert(mark==Anno);
  return a.sub;
}

Plist<Anchor>*
Output::anno_anchors() const
{
  assert(mark==Anno);
  return a.anchors;
}

Anchor*
Output::label() const
{
  assert(mark==Label);
  return lab.anchor;
}

Output*
Output::label_subtree() const
{
  assert(mark==Label);
  return lab.sub;
}

static Output*
makeBlock(Output* o1, Output* o2)
{
  Plist<Output>* conc = new Plist<Output>();
  conc->push_back(o1);
  conc->push_back(o2);
  return new Output(Output::Consistent,conc,0);
}

Output*
oconcat(Output* o1, Output* o2)
{
  assert(o1!=NULL && o2!=NULL);
  // Reuse Block nodes when possible.
  // XXX: Maybe this is not a good idea; we make intensive use of sharing when
  // creating output.
  if (o1->mark == Output::Block) {
    if (o2->mark == Output::Block) {
      if (o1->type()==o2->type() && o1->level()==o2->level()) {
        // Move o2's list to the end of o1's.
        o1->b.lst->splice(o1->b.lst->end(), *o2->b.lst);
        return o1;
      }
      else {
        // Make a new block.
        return makeBlock(o1,o2);
      }
    }
    else {      
      // Put o2 last in o1's list.
      if (o1->level()==0) {
        o1->b.lst->push_back(o2);
        return o1;
      }
      else
        return makeBlock(o1,o2);
    }
  }
  else {
    if (o2->mark == Output::Block) {
      if (o2->level()==0) {
        // Put o1 in the front of o2's list.
        o2->b.lst->push_front(o1);
        return o2;
      }
      else
        return makeBlock(o1,o2);
    }
    else {      
      // Make a new node.
      return makeBlock(o1,o2);        
    }
  }
}

/////////////////////////////////////////////////////

TextAttribute::TextAttribute(const char* n)
  : num(++current_number), id(n)
{}

/////////////////////////////////////////////////////

// Construct a new type.
OType::OType(const char* s)
  : Numbered(Numbered::OTYPE), thisnum(++current_type_nr), id(s), farther(this)
{
  assert(s!=NULL);
}

// Construct a new type.
OType::OType(const char* s, OType* t)
  : Numbered(Numbered::OTYPE), thisnum(++current_type_nr), id(s), farther(t)
{
  assert(s!=NULL && t!=NULL);
  // Append this type to the parent's list of children.
  t->sons.push_back(this);
}

const OType*
OType::parent() const
{
  return farther;
}

const Plist<OType>&
OType::children() const
{
  return sons;
}

const char*
OType::name() const
{
  return id;
}

unsigned
OType::number() const
{
  return thisnum;
}

/////////////////////////////////////////////////////


Anchor::Anchor(OType* t)
  : num(++current_anchor), belongsTo(t)
{
  assert(t!=NULL);
}

OType*
Anchor::isIn() const
{
  return belongsTo;
}

unsigned
Anchor::anchor() const
{
  return num;
}

/////////////////////////////////////////////////////

OutputContainer::OutputContainer(const OType* t,
                                 const Plist<TextAttribute>* att,
                                 const char* p)
  : outputList(NULL), attributes(att), producer(p)
{
  assert(p!=NULL && att!=NULL);
  assert(!att->empty());
  // The OType must be a root.
  assert(t->number() == t->parent()->number());
  // Traverse the type tree and associate a new list of output with every
  // type.
  traverseTypes(t);
}

void
OutputContainer::traverseTypes(const OType* node)
{
  alltypes.push_back(node);  
  // Make a new list to put output of this type in.
  outputList[node] = new Plist<Output>();
  // Do a Michael J.
  // (who the fuck is Michael J.?)
  foreach(child, node->children(), Plist<OType>) {
    traverseTypes(*child);
  }
}


void
OutputContainer::add(Output* o, const OType* t)
{
  assert(o!=NULL && t!=NULL);
  // Find the list of output trees for this type.
  Plist<Output>* olist = outputList[t];
  assert(olist != NULL);
  olist->push_back(o);
}

void
OutputContainer::do_export(ostream& o) const
{
  // Print the producer.
  o << "Producer: \"" << producer << "\"" << endl << endl;
  // Print the types.
  o << "Types:" << endl;
  foreach(ot,alltypes,Plist<const OType>) {
      ot->do_export(o) ;
      o << endl ;
  }
  // Print the attributes.
  o << "Attributes:" << endl;
  foreach(att, *attributes, Plist<TextAttribute>) {
    (*att)->do_export(o);
    o << endl;
  }
  // Print the output trees.
  o << "Output:" << endl;
  foreach(ot2,alltypes,Plist<const OType>) {
    // Print the type number.
    o << "#" << ot2->number() << endl;
    // For each tree of this type.
    foreach(output, *outputList[*ot2], Plist<Output>)
      (*output)->do_export(o);
  }
}

void
Output::do_export(ostream& o) const
{
  switch (mark) {
  case Block: {
    // Here we could have an optimization that skiped a C0 block
    // if it only contained another block (by calling this
    // function once more). This would reduce the output quite a
    // bit.
    switch (type()) {
    case Consistent:
      o << "C";
      break;
    case Inconsistent:
      o << "I";
      break;
    };
    o << level() << "{";
    foreach( elem, *blocks(), Plist<Output> )
        if ( *elem ) {
            elem->do_export(o);
            o << " ";
        };
    o << "}" << endl;
    break; }
  case Break:
    o << "?";
    // When spaces=1 and offset=0 we omit them.
    if (break_offset() != 0 || break_spaces() != 1) {
      o << "<" << break_spaces() << "," << break_offset() << ">";
    }
    break;
  case HardNL:
    o << "!";
    break;
  case Text: {
    // Escape " and \.
    o << "\"";
    for(const char* s = text(); *s != '\0'; s++) {
      if (*s == '\\' || *s == '"') o << '\\';
      o << *s;
    }
    o << "\"" << attribute()->number() << " ";
    break; }
  case Anno:
    // There's no need to do_export an empty list.
    if (! anno_anchors()->empty()) {
      o << "@[";
      foreach( anno, *anno_anchors(), Plist<Anchor> ) {
        anno->do_export(o);
      };
      o << "]";
    };
    anno_subtree()->do_export(o);
    break;
  case Label:
    o << label()->anchor() << ":";
    label_subtree()->do_export(o);       
    break;
  };

}


void
Anchor::do_export(ostream& o) const
{
  o << "(" << anchor() << "," << isIn()->number() << ")";
}

void
OType::do_export(ostream& o) const
{
  o << "\"" << name() << "\" = " << number() << " (" << parent()->number()
    << ")";
}

void
TextAttribute::do_export(ostream& o) const
{
  o << "\"" << name() << "\" = " << number();
}

const char*
TextAttribute::name() const
{
  return id;
}

unsigned
TextAttribute::number() const
{
  return num;
}


