#include<iostream>
#include"opencv2\opencv.hpp"
//#include"opencv2\highgui.hpp"
#include<vector>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace std;
using namespace cv;
#define CAMERA_WIDTH 1280
#define CAMERA_HEIGHT 960
#define RESIZE_RATE 0.5

#define REAL_WIDTH 0.68
#define REAL_HEIGHT 0.50
#define ARM_LIMIT 0.42

#define FIELD_WIDTH 0.7
#define FIELD_HEIGHT 0.4

#define RED_HSV 0
#define BLUE_HSV 90
#define YELLOW_HSV 32

vector<char> detected_home;


#define DEBUG


cv::Point maxPoint(vector<cv::Point> contours);
cv::Point minPoint(vector<cv::Point> contours);
vector<cv::Point> resultBox(30);
void orbBasic(cv::Mat img1, cv::Mat img2);
double shapeMatchBasic(cv::Mat, cv::Mat);
cv::Mat poline(vector<vector<Point>>, Mat);
int color_Detection(Mat img);
//void poLine(vector<vector<Point>> contours);

typedef struct {
	int x;
	int y;
}Object_XY;

typedef struct {
	float x=0;
	float y=0;
}Object_Distance;



/*ロボットアームの可動域フィールドを指定して幅で区切った時の
　物体の座標位置を表す。
　原点は画像の左上、横70cm,縦40cmの長方形を10cm×10cmのマスで区切っている
*/
typedef struct {
	int x = 0;
	int y = 0;
}Object_Point;


Object_Distance object_calc(Object_XY);
float distance_calc(Object_Distance dis);
Object_Point Object_Point_calc(Object_Distance);



vector<Object_Point> Object_Detecter(void) {
	//カメラからの読み込み
	cv::VideoCapture cap(1);
	cap.set(cv::CAP_PROP_FPS, 30.0);
	cap.set(CV_CAP_PROP_FRAME_WIDTH, CAMERA_WIDTH);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, CAMERA_HEIGHT);
	if (!cap.isOpened()) {
		printf("sss\n\n");
		return vector<Object_Point>(0);
	}
	cv::Mat image;
	cap >> image;
	//リサイズする。
	cv::resize(image, image, cv::Size(), RESIZE_RATE, RESIZE_RATE);
	cv::Mat origen=image.clone();
	//画像保存
#ifdef DEBUG
	//cv::imwrite("1TEST_SIZE.png", image);

#endif // DEBUG

	//画像読み込み
	//cv::Mat image = cv::imread("find_kukei.png");
	cv::Mat copy;
	copy = image.clone();

	// グレースケール画像に変換
	cv::Mat grayImage;
	cv::cvtColor(image, grayImage, CV_BGR2GRAY);
	// 二値画像に変換
	cv::Mat binaryImage;
	const double threshold = 120.0;
	const double maxValue = 255.0;
	cv::threshold(grayImage, binaryImage, threshold, maxValue, cv::THRESH_BINARY);
	//cv::threshold(grayImage, binaryImage, 0.0, 255.0, CV_THRESH_OTSU);
	//白黒反転
	binaryImage = ~binaryImage;
	
/*白検出の処理*/
	///*明りの反射などを考慮して閾値を上げて、その後白黒反転する*/
	//cv::threshold(grayImage, binaryImage, 20.0, 255.0, CV_THRESH_OTSU);	//白いものの輪郭検出
	//binaryImage = ~binaryImage;
	//cv::threshold(binaryImage, binaryImage, 10.0, 255.0, CV_THRESH_BINARY_INV);
/*↑白検出の処理*/


	vector<vector<cv::Point>> contours,contCopy(100);
	vector<cv::Vec4i> hierarchy;
	cv::findContours(binaryImage, contours, hierarchy, /*CV_RETR_EXTERNAL*/0, CV_CHAIN_APPROX_SIMPLE);
	vector<cv::Rect> copyContours(100);

	drawContours(binaryImage, contours, -1,Scalar(111, 0, 222));
	imshow("image", binaryImage);
	cvWaitKey();

	double area;
	int pointCnt = 0;
	//各輪郭の最大最小座標を求める
	for (int i = 0; i<contours.size(); i++) {

		area = contourArea(contours.at(i), false);//検出した矩形の面積を求めている。
		printf("面積は[ %f ] \n", area);

		if ((area < 25000) && (area>150)) {
			cv::Point minP = minPoint(contours.at(i));
			cv::Point maxP = maxPoint(contours.at(i));
			cv::Rect rect(minP, maxP);
			contCopy[pointCnt] = contours[i];
			copyContours[pointCnt] = Rect(rect.x, rect.y, rect.width , rect.height );

			if (rect.x>20) {
				copyContours[pointCnt].x = rect.x-20; 
				copyContours[pointCnt].width = rect.width + 20;
			}
			if (rect.y>20) { 
				copyContours[pointCnt].y = rect.y - 20;
				copyContours[pointCnt].height = rect.height + 20;
			}
			if (binaryImage.cols > rect.x + rect.width + 40) { copyContours[pointCnt].width = rect.width + 40; }
			if (binaryImage.rows > rect.y + rect.height + 40) { copyContours[pointCnt].height = rect.height + 40; }

			cv::Point center((maxP.x + minP.x) / 2, (maxP.y + minP.y) / 2);
			printf("minP:::x:%d,,,y:%d\n\n", minP.x, minP.y);
			printf("maxP:::x:%d,,,y:%d\n\n", maxP.x, maxP.y);
			printf("中心:::x:%d,,,y:%d\n\n", (maxP.x+minP.x)/2, (maxP.y+minP.y)/2);
			cout << area << endl;

			//矩形を描く
			//cv::rectangle(image, rect, cv::Scalar(0, 255, 0), 2, 8);
			cv::rectangle(image, cv::Rect(minP,maxP), cv::Scalar(0, 255, 0), 2, 8);
			cv::circle(image, center, 4, cv::Scalar(100, 0, 0), 2, cv::LINE_AA, 0);
			//resultBox[i] = center;
			resultBox[pointCnt] = center;
			// 画像，テキスト，位置（左下），フォント，スケール，色，線太さ，種類
			cv::putText(image, std::to_string(area), cv::Point(minP), cv::FONT_HERSHEY_SIMPLEX, 0.4 , cv::Scalar(0, 0, 200), 1, CV_AA);
			pointCnt++;
			cv::putText(image, std::to_string(pointCnt), cv::Point(maxP.x-30,maxP.y-30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(200, 0, 200), 1.2, CV_AA);

		}
		
	}

	//
	printf("\n\n\n");
	for (int i=0;i<resultBox.size();i++) {
		printf("%d 番目の引き渡すパラメータ\nｘ：%d\nｙ：%d\n",i,resultBox[i].x,resultBox[i].y);
	}
	//
	 //cv::Point aminP = minPoint(contours.at(0));
	 //cv::Point amaxP = maxPoint(contours.at(0));
	//cv::Rect rect(cv::Point(aminP),cv::Size(amaxP.x-aminP.x,amaxP.y-aminP.y));
	//cv::Rect rect( (float)minP.x,(float)minP.y,(float)maxP.x- (float)minP.x,(float)maxP.y-(float)minP.y);
	//cv::Rect rect(aminP, amaxP);
	//cv::Mat detectTemp = imread("IMG_20190223_154107.png", IMREAD_GRAYSCALE);
	//Mat orbImg(binaryImage, rect);
	//orbBasic(detectTemp,orbImg);

//	cv::Mat detectTemp = imread("./img/nippa.jpg",IMREAD_GRAYSCALE);
	
	//cv::threshold(detectTemp, detectTemp, 100 + 40, 255.0, cv::THRESH_BINARY);

	//detectTemp = ~detectTemp;

//	cv::resize(detectTemp, detectTemp, Size(0,0),1, 1);

	////cvtColor(detectTemp, detectTemp, CV_BGR2GRAY);
	//cv::Mat orbImg(grayImage);

//	// ROI を利用してコピー
	//cv::Mat p_mat(detectTemp.rows,detectTemp.cols, CV_8U );
	//Mat dst_gray = grayImage.clone();
	//orbImg.copyTo(p_mat);
	//orbBasic(detectTemp, copy);
	//poline(contCopy, binaryImage);
	//shapeMatchBasic(detectTemp, binaryImage);

	vector<Object_Distance> distance;
	vector<float> arm_distance;


	for (int i = 0; i < pointCnt; i++) {
		//cv::Mat roi_mat(grayImage, copyContours.at(i));
		//Mat roi_mat(grayImage, copyContours.at(i));
		//Mat p=grayImage(copyContours.at(i));
		//Mat p = image(copyContours.at(i));
		/**/
		Mat roi = grayImage(copyContours.at(i));
		//shapeMatchBasic(detectTemp, roi);
		/**/
		//p.copyTo(p_mat);
		//normalize(roi_mat, roi_mat,NORM_HAMMING);
//		orbBasic(detectTemp, roi);

		Object_XY object;
		//Object_Distance distance;
		object.x = resultBox.at(i).x;
		object.y = resultBox.at(i).y;
		distance.push_back(object_calc(object));

		arm_distance.push_back( distance_calc(distance.at(i)) );

		printf("Ddistance.x : %f", distance.at(i).x);
		printf("Ddistance.y : %f", distance.at(i).y);
	}
	vector<Rect> object_rect;
	vector<Object_Distance> Distance_for_IKSolver;	//ロボットアームの位置から物体へのの距離を(x,y)で表現
	for (int i = 0,p=0; i < pointCnt; i++) {
		/*物体が*/
		//if (distance.at(i).x < ARM_LIMIT || (distance.at(i).x<0 && distance.at(i).x>ARM_LIMIT*(-1)) || distance.at(i).y < ARM_LIMIT) {
		if(arm_distance.at(i)>ARM_LIMIT){
			//Distance_for_IKSolver.push_back(distance.at(i));
			printf("\n %d 番目対象物体がARM_LIMITを超えています\n", i + 1);
			printf("違反IK@DISTANCE.x : %f		", distance.at(i).x);
			printf("違反IK@DISTANCE.y : %f\n", distance.at(i).y);
			printf("違反IK@DISTANCE : %f\n", arm_distance.at(i));
			continue;
		}
		Distance_for_IKSolver.push_back(distance.at(i));
		object_rect.push_back(copyContours.at(i));
		printf("DISTANCE.x : %f		", Distance_for_IKSolver.at(p).x);
		printf("DISTANCE.y : %f\n", Distance_for_IKSolver.at(p).y);
		p++;
		//printf("ARM_DISTANCE : %f\n", arm_distance.at(i));

	}
	printf("distance_の要素 : %d\n", distance.size());
	printf("IK_の要素 : %d\n", Distance_for_IKSolver.size());
	/**/
	vector<Object_Point> Object_Pointer;
	for (int i = 0; i < Distance_for_IKSolver.size(); i++) {
		Object_Pointer.push_back(Object_Point_calc(Distance_for_IKSolver.at(i)));
		printf("\n添え字Xの値:%d", Object_Pointer.at(i).x);
		printf("	添え字Yの値:%d", Object_Pointer.at(i).y);
	}
	printf("\n添え字サイズ%d", Object_Pointer.size());

	for (int i = 0; i < Distance_for_IKSolver.size(); i++) {
		Mat roi = origen(object_rect.at(i));
		color_Detection(roi);
		
	}

	/**/


	//poline(contours,binaryImage);
	//cv::Mat roi_mat(p_mat, copyContours.at(1));
	//orbBasic(detectTemp, orbImg);
	//orbBasic(orbImg, detectTemp);
	//**/
	//orbBasic((cv::Mat)detectTemp, (cv::Mat)orbImg);
	//orbBasic(detectTemp, roi_mat);
	cv::imshow("testDATA", grayImage);
	cv::imshow("grayscale", binaryImage);
	cv::imshow("test", image);
	//cv::imshow("MOTODATA", copy);
	cvWaitKey(0);
	cvDestroyAllWindows();

	return Object_Pointer;
}

int main(void) {
	vector<Object_Point> dis;

	dis=Object_Detecter();

#ifdef DEBUG
	printf("\n正常に終了してます\n");
	printf("\nhairetu のサイズ : %d		\n", dis.size());
	printf("\n正常に終了してます\n");
#endif // DEBUG

	return 0;
}

float distance_calc(Object_Distance dis) {
	float i;
	i = (dis.x*dis.x + dis.y*dis.y);
	i=sqrt(i);
#ifdef DEBUG
	printf("\nARMからの直線距離:%f", i);
#endif // DEBUG

	return i;
}
/*

#define FIELD_WIDTH 0.7
#define FIELD_HEIGHT 0.4

*/

/*Object_Distanceの情報を元に二次元配列に落とし込む
　二次元配列で要素数は[FIELD_HEIGHT/0.1][FIELD_WIDTH/0.1]を想定。(0.1m四方の正方形で区画分けしていく。)

*/
Object_Point Object_Point_calc(Object_Distance dis) {
	Object_Point point;
	int x, y;
	/*Yの処理*/
	y = (int)(dis.y * 10);
	/*Xの処理*/
	x = (int)(dis.x * 100);
	if(x>0){
		x = ((x / 5) + 1) / 2;
	}
	else {
		x = ((x / 5) - 1) / 2;
	}
	x = x + 3;
	point.x = x;
	point.y = y;
#ifdef DEBUG
	printf("\nDIS.X:%f		添え字Xの値:%d", dis.x, point.x);
	printf("\nDIS.Y:%f		添え字Yの値:%d", dis.y, point.y);
#endif // DEBUG
	return point;

}




//最小座標を求める
cv::Point minPoint(vector<cv::Point> contours) {
	double minx = contours.at(0).x;
	double miny = contours.at(0).y;
	for (int i = 1; i<contours.size(); i++) {
		if (minx > contours.at(i).x) {
			minx = contours.at(i).x;
		}
		if (miny > contours.at(i).y) {
			miny = contours.at(i).y;
		}
	}
	return cv::Point(minx, miny);
}


//最大座標を求める
cv::Point maxPoint(vector<cv::Point> contours) {
	double maxx = contours.at(0).x;
	double maxy = contours.at(0).y;
	for (int i = 1; i<contours.size(); i++) {
		if (maxx < contours.at(i).x) {
			maxx = contours.at(i).x;
		}
		if (maxy < contours.at(i).y) {
			maxy = contours.at(i).y;
		}
	}
	return cv::Point(maxx, maxy);
}
/*ロボットアームは画像の真ん中の上部に位置するものとする
*/
Object_Distance object_calc(Object_XY object) {
	Object_Distance distance;
	distance.x = object.x*(REAL_WIDTH / (CAMERA_WIDTH * RESIZE_RATE));
	if (distance.x > REAL_WIDTH / 2) {
		distance.x -= REAL_WIDTH / 2;
	}
	else if (distance.x < REAL_WIDTH / 2) {
		distance.x = (REAL_WIDTH / 2 - distance.x)*(-1);
	}
	distance.y = object.y*(REAL_HEIGHT / (CAMERA_HEIGHT * RESIZE_RATE));
#ifdef DEBUG
	printf("distance.x : %f\n", distance.x);
	printf("distance.y : %f\n", distance.y);
#endif // DEBUG
	return distance;
}
/**/
bool IsSimilar(int ref, int target, int thr) {
	if (abs(ref - target)<thr)return 1;
	else if (abs(ref - target + 180)<thr || abs(ref - target - 180)<thr)return 1;
	else return 0;
}
bool IsSimilarSV(int ref, int target, int thr) {
	if (abs(ref - target)<thr)return 1;
	else if (abs(ref - target + 255)<thr || abs(ref - target - 255)<thr)return 1;
	else return 0;
}

int color_Detection(Mat img) {
	Mat hsv_img;
	cvtColor(img, hsv_img, CV_BGR2HSV);

	Mat green_img = Mat(Size(img.cols, img.rows), CV_8U);
	Mat yellow_img = Mat(Size(img.cols, img.rows), CV_8U);

	int green_pixels = 0;
	int red_pixels = 0;
	int blue_pixels = 0;
	int yellow_pixels = 0;
	int brown_pixels = 0;
	int orange_pixels = 0;
	for (int y = 0; y < hsv_img.rows; ++y) {
		for (int x = 0; x < hsv_img.cols; ++x) {
			//OpenCVでは色相Hの範囲が0~180になっていることに注意
			//緑は120度 OpenCVでは 120/2=60 あたり

			if (
				hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 1]>100 &&
				IsSimilar(hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 0], RED_HSV, 17)
				)
			{
				green_img.data[y * green_img.step + x * green_img.elemSize()] = 255;
				red_pixels++;
			}
			if (
				hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 1]>100 &&
				IsSimilar(hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 0], BLUE_HSV, 17)
				)
			{
				green_img.data[y * green_img.step + x * green_img.elemSize()] = 255;
				blue_pixels++;
			}

			if (
				//IsSimilarSV(hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 1], 182, 100) &&
				IsSimilar(hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 0], 10, 20)&&
				IsSimilarSV(hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 2], 76,40 )
				)
			{
				green_img.data[y * green_img.step + x * green_img.elemSize()] = 255;
				brown_pixels++;
			}

			if (
				IsSimilar(hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 1], 75 * 2.55, 20 * 2.5) &&
				IsSimilar(hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 0], 10, 17)
				)
			{
				green_img.data[y * green_img.step + x * green_img.elemSize()] = 255;
				orange_pixels++;
			}

			if (
				hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 1]>100 &&
				IsSimilar(hsv_img.data[y * hsv_img.step + x * hsv_img.elemSize() + 0], YELLOW_HSV, 17)
				)
			{
				green_img.data[y * green_img.step + x * green_img.elemSize()] = 255;
				yellow_pixels++;
			}


		}
	}

	imshow("IMAGE", img);
	imshow("green-area", green_img);
	int red, blue, yellow, orange, brown;
	red = red_pixels / img.cols / img.rows * 100;
	blue = blue_pixels / img.cols / img.rows * 100;
	yellow = yellow_pixels / img.cols / img.rows * 100;
	brown = brown_pixels / img.cols / img.rows * 100;
	orange = orange_pixels / img.cols / img.rows * 100;
	cout << "赤色：" << red_pixels << " pixels; " << (double)red_pixels / img.cols / img.rows * 100 << " %" << endl;
	cout << "青色：" << blue_pixels << " pixels; " << (double)blue_pixels / img.cols / img.rows * 100 << " %" << endl;
	cout << "黄色：" << yellow_pixels << " pixels; " << (double)yellow_pixels / img.cols / img.rows * 100 << " %" << endl;
	cout << "茶色：" << brown_pixels << " pixels; " << (double)brown_pixels / img.cols / img.rows * 100 << " %" << endl;
	cout << "橙色：" << orange_pixels << " pixels; " << (double)orange_pixels / img.cols / img.rows * 100 << " %" << endl;
	cvWaitKey();
	vector<int> v;
	/*赤を最大検出*/
	if (red > 8 && red > blue&&red > yellow&&red > brown) {
		detected_home.push_back(0);
	}
	/*青最大検出*/
	else if (blue > yellow&&blue > brown&&blue > orange) {
		detected_home.push_back(1);
	}
	/*黄色と茶色を一定量検出*/
	else if (yellow + brown > 15) {
		detected_home.push_back(2);
	}
	else if (yellow > orange&&yellow > brown) {
		detected_home.push_back(3);
	}
	else {
		detected_home.push_back(4);
	}

	return 0;
}

double shapeMatchBasic(cv::Mat temp, cv::Mat roi) {
	static char lacbelCnt = 0;
	const double thres = 0.01;	//閾値
	Mat tempcopy = temp.clone();
	double result = cv::matchShapes(temp, roi, CONTOURS_MATCH_I1, 0);
	cout << result << endl;
	lacbelCnt++;
	if (result < thres) { 
		printf("形状% d 番目\n", lacbelCnt);
		cout << "良好なマッチングです" << endl; }
	//imshow("temp", tempcopy);
	imshow(to_string(lacbelCnt), roi);
	cvWaitKey(0);
	return result;
}



//
//void orbBasic(cv::Mat img1,cv::Mat img2)
//{
//	// 画像の読み込み
//	//Mat img1 = imread(imagePath1, IMREAD_GRAYSCALE);
//	//Mat img2 = imread(imagePath2, IMREAD_GRAYSCALE);
//	/**/
//	float ratio = 0.8;
//	// FeatureDetectorオブジェクト
//	//Ptr<Feature2D> detector = ORB::create();//7
//	//Ptr<Feature2D> detector2 = ORB::create();//7
//
//	Ptr<Feature2D> detector = ORB::create(80, 1.35f, 4, 7, 0, 2, ORB::HARRIS_SCORE, 31);//7
//	Ptr<Feature2D> detector2 = ORB::create(400, 1.25f, 5, 31, 0, 2, ORB::HARRIS_SCORE, 31);//7
//
//	//Ptr<Feature2D> detector = BRISK::create(120,3,0.6f);
//	//Ptr<Feature2D> detector2 = BRISK::create(120, 3, 0.6f);
//
//
//
//	// DescriptorMatcherオブジェクトの生成
//	//Ptr<DescriptorMatcher> matcher = DescriptorMatcher::create("BruteForce-Hamming");
//	vector<DMatch> matches, goodMatches;
//	//BFMatcher matcher(NORM_HAMMING, true);
//	BFMatcher matcher(NORM_HAMMING,true);
//
//
//	// 特徴点情報を格納するための変数
//	vector<KeyPoint> keypoints1;
//	vector<KeyPoint> keypoints2;
//	
//	//detector->detect(img1, keypoints1);
//	//detector->detect(img2, keypoints2);
//
//	// 画像の特徴情報を格納するための変数
//	Mat descriptor1;
//	Mat descriptor2;
//
//	///// 特徴点抽出の実行と特徴記述の計算を実行
//	detector2->detectAndCompute(img1, cv::noArray(), keypoints1, descriptor1);
//	detector->detectAndCompute(img2, cv::noArray(), keypoints2, descriptor2);
//	if (keypoints1.empty() || keypoints2.empty()) { 
//		cout << "keypoints_EMPTY" << endl;
//		return; }
//	
//	// 特徴点のマッチング情報を格納する変数
//	vector<DMatch> dmatch;
//
//	// 特徴点マッチングの実行
//	matcher.match(descriptor1, descriptor2, dmatch);
//	//matcher.radiusMatch(descriptor1, descriptor2, &dmatch, 500,noArray(),false);
//	//matcher.knnMatch(descriptor1, descriptor2, dmatch, 500);
//	/*aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa*/
//	cout << "Matches1-2:" << dmatch.size() << endl;
//	//cout << "Matches2-1:" << matches21.size() << endl;
//
//	// ratio test proposed by David Lowe paper = 0.8
//	vector<DMatch> good_matches1;
//	const float threshold = 50.0f;
//	// Yes , the code here is redundant, it is easy to reconstruct it ....
//
//	for (vector<DMatch>::iterator  it=dmatch.begin();it!=dmatch.end(); ++it) {
//		if (threshold > it->distance)
//			cout << it->distance << endl;
//			good_matches1.push_back(*it);
//	}
//
//
//
//	cout << "Good matches1:" << good_matches1.size() << endl;
//	
//	/*aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa*/
//	
//	//for(auto it=matcher.be)
//	//for (float i = 0; i < keypoints1.size()&&i<keypoints2.size(); i++) {
//
//	//	printf("   Match\nX::%d   Y::%d", keypoints1[i].pt.x, keypoints1[i].pt.y);
//	//	printf("   Match\nX2::%d   Y2::%d", keypoints2[i].pt.x, keypoints2[i].pt.y);
//	//	//printf("   Match\nX::%f   Y::%f", dmatch.pt.x, keypoints1[i].pt.y);
//	//}
//
//	printf("おわり");
//	// マッチング結果画像の作成
//	Mat result;
//	drawMatches(img1, keypoints1, img2, keypoints2, good_matches1, result);
//	imshow("matching", result);
//
//	cvWaitKey(0);
//	return;
//}



//
//Mat testKukei(void) {
//	vector<vector<Point>> contours;
//	CvSeq* contours;  //hold the pointer to a contour in the memory block
//	CvSeq* result;   //hold sequence of points of a contour
//	CvMemStorage *storage = cvCreateMemStorage(0); //storage area for all contours
//
//	while (true)
//	{
//		result = cvApproxPoly(contours, sizeof(CvContour), storage, CV_POLY_APPROX_DP, cvContourPerimeter(contours)*0.02, 0);
//		//cv::approxPolyDP(contours, approx, 0.01 * cv::arcLength(*contour, true), true);
//
//		/*****************置換*********************************************/
//		const int conThree = 3;
//		int i = 0;
//		if (result->total == conThree) {
//
//			CvPoint *pt[conThree];
//			//CvPoint *pt = new CvPoint[conThree];
//
//			//CvPoint  *pt;
//			//pt = (CvPoint*)malloc(sizeof(CvPoint)*conThree);
//			for (i = 0; i < conThree; i++) {
//				pt[i] = (CvPoint*)cvGetSeqElem(result, i);
//			}
//			for (i = 0; i < conThree - 1; i++) {
//				cvLine(frame, *pt[i], *pt[i + 1], cvScalar(255, 0, 0), 4);
//			}
//			cvLine(frame, *pt[i], *pt[0], cvScalar(255, 0, 0), 4);
//
//			printf("三角形X::%d///Y::%d\n", pt[0]->x, pt[0]->y);
//
//		}
//		const int conFor = 4;
//		//int i = 0;
//		if (result->total == conFor) {
//
//			CvPoint *pt[conFor];
//			//CvPoint *pt = new CvPoint[conThree];
//
//			//CvPoint  *pt;
//			//pt = (CvPoint*)malloc(sizeof(CvPoint)*conThree);
//			for (i = 0; i < conFor; i++) {
//				pt[i] = (CvPoint*)cvGetSeqElem(result, i);
//			}
//			for (i = 0; i < conFor - 1; i++) {
//				cvLine(frame, *pt[i], *pt[i + 1], cvScalar(255, 0, 255), 4);
//			}
//			cvLine(frame, *pt[i], *pt[0], cvScalar(255, 0, 255), 4);
//
//			printf("四角形X::%d///Y::%d\n", pt[0]->x, pt[0]->y);
//
//		}
//
//		//obtain the next contour
//		contours = contours->h_next;
//	}
//}

cv::Mat poline(vector<vector<Point>> contours , Mat img) {
	
	//輪郭の数
	int roiCnt = 0;

	//輪郭のカウント   
	int i = 1;
	int conCnt = 0;

	std::vector< cv::Point > approx;

	cv::drawContours(img, contours, -1, Scalar(100, 100, 255),1,8,noArray());
	imshow("sss", img);
	cvWaitKey(0);
	if (contours.size() <= 0) {
		cout << "ERROR" << endl;
		return Mat(0,0,0);
	}
	
	cv::approxPolyDP((contours.at(i)), approx, 0.01 * cv::arcLength((contours.at(i)), true), true);
	cv::drawContours(img, contours.at(i), -1, Scalar(100, 100, 255));
	imshow("DrawCONTOUR", img);
	//cvWaitKey();

	for (vector<Point> contour = (contours.at(conCnt)); conCnt < contours.size(); conCnt++) {

		std::vector< cv::Point > approx;

		//輪郭を直線近似する
		cv::approxPolyDP((contour), approx, 0.01 * cv::arcLength((contour), true), true);

		cv::drawContours(img, contour, -1, Scalar(100, 100, 255));
		imshow("DrawCONTOUR", img);
		cvWaitKey(0);
			////青で囲む場合            
			//cv::polylines(img, approx, true, cv::Scalar(255, 0, 0), 2);
			//std::stringstream sst;
			//cv::putText(img, sst.str(), approx[conCnt], CV_FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 128, 0));

			////輪郭に隣接する矩形の取得
			//cv::Rect brect = cv::boundingRect(cv::Mat(approx).reshape(2));
			//vector<Mat> roi;
			//roi[roiCnt]= cv::Mat(img, brect);

			////入力画像に表示する場合
			////cv::drawContours(imgIn, contours, i, CV_RGB(0, 0, 255), 4);

			////表示
			//cv::imshow("label" + std::to_string(roiCnt + 1), roi[roiCnt]);

			roiCnt++;

			//念のため輪郭をカウント
			if (roiCnt == 99)
			{
				break;
			}
		

		i++;
	}

	//全体を表示する場合
	cv::imshow("coun", img);
	cvWaitKey(0);
	return img;
}