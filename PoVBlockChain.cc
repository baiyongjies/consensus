/*
 * PoVBlockChain.cc
 *
 *  Created on: 2018年4月11日
 *      Author: blackguess
 */

#include "PoVBlockChain.h"
#include <thread>
#include "utils.h"
//#include <experimental/optional>
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::finalize;

PoVBlockChain::PoVBlockChain() {
	// TODO Auto-generated constructor stub

}

PoVBlockChain::~PoVBlockChain() {
	// TODO Auto-generated destructor stub
}

void PoVBlockChain::setPubkey(std::string pubkey)
{
	this->pubkey=pubkey.substr(0,40);
}

bool PoVBlockChain::pushbackBlock(PoVBlock block)
{
	PoVHeader header=block.getPoVHeader();
	if(header.getHeight()!=getHeight()+1)
	{
		return false;
	}
	blockchain.push_back(block);
	return true;
}

PoVBlock PoVBlockChain::getBlock(uint32_t i)
{
	//return blockchain.at(i);
	height_mutex.lock();
	if(i==height)
	{
		height_mutex.unlock();
		return recent_block;
	}
	else
	{
		height_mutex.unlock();
		return getBlockFromDatabase(i);
	}
}

int PoVBlockChain::getHeight()
{
	/*
	uint32_t num=blockchain.size();
	if(num==0)
		return -1;
	return blockchain.at(num-1).getHeight();
	*/
	//return queryHeight();
	height_mutex.lock();
	int h=height;
	height_mutex.unlock();
	return h;
}

uint32_t PoVBlockChain::getAmout()
{
	//return blockchain.size();
	return queryHeight()+1;
}

bool PoVBlockChain::validateBlockChain()
{
	if(getHeight()>=0)
	{
		for(uint32_t i=0;i<blockchain.size()-1;i++)
		{
			if(getBlock(i).getHeight()!=getBlock(i+1).getHeight())
			{
				return false;
			}
		}
	}
	return true;
}

void PoVBlockChain::clear()
{
	blockchain.clear();
}

/*
void PoVBlockChain::setCollection(mongocxx::client &client,std::string pubkey)
{
	coll=client["blockchain"][pubkey];
}
*/



bool PoVBlockChain::pushbackBlockToDatabase(PoVBlock block)
{
	//std::cout<<"before get coll\n";
	//mongocxx::client c{mongocxx::uri{}};
	mongocxx::client client{mongocxx::uri{}};
	auto coll=client["blockchain"][pubkey];
	//std::cout<<"after get find\n";
	rapidjson::Document& doc=block.getBlock();
	uint32_t height=block.getHeight();
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	std::string json=buffer.GetString();
	bsoncxx::builder::stream::document temp_builder{};
	bsoncxx::document::value value=bsoncxx::from_json(json);
	temp_builder<<"height"<<static_cast<int>(height)
				<<"block"<<bsoncxx::builder::stream::open_document <<bsoncxx::builder::concatenate_doc{value.view()}<<bsoncxx::builder::stream::close_document;
	//std::cout<<"before find\n";
	mongocxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(
			bsoncxx::builder::stream::document{}<<"height"<<static_cast<int>(height)
			<<bsoncxx::builder::stream::finalize);
	//std::cout<<"after find\n";
	delete &doc;
	if(!result)
	{

		coll.insert_one(temp_builder.view());
		//bsoncxx::stdx::optional<mongocxx::result::insert_one> insert_result=coll.insert_one(temp_builder.view());
		//std::cout<<"after insert\n";
		this->recent_block=block;
		height_mutex.lock();
		this->height++;
		height_mutex.unlock();
		return true;
	}
	std::cout<<"this block has existed!\n";
	return false;

}


rapidjson::Document& PoVBlockChain::getBlockFromDatabaseByCondition(rapidjson::Value& cond)
{
	mongocxx::client c{mongocxx::uri{}};
	auto coll=c["blockchain"][pubkey];
	std::string condition_json=getDocumentString(cond);
	bsoncxx::document::value cond_value=bsoncxx::from_json(condition_json);
	//document condition{};
	//condition<<"data."+key<<value;
	mongocxx::cursor cursor = coll.find(cond_value.view());
	//int height=0;
	rapidjson::Document& data_array=*(new rapidjson::Document);
	data_array.SetArray();
	rapidjson::Document::AllocatorType& allocator=data_array.GetAllocator();
	for(auto doc : cursor) {
	  //std::cout << bsoncxx::to_json(doc) << "\n";
	  rapidjson::Document d;
	  std::string json_doc=bsoncxx::to_json(doc);
	  //std::cout<<"json:"<<json_doc<<"\n";
	  //std::cout<<"in getDataFromDatabase-document:1\n";
	  d.Parse(json_doc.c_str(),json_doc.size());
	  //std::cout<<"in getDataFromDatabase-document:2\n";
	  //print_document(d);
	  rapidjson::Value& json_value=*(new rapidjson::Value());
	  json_value.CopyFrom(d["block"],allocator);
	  data_array.PushBack(json_value,allocator);
	}
	return data_array;
}

PoVBlock PoVBlockChain::getBlockFromDatabase(uint32_t height)
{
	mongocxx::client c{mongocxx::uri{}};
	auto coll=c["blockchain"][pubkey];
	PoVBlock block;
	mongocxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(
			bsoncxx::builder::stream::document{}<<"height"<<static_cast<int>(height)
			<<bsoncxx::builder::stream::finalize);
	if(result)
	{
		bsoncxx::types::b_document document=(*result).view()["block"].get_document();
		bsoncxx::document::view view = document.view();
		std::string json=bsoncxx::to_json(view);
		rapidjson::Document doc;
		doc.Parse(json.c_str(),json.size());
		block.setBlock(doc);
		return block;
	}
	return block;
}
void PoVBlockChain::deleteBlockChainFromDatabase()
{
	mongocxx::client c{mongocxx::uri{}};
	auto coll=c["blockchain"][pubkey];
	coll.delete_many(bsoncxx::builder::stream::document{}<<bsoncxx::builder::stream::finalize);
	//if(result) {
	//  std::cout <<"删除区块数量:"<< result->deleted_count(bsoncxx::builder::stream::document{}<<bsoncxx::builder::stream::finalize) << "\n";
	//}
}
void PoVBlockChain::saveBlockChain()
{
	for(std::vector<PoVBlock>::const_iterator i=blockchain.begin();i<blockchain.end();i++)
	{
		bool result=pushbackBlockToDatabase(*i);
	}
}
void PoVBlockChain::loadBlockChain()
{
	mongocxx::client c{mongocxx::uri{}};
	auto coll=c["blockchain"][pubkey];
	blockchain.clear();
	int height=0;
	mongocxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(
			bsoncxx::builder::stream::document{}<<"height"<<height
			<<bsoncxx::builder::stream::finalize);
	while(result)
	{
		this->height++;
//		std::string json=bsoncxx::to_json((*result).view());
//		rapidjson::Document json_doc;
//		json_doc.Parse(json.c_str(),json.size());
//		rapidjson::Value& value=json_doc["block"];
//		PoVBlock block;
//		block.setBlock(value);
//		blockchain.push_back(block);
		height++;
		result = coll.find_one(
					bsoncxx::builder::stream::document{}<<"height"<<height
					<<bsoncxx::builder::stream::finalize);
	}
	result = coll.find_one(
				bsoncxx::builder::stream::document{}<<"height"<<height-1
				<<bsoncxx::builder::stream::finalize);
	if(result)
	{
		std::string json=bsoncxx::to_json((*result).view());
		rapidjson::Document json_doc;
		json_doc.Parse(json.c_str(),json.size());
		rapidjson::Value& value=json_doc["block"];
		recent_block.setBlock(value);
	}

}

int PoVBlockChain::queryHeight()
{
	mongocxx::client c{mongocxx::uri{}};
	auto coll=c["blockchain"][pubkey];
	mongocxx::cursor cursor = coll.find({});
	int height=-1;
	for(auto doc : cursor) {
	  //std::cout << bsoncxx::to_json(doc) << "\n";
	  rapidjson::Document d;
	  std::string json_doc=bsoncxx::to_json(doc);
	  d.Parse(json_doc.c_str(),json_doc.size());
	  if(d.IsObject()&&d.HasMember("height"))
	  {
		  int h=d["height"].GetInt();
		  if(h>height)
			  height=h;
	  }
	}
	return height;
}

rapidjson::Document& PoVBlockChain::queryLogByTime(long minT,long maxT)
{
	mongocxx::client c{mongocxx::uri{}};
	auto coll=c["blockchain"][pubkey];
	mongocxx::cursor cursor = coll.find({});
	int height=0;
	rapidjson::Document& log_array=*(new rapidjson::Document);
	log_array.SetArray();
	rapidjson::Document::AllocatorType& allocator=log_array.GetAllocator();
	for(auto doc : cursor) {
	  //std::cout << bsoncxx::to_json(doc) << "\n";
	  rapidjson::Document d;
	  std::string json_doc=bsoncxx::to_json(doc);
	  d.Parse(json_doc.c_str(),json_doc.size());
	  if(d.IsObject()&&d.HasMember("block"))
	  {
		  rapidjson::Value& txs=d["block"]["transactions"];
		  for(int i=0;i<txs.Size();i++)
		  {
			  //print_document(txs[i]);
			  if(txs[i].HasMember("metatype"))
			  {
				  if(!txs[i]["metatype"].IsString())
				  {
					  std::cout<<"metatype不是字符串\n";
					  //print_document(txs[i]["metatype"]);
				  }
				  std::string metatype=txs[i]["metatype"].GetString();
				  if(metatype=="Normal")
				  {
					  rapidjson::Value& content=txs[i]["content"];
					  if(content.HasMember("detail"))
					  {
						  if(content["detail"].IsObject()&& content["detail"].HasMember("Timestamp"))
						  {
							  //std::cout<<"遍历的日志：\n";
							  //print_document(content);
							  std::string timestamp=content["detail"]["Timestamp"].GetString();
							  int final=timestamp.find("\n");
							  timestamp=timestamp.substr(0,final);
							  long t=convertTime(timestamp);
							  if(t>=minT&&t<maxT)
							  {
								  rapidjson::Value& v=*(new rapidjson::Value);
								  v.CopyFrom(content,allocator);
								  log_array.PushBack(v,allocator);
							  }
							  else
							  {
								  //std::cout<<"minT="<<minT<<"\n";
								  //std::cout<<"maxT="<<maxT<<"\n";
								  //std::cout<<"t="<<t<<"\n";
								  std::cout<<"时间范围错误\n";
							  }
						  }//timestamp
						  else
						  {
							  std::cout<<"查询日志错误：没有timestamp\n";
						  }
					  }//detail
					  else
					  {
						  std::cout<<"查询日志错误：没有detail\n";
					  }
				  }//normal
				  else
				  {
					  std::cout<<"查询日志错误：没有normal\n";
				  }
			  }//metatype
			  else
			  {
				  std::cout<<"查询日志错误：没有metatype\n";
			  }
		  }//txs loop
	  }//block
	  else
	  {
		  std::cout<<"查询日志错误：没有block\n";
	  }
	}//blocks loop
	return log_array;
}

rapidjson::Document& PoVBlockChain::queryLogByErrorCode(std::string err_code)
{
	mongocxx::client c{mongocxx::uri{}};
	auto coll=c["blockchain"][pubkey];
	mongocxx::cursor cursor = coll.find({});
	int height=0;
	rapidjson::Document& log_array=*(new rapidjson::Document);
	log_array.SetArray();
	rapidjson::Document::AllocatorType& allocator=log_array.GetAllocator();
	for(auto doc : cursor) {
	  //std::cout << bsoncxx::to_json(doc) << "\n";
	  rapidjson::Document d;
	  std::string json_doc=bsoncxx::to_json(doc);
	  d.Parse(json_doc.c_str(),json_doc.size());
	  if(d.IsObject()&&d.HasMember("block"))
	  {
		  rapidjson::Value& txs=d["block"]["transactions"];
		  for(int i=0;i<txs.Size();i++)
		  {
			  print_document(txs[i]);
			  if(txs[i].HasMember("metatype"))
			  {
				  if(!txs[i]["metatype"].IsString())
				  {
					  std::cout<<"metatype不是字符串\n";
					  print_document(txs[i]["metatype"]);
				  }
				  std::string metatype=txs[i]["metatype"].GetString();
				  if(metatype=="Normal")
				  {
					  rapidjson::Value& content=txs[i]["content"];
					  if(content.HasMember("detail"))
					  {
						  if(content["detail"].IsObject()&& content["detail"].HasMember("errorCode"))
						  {
							  //std::cout<<"遍历的日志：\n";
							  //print_document(content);
							  std::string errorCode=content["detail"]["errorCode"].GetString();
							  if(errorCode==err_code)
							  {
								  rapidjson::Value& v=*(new rapidjson::Value);
								  v.CopyFrom(content,allocator);
								  log_array.PushBack(v,allocator);
							  }
							  else
							  {
								  std::cout<<"没有该时间短范围内的日志\n";
							  }
						  }//timestamp
						  else
						  {
							  std::cout<<"查询日志错误：没有errorCode\n";
						  }
					  }//detail
					  else
					  {
						  std::cout<<"查询日志错误：没有detail\n";
					  }
				  }//normal
				  else
				  {
					  std::cout<<"查询日志错误：没有normal\n";

				  }
			  }//metatype
			  else
			  {
				  std::cout<<"查询日志错误：没有metatype\n";
			  }
		  }//txs loop
	  }//block
	  else
	  {
		  std::cout<<"查询日志错误：没有block\n";
	  }
	}//blocks loop
	return log_array;
}

//mongocxx指定某个field的查找示例：
//https://stackoverflow.com/questions/39092512/how-to-retrieve-the-value-of-a-specific-field-using-mongo-cxx-driver?rq=1
bool PoVBlockChain::hasData(std::string key,std::string value,std::string type)
{
	std::string subname=pubkey+"-"+type;
	mongocxx::client client{mongocxx::uri{}};
	auto coll=client["blockchain"][subname];
	document condition{};
	condition<<"data."+key<<value;
//	condition<<"data"
//			<<open_document
//			<<key<<open_document<<"$eq"<<value<<close_document
//			<<close_document;
	int64_t result=coll.count(condition.view());
	std::cout<<"找到"<<result<<"个数据\n";
	if(result>0)
		return true;
	return false;
}
//mongocxx需要注意finalize的使用，参考：https://stackoverflow.com/questions/40615872/when-to-use-finalize-in-mongodb-cxx-r3-0-2-driver
//使用finalize后会返回bsoncxx::document::value的值，document中的值会转移到该变量中，因此可以在传递document值时应选用document.view()或者document<<finalize中的一种。
int PoVBlockChain::saveDataToDatabase(rapidjson::Document& data,std::string type,bool update,std::string key,std::string value)
{
	//std::cout<<"before get coll\n";
	//mongocxx::client c{mongocxx::uri{}};
	std::string subname=pubkey+"-"+type;
	mongocxx::client client{mongocxx::uri{}};
	auto coll=client["blockchain"][subname];


	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	data.Accept(writer);
	std::string json=buffer.GetString();
	bsoncxx::builder::stream::document temp_builder{};
	bsoncxx::document::value value_bson=bsoncxx::from_json(json);
	temp_builder<<"data"<<open_document <<bsoncxx::builder::concatenate_doc{value_bson.view()}<<close_document;

	//查找是否存在该数据
	bsoncxx::builder::stream::document condition;
	condition<<"data."+key<<value;
	std::string json_test=bsoncxx::to_json(condition.view());
	//std::cout<<"condition.view()"<<json_test<<"\n";
	mongocxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(bsoncxx::builder::stream::document{}<<"data."+key<<value
			<<bsoncxx::builder::stream::finalize);
	//int64_t allartnumber=coll.count(condition.view());
	//std::cout<<"count data:"<<allartnumber<<"\n";
	if(result)
	{
		//若已存在，则更新原有值
//		//std::string json_doc=bsoncxx::to_json(result);
//		std::cout<<"找到data\n";
//		  rapidjson::Document d;
//		  std::string json_doc=bsoncxx::to_json(result->view());
//		  d.Parse(json_doc.c_str(),json_doc.size());
//		  std::cout<<"in saveDataToDatabase:\n";
//		  print_document(d);
		if(update)
		{
			std::cout<<"更新用户数据\n";
			bsoncxx::stdx::optional<mongocxx::result::update> update_result=  coll.update_one(condition.view(),bsoncxx::builder::stream::document{}
			  	  	  <<"$set"
					  <<bsoncxx::builder::stream::open_document
					  <<"data"<<bsoncxx::builder::stream::open_document
					  <<bsoncxx::builder::concatenate_doc{value_bson.view()}
			  	  	  <<bsoncxx::builder::stream::close_document
					  <<bsoncxx::builder::stream::close_document<<bsoncxx::builder::stream::finalize);
			return 1;
		}
		else
		{
			return -1;
		}
//		  if(update_result)
//		  {
//			  std::cout<<"update success\n";
//		  }
//		  else
//		  {
//			  std::cout<<"update failed\n";
//		  }
	}
	else
	{
		//若未存在
		//std::cout<<"找不到data\n";
		std::cout<<"插入用户数据\n";
		bsoncxx::stdx::optional<mongocxx::result::insert_one> ret=coll.insert_one(temp_builder.view());
		if(ret)
		{
			//std::cout<<"insert数据成功：\n";
			//std::cout<<"insert_count()="<<ret->result().inserted_count() <<"\n";
			//std::cout<<"insert_type="<<std::string(ret->inserted_id().get_utf8().value)<<"\n";
			//std::string json_data=bsoncxx::to_json(temp_builder.view());
			//std::cout<<json_data<<"\n";
		}
		else
		{
			std::cout<<"insert数据失败\n";
		}
		return 0;
	}
	//std::cout<<"after get find\n";

	//temp_builder<<"height"<<static_cast<int>(height)
	//			<<"block"<<bsoncxx::builder::stream::open_document <<bsoncxx::builder::concatenate_doc{value.view()}<<bsoncxx::builder::stream::close_document;
	//std::cout<<"before find\n";
//	mongocxx::stdx::optional<bsoncxx::document::value> result = coll.find_one(
//			bsoncxx::builder::stream::document{}<<"height"<<static_cast<int>(height)
//			<<bsoncxx::builder::stream::finalize);
//	//std::cout<<"after find\n";
//	if(!result)
//	{
//		coll.insert_one(temp_builder.view());
//		//this->recent_block=block;
//		//this->height++;
//		return true;
//	}
//	std::cout<<"this block has existed!\n";
//	return false;
	return 0;
}

rapidjson::Document& PoVBlockChain::getAllDataFromDatabase(std::string type)
{
	std::string subname=pubkey+"-"+type;
	mongocxx::client client{mongocxx::uri{}};
	auto coll=client["blockchain"][subname];
	//auto builder=document{};
	document condition{};
	//condition<<"data."+key<<value;
	mongocxx::cursor cursor = coll.find(condition.view());
	//int height=0;
	rapidjson::Document& data_array=*(new rapidjson::Document);
	data_array.SetArray();
	rapidjson::Document::AllocatorType& allocator=data_array.GetAllocator();
	for(auto doc : cursor) {
	  //std::cout << bsoncxx::to_json(doc) << "\n";
	  rapidjson::Document d;
	  std::string json_doc=bsoncxx::to_json(doc);
	  //std::cout<<"json:"<<json_doc<<"\n";
	  //std::cout<<"in getDataFromDatabase-document:1\n";
	  d.Parse(json_doc.c_str(),json_doc.size());
	  //std::cout<<"in getDataFromDatabase-document:2\n";
	  //print_document(d);
	  rapidjson::Value& json_value=*(new rapidjson::Value());
	  json_value.CopyFrom(d["data"],allocator);
	  data_array.PushBack(json_value,allocator);
	}
	return data_array;
}

rapidjson::Document& PoVBlockChain::getDataFromDatabase(std::string key,std::string value,std::string type,bool isMultiple)
{
	std::string subname=pubkey+"-"+type;
	mongocxx::client client{mongocxx::uri{}};
	auto coll=client["blockchain"][subname];
	//auto builder=document{};
	document condition;
	condition<<"data."+key<<value;

	mongocxx::cursor cursor = coll.find(condition.view());
	//int height=0;
	rapidjson::Document& data_array=*(new rapidjson::Document);

	if(isMultiple)
	{
		data_array.SetArray();
	}
	else
	{
		data_array.SetObject();
	}
	rapidjson::Document::AllocatorType& allocator=data_array.GetAllocator();
	for(auto doc : cursor) {
	  //std::cout << bsoncxx::to_json(doc) << "\n";
	  rapidjson::Document d;
	  std::string json_doc=bsoncxx::to_json(doc);
	  //std::cout<<"json:"<<json_doc<<"\n";
	  //std::cout<<"in getDataFromDatabase-document:1\n";
	  d.Parse(json_doc.c_str(),json_doc.size());
	  //std::cout<<"in getDataFromDatabase-document:2\n";
	  //print_document(d);
	  rapidjson::Value& json_value=*(new rapidjson::Value);
	  json_value.CopyFrom(d["data"],allocator);
	  if(isMultiple)
	  {
		  data_array.PushBack(json_value,allocator);
	  }
	  else
	  {
		  data_array.CopyFrom(json_value,allocator);
		  return data_array;
	  }
	}
	return data_array;
}

int PoVBlockChain::deleteData(std::string key,std::string value,std::string type,bool isAll)
{
	std::string subname=pubkey+"-"+type;
	mongocxx::client client{mongocxx::uri{}};
	auto coll=client["blockchain"][subname];
	document condition;
	condition<<"data."+key<<value;
	int ret=0;
	if(isAll)
	{
		bsoncxx::stdx::optional<mongocxx::result::delete_result> result =
				coll.delete_many(condition.view());

		if(result) {
		  //std::cout << result->deleted_count() << "\n";
		  ret=result->deleted_count();
		}
	}
	else
	{
		auto result=coll.delete_one(condition.view());
		if(result)
		{
			ret=1;
		}
	}
	return ret;

}
void PoVBlockChain::deleteAllDataFromDatabase(std::string type)
{
	std::string subname=pubkey+"-"+type;
	mongocxx::client client{mongocxx::uri{}};
	auto coll=client["blockchain"][subname];
	coll.delete_many(bsoncxx::builder::stream::document{}<<bsoncxx::builder::stream::finalize);
}
//template<typename T> rapidjson::Document& PoVBlockChain::getDataFromDatabase(std::string key,T value, std::string type,bool isMultiple)
//{
//	std::string subname=pubkey+"-"+type;
//	mongocxx::client client{mongocxx::uri{}};
//	auto coll=client["blockchain"][subname];
//	mongocxx::cursor cursor = coll.find({});
//	int height=0;
//	rapidjson::Document& data_array=*(new rapidjson::Document);
//	if(isMultiple)
//	{
//		data_array.SetArray();
//	}
//	else
//	{
//		data_array.SetObject();
//	}
//	rapidjson::Document::AllocatorType& allocator=data_array.GetAllocator();
//	for(auto doc : cursor) {
//	  //std::cout << bsoncxx::to_json(doc) << "\n";
//	  rapidjson::Document d;
//	  std::string json_doc=bsoncxx::to_json(doc);
//	  d.Parse(json_doc.c_str(),json_doc.size());
//	  if(!d.IsObject())
//	  {
//		  continue;
//	  }
//	  if(!d.HasMember(key.c_str()))
//	  {
//		  continue;
//	  }
//	  T v=d[key.c_str()].Get<T>();
//	  if(v!=type)
//	  {
//		  continue;
//	  }
//	  if(isMultiple)
//	  {
//		  data_array.PushBack(d,allocator);
//	  }
//	  else
//	  {
//		  data_array.CopyFrom(d,allocator);
//	  }
//	}
//	return data_array;
//}

/*
rapidjson::Document& PoVBlockChain::queryBlock(int height)
{

}
*/

