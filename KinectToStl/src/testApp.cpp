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
	ofSetVerticalSync(true);
	
	panel.setup("Control Panel", 5, 5, 250, 600);
	panel.addPanel("Settings");
	panel.addToggle("drawMesh", "drawMesh", true);
	panel.addToggle("useSmoothing", "useSmoothing", true);
	panel.addSlider("zCutoff", "zCutoff", 100, 20, 200);
	panel.addToggle("exportStl", "exportStl", false);
	
	panel.addPanel("Extra");
	panel.addSlider("smoothingSize", "smoothingSize", 0, 0, 4, true);
	panel.addSlider("backOffset", "backOffset", 1, 0, 10);
	panel.addSlider("lightDistance", "lightDistance", 800, 0, 1000);
	panel.addSlider("lightOffset", "lightOffset", 400, 0, 1000);
	panel.addToggle("useWatermark", "useWatermark", true);
	panel.addSlider("watermarkScale", "watermarkScale", 1, 0, 1);
	panel.addSlider("watermarkXOffset", "watermarkXOffset", 10, 0, 320, true);
	panel.addSlider("watermarkYOffset", "watermarkYOffset", 10, 0, 240, true);
	
	watermark.loadImage("watermark.png");
	watermark.setImageType(OF_IMAGE_GRAYSCALE);
	
	cam.setDistance(100);
	
	kinect.init();
	kinect.open();
	
	light.setup();	
	ofSetGlobalAmbientColor(ofColor(0));
	
	surface.resize(Xres * Yres);
	triangles.resize((Xres - 1) * (Yres - 1) * 2);
	normals.resize(triangles.size() * 3);
	
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
	zCutoff = panel.getValueF("zCutoff");
	
	if(panel.getValueB("exportStl")) {		
		string pocoTime = Poco::DateTimeFormatter::format(Poco::LocalDateTime(), "%Y-%m-%d at %H.%M.%S");
		
		ofxSTLExporter exporter;
		exporter.beginModel("Kinect Export");
		addTriangles(exporter, triangles, normals);
		addTriangles(exporter, backTriangles, backNormals);
		exporter.saveModel("Kinect Export " + pocoTime + ".stl");
		
		panel.setValueB("exportStl", false);
	}
	
	kinect.update();
	if(kinect.isFrameNew())	{
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
		
		if(panel.getValueB("drawMesh")) {
			startTimer();
			updateTriangles();
			updateTrianglesTime = stopTimer();
			
			startTimer();
			updateBack();
			updateBackTime = stopTimer();
		}
	}
	
	light.setPosition(panel.getValueF("lightOffset"), 0, panel.getValueF("lightDistance"));
}

void testApp::smoothKinect() {
	int w = kinect.getWidth();
	int h = kinect.getHeight();
	float* z = kinect.getDistancePixels();
	float* zx = new float[w * h];
	float* zy = new float[w * h];
	
	for(int y = 0; y < h; y++) {
		int distance = 0;
		float stepSize = 0;
		float running = 0;
		int i = y * w;
		for(int x = 0; x < w; x++) {
			float cur = z[i];
			if(distance == 0) {
				int remaining = w - x;
				bool found = false;
				for(int k = 0; k < remaining; k++) {
					if(z[i + k] != cur) {
						distance = k;
						float diff = z[i + k] - cur;
						stepSize = diff / distance;
						found = true;
						break;
					}
				}
				if(!found) {
					distance = remaining;
					stepSize = 0;
				}
				running = cur;
			} else {
				running += stepSize;
				distance--;
			}
			if(cur != zCutoff) {
				zx[i] = running;
			} else {
				zx[i] = cur;
			}
			i++;
		}
	}
	
	for(int x = 0; x < w; x++) {
		int distance = 0;
		float stepSize = 0;
		float running = 0;
		int i = x;
		for(int y = 0; y < h; y++) {
			float cur = z[i];
			if(distance == 0) {
				int remaining = h - y;
				bool found = false;
				for(int k = 0; k < remaining; k++) {
					if(z[i + k * w] != cur) {
						distance = k;
						float diff = z[i + k * w] - cur;
						stepSize = diff / distance;
						found = true;
						break;
					}
				}
				if(!found) {
					distance = remaining;
					stepSize = 0;
				}
				running = cur;
			} else {
				running += stepSize;
				distance--;
			}
			if(cur != zCutoff) {
				zy[i] = running;
			} else {
				zy[i] = cur;
			}
			i += w;
		}
	}
	
	int n = w * h;
	for(int i = 0; i < n; i++) {
		z[i] = (zx[i] + zy[i]) / 2;
	}
	
	delete [] zx;
	delete [] zy;	
	
	Mat zMat(kinect.getHeight(), kinect.getWidth(), CV_32FC1, z, 0);
	Mat buffer;
	zMat.copyTo(buffer);
	medianBlur(buffer, zMat, 3);
	int smoothingSize = panel.getValueI("smoothingSize") * 2 + 1;
	GaussianBlur(zMat, zMat, cv::Size(smoothingSize, smoothingSize), 0);	
}

void testApp::injectWatermark() {
	float* kinectPixels = kinect.getDistancePixels();
	unsigned char* watermarkPixels = watermark.getPixels();
	int w = watermark.getWidth();
	int h = watermark.getHeight();
	int i = 0;
	float watermarkScale = panel.getValueF("watermarkScale") / 255.;
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

void calculateNormals(vector<Triangle>& triangles, vector<ofVec3f>& normals) {
	int j = 0;
	ofVec3f normal;
	for(int i = 0; i < triangles.size(); i++) {
		normal = getNormal(triangles[i]);
		for(int m = 0; m < 3; m++) {
			normals[j++] = normal;
		}
	}
}

void testApp::updateTriangles() {
	int j = 0;
	int k = 0;
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
	
	calculateNormals(triangles, normals);
}

void testApp::updateBack() {
	int j = 0;
	Triangle* cur = &backTriangles[0];
	ofVec3f offset(0, 0, panel.getValueF("backOffset"));
	
	// top and bottom triangles
	for(int x = 0; x < Xres - 1; x++) {
		int i = 0 * Xres + x;
		cur->vert1 = surface[i] + offset;
		cur->vert2 = surface[i + 1];
		cur->vert3 = surface[i];
		cur++;
		
		cur->vert1 = surface[i] + offset;
		cur->vert2 = surface[i + 1] + offset;
		cur->vert3 = surface[i + 1];
		cur++;
		
		i += (Yres - 1) * Xres;
		cur->vert1 = surface[i];
		cur->vert2 = surface[i + 1];
		cur->vert3 = surface[i] + offset;
		cur++;
		
		cur->vert1 = surface[i + 1];
		cur->vert2 = surface[i + 1] + offset;
		cur->vert3 = surface[i] + offset;
		cur++;
	}
	
	// left and right triangles
	for(int y = 0; y < Yres - 1; y++) {
		int i = y * Xres + 0;
		cur->vert3 = surface[i] + offset;
		cur->vert2 = surface[i + Xres];
		cur->vert1 = surface[i];
		cur++;
		
		cur->vert3 = surface[i] + offset;
		cur->vert2 = surface[i + Xres] + offset;
		cur->vert1 = surface[i + Xres];
		cur++;
		
		i += Xres - 1;
		cur->vert3 = surface[i];
		cur->vert2 = surface[i + Xres];
		cur->vert1 = surface[i] + offset;
		cur++;
		
		cur->vert3 = surface[i + Xres];
		cur->vert2 = surface[i + Xres] + offset;
		cur->vert1 = surface[i] + offset;
		cur++;
	}
	
	int nw = 0;
	int ne = nw + (Xres - 1);
	int sw = (Yres - 1) * Xres;
	int se = sw + (Xres - 1);
	
	cur->vert1 = surface[sw] + offset;
	cur->vert2 = surface[ne] + offset;
	cur->vert3 = surface[nw] + offset;
	cur++;
	
	cur->vert1 = surface[sw] + offset;
	cur->vert2 = surface[se] + offset;
	cur->vert3 = surface[ne] + offset;
	
	calculateNormals(backTriangles, backNormals);
}

void drawTriangleArray(vector<Triangle>& triangles, vector<ofVec3f>& normals) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &(triangles[0].vert1.x));
	glNormalPointer(GL_FLOAT, sizeof(ofVec3f), &(normals[0].x));
	glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void drawPointArray(vector<ofVec3f>& points) {
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &(points[0].x));
	glDrawArrays(GL_POINTS, 0, points.size());
	glDisableClientState(GL_VERTEX_ARRAY);
}

void testApp::draw() {
	ofBackground(0, 0, 0);
	ofSetColor(255);
	
	cam.begin();
	ofRotateY(180);
	ofEnableLighting();
	
	if(!surface.empty()) {
		ofTranslate(offset);
		
		glEnable(GL_DEPTH_TEST);
		
		startTimer();
		
		if(panel.getValueB("drawMesh")) {
			// draw triangles
			drawTriangleArray(backTriangles, backNormals);
			drawTriangleArray(triangles, normals);
		} else {
			// draw point cloud
			drawPointArray(surface);
		}
		
		renderTime = stopTimer();
		
		glDisable(GL_DEPTH_TEST);
	}
	
	ofDisableLighting();	
	cam.end();
	
	ofPushMatrix();
	ofTranslate(ofGetWidth() - 200, ofGetHeight() - 60);
	ofDrawBitmapString("injectWatermarkTime: " + ofToString((int) injectWatermarkTime), 0, 0);
	ofDrawBitmapString("updateSurfaceTime: " + ofToString((int) updateSurfaceTime), 0, 10);
	ofDrawBitmapString("updateTrianglesTime: " + ofToString((int) updateTrianglesTime), 0, 20);
	ofDrawBitmapString("updateBackTime: " + ofToString((int) updateBackTime), 0, 30);
	ofDrawBitmapString("renderTime: " + ofToString((int) renderTime), 0, 40);
	ofPopMatrix();
}

void testApp::exit() {
	kinect.close();
}
