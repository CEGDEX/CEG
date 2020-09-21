﻿/*
	CEG is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	CEG is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with CEG.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef val_CACHE_H
#define val_CACHE_H

#include <map>
#include <memory>

namespace utils {
	/************************************************************************
			|add	|mod	|del
			____|_______|_______|_______
			add	|false	|add	|remove |
			mod	|false	|mod	|del    |
			del	|add	|false	|false  |

			************************************************************************/
	enum ChangeAction {
		KEEP = 0,
		ADD = 1,
		MOD = 2,
		DEL = 3
	};

	template <class Key, class Value, class Sort>
	class EntryCache {
	protected:
	public:
		typedef std::shared_ptr<Value> pointer;

		struct Record {
			Record(pointer val, const ChangeAction &action) :value_(val), action_(action) {}
			Record() {}
			std::shared_ptr<Value> value_;
			ChangeAction action_;
		};

		std::map<Key, Record, Sort> entries_;
		std::shared_ptr<EntryCache> parent_;

	private:
		bool GetRecord(const Key &key, Record &r) {
			auto it = entries_.find(key);
			if (it != entries_.end()) {
				r = it->second;
				return true;
			}

			if (parent_) {
				return parent_->GetRecord(key, r);
			}

			if (LoadValue(key, r.value_)) {
				return true;
			}
			return false;
		}

	public:
		EntryCache(std::shared_ptr<EntryCache> parent) {
			parent_ = parent;
		}

		EntryCache() {
			parent_ = nullptr;
		}

		~EntryCache() {}

		virtual bool LoadValue(const Key&, pointer &v_pt) = 0;
		bool MergeFromBranch(EntryCache &branch) {

			auto &branch_entries = branch.entries_;
			for (auto it = branch.entries_.begin(); it != branch.entries_.end(); it++) {
				switch (it->second.action_) {
				case ADD:{
					auto x = entries_.find(it->first);
					if (x != entries_.end()) {
						if (x->second.action_ == DEL) {
							x->second = it->second;
							x->second.action_ = MOD;
						}
						else
							return false;
					}
					else {
						entries_.insert({ it->first, it->second });
					}
				}
				case MOD:{
					auto x = entries_.find(it->first);
					if (x != entries_.end()) {
						x->second = it->second;
					}
					else {
						entries_.insert({ it->first, it->second });
					}
					break;
				}
				case DEL:{
					auto x = entries_.find(it->first);
					if (x != entries_.end()) {
						if (x->second.action_ == ADD) {
							entries_.erase(x);
						}
						else if (x->second.action_ == MOD) {
							x->second.action_ = DEL;
						}
						else if (x->second.action_ == KEEP) {
							x->second.action_ = DEL;
						}
						else if (x->second.action_ == DEL) {
							return false;
						}
					}
					else {
						entries_.insert({ it->first, it->second });
					}
					break;
				}
				default:
					break;
				}
			}
			return true;
		}

		bool GetEntry(const Key &key, pointer &pval) {
			auto it = entries_.find(key);
			if (it != entries_.end()) {
				if (it->second.action_ != DEL) {
					pval = it->second.value_;
					return true;
				}

				return false;
			}

			Record r;
			if (parent_ != nullptr) {
				if (parent_->GetRecord(key, r) && r.action_ != DEL) {
					pval = std::make_shared<Value>(r.value_);
					entries_.insert({ key, Record(pval, MOD) });
					return true;
				}
					
				return false;
			}

			if (LoadValue(key, pval)) {
				entries_.insert({ key, Record(pval, MOD) });
				return true;
			}

			return false;
		}

		bool AddEntry(const Key &key, pointer pval) {
			auto it = entries_.find(key);
			if (it != entries_.end()) {
				if (it->second.action_ == DEL) {
					it->second = Record(pval, MOD);
					return true;
				}

				return false;
			}

			Record r;
			if (parent_ != nullptr) {
				if (parent_->GetRecord(key, r) && r.action_ != DEL) {
					return false;
				}

				entries_.insert({ key, Record(pval, ADD) });
				return true;
			}

			if (!LoadValue(key, pval)) {
				entries_.insert({ key, Record(pval, ADD) });
				return true;
			}

			return false;
		}

		bool DeleteEntry(const Key &key) {
			auto it = entries_.find(key);
			if (it != entries_.end()) {
				if (it->second.action_ == DEL) {
					return false;
				}
				it->second.action_ = DEL;
			}

			Record r;
			if (GetRecord(key, r) && r.action_ != DEL) {
				r.action_ = DEL;
				entries_.insert({ key, r });
				return true;
			}
			return false;
		}

		std::map<Key, Record, Sort> GetEntries() {
			return entries_;
		}
	};
}
#endif
