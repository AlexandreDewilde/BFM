#pragma once

#include <stdlib.h>

#include <bfm/matrix.h>

typedef enum {
	BFM_ELEM_KIND_SIMPLEX = 3,
	BFM_ELEM_KIND_QUAD = 4,
} bfm_elem_kind_t;

typedef enum {
	BFM_PLANAR_STRAINS,
} bfm_problem_type_t;

typedef struct {
	double* weights;
	double* xsi;
	double* eta;
	double* dphi_dxsi;
	double* dphi_deta;
} bfm_rule_t;

typedef struct {
	bfm_state_t* state;

	size_t dim;

	size_t n_elems;
	size_t n_nodes;
	size_t n_local_nodes;
	bfm_elem_kind_t kind;

	bfm_rule_t* rule;
	size_t* elems;
	double* coords;
} bfm_mesh_t;

typedef struct {
	size_t n_local_nodes;
	size_t* map;
	double* x;
	double* y;
} bfm_local_element_t;

int bfm_mesh_read_lepl1110(bfm_mesh_t* mesh, bfm_state_t* state, char const* name);
int bfm_mesh_destroy(bfm_mesh_t* mesh);

int bfm_build_elasticity_system(bfm_mesh_t* mesh, bfm_system_t* system, double young_modulus, double poisson_ratio, double rho, double force);
