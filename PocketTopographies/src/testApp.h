#pragma once

#include "ofMain.h"
#include "ofxSTL.h"
#include "ofxAutoControlPanel.h"
#include "ofPolyUtils.h"

#include "ofxCv.h"
using namespace ofxCv;
using namespace cv;

// reverse the normals on both
// first, flip the back around to face 'backwards' (check stl to see where it's going right now)
// then get the elevation centered
// then use elevation as a mask rather than just as data directly

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
	
	ofImage elevation, heatmap;
	ofMesh surfaceMesh;
	
	ofImage stencil;
	ofImage thresh;
	ofMesh wallMesh, bottomMesh, topMesh;
	vector<bool> holes;
	vector<ofPolyline> contours;
	
	ofxAutoControlPanel panel;
};
