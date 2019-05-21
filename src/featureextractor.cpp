#include "featureextractor.h"

FeatureExtractor::FeatureExtractor() {
}

void FeatureExtractor::setup(std::string path)
{
    this->videoFilePath = path;
    this->frameCounter = 0;
    this->luminance = -1.0;
	this->prevHist = NULL;
	this->rythm = -1.0;

	this->avgColors.clear();
	for (int i = 0; i < 3; i++) {
		this->avgColors.push_back(-1.0);
	}

    ofFile fileToRead(path);
    fileToRead.copyTo(tempFilename, true, true);
}


double FeatureExtractor::getLuminance()
{
    return this->luminance;
}

vector<double> FeatureExtractor::getAvgColors()
{
	return avgColors;
}

double FeatureExtractor::getRythm()
{
	return rythm;
}

void FeatureExtractor::calculate() {

    if (videoFilePath == "?") return;

    videoPlayer.load(tempFilename);
    videoPlayer.setLoopState(OF_LOOP_NONE);
    videoPlayer.play();

    ofLog(OF_LOG_NOTICE, "[LumExtractor] starts for " + videoFilePath + "...");

    double currentLumi;
	vector<double> currentColors;

    while (true) {

        videoPlayer.update();
        if (videoPlayer.getPosition() >= 0.999) {
            break;
        }

        if (videoPlayer.isFrameNew()) {
            this->frameCounter++;
            if (frameCounter % frameStep == 0) {
                currentColors = this->calculateFrame();
                luminance += currentColors[3];
				rythm += this->calculateDiffBetweenFrames();
                ofLog(OF_LOG_WARNING, ofToString(currentColors[3]));

				for (int i = 0; i < 3; i++) {
					avgColors[i] += currentColors[i];
				}

            }
        }
    }

	rythm = rythm / (double)(frameCounter / frameStep);
	ofLog(OF_LOG_NOTICE, "[RythmExtractor] RYTHM = " + ofToString(rythm));

    luminance = luminance / (double)(frameCounter / frameStep);
    ofLog(OF_LOG_NOTICE, "[LumExtractor] LUMI = " + ofToString(luminance));

	for (int i = 0; i < 3; i++) {
		avgColors[i] /= (double)(frameCounter / frameStep);
	}

    videoPlayer.close();    
}

vector<double> FeatureExtractor::calculatePixel(ofPixels pixels, int i, int j, int vidWidth, int nChannels) {
	vector<double> colors;

	for (int k = 0; k < 3; k++) {
		colors.push_back((double)pixels[(j * vidWidth + i) * nChannels + k]);
	}

	return colors;
}

vector<double> FeatureExtractor::calculateFrame() {
    double result[4] = {0.0};

    ofPixels & pixels = videoPlayer.getPixels();
    int vidWidth = pixels.getWidth();
    int vidHeight = pixels.getHeight();
    int nChannels = pixels.getNumChannels();

    double currentLumi;
    for (int i = 0; i < vidWidth; i += skipStep) {
        for (int j = 0; j < vidHeight; j += skipStep) {
			vector<double> currentColors = calculatePixel(pixels, i, j, vidWidth, nChannels);
            currentLumi = rc * currentColors[0] + gc * currentColors[1] + bc * currentColors[2];
            
			for (int k = 0; k < 3; k++) {
				result[k] += currentColors[k];
			}
			result[3] += currentLumi;
        }
    }

	for (int k = 0; k < 4; k++) {
		result[k] /= (double)(vidWidth / skipStep * vidHeight / skipStep);
	}

    int n = sizeof(result) / sizeof(result[0]);
    vector<double> vresult(result, result+n);
    return vresult;
}

double FeatureExtractor::calculateDiffBetweenFrames() {

	ofxCvColorImage colorImg;
	ofxCvGrayscaleImage grayImg;

	colorImg.setFromPixels(videoPlayer.getPixels());
	grayImg = colorImg;

	IplImage* grayCVImg = grayImg.getCvImage();
	int g_bins = 32;
	CvHistogram* hist;
	{
		int hist_size[] = { g_bins };
		float g_ranges[] = { 0, 255 };
		float* ranges[] = { g_ranges };
		hist = cvCreateHist(
			1,
			hist_size,
			CV_HIST_ARRAY,
			ranges,
			1
		);
	}
	cvCalcHist(&grayCVImg, hist);

	double diff = 0;

	if (prevHist != NULL)
		diff = cvCompareHist(hist, prevHist, CV_COMP_INTERSECT);

	prevHist = hist;

	return diff;
}

void FeatureExtractor::convertPixels(ofPixels &inPixels, ofPixels &newPixels, int vidWidth, int vidHeight) {

    double currentLumi;
    for (int i = 0; i < vidWidth; ++i) {
        for (int j = 0; j < vidHeight; ++j) {
			vector<double> colors = calculatePixel(inPixels, i, j, vidWidth, 3);
            currentLumi = rc * colors[0] + gc * colors[1] + bc * colors[2];
            newPixels[j * vidWidth + i] = (int) currentLumi;
        }
    }
}
