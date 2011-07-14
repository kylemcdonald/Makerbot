#pragma once

#include "ofMain.h"

#include "ofxCv.h"
using namespace ofxCv;
using namespace cv;

#include "ofxAutoControlPanel.h"

#include "ofPolyUtils.h"

class testApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	
	ofVec3f projectPoint(const cv::Point& point);
	string getG1(const ofVec3f& point);
	
	ofImage heatmap, elevation;
	ofImage thresh;
	ofEasyCam cam;
	ofLight light;
	vector< vector<cv::Point> > contours;
	
	ofxAutoControlPanel panel;
};
