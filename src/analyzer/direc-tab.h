typedef union{
    char* str;
    int numeric;
    AnnoAtom *annotation ;
    GeneratorDirective* generator ;
    Plist<GeneratorDirective::Param>* params ;
    GeneratorDirective::Param* param ;
    PositionInUnion pos;
} YYSTYPE;
#define	DEFINE	258
#define	SOURCE	259
#define	GENERATOR	260
#define	OUTPUTBASE	261
#define	HEADER	262
#define	SPECTIME	263
#define	RESIDUAL	264
#define	VISIBLE	265
#define	DANGEROUS	266
#define	ANYTIME	267
#define	PURE	268
#define	STATELESS	269
#define	ROSTATE	270
#define	RWSTATE	271
#define	WELLKNOWN	272
#define	CONSTANT	273
#define	TABOO	274
#define	SPECIALIZES	275
#define	PRODUCING	276
#define	GOAL	277
#define	COLONCOLON	278
#define	DOLLAR	279
#define	DEBUG	280
#define	AT	281
#define	MOST	282
#define	UNSIGNED	283
#define	CHARS	284
#define	ARE	285
#define	GLYPHS	286
#define	LIFT	287
#define	LONG	288
#define	DOUBLE	289
#define	WORD	290
#define	INT	291
#define	IDENT	292
#define	STRING	293


extern YYSTYPE cmxlval;
