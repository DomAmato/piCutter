#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxSvg.h"
#ifndef TARGET_LINUX
#include "ofxOpenCV.h"
#endif

#ifdef TARGET_LINUX
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "ofxOpenCv.h"
#endif

/*
pin 0 button 7
pin 1 button 6
pin 2 button 9
pin 3 button 10
pin 4 button 11
pin 5 button 12
pin 6 button 5
pin 7 button 8
pins 8+9 are SDA and SCLK
Pins 10-14 are used up by the SPI module
pins 15+16 are the I2C interface
pins 17-20 are on the second breakout
pin 17 button 2
pin 18 button 4
pin 19 button 3
pin 20 com
*/




#define DIR_PIN 3
#define STEP_PIN 2

#define DIR_PIN2 5
#define STEP_PIN2 4

#define LASER_PIN 7
#define STOP_BUT 20

#define LIMIT_SWITCH 0
#define LIMIT_SWITCH2 1
#define LIMIT_SWITCH3 6
#define LIMIT_SWITCH4 18

#define in_to_mm(x) x*25.4
#define mm_to_in(x) x/25.4

#define BED_WIDTH_IN 12.0
#define BED_HEIGHT_IN 9.0

#define BED_WIDTH_MM in_to_mm(BED_WIDTH_IN)
#define BED_HEIGHT_MM in_to_mm(BED_HEIGHT_IN)

#define PIXELS_PER_INCH 600.0
#define PIXELS_PER_MM mm_to_in(PIXELS_PER_INCH)

#define BED_WIDTH BED_WIDTH_IN*PIXELS_PER_INCH
#define BED_HEIGHT BED_HEIGHT_IN*PIXELS_PER_INCH

#define WIDTH_STEPS 15000.0
// currently this translates to about 50 steps per mm and 1250 steps per inch and 2 steps per pixel
#define STEPS_PER_PIXEL WIDTH_STEPS/(BED_WIDTH)
#define STEPS_PER_INCH WIDTH_STEPS/BED_WIDTH_IN
#define STEPS_PER_MM WIDTH_STEPS/(BED_WIDTH_MM)

#define round(x)  floor(x + 0.5)


enum stepTranslation {
	PIXELS,
	INCHES,
	MM
};

class ofApp : public ofBaseApp{

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	/*void testHeight();
	void testWidth();*/

private:
	void rotateDegY(float deg, float speed);
	void rotateDegX(float deg, float speed);
	void rotateX(float steps, float speed);
	void rotateY(float steps, float speed);
	int lcm(int a, int b);
	int gcd(int a, int b);
	void moveMotors(float step1, float step2, stepTranslation);
	void moveMotors2(int step1, int step2, stepTranslation);

	void parseGCode(string filename);
	ofPoint XYposition(string lines);
	ofPoint IJposition(string lines);
	void home();

	void openFile();

	ofImage loadedImage;
	ofxCvColorImage intermediaryImage;
	ofxCvGrayscaleImage processedImage;

	vector<ofPoint>	mousePoints;
	vector<ofPolyline> mouseDraw;
	vector<ofPolyline> gcodePoly;

	ofPoint actualLaserLocation;
	ofPoint laserLocation;
	ofPoint newloc;
	ofPoint oldloc;

	bool laserOn;
	void toggleLaser();

	void printDrawing();
	void printSvg();
	bool svgLoaded;
	void printGCode();
	bool GCodeLoaded;
	void printImage();
	bool imageLoaded;

	bool moveItem;
	ofPoint movLoc;
	ofPoint imgOffset;

	void passesChanged(int & numPasses);
	void laserPowerChanged(float & lPow);

	bool limitSwitch;
	bool limitSwitch2;
	bool limitSwitch3;
	bool limitSwitch4;

	vector<ofPolyline> outlines;
	vector<vector<ofPoint> > svgPoints;

	int numStop;

	ofxFloatSlider speed;
	ofxFloatSlider threshold;
	ofxFloatSlider laserPower;
	ofxIntSlider pass;
	ofxPanel gui;
	int passes;


#ifndef TARGET_LINUX
#define LOW ARD_LOW
#define HIGH ARD_HIGH

	ofSerial	serial;
	ofArduino	ard;

	bool		bSetupArduino;	
	bool firstRun;
	bool arduinoAttached;
	

	void setupArduino(const int & version);
	void digitalPinChanged(const int & pinNum);
	void analogPinChanged(const int & pinNum);
	void updateArduino();

	int digitalRead(int pin);
	void digitalWrite(int pin, int value);
#endif
};
