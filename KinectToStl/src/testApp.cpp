#include "testApp.h"

float startTime;
void startTimer() {
	startTime = ofGetElapsedTimef();
}

float stopTimer() {
	float stopTime = ofGetElapsedTimef();
	return 1000. * (stopTime - startTime);
}

ofVec3f getNormal(Triangle& triangle) {
	ofVec3f a = triangle.vert1 - triangle.vert2;
	ofVec3f b = triangle.vert3 - triangle.vert2;
	ofVec3f normal = b.cross(a);
	normal.normalize();
	return normal;
}

void calculateNormals(vector<Triangle>& triangles, vector<ofVec3f>& normals) {
	normals.resize(triangles.size() * 3);
	
	int j = 0;
	ofVec3f normal;
	for(int i = 0; i < triangles.size(); i++) {
		normal = getNormal(triangles[i]);
		for(int m = 0; m < 3; m++) {
			normals[j++] = normal;
		}
	}
}

const float FovH=1.0144686707507438;
const float FovV=0.78980943449644714;
const float XtoZ = tanf(FovH/2)*2;
const float YtoZ = tanf(FovV/2)*2;
const unsigned int Xres = 640;
const unsigned int Yres = 480;

ofVec3f ConvertProjectiveToRealWorld(float x, float y, float z) {
	return ofVec3f((x/Xres-.5f) * z * XtoZ,
								 (.5f-y/Yres) * z * YtoZ,
								 z);
}

void testApp::setup() {
	//printer.setup("192.168.0.160", 2000);
	
	ofSetVerticalSync(true);
	
	panel.setup("Control Panel", 5, 5, 250, 800);
	panel.addPanel("Settings");
	panel.addSlider("zCutoff", "zCutoff", 100, 20, 200);
	panel.addSlider("stlWidth", "stlWidth", 60, 10, 120);
	panel.addToggle("exportStl", "exportStl", false);
	panel.addToggle("useRandomExport", "useRandomExport", false);
	
	panel.addPanel("Extra");
	panel.addToggle("drawMesh", "drawMesh", true);
	panel.addToggle("drawWire", "drawWire", false);
	panel.addToggle("useRandom", "useRandom", false);
	panel.addSlider("randomCount", "randomCount", 10000, 500, 20000, true);
	panel.addSlider("randomBlur", "randomBlur", 6, 0, 10);
	panel.addSlider("randomWeight", "randomWeight", 1.5, 0, 2);
	panel.addToggle("useSmoothing", "useSmoothing", true);
	panel.addSlider("smoothingAmount", "smoothingAmount", 1, 0, 4);
	panel.addSlider("backOffset", "backOffset", 1, 0, 4);
	panel.addToggle("useWatermark", "useWatermark", false);
	panel.addSlider("watermarkScale", "watermarkScale", 2, 0, 8);
	panel.addSlider("watermarkXOffset", "watermarkXOffset", 10, 0, 320, true);
	panel.addSlider("watermarkYOffset", "watermarkYOffset", 10, 0, 240, true);
	panel.addSlider("lightDistance", "lightDistance", 800, 0, 1000);
	panel.addSlider("lightOffset", "lightOffset", 400, 0, 1000);
	panel.addToggle("useSimplify", "useSimplify", true);
	panel.addToggle("pause", "pause", false);
	
	watermark.loadImage(".watermark.png");
	watermark.setImageType(OF_IMAGE_GRAYSCALE);
	
	cam.setDistance(100);
	
	kinect.init();
	kinect.open();
	
	light.setup();
	light.setDiffuseColor(ofColor(128));
	
	surface.resize(Xres * Yres);
	
	backTriangles.resize(2 * (((Xres - 1) + (Yres - 1)) * 2 + 1));
	backNormals.resize(triangles.size() * 3);
	
	ofSetFrameRate(60);
	
	offset.set(0, 0, -100);
}

void addTriangles(ofxSTLExporter& exporter, vector<Triangle>& triangles, vector<ofVec3f>& normals) {
	for(int i = 0; i < triangles.size(); i++) {
		exporter.addTriangle(triangles[i].vert1, triangles[i].vert2, triangles[i].vert3, normals[i * 3]);
	}
}

#include "Poco/DateTimeFormatter.h"
void testApp::update() {	
	kinect.update();
	if(!panel.getValueB("pause") && kinect.isFrameNew())	{
		zCutoff = panel.getValueF("zCutoff");
		
		ofVec3f idealLeft = ConvertProjectiveToRealWorld(0, 0, zCutoff);
		ofVec3f idealRight = ConvertProjectiveToRealWorld(Xres - 1, 0, zCutoff);
		float width = (idealRight - idealLeft).x;
		globalScale = panel.getValueF("stlWidth") / width;
		
		backOffset = panel.getValueF("backOffset") / globalScale;
		
		cutoffKinect();
		
		if(panel.getValueB("useSmoothing")) {
			smoothKinect();
		}
		
		if(panel.getValueB("useWatermark")) {
			startTimer();
			injectWatermark();
			injectWatermarkTime = stopTimer();
		}
		
		startTimer();
		updateSurface();
		updateSurfaceTime = stopTimer();
		
		startTimer();
		updateBack();
		updateBackTime = stopTimer();
		
		bool exportStl = panel.getValueB("exportStl");
		bool useRandomExport = panel.getValueB("useRandomExport");
		
		startTimer();
		if((exportStl && useRandomExport) || panel.getValueB("useRandom")) {
			updateTrianglesRandom();
		} else if(panel.getValueB("useSimplify")) {
			updateTrianglesSimplify();
		} else {
			updateTriangles();
		}
		calculateNormals(triangles, normals);
		updateTrianglesTime = stopTimer();
		
		startTimer();
		postProcess();
		postProcessTime = stopTimer();
		
		if(exportStl) {
			string pocoTime = Poco::DateTimeFormatter::format(Poco::LocalDateTime(), "%Y-%m-%d at %H.%M.%S");
			
			ofxSTLExporter exporter;
			exporter.beginModel("Kinect Export");
			addTriangles(exporter, triangles, normals);
			addTriangles(exporter, backTriangles, backNormals);
			exporter.saveModel("Kinect Export " + pocoTime + ".stl");
			
			if(printer.isConnected()) {
				printer.printToFile("/home/matt/MakerBot/repg_workspace/ReplicatorG/examples/Snake.stl", "/home/matt/Desktop/snake.s3g");
			}
			
			panel.setValueB("exportStl", false);
		}
	}
	
	light.setPosition(panel.getValueF("lightOffset"), 0, panel.getValueF("lightDistance"));
}


void testApp::postProcess(ofVec3f& vert, float scale) {
	vert.z *= -1;
	vert.x *= -1;
	vert.z += zCutoff + backOffset;
	vert *= scale;
}

void testApp::postProcess(vector<Triangle>& triangles, float scale) {
	for(int i = 0; i < triangles.size(); i++) {
		Triangle& cur = triangles[i];
		postProcess(cur.vert1, scale);
		postProcess(cur.vert2, scale);
		postProcess(cur.vert3, scale);
	}
}

void testApp::postProcess(vector<ofVec3f>& normals) {
	for(int i = 0; i < normals.size(); i++) {
		ofVec3f& cur = normals[i];
		cur.z *= -1;
		cur.x *= -1;
	}
}

void testApp::postProcess() {	
	postProcess(triangles, globalScale);
	postProcess(normals);
	postProcess(backTriangles, globalScale);
	postProcess(backNormals);
}

Mat bfBuffer;
void testApp::smoothKinect() {		
	Mat zMat(kinect.getHeight(), kinect.getWidth(), CV_32FC1, kinect.getDistancePixels());
	
	int k = ((int) panel.getValueI("smoothingAmount") * 2) + 1;
	zMat.copyTo(bfBuffer);
	GaussianBlur(bfBuffer, zMat, cv::Size(k, k), 0);
}

void testApp::injectWatermark() {
	float* kinectPixels = kinect.getDistancePixels();
	unsigned char* watermarkPixels = watermark.getPixels();
	int w = watermark.getWidth();
	int h = watermark.getHeight();
	int i = 0;
	float watermarkScale = panel.getValueF("watermarkScale") / (255. * globalScale);
	int watermarkYOffset = panel.getValueI("watermarkYOffset");
	int watermarkXOffset = panel.getValueI("watermarkXOffset");
	for(int y = 0; y < h; y++) {
		for(int x = 0; x < w; x++) {
			if(watermarkPixels[i] != 0) {
				int j = (y + 1 + watermarkYOffset) * Xres - (x + watermarkXOffset) - 1; // kinect image is backwards
				kinectPixels[j] = zCutoff - watermarkPixels[i] * watermarkScale;
			}
			i++;
		}
	}
}

void testApp::cutoffKinect() {
	float* z = kinect.getDistancePixels();
	int n = kinect.getWidth() * kinect.getHeight();
	for(int i = 0; i < n; i++) {
		if(z[i] > zCutoff || z[i] < 10) {
			z[i] = zCutoff;
		}
	}
}

void testApp::updateSurface() {
	float* z = kinect.getDistancePixels();
	int i = 0;
	for(int y = 0; y < Yres; y++) {
		for(int x = 0; x < Xres; x++) {
			surface[i] = ConvertProjectiveToRealWorld(x, y, z[i]);
			i++;
		}
	}
	
	// final pass to 0 the edges
	for(int x = 0; x < Xres; x++) {
		surface[x] = ConvertProjectiveToRealWorld(x, 0, zCutoff);
		surface[(Yres - 1) * Xres + x] = ConvertProjectiveToRealWorld(x, Yres - 1, zCutoff);
	}
	for(int y = 0; y < Yres; y++) {
		surface[y * Xres] = ConvertProjectiveToRealWorld(0, y, zCutoff);
		surface[y * Xres + (Xres - 1)] = ConvertProjectiveToRealWorld(Xres - 1, y, zCutoff);
	}
}

void testApp::updateTriangles() {
	triangles.resize((Xres - 1) * (Yres - 1) * 2);
	
	int j = 0;
	for(int y = 0; y < Yres - 1; y++) {
		for(int x = 0; x < Xres - 1; x++) {
			int i = y * Xres + x;
			
			int nw = i;
			int ne = nw + 1;
			int sw = i + Xres;
			int se = sw + 1;
			
			triangles[j].vert1 = surface[nw];
			triangles[j].vert2 = surface[ne];
			triangles[j].vert3 = surface[sw];
			j++;
			
			triangles[j].vert1 = surface[ne];
			triangles[j].vert2 = surface[se];
			triangles[j].vert3 = surface[sw];
			j++;
		}
	}
}

void testApp::updateTrianglesSimplify() {
	triangles.resize((Xres - 1) * (Yres - 1) * 2);
	
	int j = 0;
	Triangle* top;
	Triangle* bottom;
	float* z = kinect.getDistancePixels();
	for(int y = 0; y < Yres - 1; y++) {
		bool stretching = false;
		for(int x = 0; x < Xres - 1; x++) {
			int i = y * Xres + x;
			
			int nw = i;
			int ne = nw + 1;
			int sw = i + Xres;
			int se = sw + 1;
			
			bool flat = (z[nw] == z[ne] && z[nw] == z[sw] && z[nw] == z[se]);
			bool endOfRow = (x == Xres - 2);
			if(endOfRow ||
				 (stretching && flat)) {
				// stretch the quad to our new position
				top->vert2 = surface[ne];
				bottom->vert1 = surface[ne];
				bottom->vert2 = surface[se];
			} else {			
				top = &triangles[j++];
				top->vert1 = surface[nw];
				top->vert2 = surface[ne];
				top->vert3 = surface[sw];
				
				bottom = &triangles[j++];
				bottom->vert1 = surface[ne];
				bottom->vert2 = surface[se];
				bottom->vert3 = surface[sw];
				
				stretching = flat;
			}
		}
	}
	triangles.resize(j);
}

ofVec3f testApp::getSurface(XYZ& position) {
	return surface[position.y * Xres + position.x];
}

vector<ofVec2f> points;
Mat sobelxy;
Mat sobelbox;
int attempts;
void testApp::updateTrianglesRandom() {
	Mat mat = Mat(kinect.getHeight(), kinect.getWidth(), CV_32FC1, kinect.getDistancePixels());
	
	Sobel(mat, sobelxy, CV_32F, 1, 1);
	
	sobelxy = abs(sobelxy);
	int randomBlur = panel.getValueI("randomBlur") * 2 + 1;
	boxFilter(sobelxy, sobelbox, 0, cv::Size(randomBlur, randomBlur), Point2d(-1, -1), false);
	
	triangulator.init();
	points.clear();
	int i = 0;
	attempts = 0;
	int randomCount = panel.getValueI("randomCount");
	float randomWeight = panel.getValueF("randomWeight");
	while(i < randomCount) {
		Point2d curPosition(1 + (int) ofRandom(sobelbox.cols - 3), 
												1 + (int) ofRandom(sobelbox.rows - 3));
		float curSample = sobelbox.at<unsigned char>(curPosition) / 255.f;
		float curGauntlet = powf(ofRandom(0, 1), 2 * randomWeight);
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
	
	// add the edges
	int w = mat.cols;
	int h = mat.rows;
	for(int x = 0; x < w; x++) {
		triangulator.addPoint(x, 0);
		triangulator.addPoint(x, h - 1);
	}
	for(int y = 0; y < h; y++) {
		triangulator.addPoint(0, y);
		triangulator.addPoint(w - 1, y);
	}
	
	triangulator.triangulate();
	
	int n = triangulator.getNumTriangles();
	XYZ* pts = triangulator.getPoints();
	ITRIANGLE* tris = triangulator.getTriangles();
	triangles.resize(n);
	for(int i = 0; i < n; i++) {
		triangles[i].vert3 = getSurface(pts[tris[i].p1]);
		triangles[i].vert2 = getSurface(pts[tris[i].p2]);
		triangles[i].vert1 = getSurface(pts[tris[i].p3]);
	}
}

void testApp::addBack(ofVec3f a, ofVec3f b, ofVec3f c) {
	backTriangles.push_back(Triangle(a, b, c));
}

void testApp::updateBack() {
	backTriangles.clear();
	Triangle cur;
	ofVec3f offset(0, 0, backOffset);
	
	int nw = 0;
	int ne = nw + (Xres - 1);
	int sw = (Yres - 1) * Xres;
	int se = sw + (Xres - 1);
	
	ofVec3f nwOffset = surface[nw] + offset;
	ofVec3f swOffset = surface[sw] + offset;
	ofVec3f neOffset = surface[ne] + offset;
	
	// top and bottom triangles
	for(int x = 0; x < Xres - 1; x++) {
		addBack(nwOffset, surface[nw + x + 1], surface[nw + x]);
		addBack(surface[sw + x], surface[sw + x + 1], swOffset);
	}
	
	// left and right triangles
	for(int y = 0; y < Yres - 1; y++) {
		int i = Xres * y;
		addBack(surface[nw + i], surface[nw + i + Xres], nwOffset);
		addBack(neOffset, surface[ne + i + Xres], surface[ne + i]);
	}
	
	addBack(surface[nw] + offset, surface[ne] + offset, surface[ne]);
	addBack(surface[ne] + offset, surface[se] + offset, surface[se]);
	addBack(surface[sw], surface[sw] + offset, surface[nw] + offset);
	addBack(surface[se], surface[se] + offset, surface[sw] + offset);
	
	// two back faces	
	addBack(surface[sw] + offset, surface[ne] + offset, surface[nw] + offset);
	addBack(surface[sw] + offset, surface[se] + offset, surface[ne] + offset);
	
	calculateNormals(backTriangles, backNormals);
}

void drawTriangleArray(vector<Triangle>& triangles, vector<ofVec3f>& normals) {
	ofSetColor(255);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &(triangles[0].vert1.x));
	glNormalPointer(GL_FLOAT, sizeof(ofVec3f), &(normals[0].x));
	glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void drawTriangleWire(vector<Triangle>& triangles) {
	ofSetColor(255);
	int n = triangles.size();
	glBegin(GL_LINES);
	for(int i = 0; i < n; i++) {
		glVertex3fv(&(triangles[i].vert1.x));
		glVertex3fv(&(triangles[i].vert2.x));
		
		glVertex3fv(&(triangles[i].vert2.x));
		glVertex3fv(&(triangles[i].vert3.x));
		
		glVertex3fv(&(triangles[i].vert3.x));
		glVertex3fv(&(triangles[i].vert1.x));
	}
	glEnd();
}

void testApp::draw() {
	ofBackground(0, 0, 0);
	ofSetColor(255);
	
	cam.begin();
	ofEnableLighting();
	
	if(!surface.empty()) {
		
		glEnable(GL_DEPTH_TEST);
		
		ofPushStyle();
		ofPopStyle();
		
		startTimer();
		
		if(panel.getValueB("drawMesh")) {
			// draw triangles
			ofSetGlobalAmbientColor(ofColor(128));
			drawTriangleArray(backTriangles, backNormals);
			drawTriangleArray(triangles, normals);
		}
		
		if(panel.getValueB("drawWire")) {
			drawTriangleWire(backTriangles);
			drawTriangleWire(triangles);
		}
		
		renderTime = stopTimer();
		
		glDisable(GL_DEPTH_TEST);
	}
	
	ofDisableLighting();	
	cam.end();
	
	ofPushMatrix();
	ofTranslate(ofGetWidth() - 200, ofGetHeight() - 80);
	ofDrawBitmapString("injectWatermarkTime: " + ofToString((int) injectWatermarkTime), 0, 0);
	ofDrawBitmapString("updateSurfaceTime: " + ofToString((int) updateSurfaceTime), 0, 10);
	ofDrawBitmapString("updateTrianglesTime: " + ofToString((int) updateTrianglesTime), 0, 20);
	ofDrawBitmapString("updateBackTime: " + ofToString((int) updateBackTime), 0, 30);
	ofDrawBitmapString("renderTime: " + ofToString((int) renderTime), 0, 40);
	ofDrawBitmapString("tris: " + ofToString((int) triangles.size()), 0, 50);
	ofDrawBitmapString("back: " + ofToString((int) backTriangles.size()), 0, 60);
	ofDrawBitmapString("postProcessTime: " + ofToString((int) postProcessTime), 0, 70);
	ofPopMatrix();
}

void testApp::keyPressed(int key) {
}

void testApp::exit() {
	kinect.close();
}
