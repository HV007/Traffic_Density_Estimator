#include <opencv2/opencv.hpp>
#include <bits/stdc++.h>

using namespace cv;
using namespace std;

bool finished;
Mat h, empty_frame;
vector<Point2f> pts_src;
vector<Point2f> pts_dst;

vector<Mat> frames;
vector<double> Queue;
vector<double> dynamic;
vector<double> baseline_queue;
vector<double> baseline_dynamic;
int frame_number = 0;
int resolve = 1;
int x = 5;
int space_threads = 1;
int time_threads = 5;
bool space_opt = false;
bool print_data = false;

void CallBackFunc(int event,int x,int y,int flags,void* userdata) {
    // Store the coordinates of corners of the road

    if(event==EVENT_LBUTTONDOWN) {
        pts_src.push_back(Point2f(x,y));
        if(pts_src.size() == 4) finished = true;
        return;
    }
}

Mat processImage(Mat frame) {
    // Transform and crop the frame to a perpendicular view

    if(pts_src.size() != 4) {
        // If corner coordinates are not stored, take input from the user

        namedWindow("Original Frame", 1);
        setMouseCallback("Original Frame", CallBackFunc, nullptr);
        finished = false;
        while(!finished){
            imshow("Original Frame", frame);
            waitKey(50);
        }

        pts_dst.push_back(Point2f(472, 52));
        pts_dst.push_back(Point2f(472, 830));
        pts_dst.push_back(Point2f(800, 830));
        pts_dst.push_back(Point2f(800, 52));
        h = findHomography(pts_src, pts_dst);

        Mat im_trans;
        warpPerspective(frame, im_trans, h, frame.size());
        empty_frame = im_trans(Rect(472, 52, 328, 778));
        resize(empty_frame, empty_frame, Size(empty_frame.cols/resolve, empty_frame.rows/resolve));

        return empty_frame;
    } else {
        Mat im_trans;
        warpPerspective(frame, im_trans, h, frame.size());
        Mat processed = im_trans(Rect(472, 52, 328, 778));
        resize(processed, processed, Size(processed.cols/resolve, processed.rows/resolve));

        return processed;
    }
}

double calcDiff(Mat img1, Mat img2) {
    // Calculate difference between two images (single number metric)

    Mat gray1, gray2, delta;
    cvtColor(img1, gray1, COLOR_BGR2GRAY);
    cvtColor(img2, gray2, COLOR_BGR2GRAY);
    absdiff(gray1, gray2, delta);

    return sum(delta)[0];
}




void get_frames(string video){
    VideoCapture cap(video);

    if(!cap.isOpened()) {
        cout << "Error opening video file " << video << "\n";
        exit(1);
    }

    int count = 0;
    while(true){
        Mat frame; 
        cap >> frame;
        if(frame.empty()) break;
        if(count%x == 0) {
            frame_number++;
            frames.push_back(processImage(frame));
        }
        count++;
    }
}

void normalise(vector<double> &vec) {
    double ma = 0;
    double mi = vec[0];
    for(double e : vec) {
        ma = max(ma, e);
        mi= min(mi,e);
    }
    for(double &e: vec) {
        e = (e-mi)/(ma-mi);
    }
}

struct thread_data {
   int c;
   int r;
};


void *cal_density_space(void *threadarg) {
    struct thread_data *my_data;
    my_data = (struct thread_data *) threadarg;
    int c = my_data->c;
    int r = my_data->r;

    double diff;
    double dyn_diff;

    Mat prev_frame;

    for(int i = 0; i < frame_number; i++) {
        int cols = frames[i].cols/space_threads;
        int rows = frames[i].rows/space_threads;
        Mat temp1 = frames[i](Rect(c*cols, r*rows, cols, rows));
        Mat temp2 = empty_frame(Rect(c*cols, r*rows, cols, rows));
        Mat temp3;
        if(i != 0) temp3 = prev_frame(Rect(c*cols, r*rows, cols, rows));
        diff = calcDiff(temp1, temp2);  
        if(i == 0) dyn_diff = 0.0;
        else dyn_diff = calcDiff(temp1, temp3);  
        prev_frame = frames[i];   
        Queue[i] += diff;
        dynamic[i] += dyn_diff;
    }
    pthread_exit(NULL);
}

void get_density_space() {

    pthread_t threads[space_threads][space_threads];
    pthread_attr_t attr;
    void *status;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(int i = 0; i < space_threads; i++) {
        for(int j = 0; j < space_threads; j++) {
            struct thread_data td;
            td.c = i; td.r = j;
            int rc = pthread_create(&threads[i][j], &attr, cal_density_space, (void *)&td);
            if (rc) {
                cout << "Error:unable to create thread," << rc << endl;
                exit(-1);
            }
        }
    }
    pthread_attr_destroy(&attr);

    for(int i = 0; i < space_threads; i++) {
        for(int j = 0; j < space_threads; j++) {
            int rc = pthread_join(threads[i][j], &status);
            if (rc) {
                cout << "Error:unable to join," << rc << endl;
                exit(-1);
            }
        }
    }
}

void *cal_density_time(void *threadid) {
    int off = static_cast<int>(reinterpret_cast<intptr_t>(threadid)); 

    double diff;
    double dyn_diff;

    Mat prev_frame;

    for(int i = (off*frame_number)/time_threads; i < ((off+1)*frame_number)/time_threads; i++) {
        diff = calcDiff(frames[i], empty_frame);
        if(i == (off*frame_number)/time_threads) dyn_diff = 0.0;
        else dyn_diff = calcDiff(frames[i], prev_frame);  
        prev_frame = frames[i];   
        Queue[i] = diff;
        dynamic[i] = dyn_diff;
    }
    pthread_exit(NULL);
}

void get_density_time() {
    pthread_t threads[time_threads];
    pthread_attr_t attr;
    void *status;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for(int i = 0; i < time_threads; i++) {
        int rc = pthread_create(&threads[i], &attr, cal_density_time, (void *)i);
        if (rc) {
            cout << "Error:unable to create thread," << rc << endl;
            exit(-1);
        }
    }
    pthread_attr_destroy(&attr);

    for(int i = 0; i < time_threads; i++) {
        int rc = pthread_join(threads[i], &status);
        if (rc) {
            cout << "Error:unable to join," << rc << endl;
            exit(-1);
        }
    }
}

void get_density() {

    for(int i = 0; i < frame_number; i++) {Queue.push_back(0); dynamic.push_back(0);}

    if(space_opt) get_density_space();
    else get_density_time();

    normalise(Queue);
    normalise(dynamic);
}

void get_baseline(string file_name) {
    // get baseline Queue and dyn vector from csv
    ifstream fin(file_name);
  
    int count = 0;
  
    vector<string> row;
    string line, word;
  
    while (getline(fin, line)) {
  
        row.clear();
  
        // read an entire row and
        // store it in a string variable 'line'
        
  
        // used for breaking words
        stringstream s(line);
  
        // read every column data of a row and
        // store it in a string variable, 'word'
        while (getline(s, word, ',')) {
  
            // add all the column data
            // of a row to a vector
            row.push_back(word);
        }

        count++;
        if (count == 1 || count%x != 2%x) continue;
        baseline_queue.push_back(stold(row[1]));
        baseline_dynamic.push_back(stold(row[2]));

    }
    
}

void utility_cal(){
    double utility_queue = 0, utility_dynamic = 0;
    for(int i = 0;i < frame_number; i++){
        utility_queue+=((Queue[i]-baseline_queue[i])*(Queue[i]-baseline_queue[i]));
        utility_dynamic+=((dynamic[i]-baseline_dynamic[i])*(dynamic[i]-baseline_dynamic[i]));
    }    
    utility_queue = utility_queue/frame_number;
    utility_dynamic = utility_dynamic/frame_number;

    cout << "Utility of queue is : " << fixed << utility_queue << setprecision(5);
    cout<<"\n";
    cout << "Utility of dynamic is : " << fixed << utility_dynamic << setprecision(5);
    cout<<"\n";

}

void printData() {
    // Normalise and print data

    cout << "Frame,Queue,Moving\n";
    for(int i = 0; i < frame_number; i++) cout << i << "," << Queue[i] << "," << dynamic[i] << "\n";
}

int main(int argc, char* argv[]) {

    time_t start, end;
    double time_taken;
    time(&start);

    // if(argc < 3) {
    //     cout << "Error: No input video and/or image provided\n Please execute as ./part3 <video> <empty_frame>\n";
    //     exit(1);
    // } else if(argc > 8) {
    //     cout << "Warning: More than three arguments provided\n";             // ./part3 <video> <empty_frame> xrd x r d baseline.csv     
    // }

    // get_baseline(argv[argc-1]);

    
    // if (argc>=4){
    //     string arg=argv[3];
    //     if (arg=="x") x=stoi(argv[4]);
    //     else if (arg=="r") resolve=stoi(argv[4]);
    //     else if (arg=="d") print_data=true;
    //     else if (arg=="xr") x=stoi(argv[4]),resolve=stoi(argv[5]);
    //     else if (arg=="xd") x=stoi(argv[4]),print_data=true;
    //     else if (arg=="rd") resolve=stoi(argv[4]),print_data=true;
    //     else if (arg=="xrd") x=stoi(argv[3]),resolve=stoi(argv[4]),print_data=true;
    // }

    Mat im_src = imread(argv[2]);
    if(im_src.empty()) {
        cout << "Error reading image " << argv[2] << "\n";
        exit(1);
    }
    processImage(im_src);
    destroyAllWindows();

    get_frames(argv[1]);
    get_density();
    get_baseline("baseline.csv");

    utility_cal();

    if (print_data) printData();

    time(&end);
    time_taken = double(end - start);
    cout << "Time taken by program is : " << fixed << time_taken << setprecision(5);
    cout << " sec " << endl;

}
