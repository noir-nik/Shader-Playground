#version 450
layout(location = 0) in vec2 u_fragCoord;
layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
	vec2 iResolution;
	vec2 iMouse;
	float iTime;
	float iTimeDelta;
	int iFrame;
};

vec2 fragCoord = u_fragCoord.xy * iResolution.xy;

#define PI 3.141592653589793

float Fill(in float d) {
	return 1.0 - smoothstep(0.0, 3.0, d);
}

void AddElement(inout vec3 bg_color, in float sdf, in vec3 color) {
	bg_color = mix(bg_color, color, sdf);
}

vec2 translate(vec2 samplePosition, vec2 offset) {
	return samplePosition - offset;
}

vec2 rotate(vec2 pos, float theta) {
	return vec2(pos.x * cos(theta) - pos.y * sin(theta), pos.x * sin(theta) + pos.y * cos(theta));
}

float sRect(in vec2 p, in vec2 w) {
	float d = max(abs(p.x / w.x), abs(p.y / w.y)) * 2.0;
	float m = max(w.x, w.y);
	return d * m - m;
}

float sRoundrect(in vec2 p, in vec2 w, in float corner) {
	vec2 d = abs(p) - w * 0.5 + corner;
	return (min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - corner) * 2.0;
}

float opBound(float sdf, float r) {
	return abs(sdf) - r;
}

float sDifference(float a, float b) {
	return max(a, -b);
}

float sUnion(float a, float b) {
	return min(a, b);
}

float sLetterE(vec2 in_pos, vec2 in_size, float in_thickness, float in_rounding) {
	in_pos = translate(in_pos, vec2(-in_size.x * 0.5, 0.0));
	float dist = sRoundrect(translate(in_pos, vec2(+in_thickness * 0.5, 0.0)), vec2(in_thickness, in_size.y), in_rounding);
	float horizontal_offset = in_size.x * 0.5;
	dist = sUnion(dist, sRoundrect(in_pos - vec2(horizontal_offset, (in_size.y * 0.5 - in_thickness * 0.5)), vec2(in_size.x, in_thickness), in_rounding));
	dist = sUnion(dist, sRoundrect(in_pos - vec2(horizontal_offset, -(in_size.y * 0.5 - in_thickness * 0.5)), vec2(in_size.x, in_thickness), in_rounding));
	dist = sUnion(dist, sRoundrect(in_pos - vec2(horizontal_offset, 0.0), vec2(in_size.x, in_thickness), in_rounding));
	return dist;
}

float sLetterR(vec2 in_pos, vec2 in_size, float in_thickness, float in_rounding) {
	float dist;

	// Vertical Line
	vec2 pos_vertical_line;
	{
		pos_vertical_line = in_pos - (in_size * vec2(-0.5, 0.0) + in_thickness * vec2(0.5, 0.0));
		dist = sRoundrect(pos_vertical_line, vec2(in_thickness, in_size.y), in_rounding);
	}

	// Clip Rect
	float s_clip_rect;
	{
		vec2 size = in_size * vec2(5, 2.);
		vec2 pos = pos_vertical_line - vec2(-size.x * 0.5, 0.);
		s_clip_rect = sRect(pos, size);
	}

	// Circle
	vec2 circle_size;
	{
		vec2 NW = in_pos - in_size * vec2(-0.5, -0.5);
		circle_size = in_size * vec2(2.0, 0.48) - in_thickness * vec2(0.0, 0.5);
		float rounding = min(in_size.x, circle_size.y) * 0.5;
		vec2 pos = NW - circle_size * vec2(0, 0.5) - in_thickness * vec2(-0.5, 0.5);
		float sd_circle = opBound(sRoundrect(pos, circle_size, rounding), in_thickness);
		dist = sUnion(dist, sDifference(sd_circle, s_clip_rect));
	}

	// Leg
	{
		vec2 SE = in_pos - in_size * vec2(0.5, 0.5);
		float rotation = -atan(in_size.y / in_size.x * 1.25);
		float leg_len = in_size.y - circle_size.y + in_thickness;
		leg_len -= in_thickness * length(vec2(sin(rotation), -cos(rotation)));
		vec2 leg_pos = SE;
		leg_pos -= in_thickness * 0.25 * vec2(sin(rotation) * 2., -cos(rotation));
		vec2 r_pos = rotate(leg_pos, rotation);
		r_pos -= leg_len * 0.5 * vec2(-1, 0.);
		dist = sUnion(dist, sRoundrect(r_pos, vec2(leg_len, in_thickness), in_rounding));
	}

	return dist;
}

float sLetterO(vec2 in_pos, vec2 in_size, float in_thickness, float in_rounding) {
	// Circle
	float dist;
	{
		vec2 size = in_size - in_thickness;
		float rounding = min(size.x, size.y) * 0.5;
		vec2 pos = in_pos;
		dist = opBound(sRoundrect(pos, size, rounding), in_thickness);
	}
	return dist;
}

float sERROR(vec2 pos, vec2 size, float thickness, float rounding, float spacing) {
	// E
	float dist = sLetterE(pos, size, thickness, rounding);

	// R
	pos = translate(pos, vec2(spacing, 0.0));
	dist = sUnion(dist, sLetterR(pos, size, thickness, rounding));

	// R
	pos = translate(pos, vec2(spacing, 0.0));
	dist = sUnion(dist, sLetterR(pos, size, thickness, rounding));

	// O
	pos = translate(pos, vec2(spacing, 0.0));
	dist = sUnion(dist, sLetterO(pos, size, thickness, rounding));

	// R
	pos = translate(pos, vec2(spacing, 0.0));
	dist = sUnion(dist, sLetterR(pos, size, thickness, rounding));
	return dist;
}

void main() {
	vec2 uv = (fragCoord.xy);
	// Make origin top left
	uv.y = iResolution.y - uv.y;
	vec3 color = vec3(0.1);
	vec3 accent = vec3(0.85);
	vec2 pos = translate(uv, vec2(35.0, 50.0));
	float letter_thickness = 5.0;
	float letter_rounding = letter_thickness * 0.4;
	vec2 letter_size = vec2(30.0, 50.0);
	float letter_spacing = letter_size.x + 5.0;
	float error = sERROR(pos, letter_size, letter_thickness, letter_rounding, letter_spacing);
	AddElement(color, Fill(error), accent);
	fragColor = vec4(color, 1.0);
}
