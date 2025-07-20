// @ts-check

class WebsocketSession {
    url;
    #socket;
    #msgID;
    #inFlight;
    #pendingAtts;

    /**
     * @param {string} url - The WebSocket server URL
     */
    constructor(url) {
        this.url = url;
        this.#msgID = 1;
        this.#inFlight = {};
        this.#pendingAtts = [];
    }

    /**
     * Opens the WebSocket connection
     * @returns {Promise<WebsocketSession>} A promise that resolves when the connection is established
     */
    async open() {
        const socket = new WebSocket(this.url);

        socket.onclose = (event) => {
            console.log('WebSocket disconnected: ', event);
        };

        socket.onerror = (error) => {
            console.error('WebSocket error:', error);
        };

        socket.onmessage = (event) => {
            this.#handle(event);
        };

        return new Promise(resolve => {
            socket.onopen = () => {
                console.log('WebSocket connected: ', this.url);
                this.#socket = socket;
                resolve(this);
            };
        });
    }

    /**
     * Closes the WebSocket connection
     */
    close() {
        if (this.#socket) {
            console.log('WebSocket disconnecting: ', this.#socket.url);
            this.#socket.close(1000, 'manual');
        }
    }

    /**
     * Makes a remote procedure call over WebSocket
     * @param {string} method - The RPC method name
     * @param {Object} [params={}] - The parameters for the RPC method
     * @returns {Promise<any>} A promise that resolves with the RPC result
     * @throws {Error} If the WebSocket is not ready
     */
    async call(method, params = {}) {
        if (!this.#socket || this.#socket.readyState !== WebSocket.OPEN) {
            throw new Error('WebSocket not ready');
        }
        const id = `${this.#msgID++}`;
        const msg = JSON.stringify({ id, method, params });
        const promise = new Promise((resolve, reject) => {
            this.#inFlight[id] = { resolve, reject };
        });
        this.#socket.send(msg);
        return promise;
    }

    /**
     * Handles incoming WebSocket messages
     * @param {MessageEvent} event - The message event
     */
    #handle(event) {
        if (event.data instanceof ArrayBuffer || event.data instanceof Blob) {
            this.#pendingAtts.push(event.data);
            return;
        }

        let payload;
        try {
            payload = JSON.parse(event.data);
        } catch (e) {
            console.error('Malformed message: ', event.data);
        }
        if (!payload || !payload.id) return;

        payload.attachments = this.#pendingAtts.reverse();
        this.#pendingAtts = [];

        const promise = this.#inFlight[payload.id];
        if (promise) {
            delete this.#inFlight[payload.id];
        } else {
            console.error('Orphan message: ', payload);
            return;
        }

        if (payload.error) {
            console.error('Error message: ', payload);
            promise.reject(payload.error);
            return;
        }

        if (payload.result) {
            promise.resolve([payload.result, ...payload.attachments]);
            return;
        }

        console.error('Unknown message: ', payload);
    }
}

export default WebsocketSession;