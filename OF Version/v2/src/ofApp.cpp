#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){

#ifdef TARGET_LINUX
	if(wiringPiSetup()==-1)
		ofLogError("Main") << "Wiring Pi setup failed";
	pinMode(LIMIT_SWITCH, INPUT);
	pinMode(LIMIT_SWITCH2, INPUT);
	pinMode(LIMIT_SWITCH3, INPUT);
	pinMode(LIMIT_SWITCH4, INPUT);
	pinMode(STOP_BUT, INPUT);

	pullUpDnControl(LIMIT_SWITCH, PUD_UP);
	pullUpDnControl(LIMIT_SWITCH2, PUD_UP);
	pullUpDnControl(LIMIT_SWITCH3, PUD_UP);
	pullUpDnControl(LIMIT_SWITCH4, PUD_UP);
	pullUpDnControl(STOP_BUT, PUD_UP);

	pinMode(DIR_PIN, OUTPUT);
	pinMode(STEP_PIN, OUTPUT);
	pinMode(DIR_PIN2, OUTPUT);
	pinMode(STEP_PIN2, OUTPUT);
	pinMode(LASER_PIN, OUTPUT);

	wiringPiSPISetup(0, 10000000);
	unsigned char buffer[2];
	buffer[0]= 0xB0000;
	buffer[1]= 255;
	wiringPiSPIDataRW(0, buffer, 2);
#else
	cout << STEPS_PER_PIXEL << ", " << STEPS_PER_MM << ", " << STEPS_PER_INCH << endl;
	cout << BED_WIDTH << ", " << BED_HEIGHT << ", " << BED_WIDTH_MM << ", " << BED_HEIGHT_MM << endl;
	arduinoAttached=false;
	vector<string> devices;
	vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();
	vector<string> temp = serial.getDeviceFriendlyNames();
	for(int i=0;i<temp.size();i++){
		if(strstr(temp[i].c_str(), "Serial") != NULL){
			cout << "Setting up firmata on " << deviceList[i].getDeviceName() << endl;
			arduinoAttached = ard.connect(deviceList[i].getDeviceName(), 125000); //teensy's friendly name is USB Serial
		} 
	}
	ofAddListener(ard.EInitialized, this, &ofApp::setupArduino);
	bSetupArduino=false;
#endif

	ofSetVerticalSync(true);

	ofBackground(80);

	laserLocation.set(0,0);
	actualLaserLocation.set(0,0);
	newloc.set(0,0);
	oldloc.set(0,0);
	imgOffset.set(0,0);
	movLoc.set(50*PIXELS_PER_INCH/72.0,30*PIXELS_PER_INCH/72.0);

	laserOn = false;

	numStop=0;

	limitSwitch=false;
	limitSwitch2=false;
	limitSwitch3=false;
	limitSwitch4=false;

	svgLoaded=false;
	GCodeLoaded=false;
	imageLoaded=false;
	moveItem=false;

	pass.addListener(this, &ofApp::passesChanged);
	laserPower.addListener(this, &ofApp::laserPowerChanged);
	passes=1;

	gui.setup("Parameters", "", BED_WIDTH_IN*72-170, BED_HEIGHT_IN*72-60); 
	gui.add(speed.setup( "Speed", .2, .005, 1));
	gui.add(laserPower.setup("Laser Power", 1, 0, 1));
	gui.add(pass.setup("Passes", 1, 1, 10));
	gui.add(threshold.setup("Threshold", 0, 0, 255));
}

//--------------------------------------------------------------
void ofApp::update(){
#ifndef TARGET_LINUX
	if(arduinoAttached){
		updateArduino();
		if (ard.isArduinoReady() && !bSetupArduino){

			cout << "setting up arduino" << endl;
			setupArduino(0);

		}
	}
#endif
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofPushMatrix();
	ofSetColor(255);
	ofRect(50,30, BED_WIDTH_IN*72, BED_HEIGHT_IN*72);
	ofSetColor(0);
	ofSetLineWidth(1);
	for(int i=0;i<BED_WIDTH_IN;i++){
		ofLine(50+i*72, 30, 50+i*72, 45);
		ofLine(86+i*72, 30, 86+i*72, 40);
	}
	for(int i=0;i<BED_HEIGHT_IN;i++){
		ofLine(50, 30+i*72, 65, 30+i*72);
		ofLine(50, 66+i*72, 60, 66+i*72);
	}
	ofTranslate(50,30);
	ofScale(72.0/PIXELS_PER_INCH, 72.0/PIXELS_PER_INCH);

	//we need to scale everything because the DPI is set to 600 for accuracy purposes 

	for(int i=0;i<outlines.size();i++){
		ofSetColor(ofColor::green);
		ofEllipse(svgPoints[i][0], 5,5);
		ofSetColor(ofColor::blue);
		for(int j=1; j<svgPoints[i].size();j++){
			ofEllipse(svgPoints[i][j], 2,2);
		}
		ofSetColor(0);
		outlines[i].draw();
	}

	if(loadedImage.isAllocated()){
		loadedImage.draw(loadedImage.getWidth(),0);
		//intermediaryImage.draw(0,loadedImage.getHeight());
	}
	ofSetColor(255);
	if(processedImage.bAllocated)
		processedImage.draw(imgOffset);
	ofSetColor(0);
	ofSetLineWidth(4);
	for(int j=0;j<mouseDraw.size();j++){
		mouseDraw[j].getSmoothed(3).draw();
	}
	for(int j=0;j<gcodePoly.size();j++){
		gcodePoly[j].draw();
	}
	for(int i=0;i<mousePoints.size();i++)
	{
		ofEllipse(mousePoints[i].x, mousePoints[i].y, 3,3);
	}
	ofTranslate(-50, -30);
	ofScale(PIXELS_PER_INCH/72.0, PIXELS_PER_INCH/72.0);
	ofPopMatrix();
	gui.draw();


}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key)
	{
	case OF_KEY_LEFT:
		rotateX(50, 1);
		break;
	case OF_KEY_RIGHT:
		rotateX(-50, 1);
		break;
	case OF_KEY_UP:
		rotateY(-50, 1);
		break;
	case OF_KEY_DOWN:
		rotateY(50, 1);
		break;
	case 'h':
		home();
		break;
	case ' ':
		digitalWrite(LASER_PIN, HIGH);
		break;
	case 'o':
		openFile();
		break;
	case 'p':
		printDrawing();
		break;
	case 'w':
		printSvg();
		break;
	case 'g':
		printGCode();
		break;
	case 'i':
		printImage();
		break;
	case 'm':
		moveItem=!moveItem;
		break;
	case 'z':
		laserLocation.set(0,0);
		actualLaserLocation.set(0,0);
		break;
	case 'q':
		mouseDraw.clear();
		outlines.clear();
		gcodePoly.clear();
		processedImage.clear();
		svgLoaded=false;
		GCodeLoaded=false;
		imageLoaded=false;
		imgOffset.set(0,0);
		movLoc.set(50*PIXELS_PER_INCH/72.0,30*PIXELS_PER_INCH/72.0);
		break;
	case OF_KEY_BACKSPACE:
		if(mouseDraw.size()>0)
			mouseDraw.erase(mouseDraw.begin()+mouseDraw.size()-1);
		break;
	case 'f':
		ofToggleFullscreen();
		break;
		/*case 't':
		testWidth();
		testHeight();
		break;*/
	default:
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	if(key==' ')
		digitalWrite(LASER_PIN, LOW);
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	if(moveItem){
		if(imageLoaded){
			imgOffset.set(x*PIXELS_PER_INCH/72.0-50, y*PIXELS_PER_INCH/72.0-30);
		} else if(svgLoaded){

			float xoffset=x*PIXELS_PER_INCH/72.0-movLoc.x;
			float yoffset=y*PIXELS_PER_INCH/72.0-movLoc.y;
			movLoc.x+=xoffset;
			movLoc.y+=yoffset;
			// iterate over it again and shift everthing over by the offset amount
			for(int i=0;i<svgPoints.size();i++){
				for(int j=0;j<svgPoints[i].size();j++){
					svgPoints[i][j].x+=xoffset;
					svgPoints[i][j].y+=yoffset;
				}
			}
			//do the same for the lines
			outlines.clear();
			for(int i=0;i<svgPoints.size();i++){
				ofPolyline temp;
				for(int j=0;j<svgPoints[i].size();j++){
					temp.addVertex(svgPoints[i][j]);
				}
				outlines.push_back(temp);
			}
		} else if (GCodeLoaded) {
			float xoffset=x*PIXELS_PER_INCH/72.0-movLoc.x;
			float yoffset=y*PIXELS_PER_INCH/72.0-movLoc.y;
			movLoc.x+=xoffset;
			movLoc.y+=yoffset;
			// iterate over it again and shift everthing over by the offset amount
			for(int i=0;i<gcodePoly.size();i++){
				for(int j=0;j<gcodePoly[i].size();j++){
					gcodePoly[i][j].x+=xoffset;
					gcodePoly[i][j].y+=yoffset;
				}
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	if(!moveItem){
		if(x>50 && x<BED_WIDTH+50 && y>30 && y<BED_HEIGHT+30)
			mousePoints.push_back(ofPoint((x-50)*PIXELS_PER_INCH/72.0,(y-30)*PIXELS_PER_INCH/72.0));
	}
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	if(!moveItem){
		if(x>50 && x<BED_WIDTH+50 && y>30 && y<BED_HEIGHT+30)
			mousePoints.push_back(ofPoint((x-50)*PIXELS_PER_INCH/72.0,(y-30)*PIXELS_PER_INCH/72.0));
	} else {
		if(imageLoaded){
			imgOffset.set(x*PIXELS_PER_INCH/72.0-50, y*PIXELS_PER_INCH/72.0-30);
		} else if(svgLoaded){
			float xoffset=x*PIXELS_PER_INCH/72.0-movLoc.x;
			float yoffset=y*PIXELS_PER_INCH/72.0-movLoc.y;
			movLoc.x+=xoffset;
			movLoc.y+=yoffset;

			// iterate over it again and shift everthing over by the offset amount
			for(int i=0;i<svgPoints.size();i++){
				for(int j=0;j<svgPoints[i].size();j++){
					svgPoints[i][j].x+=xoffset;
					svgPoints[i][j].y+=yoffset;
				}
			}
			//do the same for the lines
			outlines.clear();
			for(int i=0;i<svgPoints.size();i++){
				ofPolyline temp;
				for(int j=0;j<svgPoints[i].size();j++){
					temp.addVertex(svgPoints[i][j]);
				}
				outlines.push_back(temp);
			}
		} else if (GCodeLoaded) {
			float xoffset=x*PIXELS_PER_INCH/72.0-movLoc.x;
			float yoffset=y*PIXELS_PER_INCH/72.0-movLoc.y;
			movLoc.x+=xoffset;
			movLoc.y+=yoffset;
			// iterate over it again and shift everthing over by the offset amount
			for(int i=0;i<gcodePoly.size();i++){
				for(int j=0;j<gcodePoly[i].size();j++){
					gcodePoly[i][j].x+=xoffset;
					gcodePoly[i][j].y+=yoffset;
				}
			}
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	if(moveItem) {
		moveItem=false;
	} else {
		if(x>50 && x<BED_WIDTH+50 && y>30 && y<BED_HEIGHT+30)
			mousePoints.push_back(ofPoint((x-50)*PIXELS_PER_INCH/72.0,(y-30)*PIXELS_PER_INCH/72.0));
		if(mousePoints.size()>0){
			oldloc=newloc;
			newloc = mousePoints[0];
			ofPolyline temp;
			for(int i=0;i<mousePoints.size();i++)
			{
				temp.addVertex(mousePoints[i].x, mousePoints[i].y);
			}
			mouseDraw.push_back(temp);
			mousePoints.clear();
		}
	}
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
void ofApp::rotateX(float steps, float speed){
	//rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
	//speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
	if(!(steps<0 && digitalRead(LIMIT_SWITCH2)==LOW)  && !(steps>0 && digitalRead(LIMIT_SWITCH)==LOW)){
		int dir = (steps > 0)? HIGH:LOW;
		int isteps = round(abs(steps));

		digitalWrite(DIR_PIN,dir);
		float usDelay = (1/speed) * 100;

		actualLaserLocation.x+=isteps*((dir==LOW)?-1:1);
		for(int i=0; i < isteps; i++){
			digitalWrite(STEP_PIN, HIGH);
#ifdef TARGET_LINUX
			delayMicroseconds(usDelay);
#endif

			digitalWrite(STEP_PIN, LOW);
#ifdef TARGET_LINUX
			delayMicroseconds(usDelay);
#endif
		}
	}
}

void ofApp::rotateDegX(float deg, float speed){
	//rotate a specific number of degrees (negitive for reverse movement)
	//speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
	if(!(deg<0 && digitalRead(LIMIT_SWITCH2)==LOW ) && !(deg>0 && digitalRead(LIMIT_SWITCH)==LOW )){
		int dir = (deg > 0)? HIGH:LOW;
		digitalWrite(DIR_PIN,dir);

		int steps = round(abs(deg)*(1/0.225));
		float usDelay = (1/speed) * 100;
		actualLaserLocation.x+=steps*((dir==LOW)?-1:1);
		for(int i=0; i < steps; i++){
			digitalWrite(STEP_PIN, HIGH);
#ifdef TARGET_LINUX
			delayMicroseconds(usDelay);
#endif
			digitalWrite(STEP_PIN, LOW);
#ifdef TARGET_LINUX
			delayMicroseconds(usDelay);
#endif
		}
	}
}

void ofApp::rotateY(float steps, float speed){
	//rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
	//speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
	if(!(steps<0 && digitalRead(LIMIT_SWITCH3)==LOW) && !(steps>0 && digitalRead(LIMIT_SWITCH4)==LOW )){
		int dir = (steps > 0)? HIGH:LOW;
		int isteps = round(abs(steps));

		digitalWrite(DIR_PIN2,dir);
		actualLaserLocation.y+=isteps*((dir==LOW)?-1:1);
		float usDelay = (1/speed) * 100;
		for(int i=0; i < isteps; i++){
			digitalWrite(STEP_PIN2, HIGH);
#ifdef TARGET_LINUX
			delayMicroseconds(usDelay);
#endif
			digitalWrite(STEP_PIN2, LOW);
#ifdef TARGET_LINUX
			delayMicroseconds(usDelay);
#endif
		}
	}
}

void ofApp::rotateDegY(float deg, float speed){
	//rotate a specific number of degrees (negitive for reverse movement)
	//speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
	if(!(deg<0 && digitalRead(LIMIT_SWITCH3)==LOW ) && !(deg>0 && digitalRead(LIMIT_SWITCH4)==LOW )){
		int dir = (deg > 0)? HIGH:LOW;
		digitalWrite(DIR_PIN2,dir);

		int steps = round(abs(deg)*(1/0.225));
		float usDelay = (1/speed) * 100;
		actualLaserLocation.x+=steps*((dir==LOW)?-1:1);
		for(int i=0; i < steps; i++){
			digitalWrite(STEP_PIN2, HIGH);
#ifdef TARGET_LINUX
			delayMicroseconds(usDelay);
#endif

			digitalWrite(STEP_PIN2, LOW);
#ifdef TARGET_LINUX
			delayMicroseconds(usDelay);
#endif
		}
	}
}


void ofApp::toggleLaser(){
	if(laserOn)
		digitalWrite(LASER_PIN, HIGH);
	else
		digitalWrite(LASER_PIN, LOW);
	laserOn = !laserOn;
}
int ofApp::lcm(int a, int b)
{
	return (a*b)/gcd(a,b);
}
int ofApp::gcd(int a, int b)
{
	if (b == 0)
		return a;
	else
		return gcd(b, a%b);
}
void ofApp::moveMotors(float stepf1, float stepf2, stepTranslation type){
	switch (type)
	{
	case PIXELS:
		stepf1*=STEPS_PER_PIXEL;
		stepf2*=STEPS_PER_PIXEL;
		break;
	case INCHES:
		stepf1*=STEPS_PER_INCH;
		stepf2*=STEPS_PER_INCH;
		break;
	case MM:
		stepf1*=STEPS_PER_MM;
		stepf2*=STEPS_PER_MM;
		break;
	default:
		break;
	}
	//determine direction by sign
	int dir1 = stepf1>=0?1:-1;
	int dir2 = stepf2>=0?1:-1;
	//there are no negative steps we need to work in the positive realm
	stepf1=abs(stepf1);
	stepf2=abs(stepf2);
	//no matter how small the step is unless it is explicitly 0 it needs to move
	int step1=ceil(stepf1);
	int step2=ceil(stepf2);

	int total_micro_step=0;
	int micro_step1=0;
	int micro_step2=0;

	if (step1==0){ //no x movment
		total_micro_step=step2;
		micro_step2=1;
		micro_step1=step2+100;  //set [micro_step1]>[total_micro_step], so stepper motor will not turn
	} else if (step2==0){ //no y movement
		total_micro_step=step1;
		micro_step1=1;
		micro_step2=step1+100;
	} else{ //we need to interpolate diagonal movement
		total_micro_step=lcm(step1,step2); //find the least common multiple
		micro_step1= total_micro_step/step1; //ends up as divind by zero on really small increments
		micro_step2=total_micro_step/step2;
	}

	for (int i=1; i<total_micro_step+1;i++){    //i is the iterator for the micro_step. i cannot start from 0
		if ((i % micro_step1)==0) //motor 1 need to turn one step
			rotateX(dir1,speed);

		if ((i % micro_step2)==0) //motor 2 need to turn one step
			rotateY(dir2,speed);
	}
}
void ofApp::moveMotors2(int step1, int step2, stepTranslation type){
	switch (type)
	{
	case PIXELS:
		step1*=STEPS_PER_PIXEL;
		step2*=STEPS_PER_PIXEL;
		break;
	case INCHES:
		step1*=STEPS_PER_INCH;
		step2*=STEPS_PER_INCH;
		break;
	case MM:
		step1*=STEPS_PER_MM;
		step2*=STEPS_PER_MM;
		break;
	default:
		break;
	}
	int dir1 = step1>=0?1:-1;
	int dir2 = step2>=0?1:-1;
	step1=abs(step1);
	step2=abs(step2);
	int total_micro_step=0;
	int micro_step1=0;
	int micro_step2=0;
	if (step1==0){
		total_micro_step=step2;
		micro_step2=1;
		micro_step1=step2+100;  //set [micro_step1]>[total_micro_step], so stepper motor will not turn
	} else if (step2==0){
		total_micro_step=step1;
		micro_step1=1;
		micro_step2=step1+100;
	} else{
		total_micro_step=lcm(step1,step2);
		micro_step1=total_micro_step/step1;
		micro_step2=total_micro_step/step2;
	}

	for (int i=1; i<total_micro_step+1;i++){    //i is the iterator for the micro_step. i cannot start from 0
		if ((i % micro_step1)==0) //motor 1 need to turn one step
			rotateX(dir1,1);

		if ((i % micro_step2)==0) //motor 2 need to turn one step
			rotateY(dir2,1);
	}
}
void ofApp::parseGCode(string filename){
	float old_x_pos=0;
	float x_pos=0;
	float old_y_pos = 0;
	float y_pos = 0;
	float i_pos=0;
	float j_pos=0;
	ofBuffer buffer = ofBufferFromFile(filename);
	string line = buffer.getFirstLine(); //file name of the G code commands
	gcodePoly.clear();
	ofPolyline tempPolyline;
	bool inch = false;
	bool mm = true;
	while(!buffer.isLastLine()){

		if (line.find("G90") != -1)
			printf("started parsing gcode");

		if (line.find("G20") != -1){// working in inch;
			ofLogNotice("G-Code") << "Working in Inches";
			inch=true;
			mm = false;
		}

		if (line.find("G21") != -1)// working in mm;
		{
			ofLogNotice("G-Code") << "Working in Millimeters";
			inch=false;
			mm = true;
		}

		if (line.find("M05") != -1){}
		if (line.find("M03") != -1){}
		if (line.find("M02") != -1){}
		if (line.find("G1F") != -1 || line.find("G1 F") != -1){}
		if (line.find("Z")){
			int zchar_loc=line.find('Z');
			int i=zchar_loc+1;
			while(line[i]>47 && line[i]<58 || line[i]=='-' || line[i]=='.'){
				i++;
			}
			float z_pos=ofToFloat(line.substr(zchar_loc+1, i-zchar_loc));
			ofClamp(z_pos,0,1);
		}
		if (line.find("G00") != -1 || line.find("G0 ") != -1 || line.find("G1 ") != -1 || line.find("G01") != -1){
			if (line.find("X") != -1 && line.find("Y") != -1){ //Ignore line not dealing with XY plane
				//linear engraving movement
				if (line.find("G00") != -1 || line.find("G0 ") != -1 ){
					if(tempPolyline.size()>0)
						gcodePoly.push_back(tempPolyline);
					tempPolyline.clear();//make new polyline
				}

				ofPoint xy = XYposition(line);
				if(inch){
					x_pos = PIXELS_PER_INCH*xy.x;
					y_pos = PIXELS_PER_INCH*xy.y;
				} else {
					x_pos = PIXELS_PER_MM*xy.x;
					y_pos = PIXELS_PER_MM*xy.y;
				}
				tempPolyline.addVertex(x_pos, y_pos); //add vertex. if g0 start new polyline
			}
		}

		if (line.find("G02") != -1 || line.find("G03") != -1){ //circular interpolation
			if (line.find("X") != -1 && line.find("Y") != -1 && line.find("I") != -1 && line.find("J") != -1){
				old_x_pos=x_pos;
				old_y_pos=y_pos;

				ofPoint xy = XYposition(line);
				ofPoint ij = IJposition(line);
				if(inch){
					x_pos = PIXELS_PER_INCH*xy.x;
					y_pos = PIXELS_PER_INCH*xy.y;
					i_pos = PIXELS_PER_INCH*ij.x;
					j_pos = PIXELS_PER_INCH*ij.y;
				} else {
					x_pos = PIXELS_PER_MM*xy.x;
					y_pos = PIXELS_PER_MM*xy.y;
					i_pos = PIXELS_PER_MM*ij.x;
					j_pos = PIXELS_PER_MM*ij.y;
				}
				float xcenter=old_x_pos+ij.x;   //center of the circle for interpolation
				float ycenter=old_y_pos+ij.y;


				float Dx=x_pos-xcenter;
				float Dy=y_pos-ycenter;     //vector [Dx,Dy] points from the circle center to the new position

				float r=sqrt(pow(i_pos,2)+pow(j_pos,2));   // radius of the circle

				ofPoint e1 = ofPoint(-i_pos,-j_pos); //pointing from center to current position
				ofPoint e2;
				if (line.find("G02") != -1) //clockwise
					e2 =ofPoint(e1[1],-e1[0]);      //perpendicular to e1. e2 and e1 forms x-y system (clockwise)
				else                   //counterclockwise
					e2= ofPoint(-e1[1],e1[0]);      //perpendicular to e1. e1 and e2 forms x-y system (counterclockwise)

				//[Dx,Dy]=e1*cos(theta)+e2*sin(theta), theta is the open angle

				float costheta=(Dx*e1[0]+Dy*e1[1])/pow(r,2);
				float sintheta=(Dx*e2[0]+Dy*e2[1])/pow(r,2);        //theta is the angule spanned by the circular interpolation curve

				if (costheta>1)  // there will always be some numerical errors! Make sure abs(costheta)<=1
					costheta=1;
				else if (costheta<-1)
					costheta=-1;

				float theta=acos(costheta);
				if (sintheta<0)
					theta=2.0*PI-theta;

				int no_step=int(round(r*theta/5.0));   // number of point for the circular interpolation

				for (int i=1; i<no_step+1;i++){
					float tmp_theta=i*theta/no_step;
					float tmp_x_pos=xcenter+e1[0]*cos(tmp_theta)+e2[0]*sin(tmp_theta);
					float tmp_y_pos=ycenter+e1[1]*cos(tmp_theta)+e2[1]*sin(tmp_theta);
					tempPolyline.addVertex(tmp_x_pos, tmp_y_pos); //add vertex to polyline

				}
			}
		}
		line = buffer.getNextLine();

	}
	//we need to check for any x/y offset into the negative area so iterate over the polylines
	float xoffset=0;
	float yoffset=0;
	for(int i=0;i<gcodePoly.size();i++){
		for(int j=0;j<gcodePoly[i].size();j++){
			gcodePoly[i][j].x<xoffset?xoffset=gcodePoly[i][j].x:1;
			gcodePoly[i][j].y<yoffset?yoffset=gcodePoly[i][j].y:1;
		}
	}
	xoffset=abs(xoffset-5);
	yoffset=abs(yoffset-5);
	// iterate over it again and shift everthing over by the offset amount
	for(int i=0;i<gcodePoly.size();i++){
		for(int j=0;j<gcodePoly[i].size();j++){
			gcodePoly[i][j].x+=xoffset;
			gcodePoly[i][j].y+=yoffset;
		}
	}
}
ofPoint ofApp::XYposition(string lines){
	//given a movement command line, return the X Y position
	int xchar_loc=lines.find('X');
	int i=xchar_loc+1;
	while(lines[i]>47 && lines[i]<58 || lines[i]=='-' || lines[i]=='.'){
		i++;
	}
	float x_pos=ofToFloat(lines.substr(xchar_loc+1, i-xchar_loc));

	int ychar_loc=lines.find('Y');
	i=ychar_loc+1;
	while(lines[i]>47 && lines[i]<58 || lines[i]=='-' || lines[i]=='.'){
		i++;
	}
	float y_pos=ofToFloat(lines.substr(ychar_loc+1, i-ychar_loc));

	return ofPoint(x_pos, y_pos);
}
ofPoint ofApp::IJposition(string lines){
	//given a G02 or G03 movement command line, return the I J position
	int ichar_loc=lines.find('I');
	int i=ichar_loc+1;
	while(lines[i]>47 && lines[i]<58 || lines[i]=='-' || lines[i]=='.'){
		i++;
	}
	float i_pos=ofToFloat(lines.substr(ichar_loc+1, i-ichar_loc));

	int jchar_loc=lines.find('J');
	i=jchar_loc+1;
	while(lines[i]>47 && lines[i]<58 || lines[i]=='-' || lines[i]=='.'){
		i++;
	}
	float j_pos=ofToFloat(lines.substr(jchar_loc+1, i-jchar_loc));

	return ofPoint(i_pos, j_pos);
}
void ofApp::openFile()
{
	ofFileDialogResult dialog;
#ifdef TARGET_LINUX
	dialog = ofSystemLoadDialog("Open FIle", false, "/home/pi/Desktop");
#else
	dialog = ofSystemLoadDialog("Open FIle");
#endif
	if(dialog.getPath() != ""){
		string fileName = dialog.getPath();
		string fileExt = fileName.substr(fileName.find_last_of('.'), fileName.length()-fileName.find_last_of('.'));
		if(fileExt.compare(".svg")==0){
			svgLoaded=true;
			ofxSVG tempSvg;
			tempSvg.load(fileName);
			outlines.clear();
			svgPoints.clear();
			for (int i = 0; i < tempSvg.getNumPath(); i++){
				ofPath p = tempSvg.getPathAt(i);
				//svg defaults to non zero winding which doesn't look so good as contours
				p.setPolyWindingMode(OF_POLY_WINDING_ODD);
				vector<ofPolyline>& lines = p.getOutline();
				for(int j=0;j<(int)lines.size();j++){
					outlines.push_back(lines[j].getResampledBySpacing(1));
				}

			}
			for(int i=0;i<outlines.size();i++){
				svgPoints.push_back(outlines[i].getVertices());
			}
			float xoffset=10000;
			float yoffset=10000;
			for(int i=0;i<svgPoints.size();i++){
				for(int j=0;j<svgPoints[i].size();j++){
					svgPoints[i][j].x<xoffset?xoffset=svgPoints[i][j].x:1;
					svgPoints[i][j].y<yoffset?yoffset=svgPoints[i][j].y:1;
				}
			}
			xoffset<=0?xoffset=abs(xoffset-5):xoffset*=-1;
			yoffset<=0?yoffset=abs(yoffset-5):yoffset*=-1;
			// iterate over it again and shift everthing over by the offset amount
			for(int i=0;i<svgPoints.size();i++){
				for(int j=0;j<svgPoints[i].size();j++){
					svgPoints[i][j].x+=xoffset;
					svgPoints[i][j].y+=yoffset;
				}
			}
			//scale it up to the higher resolution
			for(int i=0;i<svgPoints.size();i++){
				for(int j=0;j<svgPoints[i].size();j++){
					svgPoints[i][j].x*=PIXELS_PER_INCH/72.0;
					svgPoints[i][j].y*=PIXELS_PER_INCH/72.0;
				}
			}
			outlines.clear();
			for(int i=0;i<svgPoints.size();i++){
				ofPolyline temp;
				for(int j=0;j<svgPoints[i].size();j++){
					temp.addVertex(svgPoints[i][j]);
				}
				outlines.push_back(temp);
			}
		}
		if(fileExt.compare(".jpg")==0 || fileExt.compare(".jpeg")==0 || fileExt.compare(".png")==0 || fileExt.compare(".tiff")==0){
			imageLoaded=true;
			loadedImage.loadImage(fileName);
			//images are typically 72 dpi we need to resize things to deal with the higher resolution 
			loadedImage.resize(loadedImage.getWidth()*(PIXELS_PER_INCH/72.0), loadedImage.getHeight()*(PIXELS_PER_INCH/72.0));		
			processedImage.allocate(loadedImage.getWidth(), loadedImage.getHeight());
			intermediaryImage.setFromPixels(loadedImage.getPixels(), loadedImage.getWidth(), loadedImage.getHeight());
			//We need the intermediary image here because it does the tranlation for us in terms of channels 
			processedImage=intermediaryImage;
			//release the memory assets since we dont need them anymore
			loadedImage.clear();
			intermediaryImage.clear();
		}
		if(fileExt.compare(".ngc")==0 || fileExt.compare(".gcode")==0 || fileExt.compare(".cnc")==0 || fileExt.compare(".nc")==0){
			GCodeLoaded=true;
			parseGCode(fileName);
		} 

		ofLogNotice("Open File") << "Opened File Successfully";
	}
	else
	{
		// GetOpenFileName can return FALSE because the user cancelled,
		// or because it failed. Check for errors.
		ofLogNotice("Open File") << "Failed to Load File";
	}
}
void ofApp::home(){
	moveMotors2(laserLocation.x, laserLocation.y, PIXELS);
	while(!digitalRead(LIMIT_SWITCH)==LOW ){
		rotateX(10,1);
	}
	while(!digitalRead(LIMIT_SWITCH4)==LOW ){
		rotateY(10,1);
	}
	rotateX(-500,1);
	rotateY(-500, 1);
	laserLocation.set(0,0);
	actualLaserLocation.set(0,0);
}

void ofApp::printDrawing(){//the mouse drawing is all integer based so we don't need to utilize floating point algorithm
	numStop=0;
	if(mouseDraw.size()>0){
		for(int k=0;k<passes;k++){
			cout << ofToString(actualLaserLocation) << ", " << ofToString(laserLocation) << endl;
			for(int j=0;j<mouseDraw.size();j++){
				if(numStop>3)
					goto stopPrint;
				float deltax = laserLocation.x-mouseDraw[j][0].x;
				float deltay = laserLocation.y-mouseDraw[j][0].y;
				rotateX(round(deltax*STEPS_PER_PIXEL),.6);
				rotateY(round(deltay*STEPS_PER_PIXEL),.6);
				laserLocation=mouseDraw[j][0];
				laserOn=true;
				toggleLaser();
				for(int i=1; i<mouseDraw[j].size();i++){
					if(digitalRead(STOP_BUT)==LOW)
						numStop++;
					moveMotors2((mouseDraw[j][i-1].x-mouseDraw[j][i].x), (mouseDraw[j][i-1].y-mouseDraw[j][i].y), PIXELS);
					laserLocation = mouseDraw[j][i];
				}
				laserOn=false;
				toggleLaser();
			}
		}
		cout << ofToString(actualLaserLocation) << ", " << ofToString(laserLocation) << endl;
		laserOn=false;
		toggleLaser();
		home();
	}
stopPrint:
	laserOn=false;
	toggleLaser();
}
void ofApp::printSvg(){
	bool firstRun = true;
	numStop=0;
	if(svgPoints.size()>0){
		float dx = laserLocation.x-svgPoints[0][0].x;
		float dy = laserLocation.y-svgPoints[0][0].y;
		//cout << deltax << ", " << deltay << endl;
		rotateX(round(dx*STEPS_PER_PIXEL),.6);
		rotateY(round(dy*STEPS_PER_PIXEL),.6);
		actualLaserLocation.set(0,0);
		laserLocation=svgPoints[0][0];
		for(int k=0;k<passes;k++){
			rotateX(round(0-actualLaserLocation.x),.6);
			rotateY(round(0-actualLaserLocation.y),.6);
			firstRun = true;
			cout << ofToString(actualLaserLocation) << ", " << ofToString(laserLocation) << endl;
			for(int i=0;i<svgPoints.size();i++){
				if(numStop>3)
					goto stopPrint;
				if(!firstRun){
					float deltax = laserLocation.x-svgPoints[i][0].x;
					float deltay = laserLocation.y-svgPoints[i][0].y;
					//cout << deltax << ", " << deltay << endl;
					rotateX(round(deltax*STEPS_PER_PIXEL),.6);
					rotateY(round(deltay*STEPS_PER_PIXEL),.6);
					laserLocation=svgPoints[i][0];
				}
				firstRun=false;
				laserOn=true;
				toggleLaser();
				for(int j=1; j<svgPoints[i].size();j++){
					if(digitalRead(STOP_BUT)==LOW)
						numStop++;
					moveMotors((svgPoints[i][j-1].x-svgPoints[i][j].x), (svgPoints[i][j-1].y-svgPoints[i][j].y),PIXELS);
					laserLocation = svgPoints[i][j];
				}
				laserOn=false;
				toggleLaser();
			}
			cout << ofToString(actualLaserLocation) << ", " << ofToString(laserLocation) << endl;
		}
		laserOn=false;
		toggleLaser();
		home();
	}
stopPrint:
	laserOn=false;
	toggleLaser();

}
void ofApp::printGCode(){
	numStop=0;
	if(gcodePoly.size()>0){
		for(int k=0;k<passes;k++){
			cout << ofToString(actualLaserLocation) << ", " << ofToString(laserLocation) << endl;
			for(int j=0;j<gcodePoly.size();j++){
				if(numStop>3)
					goto stopPrint;
				float deltax = laserLocation.x-gcodePoly[j][0].x;
				float deltay = laserLocation.y-gcodePoly[j][0].y;
				rotateX(round(deltax*STEPS_PER_PIXEL),.6);
				rotateY(round(deltay*STEPS_PER_PIXEL),.6);
				laserLocation=gcodePoly[j][0];
				laserOn=true;
				toggleLaser();
				for(int i=1; i<gcodePoly[j].size();i++){
					if(digitalRead(STOP_BUT)==LOW)
						numStop++;
					moveMotors((gcodePoly[j][i-1].x-gcodePoly[j][i].x), (gcodePoly[j][i-1].y-gcodePoly[j][i].y), PIXELS);
					laserLocation = gcodePoly[j][i];
				}
				laserOn=false;
				toggleLaser();
			}
		}
		cout << ofToString(actualLaserLocation) << ", " << ofToString(laserLocation) << endl;
		laserOn=false;
		toggleLaser();
		home();
	}
stopPrint:
	laserOn=false;
	toggleLaser();

}
void ofApp::printImage(){
	numStop=0;
	if(processedImage.getHeight()>0 && processedImage.getWidth()>0){
		ofPoint homeLoc = ofPoint(laserLocation.x, laserLocation.y);
		ofPixels currentPixel = ofPixels(processedImage.getPixelsRef());
		rotateX(round(imgOffset.x*PIXELS_PER_INCH/72.0*STEPS_PER_PIXEL),.6);
		rotateY(round(imgOffset.y*PIXELS_PER_INCH/72.0*STEPS_PER_PIXEL),.6);
		for(int k=0;k<passes;k++){
			cout << ofToString(actualLaserLocation) << ", " << ofToString(laserLocation) << endl;
			for(int j=0;j<processedImage.getHeight();j++){
				if(numStop>3)
					goto stopPrint;
				rotateY(round(-1*STEPS_PER_PIXEL),.6);
				laserLocation.y-=1;
				laserOn=true;
				toggleLaser();
				for(int i=j%2==0?processedImage.getWidth()-1:0; j%2==0?i>=0:i<processedImage.getWidth();j%2==0?i--:i++){
					if(digitalRead(STOP_BUT)==LOW)
						numStop++;
					int lVal = currentPixel.getPixelIndex(i,j);
					if (lVal<=threshold) {
						laserOn=false;
						toggleLaser();
					}
#ifdef TARGET_LINUX
					else {
						unsigned char buffer[2]; //needs to be saved to the z of ofPoint and power is adjusted during printing
						//currently firmata doesnt support SPI communication will probably have to alter standard firmata code
						buffer[0]= 0xB0000;
						buffer[1]= 255-lVal;//processedImage.getPixels()[const (j*processedImage.getWidth())+i];
						wiringPiSPIDataRW(0, buffer, 2);
					}
#endif
					rotateX(round((j%2==0?-1:1)*STEPS_PER_PIXEL),speed);
					j%2==0?laserLocation.x-=1:laserLocation.x+=1;
				}
				laserOn=false;
				toggleLaser();
			}
			rotateX(round(homeLoc.x-actualLaserLocation.x),.6);
			rotateY(round(homeLoc.y-actualLaserLocation.y),.6);
			laserLocation = homeLoc;
		}
		cout << ofToString(actualLaserLocation) << ", " << ofToString(laserLocation*STEPS_PER_PIXEL) << endl;
		laserOn=false;
		toggleLaser();
		home();
	}
stopPrint:
	laserOn=false;
	toggleLaser();

}
//void ofApp::testWidth(){
//	while(!digitalRead(LIMIT_SWITCH)==LOW ){
//		rotateX(10,1);
//	}
//	int numSteps=0;
//	while(!digitalRead(LIMIT_SWITCH2)==LOW ){
//		rotateX(-10,1);
//		numSteps+=10;
//	}
//	ofLogNotice() << "Width Tested: " << numSteps << " steps";
//}
//void ofApp::testHeight(){
//	while(!digitalRead(LIMIT_SWITCH4)==LOW ){
//		rotateY(10,1);
//	}
//	int numSteps=0;
//	while(!digitalRead(LIMIT_SWITCH3)==LOW ){
//		rotateY(-10,1);
//		numSteps+=10;
//	}
//	ofLogNotice() << "Height Tested: " << numSteps << " steps";
//}

void ofApp::passesChanged(int & numPasses){
	passes=numPasses;
}
void  ofApp::laserPowerChanged(float & lPow){
#ifdef TARGET_LINUX
	unsigned char buffer[2]; //needs to be saved to the z of ofPoint and power is adjusted during printing
	//currently firmata doesnt support SPI communication will probably have to alter standard firmata code
	buffer[0]= 0xB0000;
	buffer[1]= 255-(255*lPow)
	wiringPiSPIDataRW(0, buffer, 2);
#endif
}
#ifndef TARGET_LINUX
void ofApp::setupArduino(const int & version) {

	// remove listener because we don't need it anymore
	ofRemoveListener(ard.EInitialized, this, &ofApp::setupArduino);

	// it is now safe to send commands to the Arduino
	bSetupArduino = true;

	// printf( firmware name and version to the console
	ofLogNotice() << ard.getFirmwareName();
	ofLogNotice() << "firmata v" << ard.getMajorFirmwareVersion() << "." << ard.getMinorFirmwareVersion();

	ard.sendDigitalPinMode(STEP_PIN, ARD_OUTPUT);
	ard.sendDigitalPinMode(STEP_PIN2, ARD_OUTPUT);
	ard.sendDigitalPinMode(DIR_PIN, ARD_OUTPUT);
	ard.sendDigitalPinMode(DIR_PIN2, ARD_OUTPUT);
	ard.sendDigitalPinMode(LASER_PIN, ARD_OUTPUT);

	ard.sendDigitalPinMode(LIMIT_SWITCH, ARD_INPUT_PULLUP);
	ard.sendDigitalPinMode(LIMIT_SWITCH2, ARD_INPUT_PULLUP);
	ard.sendDigitalPinMode(LIMIT_SWITCH3, ARD_INPUT_PULLUP);
	ard.sendDigitalPinMode(LIMIT_SWITCH4, ARD_INPUT_PULLUP);
	ard.sendDigitalPinMode(STOP_BUT, ARD_INPUT_PULLUP);


	// Listen for changes on the digital and analog pins
	ofAddListener(ard.EDigitalPinChanged, this, &ofApp::digitalPinChanged);
	ofAddListener(ard.EAnalogPinChanged, this, &ofApp::analogPinChanged);
}

//--------------------------------------------------------------
void ofApp::updateArduino(){

	// update the arduino, get any data or messages.
	// the call to ard.update() is required
	if(bSetupArduino){
		if(ard.getDigital(STOP_BUT)==ARD_HIGH && numStop>0) numStop--;
		else if(ard.getDigital(STOP_BUT)==ARD_LOW) numStop++;
		ard.update();
	}

}

// digital pin event handler, called whenever a digital pin value has changed
// note: if an analog pin has been set as a digital pin, it will be handled
// by the digitalPinChanged function rather than the analogPinChanged function.

//--------------------------------------------------------------
void ofApp::digitalPinChanged(const int & pinNum) {
	if(pinNum == LIMIT_SWITCH && ard.getDigital(LIMIT_SWITCH)==ARD_LOW){
		limitSwitch=true;
	}
	if(pinNum == LIMIT_SWITCH2 && ard.getDigital(LIMIT_SWITCH2)==ARD_LOW){
		limitSwitch2=true;
	}
	if(pinNum == LIMIT_SWITCH3 && ard.getDigital(LIMIT_SWITCH3)==ARD_LOW){
		limitSwitch3=true;
	}
	if(pinNum == LIMIT_SWITCH4 && ard.getDigital(LIMIT_SWITCH4)==ARD_LOW){
		limitSwitch4=true;
	}
	if(pinNum == LIMIT_SWITCH && ard.getDigital(LIMIT_SWITCH)==ARD_HIGH){
		limitSwitch=false;
	}
	if(pinNum == LIMIT_SWITCH2 && ard.getDigital(LIMIT_SWITCH2)==ARD_HIGH){
		limitSwitch2=false;
	}
	if(pinNum == LIMIT_SWITCH3 && ard.getDigital(LIMIT_SWITCH3)==ARD_HIGH){
		limitSwitch3=false;
	}
	if(pinNum == LIMIT_SWITCH4 && ard.getDigital(LIMIT_SWITCH4)==ARD_HIGH){
		limitSwitch4=false;
	}
	if(pinNum == STOP_BUT && ard.getDigital(STOP_BUT)==ARD_LOW){
		numStop+=2;
	}
}

// 
// analog pin event handler, called whenever an analog pin value has changed
//--------------------------------------------------------------
void ofApp::analogPinChanged(const int & pinNum) {
	// do something with the analog input. here we're simply going to printf( the pin number and
	// value to the screen each time it changes
}
int ofApp::digitalRead(int pin){
	return ard.getDigital(pin);
}
void ofApp::digitalWrite(int pin, int value){
	ard.sendDigital(pin, value);
}
#endif