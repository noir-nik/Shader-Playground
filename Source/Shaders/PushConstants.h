#ifndef SHADER_PLAYGROUND_PUSHCONSTANTS_H
#define SHADER_PLAYGROUND_PUSHCONSTANTS_H

#ifdef __cplusplus
struct PushConstants {
	float resolution[2];
	float mouse[2];
	float time;
	float time_delta;
	int   frame;
};
#endif

#ifdef GL_core_profile
struct PushConstants {
	vec2  resolution;
	vec2  mouse;
	float time;
	float time_delta;
	int   frame;
};
#endif

#ifdef __SLANG__
struct PushConstants {
	float2 resolution;
	float2 mouse;
	float  time;
	float  time_delta;
	int    frame;
};
#endif

#endif // SHADER_PLAYGROUND_PUSHCONSTANTS_H
