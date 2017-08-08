/* Authors:  Henning Makholm
 * Content:  C-Mix annotation browser: tree repository and link hash
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <assert.h>
#include "annofront.h"

magic RootType = 0 ;
static struct TypeRec** AllTypes = NULL ;
static int NumberOfTypes = 0 ; 

struct TypeRec* typerec(magic type) {
    if ( type >= NumberOfTypes ) {
        magic newcount = type>10 ? 2*(type+1) : type+5 ;
        magic i ;
        struct TypeRec** NewAllTypes
            = safe_malloc(newcount*sizeof(struct TypeRec*)) ;
        for(i = 0 ; i < newcount ; i++ )
            if ( i < NumberOfTypes )
                NewAllTypes[i] = AllTypes[i] ;
            else
                NewAllTypes[i] = NULL ;
        free(AllTypes) ;
        AllTypes = NewAllTypes ;
        NumberOfTypes = newcount ;
    }
    if ( AllTypes[type] == NULL ) {
        AllTypes[type] = alloc(struct TypeRec) ;
        AllTypes[type]->Name = "Internal error: stray type!" ;
        AllTypes[type]->defined = 0 ;
        AllTypes[type]->NextSibling = 0 ;
        AllTypes[type]->FirstChild = 0 ;
        AllTypes[type]->Showmode = SHOW_HIDDEN ;
        AllTypes[type]->ShownAsColor = NULL ;
        AllTypes[type]->HasBeenHashed = 0 ;
        AllTypes[type]->trees = NULL ;
    }
    return AllTypes[type] ;
}

void DoForAllTypes(void (*callback)(magic,struct TypeRec*)) {
    magic i ;
    for ( i = 0 ; i < NumberOfTypes ; i++ )
        if ( AllTypes[i] )
            callback(i,AllTypes[i]) ;
}

magic NumberOfLabels = 0 ;

static void CountLinks(struct AnnoTree *t) {
    while(1)
        switch(t->TreeTag) {
        case TREE_LIST:
        {  
            struct BlockElement *diver = t->BlockContent ;
            while ( diver ) {
                CountLinks(diver->Contents);
                diver = diver->next ;
            }
        }
           return ;
        case TREE_CONCAT:
            CountLinks(t->TreeContent1);
            t = t->TreeContent2 ;
            break ;
        case TREE_LABEL:
            NumberOfLabels++ ;
            /* fall through */
        case TREE_LINK:
            t = t->TreeContent1 ;
            break ;
        case TREE_STRING:
            return ;
        default:
            die("Unknown tree tag %d",t->TreeTag);
        }
}

void AddAnnoTree(magic type,struct AnnoTree* tree) {
    struct TreelistElt *elt = alloc(struct TreelistElt);
    struct TypeRec *trec = typerec(type);
    elt->data = tree ;
    elt->next = trec->trees ;
    trec->trees = elt ;
    CountLinks(tree);
}

void DoForAllTrees(void (*callback)(magic,struct AnnoTree*)) {
    magic type ;
    for ( type = 0 ; type < NumberOfTypes ; type++ )
    if ( AllTypes[type] ) {
        struct TreelistElt *diver ;
        for (diver = AllTypes[type]->trees ;
             diver ; diver = diver->next)
            callback(type,diver->data);
    }
}

struct HashElt {
    magic type, label ;
    struct AnnoTree *resolved ;
} *Hashtab = NULL ;

magic do_hash(magic type,magic label) {
    magic guess, hash2 ;
    assert(Hashtab);
    guess = label % NumberOfLabels ;
    hash2 = ((type+1)*6 + 1) % NumberOfLabels ;
    while ( Hashtab[guess].resolved ) {
        if ( Hashtab[guess].type == type &&
             Hashtab[guess].label == label )
            return guess ;
        guess = (guess+hash2) % NumberOfLabels ;
    }
    Hashtab[guess].type = type ;
    Hashtab[guess].label = label ;
    return guess ;
}   

static void IndexLabels(struct AnnoTree *t,magic type,struct AnnoTree *root) {
    while(1)
        switch(t->TreeTag) {
        case TREE_LIST:
        {  
            struct BlockElement *diver = t->BlockContent ;
            while ( diver ) {
                IndexLabels(diver->Contents,type,root);
                diver = diver->next ;
            }
        }
           return ;
        case TREE_CONCAT:
            IndexLabels(t->TreeContent1,type,root);
            t = t->TreeContent2 ;
            break ;
        case TREE_LABEL:
        {
            magic i = do_hash(type,t->Label);
            if ( Hashtab[i].resolved )
                fprintf(stderr,
                        "Two instances of label ("PCT_MAGIC","PCT_MAGIC")\n",
                        type, t->Label);
            Hashtab[i].resolved = root ;
        }
         /* fall through */
        case TREE_LINK:
            t = t->TreeContent1 ;
            break ;
        case TREE_STRING:
            return ;
        default:
            die("Unknown tree tag %d",t->TreeTag);
        }
}

struct AnnoTree* FollowLabel(magic type,magic label) {
    magic found ;
    struct TypeRec *t = typerec(type);
    
    if ( ! t->HasBeenHashed ) {
        struct TreelistElt *diver ;

        if ( !Hashtab ) {
            magic i ;

            /* provide slack */
            NumberOfLabels = NumberOfLabels + NumberOfLabels / 4 ;
            
            /* compute size of hashtable, put it in NumberOfLabels */
            /* the size will only have 2 and 3 as prime factors */
            i = 256 ;
            while ( i < NumberOfLabels ) i *= 2 ;
            if ( i - i/4 > NumberOfLabels)
                i -= i/4 ;
            NumberOfLabels = i ;
            
            /* allocate and initialise hashtable */
            Hashtab = safe_malloc(NumberOfLabels*sizeof(struct HashElt));
            for ( i = 0 ; i <= NumberOfLabels ; i++ )
                Hashtab[i].resolved = NULL ;
        }
            
        for ( diver = t->trees ; diver ; diver=diver->next )
            IndexLabels(diver->data,type,diver->data);
        t->HasBeenHashed = 1 ;
    }   
    
    found = do_hash(type,label);
    if (!Hashtab[found].resolved) {
        static char sbuffer[100];
        static struct AnnoTree *t = NULL ;
        sprintf(sbuffer,"Unresolved link ("PCT_MAGIC","PCT_MAGIC")",
                type,label);
        if ( t == NULL ) {
            t = alloc(struct AnnoTree);
            t->TreeTag = TREE_STRING ;
            t->TextContent = sbuffer ;
            t->Attribute = 0 ;
        }
        t->NativeWidth = -1 ;
        t->PrettyHeight = 0 ;
        return t ;
    }
    return Hashtab[found].resolved;
}

struct AnnoTree* GetMainTree(void) {
    magic i ;
    for ( i = 0 ; i < NumberOfTypes ; i++ )
        if ( AllTypes[i] && AllTypes[i]->trees )
            return AllTypes[i]->trees->data ;
    die("No text in input file");
    return NULL /* shut op gcc */ ;
}

/*************************************************************/

static void FreeTree(struct AnnoTree *t) {
    struct AnnoTree *u ;
 again:
    switch ( t-> TreeTag ) {
    case TREE_CONCAT:
        FreeTree(t->TreeContent2);
        /* fall through */
    case TREE_LINK:
    case TREE_LABEL:
    case TREE_UNINDENT:
        u = t ;
        t = t->TreeContent1 ;
        free(u) ;
        goto again ;
    case TREE_STRING:
        if ( t->TextContent && t->TextContent[0] )
            free (t->TextContent) ;
        free(t);
        return ;
    case TREE_LIST:
        {
            struct BlockElement *diver, *temp ;
            diver = t->BlockContent ;
            while ( diver ) {
                FreeTree(diver->Contents);
                temp = diver ;
                diver = diver->next ;
                free(temp);
            }
        }
        free(t);
        return ;
    default:
        die("Unknown tree tag %d",t->TreeTag);
    }
}

void FreeEverything() {
    magic type ;
    struct TreelistElt *elt, *temp ;

    if ( Hashtab ) {
        free (Hashtab);
        Hashtab = NULL ;
    }
    NumberOfLabels = 0 ;

    for ( type = 0 ; type < NumberOfTypes ; type++ )
        if ( AllTypes[type] ) {
            elt = AllTypes[type]->trees ;
            while ( elt ) {
                FreeTree(elt->data);
                temp = elt ;
                elt = elt->next ;
                free(temp);
            }
            if ( AllTypes[type]->defined && AllTypes[type]->Name[0] )
                free(AllTypes[type]->Name);
            AllTypes[type]->Name = "Internal error: stray type!" ;
            AllTypes[type]->defined = 0 ;
            AllTypes[type]->NextSibling = 0 ;
            AllTypes[type]->FirstChild = 0 ;
            AllTypes[type]->HasBeenHashed = 0 ;
            AllTypes[type]->trees = NULL ;
        }
    RootType = 0 ;
    /* Well, most everything; the types themselves are not freed */
}
