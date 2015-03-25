
#include "cse788_floorplan.h"

#include "plus/pie_assert.h"
#include "plus/pie_except.h"
#include "plus/pie_file.h"
#include "plus/pie_list.h"
#include "plus/pie_mem.h"
#include "plus/pie_random.h"

#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#define POLISH_V -1
#define POLISH_H -2
#define BUF_LEN 1024
#define INTERP_POINTS 3
#define NON_FIT_DCOST 300.0

const struct PieExcept cse788_floorplan_failed = {"floorplan failed"};

struct cse788_corner_point_t {
        double x, y;
        int child_idx[2];
};

struct cse788_floorplan_node_t {
        int index;
        struct PieList *bounding_curve; /* r<cse788_corner_point_t> */

        struct cse788_floorplan_node_t *child[2];

        double x, y, w, h;
};

struct cse788_floorplan_t {
        int *polish, *polish_d, len_polish;
        double p, q;
        struct PieList *list_nodes;     /* m<cse788_floorplan_node_t *> */
        struct PieList *stack;          /* r<cse788_floorplan_node_t *> */
        struct cse788_floorplan_node_t *free_nodes;
        struct cse788_floorplan_node_t *root;
        struct PieRandom *seed;
        PieBool show_progress;
};

static struct cse788_floorplan_node_t *
cse788_floorplan_node_alloc(struct cse788_floorplan_t *t)
{
        struct cse788_floorplan_node_t *node;

        if (t->free_nodes) {
                node = t->free_nodes;
                t->free_nodes = node->child[0];
        } else {
                node = PIE_NEW0(struct cse788_floorplan_node_t);
                node->bounding_curve = PieList_new(
                                sizeof(struct cse788_corner_point_t));
        }

        return node;
}

static void 
cse788_floorplan_node_dealloc(struct cse788_floorplan_t *t, 
                              struct cse788_floorplan_node_t *node)
{
        node->child[0] = t->free_nodes;
        t->free_nodes = node;
}

static struct PieList *
cse788_floorplan_calc_bc(double a, double r, double s, int domain)
{
        struct PieList *bc;
        struct cse788_corner_point_t pt, pt2, ptt;
        int i, j;
        double tmp;

        bc = PieList_new(sizeof(struct cse788_corner_point_t));
        PIE_ASSERT(domain == 1 || domain == 2);

        for (i = 0; i < domain; i++) {
                if (s - r <= 0.01) {    /* rigid module */
                        pt.x = sqrt(a / s);
                        pt.y = s * pt.x;
                        PieList_push(bc, &pt);
                } else {                /* flexible module */
                        double itvl;

                        pt.x = sqrt(a / s);
                        pt.y = s * pt.x;
                        PieList_push(bc, &pt);
                        pt2.x = sqrt(a / r);
                        pt2.y = r * pt2.x;
                        PieList_push(bc, &pt2);

                        /* Interporation */
                        itvl = (pt2.x - pt.x) / (1.0 + INTERP_POINTS);
                        ptt.x = pt.x;
                        for (j = 0; j < INTERP_POINTS; j++) {
                                ptt.x += itvl;
                                ptt.y = a / ptt.x;
                                PieList_push(bc, &ptt);
                        }
                }

                /* change orientation */
                tmp = r;
                r = 1.0 / s;
                if (r - tmp <= 0.01 && r - tmp >= -0.01)
                        break;
                s = 1.0 / tmp;
        }

        return bc;
}

static void 
cse788_floorplan_import(struct cse788_floorplan_t *t, struct PieFile *file)
{
        int ret, num_modules;
        char linebuf[BUF_LEN];
        /* int i; */

        PIE_ASSERT(t);
        PIE_ASSERT(file);

        /* first line */
        file->readLine(file, linebuf, BUF_LEN);
        ret = sscanf(linebuf, "%d %lf %lf", &num_modules, &t->p, &t->q);
        if (ret != 3) {
                PIE_RAISE_INFO(cse788_floorplan_failed, 
                        ("Invalid format: %s", linebuf));
        }

        while (file->readLine(file, linebuf, BUF_LEN)) {
                double a, r, s;
                int index, domain;
                struct cse788_floorplan_node_t node;

                ret = sscanf(linebuf, "%d %lf %lf %lf %d", &index, &a, &r, &s, 
                                &domain);
                if (ret != 5 || index < 0 || a < 0.0 || r > s) {
                        PIE_RAISE_INFO(cse788_floorplan_failed,
                                ("Invalid format: %s", linebuf));
                }
                /* node = PIE_NEW(struct cse788_floorplan_node_t); */
                node.index = index;
                node.bounding_curve = cse788_floorplan_calc_bc(a, r, s, 
                                domain);
                node.child[0] = PIE_NULL;
                node.child[1] = PIE_NULL;
                node.x = 0.0;
                node.y = 0.0;
                node.w = 0.0;
                node.h = 0.0;
                PieList_push(t->list_nodes, &node);

                /* do { */
                        /* printf("\n"); */
                        /* int i; */
                        /* struct PieList *l = node->bounding_curve; */
                        /* for (i = 0; i < PieList_length(l); i++) { */
                                /* struct cse788_corner_point_t *p; */
                                /* p = (struct cse788_corner_point_t *) */
                                        /* PieList_get(l, i); */
                                /* printf("(%.2lf, %.2lf)\n", p->x, p->y); */
                        /* } */
                /* } while (0); */
        }

        if (PieList_length(t->list_nodes) != num_modules) {
                PIE_RAISE_INFO(cse788_floorplan_failed,
                        ("Inconsistent module number"));
        }
        
        /* Initiate naive layout */
        t->polish = (int *)PIE_MALLOC(sizeof(int) * 2 * num_modules);
        t->polish_d = (int *)PIE_MALLOC(sizeof(int) * 2 * num_modules);
}

static void 
cse788_floorplan_parsepolish(struct cse788_floorplan_t *t, int *polish)
{
        int i;
        struct PieList *stack;
        const struct PieListRep *rep;
        struct cse788_floorplan_node_t *node;

        stack = t->stack;
        rep = PieList_rep(t->list_nodes);
        for (i = 0; i < t->len_polish; i++) {
                int idx = polish[i];
                if (i > 0 && idx == polish[i - 1]) {
                        PIE_RAISE_INFO(cse788_floorplan_failed,
                                ("Polish expression not skewed"));
                }
                if (idx >= 0) {
                        node = (struct cse788_floorplan_node_t *)rep->ary[idx];
                        PieList_push(stack, node);
                } else {
                        struct cse788_floorplan_node_t *n;
                        n = cse788_floorplan_node_alloc(t);
                        n->index = idx;
                        n->child[1] = (struct cse788_floorplan_node_t *)
                                        PieList_pop(stack, PIE_NULL);
                        n->child[0] = (struct cse788_floorplan_node_t *)
                                        PieList_pop(stack, PIE_NULL);
                        PieList_push(stack, n);
                }
        }

        if (PieList_length(stack) != 1) {
                PIE_RAISE_INFO(cse788_floorplan_failed,
                        ("Invalid polish expression"));
        }
        t->root = (struct cse788_floorplan_node_t *)PieList_pop(stack, 
                        PIE_NULL);
}

static void 
cse788_floorplan_comb_bc(struct PieList *bc1, struct PieList *bc2,
                         struct PieList *bc, int d)
{
        int i, j, idx;
        struct cse788_corner_point_t *p1, *p2, p;
        const struct PieListRep *l1, *l2, *ll;

        l1 = PieList_rep(bc1);
        l2 = PieList_rep(bc2);
        if (d == POLISH_V) {
                for (i = 0; i < l1->len; i++) {
                        p1 = (struct cse788_corner_point_t *)l1->ary[i];
                        for (j = 0; j < l2->len; j++) {
                                p2 = (struct cse788_corner_point_t *)l2->ary[j];
                                p.x = p1->x > p2->x ? p1->x : p2->x;
                                p.y = p1->y + p2->y;
                                p.child_idx[0] = i;
                                p.child_idx[1] = j;
                                PieList_push(bc, &p);
                        }
                }
        } else {
                for (i = 0; i < l1->len; i++) {
                        p1 = (struct cse788_corner_point_t *)l1->ary[i];
                        for (j = 0; j < l2->len; j++) {
                                p2 = (struct cse788_corner_point_t *)l2->ary[j];
                                p.y = p1->y > p2->y ? p1->y : p2->y;
                                p.x = p1->x + p2->x;
                                p.child_idx[0] = i;
                                p.child_idx[1] = j;
                                PieList_push(bc, &p);
                        }
                }
        }
        /* remove redundency */
        ll = PieList_rep(bc);
        for (i = 0, idx = 0; i < ll->len; i++) {
                p1 = (struct cse788_corner_point_t *)ll->ary[i];
                for (j = 0; j < ll->len; j++) {
                        p2 = (struct cse788_corner_point_t *)ll->ary[j];
                        if ((p1->x >= p2->x && p1->y > p2->y) ||
                            (p1->x > p2->x && p1->y >= p2->y))
                                break;
                }
                if (j >= ll->len) {
                        PieList_set(bc, idx, p1);
                        idx++;
                }
        }
        PieList_truncate(bc, idx);
}

static struct PieList *
cse788_floorplan_recur_bc(struct cse788_floorplan_node_t *node)
{
        struct PieList *bc1, *bc2, *bc;

        if (node->index >= 0)
                return node->bounding_curve;

        PIE_ASSERT(node->child[0] && node->child[1]);
        bc1 = cse788_floorplan_recur_bc(node->child[0]);
        bc2 = cse788_floorplan_recur_bc(node->child[1]);
        bc = node->bounding_curve;
        cse788_floorplan_comb_bc(bc1, bc2, bc, node->index);

        return node->bounding_curve;
}

void ppp(struct cse788_floorplan_node_t *node, int level)
{
        int i;

        for (i = 0; i < level; i++)
                printf("-");
        printf("n:%p,i:%d,l:%p,r:%p\n", node, node->index, node->child[0],
                        node->child[1]);
        for (i = 0; i < level; i++)
                printf("-");
        for (i = 0; i < PieList_length(node->bounding_curve); i++) {
                struct cse788_corner_point_t *p;
                p = (struct cse788_corner_point_t *)PieList_get(
                                node->bounding_curve, i);
                printf("%d[%.2lf,%.2lf,%d,%d]", i, p->x, p->y, 
                                node->index < 0 ? p->child_idx[0] : 0,
                                node->index < 0 ? p->child_idx[1] : 0);
        }
        printf("\n");
        if (node->child[0] && node->child[1]) {
                ppp(node->child[0], level + 1);
                ppp(node->child[1], level + 1);
        }
}

void cse788_print_polish(struct PieList *nodes, int len, int *polish) {
        int i;
        for (i = 0; i < len; i++) {
                if (polish[i] == POLISH_H)
                        printf("+");
                else if (polish[i] == POLISH_V)
                        printf("*");
                else {
                        struct cse788_floorplan_node_t *n;
                        n = (struct cse788_floorplan_node_t *)PieList_get(
                                        nodes, polish[i]);
                        printf("(%d)", n->index);
                }
        }
        printf("\n");
}

static void 
cse788_floorplan_random_move(struct cse788_floorplan_t *t, int *polish, 
                             int *polish_d)
{
        int op, idx, idx1, i, j, try, tried;
        int len;
        PieBool after, fb;

        len = PieList_length(t->list_nodes);
        tried = 0;
        op = (int)PieRandom_range(t->seed, 0.0, 3.0);
        switch (op) {
        case 0: /* M1 */
                idx = (int)PieRandom_range(t->seed, 0.0, (double)len - 1.0);
                for (i = 0; i < t->len_polish; i++) {
                        if (polish[i] >= 0 && idx-- <= 0)
                                break;
                }
                for (j = i + 1; j < t->len_polish; j++) {
                        if (polish[j] >= 0) {
                                idx = polish[i];
                                polish[i] = polish[j];
                                polish[j] = idx;
                                break;
                        }
                }
                /* printf("M1\n"); */
                break;
        case 1: /* M2 */
                idx = (int)PieRandom_range(t->seed, 0.0, (double)len);
                try = PieRandom_bin(t->seed, 0.5) ? 1 : -1;
                do {
                        for (i = 0, idx1 = idx; i < t->len_polish; i++) {
                                if (polish[i] >= 0 && idx1-- <= 0)
                                        break;
                        }
                        if (polish[++i] >= 0) {
                                idx += try;
                                if (idx < 0)
                                        idx = len - 1;
                                else if (idx >= len)
                                        idx = 0;
                                tried++;
                                continue;
                        }
                        for (; i < t->len_polish; i++) {
                                if (polish[i] >= 0)
                                        break;
                                polish[i] = polish[i] == POLISH_H ? 
                                                POLISH_V : POLISH_H;
                        }
                        break;
                } while (tried < len);
                /* printf("M2\n"); */
                break;
        case 2: /* M3 */
                idx = (int)PieRandom_range(t->seed, 0.0, (double)len);
                try = PieRandom_bin(t->seed, 0.5) ? 1 : -1;
                after = PieRandom_bin(t->seed, 0.5);
                fb = PIE_FALSE;
                do {
                        for (i = 0, idx1 = idx; i < t->len_polish; i++) {
                                if (polish[i] >= 0 && idx1-- <= 0)
                                        break;
                        }
                        if (i <= 1 || after) {
                                if (polish[i + 1] < 0 &&
                                    2 * polish_d[i + 1] <= i &&
                                    polish[i - 1] != polish[i + 1]) {
                                        idx = polish[i];
                                        polish[i] = polish[i + 1];
                                        polish[i + 1] = idx;
                                        polish_d[i]++;
                                        break;
                                }
                        } else {
                                if (polish[i - 1] < 0 &&
                                    polish[i + 1] != polish[i - 1]) {
                                        idx = polish[i];
                                        polish[i] = polish[i - 1];
                                        polish[i - 1] = idx;
                                        polish_d[i - 1]--;
                                        break;
                                }
                        }
                        if (!fb) {
                                after = !after;
                                fb = PIE_TRUE;
                                continue;
                        }
                        idx += try;
                        if (idx < 0)
                                idx = len - 1;
                        else if (idx >= len)
                                idx = 0;
                        tried++;
                        fb = PIE_FALSE;
                } while (tried < len);
                /* printf("M3\n"); */
                break;
        default:
                PIE_ASSERT_RELEASE(0);
        }
}

static PieBool 
cse788_floorplan_cost(struct cse788_floorplan_t *t, double *pcost, int *pidx)
{
        int i, idx = 0, idxf = 0;
        const struct PieListRep *rep;
        struct cse788_corner_point_t *p;
        double cost, a, costf;
        PieBool retval = PIE_FALSE;

        rep = PieList_rep(t->root->bounding_curve);
        cost = -1.0;
        costf = -1.0;
        for (i = 0; i < rep->len; i++) {
                p = (struct cse788_corner_point_t *)rep->ary[i];
                a = p->x * p->y;
                if (cost < 0.0 || a < cost) {
                        cost = a;
                        idx = i;
                }
                if (p->y > p->x * t->q || p->y < p->x * t->p)
                        continue;
                if (costf < 0.0 || a < costf) {
                        costf = a;
                        idxf = i;
                }
                retval = PIE_TRUE;
        }
        if (retval) {
                *pcost = costf;
                *pidx = idxf;
        } else {
                *pcost = cost;
                *pidx = idx;
        }
        
        return retval;
}

static void 
cse788_floorplan_clean_bc(struct cse788_floorplan_t *t, 
                          struct cse788_floorplan_node_t *node)
{
        struct cse788_floorplan_node_t *c1, *c2;

        if (node->index >= 0)
                return;
        c1 = node->child[0];
        c2 = node->child[1];
        PieList_clean(node->bounding_curve);
        cse788_floorplan_node_dealloc(t, node);
        cse788_floorplan_clean_bc(t, c2);
        cse788_floorplan_clean_bc(t, c1);
}

static void
cse788_floorplan_recur_plot(struct cse788_floorplan_node_t *node, int best_idx,
                            double basex, double basey, double w, double h,
                            const struct cse788_floorplan_report_i *rp)
{
        double mx, my;
        struct cse788_corner_point_t *p;
        p = (struct cse788_corner_point_t *)PieList_get(
                        node->bounding_curve, best_idx);
        mx = basex + (w - p->x) / 2;
        my = basey + (h - p->y) / 2;

        if (node->index < 0) {
                struct cse788_corner_point_t *p1, *p2;
                double x1, y1, w1, h1;
                double x2, y2, w2, h2;
                rp->plotSlice(rp->inst, mx, my, p->x, p->y);
                p1 = (struct cse788_corner_point_t *)PieList_get(
                                node->child[0]->bounding_curve, 
                                p->child_idx[0]);
                p2 = (struct cse788_corner_point_t *)PieList_get(
                                node->child[1]->bounding_curve, 
                                p->child_idx[1]);
                x1 = mx;
                y1 = my;
                if (node->index == POLISH_V) {
                        w1 = p->x;
                        h1 = (p1->y / (p1->y + p2->y)) * p->y;
                        x2 = mx;
                        y2 = my + h1;
                        w2 = p->x;
                        h2 = p->y - h1;
                } else {
                        w1 = (p1->x / (p1->x + p2->x)) * p->x;
                        h1 = p->y;
                        x2 = mx + w1;
                        y2 = my;
                        w2 = p->x - w1;
                        h2 = p->y;
                }
                cse788_floorplan_recur_plot(node->child[0], p->child_idx[0],
                                x1, y1, w1, h1, rp);
                cse788_floorplan_recur_plot(node->child[1], p->child_idx[1],
                                x2, y2, w2, h2, rp);
        } else {
                rp->plotModule(rp->inst, mx, my, p->x, p->y, node->index);
                node->x = mx;
                node->y = my;
                node->w = p->x;
                node->h = p->y;
        }
}

static void 
cse788_floorplan_plot(struct cse788_floorplan_t *t, int best_idx,
                      const struct cse788_floorplan_report_i *rp)
{
        double c;
        struct cse788_corner_point_t *p;
        PIE_ASSERT(rp);
        p = (struct cse788_corner_point_t *)PieList_get(
                        t->root->bounding_curve, best_idx);
        c = p->x > p->y ? p->x : p->y;
        rp->plotLimit(rp->inst, c, c);
        cse788_floorplan_recur_plot(t->root, best_idx, 0, 0, c, c, rp);
        rp->present(rp->inst);
}

static void cse788_floorplan_print_results(struct cse788_floorplan_t *t)
{
        int i;
        const struct PieListRep *nodes;
        struct cse788_floorplan_node_t *node;

        nodes = PieList_rep(t->list_nodes);
        for (i = 0; i < nodes->len; i++) {
                node = (struct cse788_floorplan_node_t *)nodes->ary[i];
                printf("node: %d, x: %.2lf, y: %.2lf, w: %.2lf, h: %.2lf\n",
                                node->index, node->x, node->y, 
                                node->w, node->h);
        }
}

double 
cse788_floorplan_annealing(struct cse788_floorplan_t *t, double temp0, 
                           double temp_e, double temp_r, double accept_e, 
                           int m, const struct cse788_floorplan_report_i *rp)
{
        PieBool accept, fit1, fit2;
        double accept_rate, temp, cost1, cost2, pp;
        int num_accept, num_dmove, num_move, best_idx, num_umove, num_uattemp;
        int *polish, *polish_d, i, d;

        PIE_ASSERT(t);
        PIE_ASSERT(temp0 > 0.0);
        PIE_ASSERT(temp_e > 0.0 && temp_e < temp0);
        PIE_ASSERT(temp_r > 0.0 && temp_r < 1.0);
        PIE_ASSERT(accept_e > 0.0 && accept_e < 1.0);
        PIE_ASSERT(m > 2);

        /* initiate naive polish layout */
        t->polish[0] = 0;
        for (i = 1; i < PieList_length(t->list_nodes); i++) {
                t->polish[2 * i - 1] = i;
                t->polish[2 * i] = POLISH_V;
        }
        t->len_polish = 2 * PieList_length(t->list_nodes)- 1;
        for (i = 0, d = 0; i < t->len_polish; i++) {
                if (t->polish[i] < 0)
                        d++;
                t->polish_d[i] = d;
        }
        cse788_floorplan_parsepolish(t, t->polish);
        cse788_floorplan_recur_bc(t->root);

        accept_rate = 1.0;
        temp = temp0;
        polish = (int *)PIE_MALLOC(sizeof *polish * t->len_polish);
        polish_d = (int *)PIE_MALLOC(sizeof *polish_d * t->len_polish);
        fit1 = cse788_floorplan_cost(t, &cost1, &best_idx);

        if (t->show_progress) {
                printf("temp: %lf, cost: %lf, ", temp, cost1);
                cse788_print_polish(t->list_nodes, t->len_polish, t->polish);
        }

        while (accept_rate > accept_e && temp > temp_e) {
                num_accept = 0;
                num_dmove = 0;
                num_move = 0;
                num_umove = 0;
                num_uattemp = 0;

                while (num_move < m && num_dmove < m / 2) {
                        for (i = 0; i < t->len_polish; i++) {
                                polish[i] = t->polish[i];
                                polish_d[i] = t->polish_d[i];
                        }
                        cse788_floorplan_random_move(t, polish, polish_d);
                        cse788_floorplan_clean_bc(t, t->root);
                        cse788_floorplan_parsepolish(t, polish);
                        cse788_floorplan_recur_bc(t->root);
                        fit2 = cse788_floorplan_cost(t, &cost2, &best_idx);

                        if (fit1 && !fit2) 
                                cost2 = cost1 + NON_FIT_DCOST;
                        /* else if (!fit1 && fit2) */
                                /* cost2 = cost1 - 1.0; */
                        pp = exp(-(cost2 - cost1) / temp);
                        if (cost2 > cost1)
                                num_uattemp++;
                        if (cost2 < cost1) {
                                accept = PIE_TRUE;
                                num_dmove++;
                        } else if (PieRandom_bin(t->seed, pp)) {
                                accept = PIE_TRUE;
                                if (cost2 != cost1)
                                        num_umove++;
                        } else
                                accept = PIE_FALSE;
                        if (accept) {
                                num_accept++;
                                cost1 = cost2;
                                fit1 = fit2;
                                for (i = 0; i < t->len_polish; i++) {
                                        t->polish[i] = polish[i];
                                        t->polish_d[i] = polish_d[i];
                                }
                        }
                        num_move++;
                }
                accept_rate = 1.0 * num_accept / num_move;
                temp = temp * temp_r;

                /* output the result */
                if (t->show_progress) {
                        printf("temp: %lf, cost: %lf, dm: %d, um: %d, am: %d, "
                                        "m: %d, ur: %lf\n", temp, cost1, 
                                        num_dmove, num_umove, num_accept, 
                                        num_move, 
                                        1.0 * num_umove / num_uattemp);
                        /* cse788_print_polish(t->list_nodes, t->len_polish,  */
                                        /* t->polish); */
                        /* cse788_floorplan_clean_bc(t, t->root); */
                        /* cse788_floorplan_parsepolish(t, t->polish); */
                        /* cse788_floorplan_recur_bc(t->root); */
                        /* cse788_floorplan_cost(t, &cost1, &best_idx); */
                        /* cse788_floorplan_plot(t, best_idx, rp); */
                }
        }

        cse788_floorplan_clean_bc(t, t->root);
        cse788_floorplan_parsepolish(t, t->polish);
        cse788_floorplan_recur_bc(t->root);
        cse788_floorplan_cost(t, &cost1, &best_idx);
        cse788_floorplan_plot(t, best_idx, rp);
        cse788_print_polish(t->list_nodes, t->len_polish, t->polish);
        cse788_floorplan_print_results(t);
        do {
                struct cse788_corner_point_t *bp;
                bp = (struct cse788_corner_point_t *)PieList_get(
                                t->root->bounding_curve, best_idx);
                printf("area: %lf = %.2lf * %.2lf\n", cost1, bp->x, bp->y);
        } while (0);

        PIE_FREE(polish);
        PIE_FREE(polish_d);

        return cost1;
}

struct cse788_floorplan_t *cse788_floorplan_new(const char *path)
{
        struct cse788_floorplan_t *t;
        struct PieFile *file;

        PIE_ASSERT(path);

        t = PIE_NEW(struct cse788_floorplan_t);
        t->list_nodes = PieList_new(sizeof(struct cse788_floorplan_node_t));
        t->seed = PieRandom_new(time(NULL));

        file = PieFile_open(path, "r");
        cse788_floorplan_import(t, file);
        PieFile_close(&file);

        t->stack = PieList_new(0);
        t->free_nodes = PIE_NULL;
        t->show_progress = PIE_TRUE;

        return t;
}

void cse788_floorplan_del(struct cse788_floorplan_t **_t)
{
        struct cse788_floorplan_t *t;

        PIE_ASSERT(_t);
        t = *_t;
        if (t) {
                int i;
                struct cse788_floorplan_node_t *n, *n1;

                cse788_floorplan_clean_bc(t, t->root);
                n = t->free_nodes;
                while (n) {
                        PieList_del(&(n->bounding_curve));
                        n1 = n;
                        n = n->child[0];
                        PIE_FREE(n1);
                }
                for (i = 0; i < PieList_length(t->list_nodes); i++) {
                        n = (struct cse788_floorplan_node_t *)PieList_get(
                                        t->list_nodes, i);
                        PieList_del(&(n->bounding_curve));
                }
                PieList_del(&(t->list_nodes));
                PIE_FREE(t->polish);
                PIE_FREE(t->polish_d);
                PieList_del(&(t->stack));
        }
}

void 
cse788_floorplan_show_progress(struct cse788_floorplan_t *t, PieBool a)
{
        PIE_ASSERT(t);
        t->show_progress = a;
}
