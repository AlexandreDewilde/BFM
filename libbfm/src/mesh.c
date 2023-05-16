#include <stdio.h>
#include <string.h>

#include <bfm/mesh.h>

static double const xsi_triangles[3] = {0., 1., 0.};
static double const eta_triangles[3] = {0., 0., 1.};
static double const xsi_quads[4] = {1., -1., -1., 1.};
static double const eta_quads[4] = {1., 1., -1., -1.};

int bfm_mesh_create(bfm_mesh_t* mesh, bfm_state_t* state, size_t dim, bfm_elem_kind_t kind) {
	memset(mesh, 0, sizeof *mesh);
	mesh->state = state;

	mesh->dim = dim;
	mesh->kind = kind;

	return 0;
}

int bfm_mesh_destroy(bfm_mesh_t* mesh) {
	bfm_state_t* const state = mesh->state;

	state->free(mesh->coords);
	state->free(mesh->elems);
	state->free(mesh->edges);

	for (size_t i = 0; i < mesh->n_domains; i++) {
		bfm_domain_t* const domain = mesh->domains[i];
		state->free(domain->elements);
		state->free(domain);
	}
	state->free(mesh->domains);

	return 0;
}

static int cmp_edge(const void* e1, const void* e2) {
	bfm_edge_t* edge1 = (bfm_edge_t*) e1;
	bfm_edge_t* edge2 = (bfm_edge_t*) e2;
	// Can I use unsigned int for m1, m2 with substraction afterwards?
	int m1 = BFM_MIN(edge1->nodes[0], edge1->nodes[1]);
	int m2 = BFM_MIN(edge2->nodes[0], edge2->nodes[1]);
	int diff = m1 - m2;
	if (diff > 0)
		return -1;
	if (diff < 0)
		return 1;

	int M1 = BFM_MAX(edge1->nodes[0], edge1->nodes[1]);
	int M2 = BFM_MAX(edge2->nodes[0], edge2->nodes[1]);

	return M1 - M2;
}

static int compute_edges(bfm_mesh_t* mesh) {
	size_t n_elems = mesh->n_elems;
	size_t n_local_nodes = mesh->kind;
	size_t n_edges = n_elems * n_local_nodes;

	mesh->edges = mesh->state->alloc(n_elems * n_local_nodes * sizeof(bfm_edge_t));
	if (!mesh->edges)
		return -1;

	for (size_t elem = 0; elem < n_elems; elem++) {
		for (size_t j = 0; j < n_local_nodes; j++) {
			mesh->edges[elem * n_local_nodes + j].nodes[0] = mesh->elems[elem * n_local_nodes + j];
			mesh->edges[elem * n_local_nodes + j].nodes[1] = mesh->elems[elem * n_local_nodes + (j + 1) % n_local_nodes];
			mesh->edges[elem * n_local_nodes + j].elems[0] = elem;
            mesh->edges[elem * n_local_nodes + j].elems[1] = -1;
		}
	}

	qsort(mesh->edges, n_edges, sizeof *mesh->edges, cmp_edge);


	size_t current = 0;
	for (size_t i = 1; i < n_edges; i++) {
		if ((mesh->edges[i - 1].nodes[0] == mesh->edges[i].nodes[1] && mesh->edges[i - 1].nodes[1] == mesh->edges[i].nodes[0])) {
			mesh->edges[current] = mesh->edges[i - 1];
			mesh->edges[current].elems[1] = mesh->edges[i].elems[0];
			i++;
		}
		else {
			mesh->edges[current] = mesh->edges[i - 1];
		}
		current++;
	}
	mesh->n_edges = current;
	mesh->edges = mesh->state->realloc(mesh->edges, current * sizeof(bfm_edge_t));
	if (!mesh->edges)
		return -1;

	// mesh->boundary_nodes = mesh->state->alloc(mesh->n_nodes * sizeof *mesh->boundary_nodes);
	// if (!mesh->boundary_nodes)
		// return -1;
	// memset(mesh->boundary_nodes, 0, mesh->n_nodes);
	// for (size_t i = 0; i < current; i++) {
	// 	if (mesh->edges[i].elems[1] == -1) {
	// 		mesh->boundary_nodes[mesh->edges[i].nodes[0]] = true;
	// 		mesh->boundary_nodes[mesh->edges[i].nodes[1]] = true;
	// 	}
	// }
	return 0;
}

int bfm_mesh_read_lepl1110(bfm_mesh_t* mesh, bfm_state_t* state, char const* name) {
	int rv = -1;

	mesh->state = state;
	mesh->dim = 2; // LEPL1110 only looks at 2D meshes

	// TODO error messages & more error checking (alloc's/fscanf's)

	FILE* const fp = fopen(name, "r");

	if (fp == NULL)
		goto err_fopen; // TODO error message

	// read nodes

	fscanf(fp, "Number of nodes %zu\n", &mesh->n_nodes);
	mesh->coords = state->alloc(mesh->n_nodes * 2 * sizeof *mesh->coords);

	size_t _;

	for (size_t i = 0; i < mesh->n_nodes; i++)
		fscanf(fp, "\t%zu :\t%lf\t%lf\n", &_, &mesh->coords[i * 2], &mesh->coords[i * 2 + 1]);

	// read edges
	fscanf(fp, "Number of edges %zu\n", &mesh->n_edges);
	mesh->edges = state->alloc(mesh->n_edges * sizeof *mesh->edges);
	for (size_t i = 0; i < mesh->n_edges; i++)
		fscanf(fp, "\t%zu :\t%zu\t%zu\n", &mesh->edges->elems[0], &mesh->edges[i].nodes[0], &mesh->edges[i].nodes[1]);
	
	// read elements

	char kind_str[16];
	fscanf(fp, "Number of %15s %zu\n", kind_str, &mesh->n_elems);

	if (strcmp(kind_str, "triangles") == 0)
		mesh->kind = BFM_ELEM_KIND_SIMPLEX;

	else if (strcmp(kind_str, "quads") == 0)
		mesh->kind = BFM_ELEM_KIND_QUAD;

	else
		goto err_kind; // TODO error message

	mesh->elems = state->alloc(mesh->n_elems * mesh->kind * sizeof *mesh->elems);

	for (size_t i = 0; mesh->kind == BFM_ELEM_KIND_SIMPLEX && i < mesh->n_elems; i++)
		fscanf(fp, "\t%zu :\t%zu\t%zu\t%zu\n", &_, &mesh->elems[i * 3], &mesh->elems[i * 3 + 1], &mesh->elems[i * 3 + 2]);

	for (size_t i = 0; mesh->kind == BFM_ELEM_KIND_QUAD && i < mesh->n_elems; i++)
		fscanf(fp, "\t%zu :\t%zu\t%zu\t%zu\t%zu\n", &_, &mesh->elems[i * 4], &mesh->elems[i * 4 + 1], &mesh->elems[i * 4 + 2], &mesh->elems[i * 4 + 3]);


	fscanf(fp, "Number of domains %zu\n", &mesh->n_domains);
	mesh->domains = state->alloc(mesh->n_domains * sizeof(bfm_domain_t*));
	for (size_t i = 0; i < mesh->n_domains; i++) {
		size_t domain_id;
		fscanf(fp, "Domain :\t%zu\n", &domain_id);
		mesh->domains[domain_id] = state->alloc(sizeof(bfm_domain_t));
		bfm_domain_t* const domain = mesh->domains[domain_id];
		domain->n_elements = 0;

		fscanf(fp, "Name : %s\n", domain->name);
		fscanf(fp, "Number of elements :\t%zu\n", &domain->n_elements);
		domain->elements = state->alloc(domain->n_elements * sizeof *domain->elements);

		for (size_t j = 0; j < domain->n_elements; j++) {
			fscanf(fp, "%zu", &domain->elements[j]);
			if (j + 1 != domain->n_elements && (j + 1) % 10 == 0)
				fscanf(fp, "\n");
		fscanf(fp, "\n");
		}
	}


	// success

	rv = 0;

err_kind:

	fclose(fp);

err_fopen:

	return rv;
}
