/*
 * TestHelper.h
 *
 *  Created on: 2019年3月22日
 *      Author: blackguess
 */

#ifndef SRC_CONSENSUS_TESTHELPER_H_
#define SRC_CONSENSUS_TESTHELPER_H_
#include "POV.h"
//#include "KeyManager.h"
//#include "MessageManager.h"
class TestHelper {
public:
	TestHelper();
	virtual ~TestHelper();
	void testGenNDN();
	void testUpdateNDN();
	void testDeleteNDN();
	void testQueryNDN();
	void testRegistryNDN();
	void testgetUserNDN();
private:
	POV *pov;
};

#endif /* SRC_CONSENSUS_TESTHELPER_H_ */
