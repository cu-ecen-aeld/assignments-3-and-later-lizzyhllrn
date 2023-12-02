/*
*/
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/video/background_segm.hpp>

#include <unistd.h>
#include <pthread.h>

#define DATA_FILE "/var/tmp/aesdsocketdata"
#define THRES 200

using namespace cv;
using namespace std;

extern "C" int capture_motion(pthread_mutex_t fileMutex);

int capture_motion(pthread_mutex_t fileMutex) {

    Mat frame, fgmask;
    vector<vector<Point> > cnts;
    VideoCapture camera(0, CAP_V4L2); //open camera
    if (!camera.isOpened()){
    //error in opening the video input
        cerr << "Unable to open camera " << endl;
        return -1;
    }
    bool activeMotion = false;
    bool frameMotion = false;
    int motionframecount = 0;
    int motionstillcount=0;
    int thissum;
    int frameNum = 0;
    
    //set the video size to 512x288 to process faster
    camera.set(3, 512);
    camera.set(4, 288);

        //create Background Subtractor objects
    Ptr<BackgroundSubtractor> pBackSub;
    pBackSub = createBackgroundSubtractorMOG2();

    //printf("waiting on camera to warm up\n");
    sleep(1);

    while(camera.read(frame)) {


        //update the background model
        pBackSub->apply(frame, fgmask);
        frameNum += 1;

        //reset motion indicator for this frame
        frameMotion = false;

        if (frameNum > 100 ) { // give the background filter some time to adjust

        //look for motion in frame
        thissum = countNonZero(fgmask);
        if (thissum > THRES) { // there is motion in this frame
            activeMotion = true;
            frameMotion = true;
            motionframecount +=1; // add to frames of motion
            motionstillcount = 0; // reset frames of still

            if (motionframecount == 1 ) {//first time we detect motion, log it
                if (pthread_mutex_lock(&fileMutex) !=0) {
                    printf("error with mutex lock in capture start\n");}
                FILE* file = fopen(DATA_FILE, "a+");
                if (file != NULL) {
                    time_t rawtime;
                    struct tm* timeinfo;
                    // Get current time
                    time(&rawtime);
                    timeinfo = localtime(&rawtime);
                    char timestamp[26];
                    // Format timesteamp
                    strftime(timestamp, sizeof(timestamp),"%a, %d %b %Y %H:%M:%S %z", timeinfo);
                    fprintf(file, "Motion Started at: %s\n", timestamp);
                    fclose(file);
                }
                else {
                    printf("file didn't open\n");
                }
                pthread_mutex_unlock(&fileMutex); 
                //printf("Motion Detected, %d, motionframecount: %d, motionstillcount: %d\n", thissum, motionframecount, motionstillcount);
            }

        } else { //there is not motion in this frame
            motionstillcount +=1; //add to still frames
            
            //if there was motion but not in this frame and it has been still for a few frames, motion has stopped
            if (activeMotion && !frameMotion && motionstillcount > 10) //we have detected motion before but not in this frame
            {
                if (pthread_mutex_lock(&fileMutex) !=0) {
                    printf("error with mutex lock in capture stop\n");}

                FILE* file = fopen(DATA_FILE, "a+");
                if (file != NULL) {
                    time_t rawtime;
                    struct tm* timeinfo;

                    // Get current time
                    time(&rawtime);
                    timeinfo = localtime(&rawtime);

                    char timestamp[26];
                    // Format timesteamp
                    strftime(timestamp, sizeof(timestamp),"%a, %d %b %Y %H:%M:%S %z", timeinfo);

                    fprintf(file, "Motion Stopped at: %s\n", timestamp);
                    fclose(file);
                }
                else {
                    printf("file didn't open\n");
                }
                pthread_mutex_unlock(&fileMutex); 
                //printf("Motion Stopped, %d, motionframecount: %d, motionstillcount: %d\n", thissum, motionframecount, motionstillcount);
                activeMotion = false; //reset motion detected
                motionframecount = 0;
                break;

            }
        }
        
        }

        if (motionframecount > 10000) {
            printf("lots of motion\n");
            break;
        }
    
    }

    return 0;
}
