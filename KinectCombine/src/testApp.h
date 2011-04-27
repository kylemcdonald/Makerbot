#pragma once

#include "ofMain.h"
#include "ofxKinect.h"

#include "ofxCv.h"
using namespace cv;
using namespace ofxCv;

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	
	void keyPressed(int key);
	
	vector<Point2f> corners;
	ofVideoGrabber cam;
	Calibration calibration;
	ofEasyCam easyCam;
	
	Mat grayMat;
	Mat rotation, translation;
	bool locationFound;
	
	ofxKinect kinect;
	vector<ofVec3f> surface;
};
