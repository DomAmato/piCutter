#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxSvg.h"

#ifdef TARGET_LINUX
#include <wiringPi.h>
#include <wiringPiSPI.h>
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
*/




#define DIR_PIN 4
#define STEP_PIN 5

#define DIR_PIN2 2
#define STEP_PIN2 3

#define LASER_PIN 17
#define STOP_BUT 18

#define LIMIT_SWITCH 6
#define LIMIT_SWITCH2 1
#define LIMIT_SWITCH3 0
#define LIMIT_SWITCH4 7


#define BED_WIDTH 1080
#define BED_WIDTH_IN 15
#define BED_WIDTH_MM 381
#define BED_HEIGHT 1008
#define BED_HEIGHT_IN 14
#define BED_HEIGHT_MM 355.6

#define PIXELS_PER_INCH BED_WIDTH/BED_WIDTH_IN
#define PIXELS_PER_MM BED_WIDTH/BED_WIDTH_MM

#define in_to_mm(x) x*25.4
#define mm_to_in(x) x/25.4

#define WIDTH_STEPS 37000  //should now be about 45000 and 42000 for height
#define STEPS_PER_PIXEL WIDTH_STEPS/BED_WIDTH
#define STEPS_PER_INCH WIDTH_STEPS/BED_WIDTH_IN
#define STEPS_PER_MM WIDTH_STEPS/BED_WIDTH_MM

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

private:
	void rotateDeg2(float deg, float speed);
	void rotateDeg(float deg, float speed);
	void rotate(float steps, float speed);
	void rotate2(float steps, float speed);
	int lcm(int a, int b);
	int gcd(int a, int b);
	void moveMotors(float step1, float step2, stepTranslation);
	void moveMotors2(int step1, int step2, stepTranslation);

	void parseGCode(string filename);
	ofPoint XYposition(string lines);
	ofPoint IJposition(string lines);
	void home();

	void openFile();

	vector<ofPoint>	mousePoints;
	vector<ofPolyline> mouseDraw;
	vector<ofPolyline> gcodePoly;

	ofPoint laserLocation;
	ofPoint newloc;
	ofPoint oldloc;

	bool laserOn;
	void toggleLaser();

	void printDrawing();
	void printSvg();
	void printGCode();

	bool limitSwitch;
	bool limitSwitch2;
	bool limitSwitch3;
	bool limitSwitch4;

	vector<ofPolyline> outlines;
	vector<vector<ofPoint>> svgPoints;

	bool stop;
	int numStop;
};
