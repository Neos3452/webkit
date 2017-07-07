(function(){
    'use strict';

    var DEBUG = 1;

    function LOG(msg) { if (DEBUG) console.log(msg); }
    function WARN(msg) { if (DEBUG) console.warn(msg); }
    function ERR(msg) { console.error(msg); }

    LOG('Begin injection');

    function HTMLMediaElementController(mediaElement) {
        LOG('injected!');
        this.mediaElement = mediaElement;
        this.mediaElement.hijackController = this;
        this.suppressCommands = false;
        // this.delegate = null;
        this.paused = true;
        this.time = this.mediaElement.currentTime;
        mediaElement.addEventListener('playing', function () {
            LOG('play...');
            // if (!this.suppressCommands) {
            //     this.paused = false;
            // }
            // this.onPlay();
            this.paused = false;
        }.bind(this));
        mediaElement.addEventListener('pause', function () {
            LOG('pause..');
            // if (!this.suppressCommands) {
            //     this.paused = true;
            // }
            // this.onPause();
            this.paused = true;
        }.bind(this));
        mediaElement.addEventListener('timeupdate', function () {
            if (!this.suppressCommands)
                this.time = this.mediaElement.currentTime;
        }.bind(this));
    }

    HTMLMediaElementController.prototype.onPause = function() {
        // if (this.delegate)
        // this.delegate.pause();
        window.postMessage({webdialtag:'video', cmd:'pause'}, '*');
        return !this.suppressCommands;
    };

    HTMLMediaElementController.prototype.onPlay = function() {
        // if (this.delegate)
        // this.delegate.play();
        window.postMessage({webdialtag:'video', cmd:'play'}, '*');
        return !this.suppressCommands;
    };
    HTMLMediaElementController.prototype.onSeek = function(time) {
        this.setTime(time);
        window.postMessage({webdialtag:'video', cmd:'seek', time:time}, '*');
    };

    HTMLMediaElementController.prototype.pause = function() {
        LOG('synthetic pause');
        this.paused = true;
        if (!this.suppressCommands) {
            this.mediaElement.pause();
        } else {
            this.mediaElement.dispatchEvent(new Event('pause'));
        }
    };

    HTMLMediaElementController.prototype.play = function() {
        LOG('synthetic play');
        this.paused = false;
        if (!this.suppressCommands) {
            this.mediaElement.play();
        } else {
            this.mediaElement.dispatchEvent(new Event('playing'));
        }
    };

    HTMLMediaElementController.prototype.setTime = function(time) {
        if (!this.suppressCommands) {
            LOG('cannot set time while not supressing');
        } else {
            this.time = time;
            this.mediaElement.dispatchEvent(new Event('timeupdate'));
        }
    };

    HTMLMediaElementController.prototype.orgPaused = HTMLMediaElement.prototype.__lookupGetter__('paused');
    HTMLMediaElement.prototype.__defineGetter__('paused', function() {
        var orgState = HTMLMediaElementController.prototype.orgPaused.call(this);
        if (this.hijackController) {
            if (orgState != this.hijackController.paused)
                LOG('Org state ' + orgState + ' returning ' + this.hijackController.paused);
            return this.hijackController.paused;
        }
        return orgState;
    });

    HTMLMediaElementController.prototype.orgCurrentTime = HTMLMediaElement.prototype.__lookupGetter__('currentTime');
    HTMLMediaElement.prototype.__defineGetter__('currentTime', function() {
        if (this.hijackController && this.hijackController.suppressCommands) {
            return this.hijackController.time;
        }
        return HTMLMediaElementController.prototype.orgCurrentTime.call(this);
    });

    HTMLMediaElementController.prototype.orgCurrentTimeSet = HTMLMediaElement.prototype.__lookupSetter__('currentTime');
    HTMLMediaElement.prototype.__defineSetter__('currentTime', function(time) {
        if (this.hijackController && this.hijackController.suppressCommands) {
            LOG('App seeked to ' + time);
            this.hijackController.onSeek(time);
        }
        HTMLMediaElementController.prototype.orgCurrentTimeSet.call(this, time);
    });

    HTMLMediaElementController.prototype.orgPlay = HTMLMediaElement.prototype.play;
    HTMLMediaElement.prototype.play = function() {
        if (this.hijackController) {
            LOG('hijacking play');
            if (!this.hijackController.onPlay()) {
                return;
            }
        }
        HTMLMediaElementController.prototype.orgPlay.call(this);
    };

    HTMLMediaElementController.prototype.orgPause = HTMLMediaElement.prototype.pause;
    HTMLMediaElement.prototype.pause = function() {
        if (this.hijackController) {
            LOG('hijacking pause');
            if (!this.hijackController.onPause()) {
                return;
            }
        }
        HTMLMediaElementController.prototype.orgPause.call(this);
    };

// var orgCtr = HTMLMediaElement.prototype.constructor;
// HTMLMediaElement.prototype.constructor = function() {
// 	orgCtr.call(this, arguments);
// 	this.hijackController = new HTMLMediaElementController(this);
// 	LOG('injection complete');
// };

window.addEventListener('message', function(e) {
	if (e.data.webdialtag && e.data.webdialtag == 'control') {
        var video = document.querySelector('video');
        if (video) {
        switch (e.data.cmd) {
            case 'inject':
                video.hijackController = new HTMLMediaElementController(video);
                break;
            case 'supress':
                video.hijackController.suppressCommands = true;
                break;
            case 'unsupress':
                video.hijackController.suppressCommands = false;
                video.currentTime = video.hijackController.time;
                break;
            case 'play':
                video.hijackController.play();
                break;
            case 'pause':
                video.hijackController.pause();
                break;
            case 'timeupdate':
                video.hijackController.setTime(e.data.time);
                break;
            default:
                LOG('unrecognized cmd ' + e.data.cmd);
        }
        } else {
            LOG('No video element');
        }
	}
}, false);

// window.HTMLMediaElementController = HTMLMediaElementController;
    LOG('setup complete');
})();