#include "testApp.h"

void testApp::setup() {
	depthImage.loadImage("depth.png");
}

vector<ofVec2f> points;
Mat sobelxy;
Mat sobelbox;
int attempts;
void testApp::update() {
	Mat mat = getMat(depthImage);
	
	Sobel(mat, sobelxy, CV_32F, 1, 1);
	sobelxy = abs(sobelxy);
	boxFilter(sobelxy, sobelbox, 0, cv::Size(7, 7));
	
	triangulator.init();
	points.clear();
	int i = 0;
	attempts = 0;
	while(i < 5000) {
		Point2d curPosition((int) ofRandom(sobelbox.cols - 1), (int) ofRandom(sobelbox.rows - 1));
		float curSample = sobelbox.at<unsigned char>(curPosition) / 255.f;
		float curGauntlet = powf(ofRandom(0, 1), 2 * (float) mouseX / ofGetWidth());
		if(curSample > curGauntlet) {
			points.push_back(makeVec(curPosition));
			triangulator.addPoint(curPosition.x, curPosition.y);
			sobelbox.at<unsigned char>(curPosition) = 0; // don't do the same point twice
			i++;
		}
		attempts++;
		if(i > attempts * 100) {
			break;
		}
	}
	
	int w = mat.cols - 1;
	int h = mat.rows - 1;
	triangulator.addPoint(0, 0);
	triangulator.addPoint(w, 0);
	triangulator.addPoint(w, h);
	triangulator.addPoint(0, h);
	
	triangulator.triangulate();	
}

void testApp::draw() {
	ofBackground(0);

	depthImage.draw(0, 0);
	
	ofEnableBlendMode(OF_BLENDMODE_ADD);
	drawMat(sobelxy, 0, 0);
	ofDisableBlendMode();
	
	drawMat(sobelbox, 640, 0);
	
	triangulator.drawLines();
	
	ofDrawBitmapString(ofToString(attempts), 10, 20);
}
