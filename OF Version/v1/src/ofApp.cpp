#include "ofApp.h"



//--------------------------------------------------------------
void ofApp::setup(){
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

	ofSetVerticalSync(true);
	ofSetFrameRate(60);

	ofBackground(80);

	laserLocation.set(0,0);
	newloc.set(0,0);
	oldloc.set(0,0);

	laserOn = false;

	numStop=0;

	limitSwitch=false;
	limitSwitch2=false;
	stop = false;
}

//--------------------------------------------------------------
void ofApp::update(){
	if(digitalRead(LIMIT_SWITCH)==LOW){
		limitSwitch=true;
	}
	if(digitalRead(LIMIT_SWITCH2)==LOW){
		limitSwitch2=true;
	}
	if(digitalRead(LIMIT_SWITCH3)==LOW){
		limitSwitch3=true;
	}
	if(digitalRead(LIMIT_SWITCH4)==LOW){
		limitSwitch4=true;
	}
	if(digitalRead(LIMIT_SWITCH)==HIGH){
		limitSwitch=false;
	}
	if(digitalRead(LIMIT_SWITCH2)==HIGH){
		limitSwitch2=false;
	}
	if(digitalRead(LIMIT_SWITCH3)==HIGH){
		limitSwitch3=false;
	}
	if(digitalRead(LIMIT_SWITCH4)==HIGH){
		limitSwitch4=false;
	}
	if(digitalRead(STOP_BUT)==LOW){
		numStop+=2;
	}
	if(numStop>5){
		numStop=0;
		stop = true;
	}
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofSetColor(255);
	ofRect(100,30, BED_WIDTH, BED_HEIGHT);
	ofTranslate(100,30);
	ofSetLineWidth(1);
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
	ofTranslate(-100, -30);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key)
	{
	case OF_KEY_LEFT:
		rotate(100, 1); 
		break;
	case OF_KEY_RIGHT:
		rotate(-100, 1); 
		break;
	case OF_KEY_UP:
		rotate2(-100, 1); 
		break;
	case OF_KEY_DOWN:
		rotate2(100, 1); 
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
	case 'q':
		mouseDraw.clear();
		outlines.clear();
		gcodePoly.clear();
		break;
	case OF_KEY_BACKSPACE:
		if(mouseDraw.size()>0)
			mouseDraw.erase(mouseDraw.begin()+mouseDraw.size()-1);
		break; 
	case 't':
		rotate(-WIDTH_STEPS,1);
		break;
	case '1':
		moveMotors(5, 5, PIXELS);
		break;
	case '3':
		moveMotors(-5, 5, PIXELS);
		break;
	case '7':
		moveMotors(5, -5, PIXELS);
		break;
	case '9':
		moveMotors(-5, -5, PIXELS);
		break;
	default:
		break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
	if(key==' ')
		digitalWrite(LASER_PIN, ARD_LOW);
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
	if(x>100 && x<BED_WIDTH+100 && y>30 && y<BED_HEIGHT+30)
		mousePoints.push_back(ofPoint(x-100,y-30));
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
	if(x>100 && x<BED_WIDTH+100 && y>30 && y<BED_HEIGHT+30)
		mousePoints.push_back(ofPoint(x-100,y-30));
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
	if(x>100 && x<BED_WIDTH+100 && y>30 && y<BED_HEIGHT+30)
		mousePoints.push_back(ofPoint(x-100,y-30));
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

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
void ofApp::rotate(float steps, float speed){ 
	//rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
	//speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
	if(!(steps<0 && limitSwitch2) && !(steps>0 && limitSwitch)){
		int dir = (steps > 0)? ARD_HIGH:ARD_LOW;
		int isteps = round(abs(steps));

		digitalWrite(DIR_PIN,dir); 


		for(int i=0; i < isteps; i++){ 
			digitalWrite(STEP_PIN, ARD_HIGH); 
			//ofSleepMillis(1); 

			digitalWrite(STEP_PIN, ARD_LOW); 
			//ofSleepMillis(1); 
		} 
	}
} 

void ofApp::rotateDeg(float deg, float speed){ 
	//rotate a specific number of degrees (negitive for reverse movement)
	//speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
	if(!(deg<0 && limitSwitch2) && !(deg>0 && limitSwitch)){
		int dir = (deg > 0)? ARD_HIGH:ARD_LOW;
		digitalWrite(DIR_PIN,dir); 

		int steps = round(abs(deg)*(1/0.225));

		for(int i=0; i < steps; i++){ 
			digitalWrite(STEP_PIN, ARD_HIGH); 
			//ofSleepMillis(1);

			digitalWrite(STEP_PIN, ARD_LOW); 
			//ofSleepMillis(1);
		} 
	}
}

void ofApp::rotate2(float steps, float speed){ 
	//rotate a specific number of microsteps (8 microsteps per step) - (negitive for reverse movement)
	//speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
	if(!(steps<0 && limitSwitch2) && !(steps>0 && limitSwitch)){
		int dir = (steps > 0)? ARD_HIGH:ARD_LOW;
		int isteps = round(abs(steps));

		digitalWrite(DIR_PIN2,dir); 


		for(int i=0; i < isteps; i++){ 
			digitalWrite(STEP_PIN2, ARD_HIGH); 
			//ofSleepMillis(1);

			digitalWrite(STEP_PIN2, ARD_LOW); 
			//ofSleepMillis(1);
		} 
	}
} 

void ofApp::rotateDeg2(float deg, float speed){ 
	//rotate a specific number of degrees (negitive for reverse movement)
	//speed is any number from .01 -> 1 with 1 being fastest - Slower is stronger
	if(!(deg<0 && limitSwitch3) && !(deg>0 && limitSwitch4)){
		int dir = (deg > 0)? ARD_HIGH:ARD_LOW;
		digitalWrite(DIR_PIN2,dir); 

		int steps = round(abs(deg)*(1/0.225));

		for(int i=0; i < steps; i++){ 
			digitalWrite(STEP_PIN2, ARD_HIGH); 
			//ofSleepMillis(1);

			digitalWrite(STEP_PIN2, ARD_LOW); 
			//ofSleepMillis(1); 
		} 
	}
}


void ofApp::toggleLaser(){
	if(laserOn)
		digitalWrite(LASER_PIN, ARD_HIGH);
	else
		digitalWrite(LASER_PIN, ARD_LOW);
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
		if(stop)
			break;
		if ((i % micro_step1)==0) //motor 1 need to turn one step
			rotate(dir1,1);

		if ((i % micro_step2)==0) //motor 2 need to turn one step
			rotate2(dir2,1);
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
		if(stop)
			break;
		if ((i % micro_step1)==0) //motor 1 need to turn one step
			rotate(dir1,1);

		if ((i % micro_step2)==0) //motor 2 need to turn one step
			rotate2(dir2,1);
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
			int lpow=int(z_pos*255);
			unsigned char buffer[2];
			buffer[0]= 0xB0000;
			buffer[1]= 255-lpow;
			wiringPiSPIDataRW(0, buffer, 2);
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
	// needs to be adapted to use linux's open file dialog
	/*const WCHAR *lpstrFilter = 
	L"Vector Images\0*.svg;\0"
	L"GCode Files\0*.nc;*.ngc;*.cnc;*.gcode\0"
	L"All files\0*.*\0";

	HRESULT hr = S_OK;

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	WCHAR szFileName[MAX_PATH];
	szFileName[0] = L'\0';

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.hInstance = GetInstance();
	ofn.lpstrFilter = lpstrFilter;
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
	char ch[260];
	char DefChar = ' ';
	WideCharToMultiByte(CP_ACP,0,szFileName,-1, ch,260,&DefChar, NULL);

	string fileName = ofToString(ch);
	string fileExt = fileName.substr(fileName.find_last_of('.'), fileName.length()-fileName.find_last_of('.'));
	if(fileExt.compare(".svg")==0){
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

	}
	if(fileExt.compare(".ngc")==0 || fileExt.compare(".gcode")==0 || fileExt.compare(".cnc")==0 || fileExt.compare(".nc")==0)
	parseGCode(fileName);
	ofLogNotice("Open File") << "Opened File Successfully";
	}
	else
	{
	// GetOpenFileName can return FALSE because the user cancelled,
	// or because it failed. Check for errors.

	ofLogNotice("Open File") << "Failed to Load File";
	}*/
}
void ofApp::home(){
	moveMotors2(laserLocation.x, laserLocation.y, PIXELS);
	while(!limitSwitch){
		rotate(100,1);
	}
	rotate(-1300,1);
	laserLocation.set(0,0);
}

void ofApp::printDrawing(){//the mouse drawing is all integer based so we don't need to utilize floating point algorithm
	stop = false;
	if(mouseDraw.size()>0){			
		for(int j=0;j<mouseDraw.size();j++){
			if(stop)
				goto stopPrint;
			float deltax = laserLocation.x-mouseDraw[j][0].x;
			float deltay = laserLocation.y-mouseDraw[j][0].y;
			rotate(round(deltax*STEPS_PER_PIXEL),1);
			rotate2(round(deltay*STEPS_PER_PIXEL),1);
			laserLocation=mouseDraw[j][0];
			laserOn=true;
			toggleLaser();
			for(int i=1; i<mouseDraw[j].size();i++){
				moveMotors2((mouseDraw[j][i-1].x-mouseDraw[j][i].x), (mouseDraw[j][i-1].y-mouseDraw[j][i].y), PIXELS);
				laserLocation = mouseDraw[j][i];
			}
			laserOn=false;
			toggleLaser();
		}
		laserOn=false;
		toggleLaser();
		home();

	}
stopPrint:
	laserOn=false;
	toggleLaser();
	stop=false;
}
void ofApp::printSvg(){
	stop=false;
	if(svgPoints.size()>0){
		for(int i=0;i<svgPoints.size();i++){
			if(stop)
				goto stopPrint;
			float deltax = laserLocation.x-svgPoints[i][0].x;
			float deltay = laserLocation.y-svgPoints[i][0].y;
			rotate(deltax*STEPS_PER_PIXEL,1);
			rotate2(deltay*STEPS_PER_PIXEL,1);
			laserLocation=svgPoints[i][0];
			laserOn=true;
			toggleLaser();
			for(int j=1; j<svgPoints[i].size();j++){
				moveMotors((svgPoints[i][j-1].x-svgPoints[i][j].x), (svgPoints[i][j-1].y-svgPoints[i][j].y),PIXELS);
				laserLocation = svgPoints[i][j];
			}
			laserOn=false;
			toggleLaser();
		}
		laserOn=false;
		toggleLaser();
		home();
	}
stopPrint:
	laserOn=false;
	toggleLaser();
	stop=false;
}
void ofApp::printGCode(){
	stop=false;
	if(gcodePoly.size()>0){			
		for(int j=0;j<gcodePoly.size();j++){
			if(stop)
				goto stopPrint;
			float deltax = laserLocation.x-gcodePoly[j][0].x;
			float deltay = laserLocation.y-gcodePoly[j][0].y;
			rotate(round(deltax*STEPS_PER_PIXEL),1);
			rotate2(round(deltay*STEPS_PER_PIXEL),1);
			laserLocation=gcodePoly[j][0];
			laserOn=true;
			toggleLaser();
			for(int i=1; i<gcodePoly[j].size();i++){
				moveMotors((gcodePoly[j][i-1].x-gcodePoly[j][i].x), (gcodePoly[j][i-1].y-gcodePoly[j][i].y), PIXELS);
				laserLocation = gcodePoly[j][i];
			}
			laserOn=false;
			toggleLaser();
		}
		laserOn=false;
		toggleLaser();
		home();

	}
stopPrint:
	laserOn=false;
	toggleLaser();
	stop=false;
}