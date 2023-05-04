from .libbfm import lib, ffi
from .mesh import Mesh
from .state import default_state

from collections.abc import Callable

class Condition:
	def __init__(self, mesh: Mesh):
		self.c_condition = ffi.new("bfm_condition_t*")
		assert not lib.bfm_condition_create(self.c_condition, default_state, mesh.c_mesh)

		self.mesh = mesh

	def __del__(self):
		assert not lib.bfm_condition_destroy(self.c_condition)

	def populate(self, discriminator: Callable[[Mesh, tuple[float]], bool]):
		for i in range(self.mesh.c_mesh.n_nodes):
			coord = tuple(self.mesh.coords[i * self.mesh.dim + j] for j in range(self.mesh.dim))
			self.c_condition.nodes[i] = discriminator(self.mesh, coord)