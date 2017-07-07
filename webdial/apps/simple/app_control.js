(function(window) {
    'use strict';

    var appCounter = 0;

    window.AppControl = function(app) {
        if (this instanceof AppControl) {
            this.app = app;
            this.container = document.createElement('div');

            // Title
            var nameLabel = this.container.appendChild(document.createElement('p'));
            nameLabel.textContent = 'App ' + appCounter++ + ' ';
            this.stateLabel = nameLabel.appendChild(document.createElement('span'));
            this.stateLabel.textContent = this.app.connectionState;
            app.addEventListener('statechange', this.onStateChange.bind(this));

            // Buttons
            var disconnectButton = this.container.appendChild(document.createElement('button'));
            disconnectButton.textContent = 'Disconnect';
            disconnectButton.addEventListener('click', this.disconnect.bind(this));
            var sendButton = this.container.appendChild(document.createElement('button'));
            sendButton.textContent = 'Send';
            sendButton.addEventListener('click', this.send.bind(this));
            this.messageInput = this.container.appendChild(document.createElement('input'));
            this.messageInput.type = 'text';
            this.messageInput.value = 'Hello!';

            this.container.appendChild(document.createElement('br'));

            // Logger
            this.loggingElement = this.container.appendChild(document.createElement('p'));
            this.loggingElement.setAttribute('style', 'white-space: pre-line;');

            app.addEventListener('message', this.onMessageReceived.bind(this));
        } else {
            return new AppControl(app);
        }
    };

    AppControl.prototype.send = function() {
        var message = this.messageInput.value;
        this.log('Sending "' + message + '"');
        try {
            this.app.send(message);
        } catch(exception) {
            this.log('Exception: ' + exception.message);
        }
    };

    AppControl.prototype.disconnect = function() {
        this.log('Disconnecting');
        try {
            this.app.disconnect();
        } catch(exception) {
            this.log('Exception: ' + exception.message);
        }

    };

    AppControl.prototype.onMessageReceived = function(message) {
       this.log('Message received "' + message.data + '"');
    };

    AppControl.prototype.onStateChange = function() {
        this.stateLabel.textContent = this.app.connectionState;
        this.log('Changed state to ' + this.app.connectionState);
    };

    AppControl.prototype.log = function(message) {
        this.loggingElement.textContent += Date.now() + ' - ' + message + '\n';
    }
})(this);

