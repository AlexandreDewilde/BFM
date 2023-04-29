#include "mesh.h"

static int get_local_element(bfm_mesh_t *mesh, size_t e, bfm_local_element_t *element) {
    size_t n_local_nodes = mesh->n_local_nodes;

    size_t *elems = mesh->elems;
    double *coords = mesh->coords;

    size_t *map = element->map;
    double *x = element->x;
    double *y = element->y;

    for (size_t i = 0; i < n_local_nodes; i++) {
        map[i] = elems[e * n_local_nodes + i];
        x[i] = mesh->coords[map[i] * 2];
        y[i] = mesh->coords[map[i] * 2 + 1];
    }
}

int bfm_build_elasticity_system(bfm_mesh_t *mesh, bfm_matrix_t *A, bfm_matrix_t *B) {
    bfm_local_element_t element;
    element.n_local_nodes = mesh->n_local_nodes;
    bfm_rule_t *rule = mesh->rule;
    for (size_t e = 0; e < mesh->n_elems; e++) {
        get_local_element(mesh, e, &element);        

    }
}


int bfm_build_elasticity_system_local(bfm_local_element_t* element, bfm_rule_t *rule, bfm_matrix_t* A, bfm_vec_t* B) {
    double* x = element->x;
    double* y = element->y;
    double* dphi_dxsi = rule->dphi_dxsi;
    double* dphi_deta = rule->dphi_deta;
    size_t *map = element->map;

    for (int i = 0; i < element->n_local_nodes; i++) {
        double const weight = rule->weights[i];

        double const xsi = rule->xsi[i];
        double const eta = rule->eta[i];

        double dx_dxsi = 0;
        double dx_deta = 0;
        double dy_dxsi = 0;
        double dy_deta = 0;

        for (size_t k = 0; k < element->n_local_nodes; k++) {
            dx_dxsi += x[k] * dphi_dxsi[k];
            dx_deta += x[k] * dphi_deta[k];
            dy_dxsi += y[k] * dphi_dxsi[k];
            dy_deta += y[k] * dphi_deta[k];
        }

        double det_jacobian = fabs(dx_dxsi*dy_deta - dx_deta*dy_dxsi);

        double dphi_dx[4];
        double dphi_dy[4];
        for (size_t j = 0; j < element->n_local_nodes; j++) {
            dphi_dx[j] = (dphi_dxsi[j] * dy_deta - dphi_deta[j] * dx_dxsi) / det_jacobian;
            dphi_dy[j] = (dphi_deta[j] * dx_dxsi - dphi_dxsi[j] * dx_deta) / det_jacobian;
        }

        for (size_t j = 0; j < element->n_local_nodes; j++) {
            // int const index_i = 2 * map[j] + 1;
            // B[index_i] += det_J * weight * phi[j] * -g * rho;
        }

        for (size_t j = 0; j < element->n_local_nodes; j++) {
            int const index_i = 2 * map[j];
            for (size_t k = 0; k < element->n_local_nodes; k++) {
                int const index_j = 2 * map[k];
                
            }
        }
    }
    return 0;
}