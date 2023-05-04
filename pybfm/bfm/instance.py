from .libbfm import lib, ffi
from .condition import Condition
from .obj import Obj
from .state import default_state

import functools

class Instance:
	def __init__(self, obj: Obj):
		self.c_instance = ffi.new("bfm_instance_t*")
		assert not lib.bfm_instance_create(self.c_instance, default_state, obj.c_obj)

		self.obj = obj

		self.gen_buffers()

	def __del__(self):
		# TODO maybe delete buffers idk
		assert not lib.bfm_instance_destroy(self.c_instance)

	def add_condition(self, condition: Condition)
		assert not lib.bfm_instance_add_condition(self.c_instance, condition.c_condition

	# visualisation stuff

	def gen_buffers(self):
		mesh = self.obj.mesh

		# create VAO

		self.vao = gl.GLuint(0)
		gl.glGenVertexArrays(1, ctypes.byref(self.vao))
		gl.glBindVertexArray(self.vao)

		# create coords VBO

		self.vbo = gl.GLuint(0)
		gl.glGenBuffers(1, ctypes.byref(self.vbo))
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, self.vbo)

		coords_t = gl.GLfloat * len(mesh.coords)

		gl.glBufferData(gl.GL_ARRAY_BUFFER,
			ctypes.sizeof(coords_t),
			(coords_t) (*mesh.coords),
			gl.GL_STATIC_DRAW)

		gl.glVertexAttribPointer(0, mesh.dim, gl.GL_FLOAT, gl.GL_FALSE, 0, 0)
		gl.glEnableVertexAttribArray(0)

		# create effects VBO
		# XXX this is a separate VBO because we'd like to be able to update this independently of the VBO responsible for rendering the mesh itself

		self.effects_vbo = gl.GLuint(0)
		gl.glGenBuffers(1, ctypes.byref(self.effects_vbo))
		gl.glBindBuffer(gl.GL_ARRAY_BUFFER, self.effects_vbo)

		gl.glVertexAttribPointer(1, mesh.dim, gl.GL_FLOAT, gl.GL_FALSE, 0, 0)
		gl.glEnableVertexAttribArray(1)

		# create IBO

		self.ibo = gl.GLuint(0)
		gl.glGenBuffers(1, self.ibo)
		gl.glBindBuffer(gl.GL_ELEMENT_ARRAY_BUFFER, self.ibo)

		indices_t = gl.GLuint * len(mesh.indices)

		gl.glBufferData(gl.GL_ELEMENT_ARRAY_BUFFER,
			ctypes.sizeof(indices_t),
			(indices_t) (*mesh.indices),
			gl.GL_STATIC_DRAW)

	@functools.cached_property
	def effects(self):
		effects = []

		for i in range(self.c_instance.n_effects):
			effects.append(self.c_instance.effects[i])

		return effects

	def update_effects(self):
		gl.glBindVertexArray(self.vao)
		gl.glBindBuffer(self.effects_vbo)

		effects_t = gl.GLfloat * len(self.effects)

		gl.glBufferData(gl.GL_ARRAY_BUFFER,
			ctypes.sizeof(effects_t),
			(effects_t) (*self.effects),
			gl.GL_STATIC_DRAW)

	def draw(self):
		gl.glBindVertexArray(self.vao)

		if mesh.kind == Mesh.SIMPLEX:
			mode = gl.GL_TRIANGLES

		elif mesh.kind == Mesh.QUAD:
			mode = gl.GL_QUADS

		gl.glDrawElements(mode, len(mesh.indices), gl.GL_UNSIGNED_INT, None)
