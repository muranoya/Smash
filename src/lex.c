#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "smash.h"

static FILE *file = NULL;

static int *stack;
static int stack_p;
static int stack_size;

/* prototype */
static void  stack_push(int c);
static int   stack_pop();
static int   stack_count();
static Token *make_invalid();
static Token *make_eof();
static Token *make_pnct(int c);
static Token *make_ident(int c);
static Token *make_number(int c);
static bool  make_number_base(int c, int base, String *p, int *t);
static bool  make_float_base(int c, int base, String *p, int *t);
static int   char2int(char c);
static void  conv_number(Token *tk, int base);
static Token *make_string_literal();
static Token *make_char_literal();
//static bool  is_simple_escape(int c);
static bool  is_nondigit(int c);
static bool  is_digit(int c, int base);
static bool  is_space(int c);
static bool  is_return(int c);
static void  set_keyword(Token *tk);
static void  skip();
static bool  estimate(int x);
static bool  estimate2(int x, int y);
static int   peek_char();
static void  unread_char(int c);
static int   read_char();

static void
stack_push(int c)
{
    if (stack_p >= stack_size)
    {
        stack = (int*)realloc(stack, sizeof(int)*stack_size*2);
        stack_size *= 2;
    }
    stack[stack_p++] = c;
}

static int
stack_pop()
{
    assert(stack_p > 0);
    return stack[--stack_p];
}

static int
stack_count() { return stack_p; }

static Token *
make_invalid()
{
    Token *tk = (Token*)malloc(sizeof(Token));
    tk->kind = TK_INVALID;
    tk->str = NULL;
    return tk;
}

static Token *
make_eof()
{
    Token *tk = (Token*)malloc(sizeof(Token));
    tk->kind = TK_EOF;
    tk->str = NULL;
    return tk;
}

static Token *
make_pnct(int c)
{
    Token *tk = (Token*)malloc(sizeof(Token));
    tk->kind = c;
    tk->str = NULL;
    return tk;
}

static Token *
make_ident(int c)
{
    Token *tk;
    char buf[128];
    int p;

    tk = (Token*)malloc(sizeof(Token));
    tk->kind = TK_IDENT;
    tk->str = make_string("");
    for (p = 0; is_digit(c, 10) || is_nondigit(c); p++, c = read_char())
    {
        if (p == 128-1)
        {
            buf[p] = '\0';
            append_chars(tk->str, buf);
            p = 0;
        }
        buf[p] = c;
    }
    buf[p] = '\0';
    append_chars(tk->str, buf);
    unread_char(c);

    set_keyword(tk);
    return tk;
}

static Token *
make_number(int c)
{
    int base;
    Token *tk;
    tk = (Token*)malloc(sizeof(Token));
    tk->kind = TK_NUMBER;
    tk->str = make_string("");

    if (c == '0')
    {
        c = read_char();
        if (c == 'x' || c == 'X')
        {
            base = 16;
            append_chars(tk->str, "0x");
            make_number_base(read_char(), 16, tk->str, &(tk->id));
        }
        else
        {
            base = 8;
            append_chars(tk->str, "0");
            make_number_base(read_char(), 8, tk->str, &(tk->id));
        }
    }
    else
    {
        base = 10;
        make_number_base(c, 10, tk->str, &(tk->id));
    }
    conv_number(tk, base);
    free_string(tk->str);
    return tk;
}

static bool
make_number_base(int c, int base, String *p, int *t)
{
    for (; is_digit(c, base); c = read_char())
    {
        append_char(p, c);
    }

    if (c == '.' || c == 'e' || c == 'E' || c == 'p' || c == 'P')
    {
        return make_float_base(c, (base == 16) ? 16 : 10, p, t);
    }

    if (c == 'u' || c == 'U')
    {
        append_chars(p, "u");
        if (estimate2('l', 'l') || estimate2('L', 'L'))
        {
            append_chars(p, "ll");
            *t = T_ULLINT;
        }
        else if (estimate('l') || estimate('L'))
        {
            append_chars(p, "l");
            *t = T_ULINT;
        }
        else
        {
            *t = T_UINT;
        }
    }
    else
    {
        unread_char(c);
        if (estimate2('l', 'l') || estimate2('L', 'L'))
        {
            append_chars(p, "ll");
            *t = T_LLINT;
        }
        else if (estimate('l') || estimate('L'))
        {
            append_chars(p, "l");
            *t = T_LINT;
        }
        else
        {
            *t = T_INT;
        }
    }
    return true;
}

static bool
make_float_base(int c, int base, String *p, int *t)
{
    if (c == '.')
    {
        append_char(p, c);
        c = read_char();
        if (is_digit(c, 10))
        {
            for (; is_digit(c, 10); c = read_char())
            {
                append_char(p, c);
            }
        }
    }

    if (c == 'p' || c == 'P' || c == 'e' || c == 'E')
    {
        if (base == 10 && (c == 'p' || c == 'P'))
        {
            // error
            assert(0);
        }
        else if (base == 16 && (c == 'e' || c == 'E'))
        {
            // error
            assert(0);
        }
        append_char(p, c);
        c = read_char();
        if (c == '+' || c == '-')
        {
            append_char(p, c);
            c = read_char();
        }
        
        for (; is_digit(c, 10); c = read_char())
        {
            append_char(p, c);
        }
    }

    if (c == 'f' || c == 'F')
    {
        *t = T_FLOAT;
        append_char(p, c);
    }
    else if (c == 'l' || c == 'L')
    {
        *t = T_LDOUBLE;
        append_char(p, c);
    }
    else
    {
        *t = T_DOUBLE;
        unread_char(c);
    }
    return true;
}

static int
char2int(char c)
{
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return c - '0';
}

#define str2int(type, m, base) \
    do { \
        const char *str = string2char(tk->str); \
        type val = 0; \
        for ( ; is_digit(*str, base); str++) \
        { \
            val *= (type)base; \
            val += (type)char2int(*str); \
        } \
        tk->m = val; \
        return; \
    } while (0)

static void
conv_number(Token *tk, int base)
{
    switch (tk->id)
    {
        case T_INT:
            str2int(int, i, base);
        case T_LINT:
            str2int(long int, li, base);
        case T_LLINT:
            str2int(long long int, lli, base);
        case T_UINT:
            str2int(unsigned int, ui, base);
        case T_ULINT:
            str2int(unsigned long int, uli, base);
        case T_ULLINT:
            str2int(unsigned long long int, ulli, base);
        case T_FLOAT:
        case T_DOUBLE:
        case T_LDOUBLE:
            break;
    }
}

#undef str2int

/*
static void
str2float(Token *tk, int base)
{
    do
    {
        const char *str = string2char(tk->str);
        double val = 0.0;
        for ( ; is_digit(*str, base); str++)
        {
        }
    } while (0);
}
*/

static Token *
make_string_literal()
{
    Token *tk;
    int c;

    tk = (Token*)malloc(sizeof(Token));
    tk->kind = TK_STRING;
    tk->str = make_string("\"");

    for (;;)
    {
        c = read_char();
        if (is_return(c))
        {
            // error
            assert(0);
            return NULL;
        }
        else if (c == '"')
        {
            append_chars(tk->str, "\"");
            return tk;
        }
        else if (c == '\\')
        {
            append_char(tk->str, c);
            append_char(tk->str, read_char());
        }
        else
        {
            append_char(tk->str, c);
        }
    }
}

static Token *
make_char_literal()
{
    Token *tk;
    int c;

    tk = (Token*)malloc(sizeof(Token));
    tk->kind = TK_CHAR;
    tk->str = make_string("'");

    for (;;)
    {
        c = read_char();
        if (is_return(c))
        {
            // error
            assert(0);
            return NULL;
        }
        else if (c == '\'')
        {
            append_chars(tk->str, "'");
            return tk;
        }
        else if (c == '\\')
        {
            append_char(tk->str, c);
            append_char(tk->str, read_char());
        }
        else
        {
            append_char(tk->str, c);
        }
    }
}

/*
static bool
is_simple_escape(int c)
{
    switch (c)
    {
    case '\'': case '"': case '?':  case '\\':
    case 'a':  case 'b': case 'f':  case 'n':
    case 'r':  case 't': case 'v':
        return true;
    default:
        return false;
    }
}
*/

static bool
is_nondigit(int c) { return isalpha(c) || c == '_'; }

static bool
is_digit(int c, int base)
{
    switch (base)
    {
    case 8:  return isdigit(c) && c != '8' && c != '9';
    case 10: return isdigit(c);
    case 16: return isdigit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
    }
    return false;
}

static bool
is_space(int c) { return c == ' ' || c == '\t' || c == '\v'; }

static bool
is_return(int c) { return c == '\n' || c == '\r'; }

static void
set_keyword(Token *tk)
{
#define op(x, y)
#define keyword(x, y) \
    if (strcmp(string2char(tk->str), y) == 0) \
    { \
        free_string(tk->str); \
        tk->str = NULL; \
        tk->kind = x; \
        return; \
    }
#include "keyword.inc"
#undef op
#undef keyword
}

static void
skip()
{
    int c;
    for (;;)
    {
        c = read_char();
        if (is_space(c) || is_return(c))
        {
            continue;
        }

        if (c == '/')
        {
            if (estimate('*'))
            {
                for (;;)
                {
                    if ((c = read_char()) == '*')
                    {
                        if (estimate('/')) break;
                    }
                }
            }
            else if (estimate('/'))
            {
                for (; !is_return(c); c = read_char());
            }
            else
            {
                unread_char(c);
                return;
            }
        }
        else
        {
            unread_char(c);
            return;
        }
    }
}

static bool
estimate(int x)
{
    int c = read_char();
    if (x == c) return true; else unread_char(c);
    return false;
}

static bool
estimate2(int x, int y)
{
    int c = read_char();
    int d = read_char();
    if (c == x && d == y)
    {
        return true;
    }
    else
    {
        unread_char(d);
        unread_char(c);
        return false;
    }
}

static int
peek_char()
{
    int c = read_char();
    unread_char(c);
    return c;
}

static void
unread_char(int c)
{
    stack_push(c);
}

static int
read_char()
{
    if (stack_count() > 0)
    {
        return stack_pop();
    }
    else
    {
        int c = fgetc(file);
        if (c == '\\')
        {
            int c2 = fgetc(file);
            if (c2 == '\n')
            {
                return fgetc(file);
            }
            else
            {
                stack_push(c2);
                return c;
            }
        }
        else
        {
            return c;
        }
    }
}

void
lex_init(const char *path)
{
    file = fopen(path, "r");
    if (!file) eperror("fopen");

    stack = (int*)malloc(sizeof(int)*128);
    stack_size = 128;
    stack_p = 0;
}

void
free_token(Token *tk)
{
    if (tk->kind == TK_IDENT
     || tk->kind == TK_STRING
     || tk->kind == TK_CHAR)
    {
        free(tk->str);
    }
    free(tk);
}

Token *
read_token()
{
    int c;
    skip();
    switch (c = read_char())
    {
        case '[': case ']': case '{': case '}': case '(': case ')':
        case '~': case ';': case ':': case ',': case '?':
            return make_pnct(c);
        case '.':
            if (is_digit(peek_char(), 10)) return make_number(c);
            if (estimate2('.', '.'))       return make_pnct(OP_THR);
            return make_pnct('.');
        case '-':
            if (estimate('>')) return make_pnct(OP_ARROW);
            if (estimate('-')) return make_pnct(OP_DEC);
            if (estimate('=')) return make_pnct(OP_A_SUB);
            return make_pnct('-');
        case '+':
            if (estimate('+')) return make_pnct(OP_INC);
            if (estimate('=')) return make_pnct(OP_A_ADD);
            return make_pnct('+');
        case '&':
            if (estimate('&')) return make_pnct(OP_LOG_AND);
            if (estimate('=')) return make_pnct(OP_A_AND);
            return make_pnct('&');
        case '*':
            if (estimate('=')) return make_pnct(OP_A_MUL);
            return make_pnct('*');
        case '^': // caret
            if (estimate('=')) return make_pnct(OP_A_XOR);
            return make_pnct('^');
        case '/':
            if (estimate('=')) return make_pnct(OP_A_DIV);
            return make_pnct('/');
        case '%':
            if (estimate('=')) return make_pnct(OP_A_MOD);
            return make_pnct('%');
        case '<':
            if (estimate2('<', '=')) return make_pnct(OP_A_LSHF);
            if (estimate('<'))       return make_pnct(OP_LSHF);
            if (estimate('='))       return make_pnct(OP_LESSEQ);
            return make_pnct('<');
        case '>':
            if (estimate2('>', '=')) return make_pnct(OP_A_RSHF);
            if (estimate('>'))       return make_pnct(OP_RSHF);
            if (estimate('='))       return make_pnct(OP_GRTREQ);
            return make_pnct('>');
        case '=':
            if (estimate('=')) return make_pnct(OP_EQ);
            return make_pnct('=');
        case '!':
            if (estimate('=')) return make_pnct(OP_NOTEQ);
            return make_pnct('!');
        case '|':
            if (estimate('=')) return make_pnct(OP_A_OR);
            if (estimate('|')) return make_pnct(OP_LOG_OR);
            return make_pnct('|');
        case '\"':
            return make_string_literal();
        case '\'':
            return make_char_literal();
        case EOF:
            return make_eof();
        default:
            if (is_digit(c, 10)) return make_number(c);
            if (is_nondigit(c))  return make_ident(c);
    }
    return make_invalid();
}

#ifdef TEST_LEX
int
main(int argc, char *argv[])
{
    Token *tk;
    if (argc != 2) exit(EXIT_FAILURE);

    lex_init(argv[1]);
    for (;;)
    {
        tk = read_token();
        switch (tk->kind)
        {
            case TK_IDENT:
                printf("IDENT: %s\n", string2char(tk->str));
                break;
            case TK_NUMBER:
                printf("NUMBER: %s\n", string2char(tk->str));
                break;
            case TK_STRING:
                printf("STRING: %s\n", string2char(tk->str));
                break;
            case TK_CHAR:
                printf("CHAR: %s\n", string2char(tk->str));
                break;
            case TK_EOF:
                printf("EOF\n");
                goto LEND;
            case TK_INVALID:
                printf("INVALID\n");
                goto LEND;
            default:
                printf("KEYWORD: %d\n", tk->kind);
                break;
        }
        free_token(tk);
    }
LEND:
    return EXIT_SUCCESS;
}
#endif

