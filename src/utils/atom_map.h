#ifndef TEMPLATE_ATOMIC_MAP_H
#define TEMPLATE_ATOMIC_MAP_H

#include <unordered_map>
#include <string>
#include <memory>
#include <exception>
#include "logger.h"

namespace utils
{
	enum actType
	{
		ADD = 0,
		MOD = 1,
		DEL = 2,
		REV = 3,
		MAX,
	};

	template<class KEY, class VALUE, class Hash = std::hash<KEY>, class KeyEqual = std::equal_to<KEY>>
	class AtomMap
	{
	public:
		typedef std::shared_ptr<VALUE> VALUE_PTR;

		struct Record{
			actType type_;
			VALUE_PTR ptr_;

			Record(actType type = MAX) :type_(type){}
			Record(const VALUE_PTR& ptr, actType type = MAX) :ptr_(ptr), type_(type){}
		};

		typedef std::unordered_map<KEY, Record, Hash, KeyEqual> Map;

	protected:
		Map  buff_;
		Map  stor_;
		Map* data_;

	public:
		AtomMap(){
			data_ = &stor_; //avoid manual memory management
		}

		AtomMap(Map* data){
			if (data){
				data_ = data;
			}
			else{
				data_ = &stor_; //avoid manual memory management
			}
		}

		AtomMap(const AtomMap& other){
			Copy(other);
		}

		AtomMap& operator=(const AtomMap& other){
			buff_.clear();
			stor_.clear();
			data_ = nullptr;

			Copy(other);

			return *this;
		}

	private:
		void Copy(const AtomMap& other){
			for (auto kvAct : other.buff_){
				buff_[kvAct.first] = Record(std::make_shared<VALUE>(*(kvAct.second.ptr_)), kvAct.second.type_);
			}

			for (auto kvData : *(other.data_)){
				stor_[kvData.first] = Record(std::make_shared<VALUE>(*(kvData.second.ptr_)), kvData.second.type_);
			}

			data_ = &stor_;
		}

		void SetValue(const KEY& key, const VALUE_PTR& ptr){
			buff_[key] = Record(ptr, MOD);
		}

		bool GetValue(const KEY& key, VALUE_PTR& ptr){
			auto itAct = buff_.find(key);
			if (itAct != buff_.end()){
				if (itAct->second.type_ == DEL){
					return false;
				}

				ptr = itAct->second.ptr_;
				return true;
			}
			
			auto itData = data_->find(key);
			if (itData != data_->end()){
				if (itData->second.type_ == DEL){
					return false;
				}

				//can't be assigned directly, because itData->second.ptr_ is smart pointer
				auto pv = std::make_shared<VALUE>(*(itData->second.ptr_));
				if (!pv){
					return false;
				}

				buff_[key] = Record(pv, MOD);
				ptr = pv;
				return true;
			}
			
			if (!GetFromDB(key, ptr)){
				return false;
			}

			buff_[key] = Record(ptr, ADD);
			return true;
		}

	public:
		const Map& GetData(){
			return *data_;
		}

		Map& GetChangeBuf(){
			return buff_;
		}

		bool Set(const KEY& key, const VALUE_PTR& ptr){
			bool ret = true;

			try{
				SetValue(key, ptr);
			}
			catch(std::exception& e){ 
				LOG_ERROR("Catched an set exception, detail: %s", e.what());
				ret = false;
			}

			return ret;
		}

		bool Get(const KEY& key, VALUE_PTR& ptr){
			bool ret = true;

			try{
				ret = GetValue(key, ptr);
			}
			catch(std::exception& e){ 
				LOG_ERROR("Catched an get exception, detail: %s", e.what());
				ret = false;
			}
			return ret;
		}

		bool Del(const KEY& key)
		{
			bool ret = true;

			try{
				buff_[key] = Record(DEL);
			}
			catch(std::exception& e){ 
				LOG_ERROR("Catched an delete exception, detail: %s", e.what());
				ret = false;
			}

			return ret;
		}

	private:
		bool CopyCommit(){
			Map copyBuf = *data_;
			try{
				for (auto act : buff_){
					copyBuf[act.first] = act.second;
				}
			}
			catch (std::exception& e){
				LOG_ERROR("Catched an copy exception, detail: %s", e.what());
				buff_.clear();
				return false;
			}

			data_->swap(copyBuf);

			//CAUTION: now the pointers in buff_ and copyBuf_ are overlapped with data_,
			//so it must be clear, otherwise the later modification to them will aslo directly act on data_.
			buff_.clear(); 
			return true;
		}

		bool DirectCommit(){
			try{
				for (auto act : buff_){
					(*data_)[act.first] = act.second;
				}
			}
			catch (std::exception& e){
				LOG_ERROR("Catched an assign exception, detail: %s", e.what());
				buff_.clear();
				return false;
			}

			//CAUTION: now the pointers in buff_ and copyBuf_ are overlapped with data_,
			//so it must be clear, otherwise the later modification to them will aslo directly act on data_.
			buff_.clear();
			return true;
		}

	public:
		bool Commit(bool bakMode = false){
			if (!bakMode){
				return DirectCommit();
			}
			else{
				return CopyCommit();
			}
			
		}

		//Call ClearChange to discard the modification if Commit failed
		void ClearChangeBuf(){
			buff_.clear();
		}

		virtual bool GetFromDB(const KEY& key, VALUE_PTR& ptr){
			return false;
		}

		virtual void updateToDB(){}
	};
}

#endif //TEMPLATE_ATOMIC_MAP_H
