#pragma once

#include "ofMain.h"
#include "ofxKinect.h"
#include "ofxSTL.h"
#include "ofxAutoControlPanel.h"
#include "ofxDelaunay.h"

#include "ofxCv.h"
using namespace ofxCv;
using namespace cv;

#ifdef USE_REPLICATORG
#include "ofxReplicatorG.h"
#endif

// constant scale regardless of size
// oriented correctly and sitting against the bed

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
	void exit();
	
	void keyPressed(int key);
	
	void cutoffKinect();
	void smoothKinect();
	void injectWatermark();
	void updateSurface();
	void updateTriangles();
	void updateTrianglesSimplify();
	void updateTrianglesRandom();
	void addBack(ofVec3f a, ofVec3f b, ofVec3f c);
	void updateBack();
	
	ofVec3f getSurface(XYZ& position);
	
	void postProcess(ofVec3f& vert, float scale);
	void postProcess(vector<ofVec3f>& normals);
	void postProcess(vector<Triangle>& triangles, float scale);
	void postProcess();
	
	ofxKinect kinect;
	ofEasyCam cam;
	
	vector<ofVec3f> surface;
	vector<Triangle> triangles;
	vector<ofVec3f> normals;
	
	vector<Triangle> backTriangles;
	vector<ofVec3f> backNormals;
	
	ofVec3f offset;
	float backOffset;
	float zCutoff;
	float globalScale;
	
	ofLight light;
	
	ofImage watermark;
	
	ofxAutoControlPanel panel;
	
	#ifdef USE_REPLICATORG
	ofxReplicatorG printer;
	#endif
	
	ofxDelaunay triangulator;
	
	float injectWatermarkTime, updateSurfaceTime, updateTrianglesTime, updateBackTime, postProcessTime, renderTime;
};
