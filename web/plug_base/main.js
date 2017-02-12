var APPKEY = '5697113d4407a3cd028abead';
var TOPIC_REPORT = 'yunba_smart_plug';
var ALIAS = 'plc_0';

$(window).bind("beforeunload", function() {
    yunba.unsubscribe({ 'topic': TOPIC_REPORT }, function(success, msg) {
        if (!success) {
            console.log(msg);
        }
    });
})

$(document).ready(function() {
    window.send_time = null;
    window.first_msg = true;

    $('#span-status').text('正在连接云巴服务器...');
    center = { lat: 22.542955, lng: 114.059688 };

    window.yunba = new Yunba({
        server: 'sock.yunba.io',
        port: 3000,
        appkey: APPKEY
    });

    // 初始化云巴 SDK
    yunba.init(function(success) {
        if (success) {
            var cid = Math.random().toString().substr(2);
            console.log('cid: ' + cid);
            window.alias = cid;

            // 连接云巴服务器
            yunba.connect_by_customid(cid,
                function(success, msg, sessionid) {
                    if (success) {
                        console.log('sessionid：' + sessionid);

                        // 设置收到信息回调函数
                        yunba.set_message_cb(yunba_msg_cb);
                        // TOPIC
                        yunba.subscribe({
                                'topic': TOPIC_REPORT
                            },
                            function(success, msg) {
                                if (success) {
                                    console.log('subscribed');
                                    yunba_sub_ok();
                                } else {
                                    console.log(msg);
                                }
                            }
                        );
                    } else {
                        console.log(msg);
                    }
                });
        } else {
            console.log('yunba init failed');
        }
    });
});

function yunba_msg_cb(data) {
    console.log(data);
    if (data.topic != TOPIC_REPORT) {
        return;
    }

    var msg = JSON.parse(data.msg);
    if (msg.devid != ALIAS) {
        return;
    }
    $('#span-loading').css("display", "none");
    if (msg.status == 1) {
        $('#span-status').text('状态: 已打开');
        $('#btn-on').css("display", "none");
        $('#btn-off').css("display", "block");
        $("path").attr("fill", "rgb(200, 50, 50)");
    } else if (msg.status == 0) {
        $('#span-status').text('状态: 已关闭');
        $('#btn-off').css("display", "none");
        $('#btn-on').css("display", "block");
        $("path").attr("fill", "rgb(200, 200, 200)");
    }
}

function send_message(message) {
    yunba.publish_to_alias({
        'alias': ALIAS,
        'msg': message,
    }, function(success, msg) {
        if (!success) {
            console.log(msg);
        }
    });
}

function yunba_sub_ok() {
    $('#span-status').text('正在获取插座状态...');
    var msg = JSON.stringify({ cmd: "plug_get", devid: ALIAS });
    send_message(msg)
}

$('#btn-on').click(function() {
    var msg = JSON.stringify({ cmd: "plug_set", devid: ALIAS, status: 1 });
    send_message(msg)
});

$('#btn-off').click(function() {
    var msg = JSON.stringify({ cmd: "plug_set", devid: ALIAS, status: 0 });
    send_message(msg)
});
