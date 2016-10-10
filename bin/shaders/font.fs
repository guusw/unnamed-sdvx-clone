#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location=1) in vec2 fsTex;
layout(location=0) out vec4 target;

uniform sampler2D mainTex;
uniform vec4 color;

void main()
{
	ivec2 size = textureSize(mainTex, 0);
	float alpha = texture2D(mainTex, fsTex / vec2(size)).a;
	target = vec4(color.xyz, alpha * color.a);
}