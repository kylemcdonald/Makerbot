#include "testApp.h"

void scale(ofMesh& mesh, ofVec3f scale) {
	for(int i = 0; i < mesh.getNumVertices(); i++) {
		mesh.getVertices()[i] *= scale;
	}
}

void mask(ofImage& maskTarget, ofImage& maskSource) {
	int n = maskTarget.getWidth() * maskTarget.getHeight();
	for(int i = 0; i < n; i++) {
		if(maskSource.getPixels()[i] == 0) {
			maskTarget.getPixels()[i] = 0;
		}
	}
}

void zeroEdges(ofImage& img) {
	int w = img.getWidth();
	int h = img.getHeight();
	for(int y = 0; y < h; y++) {
		img.setColor(0, y, ofColor::black);
		img.setColor(w - 1, y, ofColor::black);
	}
	for(int x = 0; x < w; x++) {
		img.setColor(x, 0, ofColor::black);
		img.setColor(x, h - 1, ofColor::black);
	}
}

void addFace(ofMesh& mesh, ofVec3f a, ofVec3f b, ofVec3f c) {
	ofVec3f normal = ((c - b).cross(a - b)).normalize();
	mesh.addNormal(normal);
	mesh.addVertex(a);
	mesh.addNormal(normal);
	mesh.addVertex(b);
	mesh.addNormal(normal);
	mesh.addVertex(c);
}

void addFace(ofMesh& mesh, ofVec3f a, ofVec3f b, ofVec3f c, ofVec3f d) {
	addFace(mesh, a, b, c);
	addFace(mesh, a, c, d);
}

void setNormals(ofMesh& mesh, ofVec3f normal) {
	mesh.getNormals().resize(mesh.getNumVertices(), normal);
}

float heatmapScale, heatmapOffset, heatmapBase;
ofVec3f getOrthographicVertex(ofImage& img, int x, int y) {
	int h = img.getHeight();
	int i = y * h + x;
	unsigned char cur = img.getPixels()[i];
	ofVec3f result(x, h - y - 1, heatmapBase);
	if(cur > 0) {
		result.z += cur * heatmapScale + heatmapOffset;
	}
	return result;
}

void buildOrthographicSurface(ofImage& img, ofMesh& mesh) {	
	mesh.setMode(OF_TRIANGLES_MODE);
	int w = img.getWidth();
	int h = img.getHeight();
	for(int y = 0; y < h - 1; y++) {
		for(int x = 0; x < w - 1; x++) {
			ofVec3f nw = getOrthographicVertex(img, x, y);
			ofVec3f ne = getOrthographicVertex(img, x + 1, y);
			ofVec3f sw = getOrthographicVertex(img, x, y + 1);
			ofVec3f se = getOrthographicVertex(img, x + 1, y + 1);
			
			addFace(mesh, nw, sw, se, ne);
		}
	}
}

void testApp::setup() {	
	ofSetVerticalSync(true);
	
	elevation.loadImage("elevation.png");
	elevation.setImageType(OF_IMAGE_GRAYSCALE);
	
	zeroEdges(elevation);
	
	heatmap.loadImage("heatmap.png");
	heatmap.setImageType(OF_IMAGE_GRAYSCALE);
	
	zeroEdges(heatmap);
	
	//mask(heatmap, elevation);
	
	stencil.loadImage("stencil.png");
	//stencil.mirror(true, false);
	stencil.setImageType(OF_IMAGE_GRAYSCALE);
	
	panel.setup(250, 800);
	panel.addPanel("Settings");
	panel.addSlider("totalScale", 80, 0, 100);
	panel.addSlider("layerHeight", .3, .3, .4);
	panel.addSlider("heatmapScale", 20, 0, 30, true);
	panel.addSlider("heatmapOffset", 2, 0, 10, true);
	panel.addSlider("stencilOffset", 2, 0, 10, true);
	panel.addSlider("stencilScale", 4, 0, 10, true);
	panel.addSlider("thresholdValue", 128, 0, 255);
	panel.addSlider("minSide", 0, 0, 100);
	panel.addSlider("sigma", 3, 0, 5);
	panel.addToggle("drawSurface", true);
	panel.addToggle("drawDebug", false);
	panel.addToggle("drawWireframe", false);
	panel.addToggle("exportStl", false);
	
	panel.addPanel("Lighting");
	panel.addToggle("drawLight", true);
	panel.addSlider("lightDistance", 25, 0, 50);
	panel.addSlider("lightRotation", 0, -PI, PI);
	panel.addSlider("lightY", -90, -150, 150);
	panel.addSlider("lightZ", 550, 0, 1000);
	panel.addSlider("diffuseAmount", 245, 0, 255);
	
	cam.setDistance(100);
	
	redLight.setup();
	greenLight.setup();
	blueLight.setup();
	
	thresh.allocate(stencil.getWidth(), stencil.getHeight(), OF_IMAGE_GRAYSCALE);
}

void addMesh(ofxSTLExporter& exporter, ofMesh& mesh) {
	vector<ofVec3f>& vertices = mesh.getVertices();
	vector<ofVec3f>& normals = mesh.getNormals();
	vector<ofIndexType>& indices = mesh.getIndices();
	if(indices.size() > 0) {
		for(int i = 0; i < indices.size() - 2; i += 3) {
			exporter.addTriangle(vertices[indices[i+0]], vertices[indices[i+1]], vertices[indices[i+2]], normals[indices[i]]);
		}
	} else {
		if(mesh.getMode() == OF_TRIANGLES_MODE) {
			for(int i = 0; i < vertices.size() - 2; i += 3) {
				exporter.addTriangle(vertices[i+0], vertices[i+1], vertices[i+2], normals[i]);
			}
		} else if(mesh.getMode() == OF_TRIANGLE_STRIP_MODE) {
			for(int i = 0; i < vertices.size() - 2; i++) {
				exporter.addTriangle(vertices[i+0], vertices[i+1], vertices[i+2], normals[i]);
			}
		}
	}
}

void saveStl(ofMesh& mesh, string filename) {
	ofxSTLExporter exporter;
	exporter.beginModel("STL Export");
	addMesh(exporter, mesh);
	exporter.saveModel(filename);
}

void testApp::update() {
	float layerHeight = panel.getValueF("layerHeight");
	float stencilScale = layerHeight * panel.getValueI("stencilScale");
	float stencilOffset = layerHeight * panel.getValueI("stencilOffset");
	int thresholdValue = panel.getValueF("thresholdValue");
	float minSide = panel.getValueF("minSide");
	float minArea = minSide * minSide;
	float sigma = panel.getValueF("sigma");
	float totalScale = panel.getValueF("totalScale");
	ofVec3f totalScaleVec(totalScale / stencil.getWidth(), totalScale / stencil.getWidth(), 1);
	
	if(panel.hasValueChanged(variadic("heatmapScale")("heatmapOffset")("totalScale")("stencilScale")("stencilOffset")("drawSurface"))) {
		heatmapScale = (layerHeight * panel.getValueI("heatmapScale")) / 255.;
		heatmapOffset = layerHeight * panel.getValueI("heatmapOffset");
		heatmapBase = stencilScale + stencilOffset;
		surfaceMesh.clear();
		if(panel.getValueB("drawSurface")) {
			buildOrthographicSurface(heatmap, surfaceMesh);
		} else {
			int w = heatmap.getWidth();
			int h = heatmap.getHeight();
			addFace(surfaceMesh,
				getOrthographicVertex(heatmap, 0, 0),
				getOrthographicVertex(heatmap, 0, h),
				getOrthographicVertex(heatmap, w, h),
				getOrthographicVertex(heatmap, w, 0));
		}
		ofVec3f centering(-heatmap.getWidth() / 2, -heatmap.getHeight() / 2, 0);
		vector<ofVec3f>& surfaceVertices = surfaceMesh.getVertices();
		for(int i = 0; i < surfaceVertices.size(); i++) {
			surfaceVertices[i] += centering;
		}
		scale(surfaceMesh, totalScaleVec);
		panel.clearAllChanged();
	}
	
	threshold(stencil, thresh, thresholdValue);
	Mat threshMat = toCv(thresh);
	vector< vector<cv::Point> > rawContours;
	vector<Vec4i> hierarchy;
	findContours(threshMat, rawContours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	contours.clear();
	bool holeSection = false;
	holes.clear();
	int largest = 0;
	double largestArea = 0;
	for(int i = 0; i < rawContours.size(); i++) {
		
		if(i > 0 && hierarchy[i][1] == -1) {
			holeSection = true;
		}
		
		double area = contourArea(Mat(rawContours[i]));
		if(area > minArea) {
			vector<cv::Point>& rawContour = rawContours[i];
			
			ofPolyline contour;
			contour.setClosed(true);
			for(int k = 0; k < rawContour.size(); k++) {
				contour.addVertex(ofVec2f(rawContour[k].x, rawContour[k].y));
			}
			
			if(area > largestArea || i == 0) {
				largestArea = area;
				largest = contours.size();
			}
			
			contours.push_back(contour);
			holes.push_back(holeSection);
		}
	}
	
	for(int i = 0; i < contours.size(); i++) {
		if(i != largest) {
			contours[i] = ofGetSmoothed(contours[i], (int) sigma);
		} else {
			ofPolyline& cur = contours[i];
			cur.clear();
			cur.addVertex(0, 0);
			cur.addVertex(0, stencil.getHeight());
			cur.addVertex(stencil.getWidth(), stencil.getHeight());
			cur.addVertex(stencil.getWidth(), 0);
		}
	}
	
	wallMesh.clear();
	ofVec3f middleOffset(0, 0, stencilScale);
	ofVec3f topOffset(0, 0, stencilScale + stencilOffset);
	for(int k = 0; k < contours.size(); k++) {
		for(int i = 0; i < contours[k].size(); i++) {
			ofVec3f a, b;
			int cur = i;
			int next = (i + 1) % contours[k].size();
			a.set(contours[k][cur].x, contours[k][cur].y, 0);
			b.set(contours[k][next].x, contours[k][next].y, 0);
			if(k != largest) {
				addFace(wallMesh, a + middleOffset, b + middleOffset, b, a);
			} else {
				addFace(wallMesh, a + topOffset, b + topOffset, b, a);
			}
		}
	}
	
	ofPath top;
	top.setPolyWindingMode(OF_POLY_WINDING_NONZERO);
	for(int k = 0; k < contours.size(); k++) {
		if(k != largest) {
			top.newSubPath();
			for(int i = contours[k].size() - 1; i >= 0; i--) {
				top.lineTo(contours[k][i].x, contours[k][i].y);
			}
			top.close();
		}
	}
	topMesh = top.getTessellation();
	setNormals(topMesh, ofVec3f(0, 0, -1));
	
	ofPath bottom;
	for(int k = 0; k < contours.size(); k++) {
		bottom.newSubPath();
		for(int i = 0; i < contours[k].size(); i++) {
			bottom.lineTo(contours[k][i].x, contours[k][i].y);
		}
		bottom.close();
	}
	bottomMesh = bottom.getTessellation();
	setNormals(bottomMesh, ofVec3f(0, 0, -1));
	
	ofVec3f centering(-stencil.getWidth() / 2, -stencil.getHeight() / 2, 0);
	
	vector<ofVec3f>& bottomVertices = bottomMesh.getVertices();
	for(int i = 0; i < bottomVertices.size(); i++) {
		bottomVertices[i] += centering;
	}
	
	vector<ofVec3f>& topVertices = topMesh.getVertices();
	for(int i = 0; i < topVertices.size(); i++) {
		topVertices[i] += centering + middleOffset;
	}
	
	vector<ofVec3f>& wallVertices = wallMesh.getVertices();
	for(int i = 0; i < wallVertices.size(); i++) {
		wallVertices[i] += centering;
	}
	
	scale(topMesh, totalScaleVec);
	scale(bottomMesh, totalScaleVec);
	scale(wallMesh, totalScaleVec);
	
	bool exportStl = panel.getValueB("exportStl");
	if(exportStl) {
		ofxSTLExporter exporter;
		exporter.beginModel("STL Export");
		addMesh(exporter, wallMesh);
		addMesh(exporter, topMesh);
		addMesh(exporter, bottomMesh);
		addMesh(exporter, surfaceMesh);
		exporter.saveModel("stencil.stl");
		panel.setValueB("exportStl", false);
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

void testApp::draw() {
	ofBackground(0, 0, 0);
	
	if(panel.getValueB("drawDebug")) {
		ofPushMatrix();
		ofSetColor(255);
		stencil.draw(0, 0);
		
		for(int i = 0; i < contours.size(); i++) {
			ofNoFill();
			ofSetColor(holes[i] ? ofColor::blue : ofColor::red);
			ofBeginShape();
			for(int j = 0; j < contours[i].size(); j++) {
				ofVertex(contours[i][j].x, contours[i][j].y);
			}
			ofEndShape(true);
		}
		
		ofTranslate(stencil.getWidth() * 1.5, stencil.getHeight() / 2);
		topMesh.drawFaces();
		
		ofTranslate(stencil.getWidth(), 0);
		bottomMesh.drawFaces();
		
		ofPopMatrix();
		
		heatmap.draw(0, stencil.getHeight());
	}
	
	glEnable(GL_DEPTH_TEST);
	
	cam.begin();
	ofEnableLighting();
	ofSetColor(255);
	if(panel.getValueB("drawWireframe")) {
		wallMesh.drawWireframe();
		bottomMesh.drawWireframe();
		topMesh.drawWireframe();
		surfaceMesh.drawWireframe();
	} else {
		wallMesh.drawFaces();
		bottomMesh.drawFaces();
		topMesh.drawFaces();
		surfaceMesh.drawFaces();
	}
	ofDisableLighting();	
	
	cam.end();
	glDisable(GL_DEPTH_TEST);
}

void testApp::keyPressed(int key) {
}
