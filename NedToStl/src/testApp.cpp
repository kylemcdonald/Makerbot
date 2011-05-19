#include "testApp.h"

template <class T>
T swapEndianness(T x) {
	T y;
	unsigned char* xp = (unsigned char*) &x;
	unsigned char* yp = (unsigned char*) &y;
	for(int i = 0; i < sizeof(T); i++) {
		yp[i] = xp[(sizeof(T) - 1) - i];
	}
	return y;
}

void saveStl(ofMesh& mesh, string filename) {
	ofxSTLExporter exporter;
	exporter.beginModel("Ned Export");
	vector<ofVec3f>& vertices = mesh.getVertices();
	vector<ofVec3f>& normals = mesh.getNormals();
	for(int i = 0; i < vertices.size(); i += 3) {
		exporter.addTriangle(vertices[i+2], vertices[i+1], vertices[i+=0], normals[i]);
	}
	exporter.saveModel(filename);
}

#define FLT_VOID (0) // voids are assigned this value
#define FLT_NED_13_SIZE (10812)

void Flt::setup(string filename) {
	ofFile file(filename);
	ofLogVerbose() << "readToBuffer()";
	fileBuffer = file.readToBuffer();
	ofLogVerbose() << "getBinaryBuffer()";
	float* binaryBuffer = (float*) fileBuffer.getBinaryBuffer();
	int n = fileBuffer.size() / sizeof(float);
	ofLogVerbose() << "resize()";
	raw.resize(n);
	for(int i = 0; i < n; i++) {
		raw[i] = swapEndianness(binaryBuffer[i]);
	}
	width = height = sqrt(n);
	
	ofLogVerbose() << "width " << width;
	
	offsetAmount = 10;
	normalization = 2.;//1. / 10;
	int skip = 2;
	float targetSize = 80;
	float scaleAmount = targetSize / width;
	
	ofImage img;
	img.allocate(width, height, OF_IMAGE_GRAYSCALE);
	float min = 0;
	float max = 0;
	for(int i = 0; i < n; i++) {
		float cur = raw[i];
		if(i == 0 || cur < min) {
			min = cur;
		}
		if(i == 0 || cur > max) {
			max = cur;
		}
	}
	for(int i = 0; i < n; i++) {
		img.getPixels()[i] = ofMap(raw[i], min, max, 0, 255);
	}
	ofLogVerbose() << "min(" << (min * normalization + offsetAmount) * scaleAmount << ") max(" << (max * normalization + offsetAmount) * scaleAmount << ")";
	img.saveImage("normalized.png");

	// inefficient zeroing of edges
	ofLogVerbose() << "zeroing" << endl;
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			if(y == 0 || x == 0 || y + skip >= height - skip || x + skip >= width - skip) {
				set(x, y, 0);
			}
		}
	}
	
	ofLogVerbose() << "mesh";
	mesh.setMode(OF_TRIANGLES_MODE);
	for(int y = 0; y < height - skip; y += skip) {
		for(int x = 0; x < width - skip; x += skip) {
			ofVec3f nw = get(x, y);
			ofVec3f ne = get(x + skip, y);
			ofVec3f sw = get(x, y + skip);
			ofVec3f se = get(x + skip, y + skip);
			
			addFace(nw, ne, se, sw);
			
			lastWidth = x + skip;
		}
		lastHeight = y + skip;
	}
	
	addBack();
	
	vector<ofVec3f>& vertices = mesh.getVertices();
	ofVec3f offset(-width / 2, -height / 2, offsetAmount);
	ofVec3f scaling(scaleAmount, scaleAmount, scaleAmount);
	for(int i = 0; i < vertices.size(); i++) {
		vertices[i] += offset;
		vertices[i] *= scaling;
	}
	
	saveStl(mesh, "output.stl");
}

void Flt::addFace(ofVec3f a, ofVec3f b, ofVec3f c) {
	ofVec3f normal = ((c - a).cross(b - a)).normalize();
	mesh.addNormal(normal);
	mesh.addVertex(a);
	mesh.addNormal(normal);
	mesh.addVertex(b);
	mesh.addNormal(normal);
	mesh.addVertex(c);
}

void Flt::addFace(ofVec3f a, ofVec3f b, ofVec3f c, ofVec3f d) {
	addFace(a, b, d);
	addFace(b, c, d);
}

void Flt::addBack() {
	ofVec3f offset(0, 0, -offsetAmount);
	ofVec3f nw(0, 0, 0);
	ofVec3f ne(lastWidth, 0, 0);
	ofVec3f sw(0, lastHeight, 0);
	ofVec3f se(lastWidth, lastHeight, 0);
	addFace(nw, ne, ne + offset, nw + offset);
	addFace(ne, se, se + offset, ne + offset);
	addFace(sw + offset, se + offset, se, sw);
	addFace(nw + offset, sw + offset, sw, nw);
	addFace(nw + offset, ne + offset, se + offset, sw + offset);
}

ofVec3f Flt::get(int x, int y) {
	int i = y * height + x;
	if(raw[i] < 0) {
		return ofVec3f(x, (height - y), 0);
	} else {
		return ofVec3f(x, (height - y), raw[i] * normalization);
	}
}

void Flt::set(int x, int y, float z) {
	int i = y * height + x;
	raw[i] = z;
}

void Flt::draw() {
	ofPushMatrix();
	mesh.drawFaces();
	ofPopMatrix();
}

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	glEnable(GL_DEPTH_TEST);
	flt.setup("input.flt");
	light.setup();
	light.setDiffuseColor(ofColor(255));
	light.setAmbientColor(ofColor(0));
	light.setPosition(ofGetWidth() / 2, ofGetHeight() / 2, 5000);
}

void testApp::update() {
}

void testApp::draw() {
	glEnable(GL_DEPTH_TEST);
	ofBackground(0);
	cam.begin();
	ofSetColor(255);
	float scale = 1;
	ofScale(scale, scale, scale);
	flt.draw();
	cam.end();
}
