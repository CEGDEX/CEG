#ifndef V8_CONTRACT_H_
#define V8_CONTRACT_H_

#include "contract.h"

#include <v8.h>
#include <libplatform/libplatform.h>
#include <libplatform/libplatform-export.h>


namespace CEG {
	class V8Contract : public Contract {
		v8::Isolate* isolate_;
	public:
		V8Contract(bool readonly, const ContractParameter &parameter);
		virtual ~V8Contract();
	public:
		virtual bool Execute();
		virtual bool Cancel();
		virtual bool Query(Json::Value& jsResult);
		virtual bool SourceCodeCheck();
		static bool Initialize(int argc, char** argv);

	private:
		static bool LoadJsFuncList();
		static bool LoadJsLibSource();
		static bool LoadJslintGlobalString();
		static std::string GetGlobalString();
		static std::map<std::string, std::string> jslib_sources_;

		typedef std::map<std::string, v8::FunctionCallback> JsFunctions;
		typedef struct JsFuncList{
			JsFunctions read_;
			JsFunctions write_;
		};
		static std::map<std::string, JsFuncList> js_obj_;

		static std::string user_global_string_;
		static std::string user_global_string_gt1001_;

		//for contract interface
		static const std::string init_name_;
		static const std::string main_name_;
		static const std::string query_name_;
		
		//for built-in variables
		static const std::string sender_name_;
		static const std::string this_address_;		
		static const std::string trigger_tx_name_;
		static const std::string trigger_tx_index_name_;
		static const std::string this_header_name_;
		static const std::string pay_coin_amount_name_;
		static const std::string pay_asset_amount_name_;
		static const std::string block_timestamp_name_;
		static const std::string block_number_name_;

		//for check source
		static const std::string call_jslint_;

		static utils::Mutex isolate_to_contract_mutex_;
		static std::unordered_map<v8::Isolate*, V8Contract *> isolate_to_contract_;

		static v8::Platform* 	platform_;
		static v8::Isolate::CreateParams create_params_;

		static protocol::AssetKey GetAssetFromJsObject(v8::Isolate* isolate, v8::Local<v8::Object> js_object);
		static bool RemoveRandom(v8::Isolate* isolate, Json::Value &error_msg);
		static v8::Local<v8::Context> CreateContext(v8::Isolate* isolate, bool readonly);
		static V8Contract *GetContractFrom(v8::Isolate* isolate);
		static Json::Value ReportException(v8::Isolate* isolate, v8::TryCatch* try_catch);
		static const char* ToCString(const v8::String::Utf8Value& value);
		static void CallBackLog(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackTopicLog(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackGetAccountAsset(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackGetAccountMetadata(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackSetValidators(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackGetValidators(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackAddressValidCheck(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackPayCoin(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackCall(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackCreate(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackIssueAsset(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackPayAsset(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Include(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void InternalCheckTime(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Get transaction info from a transaction
		static void CallBackGetTransactionInfo(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackContractQuery(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackContractQueryGt1001(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackDelegateCall(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackDelegateQuery(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackGetContractProperty(const v8::FunctionCallbackInfo<v8::Value>& args);

		static V8Contract *UnwrapContract(v8::Local<v8::Object> obj);
		static bool JsValueToCppJson(v8::Handle<v8::Context>& context, v8::Local<v8::Value>& jsvalue, Json::Value& jsonvalue);
		static bool CppJsonToJsValue(v8::Isolate* isolate, const Json::Value& jsonvalue, v8::Local<v8::Value>& jsvalue);
		static void CallBackConfigFee(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Assert an expression.
		static void CallBackAssert(const v8::FunctionCallbackInfo<v8::Value>& args);

		//to base unit, equal to * pow(10, 8)
		static void CallBackToBaseUnit(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Get the balance of the given account 
		static void CallBackGetBalance(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Get the hash of one of the 1024 most recent complete blocks
		static void CallBackGetBlockHash(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackSha256(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackVerify(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackToAddress(const v8::FunctionCallbackInfo<v8::Value>& args);

		//Sends a message with arbitrary date to a given address path
		static void CallBackStorageStore(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CallBackStorageDel(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void SetMetaData(const v8::FunctionCallbackInfo<v8::Value>& args, bool is_del = false);
		static void CallBackStorageLoad(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Get caller 
		static void CallBackGetCaller(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Get block number 
		static void CallBackBlockNumber(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Get block number 
		static void CallBackGetTxOrigin(const v8::FunctionCallbackInfo<v8::Value>& args);
		//selfDestruct

		//Get block timestamp 
		static void CallBackGetBlockTimestamp(const v8::FunctionCallbackInfo<v8::Value>& args);
		//str to int64 check
		static void CallBackStoI64Check(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 add
		static void CallBackInt64Add(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 sub
		static void CallBackInt64Sub(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 div
		static void CallBackInt64Div(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 mod
		static void CallBackInt64Mod(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 mod
		static void CallBackInt64Mul(const v8::FunctionCallbackInfo<v8::Value>& args);
		//Int64 compare
		static void CallBackInt64Compare(const v8::FunctionCallbackInfo<v8::Value>& args);

	private:
		bool ExecuteCode();

		typedef enum tagDataEncodeType {
			BASE16 = 0,
			RAW_DATA = 1,
			BASE64 = 2
		}DataEncodeType;

		static bool TransEncodeType(const v8::Local<v8::Value> &arg, DataEncodeType &data_type);
		static bool TransEncodeData(const v8::Local<v8::Value> &raw_data, const DataEncodeType &encode_type, std::string &result_data);

		void SetV8InterfaceFunc(v8::Local<v8::Context> context, bool readonly);
		void CreateJsObject(v8::Local<v8::Context> context, bool readonly);
		void SetV8ObjectFunc(v8::Local<v8::Object> object, JsFunctions &js_functions);
	};
}

#endif