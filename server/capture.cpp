/*
*/
#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>
#include <unistd.h>
#include <pthread.h>

#define DATA_FILE "/var/tmp/aesdsocketdata"

using namespace cv;
using namespace std;

extern "C" int capture_motion(pthread_mutex_t fileMutex);

int capture_motion(pthread_mutex_t fileMutex) {

    Mat frame, gray, frameDelta, thresh, firstFrame;
    vector<vector<Point> > cnts;
    VideoCapture camera(0, CAP_V4L2); //open camera
    int motionDetected = 0;
    int motionframecount = 0;
    
    //set the video size to 512x288 to process faster
    camera.set(3, 512);
    camera.set(4, 288);

    if (!camera.isOpened()) 
    {
        printf("error accessing camera\n");
        return -1;
    }
    printf("waiting on camera to warm up\n");
    sleep(3);
    camera.read(frame);

    //convert to grayscale and set the first frame
    cvtColor(frame, firstFrame, COLOR_BGR2GRAY);
    GaussianBlur(firstFrame, firstFrame, Size(21, 21), 0);

    while(camera.read(frame)) {

        //convert to grayscale
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(21, 21), 0);

        //compute difference between first frame and current frame
        absdiff(firstFrame, gray, frameDelta);
        threshold(frameDelta, thresh, 25, 255, THRESH_BINARY);
        
        dilate(thresh, thresh, Mat(), Point(-1,-1), 2);
        findContours(thresh, cnts, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        bool still_looping = true;
        motionframecount = 0;

        for(int i = 0; i< cnts.size() && still_looping; i++) {
            if(contourArea(cnts[i]) > 5000) {

                // motion detected in this frame
                motionDetected += 1; //overall motion detected
                motionframecount+= 1; //
                still_looping = false;
            }
            
        } ///end for loop for frame

        if (motionDetected == 1) {//first time we detect motion, log it
            if (pthread_mutex_lock(&fileMutex) !=0) {
                printf("error with mutex lock\n");}

            FILE* file = fopen(DATA_FILE, "a+");
            if (file != NULL) {
                time_t rawtime;
                struct tm* timeinfo;

                // Get current time
                time(&rawtime);
                timeinfo = localtime(&rawtime);

                char timestamp[27];
                // Format timesteamp
                strftime(timestamp, sizeof(timestamp),"%a, %d %b %Y %H:%M:%S %z", timeinfo);

                fprintf(file, "Motion Started at: %s\n", timestamp);
                fclose(file);
            }
            else {
                printf("file didn't open\n");
            }
            pthread_mutex_unlock(&fileMutex); 

            printf("Motion Detected\n");
        }

        

        if (motionDetected > 0 && motionframecount == 0) //we have detected motion before but not in this frame
        {
            if (pthread_mutex_lock(&fileMutex) !=0) {
                printf("error with mutex lock\n");}

            FILE* file = fopen(DATA_FILE, "a+");
            if (file != NULL) {
                time_t rawtime;
                struct tm* timeinfo;

                // Get current time
                time(&rawtime);
                timeinfo = localtime(&rawtime);

                char timestamp[27];
                // Format timesteamp
                strftime(timestamp, sizeof(timestamp),"%a, %d %b %Y %H:%M:%S %z", timeinfo);

                fprintf(file, "Motion Stopped at: %s\n", timestamp);
                fclose(file);
            }
            else {
                printf("file didn't open\n");
            }
            pthread_mutex_unlock(&fileMutex); 
            printf("motion stopped\n");
            break;
        }

        if (motionDetected > 1000) {
            printf("lots of motion\n");
            break;
        }
    
    }

    return 0;
}
