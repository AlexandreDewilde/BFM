import ctypes

import pyglet.gl as gl

from .matrix import Matrix

class Shader_error(Exception):
	def __init__(self, msg):
		self.msg = msg

def create_shader(target, path):
	with open(path, "rb") as f:
		src = f.read()

	src_len = ctypes.c_int(len(src) + 1)
	src_buf = ctypes.create_string_buffer(src)

	buf_ptr = ctypes.cast(
		ctypes.pointer(ctypes.pointer(src_buf)),
		ctypes.POINTER(ctypes.POINTER(ctypes.c_char)))

	gl.glShaderSource(target, 1, buf_ptr, ctypes.byref(src_len))
	gl.glCompileShader(target)

	# handle potential errors

	log_len = gl.GLint(0)
	gl.glGetShaderiv(target, gl.GL_INFO_LOG_LENGTH, ctypes.byref(log_len))

	log_buf = ctypes.create_string_buffer(log_len.value)
	gl.glGetShaderInfoLog(target, log_len, None, log_buf)

	if log_len.value > 1:
		raise Shader_error(str(log_buf.value))

class Shader:
	def __init__(self, vert_path, frag_path):
		self.program = gl.glCreateProgram()

		# vertex shader

		self.vert_shader = gl.glCreateShader(gl.GL_VERTEX_SHADER)
		create_shader(self.vert_shader, vert_path)
		gl.glAttachShader(self.program, self.vert_shader)

		# fragment shader

		self.frag_shader = gl.glCreateShader(gl.GL_FRAGMENT_SHADER)
		create_shader(self.frag_shader, frag_path)
		gl.glAttachShader(self.program, self.frag_shader)

		# link program

		gl.glLinkProgram(self.program)

		gl.glDeleteShader(self.vert_shader)
		gl.glDeleteShader(self.frag_shader)

		# uniforms

		self.mvp_location = self.find_uniform(b"mvp_matrix")
		self.max_effect_location = self.find_uniform(b"max_effect")
		self.anim_location = self.find_uniform(b"anim")

	def __del__(self):
		# XXX work around weird bug

		if gl.glDeleteProgram is not None:
			gl.glDeleteProgram(self.program)

	def find_uniform(self, name):
		return gl.glGetUniformLocation(self.program, ctypes.create_string_buffer(name))

	def use(self):
		gl.glUseProgram(self.program)

	def mvp_matrix(self, mvp_matrix: Matrix):
		gl.glUniformMatrix4fv(self.mvp_location, 1, gl.GL_FALSE, (gl.GLfloat * 16) (*sum(mvp_matrix.data, [])))

	def max_effect(self, max_effect: float):
		gl.glUniform1f(self.max_effect_location, max_effect)

	def anim(self, anim: float):
		gl.glUniform1f(self.anim_location, anim)
