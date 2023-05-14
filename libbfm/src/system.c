#include <bfm/system.h>

int bfm_system_create(bfm_system_t* system, bfm_state_t* state, size_t n) {
	system->state = state;
	system->kind = BFM_SYSTEM_KIND_GENERIC;

	system->n = n;

	if (bfm_matrix_full_create(&system->A, state, BFM_MATRIX_MAJOR_ROW, n) < 0)
		return -1;

	if (bfm_vec_create(&system->b, state, n) < 0) {
		bfm_matrix_destroy(&system->A);
		return -1;
	}

	return 0;
}

int bfm_system_destroy(bfm_system_t* system) {
	bfm_matrix_destroy(&system->A);
	bfm_vec_destroy(&system->b);

	return 0;
}

// elasticity systems
// only 2D simplex or quad meshes are supported

typedef struct {
	bfm_elem_kind_t kind;

	size_t map[4];
	double coord[2][4];
} elem_t;

static void get_elem(elem_t* elem, bfm_mesh_t* mesh, size_t i) {
	bfm_elem_kind_t const kind = mesh->kind;
	size_t const dim = mesh->dim;

	elem->kind = kind;

	for (size_t j = 0; j < kind; j++) {
		size_t const map = mesh->elems[kind * i + j];
		elem->map[j] = map;

		for (size_t k = 0; k < dim; k++)
			elem->coord[k][j] = mesh->coords[map * dim + k];
	}
}

static int fill_elasticity_elem(elem_t* elem, bfm_system_t* system, bfm_instance_t* instance, size_t n_forces, bfm_force_t** forces) {
	bfm_state_t* const state = instance->state;
	bfm_obj_t* const obj = instance->obj;
   bfm_material_t* const material = obj->material;
	bfm_rule_t* const rule = obj->rule;
	bfm_shape_t* const shape = &rule->shape;
	size_t dim = rule->dim;

	bfm_matrix_t* const stiffness_mat = &system->A;
	bfm_vec_t* const forces_vec = &system->b;

	// constants

   double const a = material->E / (1 - material->nu * material->nu);
	double const b = material->E * material->nu / (1 - material->nu * material->nu);
	double const c = material->E / (2 * (1 + material->nu));

	// vectors to be used later

	bfm_vec_t __attribute__((cleanup(bfm_vec_destroy))) pos;

	if (bfm_vec_create(&pos, state, dim) < 0)
		return -1;

	bfm_vec_t __attribute__((cleanup(bfm_vec_destroy))) applied_force;

	if (bfm_vec_create(&applied_force, state, dim) < 0)
		return -1;

	// go through integration points

	for (size_t i = 0; i < rule->n_points; i++) {
		// element variables

		bfm_elem_kind_t const kind = elem->kind;
		size_t* const map = elem->map;

		double* const x = elem->coord[0];
		double* const y = elem->coord[1];

		// integration rule

		double const weight = rule->weights[i];

		// shape function & its derivatives wrt xsi & eta

		double phi[4];
		shape->phi(shape, rule->points[i], phi);

		double dphi_dxsi[4];
		double dphi_deta[4];

		shape->dphi(shape, 0, rule->points[i], dphi_dxsi);
		shape->dphi(shape, 1, rule->points[i], dphi_deta);

		// compute jacobian and its determinant
		// TODO when determinant is negative, our vertices are incorrectly ordered
		//      they should be flipped around automatically when creating the mesh so we don't have to deal with negative determinants here

		double dx_dxsi = 0;
		double dx_deta = 0;
		double dy_dxsi = 0;
		double dy_deta = 0;

		for (size_t j = 0; j < kind; j++) {
			dx_dxsi += x[j] * dphi_dxsi[j];
			dx_deta += x[j] * dphi_deta[j];
			dy_dxsi += y[j] * dphi_dxsi[j];
			dy_deta += y[j] * dphi_deta[j];
		}

		double const det_J = fabs(dx_dxsi * dy_deta - dx_deta * dy_dxsi);

		// shape function derivative wrt element coordinates

		double dphi_dx[4];
		double dphi_dy[4];

		for (size_t j = 0; j < kind; j++) {
			dphi_dx[j] = (dphi_dxsi[j] * dy_deta - dphi_deta[j] * dy_dxsi) / det_J;
			dphi_dy[j] = (dphi_deta[j] * dx_dxsi - dphi_dxsi[j] * dx_deta) / det_J;
		}

		// populate force vector

		for (size_t j = 0; j < kind; j++) {
			size_t const index_i = dim * map[j];

			pos.data[0] = x[j];
			pos.data[1] = y[j];

			for (size_t k = 0; k < n_forces; k++) {
				bfm_force_t* const force = forces[k];
				bfm_force_eval(force, &pos, &applied_force);

				forces_vec->data[index_i + 0] += det_J * weight * applied_force.data[0] * material->rho * phi[j];
				forces_vec->data[index_i + 1] += det_J * weight * applied_force.data[1] * material->rho * phi[j];
			}
		}

		// populate stiffness matrix

		for (size_t j = 0; j < kind; j++) {
			int const index_i = dim * map[j];

			for (size_t k = 0; k < kind; k++) {
				int const index_j = dim * map[k];

				double const f_11 = a * dphi_dx[j] * dphi_dx[k] + c * dphi_dy[j] * dphi_dy[k];
				double const f_12 = b * dphi_dx[j] * dphi_dy[k] + c * dphi_dy[j] * dphi_dx[k];
				double const f_21 = b * dphi_dy[j] * dphi_dx[k] + c * dphi_dx[j] * dphi_dy[k];
				double const f_22 = a * dphi_dy[j] * dphi_dy[k] + c * dphi_dx[j] * dphi_dx[k];

				bfm_matrix_add(stiffness_mat, index_i + 0, index_j + 0, det_J * weight * f_11);
				bfm_matrix_add(stiffness_mat, index_i + 0, index_j + 1, det_J * weight * f_12);
				bfm_matrix_add(stiffness_mat, index_i + 1, index_j + 0, det_J * weight * f_21);
				bfm_matrix_add(stiffness_mat, index_i + 1, index_j + 1, det_J * weight * f_22);
			}
		}
	}

	return 0;
}

static void apply_constraint(bfm_system_t* system, size_t node, double value) {
	// TODO deal with band matrices

	for (size_t i = 0; i < system->A.m; i++) {
		// TODO deal with non-zero conditions

		system->b.data[i] -= value * bfm_matrix_get(&system->A, i, node);
		bfm_matrix_set(&system->A, i, node, 0);
	}

	for (size_t i = 0; i < system->A.m; i++)
		bfm_matrix_set(&system->A, node, i, 0);

	bfm_matrix_set(&system->A, node, node, 1);
	system->b.data[node] = value;
}

int bfm_system_create_elasticity(bfm_system_t* system, bfm_instance_t* instance, size_t n_forces, bfm_force_t** forces) {
	bfm_state_t* const state = instance->state;
	bfm_obj_t* const obj = instance->obj;
	bfm_mesh_t* const mesh = obj->mesh;
	size_t const n = mesh->n_nodes * mesh->dim;

	// check that mesh is supported

	if (mesh->dim != 2)
		return -1;

	if (mesh->kind != BFM_ELEM_KIND_SIMPLEX && mesh->kind != BFM_ELEM_KIND_QUAD)
		return -1;

	// create system object

	if (bfm_system_create(system, state, n) < 0)
		return -1;

	system->kind = BFM_SYSTEM_KIND_ELASTICITY;

	// go through all elements

	elem_t elem;

	for (size_t i = 0; i < mesh->n_elems; i++) {
		get_elem(&elem, mesh, i);

		if (fill_elasticity_elem(&elem, system, instance, n_forces, forces) < 0)
			return -1;
	}

	// apply conditions

	for (size_t i = 0; i < instance->n_conditions; i++) {
		bfm_condition_t* const condition = instance->conditions[i];

		for (size_t j = 0; j < mesh->n_nodes; j++) {
			for (size_t k = 0; condition->nodes[j] && k < mesh->dim; k++)
				apply_constraint(system, j * mesh->dim + k, 0);
		}
	}

	return 0;
}