/**
 * Pie - Probably incomplete engineering
 * Copyright (C) 2012 - Qiuwen Chen <autchen@hotmail.com>
 *
 * This program is production out of pure fun and study purpose, without any
 * expressed or implied warrenty. Although it is suspected that the work may
 * never go public, if it does, anyone is welcomed to use or modify the codes.
 */

#include "cse788_layout.h"

#include "plus/pie_assert.h"
#include "plus/pie_mem.h"
#include "plus/pie_list.h"
#include "plus/pie_table.h"
#include "plus/pie_atom.h"

#include <time.h>

#define MAX_CONN 27

static const char *NWELL =      "nwell";
static const char *POLY =       "polysilicon";
static const char *PDIFF =      "pdiffusion";
static const char *NDIFF =      "ndiffusion";
static const char *M1 =         "metal1";
static const char *M2 =         "metal2";
static const char *PDCON =      "pdcontact";
static const char *NDCON =      "ndcontact";
static const char *M2Con =      "m2contact";

typedef struct cse788_layout_rect_t {
        const char *layer;
        int x, y, w, h;
        struct cse788_layout_rect_t *ref;
        struct cse788_layout_rect_t *link;
} cse788_layout_rect_t;

typedef struct cse788_layout_net_t  {
        PieAtom name;
        int num_nodes;
        int nodes[MAX_CONN];
} cse788_layout_net_t;

typedef struct cse788_layout_label {
        int x, y;
        const char *layer;
        PieAtom label;
        struct cse788_layout_rect_t *ref;
        struct cse788_layout_label *link;
} cse788_layout_label;

struct cse788_layout_t {
        struct cse788_layout_rect_t *ref1, *ref2;

        struct cse788_layout_rect_t *ndiff;
        struct cse788_layout_rect_t *pdiff;
        struct cse788_layout_rect_t *m2;
        struct cse788_layout_rect_t *m1;
        struct cse788_layout_rect_t *poly;
        struct cse788_layout_rect_t *nwell;
        struct cse788_layout_rect_t *pdcon;
        struct cse788_layout_rect_t *ndcon;
        struct cse788_layout_rect_t *m2con;
        struct cse788_layout_label *labels;
};

int pick_left_most(struct PieList *list, int right)
{
        int i;
        int left = MAX_CONN * 100, leftIdx = -1;
        struct cse788_layout_net_t *net;
        for (i = 0; i < PieList_length(list); i++) {
                net = PieList_get(list, i);
                if (net->nodes[0] > right && net->nodes[0] < left) {
                        left = net->nodes[0];
                        leftIdx = i;
                }
        }
        return leftIdx;
}

void find_left_right(struct cse788_layout_net_t *nn, int *left, int *right)
{
        int i, k;

        *left = MAX_CONN * 100;
        *right = -1;
        for (i = 0; i < nn->num_nodes; i++) {
                if (nn->nodes[i] < 0)
                        k = -nn->nodes[i] - 1;
                else
                        k = nn->nodes[i];
                if (k < *left)
                        *left = k;
                else if (k > *right)
                        *right = k;
        }
}

struct cse788_layout_t *
cse788_layout_new(char *seq, struct cse788_gate_conn *conn)
{
        int num, i, j, base, ttt;
        struct cse788_layout_t *t;
        char *_seq;
        struct PieList *nets;
        struct PieTable *set;
        int track, right, left, track1;
        struct cse788_layout_rect_t *rr, *rr1;
        struct cse788_layout_net_t *nn;
        struct cse788_layout_label *label;

        t = PIE_NEW0(struct cse788_layout_t);

        /* pdiff */
        t->pdiff = PIE_NEW0(struct cse788_layout_rect_t);
        t->ref2 = t->pdiff;
        t->pdiff->layer = PDIFF;
        t->pdiff->h = 9;

        /* ndiff */
        _seq = seq;
        num = 0;
        t->ndiff = PIE_NEW0(struct cse788_layout_rect_t);
        t->ref1 = t->ndiff;
        t->ndiff->h = 4;
        t->ndiff->layer = NDIFF;
        while (*_seq) {
                if (*_seq == '%') {
                        struct cse788_layout_rect_t *tmp;
                        t->ndiff->w += 4;
                        tmp = PIE_NEW0(struct cse788_layout_rect_t);
                        tmp->layer = NDIFF;
                        tmp->x = t->ndiff->w + 12;
                        tmp->h = t->ndiff->h;
                        tmp->ref = t->ndiff;
                        tmp->link = t->ndiff;
                        t->ndiff = tmp;
                } else {
                        t->ndiff->w += 8;
                        num++;
                }
                _seq++;
        }
        t->ndiff->w += 4;

        /* reorder the pins */
        _seq = seq + 1;
        base = 0;
        for (i = 0; i < num - 1; i++) {
                if (*_seq == '%') {
                        base = i + 1;
                        _seq++;
                }
                _seq++;
                if (conn[i].npins[1] == conn[i + 1].npins[1]) {
                        PieAtom tmp = conn[i + 1].npins[0];
                        conn[i + 1].npins[0] = conn[i + 1].npins[1];
                        conn[i + 1].npins[1] = tmp;
                } else if (conn[i].npins[1] != conn[i + 1].npins[0]) {
                        if (conn[i].npins[0] == conn[i + 1].npins[1]) {
                                PieAtom tmp = conn[i + 1].npins[0];
                                conn[i + 1].npins[0] = conn[i + 1].npins[1];
                                conn[i + 1].npins[1] = tmp;
                        }
                        for (j = i; j >= base; j--) {
                                PieAtom tmp = conn[j].npins[0];
                                if (tmp != conn[j + 1].npins[0])
                                        break;
                                conn[j].npins[0] = conn[j].npins[1];
                                conn[j].npins[1] = tmp;
                        }
                }
                if (conn[i].ppins[1] == conn[i + 1].ppins[1]) {
                        PieAtom tmp = conn[i + 1].ppins[0];
                        conn[i + 1].ppins[0] = conn[i + 1].ppins[1];
                        conn[i + 1].ppins[1] = tmp;
                } else if (conn[i].ppins[1] != conn[i + 1].ppins[0]) {
                        if (conn[i].ppins[0] == conn[i + 1].ppins[1]) {
                                PieAtom tmp = conn[i + 1].ppins[0];
                                conn[i + 1].ppins[0] = conn[i + 1].ppins[1];
                                conn[i + 1].ppins[1] = tmp;
                        }
                        for (j = i; j >= base; j--) {
                                PieAtom tmp = conn[j].ppins[0];
                                if (tmp != conn[j + 1].ppins[0])
                                        break;
                                conn[j].ppins[0] = conn[j].ppins[1];
                                conn[j].ppins[1] = tmp;
                        }
                }
        }

        /* n nets */
        nets = PieList_new(0);
        set = PieTable_new(10, PieTable_atomcmp, PieTable_atomhash);
        _seq = seq;
        i = 0;
        base = 0;
        while (*_seq || i <= num) {
                struct cse788_layout_net_t *net;
                PieAtom aa;

                if (*_seq == '%' || i == num) 
                        aa = conn[i - 1].npins[1];
                else
                        aa = conn[i].npins[0];

                if (PIE_NULL == (net = PieTable_get(set, aa))) {
                        net = PIE_NEW0(struct cse788_layout_net_t);
                        net->name = aa;
                        net->nodes[0] = base + i;
                        net->num_nodes = 1;
                        if (aa != PieAtom_str("OUT") && aa != PieAtom_str("GND") 
                                        && aa != PieAtom_str("VDD"))
                                PieList_push(nets, net);
                        PieTable_put(set, net->name, net);
                } else {
                        net->nodes[net->num_nodes] = base + i;
                        net->num_nodes++;
                }
                if (*_seq == '%') 
                        base += 100;
                else
                        i++;
                if (i <= num)
                        _seq++;

        }

        track = 0;
        right = -1;
        while (PieList_length(nets) != 0) {
                int left;
                struct cse788_layout_net_t *net;
                struct cse788_layout_rect_t *rect;

                left = pick_left_most(nets, right);
                if (left == -1) {
                        right = -1;
                        track++;
                        continue;
                }
                net = PieList_remove(nets, left, PIE_NULL);
                if (net->num_nodes < 2)
                        continue;

                base = net->nodes[0] / 100;
                ttt = net->nodes[0] % 100;
                rect = PIE_NEW0(struct cse788_layout_rect_t);
                rect->layer = M2;
                rect->x = 8 * ttt + 16 * base;
                rect->y = 8 + track * 8;
                base = net->nodes[net->num_nodes - 1] / 100;
                ttt = net->nodes[net->num_nodes - 1] % 100;
                rect->w = 8 * ttt + 16 * base + 4 - rect->x;
                rect->h = 4;
                rect->ref = t->ref1;
                rect->link = t->m2;
                t->m2 = rect;
                right = net->nodes[net->num_nodes - 1];

                for (i = 0; i < net->num_nodes; i++) {
                        base = net->nodes[i] / 100;
                        ttt = net->nodes[i] % 100;
                        rect = PIE_NEW0(struct cse788_layout_rect_t);
                        rect->layer = M1;
                        rect->x = 8 * ttt + 16 * base;
                        rect->y = 0;
                        rect->w = 4;
                        rect->h = 8 + track * 8 + 4;
                        rect->ref = t->ref1;
                        rect->link = t->m1;
                        t->m1 = rect;
                        /* contact */
                        rr = PIE_NEW0(struct cse788_layout_rect_t);
                        rr->layer = NDCON;
                        rr->x = rect->x;
                        rr->y = rect->y;
                        rr->w = 4;
                        rr->h = 4;
                        rr->ref = rect->ref;
                        rr->link = t->ndcon;
                        t->ndcon = rr;
                        rr = PIE_NEW0(struct cse788_layout_rect_t);
                        rr->layer = M2Con;
                        rr->x = rect->x;
                        rr->y = rect->y + rect->h - 4;
                        rr->w = 4;
                        rr->h = 4;
                        rr->ref = rect->ref;
                        rr->link = t->m2con;
                        t->m2con = rr;
                }
        }
        if (right != -1)
                track++;

        /* p nets */
        PieList_clean(nets);
        _seq = seq;
        i = 0;
        base = 0;
        while (*_seq || i <= num) {
                struct cse788_layout_net_t *net;
                PieAtom aa;

                if (*_seq == '%' || i == num) 
                        aa = conn[i - 1].ppins[1];
                else
                        aa = conn[i].ppins[0];

                if (PIE_NULL == (net = PieTable_get(set, aa))) {
                        net = PIE_NEW0(struct cse788_layout_net_t);
                        net->name = aa;
                        net->nodes[0] = base + i;
                        net->num_nodes = 1;
                        if (aa != PieAtom_str("OUT") && aa != PieAtom_str("GND") 
                                        && aa != PieAtom_str("VDD"))
                                PieList_push(nets, net);
                        PieTable_put(set, net->name, net);
                } else {
                        net->nodes[net->num_nodes] = base + i;
                        if (aa == PieAtom_str("OUT")) {
                                net->nodes[net->num_nodes] = 
                                        -net->nodes[net->num_nodes] - 1;
                        }
                        net->num_nodes++;
                }
                if (*_seq == '%') 
                        base += 100;
                else
                        i++;
                if (i <= num)
                        _seq++;

        }

        track1 = 0;
        right = -1;
        while (PieList_length(nets) != 0) {
                struct cse788_layout_net_t *net;
                struct cse788_layout_rect_t *rect;

                left = pick_left_most(nets, right);
                if (left == -1) {
                        right = -1;
                        track1++;
                        continue;
                }
                net = PieList_remove(nets, left, PIE_NULL);
                if (net->num_nodes < 2)
                        continue;

                base = net->nodes[0] / 100;
                ttt = net->nodes[0] % 100;
                rect = PIE_NEW0(struct cse788_layout_rect_t);
                rect->layer = M2;
                rect->x = 8 * ttt + 16 * base;
                rect->y = -(8 + track1 * 8);
                base = net->nodes[net->num_nodes - 1] / 100;
                ttt = net->nodes[net->num_nodes - 1] % 100;
                rect->w = 8 * ttt + 16 * base + 4 - rect->x;
                rect->h = 4;
                rect->ref = t->ref2;
                rect->link = t->m2;
                t->m2 = rect;
                right = net->nodes[net->num_nodes - 1];

                for (i = 0; i < net->num_nodes; i++) {
                        base = net->nodes[i] / 100;
                        ttt = net->nodes[i] % 100;
                        rect = PIE_NEW0(struct cse788_layout_rect_t);
                        rect->layer = M1;
                        rect->x = 8 * ttt + 16 * base;
                        rect->y = -(8 + track1 * 8);
                        rect->w = 4;
                        rect->h = 16 + track1 * 8 + 1;
                        rect->ref = t->ref2;
                        rect->link = t->m1;
                        t->m1 = rect;
                        /* contact */
                        rr = PIE_NEW0(struct cse788_layout_rect_t);
                        rr->layer = PDCON;
                        rr->x = rect->x;
                        rr->y = rect->y + rect->h - 4;
                        rr->w = 4;
                        rr->h = 4;
                        rr->ref = rect->ref;
                        rr->link = t->pdcon;
                        t->pdcon = rr;
                        rr = PIE_NEW0(struct cse788_layout_rect_t);
                        rr->layer = PDCON;
                        rr->x = rect->x;
                        rr->y = rect->y + rect->h - 4 - 5;
                        rr->w = 4;
                        rr->h = 4;
                        rr->ref = rect->ref;
                        rr->link = t->pdcon;
                        t->pdcon = rr;
                        rr = PIE_NEW0(struct cse788_layout_rect_t);
                        rr->layer = M2Con;
                        rr->x = rect->x;
                        rr->y = rect->y;
                        rr->w = 4;
                        rr->h = 4;
                        rr->ref = rect->ref;
                        rr->link = t->m2con;
                        t->m2con = rr;
                }
        }
        if (right != -1)
                track1++;

        /* pdiff */
        t->pdiff->x = 0;
        t->pdiff->y = (track + track1 + 1) * 8 + 8;
        _seq = seq;
        while (*_seq) {
                if (*_seq == '%') {
                        struct cse788_layout_rect_t *tmp;
                        t->pdiff->w += 4;
                        tmp = PIE_NEW0(struct cse788_layout_rect_t);
                        tmp->layer = PDIFF;
                        tmp->x = t->pdiff->w + 12;
                        tmp->y = 0;
                        tmp->h = t->pdiff->h;
                        tmp->ref = t->pdiff;
                        tmp->link = t->pdiff;
                        t->pdiff = tmp;
                } else {
                        t->pdiff->w += 8;
                        num++;
                }
                _seq++;
        }
        t->pdiff->w += 4;

        /* GND */
        rr = (struct cse788_layout_rect_t *)PIE_MALLOC(sizeof *rr);
        rr->layer = M1;
        rr->ref = t->ref1;
        rr->x = -6;
        rr->y = -12;
        rr->w = 6 + 6 + t->ndiff->w;
        rr1 = t->ndiff;
        while (rr1) {
                rr->w += rr1->x;
                rr1 = rr1->link;
        }
        rr->h = 8;
        rr->link = t->m1;
        t->m1 = rr;
        /* label */
        label = PIE_NEW0(struct cse788_layout_label);
        label->layer = M1;
        label->label = PieAtom_str("GND");
        label->x = rr->x + rr->w / 2;
        label->y = rr->y + rr->h / 2;
        label->ref = rr->ref;
        label->link = t->labels;
        t->labels = label;
        nn = (struct cse788_layout_net_t *)PieTable_get(set, 
                        PieAtom_str("GND"));
        for (i = 0; i < nn->num_nodes; i++) {
                base = nn->nodes[i] / 100;
                ttt = nn->nodes[i] % 100;
                rr = PIE_NEW0(struct cse788_layout_rect_t);
                rr->layer = M1;
                rr->x = 8 * ttt + 16 * base;
                rr->y = -4;
                rr->w = 4;
                rr->h = 8;
                rr->ref = t->ref1;
                rr->link = t->m1;
                t->m1 = rr;
                /* contact */
                rr1 = PIE_NEW0(struct cse788_layout_rect_t);
                rr1->layer = NDCON;
                rr1->x = rr->x;
                rr1->y = rr->y + rr->h - 4;
                rr1->w = 4;
                rr1->h = 4;
                rr1->ref = rr->ref;
                rr1->link = t->ndcon;
                t->ndcon = rr1;
        }

        /* VDD */
        rr = (struct cse788_layout_rect_t *)PIE_MALLOC(sizeof *rr);
        rr->layer = M1;
        rr->ref = t->ref2;
        rr->x = -6;
        rr->y = 12 + 1;
        rr->w = 6 + 6 + t->pdiff->w;
        rr1 = t->pdiff;
        while (rr1) {
                rr->w += rr1->x;
                rr1 = rr1->link;
        }
        rr->h = 8;
        rr->link = t->m1;
        t->m1 = rr;
        /* label */
        label = PIE_NEW0(struct cse788_layout_label);
        label->layer = M1;
        label->label = PieAtom_str("VDD");
        label->x = rr->x + rr->w / 2;
        label->y = rr->y + rr->h / 2;
        label->ref = rr->ref;
        label->link = t->labels;
        t->labels = label;
        nn = (struct cse788_layout_net_t *)PieTable_get(set, 
                        PieAtom_str("VDD"));
        for (i = 0; i < nn->num_nodes; i++) {
                base = nn->nodes[i] / 100;
                ttt = nn->nodes[i] % 100;
                rr = PIE_NEW0(struct cse788_layout_rect_t);
                rr->layer = M1;
                rr->x = 8 * ttt + 16 * base;
                rr->y = 0;
                rr->w = 4;
                rr->h = 12 + 1;
                rr->ref = t->ref2;
                rr->link = t->m1;
                t->m1 = rr;
                /* contact */
                rr1 = PIE_NEW0(struct cse788_layout_rect_t);
                rr1->layer = PDCON;
                rr1->x = rr->x;
                rr1->y = rr->y;
                rr1->w = 4;
                rr1->h = 4;
                rr1->ref = rr->ref;
                rr1->link = t->pdcon;
                t->pdcon = rr1;
                rr1 = PIE_NEW0(struct cse788_layout_rect_t);
                rr1->layer = PDCON;
                rr1->x = rr->x;
                rr1->y = rr->y + 5;
                rr1->w = 4;
                rr1->h = 4;
                rr1->ref = rr->ref;
                rr1->link = t->pdcon;
                t->pdcon = rr1;
        }

        /* OUT */
        nn = (struct cse788_layout_net_t *)PieTable_get(set, 
                        PieAtom_str("OUT"));
        find_left_right(nn, &left, &right);
        base = left / 100;
        ttt = left % 100;
        rr = PIE_NEW0(struct cse788_layout_rect_t);
        rr->layer = M2;
        rr->x = 8 * ttt + 16 * base;
        rr->y = 8 + (track) * 8;
        base = right / 100;
        ttt = right % 100;
        rr->w = 8 * ttt + 16 * base + 4 - rr->x;
        rr->h = 4;
        rr->ref = t->ref1;
        rr->link = t->m2;
        t->m2 = rr;
        /* label */
        label = PIE_NEW0(struct cse788_layout_label);
        label->layer = M2;
        label->label = PieAtom_str("OUT");
        label->x = rr->x + rr->w / 2;
        label->y = rr->y + rr->h / 2;
        label->ref = rr->ref;
        label->link = t->labels;
        t->labels = label;

        for (i = 0; i < nn->num_nodes; i++) {
                int kk;
                if (nn->nodes[i] < 0)
                        kk = -nn->nodes[i] - 1;
                else 
                        kk = nn->nodes[i];
                base = kk / 100;
                ttt = kk % 100;
                rr = PIE_NEW0(struct cse788_layout_rect_t);
                rr->layer = M1;
                rr->x = 8 * ttt + 16 * base;
                rr->w = 4;
                if (nn->nodes[i] < 0) {
                        rr->y = -(8 + track1 * 8);
                        rr->h = 16 + track1 * 8 + 1;
                        rr->ref = t->ref2;
                        /* contact */
                        rr1 = PIE_NEW0(struct cse788_layout_rect_t);
                        rr1->layer = PDCON;
                        rr1->x = rr->x;
                        rr1->y = rr->y + rr->h - 4;
                        rr1->w = 4;
                        rr1->h = 4;
                        rr1->ref = rr->ref;
                        rr1->link = t->pdcon;
                        t->pdcon = rr1;
                        rr1 = PIE_NEW0(struct cse788_layout_rect_t);
                        rr1->layer = PDCON;
                        rr1->x = rr->x;
                        rr1->y = rr->y + rr->h - 4 - 5;
                        rr1->w = 4;
                        rr1->h = 4;
                        rr1->ref = rr->ref;
                        rr1->link = t->pdcon;
                        t->pdcon = rr1;
                        rr1 = PIE_NEW0(struct cse788_layout_rect_t);
                        rr1->layer = M2Con;
                        rr1->x = rr->x;
                        rr1->y = rr->y;
                        rr1->w = 4;
                        rr1->h = 4;
                        rr1->ref = rr->ref;
                        rr1->link = t->m2con;
                        t->m2con = rr1;
                } else {
                        rr->y = 0;
                        rr->h = 8 + (track) * 8 + 4;
                        rr->ref = t->ref1;
                        /* contact */
                        rr1 = PIE_NEW0(struct cse788_layout_rect_t);
                        rr1->layer = NDCON;
                        rr1->x = rr->x;
                        rr1->y = rr->y;
                        rr1->w = 4;
                        rr1->h = 4;
                        rr1->ref = rr->ref;
                        rr1->link = t->ndcon;
                        t->ndcon = rr1;
                        rr1 = PIE_NEW0(struct cse788_layout_rect_t);
                        rr1->layer = M2Con;
                        rr1->x = rr->x;
                        rr1->y = rr->y + rr->h - 4;
                        rr1->w = 4;
                        rr1->h = 4;
                        rr1->ref = rr->ref;
                        rr1->link = t->m2con;
                        t->m2con = rr1;
                }
                rr->link = t->m1;
                t->m1 = rr;
        }

        /* poly */
        _seq = seq;
        ttt = 0;
        base = 0;
        while (*_seq) {
                if (*_seq != '%') {
                        rr = PIE_NEW0(struct cse788_layout_rect_t);
                        rr->layer = POLY;
                        rr->x = 5 + 8 * ttt + 16 * base;
                        rr->w = 2;
                        rr->y = -2;
                        rr->h = 2 + 8 * (track + track1 + 1) + 8 + 9 + 2;
                        rr->ref = t->ref1;
                        rr->link = t->poly;
                        t->poly = rr;
                        ttt++;
                        /* label */
                        label = PIE_NEW0(struct cse788_layout_label);
                        label->layer = POLY;
                        label->label = PieAtom_new(_seq, 1);
                        label->x = rr->x + rr->w / 2;
                        label->y = rr->y + rr->h / 2;
                        label->ref = rr->ref;
                        label->link = t->labels;
                        t->labels = label;
                } else {
                        base++;
                }
                _seq++;
        }

        /* nwell */
        rr1 = t->pdiff;
        while (rr1) {
                rr = PIE_NEW0(struct cse788_layout_rect_t);
                rr->layer = NWELL;
                rr->ref = rr1->ref;
                rr->x = rr1->x - 6;
                rr->y = rr1->y - 6;
                rr->w = rr1->w + 12;
                rr->h = rr1->h + 12;
                rr->link = t->nwell;
                t->nwell = rr;
                rr1 = rr1->link;
        }

        return t;
}

static void magic_rect(struct cse788_layout_rect_t *t, FILE *stream)
{
        struct cse788_layout_rect_t *rect;
        int basex, basey;
        struct cse788_layout_rect_t *rr;

        rect = t;
        fprintf(stream, "<< %s >>\n", rect->layer);
        while (rect) {
                basex = 0;
                basey = 0;
                rr = rect->ref;
                while (rr) {
                        basex += rr->x;
                        basey += rr->y;
                        rr = rr->ref;
                }
                fprintf(stream, "rect %d %d %d %d\n", 
                                rect->x + basex, rect->y + basey, 
                                rect->x + rect->w + basex, 
                                rect->y + rect->h + basey);
                rect = rect->link;
        }
}

static void magic_label(struct cse788_layout_label *t, FILE *stream)
{
        int basex, basey;
        struct cse788_layout_rect_t *rr;

        fprintf(stream, "<< %s >>\n", "labels");
        while (t) {
                basex = 0;
                basey = 0;
                rr = t->ref;
                while (rr) {
                        basex += rr->x;
                        basey += rr->y;
                        rr = rr->ref;
                }
                fprintf(stream, "rlabel %s %d %d %d %d 3 %s\n", 
                                t->layer, t->x + basex, t->y + basey, 
                                t->x + basex, t->y + basey,
                                (const char *)t->label);
                t = t->link;
        }
}

void cse788_layout_magic(struct cse788_layout_t *t, FILE *stream)
{
        fprintf(stream, "magic\n");
        fprintf(stream, "tech scmos\n");
        fprintf(stream, "timestamp %d\n", (int)time(PIE_NULL));
        magic_rect(t->ndiff, stream);
        magic_rect(t->pdiff, stream);
        magic_rect(t->m2, stream);
        magic_rect(t->m1, stream);
        magic_rect(t->poly, stream);
        magic_rect(t->nwell, stream);
        magic_rect(t->ndcon, stream);
        magic_rect(t->pdcon, stream);
        magic_rect(t->m2con, stream);
        magic_label(t->labels, stream);
        fprintf(stream, "<< end >>\n");
}
