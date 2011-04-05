#pragma once

#include "ofMain.h"
#include "ofxCv.h"
using namespace ofxCv;
using namespace cv;

#include "ofxDelaunay.h"

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	ofImage depthImage;
	ofxDelaunay triangulator;
};
