#version 440

in layout(location=1) vec2 fsTex;
out layout(location=0) vec4 target;

uniform sampler2D mainTex;
uniform vec4 color;
uniform float objectGlow;

void main()
{	
	vec4 mainColor = texture(mainTex, fsTex.xy);
	target = mainColor * color;
	target = clamp(target * (1.0f + objectGlow * 5), 0, 1);
}