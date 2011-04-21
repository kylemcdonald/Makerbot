#pragma once

#include "ofMain.h"

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	ofImage raw;
	vector<ofVec3f> surface;
	ofEasyCam cam;
	ofVec3f avg;
};
