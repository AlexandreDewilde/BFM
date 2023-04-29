#pragma once

#include <math.h>
#include <stdlib.h>
#include "matrix.h"

typedef enum {
	BFM_QUADS,
	BFM_TRIANGLES
} bfm_element_type_t;

typedef enum {
	BFM_PLANAR_STRAINS,
} bfm_problem_type_t;

typedef struct {
	double young_modulus;
	double poisson_ratio;
	double mass_density;
	double gravity;
	bfm_problem_type_t type;
	bfm_mesh_t *mesh;
} bfm_problem_t;

typedef struct {
	size_t n_elems;
	size_t n_nodes;
	size_t n_local_nodes;
	// size_t dim;
	bfm_element_type_t type;

	bfm_rule_t *rule;
	size_t *elems;
	double *coords;
} bfm_mesh_t;


typedef struct {
	double* weights;
	double* xsi;
	double* eta;
	double* dphi_dxsi;
	double* dphi_deta;
} bfm_rule_t;

typedef struct {
	size_t n_local_nodes;
	size_t *map;
	double* x;
	double* y;
} bfm_local_element_t;

int bfm_build_elasticity_system(bfm_mesh_t *mesh, bfm_matrix_t *A, bfm_matrix_t *B);



