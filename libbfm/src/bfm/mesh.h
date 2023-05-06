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
	// src (nodes[0]) <-> dst (nodes[1]) 
	// maybe rename src and dst ?
	size_t nodes[2];
	// the two faces that are adjacent to the edges, elem[1] is -1 if the edges is on the boundary
	size_t elems[2];
} bfm_edge_t;

typedef struct {
	bfm_state_t* state;

	size_t dim;
	bfm_elem_kind_t kind;

	size_t n_elems;
	size_t n_nodes;
	size_t n_edges;

	double* coords;
	size_t* elems;
	bfm_edge_t* edges;
	// bool* boundary_nodes;

	bfm_rule_t* rule;
} bfm_mesh_t;

typedef struct {
	size_t n_local_nodes;
	size_t* map;
	double* x;
	double* y;
} bfm_local_element_t;

int bfm_mesh_create_generic(bfm_mesh_t* mesh, bfm_state_t* state, size_t dim, bfm_elem_kind_t kind);
int bfm_mesh_destroy(bfm_mesh_t* mesh);

int bfm_mesh_read_lepl1110(bfm_mesh_t* mesh, bfm_state_t* state, char const* name);

int bfm_build_elasticity_system(bfm_mesh_t* mesh, bfm_system_t* system, double young_modulus, double poisson_ratio, double rho, double force);
