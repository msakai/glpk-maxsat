/*
Copyright 2013 Masahiro Sakai. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
   3. The name of the author may not be used to endorse or promote
      products derived from this software without specific prior
      written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <glpk.h>
#include <vector>

static
int read_wcnf(glp_prob *prob, const char *filename)
{
    FILE *file = fopen(filename, "r");
    char line[1024*128];
    int nv, nc;
    int64_t top = -1;
    bool isWCNF = 0;

    while (1) {
        fgets(line, sizeof(line), file);
        if (line[0] == 'c')
            continue;
        else if (line[0] == 'p') {
            int ret = sscanf(line, "p cnf %d %d", &nv, &nc);
            if (ret == 2) goto BODY;

            ret = sscanf(line, "p wcnf %d %d %"PRId64, &nv, &nc, &top);
            if (ret >= 2) {
                isWCNF = 1;
                goto BODY;
            }
        }

        fprintf(stderr, "unexpected line: %s\n", line);
        exit(1);
    }

BODY:
    glp_set_obj_dir(prob, GLP_MIN);
    glp_add_cols(prob, nv);
    for (int i = 1; i <= nv; i++) {
        char name[128];
        snprintf(name, sizeof(name), "x%d", i);
        glp_set_col_name(prob, i, name);
        glp_set_col_bnds(prob, i, GLP_DB, 0.0, 1.0);
        glp_set_col_kind(prob, i, GLP_IV);
    }

    for (int i = 1; i <= nc; i++) {
        int64_t cost = 1;
        if (isWCNF) fscanf(file, " %"PRId64, &cost);

	std::vector<int> lits;
        while (1) {
            int lit;
            fscanf(file, "%d", &lit);
            if (lit == 0) break;
	    lits.push_back(lit);
        }

        if (cost != top && lits.size() == 1) {
            int lit = lits[0];
            if (lit > 0) {
	        int v = lit;
	        // obj += cost*(1 - v)
                glp_set_obj_coef(prob, 0, glp_get_obj_coef(prob, 0) + cost);
                glp_set_obj_coef(prob, v, glp_get_obj_coef(prob, v) - cost);
            } else {
	        int v = - lit;
		// obj += cost*v
                glp_set_obj_coef(prob, v, glp_get_obj_coef(prob, v) + cost);
	    }
	    continue;
	}

        std::vector<int> inds;
        std::vector<double> vals;
	char name[128];

        if (cost != top) {
            int r = glp_add_cols(prob, 1);
            snprintf(name, sizeof(name), "r%d", i);
            glp_set_col_name(prob, r, name);
            glp_set_col_bnds(prob, r, GLP_DB, 0.0, 1.0);
            glp_set_obj_coef(prob, r, cost);
            inds.push_back(r);
            vals.push_back(1);
        }

        int rhs = 1;
	for (std::vector<int>::iterator j = lits.begin(); j != lits.end(); j++) {
            int lit = *j;
            if (lit > 0) {
                inds.push_back(lit);
                vals.push_back(1);
            } else {
                inds.push_back(-lit);
                vals.push_back(-1);
                rhs--;
            }
        }

	int row = glp_add_rows(prob, 1);
        snprintf(name, sizeof(name), "c%d", i);
        glp_set_row_name(prob, row, name);
        glp_set_row_bnds(prob, row, GLP_LO, rhs, 0.0);
        glp_set_mat_row(prob, row, inds.size(), inds.data() - 1, vals.data() - 1);
    }

    fclose(file);
    return nv;
}

static
int output_comment(void *info, const char *s)
{
    printf("c %s", s);
    return 1;
}

static
void print_model(glp_prob *prob, int nv)
{
    for (int i = 1; i <= nv; i++) {
        if (i % 10 == 1) {
            if (i != 1) puts(""); // new line
            fputs("v", stdout);
        }
        if (glp_mip_col_val(prob, i) >= 0.5)
            printf(" %d", i);
        else
            printf(" -%d", i);
    }
    puts(""); // new line
}

static
void intopt_callback(glp_tree *tree, void *info)
{
    switch (glp_ios_reason(tree)) {
    case GLP_IBINGO: // Better integer solution found
        {
            glp_prob *prob = glp_ios_get_prob(tree);
            int64_t val = (int64_t)glp_mip_obj_val(prob);
            printf("o %"PRId64 "\n", val);
        }
        break;
    default:
        break;
    }
    return;
}

int main(int argc, char **argv)
{
    if (1 >= argc) {
        fprintf(stderr, "USAGE: glpsol-maxsat [file.cnf|file.wcnf]");
        exit(1);
    }
    char *filename = argv[1];

    glp_prob *prob = glp_create_prob();
    glp_term_hook(output_comment, NULL);
    int nv = read_wcnf(prob, filename);

    glp_iocp parm;
    glp_init_iocp(&parm);
    parm.presolve = GLP_ON;
    parm.cb_func  = intopt_callback;
    int err = glp_intopt(prob, &parm);

    if (err != 0) {
        switch (err) {
        case GLP_EBOUND:
            printf("c GLP_EBOUND\n");
            break;
        case GLP_EROOT:
            printf("c GLP_EROOT\n");
            break;
        case GLP_ENOPFS:
            printf("c GLP_ENOPFS\n");
            break;
        case GLP_ENODFS:
            printf("c GLP_ENODFS\n");
            break;
        case GLP_EFAIL:
            printf("c GLP_EFAIL\n");
            break;
        case GLP_EMIPGAP:
            printf("c GLP_EMIPGAP\n");
            break;
        case GLP_ETMLIM:
            printf("c GLP_ETMLIM\n");
            break;
        case GLP_ESTOP:
            printf("c GLP_ESTOP\n");
            break;
        default:
            printf("c glp_intopt() returns unknown error code: %d\n", err);
        }
        puts("s UNKNOWN");
        exit(1);
    }

    int status = glp_mip_status(prob);
    switch (status) {
    case GLP_OPT:
        puts("s OPTIMUM FOUND");
        print_model(prob, nv);
        break;
    case GLP_FEAS:
        puts("s UNKNOWN");
        break;
    case GLP_NOFEAS:
        puts("s UNSATISFIABLE");
        break;
    default:
        printf("c glp_mip_status() returns unknown status code: %d\n", status);
        puts("s UNKNOWN");
        exit(1);
    }

    glp_delete_prob(prob);
    return 0;
}
