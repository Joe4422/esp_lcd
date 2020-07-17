#ifndef FRAMEGRABBER_GRABBER_H_
#define FRAMEGRABBER_GRABBER_H_

/****************************************************************
 * Includes
 ****************************************************************/
#include <stdbool.h>

/****************************************************************
 * Function declarations
 ****************************************************************/
bool FrameGrabber_Init();

bool FrameGrabber_Run();

bool FrameGrabber_AddPage(char * page);

bool FrameGrabber_NextPage();

bool FrameGrabber_LastPage();

bool FrameGrabber_PageAction();

#endif /* FRAMEGRABBER_GRABBER_H_ */
