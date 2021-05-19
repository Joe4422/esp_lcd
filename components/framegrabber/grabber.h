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

bool FrameGrabber_NextWidget();

bool FrameGrabber_LastWidget();

bool FrameGrabber_WidgetAction();

#endif /* FRAMEGRABBER_GRABBER_H_ */
