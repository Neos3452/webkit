console.log('main script');
var app = null;
$(window).load(function() {
'use strict';

var DEBUG = 1;
var MOCKUP = 1;

function LOG(msg) { if (DEBUG) console.log(msg); }
function WARN(msg) { if (DEBUG) console.warn(msg); }
function ERR(msg) { console.error(msg); }

function setUpMockup() {
    LOG('Setting up mockup');
    window.RemoteApplication = function(send) {
        this.send = send;
        this.messageReceived = function (data) {
            this.dispatchEvent(new MessageEvent('message', {data: data}));
        }.bind(this);
        this.disconnect = function () {
            this.setState('disconnected');
        };
        this.reconnect = function () {
            this.setState('opened');
        }.bind(this);
        this.connectionState = 'connecting';
        this.setState = function (state) {
            this.connectionState = state;
            this.dispatchEvent(new Event('statechange'));
        }.bind(this);
    };
    window.LocalNetworkDeviceDescriptor = function(userAgent) {
        if (!(typeof userAgent === 'string' || userAgent instanceof String)) {
            throw TypeError;
        }
        this.userAgent = userAgent;
    };
    window.LocalNetworkDevice = function(descriptor, launch) {
        if (!(descriptor instanceof LocalNetworkDeviceDescriptor) ||
            !(typeof launch === 'function' || launch instanceof Function)) {
            throw TypeError;
        }
        this.descriptor = descriptor;
        this.launch = launch;
    };

    navigator.getRemoteInvoker = function () {
        return null;
    };

    navigator.localNetworkDevices =  {
        enumerateDevices: function() {
            return new Promise(function (resolve, reject) {
                window.setTimeout(function () {
                    resolve([
                        new LocalNetworkDevice(new LocalNetworkDeviceDescriptor('LOL'), function () {
                            return new Promise(function (resolve, reject) {
                                window.setTimeout(function () {
                                    reject();
                                }, 0);
                            });
                        })
                    ]);
                }, 500)
            });
        }
    }
}

function isDialSupported() {
    if (!navigator.getRemoteInvoker || !navigator.localNetworkDevices) {
        if (MOCKUP) {
            setUpMockup();
            return true;
        }
        return false;
    }
    return true;
}

if (!isDialSupported()) {
    ERR('Dial not supported');
    return;
}

var remote = navigator.getRemoteInvoker();
if (!remote) {
    LOG('No remote invoker');
    
    var myScriptElement = document.createElement('script');
    myScriptElement.innerHTML = "(function(){\n" + "    'use strict';\n" + "\n" + "    var DEBUG = 1;\n" + "\n" + "    function LOG(msg) { if (DEBUG) console.log(msg); }\n" + "    function WARN(msg) { if (DEBUG) console.warn(msg); }\n" + "    function ERR(msg) { console.error(msg); }\n" + "\n" + "    LOG('Begin injection');\n" + "\n" + "    function HTMLMediaElementController(mediaElement) {\n" + "        LOG('injected!');\n" + "        this.mediaElement = mediaElement;\n" + "        this.mediaElement.hijackController = this;\n" + "        this.suppressCommands = false;\n" + "        // this.delegate = null;\n" + "        this.paused = true;\n" + "        this.time = this.mediaElement.currentTime;\n" + "        mediaElement.addEventListener('playing', function () {\n" + "            LOG('play...');\n" + "            // if (!this.suppressCommands) {\n" + "            //     this.paused = false;\n" + "            // }\n" + "            // this.onPlay();\n" + "            this.paused = false;\n" + "        }.bind(this));\n" + "        mediaElement.addEventListener('pause', function () {\n" + "            LOG('pause..');\n" + "            // if (!this.suppressCommands) {\n" + "            //     this.paused = true;\n" + "            // }\n" + "            // this.onPause();\n" + "            this.paused = true;\n" + "        }.bind(this));\n" + "        mediaElement.addEventListener('timeupdate', function () {\n" + "            if (!this.suppressCommands)\n" + "                this.time = this.mediaElement.currentTime;\n" + "        }.bind(this));\n" + "    }\n" + "\n" + "    HTMLMediaElementController.prototype.onPause = function() {\n" + "        // if (this.delegate)\n" + "        // this.delegate.pause();\n" + "        window.postMessage({webdialtag:'video', cmd:'pause'}, '*');\n" + "        return !this.suppressCommands;\n" + "    };\n" + "\n" + "    HTMLMediaElementController.prototype.onPlay = function() {\n" + "        // if (this.delegate)\n" + "        // this.delegate.play();\n" + "        window.postMessage({webdialtag:'video', cmd:'play'}, '*');\n" + "        return !this.suppressCommands;\n" + "    };\n" + "    HTMLMediaElementController.prototype.onSeek = function(time) {\n" + "        this.setTime(time);\n" + "        window.postMessage({webdialtag:'video', cmd:'seek', time:time}, '*');\n" + "    };\n" + "\n" + "    HTMLMediaElementController.prototype.pause = function() {\n" + "        LOG('synthetic pause');\n" + "        this.paused = true;\n" + "        if (!this.suppressCommands) {\n" + "            this.mediaElement.pause();\n" + "        } else {\n" + "            this.mediaElement.dispatchEvent(new Event('pause'));\n" + "        }\n" + "    };\n" + "\n" + "    HTMLMediaElementController.prototype.play = function() {\n" + "        LOG('synthetic play');\n" + "        this.paused = false;\n" + "        if (!this.suppressCommands) {\n" + "            this.mediaElement.play();\n" + "        } else {\n" + "            this.mediaElement.dispatchEvent(new Event('playing'));\n" + "        }\n" + "    };\n" + "\n" + "    HTMLMediaElementController.prototype.setTime = function(time) {\n" + "        if (!this.suppressCommands) {\n" + "            LOG('cannot set time while not supressing');\n" + "        } else {\n" + "            this.time = time;\n" + "            this.mediaElement.dispatchEvent(new Event('timeupdate'));\n" + "        }\n" + "    };\n" + "\n" + "    HTMLMediaElementController.prototype.orgPaused = HTMLMediaElement.prototype.__lookupGetter__('paused');\n" + "    HTMLMediaElement.prototype.__defineGetter__('paused', function() {\n" + "        var orgState = HTMLMediaElementController.prototype.orgPaused.call(this);\n" + "        if (this.hijackController) {\n" + "            if (orgState != this.hijackController.paused)\n" + "                LOG('Org state ' + orgState + ' returning ' + this.hijackController.paused);\n" + "            return this.hijackController.paused;\n" + "        }\n" + "        return orgState;\n" + "    });\n" + "\n" + "    HTMLMediaElementController.prototype.orgCurrentTime = HTMLMediaElement.prototype.__lookupGetter__('currentTime');\n" + "    HTMLMediaElement.prototype.__defineGetter__('currentTime', function() {\n" + "        if (this.hijackController && this.hijackController.suppressCommands) {\n" + "            return this.hijackController.time;\n" + "        }\n" + "        return HTMLMediaElementController.prototype.orgCurrentTime.call(this);\n" + "    });\n" + "\n" + "    HTMLMediaElementController.prototype.orgCurrentTimeSet = HTMLMediaElement.prototype.__lookupSetter__('currentTime');\n" + "    HTMLMediaElement.prototype.__defineSetter__('currentTime', function(time) {\n" + "        if (this.hijackController && this.hijackController.suppressCommands) {\n" + "            LOG('App seeked to ' + time);\n" + "            this.hijackController.onSeek(time);\n" + "        }\n" + "        HTMLMediaElementController.prototype.orgCurrentTimeSet.call(this, time);\n" + "    });\n" + "\n" + "    HTMLMediaElementController.prototype.orgPlay = HTMLMediaElement.prototype.play;\n" + "    HTMLMediaElement.prototype.play = function() {\n" + "        if (this.hijackController) {\n" + "            LOG('hijacking play');\n" + "            if (!this.hijackController.onPlay()) {\n" + "                return;\n" + "            }\n" + "        }\n" + "        HTMLMediaElementController.prototype.orgPlay.call(this);\n" + "    };\n" + "\n" + "    HTMLMediaElementController.prototype.orgPause = HTMLMediaElement.prototype.pause;\n" + "    HTMLMediaElement.prototype.pause = function() {\n" + "        if (this.hijackController) {\n" + "            LOG('hijacking pause');\n" + "            if (!this.hijackController.onPause()) {\n" + "                return;\n" + "            }\n" + "        }\n" + "        HTMLMediaElementController.prototype.orgPause.call(this);\n" + "    };\n" + "\n" + "// var orgCtr = HTMLMediaElement.prototype.constructor;\n" + "// HTMLMediaElement.prototype.constructor = function() {\n" + "// 	orgCtr.call(this, arguments);\n" + "// 	this.hijackController = new HTMLMediaElementController(this);\n" + "// 	LOG('injection complete');\n" + "// };\n" + "\n" + "window.addEventListener('message', function(e) {\n" + "	if (e.data.webdialtag && e.data.webdialtag == 'control') {\n" + "        var video = document.querySelector('video');\n" + "        if (video) {\n" + "        switch (e.data.cmd) {\n" + "            case 'inject':\n" + "                video.hijackController = new HTMLMediaElementController(video);\n" + "                break;\n" + "            case 'supress':\n" + "                video.hijackController.suppressCommands = true;\n" + "                break;\n" + "            case 'unsupress':\n" + "                video.hijackController.suppressCommands = false;\n" + "                video.currentTime = video.hijackController.time;\n" + "                break;\n" + "            case 'play':\n" + "                video.hijackController.play();\n" + "                break;\n" + "            case 'pause':\n" + "                video.hijackController.pause();\n" + "                break;\n" + "            case 'timeupdate':\n" + "                video.hijackController.setTime(e.data.time);\n" + "                break;\n" + "            default:\n" + "                LOG('unrecognized cmd ' + e.data.cmd);\n" + "        }\n" + "        } else {\n" + "            LOG('No video element');\n" + "        }\n" + "	}\n" + "}, false);\n" + "\n" + "// window.HTMLMediaElementController = HTMLMediaElementController;\n" + "    LOG('setup complete');\n" + "})();";
    var head = document.querySelector('head').appendChild(myScriptElement);

    var controls = $('.ytp-right-controls');
    if (controls.length) {
        LOG('Found menu settings');
        injectWebDialButton(controls);
    } else {
        ERR('Could not find menu settings, setting up MutationObserver');
        window.controlsObserver = new MutationObserver(function (mutations) {
            mutations.forEach(function (mutation) {
                for (var i = 0; i < mutation.addedNodes.length; ++i) {
                    var node = $(mutation.addedNodes[i]);
                    if (node.hasClass('ytp-right-controls')) {
                        window.controlsObserver.disconnect();
                        injectWebDialButton(node);
                    }
                }
            });
        });
        window.controlsObserver.observe(document, {childList: true});
    }
} else {
    app = remote;
    // We do not display dial control when remote invoker is present
    LOG('This was launched by other');
    var videoFound = $('video');
    if (videoFound.length == 0) {
        ERR('No video tag found!');
    } else {
        LOG('there is a video tag');
        new RemoteReceiver(remote);
    }
}

function injectWebDialButton(controls) {
    //var webDialButton = controls.find('.ytp-remote-button').clone();
    var webDialButton = controls.find('[class="ytp-button"]').clone();
    if (webDialButton.length !== 1) {
        ERR('Could not find remote button');
        return;
    }
    webDialButton.prepend('<span style="position: absolute; font-size: 60%; text-align: center; width: inherit;">Web</span>');
    webDialButton.show();
    controls.find('.ytp-fullscreen-button').before(webDialButton);
    var webDialMenu = null;
    webDialButton.click(function () {
        LOG('click!');
        if (!webDialMenu) {
            webDialMenu = new WebDialMenu();
        }
        webDialMenu.show();
    });
    LOG('button injected');
}

function YoutubeWebDialEvent(type) {
    this.type = type;
}

function WebDialMenu() {
    var that = this;

    function MenuItem(name) {
        // |name| obviously has to sanitized
        this.name = name;
        this.node = $('<div class="ytp-menuitem" tabindex="0" role="menuitemradio" aria-checked="false">' +
            '<div class="ytp-menuitem-label">' + this.name + '</div>' +
            '</div>');
    }

    MenuItem.prototype.check = function (value) {
        this.node.attr('aria-checked', value.toString());
    };

    that.load = function () {
        that.itemsContainer.append(that.loadingPanel);
        that.adjustSize();

        navigator.localNetworkDevices.enumerateDevices().then(function (devices) {
            LOG('Detected ' + devices.length + ' devices');
            that.devices = [];
            for (var i = 0; i < devices.length; ++i) {
                var newItem = new MenuItem(devices[i].descriptor.userAgent);
                newItem.device = devices[i];
                that.devices.push(newItem);
            }
            that.constructList();
        }, function (err) {
            ERR('Could not enumerate ' + err);
            that.loadingPanel.detach();
            that.itemsContainer.append(new MenuItem('Could not enumerate').node);
        })
    };

    that.launch = function (device) {
        if (device === that.launchedDevice) {
            return;
        }
        LOG('Launching on ' + device.name);

        if (that.launchedDevice.controler) {
            that.launchedDevice.controler.disconnect();
            delete that.launchedDevice.controler;
        }
        that.launchedDevice.check(false);
        device.check(true);
        that.launchedDevice = device;
        if (that.launchedDevice.device) {
            that.launchedDevice.device.launch(window.location.href).then(function (app) {
                LOG('Launch was successful');
                // that.launchedDevice.app = app;
                that.launchedDevice.controler = new RemoteController(app, that.launch.bind(that, that.thisComputer));
            }, function (failed) {
                ERR('Failed to launch');
                that.launch(that.thisComputer);
            })
        }
    };

    that.constructList = function () {
        that.itemsContainer.empty();
        that.thisComputer.node.click(that.launch.bind(that, that.thisComputer));
        that.itemsContainer.append(that.thisComputer.node);
        if (that.launchedDevice !== that.thisComputer) {
            that.launchedDevice.node.click(that.launch.bind(that, that.launchedDevice));
            that.itemsContainer.append(that.launchedDevice.node);
        }
        for (var i = 0; i < that.devices.length; ++i) {
            that.devices[i].node.click(that.launch.bind(that, that.devices[i]));
            that.itemsContainer.append(that.devices[i].node);
        }
        that.adjustSize();
    };

    that.adjustSize = function () {
        var panel = that.menuPopup.find('.ytp-panel');
        that.menuPopup.height(panel.height());
        that.menuPopup.width(panel.width());
    };

    that.show = function () {
        $(document).mouseup(that.focusOut);
        that.globalContainer.append(that.menuPopup);
        that.load();
    };

    that.hide = function () {
        $(document).off('mouseup', that.focusOut);
        that.menuPopup.detach();
    };

    that.focusOut = function (e) {
        if (!that.menuPopup.is(e.target) // if the target of the click isn't the container...
            && that.menuPopup.has(e.target).length === 0) // ... nor a descendant of the container
        {
            that.hide();
        }
    };

    that.menuPopup = $('<div class="ytp-popup ytp-settings-menu" >' +
        '<div class="ytp-panel">' +
        '<div class="ytp-panel-header">' +
        '<button class="ytp-button ytp-panel-title">Play using WebDial</button>' +
        '</div>' +
        '<div class="ytp-panel-menu" role="menu">' +
        '</div>' +
        '</div>' +
        '</div>');
    that.itemsContainer = that.menuPopup.find('.ytp-panel-menu');
    that.loadingPanel = $('<div class="ytp-menuitem">' +
        '<div class="ytp-menuitem-label">Loading...</div>' +
        '</div>');

    that.globalContainer = $('#movie_player');

    that.thisComputer = new MenuItem('This computer');
    that.thisComputer.check(true);
    that.devices = [];
    that.launchedDevice = that.thisComputer;
    that.constructList();
}

function RemoteController(app, endedCb) {
    var that = this;

    that.onStateChanged = function () {
        switch (that.app.connectionState) {
            case 'disconnected':
                LOG('End of connection');
                that.startLocalVideo();
                window.postMessage({webdialtag:'control', cmd:'unsupress'}, '*');
                endedCb();
                break;
            case 'opened':
                LOG('Applications are connected');
                /// TODO seek
                // var e = new YoutubeWebDialEvent('seek');
                // e.time = that.video.currentTime;
                // that.sendEvent(e);
                break;
            default:
                LOG('State changed to ' + that.app.connectionState);
        }
    };

    that.onMessage = function (message) {
        var event = JSON.parse(message.data);
        switch (event.type) {
            case 'playing':
                LOG('remote is playing');
                window.postMessage({webdialtag:'control', cmd:'play'}, '*');
                break;
            case 'paused':
                LOG('remote was paused');
                window.postMessage({webdialtag:'control', cmd:'pause'}, '*');
                break;
            case 'timeupdate':
                LOG('timeupdate to ' + event.time);
                window.postMessage({webdialtag:'control', cmd:'timeupdate', time:event.time}, '*');
                break;
            default:
                LOG('Received event ' + event.type);
        }
    };

    that.sendEvent = function (event) {
        LOG('Sending ' + event.type);
        if (that.app.connectionState == 'opened') {
            that.app.send(JSON.stringify(event));
        }
    };

    that.disconnect = function () {
        that.sendEvent(new YoutubeWebDialEvent('close'));
        that.app.disconnect();
    };

    that.stopLocalVideo = function () {
        that.video.pause();
        $('.ytp-remote').show();
    };

    that.startLocalVideo = function () {
        /// TODO restore video
        $('.ytp-remote').hide();
        that.video.play();
    };

    that.overrideVideoControls = function () {
        // that.video.real_play = that.video.play;
        // that.video.play = function () {
        //     that.sendEvent(new YoutubeWebDialEvent('play'));
        // };
        // that.video.real_pause = that.video.pause;
        // that.video.pause = function () {
        //     that.sendEvent(new YoutubeWebDialEvent('pause'));
        // };

        window.addEventListener('message', function(e) {
            if (e.data.webdialtag && e.data.webdialtag == 'video') {
                switch(e.data.cmd) {
                    case 'play':
                        console.log('video must be played');
                        that.sendEvent(new YoutubeWebDialEvent('play'));
                        break;
                    case 'pause':
                        console.log('video must be paused');
                        that.sendEvent(new YoutubeWebDialEvent('pause'));
                        break;
                    case 'seek':
                        console.log('seek to' + e.data.time);
                        var dialE = new YoutubeWebDialEvent('seek');
                        dialE.time = e.data.time;
                        that.sendEvent(dialE);
                        break;
                    default:
                        console.log('unknown ' + e.data.cmd);
                }
            }
        }, false);

        window.postMessage({webdialtag:'control', cmd:'inject'}, '*');

        // new HTMLMediaElementController(that.video);
        // that.video.hijackController.delegate = that;
        // that.video.hijackController.suppressCommands = true;

        // that.video.addEventListener('timeupdate', function () {
        //     var e = new YoutubeWebDialEvent('seek');
        //     e.time = that.video.currentTime;
        //     that.sendEvent(e);
        // });
    };

    that.pause = function () {
        that.sendEvent(new YoutubeWebDialEvent('pause'));
    };

    that.play = function () {
        that.sendEvent(new YoutubeWebDialEvent('play'));
    };

    that.app = app;
    that.endedCb = endedCb;
    var videoFound = $('video');
    if (videoFound.length == 0) {
        ERR('No video tag found!');
        return;
    }
    that.video = videoFound[0];

    that.app.addEventListener('statechange', that.onStateChanged);
    that.app.addEventListener('message', that.onMessage);

    that.overrideVideoControls();

    that.stopLocalVideo();

    window.postMessage({webdialtag:'control', cmd:'supress'}, '*');
}

function RemoteReceiver(app) {
    var that = this;

    that.onStateChanged = function () {
        switch (that.app.connectionState) {
            case 'disconnected':
                LOG('End of connection');
                // not exactly we want to do anything
                break;
            default:
                LOG('State changed to ' + that.app.connectionState);
        }
    };

    that.sendEvent = function (event) {
        LOG('Sending ' + event.type);
        if (that.app.connectionState == 'opened') {
            that.app.send(JSON.stringify(event));
        }
    };

    that.onMessage = function (message) {
        LOG('Got message');
        var event = JSON.parse(message.data);
        LOG('Received event ' + event.type);
        switch (event.type) {
            case 'seek':
                that.video.currentTime = event.time;
                break;
            case 'pause':
                that.video.pause();
                break;
            case 'play':
                that.video.play();
                break;
            case 'close':
                that.video.pause();
                break;
            default:
                LOG('Unknown message');
        }
    };

    that.app = app;
    that.video = $('video')[0];

    that.app.addEventListener('statechange', that.onStateChanged);
    that.app.addEventListener('message', that.onMessage);

    that.video.addEventListener('playing', function() {
        that.sendEvent(new YoutubeWebDialEvent('playing'));
    });
    that.video.addEventListener('timeupdate', function() {
        LOG('sending timeupdate ' + that.video.currentTime);
        var e = new YoutubeWebDialEvent('timeupdate');
        e.time = that.video.currentTime;
        that.sendEvent(e);
    });
    that.video.addEventListener('pause', function() {
        that.sendEvent(new YoutubeWebDialEvent('paused'));
    });
    LOG('everything set up');
}

});
