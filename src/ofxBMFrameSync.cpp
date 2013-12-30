#include "ofxBMFrameSync.h"


ofxBMFrameSync::ofxBMFrameSync(){
    
    isSlave = false;

    setup();
    
    IDeckLinkDisplayModeIterator*		pDLDisplayModeIterator;
	IDeckLinkDisplayMode*				pDLDisplayMode = NULL;
	
	// Get first available video mode for Output
	if (pDLOutput->GetDisplayModeIterator(&pDLDisplayModeIterator) == S_OK)
	{
        while(true){
            if (pDLDisplayModeIterator->Next(&pDLDisplayMode) != S_OK)
            {
                //NSRunAlertPanel(@"DeckLink error.", @"Cannot find video mode.", @"OK", nil, nil);
                pDLDisplayModeIterator->Release();
                break;
            }
            
            uiFrameWidth = pDLDisplayMode->GetWidth();
            uiFrameHeight = pDLDisplayMode->GetHeight();
            pDLDisplayMode->GetFrameRate(&frameDuration, &frameTimescale);
            
            uiFPS = ((frameTimescale + (frameDuration-1))  /  frameDuration);
            
            printf("%i x %i @ %i\n",uiFrameWidth,uiFrameHeight,uiFPS);
            
            if(uiFrameWidth == 1280 && uiFrameHeight == 720 && uiFPS == 60)
                break;
        }
		pDLDisplayModeIterator->Release();
	}
	
	uiFrameWidth = pDLDisplayMode->GetWidth();
	uiFrameHeight = pDLDisplayMode->GetHeight();
	pDLDisplayMode->GetFrameRate(&frameDuration, &frameTimescale);
	
	uiFPS = ((frameTimescale + (frameDuration-1))  /  frameDuration);
	
	if (pDLOutput->EnableVideoOutput(pDLDisplayMode->GetDisplayMode(), bmdVideoOutputFlagDefault) != S_OK)
		return false;
    
	// Flip frame vertical, because OpenGL rendering starts from left bottom corner
	if (pDLOutput->CreateVideoFrame(uiFrameWidth, uiFrameHeight, uiFrameWidth*4, bmdFormat8BitBGRA, bmdFrameFlagDefault, &pDLVideoFrame) != S_OK)
		return false;
	
	uiTotalFrames = 0;
    
    udp.Create();
    
    if(isSlave)
        udp.Bind(12345);
    else
        udp.Connect("192.168.0.19", 12345);
    
    exitThreadFlag = false;
    startThread();
    
}

ofxBMFrameSync::~ofxBMFrameSync(){

    if (pDLOutput == NULL)
		return false;
	
    exitThreadFlag = true;
    waitForThread();
    
	pDLOutput->StopScheduledPlayback(0, NULL, 0);
	pDLOutput->DisableVideoOutput();
	
	if (pDLVideoFrame != NULL)
	{
		pDLVideoFrame->Release();
		pDLVideoFrame = NULL;
	}
}


bool ofxBMFrameSync::setup(){
    
    bool bSuccess = FALSE;
	IDeckLinkIterator* pDLIterator = NULL;
    
	pDLIterator = CreateDeckLinkIteratorInstance();
	if (pDLIterator == NULL)
	{
		//NSRunAlertPanel(@"This application requires the DeckLink drivers installed.", @"Please install the Blackmagic DeckLink drivers to use the features of this application.", @"OK", nil, nil);
		goto error;
	}
    
	if (pDLIterator->Next(&pDL) != S_OK)
	{
		//NSRunAlertPanel(@"This application requires a DeckLink device.", @"You will not be able to use the features of this application until a DeckLink PCI card is installed.", @"OK", nil, nil);
		goto error;
	}
	
	if (pDL->QueryInterface(IID_IDeckLinkOutput, (void**)&pDLOutput) != S_OK)
		goto error;
	/*
	pRenderDelegate = new RenderDelegate(this);
	if (pRenderDelegate == NULL)
		goto error;
	
	if (pDLOutput->SetScheduledFrameCompletionCallback(pRenderDelegate) != S_OK)
		goto error;
	*/
	bSuccess = TRUE;
    
error:
	
	if (!bSuccess)
	{
		if (pDLOutput != NULL)
		{
			pDLOutput->Release();
			pDLOutput = NULL;
		}
		if (pDL != NULL)
		{
			pDL->Release();
			pDL = NULL;
		}
	}
	
	if (pDLIterator != NULL)
	{
		pDLIterator->Release();
		pDLIterator = NULL;
	}

}

float ofxBMFrameSync::getFramePosition(){
    
    BMDTimeValue hardwaretime, timeinframe, ticksperframe;
    pDLOutput->GetHardwareReferenceClock(frameTimescale,&hardwaretime,&timeinframe,&ticksperframe);
    
    return ((float)timeinframe)/(float)ticksperframe;
}

void ofxBMFrameSync::renderFrame(unsigned char* bytes, int length){
    
    void*	pFrame;
    
    pDLVideoFrame->GetBytes((void**)&pFrame);
    
    memcpy(pFrame, bytes, fmin(length, uiFrameWidth*uiFrameHeight*4));
    
    HRESULT hr = pDLOutput->DisplayVideoFrameSync(pDLVideoFrame);
    if (hr)
        uiTotalFrames++;
    
}

void ofxBMFrameSync::threadedFunction(){
 
    lock();
    CGLPixelFormatAttribute attribs[] =
	{
		kCGLPFAColorSize, (CGLPixelFormatAttribute)32,
		kCGLPFAAlphaSize, (CGLPixelFormatAttribute)8,
		kCGLPFADepthSize, (CGLPixelFormatAttribute)16,
		kCGLPFAAccelerated,
		(CGLPixelFormatAttribute)0
	};
	CGLPixelFormatObj pixelFormatObj;
	GLint numPixelFormats;
	CGLChoosePixelFormat (attribs, &pixelFormatObj, &numPixelFormats);
	CGLCreateContext (pixelFormatObj, 0, &contextObj);
	CGLDestroyPixelFormat (pixelFormatObj);
    
    
    
    float positionAtLastPush = 2.0f;
    
    while(true){
        if(exitThreadFlag)
            break;
        
        if(false){
            old_contextObj = CGLGetCurrentContext();
            CGLSetCurrentContext (contextObj);
            
            if(fbo.getWidth() != ofGetWidth() || fbo.getHeight()!=ofGetHeight()){
                fbo.allocate(ofGetWidth(),ofGetHeight(),GL_RGBA,8);
                pix.allocate(ofGetWidth(),ofGetHeight(),ofGetImageTypeFromGLType(ofGetGLFormatFromInternal(fbo.getTextureReference().texData.glTypeInternal)));
            }
        }
        
            
            
        static int renderFr = 0;
        
        //msatimer.start();
        
        //for(int fr = 0; fr < 10 ; fr++){
        
        int whichQuarter = renderFr%4;
        
        
        msa::Timer delayTimer;
        delayTimer.start();
        
        unlock();
        
        float startingFramePos = getFramePosition();
        
        static unsigned long long usStart = ofGetSystemTimeMicros();
        unsigned long long lastStart = usStart;
        usStart = ofGetSystemTimeMicros();

        if(!isSlave){
            udp.Send(ofToString(whichQuarter).c_str(), 2);
        }
        
        while(getFramePosition() >= 0.5f){
            //printf("b %f\n",fsOut.getFramePosition());
            //sleep(2);
        }
        while(getFramePosition() < 0.5f){
            //printf("a %f\n",fsOut.getFramePosition());
            //sleep(2);
        }
        
        if(isSlave){
            char buf[1000];
            while(udp.Receive(buf, 1000)){
                whichQuarter = buf[0]-'0';
                printf("whichQuarter = %i\n",whichQuarter);
            }
        }
        
        unsigned long long usEnd = ofGetSystemTimeMicros();
        
        lock();
        
        delayTimer.stop();
        
        //float fps = 1.0f/msatimer.getSecondsSinceLastCall();
        int diff = usStart - lastStart;
        float fps = 1000000.0f / diff ;
        
        if(fps < 58)
            printf("Bad fps: %f\n",fps);
        
        
        if(false){
            fbo.begin();
            
            ofSetupScreen();
            
            ofBackground(0,0,0);
            ofDrawBitmapString(ofToString(fps,2), 20,20);
            ofDrawBitmapString(ofToString(delayTimer.getSeconds()*1000.0f,2), 20,40);
            ofDrawBitmapString(ofToString(startingFramePos,2), 20,60);
            ofDrawBitmapString(ofToString((float)(usEnd-usStart)/1000.0f,2), 20,80);
            ofEllipse(300.0f+200.0f*sin(0.02f*renderFr), 300.0f+200.0f*cos(0.02f*renderFr), 100, 100);
    //        ofPushMatrix();
    //        ofTranslate(ofGetWidth()/2, ofGetHeight()/2);
    //        ofRotate(renderFr*3, 1, 1, 0.5);
    //        ofSetColor(255, 0, 0,100);
    //        ofBoxPrimitive(200,200,200).draw();
    //        ofPopMatrix();
            fbo.end();
            fbo.draw(0,0);
            
            //if(delayTimer.getSeconds() > 0.009)
            //    printf("delay %f\n",delayTimer.getSeconds());
            
            
            if(ofGetFrameNum()<5)
                sleep(1);
            
            
            fbo.readToPixels(pix);
        }
        
        
        if(false){
            
            int totalpix = ofGetWidth()*ofGetHeight();
            unsigned char c;
            unsigned char* buf = pix.getPixels();
            
            for(int i=0;i<totalpix;i++){
                c = buf[i*4];
                buf[i*4] = buf[i*4 + 2];
                buf[i*4+2] = c;
            }
            
            renderFrame(buf, totalpix*4);
        } else{
            
            if(ofGetFrameNum()<5)
                continue;
            int totalpix = 1280*720;
            int totalbytes = 1280*720*4;
            static unsigned char* buf = (unsigned char*) malloc(1280*720*4);
            memset(buf,0,totalbytes);
            
            int startPos = (whichQuarter)*1280*720;
            for(int i=0;i<(totalpix/4);i++){
                buf[startPos + i*4+0] = 255;
                buf[startPos + i*4+1] = 0;
                buf[startPos + i*4+2] = 0;
                buf[startPos + i*4+3] = 255;
            }
            renderFrame(buf, totalbytes);
            
        }
        
        
        
        
        positionAtLastPush = getFramePosition();
        
        renderFr++;
        
    
        
        
        
        
        
        CGLSetCurrentContext(old_contextObj);
        
        sleep(1);
    }
    
    unlock();

}