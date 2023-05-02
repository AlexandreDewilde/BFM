#pragma once

#include <bfm/mesh.h>

// int bfm_parse_problem_file(char const* filename, bfm_problem_t* problem);
int bfm_parse_mesh_file(char const* filename, bfm_mesh_t* mesh);
// int bfm_free_mesh_file(bfm_problem_t* problem);
int bfm_parse_output_file(char const* filename, bfm_mesh_t* problem);
