#include "../../Controller/APcopter/APcopter_DNNavoid.h"

namespace kai
{

APcopter_DNNavoid::APcopter_DNNavoid()
{
	m_pAP = NULL;
	m_pIN = NULL;
	m_nVision = 0;
	m_action = DA_UNKNOWN;
}

APcopter_DNNavoid::~APcopter_DNNavoid()
{
}

bool APcopter_DNNavoid::init(void* pKiss)
{
	IF_F(!this->ActionBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;
	pK->m_pInst = this;

	return true;
}

bool APcopter_DNNavoid::link(void)
{
	IF_F(!this->ActionBase::link());
	Kiss* pK = (Kiss*) m_pKiss;
	string iName;

	iName = "";
	F_INFO(pK->v("APcopter_base", &iName));
	m_pAP = (APcopter_base*) (pK->parent()->getChildInstByName(&iName));

	iName = "";
	F_INFO(pK->v("_ImageNet", &iName));
	m_pIN = (_ImageNet*) (pK->root()->getChildInstByName(&iName));
	if (!m_pIN)
	{
		LOG_E(iName << " not found");
		return false;
	}

	while(!m_pIN->bReady())
	{
		sleep(1);
	}

	Kiss** pItr = pK->getChildItr();
	OBJECT tO;
	tO.init();
	tO.m_fBBox.z = 1.0;
	tO.m_fBBox.w = 1.0;

	m_nVision = 0;
	while (pItr[m_nVision])
	{
		Kiss* pKT = pItr[m_nVision];
		IF_F(m_nVision >= DNNAVOID_N_VISION);

		DNN_AVOID_VISION* pV = &m_pVision[m_nVision];
		m_nVision++;
		pV->init();

		F_ERROR_F(pKT->v("orientation", (int* )&pV->m_orientation));
		F_INFO(pKT->v("rMin", &pV->m_rMin));
		F_INFO(pKT->v("rMax", &pV->m_rMax));
		F_INFO(pKT->v("l", &tO.m_fBBox.x));
		F_INFO(pKT->v("t", &tO.m_fBBox.y));
		F_INFO(pKT->v("r", &tO.m_fBBox.z));
		F_INFO(pKT->v("b", &tO.m_fBBox.w));

		double deg;
		if(pKT->v("angleDeg", &deg))
		{
			pV->m_angleTan = tan(deg*DEG_RAD);
		}

		pV->m_pObj = m_pIN->add(&tO);
		NULL_F(pV->m_pObj);

		string pClass[DETECTOR_N_CLASS];
		Kiss** pItrAct = pKT->getChildItr();
		pV->m_nAction = 0;
		int i=0;
		while (pItrAct[i])
		{
			if(pV->m_nAction >= DNNAVOID_N_ACTION)
			{
				LOG_E("Exceeded the max number of action:" << DNNAVOID_N_ACTION);
				return false;
			}

			Kiss* pKA = pItrAct[i++];
			int nClass = pKA->array("class", pClass, DETECTOR_N_CLASS);
			IF_CONT(nClass <= 0);

			DNN_AVOID_ACTION* pA = &pV->m_pAction[pV->m_nAction++];
			pA->init();

			for(int j=0; j<nClass; j++)
			{
				pA->addClass(m_pIN->getClassIdx(pClass[j]));
			}

			if(pA->m_mClass == 0)
			{
				LOG_E("nClass = 0");
				return false;
			}

			string strAction = "";
			if(!pKA->v("action", &strAction))
			{
				LOG_E("Action not defined");
				return false;
			}

			pA->m_action = str2actionType(strAction);
			if(pA->m_action == DA_UNKNOWN)
			{
				LOG_E("Unknown action: " << strAction);
				return false;
			}

		}
	}

	return true;
}

DNN_AVOID_ACTION_TYPE APcopter_DNNavoid::str2actionType(string& strAction)
{
	if (strAction == "safe")
		return DA_SAFE;
	else if (strAction == "warn")
		return DA_WARN;
	else if (strAction == "forbid")
		return DA_FORBID;

	return DA_UNKNOWN;
}

string APcopter_DNNavoid::actionType2str(DNN_AVOID_ACTION_TYPE aType)
{
	static const string pType[] =
	{ "unknown", "safe", "warn", "forbid" };
	return pType[aType];
}

void APcopter_DNNavoid::update(void)
{
	this->ActionBase::update();

	NULL_(m_pIN);
	NULL_(m_pAP);
	NULL_(m_pAP->m_pMavlink);
	_Mavlink* pMavlink = m_pAP->m_pMavlink;
	double alt = (double)pMavlink->m_msg.global_position_int.relative_alt;

	int i,j;
	for (i = 0; i < m_nVision; i++)
	{
		DNN_AVOID_VISION* pV = &m_pVision[i];

		for (j = 0; j < pV->m_nAction; j++)
		{
			DNN_AVOID_ACTION* pA = &pV->m_pAction[j];
			IF_CONT(!(pA->m_mClass & pV->m_pObj->m_mClass));

			m_action = pA->m_action;

			if(m_action <= DA_SAFE)
			{
				LOG_I("SAFE:" << m_pIN->getClassName(pV->m_pObj->m_iClass));
				break;
			}

			if(m_action <= DA_WARN)
			{
				LOG_I("WARNING:" << m_pIN->getClassName(pV->m_pObj->m_iClass));
				break;
			}

			LOG_I("FORBID:" << m_pIN->getClassName(pV->m_pObj->m_iClass));

			pMavlink->distanceSensor(0, //type
					pV->m_orientation,	//orientation
					pV->m_rMax,
					pV->m_rMin,
					constrain(alt*pV->m_angleTan, pV->m_rMin, pV->m_rMax));

			break;
		}

		if(m_action <= DA_WARN)
		{
			pMavlink->distanceSensor(0, //type
					pV->m_orientation,	//orientation
					pV->m_rMax,
					pV->m_rMin,
					pV->m_rMax);
		}
	}

}

bool APcopter_DNNavoid::draw(void)
{
	IF_F(!this->ActionBase::draw());
	Window* pWin = (Window*) this->m_pWindow;
	Mat* pMat = pWin->getFrame()->getCMat();

	return true;
}

}
