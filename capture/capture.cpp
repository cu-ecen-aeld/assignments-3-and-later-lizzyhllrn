
#include <opencv2/opencv.hpp>
//#include <opencv2/tracking.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/video/background_segm.hpp>
#include <unistd.h>

#define DATA_FILE "/var/tmp/aesdsocketdata"
#define THRES 200

using namespace cv;
using namespace std;
 
int main(int argc, char **argv) {


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

    printf("waiting on camera to warm up\n");
    sleep(2);

    while(camera.read(frame)) {
        //update the background model
        pBackSub->apply(frame, fgmask);
        frameNum += 1;

        //reset motion indicator for this frame
        frameMotion = false;

        //look for motion in frame
        thissum = countNonZero(fgmask);
        //printf("this sum: %d\n", thissum);
        if (frameNum > 30 && thissum > THRES) {
            activeMotion = true;
            frameMotion = true;
            motionframecount +=1;
        } else {
            motionstillcount +=1;
        }
        

        
        if (motionframecount == 1) {//first time we detect motion, log it
        /*
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
            */
           printf("Motion started: %d\n", thissum);
        }

        

        if (activeMotion && !frameMotion && motionstillcount > 10)  //we have detected motion before but not in this frame
        {
                /*
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
            */
            printf("motion stopped\n");
            activeMotion = false; //reset motion detected
            motionframecount = 0;
            //break;

        }

        if (motionframecount > 10000) {
            printf("lots of motion\n");
            break;
        }
    
    }

    return 0;
}
