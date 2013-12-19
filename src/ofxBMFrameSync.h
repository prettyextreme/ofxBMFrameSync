#pragma once


#include "DeckLinkAPI.h"
#include "ofMain.h"

class ofxBMFrameSync
{
public:
    ofxBMFrameSync();
    ~ofxBMFrameSync();
    bool setup();
    
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

};
