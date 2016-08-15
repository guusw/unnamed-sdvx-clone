#version 330
#extension GL_ARB_separate_shader_objects : enable

layout(location=1) in vec2 fsTex;
layout(location=0) out vec4 target;

uniform sampler2D mainTex;
uniform float objectGlow;

void main()
{	
	vec4 mainColor = texture(mainTex, fsTex.xy);
	target = mainColor;
	target.xyz = target.xyz * (1.0f + objectGlow * 1.0f);
	target.a = min(1, target.a + target.a * objectGlow * 0.9);
}