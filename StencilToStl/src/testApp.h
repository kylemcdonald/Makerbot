#pragma once

#include "ofMain.h"
#include "ofxSTL.h"
#include "ofxAutoControlPanel.h"
#include "ofPolyUtils.h"

#include "ofxCv.h"
using namespace ofxCv;
using namespace cv;

class Triangle {
public:
	ofVec3f vert1, vert2, vert3;
	Triangle() {
	}
	Triangle(ofVec3f a, ofVec3f b, ofVec3f c) :
	vert1(a), vert2(b), vert3(c) {
	}
};

class testApp : public ofBaseApp {
public:
	
	void setup();
	void update();
	void draw();
	
	void keyPressed(int key);
	
	ofEasyCam cam;
	ofLight redLight, greenLight, blueLight;
	
	ofImage stencil;
	ofImage thresh;
	ofMesh mesh, bottomMesh, topMesh;
	vector<bool> holes;
	vector<ofPolyline> contours;
	
	ofxAutoControlPanel panel;
};
