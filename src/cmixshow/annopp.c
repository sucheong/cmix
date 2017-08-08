/* Authors:  Henning Makholm
 * Content:  C-Mix annotation browser: pretty-printer
 *
 * Copyright © 1998. The TOPPS group at DIKU, U of Copenhagen.
 * Redistribution and modification are allowed under certain
 * terms; see the file COPYING.cmix for details.
 */

#include <assert.h>
#include <stdio.h>
#include "annofront.h"
#include "latch.h"

#ifdef putc
#undef putc
#endif

#define LINEWIDTH 75
#define MAX_PP_LINE 1024

static int NativeWidth(struct AnnoTree *t) {
    unsigned width = 0 ;
    switch(t->TreeTag) {
    case TREE_LIST:
    {  
        struct BlockElement *diver = t->BlockContent ;
        while ( diver ) {
            width += NativeWidth(diver->Contents);
            switch (diver->SeparatorTag) {
            case SEP_NEWLINE:
                width = LINEWIDTH + 1;
                break ;
            case SEP_BREAK:
                width += diver->Spaces ;
                break ;
            case SEP_NEVER:
                break ;
            default:
                die("Unknown separator tag %d",diver->SeparatorTag);
            }
            diver = diver->next ;
        }
    }
       break ;
    case TREE_CONCAT:
        width = NativeWidth(t->TreeContent1) +
                NativeWidth(t->TreeContent2);
        break ;
    case TREE_LABEL:
    case TREE_LINK:
        width = NativeWidth(t->TreeContent1);
        break ;
    case TREE_STRING:
        width = strlen(t->TextContent);
        break ;
    default:
        die("Unknown tree tag %d",t->TreeTag);
    }
    return t->NativeWidth = width ;
}

static unsigned MakeLineBreaks(struct AnnoTree *t,int *room) {
    unsigned breaks = 0 ;
    while( t->NativeWidth > *room )
        switch(t->TreeTag) {
        case TREE_LIST:
            if ( *room < LINEWIDTH/5 ) {
                struct AnnoTree *newt = alloc(struct AnnoTree) ;
                int newroom = LINEWIDTH-8 ;
                *newt = *t ;
                t->TreeTag = TREE_UNINDENT ;
                t->TreeContent1 = newt ;
                t->BlockContent = NULL ;
                return breaks + 5 + MakeLineBreaks(newt,&newroom) ;
            } else {            
                struct BlockElement *diver ;
                int Newline ;
                int baseroom = *room - t->Indentation ;
                for ( diver=t->BlockContent ; diver ; diver=diver->next ) {
                    Newline = t->ConsistentBlock ;
                    if ( *room < diver->Contents->NativeWidth ) {
                        breaks += MakeLineBreaks(diver->Contents,room);
                        Newline = 1 ;
                    } else {
                        *room -= diver->Contents->NativeWidth ;
                    }
                    switch ( diver->SeparatorTag ) {
                    case SEP_BREAK:
                        *room -= diver->Spaces ;
                        if ( !Newline &&
                             ( diver->next == NULL ||
                               diver->next->Contents->NativeWidth <= *room ) )
                            break ;
                        /* else change to SEP_NEWLINE and fall through */
                        diver->SeparatorTag = SEP_NEWLINE ;
                    case SEP_NEWLINE:
                        breaks++ ;
                        *room = baseroom - diver->NextOffset ;
                        break ;
                    case SEP_NEVER:
                        break ;
                    default:
                        die("Unknown separator tag %d",diver->SeparatorTag);
                    }
                }
                return breaks ;
            }
        case TREE_CONCAT:
            if ( t->TreeContent1->NativeWidth * 5 < *room ) {
                *room -= t->TreeContent1->NativeWidth ;
                t->ConsistentBlock = 0 ;
                t = t->TreeContent2 ;
                break ;
            } else {
                int saveroom = *room ;
                breaks += MakeLineBreaks(t->TreeContent1,room) + 1 ;
                if ( *room < t->TreeContent2->NativeWidth ) {
                    t->ConsistentBlock = 1 ;
                    *room = saveroom ;
                    t = t->TreeContent2 ;
                    breaks++ ;
                    break ;
                } else {
                    t->ConsistentBlock = 0 ;
                    *room -= t->TreeContent2->NativeWidth ;
                    return breaks ;
                }
            }
        case TREE_LABEL:
        case TREE_LINK:
            t = t->TreeContent1 ;
            break ;
        case TREE_STRING:
            goto donotmakebreaks ; /* actually break for the outer while */
        default:
            die("Unknown tree tag %d",t->TreeTag);
        }
 donotmakebreaks:
    *room -= t->NativeWidth ;
    return breaks ;
}
   
/* -------------------------------------------------------------- */

void debugprinter(struct AnnoTree *t,int indent);

static void debugindent(int i) {
    while (i--) printf("   ");
}

static void blockprinter(struct AnnoTree *t,int indent) {
    if ( t->TreeTag == TREE_CONCAT ) {
        blockprinter(t->TreeContent1,indent);
        blockprinter(t->TreeContent2,indent);
    } else
        debugprinter(t,indent);
}

void debugprinter(struct AnnoTree *t,int indent) {
    while(1)
    switch(t->TreeTag) {
    case TREE_CONCAT:
        debugindent(indent); printf("CONCAT BLOCK\n");
        blockprinter(t,indent+1);
        debugindent(indent); printf("END CONCAT BLOCK\n");
        return ;
    case TREE_LIST:
        debugindent(indent); printf("LIST (%c,%d)\n",
                                    t->ConsistentBlock?'C':'I',
                                    t->Indentation);
        {
            struct BlockElement *diver = t->BlockContent ;
            for(;diver;diver=diver->next) {
                blockprinter(diver->Contents,indent+1);
                debugindent(indent);
                switch(diver->SeparatorTag) {
                case SEP_BREAK:
                    printf("BREAK\n");
                    assert(diver->next);
                    break ;
                case SEP_NEWLINE:
                    printf("NEWLINE+%d\n",diver->NextOffset);
                    assert(diver->next);
                    break ;
                case SEP_NEVER:
                    printf("END LIST\n");
                    assert(diver->next == NULL);
                    break ;
                default:
                    assert(0);
                }
            }
        }
         return ;
    case TREE_LINK:
    case TREE_LABEL:
        t = t->TreeContent1 ;
        break ;
    case TREE_STRING:
        debugindent(indent);
        printf("\"%s\"\n",t->TextContent);
        return ;
    }
}

/* -------------------------------------------------------------- */

static unsigned LinesTillTarget ;
static int NextLineBreakIsReal ;

static int LineIndentation ;
static int LineUnindented ;
static struct latch *Latch ;
static int TargetBegun, TargetEnded ;
static magic Target ;
static FILE *dest ;
static int Column ;

static void NewLine(int newindent) {
    if ( TargetBegun )
        fputs(inpp->marker,dest);
    if ( Latch != NULL ) {
        inpp->LinkKill(Latch) ;
        inpp->DoneLine(Latch,dest) ;
        if ( --LinesTillTarget == 0 )
            fputs(inpp->targetwarning,dest);
    }
    Latch = NULL ;
    Column = LineIndentation = newindent ;

    if ( TargetEnded )
        fputs(inpp->marker,dest);
    TargetBegun = TargetEnded = 0 ;
}

static void PrepareLatch(void) {
    if ( Latch == NULL ) {
        Latch = inpp->FreshLine() ;
        if ( LineUnindented )
            latchf(Latch,"/* %d */ ",LineUnindented);
        while(LineIndentation--)
            inpp->putc(Latch,' ') ;
    }
}

static void InsertString(const char *cp) {
    if ( cp == NULL || *cp == '\0' )
        return ;
    PrepareLatch();
    inpp->LinkAjour(Latch);
    for (; *cp ; cp++ ) {
        Column++ ;
        inpp->putc(Latch,*cp);
    }
}

static void Dive(struct AnnoTree *t) {
    int column_pre = Column ;
    switch (t->TreeTag) {
    case TREE_UNINDENT:
        LineUnindented++ ;
        NewLine(0) ;
        Dive(t->TreeContent1);
        LineUnindented-- ;
        NewLine(column_pre);
        break ;
    case TREE_LIST:
        {
            struct BlockElement *diver = t->BlockContent ;
            while (diver) {
                unsigned u ;
                Dive(diver->Contents);
                switch(diver->SeparatorTag) {
                case SEP_BREAK:
                    PrepareLatch();
                    inpp->LinkKill(Latch);
                    for(u=0;u<diver->Spaces;u++)
                        inpp->putc(Latch,' ');
                    break ;
                case SEP_NEWLINE:
                    NewLine(column_pre + t->Indentation + diver->NextOffset);
                    break ;
                case SEP_NEVER:
                    break ;
                default:
                    die("Unknown separator tag %d",diver->SeparatorTag);
                }
                diver = diver->next ;
            }
        }
        break ;
    case TREE_CONCAT:
        Dive(t->TreeContent1);
        if (t->ConsistentBlock)
            NewLine(column_pre);
        Dive(t->TreeContent2);
        break ;
    case TREE_LABEL:
        if (t->Label == Target)
            TargetBegun = 1 ;
        Dive(t->TreeContent1);
        if (t->Label == Target)
            TargetEnded = 1 ;
        break ;
    case TREE_LINK:
        {
            magic oldlink ;
            inpp->BeginLink(t->Type,t->Label,&oldlink);
            Dive(t->TreeContent1);
            inpp->EndLink(t->Type,t->Label,&oldlink);
        }
        break ;
    case TREE_STRING:
        InsertString(t->TextContent);
        break ;
    default:
        die("Unknown tree tag %d",t->TreeTag);
    }
}

static void FinderNewline(void) {
    LinesTillTarget+=NextLineBreakIsReal ;
    NextLineBreakIsReal = 0 ;
}

static int FindTarget(struct AnnoTree *t) {
    while(1)
    switch(t->TreeTag) {
    case TREE_UNINDENT:
        FinderNewline();
        if ( FindTarget(t->TreeContent1) )
            return 1 ;
        FinderNewline();
        return 0;
    case TREE_STRING:
        if ( t->TextContent[0] )
            NextLineBreakIsReal = 1 ;
        return 0 ;
    case TREE_CONCAT:
        if ( FindTarget(t->TreeContent1) )
            return 1 ;
        if ( t->ConsistentBlock )
            FinderNewline();
        t = t->TreeContent2 ;
        break ;
    case TREE_LABEL:
        if ( t->Label == Target )
            return 1;
        /* else fall through */
    case TREE_LINK:
        t = t->TreeContent1 ;
        break ;
    case TREE_LIST:
        {
            struct BlockElement *diver ;
            for ( diver = t->BlockContent ; diver ; diver = diver->next ) {
                if ( FindTarget(diver->Contents) )
                    return 1 ;
                switch(diver->SeparatorTag) {
                case SEP_NEWLINE:
                    FinderNewline();
                    NextLineBreakIsReal = 0 ;
                    break ;
                case SEP_BREAK:
                    NextLineBreakIsReal = 1 ;
                    break ;
                }
            }
        }
        return 0 ;
    default:
        die("Unknown tree tag %d",t->TreeTag);
    }
}

unsigned GetPrettyHeight(struct AnnoTree *tree) {
    if ( tree->NativeWidth < 0 ) {
        int room = LINEWIDTH ;
        NativeWidth(tree);
        tree->PrettyHeight = MakeLineBreaks(tree,&room);
    }
    return tree->PrettyHeight ;
}

void PrettyPrint(FILE *f,struct AnnoTree *tree,magic target) {
    dest = f ;
    Target = target ;
    TargetBegun = TargetEnded = 0 ;
    Latch = NULL ;
    LineIndentation = Column = 0 ;
    LineUnindented = 0 ;
    inpp->Reset(f);
    GetPrettyHeight(tree); /* force line-break computation */
    LinesTillTarget = 0 ;
    NextLineBreakIsReal = 0 ;
    if ( Target ) {
        if ( FindTarget(tree) && LinesTillTarget > 5 )
            LinesTillTarget -= 5 ;
        else {
            LinesTillTarget = 0 ;
            fputs(inpp->targetwarning,f);
        }
    }
    Dive(tree) ;
    NewLine(0) ;
}

    
