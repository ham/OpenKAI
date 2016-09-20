#ifndef OPENKAI_SRC_AUTOPILOT_VISUALFOLLOW_H_
#define OPENKAI_SRC_AUTOPILOT_VISUALFOLLOW_H_

#include "../Base/common.h"
#include "ActionBase.h"
#include "../UI/UI.h"

#define MODE_ASSIST 0
#define MODE_RECTDRAW 1

namespace kai
{

class VisualFollow: public ActionBase
{
public:
	VisualFollow();
	~VisualFollow();

	bool init(Config* pConfig, AUTOPILOT_CONTROL* pAC);
	void update(void);
	bool draw(Frame* pFrame, iVec4* pTextPos);

	void followROI(void);

private:
	//Control
	CONTROL_CHANNEL m_roll;
	CONTROL_CHANNEL m_pitch;
	CONTROL_CHANNEL m_yaw;
	CONTROL_CHANNEL m_alt;

	//Detectors
	_Cascade* m_pFD;
	_ROITracker* m_pROITracker;
	_Flow* m_pFlow;

	//UI
	UI* m_pUI;

	iVec4 m_ROI;
	bool m_bSelect;
	int m_minROI;
	int m_ROImode;
	int m_ROIsize;
	int m_ROIsizeFrom;
	int m_ROIsizeTo;

	//UI
	int m_btnSize;
	int m_btnROIClear;
	int m_btnROIBig;
	int m_btnROISmall;
	int m_btnMode;

	Rect2d getROI(iVec4 mouseROI);

};

}

#endif
