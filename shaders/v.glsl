#version 330

layout(location = 0) in vec2 pos;		// World-space position

uniform mat3 xform;			// World-to-clip transform matrix

void main() {
	// Output clip-space position
	gl_Position = vec4((xform * vec3(pos, 1.0)).xy, 0.0, 1.0);
}
