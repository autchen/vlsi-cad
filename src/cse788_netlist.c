
#include "cse788_netlist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define _MAX_VAR 20
#define _DEF_CAP 3

#define _TYPE_NODE 0
#define _TYPE_PIN  1
#define _TYPE_PSEU 2

#define _LABEL_W  0
#define _LABEL_B  1
#define _LABEL_WB 2
#define _LABEL_BW 3
#define _LABEL_UNDEFINED 4

#define _SRC_IDX -1
#define _DRN_IDX -2

typedef struct cse788_netnode_t cse788_netnode_t;

typedef struct cse788_netentry_t {
        int pin;
        int src, drn;
} cse788_netentry_t;

struct cse788_netlist_t {
        int func_len;
        char func_str[_MAX_VAR * 4];
        struct cse788_netnode_t *root;
        char gate_seq[_MAX_VAR * 2];
        int gate_num;
        struct cse788_netentry_t mos[_MAX_VAR * 2];
};

typedef union cse788_netin_u {
        int type;
        struct {
                int type_pin;
                char name;
                int src;
        } pin;
        struct {
                int type_node;
                cse788_netnode_t *ptr;
        } node;
} cse788_netin_u;

struct cse788_netnode_t {
        int flip;
        int gate;
        int num, cap;
        union cse788_netin_u *in;
        union cse788_netin_u *sort[_MAX_VAR + 1];
        int color;
};

static int 
cse788_netlist_trim_func(struct cse788_netlist_t *t, const char *func_str)
{
        char *tr = t->func_str;
        while (*func_str) {
                if (*func_str == 'P' || *func_str == 'N')
                        return __LINE__;
                if (*func_str != ' ' && *func_str != '\t' && *func_str != '\n')
                        *(tr++) = *func_str;
                func_str++;
        }
        *tr = '\0';
        t->func_len = tr - t->func_str;
        return 0;
}

static int cse788_netlist_matchpth(const char *str, int len)
{
        int i, n;
        for (i = 0, n = 0; i < len; i++) {
                if (str[i] == '(')
                        n++;
                else if (str[i] == ')')
                        n--;
                if (n < 0)
                        return i;
        }
        return -1;
}

static int 
cse788_netlist_fill_node(struct cse788_netnode_t *n, const char *str, int len, 
                         int np)
{
        int i = 0;

        if ('(' == str[0]) {
                if (len - 2 == cse788_netlist_matchpth(str + 1, len - 1)) {
                        str++;
                        len -= 2;
                }
        }
        if (0 == len)
                return __LINE__;

        n->cap = _DEF_CAP;
        n->num = 0;
        n->in = (union cse788_netin_u *)calloc(n->cap, sizeof *(n->in));
        n->flip = 0;
        n->gate = 0;

        while (i < len) {
                if (n->num >= n->cap) {
                        n->cap *= 2;
                        n->in = (union cse788_netin_u *)
                                realloc(n->in, sizeof *(n->in) * n->cap);
                }

                if (isalpha(str[i])) {
                        n->in[n->num].type = _TYPE_PIN;
                        n->in[n->num].pin.name = str[i];
                        i++;
                } else if ('(' == str[i]) {
                        int end, retval;

                        n->in[n->num].type = _TYPE_NODE;
                        n->in[n->num].node.ptr = (struct cse788_netnode_t *)
                                malloc(sizeof(struct cse788_netnode_t));
                        end = cse788_netlist_matchpth(str + i + 1, len - i - 1);
                        if (-1 == end)
                                return __LINE__;
                        retval = cse788_netlist_fill_node(
                                        n->in[n->num].node.ptr, str + i + 1, 
                                        end, np);
                        if (0 != retval)
                                return retval;
                        i += (1 + end + 1);
                } else
                        return __LINE__;
                n->num++;
                        
                if (i >= len)
                        break;

                if (str[i] != '*' && str[i] != '+')
                        return __LINE__;
                if (0 == n->gate) {
                        if ('N' == np)
                                n->gate = str[i];
                        else if ('*' == str[i])
                                n->gate = '+';
                        else
                                n->gate = '*';
                                
                }
                else if (n->gate != str[i])
                        return __LINE__;
                i++;
        }

        if (n->num % 2 == 0) {
                if (n->num >= n->cap) {
                        n->cap *= 2;
                        n->in = (union cse788_netin_u *)
                                realloc(n->in, sizeof *(n->in) * n->cap);
                }
                n->in[n->num].type = _TYPE_PSEU;
                n->in[n->num].pin.name = '0';
                n->num++;
        }

        if (n->num > _MAX_VAR)
                return __LINE__;

        return 0;
}

static int cse788_netlist_parse_func(struct cse788_netlist_t *t)
{
        int np;
        char *str = t->func_str;
        int len = t->func_len;
        struct cse788_netnode_t *root;
        int retval;

        if ('~' == str[0]) {
                np = 'N';
                str++;
                len--;
        } else
                np = 'P';
        root = (struct cse788_netnode_t *)malloc(sizeof *root);
        if (NULL == root)
                return __LINE__;
        t->root = root;
        retval = cse788_netlist_fill_node(root, str, len, np);
        if (0 != retval)
                return retval;
        return 0;
}

static int cse788_netlist_sort(struct cse788_netnode_t *n)
{
        int i, mark[_MAX_VAR + 1];
        int k = 0, m = 0;
        int h, l;
        int retval;

        for (i = 0; i < _MAX_VAR + 1; i++)
                mark[i] = 0;
        for (i = 0; i < n->num; i++) {
                if (_TYPE_NODE == n->in[i].type) {
                        retval = cse788_netlist_sort(n->in[i].node.ptr);
                        if (0 != retval)
                                return retval;
                }
        }
        for (i = 0; i < n->num; i++) {
                if (_TYPE_PSEU == n->in[i].type || (
                                _TYPE_NODE == n->in[i].type &&
                                _LABEL_W == n->in[i].node.ptr->color)) {
                        n->sort[k++] = n->in + i;
                        mark[i] = 1;
                }
        }
        for (i = 0; i < n->num; i++) {
                if (_TYPE_NODE == n->in[i].type && 0 == mark[i]) {
                        struct cse788_netnode_t *tmp = n->in[i].node.ptr;
                        if (_LABEL_WB == tmp->color 
                                        || _LABEL_BW == tmp->color) {
                                if (_LABEL_BW == tmp->color)
                                        tmp->flip = 1;
                                n->sort[k++] = n->in + i;
                                mark[i] = 1;
                                break;
                        }
                }
        }
        for (i = 0; i < n->num; i++) {
                if (_TYPE_PIN == n->in[i].type || (
                                _TYPE_NODE == n->in[i].type &&
                                _LABEL_B == n->in[i].node.ptr->color)) {
                        n->sort[k++] = n->in + i;
                        mark[i] = 1;
                }
        }
        for (i = 0; i < n->num; i++) {
                if (_TYPE_NODE == n->in[i].type && 0 == mark[i]) {
                        struct cse788_netnode_t *tmp = n->in[i].node.ptr;
                        if (_LABEL_WB == tmp->color 
                                        || _LABEL_BW == tmp->color) {
                                if ((m == 0 && _LABEL_WB == tmp->color) ||
                                    (m == 1 && _LABEL_BW == tmp->color))
                                        tmp->flip = 1;
                                m ^= 1;
                                if (_LABEL_BW == tmp->color)
                                        tmp->flip = 1;
                                n->sort[k++] = n->in + i;
                                mark[i] = 1;
                        }
                }
        }
        if (k != n->num)
                return __LINE__;

        if (/* 1 */ n->sort[0]->type == _TYPE_PSEU || 
            /* 2 */ (n->sort[0]->type == _TYPE_NODE && 
            /* 2.1 */ (n->sort[0]->node.ptr->color == _LABEL_W || 
            /* 2.2 */ (n->sort[0]->node.ptr->color == _LABEL_WB &&
                      n->sort[0]->node.ptr->flip == 0) || 
            /* 2.3 */ (n->sort[0]->node.ptr->color == _LABEL_BW &&
                      n->sort[0]->node.ptr->flip == 1))))
                h = _LABEL_W;
        else
                h = _LABEL_B;
        k--;
        if (/* 1 */ n->sort[k]->type == _TYPE_PSEU || 
            /* 2 */ (n->sort[k]->type == _TYPE_NODE && 
            /* 2.1 */ (n->sort[k]->node.ptr->color == _LABEL_W || 
            /* 2.2 */ (n->sort[k]->node.ptr->color == _LABEL_WB &&
                      n->sort[k]->node.ptr->flip == 1) || 
            /* 2.3 */ (n->sort[k]->node.ptr->color == _LABEL_BW &&
                      n->sort[k]->node.ptr->flip == 0))))
                l = _LABEL_W;
        else
                l = _LABEL_B;
        if (h == _LABEL_W && l == _LABEL_W)
                n->color = _LABEL_W;
        else if (h == _LABEL_B && l == _LABEL_B)
                n->color = _LABEL_B;
        else if (h == _LABEL_B && l == _LABEL_W)
                n->color = _LABEL_BW;
        else
                n->color = _LABEL_WB;

        return 0;
}

static int 
cse788_netlist_findseq(struct cse788_netnode_t *n, char *seq, int *idx, 
                       int flip)
{
        int i, inc, end;
        int retval;

        flip ^= n->flip;
        if (flip == 0) {
                i = 0;
                inc = 1;
                end = n->num;
        } else {
                i = n->num - 1;
                inc = -1;
                end = -1;
        }

        while (i != end) {
                if (n->sort[i]->type == _TYPE_PIN)
                        seq[(*idx)++] = n->sort[i]->pin.name;
                else if (n->sort[i]->type == _TYPE_PSEU)
                        seq[(*idx)++] = '%';
                else {
                        retval = cse788_netlist_findseq(n->sort[i]->node.ptr,
                                        seq, idx, flip);
                        if (0 != retval)
                                return retval;
                }
                i += inc;
        }

        return 0;
}

static int 
cse788_netlist_postlist(struct cse788_netlist_t *t, struct cse788_netnode_t *n, 
                        int drn, int dir, int flip, int np)
{
        int i, inc, end;
        int retval, m = 0;
        int g1, g2;

        if (np == 'N') {
                g1 = '+';
                g2 = '*';
        } else {
                g2 = '+';
                g1 = '*';
        }

        flip ^= n->flip;
        if ((n->gate == g1 && flip == 0) ||
            (n->gate == g2 && (flip ^ dir) == 0)) {
                i = n->num - 1;
                inc = -1;
                end = -1;
        } else {
                i = 0;
                inc = 1;
                end = n->num;
        }

        while (i != end) {
                if (n->sort[i]->type == _TYPE_PIN) {
                        struct cse788_netentry_t *e = t->mos + t->gate_num++;
                        e->pin = n->sort[i]->pin.name;
                        e->src = n->sort[i]->pin.src;
                        e->drn = drn;
                        if (n->gate == g2)
                                break;
                } else if (n->sort[i]->type == _TYPE_NODE) {
                        int dd;
                        dd = n->gate == g1 ? dir ^ m : dir;
                        retval = cse788_netlist_postlist(t, 
                                        n->sort[i]->node.ptr, drn, dd, flip, 
                                        np);
                        if (0 != retval)
                                return retval;
                        if (n->gate == g2)
                                break;
                }
                m ^= 1;
                i += inc;
        }

        return 0;
}

static int 
cse788_netlist_findlist(struct cse788_netlist_t *t, struct cse788_netnode_t *n, 
                        int *head, int src, int dir, int flip, int np)
{
        int i, sta, inc, end;
        int retval, m = 0;
        int o1 = 0, o2 = 0;
        int g1, g2;

        if (np == 'N') {
                g1 = '+';
                g2 = '*';
        } else {
                g2 = '+';
                g1 = '*';
        }

        flip ^= n->flip;
        if ((n->gate == g1 && flip == 0) ||
            (n->gate == g2 && (flip ^ dir) == 0)) {
                sta = 0;
                inc = 1;
                end = n->num;
        } else {
                sta = n->num - 1;
                inc = -1;
                end = -1;
        }

        i = sta;
        while (i != end) {
                if (n->sort[i]->type != _TYPE_PSEU) {
                        o1 = i;
                        break;
                }
                i += inc;
        }
        i = end - inc;
        while (i != sta - inc) {
                if (n->sort[i]->type != _TYPE_PSEU) {
                        o2 = i;
                        break;
                }
                i -= inc;
        }

        i = sta;
        while (i != end) {
                int tt = (n->gate == g1 || i == o1) ? src : *head;
                if (n->sort[i]->type == _TYPE_PIN) {
                        n->sort[i]->pin.src = tt;
                        if (n->gate == g2 && i != o2) {
                                struct cse788_netentry_t *e = t->mos 
                                        + t->gate_num++;
                                e->pin = n->sort[i]->pin.name;
                                e->src = n->sort[i]->pin.src;
                                e->drn = ++(*head);
                        }
                } else if (n->sort[i]->type == _TYPE_NODE) {
                        int dd;
                        dd = n->gate == g1 ? dir ^ m : dir;
                        retval = cse788_netlist_findlist(t, 
                                        n->sort[i]->node.ptr, head, tt, dd, 
                                        flip, np);
                        if (0 != retval)
                                return retval;
                        if (n->gate == g2 && i != o2) {
                                retval = cse788_netlist_postlist(t, 
                                                n->sort[i]->node.ptr, 
                                                ++(*head), dd, flip, np);
                                if (0 != retval)
                                        return retval;
                        }
                }
                m ^= 1;
                i += inc;
        }

        return 0;
}

static void printfnode1(struct cse788_netnode_t *n, int level)
{
        int i;
        for (i = 0; i < level; i++)
                printf("- ");
        printf("%p %c %d %d %d\n", n, n->gate, n->flip, n->num, n->color);
        for (i = 0; i < level; i++)
                printf("- ");
        for (i = 0; i < n->num; i++) {
                if (n->in[i].type == _TYPE_PIN)
                        printf("%c ", n->in[i].pin.name);
                else if (n->in[i].type == _TYPE_NODE)
                        printf("%p ", n->in[i].node.ptr);
                else
                        printf("ps%c ", n->in[i].pin.name);
        }
        printf("\n");
        for (i = 0; i < n->num; i++) {
                if (n->in[i].type == _TYPE_NODE)
                        printfnode1(n->in[i].node.ptr, level + 1);
        }
        fflush(stdout);
}

static void printfnode2(struct cse788_netnode_t *n, int level)
{
        int i;
        for (i = 0; i < level; i++)
                printf("- ");
        printf("%p %c %d %d %d\n", n, n->gate, n->flip, n->num, n->color);
        for (i = 0; i < level; i++)
                printf("- ");
        for (i = 0; i < n->num; i++) {
                if (n->sort[i]->type == _TYPE_PIN)
                        printf("%c ", n->sort[i]->pin.name);
                else if (n->sort[i]->type == _TYPE_NODE)
                        printf("%p ", n->sort[i]->node.ptr);
                else
                        printf("ps%c ", n->sort[i]->pin.name);
        }
        printf("\n");
        for (i = 0; i < n->num; i++) {
                if (n->sort[i]->type == _TYPE_NODE)
                        printfnode2(n->sort[i]->node.ptr, level + 1);
        }
        fflush(stdout);
}

static void printflist(struct cse788_netlist_t *t)
{
        int i;
        printf("NMOS\n");
        for (i = 0; i < t->gate_num / 2; i++) {
                printf("%c %d %d\n", t->mos[i].pin, t->mos[i].src, 
                                t->mos[i].drn);
        }
        printf("PMOS\n");
        for (; i < t->gate_num; i++) {
                printf("%c %d %d\n", t->mos[i].pin, t->mos[i].src, 
                                t->mos[i].drn);
        }
        fflush(stdout);
}

struct cse788_netlist_t *cse788_netlist_new(const char *func_str)
{
        int retval, idx;
        struct cse788_netlist_t *t;

        if (NULL == func_str) {
                printf("cse788_netlist err: %d\n", __LINE__);
                return NULL;
        }
        t = (struct cse788_netlist_t *)malloc(sizeof *t);
        if (NULL == t) {
                printf("cse788_netlist err: %d\n", __LINE__);
                return NULL;
        }
        do {
                retval = cse788_netlist_trim_func(t, func_str);
                if (0 != retval)
                        break;
                retval = cse788_netlist_parse_func(t);
                if (0 != retval)
                        break;
                printfnode1(t->root, 0);
                retval = cse788_netlist_sort(t->root);
                if (0 != retval)
                        break;
                idx = 0;
                printfnode2(t->root, 0);
                retval = cse788_netlist_findseq(t->root, t->gate_seq, &idx, 0);
                if (0 != retval)
                        break;
                t->gate_seq[idx] = '\0';
                printf("%s\n", t->gate_seq);
                fflush(stdout);
                t->gate_num = 0;
                idx = _SRC_IDX;
                retval = cse788_netlist_findlist(t, t->root, &idx, _SRC_IDX, 
                                1, 0, 'N');
                if (0 != retval)
                        break;
                retval = cse788_netlist_postlist(t, t->root, _DRN_IDX, 
                                1, 0, 'N');
                if (0 != retval)
                        break;
                idx = _SRC_IDX;
                retval = cse788_netlist_findlist(t, t->root, &idx, _SRC_IDX, 
                                0, 0, 'P');
                if (0 != retval)
                        break;
                retval = cse788_netlist_postlist(t, t->root, _DRN_IDX, 
                                0, 0, 'P');
                if (0 != retval)
                        break;
                printflist(t);
        } while (0);

        if (0 != retval) {
                printf("cse788_netlist err: %d\n", retval);
                return NULL;
        }


        return t;
}

int cse788_netlist_gate_seq(struct cse788_netlist_t *t, char *out, int len)
{
        int i = 0;
        const char *seq;

        if (NULL == t)
                return __LINE__;
        if (NULL == out)
                return __LINE__;

        seq = t->gate_seq;
        while (*seq) {
                if (i >= len - 1)
                        return __LINE__;
                if ('%' != *seq || (0 != i && '%' != out[i - 1]))
                        out[i++] = *seq;
                seq++;
        }
        if (out[i - 1] == '%')
                i--;
        out[i] = '\0';

        return 0;
}

int cse788_netlist_list(struct cse788_netlist_t *t, FILE *stream)
{
        int i;

        if (NULL == t)
                return __LINE__;
        fprintf(stream, "netlist\n");

        fprintf(stream, "NMOS:\n");
        fprintf(stream, "%-6s%-6s%-6s\n", "gate", "src", "drn");
        for (i = 0; i < t->gate_num / 2; i++) {
                fprintf(stream, "%-6c", t->mos[i].pin);
                if (t->mos[i].src == _SRC_IDX)
                        fprintf(stream, "%-6s", "GND");
                else
                        fprintf(stream, "N%-5d", t->mos[i].src * 2);
                if (t->mos[i].drn == _DRN_IDX)
                        fprintf(stream, "%-6s", "OUT");
                else
                        fprintf(stream, "N%-5d", t->mos[i].drn * 2);
                printf("\n");
        }

        fprintf(stream, "PMOS:\n");
        fprintf(stream, "%-6s%-6s%-6s\n", "gate", "src", "drn");
        for (; i < t->gate_num; i++) {
                fprintf(stream, "%-6c", t->mos[i].pin);
                if (t->mos[i].src == _SRC_IDX)
                        fprintf(stream, "%-6s", "VDD");
                else
                        fprintf(stream, "P%-5d", t->mos[i].src * 2 + 1);
                if (t->mos[i].drn == _DRN_IDX)
                        fprintf(stream, "%-6s", "OUT");
                else
                        fprintf(stream, "P%-5d", t->mos[i].drn * 2 + 1);
                printf("\n");
        }

        return 0;
}

#include "plus/pie_atom.h"

int 
cse788_netlist_list_s(struct cse788_netlist_t *t, 
                      struct cse788_gate_conn **conn)
{
        int num, i, j;
        char *seq;
        char buf[10];

        num = t->gate_num / 2;
        seq = t->gate_seq;
        *conn = (struct cse788_gate_conn *)calloc(num, sizeof **conn);

        j = 0;
        while (*seq) {
                if (*seq == '%') {
                        seq++;
                        continue;
                }

                for (i = 0; i < num; i++) {
                        if (t->mos[i].pin == *seq) {
                                (*conn)[j].gate = t->mos[i].pin;
                                if (t->mos[i].src == _SRC_IDX)
                                        sprintf(buf, "%s", "GND");
                                else
                                        sprintf(buf, "N%d", t->mos[i].src * 2);
                                (*conn)[j].npins[0] = PieAtom_str(buf);

                                if (t->mos[i].drn == _DRN_IDX)
                                        sprintf(buf, "%s", "OUT");
                                else
                                        sprintf(buf, "N%d", t->mos[i].drn * 2);
                                (*conn)[j].npins[1] = PieAtom_str(buf);
                                break;
                        }
                }
                for (i = num; i < t->gate_num; i++) {
                        if (t->mos[i].pin == *seq) {
                                (*conn)[j].gate = t->mos[i].pin;
                                if (t->mos[i].src == _SRC_IDX)
                                        sprintf(buf, "%s", "VDD");
                                else
                                        sprintf(buf, "P%d", t->mos[i].src*2+1);
                                (*conn)[j].ppins[0] = PieAtom_str(buf);

                                if (t->mos[i].drn == _DRN_IDX)
                                        sprintf(buf, "%s", "OUT");
                                else
                                        sprintf(buf, "P%d", t->mos[i].drn*2+1);
                                (*conn)[j].ppins[1] = PieAtom_str(buf);
                                break;
                        }
                }

                seq++;
                j++;
        }

        return num;
}

static void cse788_netlist_delnode(struct cse788_netnode_t *n)
{
        int i;
        for (i = 0; i < n->num; i++) {
                if (_TYPE_NODE == n->in[i].type)
                        cse788_netlist_delnode(n->in[i].node.ptr);
        }
        free(n->in);
        free(n);
}

void cse788_netlist_del(struct cse788_netlist_t **t)
{
        if (t && *t) {
                struct cse788_netnode_t *n = (*t)->root;
                cse788_netlist_delnode(n);
                free(*t);
                *t = NULL;
        }
}
