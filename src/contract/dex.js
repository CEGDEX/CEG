let globalAttribute = {};
const globalAttributeKey = 'global_attribute';
const 1BU                = 100000000;

function feeValid(fee){
    let amount  = int64Sub(thisPayCoinAmount, fee);
    let realFee = int64Mul(amount, globalAttribute.feeRate) / 1BU;
    let trunc   = int64Mul(int64Div(realFee, 1000), 1000);

    let com = int64Compare(fee, trunc);
    return com === 0 || com === 1;
}

function ctpAllownce(issuer){
    let arg = { 'method':'allowance', 'params':{ 'own':sender, 'spender':thisAddress } };
    let res = contractQuery(issuer, arg);

    return res.allowance;
}

function comparePayAsset(asset){
    let x = thisPayAsset.key.issuer === asset.issuer;
    let y = thisPayAsset.key.code === asset.code;
    assert(x && y, 'Wrong ATP( issuer:' + asset.issuer + ', code:' + asset.code + ' ) paid.');

    return int64Compare(thisPayAsset.amount, asset.value);
}

function payCTP(issuer, from, to, value){
    let args = { 'method':'transferFrom', 'params':{ 'from':from, 'to':to, 'value':value}};
    payCoin(issuer, 0, args);
}

function revocation(key, order){
    if(order.own.issuer === undefined){ /* CEG */
        payCoin(order.maker, int64Add(order.own.value, order.fee));
    }
    else if(order.own.code === undefined){ /* CTP */
        payCTP(order.own.issuer, thisAddress, order.maker, order.own.value);
    }
    else{ /* ATP */
        payAsset(order.maker, order.own.issuer, order.own.code, order.own.value);
    }

    storageDel(key);
}

function makeOrder(own, target, fee, expiration){
    assert(blockTimestamp < expiration, 'Order date has expired.'); /*Need add time built-in interfacce*/
    //assert(stoI64Check(own.value) && stoI64Check(target.value), 'Target value must be alphanumeric.');

    if(own.issuer === undefined){ /* CEG */
        assert(addressCheck(target.issuer), 'The peer asset issuance address is invalid.');
        assert(feeValid(fee), 'Invalid fee.');
    }
    else if(own.code === undefined){ /* CTP */
        assert(target.issuer === undefined, 'Must have a party asset is CEG.');
        assert(int64Compare(ctpAllownce(own.issuer), own.value) === -1, 'Insufficient payment of CTP(ctp address: ' + own.issuer + ', value: ' + own.value + ').');
    }
    else{ /* ATP */
        assert(target.issuer === undefined, 'Must have a party asset is CEG.');
        assert(comparePayAsset(own) === -1, 'Insufficient payment of ATP(issuer: ' + own.issuer + ' code: ' + own.code + ', value: ' + own.value + ').');
    }
    
    let orderKey   = 'order_' + (globalAttribute.orderInterval[1] + 1);
    let orderValue = {'maker': sender, 'own':own, 'target':target, 'fee':fee, 'expiration': expiration};
    storageStore(orderKey, JSON.stringify(orderValue));

    globalAttribute.orderInterval[1] = globalAttribute.orderInterval[1] + 1;
}

function cancelOrder(key){
    let orderStr = storageLoad(key);
    assert(orderStr !== false, 'Order: ' + key + ' does not exist');

    let order = JSON.parse(orderStr);
    revocation(key, order);
}

function changeOrder(key, order, makerValue, takerValue, realFee){
    order.fee = int64Sub(order.fee, realFee);
    order.own.value = int64Sub(order.own.value, makerValue);
    order.target.value = int64Sub(order.target.value, takerValue);

    storageStore(key, stringify(order));
}

function other2bu(key, order, takerFee){ 
    assert(feeValid(takerFee), 'Invalid fee.');

    let total      = int64Add(order.target.value, takerFee);
    let com        = int64Compare(thisPayCoinAmount, total);
    let makerValue = order.own.value;
    if(com === -1){ /* partially take */
        let takerValue = int64Sub(thisPayCoinAmount, takerFee);
        makerValue     = int64Div(int64Mul(order.own.value, takerValue), order.target.value));
        changeOrder(key, order, makerValue, takerValue, takerFee);
    }
    else{
        storageDel(key);
        globalAttribute.orderInterval[0] = globalAttribute.orderInterval[0] + 1;
    } 

    if(order.own.code === undefined){ /*CTP*/
        payCTP(order.own.issuer, order.maker, sender, makerValue);
    }
    else{ /*ATP*/
        payAsset(sender, order.own.issuer, order.own.code, makerValue);
    }

    let bilateralFee = int64Add(takerFee, takerFee);
    payCoin(order.maker, int64Sub(thisPayCoinAmount, bilateralFee));
    if(com === 1){ /* Return the excess*/
        payCoin(sender, int64Sub(thisPayCoinAmount, total); 
    }

    globalAttribute.serviceFee = int64Add(globalAttribute.serviceFee, bilateralFee);
}

function bu2other(key, order){
    let com = 0;
    let takerValue = 0;
    if(order.target.code === undefined){
        takerValue = ctpAllownce(order.target.issuer);
        com = int64Compare(takerValue, order.target.value);
        payCTP(order.target.issuer, sender, order.maker, takerValue);
    }
    else{
        takerValue = thisPayAsset.amount; 
        com = comparePayAsset(order.target);
        payAsset(order.maker, thisPayAsset.key.issuer, thisPayAsset.key.code, takerValue);
    }

    let fee = 0;
    if(com === -1){
        let takerValue = int64Div(int64Mul(order.own.value, takerValue), order.target.value));
        let fee     = int64Div(int64Mul(order.fee, takerValue), order.target.value);

        changeOrder(key, order, takerValue, takerValue, fee);
        payCoin(sender, int64Sub(takerValue, fee));
    }
    else{
        fee = order.fee;
        payCoin(sender, int64Sub(order.own.value, fee));
        storageDel(key);

        globalAttribute.orderInterval[0] = globalAttribute.orderInterval[0] + 1;
    }

    globalAttribute.serviceFee = int64Add(globalAttribute.serviceFee, int64Add(fee, fee));
}

function takeOrder(key, fee){
    let orderStr = storageLoad(key);
    assert(orderStr !== false, 'Order: ' + key + ' does not exist');
    let order = JSON.parse(orderStr);

    if(blockTimestamp > order.expiration){
        return revocation(order);
    }

    if(order.own.issuer === undefined){
        bu2other(key, order);
    }
    else{
        other2bu(key, order, fee);
    }
}

function init(input_str){
    let params = JSON.parse(input_str).params;
    
    globalAttribute.owner = params.owner || sender;
    globalAttribute.feeRate = params.feeRate || 50000;
    globalAttribute.version = params.version || '1.0';
    globalAttribute.serviceFee = 0;
    globalAttribute.orderInterval = [0, 0];

    storageStore(globalAttributeKey, JSON.stringify(globalAttribute));
}

function main(input_str){
    let input = JSON.parse(input_str);
    globalAttribute = JSON.parse(storageLoad(globalAttributeKey));

    let params = input.params;
    if(input.method === 'makeOrder'){
        makeOrder(params.own, params.target, params.fee, params.expiration);
    }
    else if(input.method === 'cancelOrder'){
        cancelOrder(params.order);
    }
    else if(input.method === 'takeOrder'){
        takeOrder(params.order, params.fee);
    }
    else if(input.method === 'updateFeeRate'){
        updateFeeRate(params.rate);
    }
    else if(input.method === 'updateOwner'){
        updateOwner(params.owner);
    }
    else if(input.method === 'clearExpiredOrder'){
        clearExpiredOrder();
    }
    else if(input.method === 'withdrawFee'){
        withdrawFee(params.value);
    }
    else{
        throw '<Main interface passes an invalid operation type>';
    }

    storageStore(globalAttributeKey, JSON.stringify(globalAttribute));
}

function query(input_str){

    let result = {};
    let input  = JSON.parse(input_str);

    if(input.method === 'dexInfo'){
        result.dexInfo = dexInfo();
    }
    else if(input.method === 'getOrder'){
        result.order = getOrder(params.order);
    }
    else if(input.method === 'getOrderInterval'){
        result.interval = getOrderInterval();
    }
    else{
       	throw '<Query interface passes an invalid operation type>';
    }
    return JSON.stringify(result);
}
