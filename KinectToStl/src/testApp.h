#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxKinect.h"
#include "ofxSTL.h"
#include "ofxAutoControlPanel.h"

// rotate everything
// constant scale regardless of size
// multiple lights
// smooth things

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
		
		void injectWatermark();
		void updateSurface();
		void updateTriangles();
		void updateBack();
		
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
		
		ofLight light;
		
		ofImage watermark;
		
		ofxAutoControlPanel panel;
};
