#include "testApp.h"

float elevationMin = 0.426074; // mm
float elevationMax = 8.58381; // mm
float layerHeight = 2 * 0.37; // mm
float printScale = 80; // mm
float feedRate = 1080; // mm/minute
float reversalTime = 0.00125; // minutes
float pushBackTime = 0.0013; // minutes
float forwardRate = 2.003; // pwm
float reversalRate = 35; // pwm
float pushBackRate = 35; // pwm

ofVec3f toolCenter(0, 0, 20);

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	elevation.loadImage("elevation.png");
	heatmap.loadImage("heatmap.tif");
	heatmap.setImageType(OF_IMAGE_GRAYSCALE);
	thresh.allocate(heatmap.getWidth(), heatmap.getHeight(), OF_IMAGE_GRAYSCALE);
	
	panel.setup();
	panel.addPanel("Settings");
	panel.addSlider("divisions", 8, 1, 16, true);
	panel.addSlider("minSize", 4, 0, 64);
	panel.addSlider("sigma", 16, 0, 64);
	panel.addSlider("smoothedStep", 4, 1, 16, true);
	panel.addToggle("saveGcode", true);	
}

ofVec3f testApp::projectPoint(const cv::Point& point) {
	float ex = ofMap(point.x, 0, heatmap.getWidth(), 0, elevation.getWidth());
	float ey = ofMap(point.y, 0, heatmap.getHeight(), 0, elevation.getHeight());
	float cur = elevation.getColor(ex, ey).getBrightness();
	
	float x = ofMap(point.x, 0, heatmap.getWidth(), -printScale / 2, printScale / 2);
	float y = ofMap(point.y, 0, heatmap.getHeight(), printScale / 2, -printScale / 2);
	float z = ofMap(cur, 0, 255, elevationMin, elevationMax) + layerHeight;
	return ofVec3f(x, y, z);
}

string testApp::getG1(const ofVec3f& point) {
	stringstream out;
	out << "G1 X" << point.x << " Y" << point.y << " Z" << point.z << " F" << feedRate;
	return out.str();
}

void testApp::update() {
	int divisions = panel.getValueI("divisions");
	
	float minSide = panel.getValueI("minSize");
	float minArea = minSide * minSide;
	
	float sigma = panel.getValueF("sigma");
	int smoothedStep = panel.getValueI("smoothedStep");
	
	contours.clear();
	for(int i = 0; i < divisions; i++) {
		int thresholdValue = ofMap(i, -1, divisions, 0, 255);
		threshold(heatmap, thresh, thresholdValue);
		Mat threshMat = toCv(thresh);
		vector< vector<cv::Point> > curContours;
		findContours(threshMat, curContours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		for(int j = 0; j < curContours.size(); j++) {
			double area = contourArea(Mat(curContours[j]));
			if(area > minArea) {
				vector<cv::Point>& cvContour = curContours[j];
				ofPolyline ofContour;
				ofContour.setClosed(true);
				for(int k = 0; k < cvContour.size(); k++) {
					ofContour.addVertex(ofVec2f(cvContour[k].x, cvContour[k].y));
				}
				ofPolyline smoothed = ofGetSmoothed(ofContour, (int) sigma);
				cvContour.clear();
				for(int k = 0; k < smoothed.size(); k += smoothedStep) {
					cvContour.push_back(cv::Point(smoothed[k].x, smoothed[k].y));
				}
				
				contours.push_back(cvContour);
			}
		}
	}
	
	if(panel.getValueB("saveGcode")) {
		ofFile gcode;
		gcode.open("heatmap.gcode", ofFile::WriteOnly);
		for(int i = 0; i < contours.size(); i++) {
			ofVec3f finalPoint = ofVec3f(projectPoint(contours[i][0]));
			
			gcode << getG1(finalPoint) << endl; // move to first vertex
			gcode << "M108 R" << pushBackRate << endl;
			gcode << "M101" << endl; // extruder on, forward
			gcode << "G04 P" << (pushBackTime * 60 * 1000) << endl; // push back for a moment
			gcode << "M108 R" << forwardRate << endl;
			for(int j = 1; j < contours[i].size(); j++) {
				gcode << getG1(projectPoint(contours[i][j])) << endl; // add consecutive vertices
			}
			gcode << getG1(finalPoint) << endl; // close shape
			
			float returnTime = (toolCenter - finalPoint).length() / feedRate;
			ofVec3f reversalTarget = finalPoint.getInterpolated(toolCenter, reversalTime / returnTime);
			
			gcode << "M108 R" << reversalRate << endl;
			gcode << "M102" << endl; // extruder on, reverse
			gcode << getG1(reversalTarget) << endl;
			gcode << "M103" << endl; // turn extrusion off
			gcode << getG1(toolCenter) << endl << endl; // center tool
		}
		gcode.close();
		panel.setValueB("saveGcode", false);
	}
}

void testApp::draw() {
	ofBackground(0);
	ofSetColor(255);
	heatmap.draw(0, 0);
	
	for(int i = 0; i < contours.size(); i++) {
		ofNoFill();
		ofCircle(contours[i][0].x, contours[i][1].y, 4);
		ofBeginShape();
		for(int j = 0; j < contours[i].size(); j++) {
			ofVertex(contours[i][j].x, contours[i][j].y);
		}
		ofEndShape(true);
	}
	
	ofDrawBitmapString(ofToString(contours.size()), 10, 20);
}