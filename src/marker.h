#pragma once

struct Marker {
public:
	int id = 0;
	int type = 0;
	// 0: particle
	// 1: center gravity
	// 2: directional gravity
	float x = 0.0f;
	float y = 0.0f;
	float vx = 30.0f;
	float vy = -50.0f;
	float ax = 0.0f;
	float ay = 0.0f;
	float length = 100.0f;
	float width = 50.0f;
	float heading = 0.0f;
	float score = 1.0f;
	float certainty = 1.0f;
	bool enabled = true;
	bool manually_created = false;

	bool operator==(const Marker& m) const {
		return id == m.id && type == m.type && x == m.x && y == m.y &&
			length == m.length && width == m.width && heading == m.heading &&
			score == m.score && certainty == m.certainty && enabled == m.enabled &&
			manually_created == m.manually_created;
	}
};

extern bool g_show_all_markers;
extern bool g_hide_manually_created_markers;
extern bool g_use_metric_threshold;
extern float g_low_score_threshold;
extern float g_high_score_threshold;
extern float g_low_certainty_threshold;
extern float g_high_certainty_threshold;
extern Marker* g_marker;
extern bool g_is_modified;
