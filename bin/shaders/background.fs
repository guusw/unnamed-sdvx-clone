#version 440

in layout(location=1) vec2 texVp;
out layout(location=0) vec4 target;

uniform ivec2 screenCenter;
// x = bar time
// y = object glow
uniform vec2 timing;
uniform ivec2 viewport;
uniform float objectGlow;

#define pi 3.1415926535897932384626433832795

void main()
{
	float d = length(vec2(screenCenter) - texVp);
	float r = d/float(viewport.y);
	d *= 1.0f-(r*r);
	
	float t = cos(-timing.x * pi * 4 + d * 0.02) * 0.5 + 0.5;
	t = pow(t,1);
	float t1 = cos(-timing.x * pi * 8 + d * 0.081) * 0.5 + 0.5;
	t1 = pow(t1,2);
	target.xyz = t1 * vec3(0.34,0.4,0.34) * 0.06 + t * vec3(0.1,0.2,0.5) * (0.3 + 0.1 * timing.y);
	target.xyz += vec3(0.1);
	
	// Intensity fade towards bottom
	target.xyz *= vec3(1.0-0.7 * (texVp.y/viewport.y));
}