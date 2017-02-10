// Appkey: 5697113d4407a3cd028abead
// topic: yunba_smart_plug
// alias: smart_plug_0
// devid: 1234567890
// 　　控制以及状态查询使用的 publish to alias 方法。

// 　　状态上报使用的是　publish 方法。

// 控制开关：
// {"cmd": "plug_set", "devid": "<smart-plug-device-id>", "status": <status>}
// status 为１表示开，为０表示关。

// 状态查询：
// {"cmd": "plug_get", "devid": "<smart-plug-device-id>"}
// 当插座状态改变时，会主动上报自身状态：

// {"status":<status>, "devid":"<smart-plug-device-id>"}

var config = {
    APPKEY: '5697113d4407a3cd028abead',
    TOPIC: 'yunba_smart_plug',
    // ALIAS: 'smart_plug_0',
    // DEVID:'1234567890'
    ALIAS: 'plc_0',
    DEVID: 'plc_0'
}
var yunba = new Yunba({
    server: 'sock.yunba.io',
    port: 3000,
    appkey: config.APPKEY
});

$(document).ready(function() {
    // if (window.location.pathname) {};

    init();
});

/**
 * [init description]
 * @return {[type]} [description]
 */
function init() {
    var randomID = MathRand();
    yunba.init(function(success) {
        if (success) {

            yunba.connect_by_customid('' + randomID + '', function(success, msg, sessionid) {
                if (success) {
                    console.log('你已成功连接到消息服务器，会话ID：' + sessionid);
                    subscribe(config.TOPIC);
                    publish_to_alias(config.ALIAS, '{"cmd": "plug_get", "devid": "' + config.DEVID + '"}')
                    $(".stecker").removeClass("hide");
                    $(".wrapper").addClass("hide");
                } else {
                    console.log(msg);
                }
            });
        }
    });
};
/**
 * [subscribe description]
 * @param  {[type]} topic [description]
 * @return {[type]}       [description]
 */
function subscribe(topic) {
    yunba.subscribe({
            'topic': topic
        },
        function(success, msg) {
            if (success) {
                console.log('你已成功订阅频道：' + topic + '');



            } else {
                console.log(msg);

            }
        }
    );

    getMessage();
}
/**
 * [getMessage description]
 * @return {[type]} [description]
 */
function getMessage() {
    yunba.set_message_cb(function(data) {
        var msg = JSON.parse(data.msg);
        console.log(data);
        if (msg.status === 1 && msg.devid == config.DEVID) {
            setStatus("on");
            setButtonStatus("on");

            console.log("on");
            console.log(data);
            console.log(msg);
            console.log(msg.status);
            console.log(msg.devid);
        } else if (msg.status === 0 && msg.devid == config.DEVID) {
            setStatus("off");
            setButtonStatus("off");


        } else {
            console.log("invalid parameters")
        }
    });

}
/**
 * [publish_to_alias description]
 * @param  {[type]} alias [description]
 * @param  {[type]} msg   [description]
 * @param  {[type]} opts  [description]
 * @return {[type]}       [description]
 */
function publish_to_alias(alias, msg, opts) {
    console.log(msg);
    setButtonStatus("loading");
    yunba.publish2_to_alias({
        'alias': alias,
        'msg': msg,
        'opts': opts


    }, callback)
};
/**
 * @param  {[type]}
 * @param  {[type]}
 * @return {Function}
 */
function callback(success, msg) {
    console.log('value :' + success + ' msgId :' + msg.messageId);
    //console.log(msg.toString());


}
/**
 * 随机ID生成
 * 
 */
function MathRand() {
    var myDate = new Date();
    var mytime = myDate.getTime();
    var Num = "";
    for (var i = 0; i < 6; i++) {
        Num += Math.floor(Math.random() * 10);
    }
    Num = Num + "-" + mytime;
    return Num;
};

// 动画控制
function setStatus(param) {
    if (param === "on") {
        $("path").attr("fill", "rgb(50,205,50)");
    } else if (param === "off") {
        $("path").attr("fill", "#aaa");
    };
}
// 按钮控制
function setButtonStatus(status) {


    if (status === "on") {
        $("button").removeClass();
        $("button").addClass('ready');
        $('.detail').html('Plug base status: On');
    } else if (status === "loading") {
        $("button").removeClass();
        $("button").addClass('loading');
        $('.detail').html('Plug base status: Loading');
    } else if (status === "off") {
        $("button").removeClass();
        $('.detail').html('Plug base status: Off');
    };

};


$('button').on('click', function() {
    if (this.className === 'ready') {
        //发送关闭请求
        publish_to_alias(config.ALIAS, '{"cmd": "plug_set", "devid": "' + config.DEVID + '", "status": 0}');

    };
    if (this.className === '') {
        //发送打开请求
        publish_to_alias(config.ALIAS, '{"cmd": "plug_set", "devid": "' + config.DEVID + '", "status": 1}');
    };
    if (this.className === 'loading') {
        //刷新

        location.reload();

    };
})
