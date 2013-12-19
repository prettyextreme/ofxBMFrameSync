#include "ofxBMFrameSync.h"


ofxBMFrameSync::ofxBMFrameSync(){
    
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
    
}

ofxBMFrameSync::~ofxBMFrameSync(){

    if (pDLOutput == NULL)
		return false;
	
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
        
    if (pDLOutput->DisplayVideoFrameSync(pDLVideoFrame))
        uiTotalFrames++;
    
}