
#include "cse788_gordian.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_blas.h>

#include <SDL2/SDL_timer.h>

#define MIN_CUT
#define BUF_LEN 1024
#define BUF_EXPAND 1024
#define EXP_COE 2
#define CHECK_ERROR(E) \
        if (E) { \
                ret = __LINE__; \
                break; \
        }

#define FAR 0.5
#define IMB 0.1

typedef struct cse788_lnode_t {
        void *item;
        double pinx, piny;
        struct cse788_lnode_t *link;
} cse788_lnode_t;

typedef struct cse788_cell_t {
        int idx, midx;
        int num_nets;
        int movable;
        double dimx, dimy;
        double x, y;
        struct cse788_lnode_t *nets;

        int gain, lock;
        struct cse788_partition_t *F, *T;
} cse788_cell_t;

typedef struct cse788_net_t {
        int idx;
        int num_mcells, num_fcells;
        double weight;
        double x, y;
        struct cse788_lnode_t *cells;
} cse788_net_t;

typedef struct cse788_partition_t {
        double x, y, w, h;
        int num_mcells, num_fcells;
        struct cse788_cell_t **list_mcells, **list_fcells;
        struct cse788_partition_t *sibling;

        double total_size, ototal_size;
        int o;
} cse788_partition_t;

typedef struct cse788_pos_t {
        double x, y;
} cse788_pos_t;

typedef struct cse788_partition_head_t {
        struct cse788_partition_t part;
        struct cse788_partition_head_t *child;
        struct cse788_pos_t *pos_mfcells;
} cse788_partition_head_t;

typedef struct cse788_gordian_t {
        double w, h;
        int num_cells, num_mcells, num_fcells, num_nets;

        struct cse788_net_t *nets;
        struct cse788_cell_t *cells;
        struct cse788_net_t **list_nets;
        struct cse788_cell_t **list_mcells, **list_fcells;

        gsl_matrix *C, *dx, *dy;
        struct cse788_partition_head_t *root_part;
        int *order_mcells;

} cse788_gordian_t;

/* static gsl_matrix  */
gsl_matrix cse788_submatrix(gsl_matrix *m, int k1, int k2, int n1, int n2)
{
        gsl_matrix ret;
        ret.data = m->data + k1 * m->tda + k2;
        ret.size1 = n1;
        ret.size2 = n2;
        ret.tda = m->tda;
        ret.block = m->block;
        ret.owner = 0;
        return ret;
}

static int cse788_gordian_getnum(FILE *input, char *buf, int *out)
{
        char *str;
        int ret, i;

        ret = 0;
        do {
                str = fgets(buf, BUF_LEN, input);
                CHECK_ERROR(str == NULL);
                str = strchr(str, '=');
                CHECK_ERROR(str == NULL);
                i = sscanf(str + 1, "%d", out);
                CHECK_ERROR(i != 1);
        } while (0);

        return ret;
}

static int cse788_gordian_getflt(FILE *input, char *buf, double *out)
{
        char *str;
        int ret, i;

        ret = 0;
        do {
                str = fgets(buf, BUF_LEN, input);
                CHECK_ERROR(str == NULL);
                str = strchr(str, '=');
                CHECK_ERROR(str == NULL);
                i = sscanf(str + 1, "%lf", out);
                CHECK_ERROR(i != 1);
        } while (0);

        return ret;
}

static int 
cse788_gordian_getmodules(struct cse788_gordian_t *t, FILE *input, char *buf)
{
        int ret, i, a, b, cnt_nets, cnt_mcells, cnt_fcells;
        int read;
        char *str = "";
        struct cse788_lnode_t *ln;

        ret = 0;
        cnt_nets = 0;
        cnt_mcells = 0;
        cnt_fcells = 0;
        read = 1;
        while (1) {
                struct cse788_net_t *net;

                if (read == 1 && !(str = fgets(buf, BUF_LEN, input)))
                        break;
                read = 1;

                /* read a net */
                CHECK_ERROR(str[0] != 'N');
                str = strchr(str, '(');
                CHECK_ERROR(str == NULL);
                i = sscanf(str + 1, "%d", &a);
                CHECK_ERROR(i != 1 || a < 0 || a >= t->num_nets * EXP_COE);
                net = t->nets + a;
                if (net->cells == NULL)
                        t->list_nets[cnt_nets++] = net;
                net->idx = a;
                net->weight = 1.0;
                net->x = -1.0;
                net->y = -1.0;

                /* read the cells */
                while (NULL != (str = fgets(buf, BUF_LEN, input))) {
                        struct cse788_cell_t *cell;
                        double pinx, piny;

                        if (str[0] == 'N') {
                                read = 0;
                                break;
                        }
                        CHECK_ERROR(str[0] != 'C');
                        str = strchr(str, '(');
                        CHECK_ERROR(str == NULL);
                        i = sscanf(str + 1, "%d", &b);
                        CHECK_ERROR(i != 1 || b < 0 
                                        || b >= t->num_cells * EXP_COE);
                        cell = t->cells + b;
                        cell->idx = b;
                        cell->num_nets++;
                        str = strchr(str + 1, '(');
                        CHECK_ERROR(str == NULL);
                        i = sscanf(str + 1, "%lf,%lf", &(cell->dimx), 
                                        &(cell->dimy));
                        CHECK_ERROR(i != 2);
                        str = strchr(str + 1, '(');
                        CHECK_ERROR(str == NULL);
                        i = sscanf(str + 1, "%lf,%lf", &pinx, &piny);
                        CHECK_ERROR(i != 2);
                        str = strchr(str + 1, ')');
                        CHECK_ERROR(str == NULL);
                        if (str[1] == 'F') {
                                net->num_fcells++;
                                if (NULL == cell->nets) {
                                        cell->midx = -1;
                                        t->list_fcells[cnt_fcells++] = cell;
                                        str = strchr(str + 1, '(');
                                        CHECK_ERROR(str == NULL);
                                        i = sscanf(str + 1, "%lf,%lf", 
                                                        &(cell->x), &(cell->y));
                                        CHECK_ERROR(i != 2);
                                        cell->movable = 0;
                                }
                        } else {
                                net->num_mcells++;
                                if (NULL == cell->nets) {
                                        cell->midx = cnt_mcells;
                                        t->list_mcells[cnt_mcells++] = cell;
                                        cell->x = -1.0;
                                        cell->y = -1.0;
                                        cell->movable = 1;
                                }
                        }
                
                        ln = (struct cse788_lnode_t *)malloc(sizeof *ln);
                        CHECK_ERROR(ln == NULL);
                        ln->pinx = pinx;
                        ln->piny = piny;
                        ln->item = cell;
                        ln->link = net->cells;
                        net->cells = ln;

                        ln = (struct cse788_lnode_t *)malloc(sizeof *ln);
                        CHECK_ERROR(ln == NULL);
                        ln->pinx = pinx;
                        ln->piny = piny;
                        ln->item = net;
                        ln->link = cell->nets;
                        cell->nets = ln;
                }
                if (0 != ret)
                        break;
        }
        do {
                CHECK_ERROR(cnt_nets != t->num_nets);
                CHECK_ERROR(cnt_fcells != t->num_fcells);
                CHECK_ERROR(cnt_mcells != t->num_mcells);
        } while (0);

        return ret;
}

static int cse788_gordian_setobjfunc(struct cse788_gordian_t *t)
{
        gsl_matrix *C, *dx, *dy;
        int i;

        C = t->C;
        dx = t->dx;
        dy = t->dy;

        gsl_matrix_set_zero(C);
        gsl_matrix_set_zero(dx);
        gsl_matrix_set_zero(dy);

        for (i = 0; i < t->num_nets; i++) {
                struct cse788_net_t *v;
                struct cse788_lnode_t *node;
                double e, card_v;

                v = t->list_nets[i];
                card_v = (double)(v->num_fcells + v->num_mcells);
                e = v->weight * 2.0 / card_v;
                node = v->cells;
                while (node) {
                        double *cuu, *dxu, *dyu;
                        struct cse788_lnode_t *node1;
                        double pinx, piny;
                        struct cse788_cell_t *u = (struct cse788_cell_t *)
                                node->item;

                        pinx = node->pinx;
                        piny = node->piny;
                        node = node->link;
                        if (u->movable == 0)
                                continue;
                        cuu = gsl_matrix_ptr(C, u->midx, u->midx);
                        *cuu += e * (card_v - 1.0);
                        dxu = gsl_matrix_ptr(dx, u->midx, 0);
                        *dxu += pinx * e * (card_v - 1.0);
                        dyu = gsl_matrix_ptr(dy, u->midx, 0);
                        *dyu += piny * e * (card_v - 1.0);
                        node1 = v->cells;
                        while (node1) {
                                double pinx1, piny1;
                                struct cse788_cell_t *l = 
                                        (struct cse788_cell_t *)node1->item;

                                pinx1 = node1->pinx;
                                piny1 = node1->piny;
                                node1 = node1->link;
                                if (l == u)
                                        continue;
                                if (l->movable == 1) {
                                        double *cul;
                                        cul = gsl_matrix_ptr(C, u->midx, 
                                                        l->midx);
                                        *cul -= e;
                                        *dxu -= pinx1 * e;
                                        *dyu -= piny1 * e;
                                } else {
                                        *dxu -= e * (l->x + pinx1);
                                        *dyu -= e * (l->y + piny1);
                                }
                        }
                }
        }

        return 0;
}

static int 
cse788_gordian_optimize(struct cse788_gordian_t *t, 
                        struct cse788_partition_head_t *head)
{
        int ret = 0;

        do {
                int q, j, cb;
                /* int signum; */
                gsl_matrix *DB, *Di, *Z, *Ux, *Uy, *X0, *Y0, *Cm, *dxm, *dym;
                gsl_matrix *ccx, *ccy, *tmp1, *ZCZ, *ZCZi, *Xi, *Yi;
                gsl_matrix D, B, mm, Xd, Yd;
                /* gsl_permutation *pmu, *pmu1; */
                struct cse788_partition_t *p, *first;

                /* Calculate DB and Ux Uy*/
                first = &(head->part);
                for (q = 0, p = first; p; p = p->sibling, q++);
                DB = gsl_matrix_alloc(q, t->num_mcells);
                CHECK_ERROR(DB == NULL);
                gsl_matrix_set_zero(DB);
                Ux = gsl_matrix_alloc(q, 1);
                CHECK_ERROR(Ux == NULL);
                Uy = gsl_matrix_alloc(q, 1);
                CHECK_ERROR(Uy == NULL);
                for (j = 0, p = first, cb = 0; p; p = p->sibling, j++) {
                        int i, max_midx = -1;
                        double total, max;

                        for (i = 0, total = 0.0; i < p->num_mcells; i++) {
                                struct cse788_cell_t *cell = p->list_mcells[i];
                                total += cell->dimx * cell->dimy;
                        }
                        for (i = 0, max = -1.0; i < p->num_mcells; i++) {
                                int setm_midx;
                                double setm;
                                struct cse788_cell_t *cell = p->list_mcells[i];
                                double a = cell->dimx * cell->dimy / total;

                                if (a > max) {
                                        setm = max;
                                        setm_midx = max_midx;
                                        max = a;
                                        max_midx = cell->midx;
                                } else {
                                        setm = a;
                                        setm_midx = cell->midx;
                                }
                                if (setm > 0.0) {
                                        gsl_matrix_set(DB, j, cb + q, setm);
                                        t->order_mcells[cb + q] = setm_midx;
                                        cb++;
                                }
                        }
                        gsl_matrix_set(DB, j, j, max);
                        t->order_mcells[j] = max_midx;

                        gsl_matrix_set(Ux, j, 0, p->x + p->w / 2.0);
                        gsl_matrix_set(Uy, j, 0, p->y + p->h / 2.0);
                }

                /* debug */
                /* gsl_matrix_fprintf(stdout, DB, "%f"); */
                /* gsl_matrix_fprintf(stdout, Ux, "%f"); */
                /* gsl_matrix_fprintf(stdout, Uy, "%f"); */

                /* Calculate D^-1 in place */
                D = cse788_submatrix(DB, 0, 0, q, q);
                /* pmu = gsl_permutation_alloc(q); */
                /* CHECK_ERROR(pmu == NULL); */
                /* ret = gsl_linalg_LU_decomp(&D, pmu, &signum); */
                Di = gsl_matrix_alloc(q, q);
                CHECK_ERROR(Di == NULL);
                gsl_matrix_memcpy(Di, &D);
                ret = gsl_linalg_cholesky_decomp(Di);
                CHECK_ERROR(ret != 0);
                /* Di = gsl_matrix_alloc(q, q); */
                /* CHECK_ERROR(Di == NULL); */
                /* ret = gsl_linalg_LU_invert(&D, pmu, Di); */
                ret = gsl_linalg_cholesky_invert(Di);
                CHECK_ERROR(ret != 0);

                /* debug */
                /* gsl_matrix_fprintf(stdout, Di, "%f"); */

                /* Calculate -D^-1 * B */
                B = cse788_submatrix(DB, 0, q, q, t->num_mcells - q);
                Z = gsl_matrix_alloc(t->num_mcells, t->num_mcells - q);
                CHECK_ERROR(Z == NULL);

                mm = cse788_submatrix(Z, 0, 0, q, t->num_mcells - q);
                gsl_matrix_set_zero(&mm);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
                                -1.0, Di, &B, 0.0, &mm);
                CHECK_ERROR(ret != 0);

                /* Append identity to form Z */
                mm = cse788_submatrix(Z, q, 0, t->num_mcells - q, 
                                t->num_mcells - q);
                gsl_matrix_set_identity(&mm);

                /* debug */
                /* gsl_matrix_fprintf(stdout, Z, "%f"); */

                /* Calculate X0 and Y0 */
                X0 = gsl_matrix_alloc(t->num_mcells, 1);
                CHECK_ERROR(X0 == NULL);
                gsl_matrix_set_zero(X0);
                mm = cse788_submatrix(X0, 0, 0, q, 1);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
                                1.0, Di, Ux, 0.0, &mm);
                CHECK_ERROR(ret != 0);

                Y0 = gsl_matrix_alloc(t->num_mcells, 1);
                CHECK_ERROR(Y0 == NULL);
                gsl_matrix_set_zero(Y0);
                mm = cse788_submatrix(Y0, 0, 0, q, 1);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
                                1.0, Di, Uy, 0.0, &mm);
                CHECK_ERROR(ret != 0);

                /* debug - print X0 Y0 */
                /* gsl_matrix_fprintf(stdout, X0, "%f"); */
                /* gsl_matrix_fprintf(stdout, Y0, "%f"); */

                /* Calculate permuted C dx and dy */
                Cm = gsl_matrix_alloc(t->num_mcells, t->num_mcells);
                CHECK_ERROR(Cm == NULL);
                for (j = 0; j < t->num_mcells; j++) {
                        int i, oj;
                        oj = t->order_mcells[j];
                        for (i = 0; i < t->num_mcells; i++) {
                                gsl_matrix_set(Cm, j, i, gsl_matrix_get(
                                                t->C, oj, t->order_mcells[i]));
                        }
                }
                dxm = gsl_matrix_alloc(t->num_mcells, 1);
                dym = gsl_matrix_alloc(t->num_mcells, 1);
                CHECK_ERROR(dxm == NULL || dym == NULL);
                for (j = 0; j < t->num_mcells; j++) {
                        gsl_matrix_set(dxm, j, 0, gsl_matrix_get(
                                        t->dx, t->order_mcells[j], 0));
                        gsl_matrix_set(dym, j, 0, gsl_matrix_get(
                                        t->dy, t->order_mcells[j], 0));
                }

                /* debug */
                /* gsl_matrix_fprintf(stdout, Cm, "%f"); */
                /* gsl_matrix_fprintf(stdout, dxm, "%f"); */
                /* gsl_matrix_fprintf(stdout, dym, "%f"); */

                /* Calculate cc */
                ccx = gsl_matrix_alloc(1, t->num_mcells - q);
                gsl_matrix_set_zero(ccx);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
                                1.0, Cm, X0, 1.0, dxm);
                CHECK_ERROR(ret != 0);
                ret = gsl_blas_dgemm(CblasTrans, CblasNoTrans,
                                1.0, dxm, Z, 0.0, ccx);
                CHECK_ERROR(ret != 0);

                ccy = gsl_matrix_alloc(1, t->num_mcells - q);
                gsl_matrix_set_zero(ccy);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
                                1.0, Cm, Y0, 1.0, dym);
                CHECK_ERROR(ret != 0);
                ret = gsl_blas_dgemm(CblasTrans, CblasNoTrans,
                                1.0, dym, Z, 0.0, ccy);
                CHECK_ERROR(ret != 0);

                /* debug */
                /* gsl_matrix_fprintf(stdout, ccx, "%f"); */
                /* gsl_matrix_fprintf(stdout, ccy, "%f"); */

                /* Calculate ZtCZ */
                tmp1 = gsl_matrix_alloc(t->num_mcells - q, t->num_mcells);
                CHECK_ERROR(tmp1 == NULL);
                gsl_matrix_set_zero(tmp1);
                ret = gsl_blas_dgemm(CblasTrans, CblasNoTrans,
                                1.0, Z, Cm, 0.0, tmp1);
                CHECK_ERROR(ret != 0);
                ZCZ = gsl_matrix_alloc(t->num_mcells - q, t->num_mcells - q);
                CHECK_ERROR(ZCZ == NULL);
                gsl_matrix_set_zero(ZCZ);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
                                1.0, tmp1, Z, 0.0, ZCZ);
                CHECK_ERROR(ret != 0);

                /* debug */
                /* gsl_matrix_fprintf(stdout, ZCZ, "%f"); */

                /* Calculate (ZtCZ)^-1 */
                /* pmu1 = gsl_permutation_alloc(t->num_mcells - q); */
                /* CHECK_ERROR(pmu1 == NULL); */
                /* ret = gsl_linalg_LU_decomp(ZCZ, pmu1, &signum); */
                ZCZi = gsl_matrix_alloc(t->num_mcells - q, t->num_mcells - q);
                CHECK_ERROR(ZCZi == NULL);
                gsl_matrix_memcpy(ZCZi, ZCZ);
                ret = gsl_linalg_cholesky_decomp(ZCZi);
                CHECK_ERROR(ret != 0);
                /* ZCZi = gsl_matrix_alloc(t->num_mcells - q, t->num_mcells - q); */
                /* CHECK_ERROR(ZCZi == NULL); */
                /* gsl_matrix_set_zero(ZCZi); */
                /* ret = gsl_linalg_LU_invert(ZCZ, pmu1, ZCZi); */
                ret = gsl_linalg_cholesky_invert(ZCZi);
                CHECK_ERROR(ret != 0);

                /* debug */
                /* gsl_matrix_fprintf(stdout, ZCZi, "%f"); */

                /* Calculate Xi Yi */
                Xi = gsl_matrix_alloc(t->num_mcells - q, 1);
                Yi = gsl_matrix_alloc(t->num_mcells - q, 1);
                CHECK_ERROR(Xi == NULL || Yi == NULL);
                gsl_matrix_set_zero(Xi);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasTrans,
                                -1.0, ZCZi, ccx, 0.0, Xi);
                CHECK_ERROR(ret != 0);
                gsl_matrix_set_zero(Yi);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasTrans,
                                -1.0, ZCZi, ccy, 0.0, Yi);
                CHECK_ERROR(ret != 0);

                /* debug */
                /* gsl_matrix_fprintf(stdout, Xi, "%f"); */
                /* gsl_matrix_fprintf(stdout, Yi, "%f"); */

                /* Calculate Xd Yd */
                mm = cse788_submatrix(Z, 0, 0, q, t->num_mcells - q);
                Xd = cse788_submatrix(X0, 0, 0, q, 1);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
                                1.0, &mm, Xi, 1.0, &Xd);
                CHECK_ERROR(ret != 0);
                Yd = cse788_submatrix(Y0, 0, 0, q, 1);
                ret = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans,
                                1.0, &mm, Yi, 1.0, &Yd);
                CHECK_ERROR(ret != 0);

                /* debug */
                /* gsl_matrix_fprintf(stdout, &Xd, "%f"); */
                /* gsl_matrix_fprintf(stdout, &Yd, "%f"); */
                
                /* store the position of modules */
                for (j = 0; j < q; j++) {
                        struct cse788_pos_t ps;
                        ps.x = gsl_matrix_get(&Xd, j, 0);
                        ps.y = gsl_matrix_get(&Yd, j, 0);
                        head->pos_mfcells[t->order_mcells[j]] = ps;
                }
                for (j = q; j < t->num_mcells; j++) {
                        struct cse788_pos_t ps;
                        ps.x = gsl_matrix_get(Xi, j - q, 0);
                        ps.y = gsl_matrix_get(Yi, j - q, 0);
                        head->pos_mfcells[t->order_mcells[j]] = ps;
                }
                for (j = 0; j < t->num_fcells; j++) {
                        struct cse788_pos_t ps;
                        ps.x = t->list_fcells[j]->x;
                        ps.y = t->list_fcells[j]->y;
                        head->pos_mfcells[j + t->num_mcells] = ps;
                }

                /* release the matrices */
                gsl_matrix_free(DB);
                gsl_matrix_free(Di);
                gsl_matrix_free(Z);
                gsl_matrix_free(Ux);
                gsl_matrix_free(Uy);
                gsl_matrix_free(X0);
                gsl_matrix_free(Y0);
                gsl_matrix_free(Cm);
                gsl_matrix_free(dxm);
                gsl_matrix_free(dym);
                gsl_matrix_free(ccx);
                gsl_matrix_free(ccy);
                gsl_matrix_free(tmp1);
                gsl_matrix_free(ZCZ);
                gsl_matrix_free(ZCZi);
                gsl_matrix_free(Xi);
                gsl_matrix_free(Yi);
                /* gsl_permutation_free(pmu); */
                /* gsl_permutation_free(pmu1); */

        } while (0);

        return ret;
}

static int 
cse788_gordian_report(struct cse788_gordian_t *t, 
                      struct cse788_partition_head_t *head,
                      struct cse788_gordian_report_i *r)
{
        int ret = 0;

        do {
                int i;
                struct cse788_partition_t *p;

                /* Create new level */
                ret = r->inclevel(r->inst);
                CHECK_ERROR(ret != 0);

                /* Plot partition */
                p = &(head->part);
                while (p) {
                        ret = r->postpart(r->inst, t->w, t->h,
                                        p->x, p->y, p->w, p->h);
                        CHECK_ERROR(ret != 0);
                        p = p->sibling;
                }
                if (ret != 0)
                        break;

                /* Plot fixed cells */
                for (i = 0; i < t->num_fcells; i++) {
                        struct cse788_cell_t *cell;

                        cell = t->list_fcells[i];
                        ret = r->postcell(r->inst, cell->idx, cell->movable,
                                        t->w, t->h, cell->dimx, cell->dimy, 
                                        cell->x, cell->y);
                        CHECK_ERROR(ret != 0);
                }
                if (ret != 0)
                        break;

                /* Plot movable cells */
                for (i = 0; i < t->num_mcells; i++) {
                        struct cse788_cell_t *cell;

                        cell = t->list_mcells[i];
                        r->postcell(r->inst, cell->idx, cell->movable,
                                        t->w, t->h, cell->dimx, cell->dimy,
                                        head->pos_mfcells[i].x, 
                                        head->pos_mfcells[i].y);
                        CHECK_ERROR(head->pos_mfcells[i].x < -10);
                        CHECK_ERROR(ret != 0);
                }
                if (ret != 0)
                        break;

                /* Plot nets */
                for (i = 0; i < t->num_nets; i++) {
                        struct cse788_net_t *net;
                        struct cse788_lnode_t *node;

                        net = t->list_nets[i];
                        node = net->cells;
                        while (node) {
                                double x, y;
                                struct cse788_cell_t *cell;
                                cell = (struct cse788_cell_t *)(node->item);
                                if (cell->movable) {
                                        x = head->pos_mfcells[cell->midx].x;
                                        y = head->pos_mfcells[cell->midx].y;
                                } else {
                                        x = cell->x;
                                        y = cell->y;
                                }
                                x += node->pinx;
                                y += node->piny;
                                ret = r->postline(r->inst, t->w, t->h,
                                                x, y, net->x, net->y);
                                CHECK_ERROR(ret != 0);
                                node = node->link;
                        }
                        if (ret != 0)
                                break;
                        /* ret = r->postpart(r->inst, t->w, t->h, */
                                        /* net->x, net->y, 0.1, 0.1); */
                        /* CHECK_ERROR(ret != 0); */
                }
                if (ret != 0)
                        break;

                /* Make changes */
                ret = r->present(r->inst);
                CHECK_ERROR(ret);
        } while (0);

        return ret;
}
static void _calc_gain(struct cse788_gordian_t *t, struct cse788_cell_t *bc) {
        int i;
        for (i = 0; i < t->num_mcells; i++) { 
                struct cse788_partition_t *F, *T; 
                struct cse788_cell_t *cell; 
                struct cse788_lnode_t *node; 
                cell = t->list_mcells[i]; 
                if (cell->lock)
                        continue;
                cell->gain = 0; 
                F = cell->F; 
                T = cell->T; 
                node = cell->nets; 
                while (node) { 
                        struct cse788_net_t *net; 
                        struct cse788_lnode_t *node1;
                        int fn = 0, tn = 0; 
                        net = (struct cse788_net_t *)(node->item);
                        node1 = net->cells; 
                        while (node1) { 
                                struct cse788_cell_t *cell1;
                                cell1 = (struct cse788_cell_t *)
                                                (node1->item); 
                                if (cell1->F == F) 
                                        fn++; 
                                else if (cell1->F == T)
                                        tn++; 
                                node1 = node1->link;
                        } 
                        if (fn == 1) 
                                cell->gain++;
                        if (tn == 0) 
                                cell->gain--;
                        node = node->link; 
                } 
        } 
}

static struct cse788_cell_t *_find_max_gain(struct cse788_gordian_t *t) 
{
        int i;
        struct cse788_cell_t *max = NULL;
        for (i = 0; i < t->num_mcells; i++) {
                struct cse788_cell_t *cell; 
                cell = t->list_mcells[i];
                if (cell->lock == 0 && (max == NULL || max->gain < cell->gain))
                        max = cell;
        }
        return max;
}


static int 
cse788_gordian_divide(struct cse788_gordian_t *t, 
                      struct cse788_partition_head_t *head, int dir)
{
        int *sort, i, ret = 0;
        struct cse788_pos_t *pos;
        struct cse788_partition_head_t *h;
        struct cse788_partition_t *p, *pp1, *pp2;
        int stop = 1;

        h = (struct cse788_partition_head_t *)malloc(sizeof *h);
        head->child = h;
        h->child = NULL;
        h->pos_mfcells = (struct cse788_pos_t *)calloc(t->num_cells, 
                        sizeof *(h->pos_mfcells));
        pp1 = &(h->part);
        pp2 = NULL;
        pos = head->pos_mfcells;
        sort = (int *)calloc(t->num_cells, sizeof *sort);
        p = &(head->part);
        while (p) {
                int n;
                double thresh, acc, divide, total;

                if (p->w > p->h)
                        dir = 0;
                else
                        dir = 1;

                if (p->num_mcells < 2) {
                        if (pp1 == NULL) {
                                pp1 = (struct cse788_partition_t *)malloc(
                                                sizeof *pp1);
                                CHECK_ERROR(pp1 == NULL);
                        }
                        *pp1 = *p;
                        /* pp1->list_mcells = (struct cse788_cell_t **)malloc( */
                                        /* sizeof *(pp1->list_mcells)); */
                        /* pp1->list_mcells[0] = p->list_mcells[0]; */
                        pp1->o = 0;
                        pp1->list_mcells[0]->lock = 1;
                        pp1->list_mcells[0]->T = NULL;
                        if (pp2 != NULL)
                                pp2->sibling = pp1;
                        pp2 = pp1;
                        pp1 = NULL;
                        pp2->sibling = NULL;
                        p = p->sibling;
                        continue;
                }

                total = 0.0;
                for (i = 0; i < p->num_mcells; i++) {
                        struct cse788_cell_t *cell;
                        cell = p->list_mcells[i];
                        sort[i] = cell->midx;
                        total += cell->dimx * cell->dimy;
                }

                /* sort the mcells */
                n = p->num_mcells;
                do {
                        int newn = 0;
                        for (i = 1; i < n; i++) {
                                if ((dir == 0 && 
                                     pos[sort[i - 1]].x > pos[sort[i]].x) ||
                                    (dir == 1 && 
                                     pos[sort[i - 1]].y > pos[sort[i]].y)) {
                                        int tmp = sort[i - 1];
                                        sort[i - 1] = sort[i];
                                        sort[i] = tmp;
                                        newn = i;
                                }
                        }
                        n = newn;
                } while (n != 0);

                /* divide */
                thresh = total * 0.5;
                acc = 0.0;
                divide = 0.0;
                for (i = 0; i < p->num_mcells; i++) {
                        struct cse788_cell_t *cell;
                        cell = t->list_mcells[sort[i]];
                        acc += cell->dimx * cell->dimy;
                        if (acc > thresh) {
                                struct cse788_pos_t p1, p2;
                                if (i == 0)
                                        i++;
                                p1 = pos[sort[i]];
                                p2 = pos[sort[i - 1]];
                                divide = (dir == 0) ? (p1.x + p2.x) / 2 : 
                                                (p1.y + p2.y) / 2;
                                n = i;
                                acc -= cell->dimx * cell->dimy;
                                break;
                        }
                }

                /* Allocate new area */
                if (pp1 == NULL) {
                        pp1 = (struct cse788_partition_t *)malloc(sizeof *pp1);
                        CHECK_ERROR(pp1 == NULL);
                }
                pp1->x = p->x;
                pp1->y = p->y;
                pp1->total_size = acc;
                pp1->ototal_size = acc;
                if (dir == 0) {
                        pp1->w = divide - p->x;
                        pp1->h = p->h;
                } else {
                        pp1->w = p->w;
                        pp1->h = divide - p->y;
                }
                pp1->num_mcells = n;
                pp1->list_mcells = (struct cse788_cell_t **)calloc(
                                p->num_mcells, sizeof *(pp1->list_mcells));
                pp1->o = 1;
                CHECK_ERROR(pp1->list_mcells == NULL);
                for (i = 0; i < n; i++) {
                        pp1->list_mcells[i] = t->list_mcells[sort[i]];
                        t->list_mcells[sort[i]]->F = pp1;
                        if ((1.0 * i) / (1.0 * n) < FAR)
                                t->list_mcells[sort[i]]->lock = 1;
                        else
                                t->list_mcells[sort[i]]->lock = 0;
                }

                if (pp2 != NULL)
                        pp2->sibling = pp1;
                pp2 = (struct cse788_partition_t *)malloc(sizeof *pp2);
                for (i = 0; i < n; i++)
                        t->list_mcells[sort[i]]->T = pp2;
                CHECK_ERROR(pp2 == NULL);
                pp1->sibling = pp2;
                pp2->total_size = total - acc;
                pp2->ototal_size = pp2->total_size;
                if (dir == 0) {
                        pp2->x = divide;
                        pp2->y = p->y;
                        pp2->w = p->x + p->w - divide;
                        pp2->h = p->h;
                } else {
                        pp2->x = p->x;
                        pp2->y = divide;
                        pp2->w = p->w;
                        pp2->h = p->y + p->h - divide;
                }
                pp2->num_mcells = p->num_mcells - n;
                /* printf("%d, %d, %lf\n", p->num_mcells, n, thresh); */
                pp2->list_mcells = (struct cse788_cell_t **)calloc(
                                p->num_mcells, sizeof *(pp2->list_mcells));
                pp1->o = 1;
                CHECK_ERROR(pp2->list_mcells == NULL);
                for (i = n; i < p->num_mcells; i++) {
                        pp2->list_mcells[i - n] = t->list_mcells[sort[i]];
                        t->list_mcells[sort[i]]->F = pp2;
                        t->list_mcells[sort[i]]->T = pp1;
                        if ((1.0 * (i - n)) / (1.0 * (p->num_mcells - n)) 
                                        >= (1.0 - FAR))
                                t->list_mcells[sort[i]]->lock = 1;
                        else
                                t->list_mcells[sort[i]]->lock = 0;
                }
                pp2->sibling = NULL;

                if (pp1->num_mcells > 1 || pp2->num_mcells > 1)
                        stop = 0;

                pp1 = NULL;
                p = p->sibling;
        }

        free(sort);
        if (stop == 1) {
                h->child = t->root_part;
                return ret;
        }

#ifdef MIN_CUT
        /* Naive min-cut */
        _calc_gain(t, NULL);
        while (1) {
                struct cse788_cell_t *bc;
                struct cse788_partition_t *F, *T;
                double dim, r;
                int i;

                bc = _find_max_gain(t);
                if (bc == NULL || bc->gain <= 0)
                        break;
                bc->lock = 1;
                F = bc->F;
                T = bc->T;
                dim = bc->dimx * bc->dimy;
                r = (F->ototal_size - F->total_size + dim) / F->ototal_size;
                if (r > IMB)
                        continue;
                r = (T->total_size - T->ototal_size + dim) / F->ototal_size;
                if (r > IMB)
                        continue;
                T->list_mcells[T->num_mcells++] = bc;
                for (i = 0; i < F->num_mcells; i++) {
                        if (F->list_mcells[i] == bc) {
                                F->list_mcells[i] = 
                                        F->list_mcells[F->num_mcells - 1];
                                break;
                        }
                }
                F->num_mcells--;
                T->total_size += dim;
                F->total_size -= dim;
                bc->F = T;
                bc->T = F;
                _calc_gain(t, bc);
        }
#endif

        return ret;
}

static double 
cse788_gordian_totallength(struct cse788_gordian_t *t, 
                           struct cse788_partition_head_t *h) 
{
        int i;
        double len;

        for (i = 0, len = 0.0; i < t->num_nets; i++) {
                struct cse788_lnode_t *node;
                struct cse788_net_t *net = t->list_nets[i];

                node = net->cells;
                while (node) {
                        double x, y;
                        struct cse788_cell_t *cell;

                        cell = (struct cse788_cell_t *)(node->item);
                        if (cell->movable) {
                                x = h->pos_mfcells[cell->midx].x;
                                y = h->pos_mfcells[cell->midx].y;
                        } else {
                                x = cell->x;
                                y = cell->y;
                        }
                        len += (x + node->pinx - net->x) * 
                               (x + node->pinx - net->x) +
                               (y + node->piny - net->y) *
                               (y + node->piny - net->y);
                node = node->link;
                }
        }

        return len;
}

static int 
cse788_gordian_findnetcenter(struct cse788_gordian_t *t, 
                             struct cse788_partition_head_t *h) 
{
        int i;

        for (i = 0; i < t->num_nets; i++) {
                double xx, yy;
                struct cse788_lnode_t *node;
                struct cse788_net_t *net = t->list_nets[i];

                node = net->cells;
                xx = 0.0;
                yy = 0.0;
                while (node) {
                        double x, y;
                        struct cse788_cell_t *cell;

                        cell = (struct cse788_cell_t *)(node->item);
                        if (cell->movable) {
                                x = h->pos_mfcells[cell->midx].x;
                                y = h->pos_mfcells[cell->midx].y;
                        } else {
                                x = cell->x;
                                y = cell->y;
                        }
                        x += node->pinx;
                        y += node->piny;
                        xx += x;
                        yy += y;
                        node = node->link;
                }
                xx /= (net->num_fcells + net->num_mcells);
                yy /= (net->num_fcells + net->num_mcells);
                net->x = xx;
                net->y = yy;
        }

        return 0;
}

static int 
cse788_gordian_lastassign(struct cse788_gordian_t *t, 
                          struct cse788_partition_head_t *h)
{
        struct cse788_partition_t *p;
        int ret = 0;

        p = &(h->part);
        while (p) {
                struct cse788_cell_t *cell;

                /* CHECK_ERROR(p->num_mcells > 1); */
                cell = p->list_mcells[0];
                h->pos_mfcells[cell->midx].x = p->x + p->w / 2.0;
                h->pos_mfcells[cell->midx].y = p->y + p->h / 2.0;
                p = p->sibling;
        }

        return ret;
}

struct cse788_gordian_t *
cse788_gordian_new(const char *file, struct cse788_gordian_report_i *r)
{
        FILE *input;
        struct cse788_gordian_t *t;
        int ret, i, dir;
        char buf[BUF_LEN];
        struct cse788_partition_head_t *head;

        do {
                t = (struct cse788_gordian_t *)malloc(sizeof *t);
                CHECK_ERROR(t == NULL);
                input = fopen(file, "r");
                CHECK_ERROR(file == NULL);

                /* get dimension info */
                ret = cse788_gordian_getnum(input, buf, &(t->num_cells));
                CHECK_ERROR(ret != 0);
                ret = cse788_gordian_getnum(input, buf, &(t->num_mcells));
                CHECK_ERROR(ret != 0);
                ret = cse788_gordian_getnum(input, buf, &(t->num_fcells));
                CHECK_ERROR(ret != 0);
                ret = cse788_gordian_getnum(input, buf, &(t->num_nets));
                CHECK_ERROR(ret != 0);
                ret = cse788_gordian_getflt(input, buf, &(t->w));
                CHECK_ERROR(ret != 0);
                ret = cse788_gordian_getflt(input, buf, &(t->h));
                CHECK_ERROR(ret != 0);

                /* get nets and cells */
                t->cells = (struct cse788_cell_t *)calloc(
                                t->num_cells * EXP_COE, sizeof *(t->cells));
                CHECK_ERROR(t->cells == NULL);
                t->nets = (struct cse788_net_t *)calloc(
                                t->num_nets * EXP_COE, sizeof *(t->nets));
                CHECK_ERROR(t->nets == NULL);
                t->list_nets = (struct cse788_net_t **)calloc(t->num_nets, 
                                sizeof *(t->list_nets));
                CHECK_ERROR(t->nets == NULL);
                t->list_mcells = (struct cse788_cell_t **)calloc(t->num_mcells,
                                sizeof *(t->list_mcells));
                CHECK_ERROR(t->list_mcells == NULL);
                t->list_fcells = (struct cse788_cell_t **)calloc(t->num_fcells,
                                sizeof *(t->list_fcells));
                CHECK_ERROR(t->list_fcells == NULL);
                ret = cse788_gordian_getmodules(t, input, buf);
                CHECK_ERROR(ret != 0);

                /* setup objective function */
                t->C = gsl_matrix_alloc(t->num_mcells, t->num_mcells);
                CHECK_ERROR(t->C == NULL);
                t->dx = gsl_matrix_alloc(t->num_mcells, 1);
                CHECK_ERROR(t->dx == NULL);
                t->dy = gsl_matrix_alloc(t->num_mcells, 1);
                CHECK_ERROR(t->dy == NULL);
                ret = cse788_gordian_setobjfunc(t);
                CHECK_ERROR(ret != 0);

                /* debug */
                /* gsl_matrix_fprintf(stdout, t->C, "%f"); */
                /* gsl_matrix_fprintf(stdout, t->dx, "%f"); */
                /* gsl_matrix_fprintf(stdout, t->dy, "%f"); */

                /* create root partition */
                t->root_part = (struct cse788_partition_head_t *)malloc(
                                sizeof *(t->root_part));
                CHECK_ERROR(t->root_part == NULL);
                t->order_mcells = (int *)calloc(t->num_mcells, 
                                sizeof *(t->order_mcells));
                t->root_part->part.x = 0;
                t->root_part->part.y = 0;
                t->root_part->part.w = t->w;
                t->root_part->part.h = t->h;
                t->root_part->part.sibling = NULL;
                t->root_part->part.num_mcells = t->num_mcells;
                t->root_part->part.list_mcells = t->list_mcells;
                t->root_part->part.num_fcells = t->num_fcells;
                t->root_part->part.list_fcells = t->list_fcells;
                t->root_part->child = NULL;
                t->root_part->pos_mfcells = (struct cse788_pos_t *)calloc(
                                t->num_cells, sizeof(struct cse788_pos_t));

                /* start partition iteration */
                head = t->root_part;
                for (i = 0, dir = 0; i < 1000; i++, dir ^= 1) {
                        int tick = SDL_GetTicks();

                        if (head->child == NULL)
                                ret = cse788_gordian_optimize(t, head);
                        else
                                ret = cse788_gordian_lastassign(t, head);
                        if (ret != 0)
                                break;
                        ret = cse788_gordian_findnetcenter(t, head);
                        if (ret != 0)
                                break;
                        ret = cse788_gordian_report(t, head, r);
                        if (ret != 0)
                                break;

                        printf("optimize level: %-2d; time consumed: %-5.2lfs; "
                               "total length: %-.2lf\n", i, 
                               ((double)(SDL_GetTicks() - tick) / 1000.0), 
                               cse788_gordian_totallength(t, head));

                        if (head->child == NULL) {
                                ret = cse788_gordian_divide(t, head, dir);
                                if (ret != 0)
                                        break;
                        } else 
                                break;

                        head = head->child;
                }
                if (ret != 0)
                        break;

        } while (0);

        if (ret != 0)
                printf("gordian error: %d\n", ret);

        return t;
}

void cse788_gordian_del(struct cse788_gordian_t *t)
{
        int i;
        struct cse788_partition_head_t *h, *tmp1;

        gsl_matrix_free(t->C);
        gsl_matrix_free(t->dx);
        gsl_matrix_free(t->dy);
        for (i = 0; i < t->num_nets; i++) {
                struct cse788_net_t *v = t->list_nets[i];
                struct cse788_lnode_t *n = v->cells, *tmp;
                while (n) {
                        tmp = n->link;
                        free(n);
                        n = tmp;
                }
        }
        for (i = 0; i < t->num_mcells; i++) {
                struct cse788_cell_t *v = t->list_mcells[i];
                struct cse788_lnode_t *n = v->nets, *tmp;
                while (n) {
                        tmp = n->link;
                        free(n);
                        n = tmp;
                }
        }
        for (i = 0; i < t->num_fcells; i++) {
                struct cse788_cell_t *v = t->list_fcells[i];
                struct cse788_lnode_t *n = v->nets, *tmp;
                while (n) {
                        tmp = n->link;
                        free(n);
                        n = tmp;
                }
        }
        free(t->list_fcells);
        free(t->list_mcells);
        free(t->list_nets);
        free(t->cells);
        free(t->nets);
        free(t->order_mcells);
        h = t->root_part;
        while (h->child != t->root_part) {
                struct cse788_partition_t *p, *tmp;
                tmp1 = h->child;
                free(h->pos_mfcells);
                p = &(h->part);
                while (p) {
                        tmp = p->sibling;
                        /* free(p->list_fcells); */
                        if (p->o == 1)
                                free(p->list_mcells);
                        free(p);
                        p = tmp;
                }
                h = tmp1;
        }
        free(t);
}
