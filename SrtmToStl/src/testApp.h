#pragma once

#include "ofMain.h"

class Hgt {
public:
	void setup(string filename);
	void draw();
	ofVec3f get(int x, int y);
private:
	short swapEndianness(short x);
	void addTriangle(ofVec3f& a, ofVec3f& b, ofVec3f& c);
	
	ofBuffer fileBuffer;
	vector<short> raw;
	int width, height;
	ofMesh mesh;
	float normalization;
};

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	Hgt hgt;
	ofEasyCam cam;
	ofLight light;
};
