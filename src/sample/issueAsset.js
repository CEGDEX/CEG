"use strict";
function issue (code, newAmount, mark){
    let msg = {};
    let amount =storageLoad("totaCirculation");
    msg.code = code;
    msg.amount = newAmount;
    msg.blockTimestamp = blockTimestamp;
    storageStore(mark, JSON.stringify(msg));
    if (amount === false){
        storageStore("totaCirculation", newAmount);
    }else{
        storageStore("totaCirculation", int64Add(amount, newAmount));
    }
    issueAsset(code, newAmount);
    return true;
}
function drawCEG (drawCEGAmount){
    assert(drawCEGAmount !== undefined, "destAmount is nil");
    assert(storageLoad("managedAddress") === sender, "sender is not managedAddress, managedAddress:" + storageLoad(" managedAddress") + " sender:"+sender);
    let msg = {};
    msg.destAddress = sender;
    msg.drawCEGAmount = drawCEGAmount;
    msg.blockTimestamp = blockTimestamp;
    storageStore("drawCEG", JSON.stringify(msg));
    payCoin(sender, drawCEGAmount);
    return true;
}
function conversion (destAddress, code, payAmount){
    assert(addressCheck(destAddress) === true, "destAddress is error");
    assert(typeof code === "string", "destAddress is error");
    assert(stoI64Check(payAmount) === true,"payAmount is error");
    assert(payAmount === thisPayCoinAmount, "payAmount is error payAmount:" + payAmount);
    let conversionRate = storageLoad("conversionRate");
    let amount = int64Div(payAmount, conversionRate);
    payAsset(destAddress, thisAddress, code, amount);
    return true;
}
function setConversionRate (conversionRate){
    assert(storageLoad("managedAddress") === sender, "sender is not managedAddress, sender:" + sender);
    assert(stoI64Check(conversionRate), "conversionRate unreal");
    storageStore("conversionRate", conversionRate);
    return true;
}
function main (input_str){
    assert(input_str !== undefined, "input is undefined.");
    let input = JSON.parse(input_str);
    if (input.method === "setConversionRate"){
        setConversionRate(input.params.setConversionRate);
    }else if  (input.method === "conversion"){
        conversion(input.params.destAddress, input.params.code, input.params.payAmount);
    }else if  (input.method === "drawCEG"){
        drawCEG(input.params.drawCEGAmount);
    }else if  (input.method === "issue"){
        issue(input.params.code,input.params.amount, "additionalIssue");
    }else {
		throw "<undidentified operation type>";
	}
  return true;
}
function query(input_str)
{
    let result = {};
	let input = JSON.parse(input_str);
	if (input.method === "balance") {
		result.balance = getBalance(thisAddress);
	} else if (input.method === "conversionRate") {
        result.conversionRate = storageLoad("conversionRate");
    } else if (input.method === "totaCirculation") {
        result.conversionRate = storageLoad("totaCirculation");
	} else {
		throw "<unidentified operation type>";
	}
	log(result);
	return JSON.stringify(result);
}
function init(initInput)
{
    assert(initInput !== undefined, "initInput is undefined.");
    let input = JSON.parse(initInput);
    assert(getBalance(input.params.managedAddress) !== 0,"managedAddress unreal");
    assert(stoI64Check(input.params.conversionRate),"conversionRate unreal");
    assert(stoI64Check(input.params.amount) === true && typeof input.params.code === "string"&&addressCheck(input.params.managedAddress) === true );
    issue(input.params.code,input.params.amount,"initIssue");
    storageStore("conversionRate", input.params.conversionRate);
    storageStore("managedAddress", input.params.managedAddress);
}