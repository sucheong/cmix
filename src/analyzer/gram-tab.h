typedef union{
    // Temporaries and switchboards (syntax.h)
    PositionInUnion pos ;
    Parse_GeneralType* gtype;
    Parse_Type* pt;
    Parse_UserType* utype;
    UserTag uttag;
    Parse_TypedefType* tdtype;
    Plist<Parse_Typemod>* typemods;
    Parse_Declarator* idmod;
    Parse_MemberId* mempost;
    char* str;
    bool boolean;
    // Permanent structures/classes (cpgm.h)
    Type* type;
    Expr* expr;
    ConstExpr* cexpr;
    Stmt* stmt;
    Init* init;
    Plist<VarDecl>* vars;
    Plist<Expr>* exprs;
    Plist<MemberDecl>* membs;
    Plist<Stmt>* stmts;
    CProgram* cpgm;
    UnOp unop;
    AssignOp asgnop;
} YYSTYPE;
#define	AUTO	258
#define	DOUBLE	259
#define	INT	260
#define	STRUCT	261
#define	BREAK	262
#define	ELSE	263
#define	LONG	264
#define	SWITCH	265
#define	CASE	266
#define	ENUM	267
#define	REGISTER	268
#define	TYPEDEF	269
#define	CHAR	270
#define	EXTERN	271
#define	RETURN	272
#define	UNION	273
#define	CONST	274
#define	FLOAT	275
#define	SHORT	276
#define	UNSIGNED	277
#define	CONTINUE	278
#define	FOR	279
#define	SIGNED	280
#define	VOID	281
#define	DEFAULT	282
#define	GOTO	283
#define	SIZEOF	284
#define	VOLATILE	285
#define	DO	286
#define	IF	287
#define	STATIC	288
#define	WHILE	289
#define	FLOATINGconstant	290
#define	INTEGERconstant	291
#define	CHARACTERconstant	292
#define	LONGconstant	293
#define	UINTEGERconstant	294
#define	STRINGliteral	295
#define	ULONGconstant	296
#define	DOUBLEconstant	297
#define	ARROW	298
#define	ICR	299
#define	DECR	300
#define	LEFTSHIFT	301
#define	RIGHTSHIFT	302
#define	LESSEQUAL	303
#define	GREATEQUAL	304
#define	EQUAL	305
#define	NOTEQUAL	306
#define	ANDAND	307
#define	OROR	308
#define	ELLIPSIS	309
#define	MULTassign	310
#define	DIVassign	311
#define	MODassign	312
#define	PLUSassign	313
#define	MINUSassign	314
#define	LSassign	315
#define	RSassign	316
#define	ANDassign	317
#define	ERassign	318
#define	ORassign	319
#define	IDENTIFIER	320
#define	TYPEDEFname	321
#define	CMIXTAG	322


extern YYSTYPE ccclval;
