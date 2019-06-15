/*
 * LRU.h
 *
 *  Created on: 2018年11月23日
 *      Author: blackguess
 */

#ifndef SRC_CONSENSUS_LRU_H_
#define SRC_CONSENSUS_LRU_H_
#include <iostream>
#include <map>
#include <set>
using namespace std;
/*
struct cache{
	string pubkey;
	int index;
};
struct comp{
	bool operator()(const cache &c1,const cache &c2)
	{
		if(c1.pubkey<c2.pubkey)
			return true;
		else if(c1.pubkey>c2.pubkey)
			return false;
		else
		{
			if(c1.index<c2.index)
				return true;
			else
				return false;
		}
	}
};
*/
template<typename T,typename comp> class LRU
{
public:
	LRU(){

	}
	LRU(int cap){
		capacity=cap;
		origin=new node;
		origin->next=origin;
		origin->prev=origin;
	};
	void init(int cap)
	{
		capacity=cap;
		origin=new node;
		origin->next=origin;
		origin->prev=origin;
	}
	//添加元素，将该元素置于链表末尾，如果在插入前已经存在该元素，则返回false，否则返回true
	bool append(T &t){
		auto it=s.find(t);
		if(it!=s.end()){
			node* n=it->second;
			//从链表中取出n
			n->prev->next=n->next;
			n->next->prev=n->prev;
			//把n插入到链表开头
			origin->next->prev=n;
			n->next=origin->next;
			n->prev=origin;
			origin->next=n;
			return false;
		}
		else
		{
			if(nums<capacity)
			{
				node* n=new node;
				n->msg=t;
				n->next=origin->next;
				origin->next->prev=n;
				n->prev=origin;
				origin->next=n;
				s[t]=n;
				nums++;
			}
			else
			{
				node* n=origin->prev;
				//将最后一个节点取出
				n->prev->next=origin;
				n->next->prev=n->prev;
				//将取出的节点插入到链表头
				origin->next->prev=n;
				n->next=origin->next;
				origin->next=n;
				n->prev=origin;
				auto it=s.find(n->msg);
				s.erase(it);
				n->msg=t;
				s[t]=n;
			}
			return true;
		}
	}
	T pop()
	{
		if(nums>0)
		{
			node* n=origin->prev;
			T t=n->msg;
			auto it=s.find(n->msg);
			s.erase(it);
			n->prev->next=n->next;
			n->next->prev=n->prev;
			delete n;
			nums--;
			return t;
		}
		T t;
		return t;
	}
	void pop(T &t)
	{
		if(nums>0)
		{
			auto it=s.find(t);
			if(it!=s.end())
			{
				node *n=it->second;
				n->prev->next=n->next;
				n->next->prev=n->prev;
				s.erase(it);
				nums--;
			}
		}

	}
	T getElement(int i)
	{
		i=i%nums;
		node* n=origin->prev;
		for(int k=0;k<i;k++)
		{
			n=n->prev;
		}
		return n->msg;
	}
	int size()
	{
		return nums;
	}
	bool find(T &t)
	{
		auto it=s.find(t);
		if(it!=s.end())
		{
			return true;
		}
		else
			return false;
	}
	int getCapacity()
	{
		return capacity;
	}
	bool is_full()
	{
		if(capacity<=nums)
			return true;
		else
			return false;
	}
	int getNums()
	{
		return nums;
	}
private:
	struct node{
		node *prev;
		node *next;
		T msg;
	};
	int capacity=0;
	int nums=0;
	node *origin=NULL;
	map<T,node*,comp> s;
};





#endif /* SRC_CONSENSUS_LRU_H_ */
