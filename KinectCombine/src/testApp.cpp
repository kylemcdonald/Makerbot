#include "testApp.h"

const float fovWidth = 1.0144686707507438;
const float fovHeight = 0.78980943449644714;
const float XtoZ = tanf(fovWidth/2)*2;
const float YtoZ = tanf(fovHeight/2)*2;
const unsigned int camWidth = 640;
const unsigned int camHeight = 480;

ofVec3f ConvertProjectiveToRealWorld(float x, float y, float z) {
	return ofVec3f((.5f-x/camWidth) * z * XtoZ,
								 (y/camHeight-.5f) * z * YtoZ,
								 z);
}

void testApp::setup() {
	ofSetVerticalSync(true);
	
	calibration.load("mbp-isight.yml");
	locationFound = false;
	
	cam.initGrabber(640, 480);
	
	kinect.init();
	kinect.open();
	
	surface.resize(kinect.getWidth() * kinect.getHeight());
}

void testApp::update() {
	cam.update();
	if(cam.isFrameNew()) {
		cv::Size boardSize(9, 7);
		Mat camMat = toCv(cam);
		locationFound = findChessboardCorners(camMat, boardSize, corners, CALIB_CB_ADAPTIVE_THRESH);
		if(locationFound) {
			
			if(camMat.type() != CV_8UC1) {
				cvtColor(camMat, grayMat, CV_RGB2GRAY);
			} else {
				grayMat = camMat;
			}
			
			cornerSubPix(grayMat, corners, cv::Size(4, 4), cv::Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 30, 0.1));
			
			vector<Point3f> objectPoints;
			float boardScale = 25; // mm
			for(int y = 0; y < boardSize.height; y++) {
				for(int x = 0; x < boardSize.width; x++) {
					objectPoints.push_back(Point3f((x - ((boardSize.width - 1) / 2)) * boardScale,
																				 (y - ((boardSize.height - 1) / 2)) * boardScale, 0));
				}
			}
			
			Mat cameraMatrix = calibration.getDistortedIntrinsics().getCameraMatrix();
			Mat distCoeffs = calibration.getDistCoeffs();
			solvePnP(Mat(objectPoints), Mat(corners), cameraMatrix, distCoeffs, rotation, translation);
			Mat rotation3x3;
			Rodrigues(rotation, rotation3x3);
			//rotation = rotation3x3.inv();
			//translation = -translation;
		}
	}
	
	if(kinect.isConnected()) {
		kinect.update();
		if(kinect.isFrameNew()) {
			float* pixels = kinect.getDistancePixels();
			int i = 0;
			for(int y = 0; y < camHeight; y++) {
				for(int x = 0; x < camWidth; x++) {
					if(pixels[i] != 0) {
						surface[i] = ConvertProjectiveToRealWorld(x, y, pixels[i]) * 10; // convert cm to mm
					}
					i++;
				}
			}
		}
	}
}

void testApp::draw() {
	ofBackground(0);
	
	ofSetColor(255);
	cam.draw(0, 0);
	
	for(int i = 0; i < corners.size(); i++) {
		ofCircle(toOf(corners[i]), 4);
	}
	
	easyCam.begin();
	ofRotateX(180);
	glEnable(GL_DEPTH_TEST);
	ofDrawAxis(100);
	if(locationFound) {
		ofMatrix4x4 location = makeMatrix(rotation, translation);
		applyMatrix(location);
		ofSetColor(255);
		ofRect(25 * -4, 25 * -3, 25 * 8, 25 * 6);
		ofDrawAxis(25);
		
		// then we need to do another translation (and rotation if necc.)
		// to get to the kinect optical center
		glTranslatef(0, 75, 50);
		ofEnableAlphaBlending();
		ofSetColor(255, 128);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &(surface[0].x));
		glDrawArrays(GL_POINTS, 0, surface.size());
		glDisableClientState(GL_VERTEX_ARRAY);
		ofDisableAlphaBlending();
	}
	glDisable(GL_DEPTH_TEST);
	easyCam.end();
	
	stringstream out;
	out << rotation << endl << translation;
	ofDrawBitmapString(out.str(), 10, 10);
	
	int tw = 640;
	int th = 480;
	/*
	 try {
	 kinect.draw(0, 0, tw, th);
	 kinect.drawDepth(0, th, tw, th);
	 } catch (exception& e) {
	 cout << ofGetTimestampString("%H:%M:%S:%i") << " kinect.draw() failed: " << e.what() << endl;
	 }
	 
	 ofSetColor(ofColor::red);
	 ofLine(tw / 2, 0, tw / 2, ofGetHeight());
	 ofLine(0, th / 2, ofGetWidth(), th / 2);
	 ofLine(0, th + th / 2, ofGetWidth(), th + th / 2);
	 */
}

void testApp::exit() {
	kinect.close();
}

void testApp::keyPressed(int key) {
}