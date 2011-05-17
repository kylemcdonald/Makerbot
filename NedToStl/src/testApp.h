#pragma once

#include "ofMain.h"
#include "ofxSTL.h"

class Flt {
public:
	void setup(string filename);
	void draw();
	ofVec3f get(int x, int y);
	void set(int x, int y, float z);
private:
	void addFace(ofVec3f a, ofVec3f b, ofVec3f c);
	void addFace(ofVec3f a, ofVec3f b, ofVec3f c, ofVec3f d);
	void addBack();
	
	ofBuffer fileBuffer;
	vector<float> raw;
	int width, height;
	ofMesh mesh;
	float normalization;
	float offsetAmount;
	
	int lastWidth, lastHeight;
};

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	Flt flt;
	ofEasyCam cam;
	ofLight light;
};
