#include "testApp.h"

#define HGT_VOID (-32768) // voids are assigned this value
#define HGT_SRTM_1_SIZE (3601)
#define HGT_SRTM_3_SIZE (1201)

void Hgt::setup(string filename) {
	ofFile file(filename);
	fileBuffer = file.readToBuffer();
	short* binaryBuffer = (short*) fileBuffer.getBinaryBuffer();
	int n = fileBuffer.size() / sizeof(short);
	raw.resize(n);
	for(int i = 0; i < n; i++) {
		raw[i] = swapEndianness(binaryBuffer[i]);
	}
	width = height = sqrt(n);
	
	if(width == HGT_SRTM_1_SIZE) {
		normalization = 1. / 30;
	} else {
		normalization = 1. / 90;
	}
	
	cout << normalization << endl;
	
	mesh.setMode(OF_TRIANGLES_MODE);
	int skip = 1;
	for(int y = 0; y < height - skip; y += skip) {
		for(int x = 0; x < width - skip; x += skip) {
			ofVec3f nw = get(x, y);
			ofVec3f ne = get(x + skip, y);
			ofVec3f sw = get(x, y + skip);
			ofVec3f se = get(x + skip, y + skip);
			
			addTriangle(nw, ne, sw);
			addTriangle(ne, se, sw);
		}
	}
}

void Hgt::addTriangle(ofVec3f& a, ofVec3f& b, ofVec3f& c) {
	ofVec3f normal = (b - a).cross(b - c).normalize();
	mesh.addNormal(normal);
	mesh.addVertex(a);
	mesh.addNormal(normal);
	mesh.addVertex(b);
	mesh.addNormal(normal);
	mesh.addVertex(c);
}

ofVec3f Hgt::get(int x, int y) {
	int i = y * height + x;
	if(raw[i] == HGT_VOID) {
		return ofVec3f(x, height - y, 0);
	} else {
		return ofVec3f(x, height - y, raw[i] * normalization);
	}
}

void Hgt::draw() {
	ofPushMatrix();
	ofTranslate(-width / 2, -height / 2);
	mesh.drawFaces();
	ofPopMatrix();
}

short Hgt::swapEndianness(short x) {
	unsigned char* xp = (unsigned char*) &x;
	return (xp[0] << 8) | xp[1];
}


void testApp::setup() {
	hgt.setup("N35E138.hgt");
	light.setup();
	light.setDiffuseColor(ofColor(255));
	light.setAmbientColor(ofColor(0));
}

void testApp::update() {
}

void testApp::draw() {
	glEnable(GL_DEPTH_TEST);
	ofBackground(0);
	cam.begin();
	ofSetColor(255);
	hgt.draw();
	cam.end();
}
