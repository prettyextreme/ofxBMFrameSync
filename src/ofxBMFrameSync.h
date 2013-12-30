#pragma once


#include "DeckLinkAPI.h"
#include "ofMain.h"
#include "OpenGL/OpenGL.h"

#include "MSATimer.h"

#include "ofxNetwork.h"

class ofxBMFrameSync : ofThread
{
public:
    ofxBMFrameSync();
    ~ofxBMFrameSync();
    bool setup();
    
    void threadedFunction();
    bool exitThreadFlag;
    
    void externLock(){lock();}
    void externUnlock(){unlock();}
    
    
    float getFramePosition();
    void renderFrame(unsigned char* bytes, int length);

    
private:
    
	GLenum				glStatus;
	GLuint				idFrameBuf, idColorBuf, idDepthBuf;
    
	// DeckLink
	uint32_t					uiFrameWidth;
	uint32_t					uiFrameHeight;
	
	IDeckLink*					pDL;
public:
	IDeckLinkOutput*			pDLOutput;
private:
	IDeckLinkMutableVideoFrame*	pDLVideoFrame;
	
	BMDTimeValue				frameDuration;
	BMDTimeScale				frameTimescale;
	uint32_t					uiFPS;
	uint32_t					uiTotalFrames;
    
	void ResetFrame();

protected:
    
private:
    
	CGLContextObj		contextObj;
	CGLContextObj		old_contextObj;
    
    ofFbo fbo;
    ofPixels pix;
    
    msa::Timer msatimer;
    
    ofxUDPManager udp;
    
    bool isSlave;

};
