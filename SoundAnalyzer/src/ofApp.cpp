#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowShape(640, 360);
	ofSetFrameRate(60);
	bC = ofColor(0);
	ofBackground(bC);
	settings.loadFile("settings.xml");

	rms = 0.0;
	
	rmsFilter.setFc(0.05);
	colorFilter.setFc(0.08);

	//set sound devices menu
	vector<ofSoundDevice> soundDevices = stream.getDeviceList();
	vector<string> soundDevicesLabels (soundDevices.size());
	
	for (int i = 0; i < soundDevices.size(); i++) {
		soundDevicesLabels[i] = soundDevices[i].name;
	}

	soundDevice = new ofxDatGuiDropdown("SOUND DEVICE", soundDevicesLabels);
	soundDevice->setPosition(0, 0);
	soundDevice->setWidth(ofGetWindowWidth() / 2);
	soundDevice->onDropdownEvent(this, &ofApp::onDropdownEventSound);

	//set serial devices menu
	vector<ofSerialDeviceInfo> serialDevices = serial.getDeviceList();
	vector<string> serialDevicesLabels;
	if (serialDevices.size()) {
		serialDevicesLabels.resize(serialDevices.size());
		for (int i = 0; i < serialDevices.size(); i++) {
			serialDevicesLabels[i] = serialDevices[i].getDeviceName();
		}
	}
	else {
		serialDevicesLabels.resize(1);
		serialDevicesLabels[0] = "UNAVAILABLE";
	}

	serialDevice = new ofxDatGuiDropdown("SERIAL DEVICE", serialDevicesLabels);
	serialDevice->setPosition(ofGetWindowWidth() / 2, 0);
	serialDevice->setWidth(ofGetWindowWidth()/2);
	serialDevice->onDropdownEvent(this, &ofApp::onDropdownEventSerial);

	scale = 0;

	bufferSize = 1024;

	highestPoll = 0;
	colorPicker = 1;

	fft = ofxFft::create(bufferSize, OF_FFT_WINDOW_HAMMING);

	audioInput = new float[bufferSize];
	fftOutput = new float[fft->getBinSize()];

	in01 = new float[bufferSize];
	in02 = new float[bufferSize];
	bark = new float[24];
	barkSmooth = new float[24];
	for (int i = 0; i < 24; i++) {
		bark[i] = 0;
	}
	for (int i = 0; i < 24; i++) {
		barkSmooth[i] = 0;
	}

	appWidth = ofGetWindowWidth();
	appHeight = ofGetWindowHeight();
	
	stream.setDeviceID(settings.getValue("settings:soundDeviceID", 0));
	stream.setup(this, 2, 2, 44100, bufferSize, 256);

	serial.setup(settings.getValue("settings:serialDeviceID", 0), 115200);
}

//--------------------------------------------------------------
void ofApp::update(){
	//update menus
	soundDevice->update();
	serialDevice->update();

	appWidth = ofGetWindowWidth();
	appHeight = ofGetWindowHeight();
	
	barkScale(bark, fftOutput, bufferSize);
	highestPoll = 0;
	colorPicker = 1;
	for (int i = 0; i < 24; i++) {
		if (highestPoll < bark[i]) {
			colorPicker = i + 1;
			highestPoll = bark[i];
		}
	}
	
	rmsFilter.update(rms);
	bC.setHsb(255 / 24 * colorPicker, 255 * highestPoll * 100, 255 * rmsFilter.value());
	colorFilter.update(bC);
	bC = colorFilter.value();

	if (serial.isInitialized()) {
		serial.writeByte(bC.r + 1);
		serial.writeByte(bC.g + 1);
		serial.writeByte(bC.b + 1);
		
		serial.writeByte(0);
	}

	ofBackground(bC);
}

//--------------------------------------------------------------
void ofApp::draw() {
	//draw menus
	soundDevice->draw();
	serialDevice->draw();

	ofSetHexColor(0xffffff);
	ofPushMatrix();
	
	//draw bark scale
	for (int i = 0; i < 24; i++) {
		barkSmooth[i] *= 0.97;
		barkSmooth[i] = max(bark[i], barkSmooth[i]);
	}

	ofSetColor(255, 255, 255);
	for (int i = 0; i < 24; i++) {
		if (i == 500 || i == 0.1) {
			ofSetColor(255, 255, 255);
		}
		else {
			ofSetColor(128, 128, 128);
		}
		ofRect(i * (appWidth / 24), appHeight, 20, -barkSmooth[i] * 2000);
	}
}

//--------------------------------------------------------------
void ofApp::audioReceived(float* input, int bufferSize, int nChannels) {
	for (int i = 0; i < bufferSize; i++) {
		in01[i] = input[i * 2];
		in02[i] = input[i * 2 + 1];
	}

	for (int i = 0; i < bufferSize; i++)
		audioInput[i] = (input[i * 2] + input[2 * i + 1]) / 2;

	fft->setSignal(audioInput);
	memcpy(fftOutput, fft->getAmplitude(), sizeof(float) * fft->getBinSize());

	//rms
	numCounted = 0;

	for (int i = 0; i < bufferSize; i++) {
		float leftSample = input[i * 2] * 0.5;
		float rightSample = input[i * 2 + 1] * 0.5;

		rms += leftSample * leftSample;
		rms += rightSample * rightSample;
		numCounted += 2;
	}

	rms /= (float)numCounted;
	rms = sqrt(rms);
}

//--------------------------------------------------------------
void ofApp::audioOut(float* output, int bufferSize, int nChannels) {
	for (int i = 0; i < bufferSize; i++) {
		output[2 * i] = in01[i];
		output[2 * i + 1] = in02[i];
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::exit() {
	settings.saveFile("settings.xml");
	stream.stop();
	stream.close();
	if (serial.isInitialized()) {
		serial.writeByte(1);
		serial.writeByte(1);
		serial.writeByte(1);
		serial.writeByte(0);
	}
}

//--------------------------------------------------------------
void ofApp::onDropdownEventSound(ofxDatGuiDropdownEvent e) {
	stream.stop();
	stream.close();
	stream.setDeviceID(e.child);
	stream.setup(this, 2, 2, 44100, bufferSize, 256);
	stream.start();
	settings.setValue("settings:soundDeviceID", e.child);
}

//--------------------------------------------------------------
void ofApp::onDropdownEventSerial(ofxDatGuiDropdownEvent e) {
	if (serial.getDeviceList().size() > 0) {
		serial.setup(e.child, 115200);
		settings.setValue("settings::serialDeviceID", e.child);
	}
}
//--------------------------------------------------------------
//use this function to convert linear fft scale to bark scale
void ofApp::barkScale(float* barkArray, float* fftArray, int bufferSize) {
	//bark scale has only 24 poles
	vector<int> f = {0, 100, 200, 300, 400, 510, 630, 770, 920,
					1080, 1270, 1480, 1720, 2000, 2320, 2700, 3150,
					3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500};

	int c = 0;
	for (int b = 1; b < 25; b++) {
		for (int i = 0; i < bufferSize; i++) {
			if ((f[b - 1] < (i * 46.875)) && ((i * 46.875) < f[b])) {
				barkArray[b - 1] += fftArray[i];
				c += 1;
			}
		}
		barkArray[b - 1] /= c;
		//make each pole even
		barkArray[b - 1] = barkArray[b - 1] * (float(c) / (float(bufferSize) / 100));
		c = 0;
	}
	barkArray[0] *= 0.75;
	barkArray[1] *= 0.75;
	
}
