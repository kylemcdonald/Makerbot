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

int getSurfaceIndex(int x, int y) {
	return y * Xres + x;
}

ofVec3f testApp::getSurface(XYZ& position) {
	return surface[position.y * Xres + position.x];
}

ofVec3f ConvertProjectiveToRealWorld(float x, float y, float z) {
	return ofVec3f((x/Xres-.5f) * z * XtoZ,
								 (.5f-y/Yres) * z * YtoZ,
								 z);
}

void testApp::setup() {
#ifdef USE_REPLICATORG
	printer.setup("192.168.0.160", 2000);
#endif
	
	ofSetVerticalSync(true);
	
	panel.setup(250, 800);
	panel.addPanel("Settings");
	panel.addSlider("zCutoff", 80, 20, 200);
	panel.addSlider("fovWidth", .5, 0, 1);
	panel.addSlider("fovHeight", .75, 0, 1);
	panel.addSlider("stlSize", 60, 10, 120);
	panel.addToggle("exportStl", false);
	panel.addToggle("useRandomExport", false);
	
	panel.addPanel("Lighting");
	panel.addToggle("drawLight", true);
	panel.addSlider("lightDistance", 15, 0, 50);
	panel.addSlider("lightRotation", 0, -PI, PI);
	panel.addSlider("lightY", -80, -150, 150);
	panel.addSlider("lightZ", 660, 0, 1000);
	panel.addSlider("diffuseAmount", 245, 0, 255);
	
	panel.addPanel("Extra");
	panel.addToggle("drawMesh", true);
	panel.addToggle("drawWire", false);
	panel.addToggle("useRandom", false);
	panel.addSlider("temporalBlur", .9, .75, 1);
	panel.addSlider("randomCount", 10000, 500, 20000, true);
	panel.addSlider("randomBlur", 6, 0, 10);
	panel.addSlider("randomWeight", 1.5, 0, 2);
	panel.addToggle("useSmoothing", true);
	panel.addSlider("smoothingAmount", 1, 0, 4);
	panel.addSlider("backOffset", 1, 0, 4);
	panel.addToggle("useWatermark", false);
	panel.addSlider("watermarkScale", 2, 0, 8);
	panel.addSlider("watermarkXOffset", 10, 0, 320, true);
	panel.addSlider("watermarkYOffset", 10, 0, 240, true);
	panel.addToggle("useSimplify", true);
	panel.addToggle("pause", false);
	
	watermark.loadImage(".watermark.png");
	watermark.setImageType(OF_IMAGE_GRAYSCALE);
	
	cam.setDistance(100);
	
	kinect.init();
	kinect.open();
	
	surface.resize(Xres * Yres);
	
	backTriangles.resize(2 * (((Xres - 1) + (Yres - 1)) * 2 + 1));
	backNormals.resize(triangles.size() * 3);
	
	ofSetFrameRate(60);
	
	offset.set(0, 0, -100);
	
	redLight.enable();
	greenLight.enable();
	blueLight.enable();
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
		
		float fovWidth = panel.getValueF("fovWidth");
		float fovHeight = panel.getValueF("fovHeight");
		int left = Xres * (1 - fovWidth) / 2;
		int top = Yres * (1 - fovHeight) / 2;
		int right = left + Xres * fovWidth;
		int bottom = top + Yres * fovHeight;
		roiStart = Point2d(left, top);
		roiEnd = Point2d(right, bottom);
		
		ofVec3f nw = ConvertProjectiveToRealWorld(roiStart.x, roiStart.y, zCutoff);
		ofVec3f se = ConvertProjectiveToRealWorld(roiEnd.x - 1, roiEnd.y - 1, zCutoff);
		float width = (se - nw).x;
		float height = (se - nw).y;
		globalScale = panel.getValueF("stlSize") / MAX(width, height);
		
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
		updateBack();
		updateBackTime = stopTimer();
		
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
			
#ifdef USE_REPLICATORG
			if(printer.isConnected()) {
				printer.printToFile("/home/matt/MakerBot/repg_workspace/ReplicatorG/examples/Snake.stl", "/home/matt/Desktop/snake.s3g");
			}
#endif
			
			panel.setValueB("exportStl", false);
		}
	}
	
	float diffuse = panel.getValueF("diffuseAmount");
	redLight.setDiffuseColor(ofColor(diffuse / 2, diffuse / 2, 0));
	greenLight.setDiffuseColor(ofColor(0, diffuse / 2, diffuse / 2));
	blueLight.setDiffuseColor(ofColor(diffuse / 2, 0, diffuse / 2));
	
	float ambient = 255 - diffuse;
	redLight.setAmbientColor(ofColor(ambient / 2, ambient / 2, 0));
	greenLight.setAmbientColor(ofColor(0, ambient / 2, ambient / 2));
	blueLight.setAmbientColor(ofColor(ambient / 2, 0, ambient / 2));
	
	float lightY = ofGetHeight() / 2 + panel.getValueF("lightY");
	float lightZ = panel.getValueF("lightZ");
	float lightDistance = panel.getValueF("lightDistance");
	float lightRotation = panel.getValueF("lightRotation");
	redLight.setPosition(ofGetWidth() / 2 + cos(lightRotation + 0 * TWO_PI / 3) * lightDistance,
											 lightY + sin(lightRotation + 0 * TWO_PI / 3) * lightDistance,
											 lightZ);
	greenLight.setPosition(ofGetWidth() / 2 + cos(lightRotation + 1 * TWO_PI / 3) * lightDistance,
												 lightY + sin(lightRotation + 1 * TWO_PI / 3) * lightDistance,
												 lightZ);
	blueLight.setPosition(ofGetWidth() / 2 + cos(lightRotation + 2 * TWO_PI / 3) * lightDistance,
												lightY + sin(lightRotation + 2 * TWO_PI / 3) * lightDistance,
												lightZ);
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
	float alpha = panel.getValueF("temporalBlur");
	float beta = 1 - alpha;
	Mat kinectMat = Mat(480, 640, CV_32FC1, z);
	Mat kinectBuffer = Mat(480, 640, CV_32FC1, z);
	cv::addWeighted(kinectBuffer, alpha, kinectMat, beta, 0, kinectBuffer);
	ofxCv::copy(kinectMat, kinectBuffer);
	
	int i = 0;
	for(int y = 0; y < Yres; y++) {
		for(int x = 0; x < Xres; x++) {
			surface[i] = ConvertProjectiveToRealWorld(x, y, z[i]);
			i++;
		}
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
	// zero edges
	for(int x = roiStart.x; x < roiEnd.x; x++) {
		surface[getSurfaceIndex(x, roiStart.y)] = ConvertProjectiveToRealWorld(x, roiStart.y, zCutoff); // top
		surface[getSurfaceIndex(x, roiEnd.y - 1)] = ConvertProjectiveToRealWorld(x, roiEnd.y - 1, zCutoff); // bottom
	}
	for(int y = roiStart.y; y < roiEnd.y; y++) {
		surface[getSurfaceIndex(roiStart.x, y)] = ConvertProjectiveToRealWorld(roiStart.x, y, zCutoff); // left
		surface[getSurfaceIndex(roiEnd.x - 1, y)] = ConvertProjectiveToRealWorld(roiEnd.x - 1, y, zCutoff); // right
	}
	
	triangles.resize((roiEnd.x - roiStart.x) * (roiEnd.y - roiStart.y) * 2);
	int totalTriangles = 0;
	Triangle* topTriangle;
	Triangle* bottomTriangle;
	float* z = kinect.getDistancePixels();
	for(int y = roiStart.y; y < roiEnd.y - 1; y++) {
		bool stretching = false;
		for(int x = roiStart.x; x < roiEnd.x - 1; x++) {
			bool endOfRow = (x == roiEnd.x - 2);
			
			int nw = getSurfaceIndex(x, y);
			int ne = getSurfaceIndex(x + 1, y);
			int sw = getSurfaceIndex(x, y + 1);
			int se = getSurfaceIndex(x + 1, y + 1);
			
			bool flat = (z[nw] == z[ne] && z[nw] == z[sw] && z[nw] == z[se]);
			if(endOfRow ||
				 (stretching && flat)) {
				// stretch the quad to our new position
				topTriangle->vert2 = surface[ne];
				bottomTriangle->vert1 = surface[ne];
				bottomTriangle->vert2 = surface[se];				
			} else {			
				topTriangle = &triangles[totalTriangles++];
				topTriangle->vert1 = surface[nw];
				topTriangle->vert2 = surface[ne];
				topTriangle->vert3 = surface[sw];
				
				bottomTriangle = &triangles[totalTriangles++];
				bottomTriangle->vert1 = surface[ne];
				bottomTriangle->vert2 = surface[se];
				bottomTriangle->vert3 = surface[sw];
				
				stretching = flat;
			}
		}
	}
	
	triangles.resize(totalTriangles);
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
	
	triangulator.reset();
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
			points.push_back(toOf(curPosition));
			triangulator.addPoint(curPosition.x, curPosition.y, 0);
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
		triangulator.addPoint(x, 0, 0);
		triangulator.addPoint(x, h - 1, 0);
	}
	for(int y = 0; y < h; y++) {
		triangulator.addPoint(0, y, 0);
		triangulator.addPoint(w - 1, y, 0);
	}
	
	triangulator.triangulate();
	
	int n = triangulator.triangles.size();
	triangles.resize(n);
	for(int i = 0; i < n; i++) {
		triangles[i].vert3 = triangulator.triangles[i].points[0];
		triangles[i].vert2 = triangulator.triangles[i].points[1];
		triangles[i].vert1 = triangulator.triangles[i].points[2];
	}
}

void testApp::addBack(ofVec3f& a, ofVec3f& b, ofVec3f& c) {
	backTriangles.push_back(Triangle(a, b, c));
}

void testApp::addBack(ofVec3f& a, ofVec3f& b, ofVec3f& c, ofVec3f& d) {
	addBack(a, b, d);
	addBack(b, c, d);
}

void testApp::updateBack() {
	backTriangles.clear();
	Triangle cur;
	ofVec3f offset(0, 0, backOffset);
	
	ofVec3f nw = surface[getSurfaceIndex(roiStart.x, roiStart.y)];
	ofVec3f ne = surface[getSurfaceIndex(roiEnd.x - 1, roiStart.y)];
	ofVec3f sw = surface[getSurfaceIndex(roiStart.x, roiEnd.y - 1)];
	ofVec3f se = surface[getSurfaceIndex(roiEnd.x - 1, roiEnd.y - 1)];
	
	ofVec3f nwo = nw + offset;
	ofVec3f swo = sw + offset;
	ofVec3f neo = ne + offset;
	ofVec3f seo = se + offset;
	
	addBack(nwo, neo, ne, nw); // top
	addBack(ne, neo, seo, se); // right
	addBack(sw, se, seo, swo); // bottom
	addBack(nwo, nw, sw, swo); // left
	
	// two back faces
	addBack(nwo, swo, seo, neo);
	
	calculateNormals(backTriangles, backNormals);
}

void drawTriangleArray(vector<Triangle>& triangles, vector<ofVec3f>& normals) {
	if(!triangles.empty() && !normals.empty()) {
		ofSetColor(255);
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &(triangles[0].vert1.x));
		glNormalPointer(GL_FLOAT, sizeof(ofVec3f), &(normals[0].x));
		glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
	}
}

void drawTriangleWire(vector<Triangle>& triangles) {
	if(!triangles.empty()) {
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
}

void testApp::draw() {
	ofBackground(0, 0, 0);
	
	glEnable(GL_DEPTH_TEST);
	ofSetColor(255);
	
	cam.begin();
	ofEnableLighting();
		
	if(!surface.empty()) {		
		ofPushStyle();
		ofPopStyle();
		
		startTimer();
		
		if(panel.getValueB("drawMesh")) {
			// draw triangles
			drawTriangleArray(backTriangles, backNormals);
			drawTriangleArray(triangles, normals);
		}
		
		if(panel.getValueB("drawWire")) {
			drawTriangleWire(backTriangles);
			drawTriangleWire(triangles);
		}
		
		renderTime = stopTimer();
	}
	
	ofDisableLighting();	
	
	cam.end();
	glDisable(GL_DEPTH_TEST);
	
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
