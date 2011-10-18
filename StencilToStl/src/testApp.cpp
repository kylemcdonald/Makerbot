#include "testApp.h"

void testApp::setup() {	
	ofSetVerticalSync(true);
	
	panel.setup(250, 800);
	panel.addPanel("Settings");
	panel.addSlider("thresholdValue", 128, 0, 255);
	panel.addSlider("minSide", 0, 0, 100);
	panel.addSlider("zOffset", 5, 0, 100);
	panel.addSlider("sigma", 4, 0, 5);
	panel.addToggle("drawDebug", false);
	panel.addToggle("drawWireframe", false);
	panel.addToggle("exportStl", false);
	
	panel.addPanel("Lighting");
	panel.addToggle("drawLight", true);
	panel.addSlider("lightDistance", 15, 0, 50);
	panel.addSlider("lightRotation", 0, -PI, PI);
	panel.addSlider("lightY", -80, -150, 150);
	panel.addSlider("lightZ", 660, 0, 1000);
	panel.addSlider("diffuseAmount", 245, 0, 255);
	
	stencil.loadImage("stencil.png");
	//stencil.mirror(false, true);
	stencil.setImageType(OF_IMAGE_GRAYSCALE);
	
	cam.setDistance(1000);
	
	redLight.setup();
	greenLight.setup();
	blueLight.setup();
	
	thresh.allocate(stencil.getWidth(), stencil.getHeight(), OF_IMAGE_GRAYSCALE);
}

ofVec3f buildNormal(ofVec3f& a, ofVec3f& b, ofVec3f& c) {
	return ((c - a).cross(b - a)).normalize();
}

void checkNormals(ofMesh& mesh) {
	if(mesh.getNumNormals() != mesh.getNumVertices()) {
		vector<ofVec3f>& vertices = mesh.getVertices();
		vector<ofVec3f>& normals = mesh.getVertices();
		vector<ofIndexType>& indices = mesh.getIndices();
		if(indices.size() > 0) {
			for(int i = 0; i < indices.size() - 2; i += 3) {
				mesh.addNormal(buildNormal(vertices[indices[i+2]], vertices[indices[i+1]], vertices[indices[i]]));
			}
		} else {
			if(mesh.getMode() == OF_TRIANGLES_MODE) {
				for(int i = 0; i < vertices.size() - 2; i += 3) {
					mesh.addNormal(buildNormal(vertices[i+2], vertices[i+1], vertices[i+0]));
				}
			} else if(mesh.getMode() == OF_TRIANGLE_STRIP_MODE) {
				for(int i = 0; i < vertices.size() - 2; i++) {
					mesh.addNormal(buildNormal(vertices[i+2], vertices[i+1], vertices[i+0]));
				}
			}
		}
		for(int i = 0; i < 2; i++) {
			mesh.addNormal(normals.back());
		}
	}
}

void addMesh(ofxSTLExporter& exporter, ofMesh& mesh) {
	vector<ofVec3f>& vertices = mesh.getVertices();
	vector<ofVec3f>& normals = mesh.getNormals();
	vector<ofIndexType>& indices = mesh.getIndices();
	checkNormals(mesh);
	if(indices.size() > 0) {
		for(int i = 0; i < indices.size() - 2; i += 3) {
			exporter.addTriangle(vertices[indices[i+2]], vertices[indices[i+1]], vertices[indices[i]], normals[indices[i]]);
		}
	} else {
		if(mesh.getMode() == OF_TRIANGLES_MODE) {
			for(int i = 0; i < vertices.size() - 2; i += 3) {
				exporter.addTriangle(vertices[i+2], vertices[i+1], vertices[i+0], normals[i]);
			}
		} else if(mesh.getMode() == OF_TRIANGLE_STRIP_MODE) {
			for(int i = 0; i < vertices.size() - 2; i++) {
				exporter.addTriangle(vertices[i+2], vertices[i+1], vertices[i+0], normals[i]);
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

void addFace(ofMesh& mesh, ofVec3f a, ofVec3f b, ofVec3f c) {
	ofVec3f normal = ((c - a).cross(b - a)).normalize();
	mesh.addNormal(normal);
	mesh.addVertex(a);
	mesh.addNormal(normal);
	mesh.addVertex(b);
	mesh.addNormal(normal);
	mesh.addVertex(c);
}

void addFace(ofMesh& mesh, ofVec3f a, ofVec3f b, ofVec3f c, ofVec3f d) {
	addFace(mesh, a, b, d);
	addFace(mesh, b, c, d);
}

#include "Poco/DateTimeFormatter.h"
void testApp::update() {
	float zOffset = panel.getValueF("zOffset");
	int thresholdValue = panel.getValueF("thresholdValue");
	float minSide = panel.getValueF("minSide");
	float minArea = minSide * minSide;
	float sigma = panel.getValueF("sigma");
	
	threshold(stencil, thresh, thresholdValue);
	Mat threshMat = toCv(thresh);
	vector< vector<cv::Point> > rawContours;
	vector<Vec4i> hierarchy;
	findContours(threshMat, rawContours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_NONE);
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
		}
	}
	
	mesh.clear();
	ofVec3f offset(0, 0, -zOffset);
	for(int k = 0; k < contours.size(); k++) {
		for(int i = 0; i < contours[k].size(); i++) {
			ofVec3f a, b;
			int cur = i;
			int next = (i + 1) % contours[k].size();
			a.set(contours[k][cur].x, contours[k][cur].y, 0);
			b.set(contours[k][next].x, contours[k][next].y, 0);
			addFace(mesh, b, a, a + offset, b + offset);
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
	
	ofPath bottom;
	for(int k = 0; k < contours.size(); k++) {
		bottom.newSubPath();
		for(int i = 0; i < contours[k].size(); i++) {
			bottom.lineTo(contours[k][i].x, contours[k][i].y);
		}
		bottom.close();
	}
	bottomMesh = bottom.getTessellation();
	
	ofVec3f centering(-stencil.getWidth() / 2, -stencil.getHeight() / 2, 0);
	
	vector<ofVec3f>& bottomVertices = bottomMesh.getVertices();
	for(int i = 0; i < bottomVertices.size(); i++) {
		bottomVertices[i] += centering;
	}
	
	vector<ofVec3f>& topVertices = topMesh.getVertices();
	for(int i = 0; i < topVertices.size(); i++) {
		topVertices[i] += centering + offset;
	}
	
	vector<ofVec3f>& wallVertices = mesh.getVertices();
	for(int i = 0; i < wallVertices.size(); i++) {
		wallVertices[i] += centering;
	}
	
	bool exportStl = panel.getValueB("exportStl");
	if(exportStl) {
		string pocoTime = Poco::DateTimeFormatter::format(Poco::LocalDateTime(), "%Y-%m-%d at %H.%M.%S");
		ofxSTLExporter exporter;
		exporter.beginModel("STL Export");
		addMesh(exporter, mesh);
		addMesh(exporter, topMesh);
		addMesh(exporter, bottomMesh);
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
	}
	
	glEnable(GL_DEPTH_TEST);
	
	cam.begin();
	ofEnableLighting();
	ofSetColor(255);
	if(panel.getValueB("drawWireframe")) {
		mesh.drawWireframe();
		bottomMesh.drawWireframe();
		topMesh.drawWireframe();
	} else {
		mesh.drawFaces();
		bottomMesh.drawFaces();
		topMesh.drawFaces();
	}
	ofDisableLighting();	
	
	cam.end();
	glDisable(GL_DEPTH_TEST);
}

void testApp::keyPressed(int key) {
}
