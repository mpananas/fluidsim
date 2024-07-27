#version 330

uniform int render_flag;

in vec4 ellipse_uv;
in vec2 pos;
flat in int id;
flat in vec2 vel;
flat in vec3 col;

float rand(vec2 co)
{
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
	vec2 circCoord = 2.0 * gl_PointCoord - 1.0;
	float theta = dot(circCoord, ellipse_uv.xy);
	mat2 rotation_mat = mat2(cos(-theta), -sin(-theta), sin(-theta), cos(-theta));
	vec2 u = rotation_mat * ellipse_uv.xy;
	vec2 v = rotation_mat * ellipse_uv.zw;
	vec2 radial_vec = cos(theta) * v + sin(theta) * u;
	if(dot(circCoord, circCoord) > dot(radial_vec, radial_vec))
		discard;
	//gl_FragColor = vec4(rand(vec2(id, 69.0f * id)), rand(vec2(69.0f * id, id)),
	//		rand(vec2(69.0f * id, 69.0f * id)), 1.0f);
	//gl_FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	vec3 blue = vec3(0.2f, 0.2f, 1.0f);
	vec3 red = vec3(1.0f, 0.375f, 0.0f);
	if(render_flag == 0)
		gl_FragColor = vec4(mix(blue, red, length(vel)), 1.0f);
	else
		gl_FragColor = vec4(col, 1.0f);
}
