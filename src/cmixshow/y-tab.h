typedef union{
    char* str;
    unsigned long num;
    int integer;
    struct AnnoTree *tree ;
    struct BlockElement *element ;
} YYSTYPE;
#define	PRODUCER	258
#define	TYPES	259
#define	ATTRIBUTES	260
#define	OUTPUT	261
#define	STRING	262
#define	NUMBER	263


extern YYSTYPE yylval;
