#pragma once

#include "ofMain.h"
#include "ofxKinect.h"
#include "ofxSTL.h"
#include "ofxAutoControlPanel.h"

#include "ofxCv.h"
using namespace cv;

// better smoothing
// constant scale regardless of size

class Triangle {
public:
	ofVec3f vert1, vert2, vert3;
};

class testApp : public ofBaseApp {
	public:

		void setup();
		void update();
		void draw();
		void exit();
		
		void cutoffKinect();
		void smoothKinect();
		void injectWatermark();
		void updateSurface();
		void updateTriangles();
		void updateBack();
		
		ofxKinect kinect;
		ofEasyCam cam;
		
		vector<ofVec3f> surface;
		vector<ofVec3f> normalSurface;
		vector<Triangle> triangles;
		vector<ofVec3f> normals;
		
		vector<Triangle> backTriangles;
		vector<ofVec3f> backNormals;
		
		ofVec3f offset;
		float backOffset;
		float zCutoff;
		
		ofLight light;
		
		ofImage watermark;
		
		ofxAutoControlPanel panel;
		
		float injectWatermarkTime, updateSurfaceTime, updateTrianglesTime, updateBackTime, renderTime;
};
