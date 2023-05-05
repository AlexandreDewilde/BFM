#version 330

layout(location = 0) in vec2 node;
layout(location = 1) in vec2 effect;

out vec3 transformed;
out vec3 colour;

void main(void) {
	transformed = vec3(node + effect, 1.0); // displacement
	colour = vec3(effect, 1.0);

	gl_Position = vec4(transformed, 1.0);
}
