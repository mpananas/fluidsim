#version 330
layout (location = 0) in vec2 in_pos;
layout (location = 1) in vec2 in_vel;
layout (location = 2) in vec3 in_col;

uniform vec2 window_info;
uniform float rad;

out vec4 ellipse_uv;
out vec2 pos;
flat out int id;
flat out vec2 vel;
flat out vec3 col;

void main()
{
	gl_Position = vec4(2.0f * in_pos - vec2(1.0f, 1.0f), 0.0f, 1.0f);
	gl_PointSize = 2.0f * rad * max(window_info.x, window_info.y);
	ellipse_uv = vec4(1.0f, 0.0f, 0.0f, window_info.y / window_info.x);
	pos = in_pos;
	id = gl_VertexID;
	vel = in_vel;
	col = in_col;
}
