#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "smash.h"

static String *lcontinue;
static String *lbreak;
static Vector *tkvec;

/* Misc */
static void free_type(Type *t);
static Token *next();
static void pushback(Token *tk);
static Token *peek();
static void missing(const char *msg);
static bool expect(int i);
static String *gensym();
static int  get_assign_op();
/* Misc */

/* make_type */
static Type *make_type(int kind, TypeInfo *ti, Type *ptr);
/* make_type */

/* make_ast */
static Node *make_ast(Node *temp);
static Node *make_ast_ident(String *str);
static Node *make_ast_number(const Token *tk);
static Node *make_ast_char(String *str);
static Node *make_ast_string(String *str);
static Node *make_ast_1op(int op, Node *a);
static Node *make_ast_2op(int op, Node *a, Node *b);
static Node *make_ast_maccess(Node *obj, String *member);
static Node *make_ast_ternary(Node *c, Node *t, Node *e);
static Node *make_ast_if(Node *c, Node *t, Node *e);
static Node *make_ast_funccall(Node *f, Vector *arg);
static Node *make_ast_label(String *label, Node *node);
static Node *make_ast_goto(String *label);
static Node *make_ast_return(Node *expr);
static Node *make_ast_compound(Vector *vec);
static Node *make_ast_lvar(String *str);
/* make_ast */

/* expression */
static Node *base_expr();
static Node *primary_expr();
static Node *compound_literal();
static Vector *argument_expr_list();
static Node *postfix_expr();
static Node *unary_expr();
static Node *cast_expr();
static Node *mul_expr();
static Node *add_expr();
static Node *shift_expr();
static Node *rel_expr();
static Node *eq_expr();
static Node *and_expr();
static Node *xor_expr();
static Node *bitor_expr();
static Node *logand_expr();
static Node *logor_expr();
static Node *cond_expr();
static Node *assign_expr();
static Node *expr();
/* expression */

/* statement */
static Node *case_stat();
static Node *default_stat();
static Node *compound_stat();
static Node *if_stat();
static Node *switch_stat();
static Node *while_stat();
static Node *do_stat();
static Node *for_stat();
static Node *goto_stat();
static Node *continue_stat();
static Node *break_stat();
static Node *return_stat();
static Node *label_stat();
static Node *expr_stat();
static Node *stat();
/* statement */

/* declaration */
static Node   *direct_decl();
static Node   *declarator();
static Node   *initializer();
static Node   *init_decl();
static Vector *init_decl_list(Type *t);
static Type   *decl_spec();
static Vector *decl();
static bool   is_decl();
/* declaration */


/* Misc */
static void
free_type(Type *t)
{
    free(t);
}

static Token *
next()
{
    Token *tk;
    tk = vec_cnt(tkvec) > 0 ? (Token*)vec_pop(tkvec) : read_token();
    if (tk->kind == TK_INVALID)
    {
        printf("Error: Invalid token\n");
        exit(EXIT_FAILURE);
    }

    return tk;
}

static void
pushback(Token *tk)
{
    vec_push(tkvec, (void*)tk);
}

static Token *
peek()
{
    Token *tk = next();
    pushback(tk);
    return tk;
}

static void
missing(const char *msg)
{
    fprintf(stderr, "Error: missing %s\n", msg);
    exit(EXIT_FAILURE);
}

static bool
expect(int i)
{
    Token *tk = next();
    if (tk->kind == i)
    {
        free_token(tk);
        return true;
    }
    else
    {
        pushback(tk);
        return false;
    }
}

static String *
gensym()
{
    static unsigned int id = 0;
    char buf[256];
    snprintf(buf, sizeof(buf)/sizeof(buf[0]), ".TEMP%u", id++);
    return make_string(buf);
}

static int
get_assign_op()
{
    Token *tk = next();
    int ret;

    switch (ret = tk->kind)
    {
        case '=':
        case OP_A_MUL:  case OP_A_DIV:  case OP_A_MOD:
        case OP_A_ADD:  case OP_A_SUB:
        case OP_A_LSHF: case OP_A_RSHF:
        case OP_A_AND:  case OP_A_XOR:  case OP_A_OR:
            free_token(tk);
            return ret;
    }
    pushback(tk);
    return 0;
}
/* Misc */

/* make_type */
static Type *
make_type(int kind, TypeInfo *ti, Type *ptr)
{
    Type *type = (Type*)malloc(sizeof(Type));
    type->kind = kind;
    type->ti = ti;
    type->ptr = ptr;
    return type;
}
/* make_type */

/* make_ast */
static Node *
make_ast(Node *temp)
{
    Node *node = (Node*)malloc(sizeof(Node));
    *node = *temp;
    return node;
}

static Node *
make_ast_ident(String *str)
{
    return make_ast(&(Node){.kind = AST_IDENT, .value = str});
}

static Node *
make_ast_number(const Token *tk)
{
    Node *node = make_ast(&(Node){.kind = AST_NUMBER});
    switch (tk->id)
    {
        case T_INT:     node->i    = tk->i;    break;
        case T_LINT:    node->li   = tk->li;   break;
        case T_LLINT:   node->lli  = tk->lli;  break;
        case T_UINT:    node->ui   = tk->ui;   break;
        case T_ULINT:   node->uli  = tk->uli;  break;
        case T_ULLINT:  node->ulli = tk->ulli; break;
        case T_FLOAT:   node->f    = tk->f;    break;
        case T_DOUBLE:  node->d    = tk->d;    break;
        case T_LDOUBLE: node->ld   = tk->ld;   break;
    }
    node->type = make_type(tk->id, NULL, NULL);
    return node;
}

static Node *
make_ast_char(String *str)
{
    return make_ast(&(Node){.kind = AST_CHAR, .value = str});
}

static Node *
make_ast_string(String *str)
{
    return make_ast(&(Node){.kind = AST_STRING, .value = str});
}

static Node *
make_ast_1op(int op, Node *a)
{
    return make_ast(&(Node){.kind = op, .operand = a});
}

static Node *
make_ast_2op(int op, Node *a, Node *b)
{
    return make_ast(&(Node){.kind = op, .left = a, .right = b});
}

static Node *
make_ast_maccess(Node *obj, String *member)
{
    return make_ast(&(Node){.kind = '.', .obj = obj, .member = member});
}

static Node *
make_ast_ternary(Node *c, Node *t, Node *e)
{
    return make_ast(&(Node){.kind = AST_TERNARY, .c = c, .t = t, .e = e});
}

static Node *
make_ast_if(Node *c, Node *t, Node *e)
{
    return make_ast(&(Node){.kind = KEY_IF, .c = c, .t = t, .e = e});
}

static Node *
make_ast_funccall(Node *f, Vector *arg)
{
    return make_ast(&(Node){.kind = AST_FUNCCALL, .func = f, .args = arg});
}

static Node *
make_ast_label(String *label, Node *node)
{
    return make_ast(&(Node){.kind = AST_LABEL, .stat = node, .label = label});
}

static Node *
make_ast_goto(String *label)
{
    return make_ast(&(Node){.kind = KEY_GOTO, .value = label});
}

static Node *
make_ast_return(Node *expr)
{
    return make_ast(&(Node){.kind = KEY_RETURN, .operand = expr});
}

static Node *
make_ast_compound(Vector *vec)
{
    return make_ast(&(Node){.kind = AST_COMPOUND, .stats = vec});
}

static Node *
make_ast_lvar(String *str)
{
    return make_ast(&(Node){.kind = AST_LVAR, .varname = str});
}
/* make_ast */

/* expression */
static Node *
base_expr(const int ops[], Node *(*f)())
{
    Node *node = f();
    int i;
    for (;;)
    {
        for (i = 0; ops[i] > 0; i++)
        {
            if (!expect(ops[i])) continue;
            node = make_ast_2op(ops[i], node, f());
            break;
        }
        if (ops[i] <= 0) return node;
    }
}

static Node *
primary_expr()
{
    Node *node;
    Token *tk = next();
    switch (tk->kind)
    {
        case TK_IDENT:
            node = make_ast_ident(copy_string(tk->str));
            break;
        case TK_NUMBER:
            node = make_ast_number(tk);
            break;
        case TK_CHAR:
            node = make_ast_char(copy_string(tk->str));
            break;
        case TK_STRING:
            node = make_ast_string(copy_string(tk->str));
            break;
        case '(':
            node = expr();
            if (!expect(')')) missing(")");
            break;
        default:
            // error
            assert(0);
            break;
    }
    free_token(tk);
    return node;
}

static Node *
compound_literal()
{
    // TODO
    return NULL;
}

static Vector *
argument_expr_list()
{
    Vector *args = make_vector();
    vec_push(args, assign_expr());
    for (;;)
    {
        if (expect(','))
        {
            vec_push(args, assign_expr());
        }
        else
        {
            return args;
        }
    }
}

static Node *
postfix_expr()
{
    Node *node = primary_expr();

    for (;;)
    {
        if (expect('['))
        {
            node = make_ast_2op('+', node, expr());
            node = make_ast_1op(AST_DEREF, node);
            if (!expect(']')) missing("]");
        }
        else if (expect('('))
        {
            Vector *args;
            if (expect(')'))
            {
                args = make_vector();
            }
            else
            {
                args = argument_expr_list();
                if (!expect(')')) missing(")");
            }
            node = make_ast_funccall(node, args);
        }
        else if (expect('.'))
        {
            Token *tk = next();
            if (tk->kind != TK_IDENT) missing("identifier");
            node = make_ast_maccess(node, copy_string(tk->str));
            free_token(tk);
        }
        else if (expect(OP_ARROW))
        {
            Token *tk = next();
            if (tk->kind != TK_IDENT) missing("identifier");
            node = make_ast_maccess(make_ast_1op(AST_DEREF, node), copy_string(tk->str));
            free_token(tk);
        }
        else if (expect(OP_INC))
        {
            node = make_ast_1op(OP_POST_INC, node);
        }
        else if (expect(OP_DEC))
        {
            node = make_ast_1op(OP_POST_DEC, node);
        }
        else
        {
            return node;
        }
    }
}

static Node *
unary_expr()
{
    Node *node;
    Token *tk = next();

    switch (tk->kind)
    {
        case OP_INC:
            node = make_ast_1op(OP_PRE_INC, unary_expr());
            break;
        case OP_DEC:
            node = make_ast_1op(OP_PRE_DEC, unary_expr());
            break;
        case '&':
            node = make_ast_1op(AST_GETADDR, cast_expr());
            break;
        case '*':
            node = make_ast_1op(AST_DEREF, cast_expr());
            break;
        case '+':
            node = make_ast_1op(AST_PLUS, cast_expr());
            break;
        case '-':
            node = make_ast_1op(AST_MINUS, cast_expr());
            break;
        case '~': case '!':
            node = make_ast_1op(tk->kind, cast_expr());
            break;
        case KEY_SIZEOF:
            assert(0);
            // TODO
            return NULL;
            break;
        default:
            pushback(tk);
            return postfix_expr();
    }
    free_token(tk);
    return node;
}

static Node *
cast_expr()
{
    // TODO
    return unary_expr();
}

static Node *
mul_expr() { return base_expr((int[]){'*', '/', '%', 0}, cast_expr); }

static Node *
add_expr() { return base_expr((int[]){'+', '-', 0}, mul_expr); }

static Node *
shift_expr() { return base_expr((int[]){OP_LSHF, OP_RSHF, 0}, add_expr); }

static Node *
rel_expr() { return base_expr((int[]){'<', '>', OP_LESSEQ, OP_GRTREQ, 0}, shift_expr); }

static Node *
eq_expr() { return base_expr((int[]){OP_EQ, OP_NOTEQ, 0}, rel_expr); }

static Node *
and_expr() { return base_expr((int[]){'&', 0}, eq_expr); }

static Node *
xor_expr() { return base_expr((int[]){'^', 0}, and_expr); }

static Node *
bitor_expr() { return base_expr((int[]){'|', 0}, xor_expr); }

static Node *
logand_expr() { return base_expr((int[]){OP_LOG_AND, 0}, bitor_expr); }

static Node *
logor_expr() { return base_expr((int[]){OP_LOG_OR, 0}, logand_expr); }

static Node *
cond_expr()
{
    Node *node = logor_expr();

    if (expect('?'))
    {
        Node *then = expr();
        if (!expect(':')) missing(":");
        Node *els  = cond_expr();
        node = make_ast_ternary(node, then, els);
    }
    return node;
}

static Node *
assign_expr()
{
    int ret;
    Node *node = cond_expr();

    if ((ret = get_assign_op()))
    {
        node = make_ast_2op(ret, node, assign_expr());
    }
    return node;
}

static Node *
expr() { return base_expr((int[]){',', 0}, assign_expr); }
/* expression */

/* statement */
static Node *
case_stat()
{
    // TODO
    return NULL;
}

static Node *
default_stat()
{
    // TODO
    return NULL;
}

static Node *
compound_stat()
{
    Vector *stats = make_vector();
    for (;;)
    {
        if (expect('}')) return make_ast_compound(stats);
        if (is_decl())
        {
            Vector *decls = decl();
            vec_concat(stats, decls);
            free_vector(decls);
        }
        else
        {
            Node *s = stat();
            if (s) vec_push(stats, s);
        }
    }
}

static Node *
if_stat()
{
    Node *cond, *then, *els;
    if (!expect('(')) missing("(");
    cond = expr();
    if (!expect(')')) missing(")");
    then = stat();
    els = expect(KEY_ELSE) ? stat() : NULL;
    return make_ast_if(cond, then, els);
}

static Node *
switch_stat()
{
    // TODO
    return NULL;
}

#define START_LOOPBODY(label_start, label_end) \
    String *b_lcontinue = lcontinue; \
    String *b_lbreak = lbreak; \
    lcontinue = label_start; \
    lbreak = label_end;

#define END_LOOPBODY() \
    lcontinue = b_lcontinue; \
    lbreak = b_lbreak;

static Node *
while_stat()
{
//Before: while ( cond ) body
//After:
//{
//LOOP:
//    if ( cond ) body; else goto END;
//    goto LOOP;
//END:
//}
    String *lstart = gensym();
    String *lend = gensym();
    Vector *mbody = make_vector();
    Node *cond, *body;

    if (!expect('(')) missing("(");
    cond = expr();
    if (!expect(')')) missing(")");
    {
        START_LOOPBODY(lstart, lend)
        body = stat();
        END_LOOPBODY()
    }

    vec_push(mbody,
            make_ast_label(lstart,
                make_ast_if(cond, body, make_ast_goto(lend))));
    vec_push(mbody, make_ast_goto(lstart));
    vec_push(mbody, make_ast_label(lend, NULL));
    return make_ast_compound(mbody);
}

static Node *
do_stat()
{
//Before: do body while ( cond );
//After:
//{
//LOOP:
//    body;
//    if ( cond ) goto LOOP;
//END:
//}
    String *lstart = gensym();
    String *lend = gensym();
    Vector *mbody = make_vector();
    Node *body, *cond;

    {
        START_LOOPBODY(lstart, lend)
        body = stat();
        END_LOOPBODY()
    }
    if (!expect(KEY_WHILE)) missing("while");
    if (!expect('(')) missing("(");
    cond = expr();
    if (!expect(')')) missing(")");
    if (!expect(';')) missing(";");

    vec_push(mbody, make_ast_label(lstart, body));
    vec_push(mbody, make_ast_if(cond, make_ast_goto(lstart), NULL));
    vec_push(mbody, make_ast_label(lend, NULL));
    return make_ast_compound(mbody);
}

static Node *
for_stat()
{
//Before: for ( init ; cond ; loop ) body
//After:
//{
//    init;
//LOOP:
//    if ( cond ) body; else goto END;
//    loop;
//    goto LOOP;
//END:
//}
    String *lstart = gensym();
    String *lend = gensym();
    Vector *mbody = make_vector();
    Node *init, *cond, *loop, *body;

    if (!expect('(')) missing("(");
    
    // initializer-expression
    if (expect(';')) init = NULL;
    else
    {
        init = expr();
        if (!expect(';')) missing(";");
    }

    // condition-expression
    if (expect(';')) cond = NULL;
    else
    {
        cond = expr();
        if (!expect(';')) missing(";");
    }

    // loop-expression
    if (expect(')')) loop = NULL;
    else
    {
        loop = expr();
        if (!expect(')')) missing(")");
    }
    // body
    {
        START_LOOPBODY(lstart, lend)
        body = stat();
        END_LOOPBODY()
    }

    if (init) vec_push(mbody, init);
    vec_push(mbody, make_ast_label(lstart, NULL));
    if (cond)
    {
        vec_push(mbody, make_ast_if(cond, body, make_ast_goto(lend)));
    }
    else
    {
        if (body) vec_push(mbody, body);
    }
    vec_push(mbody, loop);
    vec_push(mbody, make_ast_goto(lstart));
    vec_push(mbody, make_ast_label(lend, NULL));

    return make_ast_compound(mbody);
}

#undef START_LOOPBODY
#undef END_LOOPBODY

static Node *
goto_stat()
{
    Token *tk = next();
    Node *node;
    if (tk->kind != TK_IDENT) missing("identifier");
    node = make_ast_goto(copy_string(tk->str));
    free_token(tk);
    if (!expect(';')) missing(";");
    return node;
}

static Node *
continue_stat()
{
    if (!expect(';')) missing(";");
    assert(lcontinue != NULL);

    return make_ast_goto(lcontinue);
}

static Node *
break_stat()
{
    if (!expect(';')) missing(";");
    assert(lbreak != NULL);

    return make_ast_goto(lbreak);
}

static Node *
return_stat()
{
    Node *retexpr = NULL;
    if (!expect(';'))
    {
        retexpr = expr();
        if (!expect(';')) missing(";");
    }
    return make_ast_return(retexpr);
}

static Node *
label_stat()
{
    Token *tk = next();
    if (tk->kind == TK_IDENT && expect(':'))
    {
        Node *node = make_ast_label(copy_string(tk->str), stat());
        free_token(tk);
        return node;
    }
    else
    {
        missing("identifier :");
        return NULL;
    }
}

static Node *
expr_stat()
{
    if (expect(';'))
    {
        return NULL;
    }
    else
    {
        Node *node = expr();
        if (!expect(';')) missing(";");
        return node;
    }
}

static Node *
stat()
{
    Token *tk = next();
    Node *node;

    switch (tk->kind)
    {
        case KEY_CASE:     node = case_stat();     break;
        case KEY_DEFAULT:  node = default_stat();  break;
        case '{':          node = compound_stat(); break;
        case KEY_IF:       node = if_stat();       break;
        case KEY_SWITCH:   node = switch_stat();   break;
        case KEY_WHILE:    node = while_stat();    break;
        case KEY_DO:       node = do_stat();       break;
        case KEY_FOR:      node = for_stat();      break;
        case KEY_GOTO:     node = goto_stat();     break;
        case KEY_CONTINUE: node = continue_stat(); break;
        case KEY_BREAK:    node = break_stat();    break;
        case KEY_RETURN:   node = return_stat();   break;
        default:
        {
            Token *tk2 = next();
            pushback(tk2);
            pushback(tk);
            return tk2->kind == ':' ? label_stat()
                                    : expr_stat();
        }
    }
    free_token(tk);
    return node;
}
/* statement */

/* declaration */
static Node *
direct_decl()
{
    Token *tk = next();
    if (tk->kind == TK_IDENT)
    {
        return make_ast_lvar(copy_string(tk->str));
    }
    else
    {
        // TODO
    }
    return NULL;
}

static Node *
declarator(Type *t)
{
    // TODO
    // Pointer
    Node *node = direct_decl();
    node->type = t;
    return node;
}

static Node *
initializer()
{
    if (expect('{'))
    {
        // TODO
        return NULL;
    }
    else
    {
        return assign_expr();
    }
}

static Node *
init_decl(Type *t)
{
    Node *node = declarator(t);
    if (expect('=')) node->init = initializer();
    return node;
}

static Vector *
init_decl_list(Type *t)
{
    Vector *vec = make_vector();
    vec_push(vec, init_decl(t));
    for (;;)
    {
        if (expect(','))
        {
            vec_push(vec, init_decl(t));
        }
        else
        {
            return vec;
        }
    }
}

static Type *
decl_spec()
{
    // TODO
    // storage-class-specifier,
    // type-specifier,
    // type-qualifier,
    // function-specifier
    // を処理できるように実装
    if (!expect(KEY_INT)) missing("int");
    return make_type(T_INT, NULL, NULL);
}

static Vector *
decl()
{
    Type *t = decl_spec();
    if (expect(';'))
    {
        free_type(t);
        return NULL;
    }
    else
    {
        return init_decl_list(t);
    }
}

static bool
is_decl()
{
    Token *tk = peek();
    switch (tk->kind)
    {
        /* storage-class-specifier */
        case KEY_TYPEDEF:
        case KEY_EXTERN:
        case KEY_STATIC:
        case KEY_AUTO:
        case KEY_REGISTER:
        /* type-specifier */
        case KEY_VOID:
        case KEY_CHAR:
        case KEY_SHORT:
        case KEY_INT:
        case KEY_LONG:
        case KEY_FLOAT:
        case KEY_DOUBLE:
        case KEY_SIGNED:
        case KEY_UNSIGNED:
        case KEY_BOOL:
        case KEY_COMPLEX:
        /* struct-or-union-specifier */
        case KEY_STRUCT:
        case KEY_UNION:
        /* enum-specifier */
        case KEY_ENUM:
        /* typedef-name */
        /* type-qualifier */
        case KEY_CONST:
        case KEY_RESTRICT:
        case KEY_VOLATILE:
        /* function-specifier */
        case KEY_INLINE:
            return true;
    }
    return false;
}
/* declaration */

void
parser_init()
{
    lcontinue = NULL;
    lbreak = NULL;
    tkvec = make_vector();
}

Node *
read_toplevel()
{
    return stat();
}

#ifdef TEST_PARSER
static char *conv[] =
{
    "IDENT",
    "NUMBER",
    "STRING",
    "CHAR",
    
    "&",
    "*",
    "+",
    "-",

    "?",
    "^++",
    "^--",
    "$++",
    "$--",
    "cast",
#define op(x, y) y,
#define keyword(x, y) y,
#include "keyword.inc"
#undef op
#undef keyword
};

static void
print_node(FILE *f, const Node *node, int parent_id)
{
    static int id = 0;
    int node_id;

    switch (node->kind)
    {
        /* primitive */
        case AST_IDENT:
        case AST_NUMBER:
        case AST_STRING:
        case AST_CHAR:
            node_id = id++;
            fprintf(f, "%d [shape=box, label=\"%s(%s)\"];\n",
                    node_id,
                    conv[node->kind-256],
                    string2char(node->value));
            break;
        /* unary operator */
        case AST_GETADDR:
        case AST_DEREF:
        case AST_PLUS:
        case AST_MINUS:
        case OP_PRE_INC:
        case OP_PRE_DEC:
        case OP_POST_INC:
        case OP_POST_DEC:
            node_id = id++;
            fprintf(f, "%d [shape=box, label=\"%s\"];\n",
                    node_id,
                    conv[node->kind-256]);
            print_node(f, node->operand, node_id);
            break;
        case '~':
        case '!':
            node_id = id++;
            fprintf(f, "%d [shape=box, label=\"%c\"];\n",
                    node_id, (char)node->kind);
            print_node(f, node->operand, node_id);
            break;
        case OP_CAST:
            break;
        /* ternary operator */
        case AST_TERNARY:
            node_id = id++;
            fprintf(f, "%d [shape=box, label=\"?\"];\n",
                    node_id);
            print_node(f, node->c, node_id);
            print_node(f, node->t, node_id);
            print_node(f, node->e, node_id);
            break;
        /* binary operator */
        case '+': case '-':
        case '*': case '/':
        case '%':
        case '=':
        case '&':
        case '^':
        case '|':
        case '<': case '>':
            node_id = id++;
            fprintf(f, "%d [shape=box, label=\"%c\"];\n",
                    node_id,
                    (char)node->kind);
            print_node(f, node->left,  node_id);
            print_node(f, node->right, node_id);
            break;
        case OP_LOG_AND: case OP_LOG_OR:
        case OP_LSHF:    case OP_RSHF:
        case OP_EQ:      case OP_NOTEQ:
        case OP_GRTREQ:  case OP_LESSEQ:
        case OP_A_SUB:   case OP_A_ADD:
        case OP_A_MUL:   case OP_A_DIV:
        case OP_A_MOD:   case OP_A_AND:
        case OP_A_OR:    case OP_A_XOR:
        case OP_A_LSHF:  case OP_A_RSHF:
            node_id = id++;
            fprintf(f, "%d [shape=box, label=\"%s\"];\n",
                    node_id,
                    conv[node->kind-256]);
            print_node(f, node->left,  node_id);
            print_node(f, node->right, node_id);
            break;
    }
    if (parent_id >= 0)
    {
        fprintf(f, "%d -> %d;\n", parent_id, node_id);
    }
}

int
main(int argc, char *argv[])
{
    FILE *file;
    char buf[256];

    lex_init(argv[1]);
    parser_init();
    Node *node = expr();

    snprintf(buf, sizeof(buf)/sizeof(char), "%s.dot", argv[1]);
    file = fopen(buf, "w");
    
    fprintf(file, "digraph {\n");
    print_node(file, node, -1);
    fprintf(file, "}\n");

    fclose(file);

    return EXIT_SUCCESS;
}
#endif

