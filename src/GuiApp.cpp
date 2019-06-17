/*
 * GuiApp.cpp
 *
 *  Created on: Oct 28, 2014
 *      Author: arturo
 */

#include "GuiApp.h"

inline bool ends_with(string const & value, string const & ending)
{
	if (ending.size() > value.size()) return false;
	return equal(ending.rbegin(), ending.rend(), value.rbegin());
}

void GuiApp::createMetadatasFiles() {

    for (int i = 0; i < (int)dir.size(); i++) {

        xmlHandler.clear();

        vector<string> spl = ofSplitString(dir.getPath(i), ".");
        string xmlFilePath = spl[0] + ".xml";
        if (xmlHandler.loadFile(xmlFilePath)) {
            ofLog(OF_LOG_NOTICE, xmlFilePath + " metadata file loaded");
        }
        else {
            ofLog(OF_LOG_NOTICE, xmlFilePath + " does not exist");

            int mainPos = xmlHandler.addTag("metadata");
            xmlHandler.pushTag("metadata");

            xmlHandler.addValue("name", ofSplitString(dir.getName(i), ".")[0]);
            xmlHandler.addValue("resolution", "?");

            xmlHandler.addValue("luminance", -1.0);
            xmlHandler.addValue("red", -1.0);
            xmlHandler.addValue("green", -1.0);
            xmlHandler.addValue("blue", -1.0);
			xmlHandler.addValue("rythm", -1.0);

            xmlHandler.addTag("edgeHistogram");
            xmlHandler.pushTag("edgeHistogram");
            xmlHandler.addValue("edge", -1.0);
            xmlHandler.popTag();

            xmlHandler.addTag("textureMoments");
            xmlHandler.pushTag("textureMoments");
            xmlHandler.addValue("tex", -1.0);
            xmlHandler.popTag();

            xmlHandler.popTag();
            xmlHandler.saveFile(xmlFilePath);

            ofLog(OF_LOG_NOTICE, xmlFilePath + " metadata file created");
        }
    }
    xmlHandler.clear();
}

void GuiApp::setupVidGrabber() {
    vidGrabber.setDeviceID(0);
    vidGrabber.setDesiredFrameRate(60);
    vidGrabber.initGrabber(camWidth, camHeight);
}

void GuiApp::checkVidGrabberDevices() {
    vector<ofVideoDevice> devices = vidGrabber.listDevices();

    for (size_t i = 0; i < devices.size(); i++) {
        if (devices[i].bAvailable) {
            ofLogNotice() << devices[i].id << ": " << devices[i].deviceName;
        }
        else {
            ofLogNotice() << devices[i].id << ": " << devices[i].deviceName << " - unavailable ";
        }
    }
}


void GuiApp::setup(){
	ofSetVerticalSync(true);

    // START SCREEN

    checkVidGrabberDevices();
    webCamPreviewFaceFinder.setup("haarcascade_frontalface_default.xml");
    setupVidGrabber();
    ofBackground(0);

    // VIDEO LIBRARY SCREEN

	upButton.addListener(this, &GuiApp::upButtonPressed);
	downButton.addListener(this, &GuiApp::downButtonPressed);
    addButton.addListener(this, &GuiApp::addButtonPressed);
    goBackButton.addListener(this, &GuiApp::goBackButtonPressed);

	nav.setup();
	nav.add(upButton.setup("up"));
	nav.add(downButton.setup("down"));
    nav.add(addButton.setup("add new video"));
    nav.add(goBackButton.setup("BACK"));

    ofBackground(255, 0, 144);

    dir.allowExt("mov");
    dir.allowExt("mp4");
    dir.listDir("videos/");
	dir.sort(); // in linux the file system doesn't return file lists ordered in alphabetical order

    createMetadatasFiles();
    worker.setup(dir);
    worker.startThread();

	for (int i = 0; i < (int)dir.size(); i++) {
		cout << dir.getPath(i) << endl;
		Thumbnail *t = new Thumbnail();

		thumbnails.push_back(t);
		thumbnails[i]->setup(dir.getPath(i));
        thumbnails[i]->set(thumbnailsOffset, 20 + 100 + (i%3) * (thumbnails[0]->thumbnailSize + 10),
            thumbnails[i]->thumbnailSize, thumbnails[i]->thumbnailSize);
    }

	playButton.addListener(this, &GuiApp::playButtonPressed);
	pauseButton.addListener(this, &GuiApp::pauseButtonPressed);
	stopButton.addListener(this, &GuiApp::stopButtonPressed);

	details.setup();

    detailsWidth = mainPlayerWidth;
    details.setSize(detailsWidth, detailsHeight);

    // VIDEO BASIC INFO
    details.add(videoName.setup("file", "", detailsWidth));
    details.add(videoResolution.setup("size", "", detailsWidth));

    // VIDEO NAVIGATION
    details.add(playButton.setup("play", detailsWidth));
    details.add(pauseButton.setup("pause", detailsWidth));
    details.add(stopButton.setup("stop", detailsWidth));

    // VIDEO FEATURES
    details.add(videoLuminance.setup("lumi", "", detailsWidth));
    details.add(videoR.setup("red", "", detailsWidth));
    details.add(videoG.setup("green", "", detailsWidth));
    details.add(videoB.setup("blue", "", detailsWidth));
    details.add(videoRythm.setup("rythm", "", detailsWidth));
    details.add(edgeHist.setup("edge hist", "", detailsWidth));

    // BUTTONS
    startButton.addListener(this, &GuiApp::startButtonPressed);
    manageButton.addListener(this, &GuiApp::manageButtonPressed);

    // START SCREEN

    startScreenNav.setup();
    startScreenNav.setPosition(camPreviewOffset, camPreviewOffset + camHeight + 30);
    startScreenNav.add(startButton.setup("START"));
    startScreenNav.add(manageButton.setup("MANAGE"));
	startScreenNav.add(numberOfFaces.setup("num of faces", ""));
	startScreenNav.add(rythmOfCaptured.setup("rythm", ""));

    playVideo();
	ofSetVerticalSync(false);
}

void GuiApp::update(){

    if (startScreenMode) {
        vidGrabber.update();

        if (vidGrabber.isFrameNew())
        {
            colorImg.setFromPixels(vidGrabber.getPixels());
            webCamPreviewFaceFinder.findHaarObjects(colorImg);
			numberOfFaces = ofToString(webCamPreviewFaceFinder.blobs.size());
        }
    }
    else {
        for (int i = 0; i < (int)thumbnails.size(); i++) {
            thumbnails[i]->video.update();
            thumbnails[i]->update();
        }
        mainPlayer.update();    
    }

}

void GuiApp::draw(){

    if (startScreenMode) {
        vidGrabber.draw(camPreviewOffset, camPreviewOffset);
        startScreenNav.draw();

        for (unsigned int i = 0; i < webCamPreviewFaceFinder.blobs.size(); i++)
        {
            ofRectangle cur = webCamPreviewFaceFinder.blobs[i].boundingRect;
            ofRect(cur.x + camPreviewOffset, cur.y + camPreviewOffset, cur.width, cur.height);
        }

        ofSetHexColor(0xffffff);
        ofNoFill();
    }
    else {
        nav.draw();
        details.draw();
        ofDrawBitmapString(texMomentsString, details.getPosition().x, details.getPosition().y + details.getHeight() + 20.0);

        if (dir.size() > 0) {
            ofSetColor(ofColor::white);

            for (int i = thumbnailIdxOffset; i < thumbnailIdxOffset + 3; i++) {
                if (i < (int)thumbnails.size() && i >= 0) {
                    thumbnails[i]->enabled = true;
                    thumbnails[i]->draw(thumbnailsOffset,
                        20 + 100 + (i - thumbnailIdxOffset) * (thumbnails[0]->thumbnailSize + 10));
                }
            }

            mainPlayer.draw(thumbnailsOffset + thumbnails[0]->thumbnailSize + 50, 20, mainPlayerWidth, mainPlayerHeight);
        }
    }

}

void GuiApp::playVideo() {

    if (currentVideo < thumbnails.size() && currentVideo >= 0) {
        string currentName = thumbnails[currentVideo]->name;

        mainPlayer.load(currentName);
        mainPlayer.setLoopState(OF_LOOP_NORMAL);
        mainPlayer.play();
		mainPlayer.setVolume(0.0);
        this->readXML(ofSplitString(dir.getPath(currentVideo), ".")[0] + ".xml");

        details.setPosition(thumbnailsOffset + thumbnails[0]->thumbnailSize + 50,
            20 + mainPlayerHeight + 10);

	}
}

void GuiApp::readXML(string videoXMLPath) {

    if (xmlHandler.loadFile(videoXMLPath)) {

        xmlHandler.pushTag("metadata");
        videoName = xmlHandler.getValue("name", "?");
        videoResolution = xmlHandler.getValue("resolution", "?");

        double getLumi = xmlHandler.getValue("luminance", -1.0);
        videoLuminance = ofToString(getLumi);

        double getR = xmlHandler.getValue("red", -1.0);
        double getG = xmlHandler.getValue("green", -1.0);
        double getB = xmlHandler.getValue("blue", -1.0);

        videoR = ofToString(getR);
        videoG = ofToString(getG);
        videoB = ofToString(getB);

        double getRythm = xmlHandler.getValue("rythm", -1.0);
        videoRythm = ofToString(getRythm);

        string edgesStr = "";
        // read edge histogram into string
        xmlHandler.pushTag("edgeHistogram");
        int numberOfEdgeTypes = xmlHandler.getNumTags("edge");
        for (int i = 0; i < numberOfEdgeTypes; ++i) {
            edgesStr += ofToString(xmlHandler.getValue("edge", -1.0, i), 3);
            if (i < numberOfEdgeTypes-1)
                edgesStr += " | ";
        }
        xmlHandler.popTag();
        edgeHist = edgesStr;

        string texStr = "";
        // read texture moments into string
        xmlHandler.pushTag("textureMoments");
        int numberOfTextureMoments = xmlHandler.getNumTags("tex");
        for (int i = 0; i < numberOfTextureMoments; ++i) {
            texStr += ofToString(xmlHandler.getValue("tex", -1.0, i), 3);
            if (i < numberOfTextureMoments-1)
                texStr += " | ";

            if (i % 8 == 7) // :)
                texStr += "\n";
        }
        xmlHandler.popTag();
        texMomentsString = texStr;

//        ofSetColor(225);
//        verdana14.drawString(texStr, 200, 200);
        ofLog(OF_LOG_NOTICE, texStr);
//        texMoments = texStr;

    }
}


void GuiApp::downButtonPressed() {
    if (thumbnailIdxOffset + 3 < (int)thumbnails.size()) {

        thumbnailIdxOffset += 3;
        currentVideo = thumbnailIdxOffset;

        for (int i = 0; i < thumbnailIdxOffset; i++)
			thumbnails[i]->enabled = false;
		
		playVideo();
	}
}

void GuiApp::upButtonPressed() {

    if (thumbnailIdxOffset - 3 >= 0) {

        thumbnailIdxOffset -= 3;
        currentVideo = thumbnailIdxOffset;

        for (int i = thumbnailIdxOffset; i < (int)thumbnails.size(); i++)
			thumbnails[i]->enabled = false;
		
		playVideo();
	}
}

void GuiApp::addButtonPressed() {
	ofFileDialogResult result = ofSystemLoadDialog("Load file");
	if (result.bSuccess) {
		string path = result.getPath();
		if (ends_with(path, "mov") || ends_with(path, "mp4")) {
			Thumbnail *t = new Thumbnail();
			thumbnails.push_back(t);

			thumbnails[(int)thumbnails.size() - 1]->setup(path);

            thumbnails[(int)thumbnails.size() - 1]->set(thumbnailsOffset, 20 + 100 + (((int)thumbnails.size() - 1) % 3)
				* (thumbnails[0]->thumbnailSize + 10),
				thumbnails[0]->thumbnailSize, thumbnails[0]->thumbnailSize);
		}
    }
}

void GuiApp::goBackButtonPressed()
{
    if (!startScreenMode)
        startScreenMode = true;
}

void GuiApp::playButtonPressed() {
	mainPlayer.play();
	mainPlayer.setPaused(false);
}

void GuiApp::pauseButtonPressed() {
	mainPlayer.setPaused(true);
}

void GuiApp::stopButtonPressed() {
    mainPlayer.stop();
}

void GuiApp::startButtonPressed()
{

}

void GuiApp::manageButtonPressed()
{
    if (startScreenMode)
        startScreenMode = false;
}

void GuiApp::keyPressed(int key) {

	currentVideo++;

    currentVideo %= 3;
    currentVideo += thumbnailIdxOffset;

    if (currentVideo == dir.size()) {
        currentVideo = thumbnailIdxOffset;
    }

	playVideo();
}

void GuiApp::exit() {
    if (worker.isThreadRunning())
        worker.stopThread();

	upButton.removeListener(this, &GuiApp::upButtonPressed);
	downButton.removeListener(this, &GuiApp::downButtonPressed);
	playButton.removeListener(this, &GuiApp::playButtonPressed);
	pauseButton.removeListener(this, &GuiApp::pauseButtonPressed);
	stopButton.removeListener(this, &GuiApp::stopButtonPressed);
	addButton.removeListener(this, &GuiApp::addButtonPressed);
}
