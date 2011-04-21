#include "testApp.h"

const float minDistance = 50;
const float maxDistance = 115;

const float fovWidth = 1.0144686707507438;
const float fovHeight = 0.78980943449644714;
const float XtoZ = tanf(fovWidth/2)*2;
const float YtoZ = tanf(fovHeight/2)*2;
const unsigned int camWidth = 640;
const unsigned int camHeight = 480;

ofVec3f ConvertProjectiveToRealWorld(float x, float y, float z) {
	return ofVec3f((x/camWidth-.5f) * z * XtoZ,
								 (.5f-y/camHeight) * z * YtoZ,
								 -z);
}

inline void exportPlyVertex(ostream& ply, float x, float y, float z) {
	ply.write(reinterpret_cast<char*>(&x), sizeof(float));
	ply.write(reinterpret_cast<char*>(&y), sizeof(float));
	ply.write(reinterpret_cast<char*>(&z), sizeof(float));
}

int exportPlyVertices(ostream& ply, vector<ofVec3f>& surface) {
	int total = 0;
	int i = 0;
	ofVec3f zero(0, 0, 0);
	for(int i = 0; i < surface.size(); i++) {
		if (surface[i] != zero) {
			exportPlyVertex(ply, surface[i].x, surface[i].y, surface[i].z);
			total++;
		}
	}
	return total;
}


void exportPlyCloud(string filename, vector<ofVec3f>& surface) {
	ofstream ply;
	ply.open(ofToDataPath(filename).c_str(), ios::out | ios::binary);
	if (ply.is_open()) {
		// create all the vertices
		stringstream vertices(ios::in | ios::out | ios::binary);
		int total = exportPlyVertices(vertices, surface);
		
		// write the header
		ply << "ply" << endl;
		ply << "format binary_little_endian 1.0" << endl;
		ply << "element vertex " << total << endl;
		ply << "property float x" << endl;
		ply << "property float y" << endl;
		ply << "property float z" << endl;
		ply << "end_header" << endl;
		
		// write all the vertices
		ply << vertices.rdbuf();
	}
}

void testApp::setup() {
	ofSetVerticalSync(true);
	
	raw.loadImage("five-views.png");
	raw.setImageType(OF_IMAGE_GRAYSCALE);
	
	surface.resize(camWidth * camHeight);
	
	int total = 0;
	
	unsigned char* rawPixels = raw.getPixels();
	int i = 0;
	for(int y = 0; y < camHeight; y++) {
		for(int x = 0; x < camWidth; x++) {
			if(rawPixels[i] != 0) {
				float z = ofMap(rawPixels[i], 255, 0, minDistance, maxDistance);
				surface[i] = ConvertProjectiveToRealWorld(x, y, z);
				avg += surface[i];
				total++;
			}
			i++;
		}
	}
	
	avg /= total;
	
	//exportPlyCloud("mirrored-cloud.ply", surface);
}

void testApp::update() {
}

void testApp::draw() {
	ofBackground(0);
	
	ofSetColor(255);
	
	cam.begin();
	
	ofTranslate(-avg);
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(ofVec3f), &(surface[0].x));
	glDrawArrays(GL_POINTS, 0, surface.size());
	glDisableClientState(GL_VERTEX_ARRAY);
	
	cam.end();
}
