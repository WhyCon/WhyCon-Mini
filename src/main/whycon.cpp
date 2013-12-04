#include <stdlib.h>
#include "CCamera.h"
#include "CGui.h"
#include "CTimer.h"
#include "CCircleDetect.h"
#include "CPosition.h"
#include "CTransformation.h"
#include <SDL/SDL.h>
#include "CMessageClient.h"
#include "CPositionServer.h"

#define MAX_PATTERNS 400 
#define CALIB_STEPS 5 
#define AUTO_CALIB_STEPS 30 
#define AUTO_CALIB_INITS 10 

//Adjust camera resolution here
//int  imageWidth= 1920;
//int  imageHeight = 1080;
//int  imageWidth= 1280;
//int  imageHeight = 720;
int  imageWidth= 1600;
int  imageHeight = 1200;
int numFound = 0;
int numStatic = 0;
ETransformType transformType = TRANSFORM_2D;

//Adjust the black circle diameter [m] 
float circleDiameter = 0.122;

//Adjust the X and Y coordinate of the circles, which define the coordinate system
float circlePositionX = 0.50;
float circlePositionY = 0.43;

STrackedObject lastObject;
bool autotest = false;
bool autocalibrate = false;
bool track = true;
int runs = 0;
int trackOK = 0;
int numBots = 0;
int guiScale = 1;
int wasBots = 1;
int evalTime = 0;
int calibNum = 5;
STrackedObject calib[5];
STrackedObject calibTmp[CALIB_STEPS];
int calibStep = CALIB_STEPS+2;
TLogModule module = LOG_MODULE_MAIN;
bool useGui = true;
int keyF = 0;
int lastKey = 0;
bool displayTime = false;
int numSaved = 0;
bool stop = false;
bool unbarrel = false;
bool displayHelp = false;
bool drawCoords = true;
CCamera* camera;
CGui* gui;
CRawImage *image;
SDL_Event event;
CPositionServer* server;

CCircleDetect *detectorArray[MAX_PATTERNS];
STrackedObject objectArray[MAX_PATTERNS];
SSegment currentSegmentArray[MAX_PATTERNS];
SSegment lastSegmentArray[MAX_PATTERNS];

CTransformation *trans;

int moveVal = 10000000;
int moveOne = moveVal;
int keyNumber = 10000;
Uint8 lastKeys[1000];
Uint8 *keys = NULL;

void processKeys()
{
	while (SDL_PollEvent(&event)){
		if (event.type == SDL_MOUSEBUTTONDOWN){
			if (calibNum < 4 && calibStep > CALIB_STEPS){
				 calibStep = 0;
				 trans->transformType = TRANSFORM_NONE;
			}
			currentSegmentArray[numBots-1].x = event.motion.x*guiScale; 
			currentSegmentArray[numBots-1].y = event.motion.y*guiScale;
			currentSegmentArray[numBots-1].valid = true;
		}
	}
	keys = SDL_GetKeyState(&keyNumber);
	if (keys[SDLK_ESCAPE]) stop = true;
	if (keys[SDLK_SPACE] && lastKeys[SDLK_SPACE] == false) moveOne = 10000000;
	if (keys[SDLK_p] && lastKeys[SDLK_p] == false) {moveOne = 1; moveVal = 0;}
	if (keys[SDLK_m] && lastKeys[SDLK_m] == false) printf("SAVE %03f %03f %03f %03f %03f %03f %03f\n",objectArray[0].x,objectArray[0].y,objectArray[0].z,objectArray[0].error,objectArray[0].d,currentSegmentArray[0].m0,currentSegmentArray[0].m1);
	if (keys[SDLK_n] && lastKeys[SDLK_n] == false) printf("SEGM %03f %03f %03f\n",currentSegmentArray[0].x,currentSegmentArray[0].y,currentSegmentArray[0].m0);
	if (keys[SDLK_s] && lastKeys[SDLK_s] == false) image->saveBmp();
	if (keys[SDLK_d] && lastKeys[SDLK_d] == false) for (int i = 0;i<MAX_PATTERNS;i++){
			detectorArray[i]->draw = detectorArray[i]->draw==false;
			detectorArray[i]->debug = 10-detectorArray[i]->debug;
			
	}
	if (keys[SDLK_u] && lastKeys[SDLK_u] == false) unbarrel = unbarrel==false;
	if (keys[SDLK_l] && lastKeys[SDLK_l] == false) drawCoords = drawCoords == false;
	if (keys[SDLK_r] && lastKeys[SDLK_r] == false) { calibNum = 0; wasBots=numBots; numBots = 1;}; 
	if (keys[SDLK_a] && lastKeys[SDLK_a] == false) { calibStep = 0; transformType=trans->transformType; wasBots=numBots; numBots = 4;autocalibrate = true;trans->transformType=TRANSFORM_NONE;}; 
	if (keys[SDLK_v] && lastKeys[SDLK_v] == false) for (int i = 0;i<numBots;i++) detectorArray[i]->drawAll = detectorArray[i]->drawAll==false;;
	if (keys[SDLK_t] && lastKeys[SDLK_t] == false)displayTime = displayTime == false;
	if (keys[SDLK_1] && lastKeys[SDLK_1] == false) trans->transformType = TRANSFORM_NONE;
	if (keys[SDLK_2] && lastKeys[SDLK_2] == false) trans->transformType = TRANSFORM_2D;
	if (keys[SDLK_3] && lastKeys[SDLK_3] == false) trans->transformType = TRANSFORM_3D;
	if (keys[SDLK_4] && lastKeys[SDLK_4] == false) trans->transformType = TRANSFORM_4D;

	if (keys[SDLK_c] && keys[SDLK_RSHIFT] == false) camera->changeContrast(-1);
	if (keys[SDLK_c] && keys[SDLK_RSHIFT] == true) camera->changeContrast(1);
	if (keys[SDLK_g] && keys[SDLK_RSHIFT] == false) camera->changeGain(-1);
	if (keys[SDLK_g] && keys[SDLK_RSHIFT] == true) camera->changeGain(1);
	if (keys[SDLK_e] && keys[SDLK_RSHIFT] == false) camera->changeExposition(-1);
	if (keys[SDLK_e] && keys[SDLK_RSHIFT] == true) camera->changeExposition(1);
	if (keys[SDLK_b] && keys[SDLK_RSHIFT] == false) camera->changeBrightness(-1);
	if (keys[SDLK_b] && keys[SDLK_RSHIFT] == true) camera->changeBrightness(1);

	if (keys[SDLK_9] && lastKeys[SDLK_9] == false){
			 numBots = 3;
			 drawCoords = false;
	}
	if (keys[SDLK_h] && lastKeys[SDLK_h] == false) displayHelp = displayHelp == false; 
	if (keys[SDLK_PLUS]) numBots++;
	if (keys[SDLK_EQUALS]) numBots++;
	if (keys[SDLK_MINUS]) numBots--;
	if (keys[SDLK_KP_PLUS]) numBots++;
	if (keys[SDLK_KP_MINUS]) numBots--;
	if (keys[SDLK_KP0]) numBots = 0;
	if (keys[SDLK_KP1]) numBots = 13;
	memcpy(lastKeys,keys,keyNumber);
}

int main(int argc,char* argv[])
{
	dump = new CDump(NULL,256,1000000);
	if (argc < 2) {
		fprintf(stderr,"usage: %s cameraDevice\ne.g. %s /dev/video0 num_tracked\n",argv[0],argv[0]);
		return 0;
	}
	const char* cameraDevice = argv[1];
	camera = new CCamera();
	server = new CPositionServer();
	server->init("6666"); 
	if (argc > 2){
		numBots = atoi(argv[2]);
		moveOne = moveVal;
	}
	if (argc > 3){
		useGui = !(strcmp(argv[3],"nogui")==0 || strcmp(argv[3],"autotest")==0);
		autotest = (strcmp(argv[3],"autotest")==0);

		if (autotest){
			calibNum = calibStep = 0;
			circlePositionX = 0.625*atoi(argv[4]);
			circlePositionY = 1.250*atoi(argv[5]);
			transformType = (ETransformType) atoi(argv[6]);
			moveVal = moveOne = 0;
			//useGui = true;
		}
	}
	if (argc > 4) track = strcmp(argv[4],"notrack");
	camera->init(cameraDevice,&imageWidth,&imageHeight,false);
	while (imageHeight/guiScale > 800) guiScale = guiScale*2; 
	if (useGui) gui = new CGui(imageWidth,imageHeight,guiScale);
	image = new CRawImage(imageWidth,imageHeight);
	trans = new CTransformation(imageWidth,imageHeight,circleDiameter,true);

	for (int i = 0;i<MAX_PATTERNS;i++) detectorArray[i] = new CCircleDetect(imageWidth,imageHeight,i);

	//Inicializace zarizeni 
	image->getSaveNumber();

	CTimer timer;
	CTimer globalTimer;
	globalTimer.reset();
	globalTimer.start();
	int correct = 0;
	int fails = 0;
	int tmpFails = 0;
	int totTmpFails = 0;
	int runs = 0;
	int timeradd = 0;


	CRawImage *unbar = new CRawImage(imageWidth,imageHeight);
	globalTimer.reset();
	globalTimer.start();
	int frameID =0;
	while (stop == false){
		lastKey = keyF;
		keyF = camera->renewImage(image,moveOne-->0);
		if (autocalibrate==false) detectorArray[0]->applyCalibMask(image);

		if (displayTime) printf("Camera acquisition time: %i us.\n",globalTimer.getTime());
		//keyF = camera->renewImage(image,true);
		//stop = keyF == -1;
		if (stop) break;
		if (unbarrel){
			trans->unbarrel(unbar->data,image->data);
			memcpy(image->data,unbar->data,image->size);
		}else{
			//memcpy(unbar->data,image->data,image->size);
		}
		timer.reset();
		timer.start();
		if (displayTime) printf("Image unbarrel time: %i us.\n",globalTimer.getTime());
		numFound = numStatic = 0;

		for (int i = 0;i<numBots;i++){
			lastSegmentArray[i] = currentSegmentArray[i];
			detectorArray[i]->calibMode = autocalibrate;
			if (lastSegmentArray[i].valid) currentSegmentArray[i] = detectorArray[i]->findSegment(image,lastSegmentArray[i]); else currentSegmentArray[i].valid = false; 
		}

		for (int i = 0;i<numBots;i++){
			if (currentSegmentArray[i].valid == false){
				lastSegmentArray[i].valid = false;
				currentSegmentArray[i] = detectorArray[i]->findSegment(image,lastSegmentArray[i]);
			}
			if (currentSegmentArray[i].valid == false) break;
		}
		for (int i = 0;i<numBots;i++){
			if (currentSegmentArray[i].valid){
				objectArray[i] = trans->transform(currentSegmentArray[i],unbarrel);
				numFound++;
				if (currentSegmentArray[i].x == lastSegmentArray[i].x) numStatic++;
			}else{
				trackOK = 0;
			}
		}
		if (displayTime) printf("Pattern detection time: %i us. Found: %i Static: %i.\n",globalTimer.getTime(),numFound,numStatic);
		evalTime = timer.getTime();
		server->setNumOfPatterns(numBots);
		for (int i = 0;i<numBots;i++) server->updatePosition(objectArray[i],i);
		if (useGui){
			gui->drawImage(image);
			gui->drawTimeStats(evalTime,numBots);
			gui->displayHelp(displayHelp);
			gui->guideCalibration(calibNum,circlePositionX,circlePositionY);
		}
		for (int i = 0;i<numBots && useGui && drawCoords;i++){
			if (currentSegmentArray[i].valid) gui->drawStats(currentSegmentArray[i].minx-30,currentSegmentArray[i].maxy,objectArray[i],trans->transformType == TRANSFORM_2D);
		}
		if (numBots == 4 && autocalibrate && numFound == 4){
			bool saveVals = true;
			for (int i = 0;i<4;i++){
				if (detectorArray[i]->lastTrackOK == false) saveVals=false;
			}
			if (saveVals){
				for (int i = 0;i<4;i++){
					if (calibStep <= AUTO_CALIB_INITS) calib[i].x = calib[i].y = calib[i].z = 0;
					calib[i].x+=objectArray[i].x;
					calib[i].y+=objectArray[i].y;
					calib[i].z+=objectArray[i].z;
				}
				calibStep++;
				if (calibStep == AUTO_CALIB_STEPS){
					for (int i = 0;i<4;i++){
						calib[i].x = calib[i].x/(AUTO_CALIB_STEPS-AUTO_CALIB_INITS);
						calib[i].y = calib[i].y/(AUTO_CALIB_STEPS-AUTO_CALIB_INITS);
						calib[i].z = calib[i].z/(AUTO_CALIB_STEPS-AUTO_CALIB_INITS);
					}
					trans->calibrate2D(calib,circlePositionX,circlePositionY);
					trans->calibrate3D(calib,circlePositionX,circlePositionY);
					trans->calibrate4D(calib,circlePositionX,circlePositionY);
					calibNum++;
					numBots = wasBots;
					trans->saveCalibration("default.cal");
					trans->transformType = transformType;
					autocalibrate = false;
					numBots = 6;
				}
			}
		}

		if (numBots == 1){
			if (currentSegmentArray[0].valid){
				STrackedObject o = objectArray[0];
				correct++;
				totTmpFails += tmpFails;
				tmpFails = 0;
				moveOne = moveVal;
				if (calibStep < CALIB_STEPS && (autotest == false || (lastObject.x == o.x && lastObject.y == o.y && calibNum < 4)))calibTmp[calibStep++] = o;

				if (autotest && calibNum > 4){
					if (lastObject.x == o.x && lastObject.y == o.y){
						moveOne = 1;
						printf("AUTO %03f %03f %03f %03f %03f %03f %03f %03f\n",o.x,o.y,o.z,o.error,o.d,o.esterror,currentSegmentArray[0].m0,currentSegmentArray[0].m1);
					}
				}

				//printf("Object %i %i %03f %03f %03f \n",calibNum,calibNum,objectArray[0].x,objectArray[0].y,objectArray[0].z);
				if (calibStep == CALIB_STEPS){
					//average measurements from CALIB_STEPS
					o.x = o.y = o.z = 0;
					for (int k = 0;k<CALIB_STEPS;k++){
						o.x += calibTmp[k].x;
						o.y += calibTmp[k].y;
						o.z += calibTmp[k].z;
					}
					o.x = o.x/CALIB_STEPS;	
					o.y = o.y/CALIB_STEPS;	
					o.z = o.z/CALIB_STEPS;
					if (calibNum < 4){
						calib[calibNum++] = o;
					}
					if (calibNum == 4){
						trans->calibrate2D(calib,circlePositionX,circlePositionY);
						trans->calibrate3D(calib,circlePositionX,circlePositionY);
						trans->calibrate4D(calib,circlePositionX,circlePositionY);
						calibNum++;
						numBots = wasBots;
						trans->saveCalibration("default.cal");
						trans->transformType = transformType;
					}
					calibStep++;
					lastSegmentArray[0].x = 0;
					if (autotest){
						moveOne = 1;
						calibStep = 0;
					}
				}
				lastObject = o;
			}else{
				tmpFails++;
				//moveOne = 0;
				if (tmpFails > 64){
					fails++;
					totTmpFails += tmpFails;
					tmpFails = 0;
					moveOne = moveVal;
				} 
			}
		}
		//gui->saveScreen(runs);
		//printf("Success rate: %f (%i of %i) %i %i \n",(float) correct/(correct+fails),correct,correct+fails,moveOne,numBots);
		if (displayTime) printf("Drawing results time: %i us.\n",globalTimer.getTime());
		if (useGui) gui->update();
		if (displayTime) printf("GUI update time: %i us.\n",globalTimer.getTime());
		if (useGui) processKeys();
		if (displayTime) printf("Processing input time: %i us.\n",globalTimer.getTime());
		if (trackOK++ > 3){
			timeradd+=evalTime;
			runs++;
		}
		if (camera->cameraType == CT_WEBCAM){
			for (int i = 0;i<numBots;i++){
				//if (currentSegmentArray[i].valid) printf("Object %i %03f %03f %03f %03f %03f\n",i,objectArray[i].x,objectArray[i].y,objectArray[i].z,objectArray[i].error,objectArray[i].esterror);
				if (currentSegmentArray[i].valid) printf("Frame %i Object %i %.0f %.0f %.0f \n",frameID,currentSegmentArray[i].ID,1000*objectArray[i].x,1000*objectArray[i].y,objectArray[i].yaw/M_PI*180.0);
			}
			frameID++;
		}else{
			if (numStatic ==  numBots){
				//for (int i = 0;i<numBots;i++) printf("Frame %i Object %i %.0f %.0f %.0f \n",frameID,currentSegmentArray[i].ID,1000*objectArray[i].x,1000*objectArray[i].y,objectArray[i].yaw/M_PI*180.0);
				//printf("Cycles: %i.\n",moveOne);
				moveOne = 1;
				//gui->saveScreen();
				frameID++;
			}
			else 
			{
				if (moveOne-- < -100) moveOne = 1;
			}
		}
		if (displayTime) printf("Printing results time: %i ms.\n",globalTimer.getTime());
	}
	runs--;
	int sumka = 0;
	for (int i =0;i<numBots;i++) sumka+=currentSegmentArray[i].size;
	printf("Success rate: %f (%i of %i), average time %.3f ms - %.3f FPS - %i %i %i\n",(float) correct/(correct+fails),correct,correct+fails,timeradd/1000.0/runs,1000000.0/timeradd*runs,sumka,imageWidth*imageHeight,numBots);
	printf("Misdetections %i.\n",totTmpFails);
	delete image;
	delete unbar;
	if (useGui) delete gui;
	for (int i = 0;i<MAX_PATTERNS;i++) delete detectorArray[i];
	delete camera;
	delete trans;
	return 0;
}
