/*
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

#ifndef ENVIRONMENT_H_
#define ENVIRONMENT_H_

#include <proto/cpp/chain.pb.h>
#include <proto/cpp/consensus.pb.h>
#include <utils/atom_map.h>
#include <main/configure.h>
#include <json/value.h>
#include "account.h"

namespace CEG {
	class Environment : public utils::AtomMap<std::string, AccountFrm>{
	public:
		typedef utils::AtomMap<std::string, Json::Value>::Map settingKV;
		const std::string validatorsKey = "validators";
		const std::string feesKey = "configFees";

		AtomMap<std::string, Json::Value> settings_;

		Environment() = default;
		Environment(Environment const&) = delete;
		Environment& operator=(Environment const&) = delete;
		Environment(Map* data, settingKV* settings);

		bool GetEntry(const std::string& key, AccountFrm::pointer &frm);
		bool AddEntry(const std::string& key, AccountFrm::pointer frm);

		bool UpdateFeeConfig(const Json::Value &fee_config);
		bool GetVotedFee(const protocol::FeeConfig &old_fee, protocol::FeeConfig& new_fee);

		Json::Value& GetValidators();
		bool UpdateNewValidators(const Json::Value& validators);
		bool GetVotedValidators(const protocol::ValidatorSet &old_validator, protocol::ValidatorSet& new_validator);

		bool Commit();
		void ClearChangeBuf();

		virtual bool GetFromDB(const std::string &address, AccountFrm::pointer &account_ptr);
		static bool AccountFromDB(const std::string &address, AccountFrm::pointer &account_ptr);
		std::shared_ptr<Environment> NewStackFrameEnv();
	};
}
#endif