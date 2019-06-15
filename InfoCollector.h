/*
 * InfoCollector.h
 *
 *  Created on: 2019年3月6日
 *      Author: blackguess
 */

#ifndef SRC_CONSENSUS_INFOCOLLECTOR_H_
#define SRC_CONSENSUS_INFOCOLLECTOR_H_
#include <iostream>
#include<boost/array.hpp>
#include<boost/system/system_error.hpp>
#include <boost/asio.hpp>
#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "KeyManager.h"
#include "utils.h"
#include <map>
using namespace std;
using namespace boost::asio;
struct VoteData{
	string pubkey;
	int height;
	int txs_num;
	int agreement;
	string tx_type;
};
class InfoCollector {
public:
	InfoCollector();
	virtual ~InfoCollector();
	void sendMessage(string msg);
	void sendVoteMessage();
	void sendNodeMessage();
	void setNodeMessage(string pubkey,string IP,string name,bool is_butler_candidate,bool is_butler,bool is_commissioner);
	void setServerAddress(string IP,int port);
	void collectVoteData(string pubkey,int height,int agreement,int txs_num,string tx_type);
private:
	string ServerIP;
	int ServerPort;
	string pubkey;
	string IP;
	string name;
	bool is_butler_candidate;
	bool is_butler;
	bool is_commissioner;
	map<string,VoteData> voteData;
};

#endif /* SRC_CONSENSUS_INFOCOLLECTOR_H_ */
