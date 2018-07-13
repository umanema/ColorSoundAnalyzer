#pragma once

#include "ofMain.h"
#include "ofxFft.h"
#include "ofxDatGui.h"
#include "ofxXmlSettings.h"
#include "ofxBiquadFilter.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void audioReceived(float* input, int bufferSize, int nChannels);
		void audioOut(float *input, int bufferSize, int nChannels);
		void update();
		void draw();
		void exit();

		void keyPressed(int key);
		void keyReleased(int key);
		void onDropdownEventSound(ofxDatGuiDropdownEvent e);
		void onDropdownEventSerial(ofxDatGuiDropdownEvent e);
		void barkScale(float* barkArray, float* fftArray, int bufferSize);

		int bufferSize;

		ofxXmlSettings settings;
		ofxDatGuiDropdown* soundDevice;
		ofxDatGuiDropdown* serialDevice;

		ofColor bC;
		ofxDatGuiSlider* slider;
		ofxFft* fft;
		ofSoundStream stream;
		ofSerial serial;

		ofxBiquadFilter1f rmsFilter;
		ofxBiquadFilterColor colorFilter;

		float* audioInput;
		float* fftOutput;
		float* in01;
		float* in02;
		float* bark;
		float* barkSmooth;

		float appWidth;
		float appHeight;

		float rms;
		int numCounted;
		
		float highestPoll;
		float scale;
		int bin;
		int colorPicker;

		int mode;
		
};
