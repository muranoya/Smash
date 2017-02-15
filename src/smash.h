#ifndef _SMASH_H_
#define _SMASH_H_

typedef int bool;
#define true (1)
#define false (0)

enum
{
    TK_IDENT = 256,
    TK_NUMBER,
    TK_STRING,
    TK_CHAR,
    TK_EOF,
    TK_INVALID,

    AST_IDENT,
    AST_NUMBER,
    AST_STRING,
    AST_CHAR,

    AST_LVAR,
    AST_COMPOUND,
    AST_LABEL,
    AST_FUNCCALL,

    // 単項演算子
    AST_GETADDR, // &
    AST_DEREF,   // *
    AST_PLUS,    // +
    AST_MINUS,   // -

    // 三項演算子
    AST_TERNARY,
    // 前置
    OP_PRE_INC,
    OP_PRE_DEC,
    // 後置
    OP_POST_INC,
    OP_POST_DEC,
    OP_CAST,
#define op(x, _) x,
#define keyword(x, _) x,
#include "keyword.inc"
#undef op
#undef keyword
};

typedef struct
{
    char *str;
    int len;
} String;

typedef struct
{
    void **body;
    int size;
    int len;
} Vector;

typedef struct
{
    int kind;
    // TK_IDENT, TK_STRING or TK_CHAR
    String *str;
    // TK_NUMBER
    int id;
    union
    {
        int i;
        long int li;
        long long int lli;
        unsigned int ui;
        unsigned long int uli;
        unsigned long long int ulli;
        
        float f;
        double d;
        long double ld;
    };
} Token;

enum
{
    T_VOID,

    T_CHAR,
    T_SHORT,
    T_INT,
    T_LINT,
    T_LLINT,

    T_UCHAR,
    T_USHORT,
    T_UINT,
    T_ULINT,
    T_ULLINT,

    T_FLOAT,
    T_DOUBLE,
    T_LDOUBLE,

    T_STRUCT,
    T_ENUM,
    T_UNION,

    T_PTR,
};

typedef struct TypeInfo
{
    Vector *member; // Vector<Type*>
} TypeInfo;

typedef struct Type
{
    int kind;
    TypeInfo *ti;
    /* storage */
    bool is_static;
    bool is_register;
    /* qualifier */
    bool is_const;
    bool is_restrict;
    bool is_volatile;
    struct Type *ptr;
} Type;

typedef struct Node
{
    int kind;
    Type *type;
    union
    {
        // literal, goto
        String *value;
        // nunmber
        int i;
        long int li;
        long long int lli;
        unsigned int ui;
        unsigned long int uli;
        unsigned long long int ulli;
        float f;
        double d;
        long double ld;
        // decl
        struct
        {
            String *varname;
            struct Node *init;
        };
        // compound statement
        Vector *stats; // Vector<Node*>
        // label
        struct
        {
            String *label;
            struct Node *stat;
        };
        // Binary operator
        struct
        {
            struct Node *left;
            struct Node *right;
        };
        // Unary operator, return
        struct
        {
            struct Node *operand;
        };
        // if, ternary
        struct
        {
            struct Node *c;
            struct Node *t;
            struct Node *e;
        };
        // function call
        struct
        {
            struct Node *func;
            Vector *args; // Vector<Node*>
        };
        // struct or union member access
        struct
        {
            struct Node *obj;
            String *member;
        };
    };
} Node;

// util.c
void eperror(const char *msg);

// string.c
String *make_string(const char *str);
String *copy_string(const String *str);
void   free_string(String *str);
String *append_chars(String *s, const char *c);
String *append_char(String *s, const char c);
const char *string2char(String *s);

// vector.c
Vector *make_vector();
void   free_vector(Vector *vec);
void   *vec_pop(Vector *vec);
void   vec_push(Vector *vec, void *v);
void   vec_concat(Vector *dst, Vector *src);
void   *vec_peek(const Vector *vec);
int    vec_cnt(const Vector *vec);

// lex.c
void  lex_init(const char *path);
void  free_token(Token *tk);
Token *read_token();

// parser.c
void   parser_init();
Node   *read_toplevel();

#endif

